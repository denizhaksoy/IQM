#include <IQM/hdrvdp3/projection_single_gpu.h>
#include <IQM/hdrvdp3/pq_display_single_gpu.h>
// Synchronous Vulkan matrix kernel; no image arithmetic on host.
#include <IQM/hdrvdp3/pathway_masking.h>
#include <IQM/hdrvdp3/map_gpu.h>
#include <IQM/base/vulkan_runtime.h>
#include <vulkan_res.h>
#include <vulkan.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>
namespace {
static std::vector<float> runFloatKernel2D(const IQM::VulkanInstance& instance,const std::string& shader,const std::vector<float>& input,size_t outputCount,uint32_t groups,uint32_t count,uint32_t floorMode=0){
    using IQM::GPU::VulkanRuntime;const auto& device=*instance.device();
    std::ifstream file(shader,std::ios::binary|std::ios::ate);const auto length=file.tellg();if(length<=0||length%4)throw std::runtime_error("Bad product shader");std::vector<uint32_t> code(size_t(length)/4);file.seekg(0);file.read(reinterpret_cast<char*>(code.data()),length);if(!file||code[0]!=0x07230203u)throw std::runtime_error("Bad SPIR-V");
    auto module=VulkanRuntime::createShaderModule(device,code);auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(16));auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
    const unsigned bytes=unsigned(outputCount*4),inputBytes=unsigned(input.size()*4);auto src=IQM::Bin::VulkanResource::createBuffer(device,*instance.physicalDevice(),inputBytes,vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);auto dst=IQM::Bin::VulkanResource::createBuffer(device,*instance.physicalDevice(),bytes,vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);src.first.bindMemory(src.second,0);dst.first.bindMemory(dst.second,0);
    void* mapped=src.second.mapMemory(0,inputBytes);std::memcpy(mapped,input.data(),inputBytes);src.second.unmapMemory();
    auto pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,2}});const vk::DescriptorSetLayout raw=*setLayout;auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});const std::vector<vk::DescriptorBufferInfo> a{{*src.first,0,inputBytes}},b{{*dst.first,0,bytes}};device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,a),VulkanRuntime::createWriteSet(*sets[0],1,b)},{});
    instance.cmdBuf()->reset();instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
    instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});struct Push{uint32_t count,a,b,c;};instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,Push{groups,count,floorMode,0});const auto limits=instance.physicalDevice()->getProperties().limits; const uint32_t totalGroups=(groups+63)/64, gx=std::min(totalGroups,limits.maxComputeWorkGroupCount[0]), gy=(totalGroups+gx-1)/gx; if(gy>limits.maxComputeWorkGroupCount[1]||bytes>limits.maxStorageBufferRange||inputBytes>limits.maxStorageBufferRange)throw std::runtime_error("4K dispatch/storage limits"); instance.cmdBuf()->dispatch(gx,gy,1);vk::MemoryBarrier after{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eHost,{}, {after},{},{});instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);
    std::vector<float> output(outputCount);mapped=dst.second.mapMemory(0,bytes);std::memcpy(output.data(),mapped,bytes);dst.second.unmapMemory();instance.cmdBuf()->reset();return output;
}
}
namespace IQM::HDRVDP3 {
static std::vector<float> project_rgb_single_gpu_impl(const IQM::VulkanInstance& instance,
    const std::string& shader,vk::Buffer rgbInput,std::span<const float> rgbHost,unsigned pixelCount,
    std::span<const float> coefficients,SingleProjectionKind kind){
    if(kind!=SingleProjectionKind::NativeColor&&kind!=SingleProjectionKind::LMSR)
        throw std::invalid_argument("Unknown Single GPU projection");
    const uint32_t channels=kind==SingleProjectionKind::NativeColor?3:4;
    if(!pixelCount||pixelCount>3840u*2160u||coefficients.size()!=3*channels)
        throw std::invalid_argument("Single GPU projection shape/domain");
    for(float v:coefficients)if(!std::isfinite(v))throw std::invalid_argument("Invalid Single GPU matrix");
    const uint32_t count=channels*pixelCount;
    const auto limits=instance.physicalDevice()->getProperties().limits;
    const size_t inputBytes=(size_t(pixelCount)*3+coefficients.size())*4;
    if(size_t(count)*4>limits.maxStorageBufferRange||inputBytes>limits.maxStorageBufferRange)
        throw std::invalid_argument("Single GPU storage range unsupported");
    using IQM::GPU::VulkanRuntime;const auto& device=*instance.device();
    std::ifstream file(shader,std::ios::binary|std::ios::ate);const auto length=file.tellg();if(length<=0||length%4)throw std::runtime_error("Bad product shader");std::vector<uint32_t> code(size_t(length)/4);file.seekg(0);file.read(reinterpret_cast<char*>(code.data()),length);if(!file||code[0]!=0x07230203u)throw std::runtime_error("Bad SPIR-V");
    auto module=VulkanRuntime::createShaderModule(device,code);auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(16));auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
    const unsigned bytes=unsigned(count*4),inputBufBytes=unsigned(inputBytes);
    auto src=IQM::Bin::VulkanResource::createBuffer(device,*instance.physicalDevice(),inputBufBytes,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
    auto dst=IQM::Bin::VulkanResource::createBuffer(device,*instance.physicalDevice(),bytes,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc,vk::MemoryPropertyFlagBits::eDeviceLocal);
    auto staging=IQM::Bin::VulkanResource::createBuffer(device,*instance.physicalDevice(),bytes,vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
    src.first.bindMemory(src.second,0);dst.first.bindMemory(dst.second,0);staging.first.bindMemory(staging.second,0);
    void* mapped=src.second.mapMemory(0,inputBufBytes);
    if(rgbInput){ std::memset(mapped,0,size_t(pixelCount)*3*4); std::memcpy(static_cast<char*>(mapped)+size_t(pixelCount)*3*4,coefficients.data(),coefficients.size()*4); }
    else { if(rgbHost.size()!=size_t(pixelCount)*3)throw std::invalid_argument("Single GPU projection host input shape"); for(float v:rgbHost)if(!std::isfinite(v)||v<0)throw std::invalid_argument("Invalid Single GPU RGB"); std::memcpy(mapped,rgbHost.data(),rgbHost.size()*4); std::memcpy(static_cast<char*>(mapped)+rgbHost.size()*4,coefficients.data(),coefficients.size()*4); }
    src.second.unmapMemory();
    auto pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,2}});const vk::DescriptorSetLayout raw=*setLayout;auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});const std::vector<vk::DescriptorBufferInfo> a{{*src.first,0,inputBufBytes}},b{{*dst.first,0,bytes}};device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,a),VulkanRuntime::createWriteSet(*sets[0],1,b)},{});
    instance.cmdBuf()->reset();instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    if(rgbInput){
      vk::BufferCopy copyIn{.srcOffset=0,.dstOffset=0,.size=size_t(pixelCount)*3*4};
      instance.cmdBuf()->copyBuffer(rgbInput,*src.first,{copyIn});
      vk::MemoryBarrier copyDone{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
      instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eComputeShader,{}, {copyDone},{},{});
    }
    vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
    instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});struct Push{uint32_t count,pixels,floorMode,pad;};instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,Push{count,pixelCount,kind==SingleProjectionKind::NativeColor?1u:0u,0u});const uint32_t totalGroups=(count+63)/64, gx=std::min(totalGroups,limits.maxComputeWorkGroupCount[0]), gy=(totalGroups+gx-1)/gx; if(gy>limits.maxComputeWorkGroupCount[1])throw std::runtime_error("4K dispatch limits"); instance.cmdBuf()->dispatch(gx,gy,1);
    vk::MemoryBarrier after{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eTransferRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eTransfer,{}, {after},{},{});
    instance.cmdBuf()->copyBuffer(*dst.first,*staging.first,{vk::BufferCopy{0,0,bytes}});
    vk::MemoryBarrier toHost{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{}, {toHost},{},{});
    instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);
    std::vector<float> output(count);mapped=staging.second.mapMemory(0,bytes);std::memcpy(output.data(),mapped,bytes);staging.second.unmapMemory();instance.cmdBuf()->reset();return output;
}
std::vector<float> project_rgb_single_gpu(const IQM::VulkanInstance& instance,
    const std::string& shader,std::span<const float> rgb,std::span<const float> coefficients,
    SingleProjectionKind kind){
    if(rgb.empty()||rgb.size()%3)throw std::invalid_argument("Single GPU projection shape/domain");
    return project_rgb_single_gpu_impl(instance,shader,VK_NULL_HANDLE,rgb,unsigned(rgb.size()/3),coefficients,kind);
}
std::vector<float> project_rgb_single_gpu_from_device(const IQM::VulkanInstance& instance,
    const std::string& shader,vk::Buffer rgbInput,unsigned pixelCount,
    std::span<const float> coefficients,SingleProjectionKind kind){
    if(!rgbInput)throw std::invalid_argument("Single GPU projection device input is null");
    return project_rgb_single_gpu_impl(instance,shader,rgbInput,{},pixelCount,coefficients,kind);
}



GpuFloatBuffer project_rgb_single_gpu_device(const IQM::VulkanInstance& instance,
    const std::string& shader,vk::Buffer rgbInput,unsigned pixelCount,
    std::span<const float> coefficients,SingleProjectionKind kind){
    if(!rgbInput)throw std::invalid_argument("Single GPU projection device input is null");
    if(kind!=SingleProjectionKind::NativeColor&&kind!=SingleProjectionKind::LMSR)
        throw std::invalid_argument("Unknown Single GPU projection");
    const uint32_t channels=kind==SingleProjectionKind::NativeColor?3:4;
    if(!pixelCount||pixelCount>3840u*2160u||coefficients.size()!=3*channels)
        throw std::invalid_argument("Single GPU projection shape/domain");
    for(float v:coefficients)if(!std::isfinite(v))throw std::invalid_argument("Invalid Single GPU matrix");
    using IQM::GPU::VulkanRuntime;const auto& device=*instance.device();
    std::ifstream file(shader,std::ios::binary|std::ios::ate);const auto length=file.tellg();if(length<=0||length%4)throw std::runtime_error("Bad product shader");std::vector<uint32_t> code(size_t(length)/4);file.seekg(0);file.read(reinterpret_cast<char*>(code.data()),length);if(!file||code[0]!=0x07230203u)throw std::runtime_error("Bad SPIR-V");
    auto module=VulkanRuntime::createShaderModule(device,code);auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(16));auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
    const uint32_t count=channels*pixelCount; const unsigned bytes=count*4,inputBufBytes=unsigned((size_t(pixelCount)*3+coefficients.size())*4);
    auto src=IQM::Bin::VulkanResource::createBuffer(device,*instance.physicalDevice(),inputBufBytes,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
    auto dst=IQM::Bin::VulkanResource::createBuffer(device,*instance.physicalDevice(),bytes,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc,vk::MemoryPropertyFlagBits::eDeviceLocal);
    src.first.bindMemory(src.second,0);dst.first.bindMemory(dst.second,0);
    void* mapped=src.second.mapMemory(0,inputBufBytes);std::memset(mapped,0,size_t(pixelCount)*3*4);std::memcpy(static_cast<char*>(mapped)+size_t(pixelCount)*3*4,coefficients.data(),coefficients.size()*4);src.second.unmapMemory();
    auto pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,2}});const vk::DescriptorSetLayout raw=*setLayout;auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});const std::vector<vk::DescriptorBufferInfo> a{{*src.first,0,inputBufBytes}},b{{*dst.first,0,bytes}};device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,a),VulkanRuntime::createWriteSet(*sets[0],1,b)},{});
    instance.cmdBuf()->reset();instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    instance.cmdBuf()->copyBuffer(rgbInput,*src.first,{vk::BufferCopy{0,0,size_t(pixelCount)*3*4}});
    vk::MemoryBarrier copyDone{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eComputeShader,{},{copyDone},{},{});
    vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{},{before},{},{});
    instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});
    struct Push{uint32_t count,pixels,floorMode,pad;};instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,Push{count,pixelCount,kind==SingleProjectionKind::NativeColor?1u:0u,0u});
    const auto limits=instance.physicalDevice()->getProperties().limits; const uint32_t totalGroups=(count+63)/64,gx=std::min(totalGroups,limits.maxComputeWorkGroupCount[0]),gy=(totalGroups+gx-1)/gx; if(gy>limits.maxComputeWorkGroupCount[1])throw std::runtime_error("4K dispatch limits");
    instance.cmdBuf()->dispatch(gx,gy,1);
    vk::MemoryBarrier after{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eTransferRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eTransfer,{},{after},{},{});
    instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);instance.cmdBuf()->reset();
    return GpuFloatBuffer{std::move(dst.first),std::move(dst.second),bytes,count};
}

}
