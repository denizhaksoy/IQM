#include <IQM/hdrvdp3/lmsr_lm_single_gpu.h>
#include <IQM/base/vulkan_runtime.h>
#include <IQM/hdrvdp3/map_gpu.h>
#include <vulkan_res.h>
#include <vulkan_instance.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <stdexcept>
namespace IQM::HDRVDP3 {
// GPU-resident L+M addition of two clamped L/M channels.
// Records only compute dispatch into cmd; no host transfers, submit, or wait.
// All buffers must be device-local, non-host-visible, and of matching size.
// Caller owns lifetime.
void record_lmsr_lm_gpu(vk::CommandBuffer cmd,
    vk::Buffer lHandle, vk::Buffer mHandle, vk::Buffer outHandle,
    uint32_t pixels){
    using IQM::GPU::VulkanRuntime;
    const auto& device=*cmd.getDevice();
    const uint32_t n=pixels;
    const uint32_t bytes=n*4;
    const uint32_t total=(n+63)/64,gx=std::min(total,device.getProperties().limits.maxComputeWorkGroupCount[0]),gy=(total+gx-1)/gx;
    if(bytes>device.getProperties().limits.maxStorageBufferRange||gy>device.getProperties().limits.maxComputeWorkGroupCount[1])
        throw std::runtime_error("L+M device limits");
    // Load shader SPIR-V
    std::ifstream file("shaders/hdrvdp3/lmsr_lm_single.comp",std::ios::binary|std::ios::ate);
    if(!file)throw std::runtime_error("L+M shader not found");
    auto length=file.tellg();if(length<=0||length%4)throw std::runtime_error("L+M shader size");
    file.seekg(0);std::vector<uint32_t> code(size_t(length)/4);
    file.read(reinterpret_cast<char*>(code.data()),length);
    if(!file||code[0]!=0x07230203u)throw std::runtime_error("L+M SPIR-V");
    auto module=VulkanRuntime::createShaderModule(device,code);
    auto setLayout=VulkanRuntime::createDescLayout(device,
        {{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
    auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(4));
    auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
    // Descriptor set: binding 0 = L buffer, 1 = M buffer, 2 = output buffer
    auto pool=VulkanRuntime::createDescPool(device,1,
        {{vk::DescriptorType::eStorageBuffer,3}});
    const vk::DescriptorSetLayout raw=*setLayout;
    auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{
        .descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});
    const std::vector<vk::DescriptorBufferInfo> lInfo{{lHandle,0,bytes}};
    const std::vector<vk::DescriptorBufferInfo> mInfo{{mHandle,0,bytes}};
    const std::vector<vk::DescriptorBufferInfo> oInfo{{outHandle,0,bytes}};
    device.updateDescriptorSets({
        vk::WriteDescriptorSet{*sets[0],0,0,1,&lInfo[0]},
        vk::WriteDescriptorSet{*sets[0],1,0,1,&mInfo[0]},
        vk::WriteDescriptorSet{*sets[0],2,0,1,&oInfo[0]}
    },{});
    // Record compute dispatch
    cmd.reset();cmd.begin(vk::CommandBufferBeginInfo{
        .flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eMemoryRead,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});
    cmd.pushConstants<uint32_t>(*layout,vk::ShaderStageFlagBits::eCompute,0,n);
    cmd.dispatch(gx,gy,1);
    vk::MemoryBarrier after{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eMemoryWrite};
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eAllCommands,{}, {after},{},{});
    cmd.end();
}
}
// Negative-test helper.
namespace IQM::HDRVDP3 {
bool validate_lmsr_lm_resident(uint32_t pixels);
}