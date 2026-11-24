#include <IQM/hdrvdp3/adaptation_input_single_gpu_device.h>
#include <IQM/base/vulkan_runtime.h>
#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <cstring>
namespace IQM::HDRVDP3 {
GpuFloatBuffer adaptation_input_single_gpu_device(const IQM::VulkanInstance& instance,
    const std::string& shader, vk::Buffer lmsrClamped, unsigned pixelCount){
  using IQM::GPU::VulkanRuntime;
  if(!lmsrClamped||!pixelCount||pixelCount>3840u*2160u) throw std::invalid_argument("adapt input gpu device: bad input");
  const uint32_t n=pixelCount, bytes=n*4;
  const auto limits=instance.physicalDevice()->getProperties().limits;
  const uint32_t total=(n+63)/64,gx=std::min(total,limits.maxComputeWorkGroupCount[0]),gy=(total+gx-1)/gx;
  std::ifstream file(shader,std::ios::binary|std::ios::ate); auto len=file.tellg(); if(len<=0||len%4) throw std::runtime_error("adapt input shader"); std::vector<uint32_t> code(size_t(len)/4); file.seekg(0); file.read(reinterpret_cast<char*>(code.data()),len); if(!file||code[0]!=0x07230203u) throw std::runtime_error("adapt input SPIR-V");
  const auto& device=*instance.device(); auto module=VulkanRuntime::createShaderModule(device,code);
  auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
  auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(4));
  auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
  auto out=IQM::Bin::VulkanResource::createBuffer(device,*instance.physicalDevice(),bytes,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eDeviceLocal); out.first.bindMemory(out.second,0);
  auto pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,2}});
  const vk::DescriptorSetLayout raw=*setLayout; auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});
  const std::vector<vk::DescriptorBufferInfo> a{{lmsrClamped,0,vk::DeviceSize(bytes)*4}},c{{*out.first,0,bytes}};
  device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,a),VulkanRuntime::createWriteSet(*sets[0],1,c)},{});
  instance.cmdBuf()->reset(); instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  vk::BufferMemoryBarrier inReady{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=lmsrClamped,.offset=0,.size=vk::DeviceSize(bytes)*4};
  instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader,{},{},{inReady},{});
  instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);
  instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});
  instance.cmdBuf()->pushConstants<uint32_t>(*layout,vk::ShaderStageFlagBits::eCompute,0,n);
  instance.cmdBuf()->dispatch(gx,gy,1);
  vk::BufferMemoryBarrier outReady{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eTransferRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=*out.first,.offset=0,.size=bytes};
  instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eTransfer,{},{},{outReady},{});
  instance.cmdBuf()->end(); vk::CommandBuffer cmd=*instance.cmdBuf(); vk::raii::Fence fence(device,vk::FenceCreateInfo{}); instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence); instance.waitForFence(fence); instance.cmdBuf()->reset();
  return GpuFloatBuffer{std::move(out.first),std::move(out.second),bytes,n};
}
}
