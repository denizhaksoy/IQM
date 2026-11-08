#include "fft128_single_gpu.h"
#include <IQM/base/vulkan_runtime.h>
#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <fstream>
#include <cstring>
#include <cmath>
#include <stdexcept>
namespace IQM::HDRVDP3::Experimental {
std::vector<float> fft128_gpu(const IQM::Bin::VulkanInstance& instance,const std::string& shader,std::span<const float> values,bool forward){
 using IQM::GPU::VulkanRuntime;constexpr uint32_t pixels=128*128,bytes=pixels*8;
 if(values.size()!=(forward?pixels:2*pixels))throw std::invalid_argument("FFT128 input shape");for(float v:values)if(!std::isfinite(v))throw std::invalid_argument("FFT128 finite domain");
 std::vector<float> input(2*pixels);if(forward){for(size_t i=0;i<pixels;++i)input[2*i]=values[i];}else input.assign(values.begin(),values.end());
 std::ifstream file(shader,std::ios::binary|std::ios::ate);auto length=file.tellg();if(length<=0||length%4)throw std::runtime_error("FFT128 shader");std::vector<uint32_t> code(size_t(length)/4);file.seekg(0);file.read(reinterpret_cast<char*>(code.data()),length);if(!file||code[0]!=0x07230203u)throw std::runtime_error("FFT128 SPIRV");
 const auto& device=*instance.device();auto module=VulkanRuntime::createShaderModule(device,code);auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(16));auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
 auto allocate=[&](){auto b=IQM::Bin::VulkanResource::createBuffer(device,*instance.physicalDevice(),bytes,vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);b.first.bindMemory(b.second,0);return b;};auto src=allocate(),temp=allocate(),dst=allocate();void* mapped=src.second.mapMemory(0,bytes);std::memcpy(mapped,input.data(),bytes);src.second.unmapMemory();
 auto pool=VulkanRuntime::createDescPool(device,2,{{vk::DescriptorType::eStorageBuffer,4}});const vk::DescriptorSetLayout raw[]={*setLayout,*setLayout};auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=2,.pSetLayouts=raw});const std::vector<vk::DescriptorBufferInfo> a{{*src.first,0,bytes}},b{{*temp.first,0,bytes}},c{{*dst.first,0,bytes}};device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,a),VulkanRuntime::createWriteSet(*sets[0],1,b),VulkanRuntime::createWriteSet(*sets[1],0,b),VulkanRuntime::createWriteSet(*sets[1],1,c)},{});
 instance.cmdBuf()->reset();instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);
 struct Push{uint32_t n,batches,graph,fused;};Push push{128,128,forward?8u:3u,0};instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,push);instance.cmdBuf()->dispatch(4,1,1);
 vk::MemoryBarrier between{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader,{}, {between},{},{});push.graph=forward?9:4;push.batches=forward?65:128;instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[1]},{});instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,push);instance.cmdBuf()->dispatch((push.batches+31)/32,1,1);
 vk::MemoryBarrier after{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eHost,{}, {after},{},{});instance.cmdBuf()->end();vk::CommandBuffer command=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&command},fence);instance.waitForFence(fence);
 std::vector<float> output(2*pixels);mapped=dst.second.mapMemory(0,bytes);std::memcpy(output.data(),mapped,bytes);dst.second.unmapMemory();instance.cmdBuf()->reset();return output;
}
}
