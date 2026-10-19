#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <IQM/base/vulkan_runtime.h>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using IQM::GPU::VulkanRuntime;
struct Buffer {vk::raii::DeviceMemory memory=nullptr;vk::raii::Buffer handle=nullptr;};
static Buffer buffer(const IQM::Bin::VulkanInstance& instance,unsigned bytes){
    auto pair=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),bytes,
        vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
    Buffer result;result.handle=std::move(pair.first);result.memory=std::move(pair.second);result.handle.bindMemory(result.memory,0);return result;
}
static std::vector<float> read(const std::filesystem::path& path){
    std::ifstream file(path);unsigned h,w;
    if(!(file>>h>>w)||h!=64||w!=128)throw std::runtime_error("Expected one64x64 complex matrix");
    std::vector<float> values(h*w);for(float& value:values)if(!(file>>value)||!std::isfinite(value))throw std::runtime_error("Invalid reference values");
    std::string extra;if(file>>extra)throw std::runtime_error("Trailing reference values");return values;
}
int main(int argc,char** argv){
    if(argc!=4){std::cerr<<"Usage:fft64-reference-test <fixture directory> <shader.spv> forward|inverse\n";return 2;}
    try{
        const std::string direction=argv[3];if(direction!="forward"&&direction!="inverse")return 2;
        const bool forward=direction=="forward";
        const auto input=read(std::filesystem::path(argv[1])/"input64.txt");
        const auto expected=read(std::filesystem::path(argv[1])/(direction+"64.txt"));
        if(forward)for(size_t i=1;i<input.size();i+=2)if(input[i]!=0)throw std::runtime_error("Forward input must be real");
        IQM::Bin::VulkanInstance instance(true);const auto& device=*instance.device();
        std::ifstream file(argv[2],std::ios::binary|std::ios::ate);const auto size=file.tellg();
        if(size<=0||size%4)throw std::runtime_error("Invalid SPIR-V size");
        std::vector<uint32_t> code(size_t(size)/4);file.seekg(0);if(!file.read(reinterpret_cast<char*>(code.data()),size))throw std::runtime_error("Truncated shader");
        auto module=VulkanRuntime::createShaderModule(device,code);
        auto pool=VulkanRuntime::createDescPool(device,2,{{vk::DescriptorType::eStorageBuffer,4}});
        auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
        auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(16));
        auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
        const vk::DescriptorSetLayout rawLayouts[]={*setLayout,*setLayout};
        auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=2,.pSetLayouts=rawLayouts});
        const unsigned bytes=unsigned(input.size()*sizeof(float));
        auto source=buffer(instance,bytes),temporary=buffer(instance,bytes),output=buffer(instance,bytes);
        auto* mapped=source.memory.mapMemory(0,bytes);std::memcpy(mapped,input.data(),bytes);source.memory.unmapMemory();
        const std::vector<vk::DescriptorBufferInfo> a{{*source.handle,0,bytes}},b{{*temporary.handle,0,bytes}},c{{*output.handle,0,bytes}};
        device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,a),VulkanRuntime::createWriteSet(*sets[0],1,b),VulkanRuntime::createWriteSet(*sets[1],0,b),VulkanRuntime::createWriteSet(*sets[1],1,c)},{});
        instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
        instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
        instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);
        struct Push{uint32_t n,batches,graph,fused;};
        for(unsigned axis=0;axis<2;++axis){
            const Push push{64,forward&&axis==1?33u:64u,forward?(axis==0?8u:9u):(axis==0?3u:4u),0};
            instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[axis]},{});
            instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,push);
            instance.cmdBuf()->dispatch((push.batches+31)/32,1,1);
            vk::MemoryBarrier between{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=axis==0?vk::AccessFlagBits::eShaderRead:vk::AccessFlagBits::eHostRead};
            instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,axis==0?vk::PipelineStageFlagBits::eComputeShader:vk::PipelineStageFlagBits::eHost,{}, {between},{},{});
        }
        instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();
        vk::SubmitInfo submit{.commandBufferCount=1,.pCommandBuffers=&cmd};vk::raii::Fence fence(device,vk::FenceCreateInfo{});
        instance.queue()->submit(submit,fence);instance.waitForFence(fence);
        const auto* actual=static_cast<const float*>(output.memory.mapMemory(0,bytes));size_t mismatches=0;
        for(size_t i=0;i<expected.size();++i)if(!std::isfinite(actual[i])||actual[i]!=expected[i])++mismatches;
        output.memory.unmapMemory();std::cout<<direction<<" values="<<expected.size()<<" mismatches="<<mismatches<<'\n';return mismatches?1:0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 2;}
}
