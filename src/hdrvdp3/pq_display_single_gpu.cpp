#include <IQM/hdrvdp3/pq_display_single_gpu.h>
#include <IQM/hdrvdp3/pq_single.h>
#include <IQM/base/vulkan_runtime.h>
#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
namespace IQM::HDRVDP3 {
GpuFloatBuffer pq_to_display_single_gpu(const IQM::VulkanInstance& instance,
 const std::string& shader,std::span<const float> encoded,const PQDisplaySingle& display,bool rawU16Codes){
 using IQM::GPU::VulkanRuntime;
 if(encoded.empty()||encoded.size()>3840u*2160u*3u)throw std::invalid_argument("PQ GPU input geometry");
  const float inputLimit=rawU16Codes?65535.f:1.f;
 for(float v:encoded)
     if(!std::isfinite(v)||v<0.f||v>inputLimit)
         throw std::invalid_argument("Invalid PQ input range");
 if(!std::isfinite(display.ambient_lux)||display.ambient_lux<0||!std::isfinite(display.reflectivity)||display.reflectivity<0||display.reflectivity>1||!std::isfinite(display.peak_cd_m2)||display.peak_cd_m2<=0)
  throw std::invalid_argument("Invalid explicit PQ display parameters");
 const double reflected=display.ambient_lux*display.reflectivity/3.14159265358979323846;
 const double cap=display.peak_cd_m2+reflected;
 if(!std::isfinite(cap)||cap>std::numeric_limits<float>::max())throw std::invalid_argument("PQ display limit exceeds Single range");
 std::ifstream file(shader,std::ios::binary|std::ios::ate);auto length=file.tellg();
 if(length<=0||length%4)throw std::runtime_error("PQ GPU shader");
 std::vector<uint32_t> code(size_t(length)/4);file.seekg(0);file.read(reinterpret_cast<char*>(code.data()),length);
 if(!file||code[0]!=0x07230203u)throw std::runtime_error("PQ GPU SPIR-V");
 const auto& device=*instance.device();const auto limits=instance.physicalDevice()->getProperties().limits;
 const uint32_t count=uint32_t(encoded.size()),bytes=count*4;
 if(bytes>limits.maxStorageBufferRange)throw std::runtime_error("PQ GPU storage limits");
 auto module=VulkanRuntime::createShaderModule(device,code);
 auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
 auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(16));
 auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
 auto allocate=[&](unsigned size,vk::BufferUsageFlags usage,vk::MemoryPropertyFlags props){
  auto b=IQM::Bin::VulkanResource::createBuffer(device,*instance.physicalDevice(),size,usage,props);b.first.bindMemory(b.second,0);return b;};
 auto in=allocate(bytes,vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
 auto out=allocate(bytes,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc,vk::MemoryPropertyFlagBits::eDeviceLocal);
 auto stagingOut=allocate(bytes,vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
 void* p=in.second.mapMemory(0,bytes);std::memcpy(p,encoded.data(),bytes);in.second.unmapMemory();
 auto pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,2}});
 const vk::DescriptorSetLayout raw=*setLayout;
 auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});
 const std::vector<vk::DescriptorBufferInfo> a{{*in.first,0,bytes}},b{{*out.first,0,bytes}};
 device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,a),VulkanRuntime::createWriteSet(*sets[0],1,b)},{});
 instance.cmdBuf()->reset();instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
 vk::MemoryBarrier hostToShader{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
 instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {hostToShader},{},{});
 instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);
 instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});
 struct Push{uint32_t count;float ambient;float limit;uint32_t pad;};
 instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,Push{count,float(reflected),float(cap),rawU16Codes?1u:0u});
 instance.cmdBuf()->dispatch((count+255)/256,1,1);
 vk::MemoryBarrier shaderToTransfer{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eTransferRead};
 instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eTransfer,{}, {shaderToTransfer},{},{});
 // Optional readback is done by caller path; keep output resident. Staging buffer is allocated for test readback helper.
 instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});
 instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);
 instance.cmdBuf()->reset();
 (void)stagingOut;
 return GpuFloatBuffer{std::move(out.first),std::move(out.second),bytes,count};
}
GpuFloatBuffer pq_to_display_single_gpu_resident(const IQM::VulkanInstance& instance,
 const std::string& shader,const GpuFloatBuffer& encoded,const PQDisplaySingle& display){
 using IQM::GPU::VulkanRuntime;
 if(!*encoded.buffer ||
    encoded.count==0 ||
    encoded.count>3840u*2160u*3u ||
    encoded.bytes!=encoded.count*sizeof(float))
    throw std::invalid_argument("Resident PQ GPU input geometry");
if(!std::isfinite(display.ambient_lux)||display.ambient_lux<0||!std::isfinite(display.reflectivity)||display.reflectivity<0||display.reflectivity>1||!std::isfinite(display.peak_cd_m2)||display.peak_cd_m2<=0)
  throw std::invalid_argument("Invalid explicit PQ display parameters");
 const double reflected=display.ambient_lux*display.reflectivity/3.14159265358979323846;
 const double cap=display.peak_cd_m2+reflected;
 if(!std::isfinite(cap)||cap>std::numeric_limits<float>::max())throw std::invalid_argument("PQ display limit exceeds Single range");
 std::ifstream file(shader,std::ios::binary|std::ios::ate);auto length=file.tellg();
 if(length<=0||length%4)throw std::runtime_error("PQ GPU shader");
 std::vector<uint32_t> code(size_t(length)/4);file.seekg(0);file.read(reinterpret_cast<char*>(code.data()),length);
 if(!file||code[0]!=0x07230203u)throw std::runtime_error("PQ GPU SPIR-V");
 const auto& device=*instance.device();const auto limits=instance.physicalDevice()->getProperties().limits;
 const uint32_t count=uint32_t(encoded.count),bytes=count*4;
 if(bytes>limits.maxStorageBufferRange)throw std::runtime_error("PQ GPU storage limits");
 auto module=VulkanRuntime::createShaderModule(device,code);
 auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
 auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(16));
 auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
 auto allocate=[&](unsigned size,vk::BufferUsageFlags usage,vk::MemoryPropertyFlags props){
  auto b=IQM::Bin::VulkanResource::createBuffer(device,*instance.physicalDevice(),size,usage,props);b.first.bindMemory(b.second,0);return b;};

 auto out=allocate(bytes,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc,vk::MemoryPropertyFlagBits::eDeviceLocal);


 auto pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,2}});
 const vk::DescriptorSetLayout raw=*setLayout;
 auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});
 const std::vector<vk::DescriptorBufferInfo> a{{*encoded.buffer,0,bytes}},b{{*out.first,0,bytes}};
 device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,a),VulkanRuntime::createWriteSet(*sets[0],1,b)},{});
 instance.cmdBuf()->reset();instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
 vk::MemoryBarrier hostToShader{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
 instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader,{}, {hostToShader},{},{});
 instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);
 instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});
 struct Push{uint32_t count;float ambient;float limit;uint32_t pad;};
 instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,Push{count,float(reflected),float(cap),0u});
 instance.cmdBuf()->dispatch((count+255)/256,1,1);
 vk::MemoryBarrier shaderToTransfer{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eTransferRead|vk::AccessFlagBits::eShaderRead};
 instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eTransfer|vk::PipelineStageFlagBits::eComputeShader,{}, {shaderToTransfer},{},{});
 // Device-resident output for subsequent compute stages.
 instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});
 instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);
 instance.cmdBuf()->reset();

 return GpuFloatBuffer{std::move(out.first),std::move(out.second),bytes,count};
}

std::vector<float> pq_to_display_single_gpu_readback(const IQM::VulkanInstance& instance,
 const std::string& shader,std::span<const float> encoded,const PQDisplaySingle& display){
 using IQM::GPU::VulkanRuntime;
 // Reuse production compute by running full function logic in-place with readback; simple/explicit for parity test.
 auto buf=pq_to_display_single_gpu(instance,shader,encoded,display);
 const auto& device=*instance.device();auto staging=IQM::Bin::VulkanResource::createBuffer(device,*instance.physicalDevice(),buf.bytes,vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);staging.first.bindMemory(staging.second,0);
 instance.cmdBuf()->reset();instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
 vk::BufferCopy copy{.srcOffset=0,.dstOffset=0,.size=buf.bytes};
 instance.cmdBuf()->copyBuffer(*buf.buffer,*staging.first,{copy});
 vk::MemoryBarrier toHost{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};
 instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{}, {toHost},{},{});
 instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});
 instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);
 std::vector<float> result(buf.count);void* p=staging.second.mapMemory(0,buf.bytes);std::memcpy(result.data(),p,buf.bytes);staging.second.unmapMemory();instance.cmdBuf()->reset();return result;
}
}
