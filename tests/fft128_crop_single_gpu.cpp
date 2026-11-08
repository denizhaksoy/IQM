#include "fft_filter_single_gpu.h"
#include <IQM/base/vulkan_runtime.h>
#include <IQM/hdrvdp3/map_gpu.h>
#include <vulkan_res.h>
#include <vulkan_instance.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>
namespace IQM::HDRVDP3 {
std::vector<float> fft128_crop_single_gpu(const IQM::Bin::VulkanInstance& instance,
 const std::string& shader,std::span<const float> reference,unsigned width,unsigned height){
 using IQM::GPU::VulkanRuntime;
 if(!width||!height||width>128||height>128||reference.size()!=2*128*128)throw std::invalid_argument("FFT128 crop geometry");
 for(float v:reference)if(!std::isfinite(v))throw std::invalid_argument("FFT128 crop finite domain");
 const auto limits=instance.physicalDevice()->getProperties().limits;
 const uint32_t n=width*height,bytes=n*4,inputBytes=uint32_t(reference.size()*4);
 const uint32_t total=(n+63)/64,gx=std::min(total,limits.maxComputeWorkGroupCount[0]),gy=(total+gx-1)/gx;
 if(inputBytes>limits.maxStorageBufferRange||gy>limits.maxComputeWorkGroupCount[1])throw std::runtime_error("FFT filter device limits");
 std::ifstream file(shader,std::ios::binary|std::ios::ate);auto length=file.tellg();
 if(length<=0||length%4)throw std::runtime_error("FFT filter shader");
 std::vector<uint32_t> code(size_t(length)/4);file.seekg(0);file.read(reinterpret_cast<char*>(code.data()),length);
 if(!file||code[0]!=0x07230203u)throw std::runtime_error("FFT filter SPIR-V");
 const auto& device=*instance.device();auto module=VulkanRuntime::createShaderModule(device,code);
 auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
 auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(16));
 auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
 auto allocate=[&](unsigned size){auto b=IQM::Bin::VulkanResource::createBuffer(device,*instance.physicalDevice(),size,vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);b.first.bindMemory(b.second,0);return b;};
 auto ref=allocate(inputBytes),dis=allocate(4),out=allocate(bytes);
 auto upload=[&](auto& b,std::span<const float> values){void* p=b.second.mapMemory(0,inputBytes);std::memcpy(p,values.data(),inputBytes);b.second.unmapMemory();};
 upload(ref,reference);
 auto pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,3}});
 const vk::DescriptorSetLayout raw=*setLayout;
 auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});
 const std::vector<vk::DescriptorBufferInfo> a{{*ref.first,0,inputBytes}},b{{*dis.first,0,4}},c{{*out.first,0,bytes}};
 device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,a),VulkanRuntime::createWriteSet(*sets[0],1,b),VulkanRuntime::createWriteSet(*sets[0],2,c)},{});
 instance.cmdBuf()->reset();instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
 vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
 instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
 instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);
 instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});
 struct Push{uint32_t count,width,height,unused;};instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,Push{n,width,height,0});
 instance.cmdBuf()->dispatch(gx,gy,1);
 vk::MemoryBarrier after{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};
 instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eHost,{}, {after},{},{});
 instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});
 instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);
 std::vector<float> result(n);void* p=out.second.mapMemory(0,bytes);std::memcpy(result.data(),p,bytes);out.second.unmapMemory();instance.cmdBuf()->reset();return result;
}
}
