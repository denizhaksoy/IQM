#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <IQM/base/vulkan_runtime.h>
#include <bit>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace GPUChain { namespace fs=std::filesystem;
using IQM::GPU::VulkanRuntime;
struct Buffer{vk::raii::DeviceMemory memory=nullptr;vk::raii::Buffer handle=nullptr;};
Buffer buffer(const IQM::Bin::VulkanInstance& instance,unsigned bytes){auto pair=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),bytes,vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);Buffer b;b.handle=std::move(pair.first);b.memory=std::move(pair.second);b.handle.bindMemory(b.memory,0);return b;}
template<class T>std::vector<T> read(const fs::path&p,size_t n){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()!=std::streamoff(n*sizeof(T)))throw std::runtime_error("Input shape "+p.string());std::vector<T>v(n);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),n*sizeof(T));return v;}
struct Result{std::vector<double> real,spectrum,cropped;};
Result execute(const std::vector<double>& padded,const std::vector<double>& filters,const std::array<fs::path,6>& shaders,const fs::path& out){
 constexpr size_t count=36864,words=count*2;constexpr unsigned bytes=words*8;
 if(padded.size()!=count||filters.size()!=count)throw std::runtime_error("GPU convolution shape");
 std::vector<double> input;input.reserve(words);for(double v:padded){input.push_back(v);input.push_back(0.);} static IQM::Bin::VulkanInstance instance(true,"NVIDIA",true);const auto& device=*instance.device();
 const auto properties=instance.physicalDevice()->getProperties2<vk::PhysicalDeviceProperties2,vk::PhysicalDeviceFloatControlsProperties>();const auto& controls=properties.get<vk::PhysicalDeviceFloatControlsProperties>();if(!controls.shaderRoundingModeRTEFloat64||!controls.shaderSignedZeroInfNanPreserveFloat64)throw std::runtime_error("Required controls unavailable");
 std::ofstream(out/"device.txt")<<instance.selectedDevice<<" RTE64="<<controls.shaderRoundingModeRTEFloat64<<" signedZero64="<<controls.shaderSignedZeroInfNanPreserveFloat64<<" denorm64="<<controls.shaderDenormPreserveFloat64<<'\n';
 auto layoutSet=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
 auto layout=VulkanRuntime::createPipelineLayout(device,{*layoutSet},VulkanRuntime::createPushConstantRange(16));
 std::vector<vk::raii::ShaderModule> modules;std::vector<vk::raii::Pipeline> pipelines;
 for(int i=0;i<6;++i){std::ifstream f(shaders[i],std::ios::binary|std::ios::ate);auto n=f.tellg();if(n<=0||n%4)throw std::runtime_error("Invalid shader");std::vector<uint32_t> code(size_t(n)/4);f.seekg(0);f.read(reinterpret_cast<char*>(code.data()),n);modules.push_back(VulkanRuntime::createShaderModule(device,code));pipelines.push_back(VulkanRuntime::createComputePipeline(device,modules.back(),layout));}
 std::vector<Buffer> buffers;for(int i=0;i<7;++i)buffers.push_back(buffer(instance,bytes));auto filter=buffer(instance,unsigned(filters.size()*8));
 auto upload=[](Buffer& b,const std::vector<double>& v){auto p=b.memory.mapMemory(0,v.size()*8);std::memcpy(p,v.data(),v.size()*8);b.memory.unmapMemory();};upload(buffers[0],input);upload(filter,filters);
 auto pool=VulkanRuntime::createDescPool(device,6,{{vk::DescriptorType::eStorageBuffer,18}});std::vector<vk::DescriptorSetLayout> layouts(6,*layoutSet);auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=6,.pSetLayouts=layouts.data()});
 for(unsigned i=0;i<6;++i){std::vector<vk::DescriptorBufferInfo> a{{*buffers[i].handle,0,bytes}},b{{*buffers[i+1].handle,0,bytes}},c{{*filter.handle,0,filters.size()*8}};device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[i],0,a),VulkanRuntime::createWriteSet(*sets[i],1,b),VulkanRuntime::createWriteSet(*sets[i],2,c)},{});}
 auto cmd=instance.cmdBuf();cmd->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});vk::MemoryBarrier host{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};cmd->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {host},{},{});
 for(unsigned i=0;i<6;++i){std::array<uint32_t,4> push{192,(i==0||i==3)?192u:1u,0,0};const unsigned dispatch[]={6,7,576,6,12,144};cmd->bindPipeline(vk::PipelineBindPoint::eCompute,*pipelines[i]);cmd->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[i]},{});cmd->pushConstants(*layout,vk::ShaderStageFlagBits::eCompute,0,16,push.data());cmd->dispatch(dispatch[i],1,1);vk::MemoryBarrier barrier{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};cmd->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader,{}, {barrier},{},{});}
 vk::MemoryBarrier end{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};cmd->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eHost,{}, {end},{},{});cmd->end();const vk::CommandBuffer raw=**cmd;vk::SubmitInfo submit{.commandBufferCount=1,.pCommandBuffers=&raw};vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(submit,fence);instance.waitForFence(fence);
 Result result;auto inverse=static_cast<const double*>(buffers[5].memory.mapMemory(0,bytes));result.real.resize(count);for(size_t i=0;i<count;++i)result.real[i]=inverse[2*i];buffers[5].memory.unmapMemory();
 auto spectrum=static_cast<const double*>(buffers[2].memory.mapMemory(0,bytes));result.spectrum.assign(spectrum,spectrum+words);buffers[2].memory.unmapMemory();
 auto cropped=static_cast<const double*>(buffers[6].memory.mapMemory(0,bytes));result.cropped.resize(9216);for(size_t i=0;i<9216;++i)result.cropped[i]=cropped[2*i];buffers[6].memory.unmapMemory();
 cmd->reset();return result;
}
std::vector<double> baseFFT(const std::vector<double>& input,const fs::path& shader){
 if(input.size()!=288)throw std::runtime_error("Base12 input shape");
 IQM::Bin::VulkanInstance instance(true,"NVIDIA",true);const auto& device=*instance.device();constexpr unsigned bytes=288*8;
 auto source=buffer(instance,bytes),temporary=buffer(instance,bytes),output=buffer(instance,bytes);auto ptr=source.memory.mapMemory(0,bytes);std::memcpy(ptr,input.data(),bytes);source.memory.unmapMemory();
 std::ifstream f(shader,std::ios::binary|std::ios::ate);auto n=f.tellg();if(n<=0||n%4)throw std::runtime_error("Invalid base12 shader");std::vector<uint32_t> code(size_t(n)/4);f.seekg(0);f.read(reinterpret_cast<char*>(code.data()),n);
 auto module=VulkanRuntime::createShaderModule(device,code);auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(16));auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
 auto pool=VulkanRuntime::createDescPool(device,3,{{vk::DescriptorType::eStorageBuffer,6}});const vk::DescriptorSetLayout layouts[]{*setLayout,*setLayout,*setLayout};auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=3,.pSetLayouts=layouts});
 std::vector<vk::DescriptorBufferInfo>a{{*source.handle,0,bytes}},b{{*temporary.handle,0,bytes}},c{{*output.handle,0,bytes}};device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,a),VulkanRuntime::createWriteSet(*sets[0],1,b),VulkanRuntime::createWriteSet(*sets[1],0,b),VulkanRuntime::createWriteSet(*sets[1],1,c),VulkanRuntime::createWriteSet(*sets[2],0,c),VulkanRuntime::createWriteSet(*sets[2],1,c)},{});
 auto cmd=instance.cmdBuf();cmd->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});vk::MemoryBarrier host{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};cmd->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {host},{},{});cmd->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);
 for(unsigned stage=0;stage<3;++stage){std::array<uint32_t,4> push{stage,1,0,0};cmd->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[stage]},{});cmd->pushConstants(*layout,vk::ShaderStageFlagBits::eCompute,0,16,push.data());cmd->dispatch(1,1,1);vk::MemoryBarrier barrier{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eShaderWrite};cmd->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader,{}, {barrier},{},{});}
 vk::MemoryBarrier end{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};cmd->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eHost,{}, {end},{},{});cmd->end();const vk::CommandBuffer raw=**cmd;vk::SubmitInfo submit{.commandBufferCount=1,.pCommandBuffers=&raw};vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(submit,fence);instance.waitForFence(fence);
 auto actual=static_cast<const double*>(output.memory.mapMemory(0,bytes));std::vector<double> result(actual,actual+288);output.memory.unmapMemory();return result;
}
}