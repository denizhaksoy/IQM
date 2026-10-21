#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <IQM/base/vulkan_runtime.h>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <vector>
#include <algorithm>
using IQM::GPU::VulkanRuntime;
struct Buffer {vk::raii::DeviceMemory memory=nullptr;vk::raii::Buffer handle=nullptr;};
static Buffer buffer(const IQM::Bin::VulkanInstance& instance,unsigned bytes){
    auto pair=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),bytes,
        vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
    Buffer result;result.handle=std::move(pair.first);result.memory=std::move(pair.second);result.handle.bindMemory(result.memory,0);return result;
}
static std::vector<float> read(const std::filesystem::path& path,unsigned width=512){
    std::ifstream file(path);unsigned h,w;
    if(!(file>>h>>w)||h!=256||w!=width)throw std::runtime_error("Unexpected complex matrix shape");
    std::vector<float> values(h*w);for(float& value:values)if(!(file>>value)||!std::isfinite(value))throw std::runtime_error("Invalid reference values");
    std::string extra;if(file>>extra)throw std::runtime_error("Trailing reference values");return values;
}
int main(int argc,char** argv){
    const bool axisOnly=argc==6&&std::string(argv[5])=="--forward-axis";
    const bool forward=axisOnly||(argc==6&&std::string(argv[5])=="--forward");
    if(argc!=5&&!forward){std::cerr<<"Usage:fft256-inverse-test <native fixture> <shader.spv> ref|test <fresh export> [--forward|--forward-axis]\n";return 2;}
    try{
        const std::string label=argv[3];if(label!="ref"&&label!="test")return 2; const std::string direction=forward?"forward":"inverse"; const std::filesystem::path output_dir(argv[4]);if(!std::filesystem::create_directory(output_dir))throw std::runtime_error("Fresh export directory required");
        const auto input=read(std::filesystem::path(argv[1])/(label+(forward?"_mtf_stage0.txt":"_mtf_stage2.txt")));
        const auto expected=read(std::filesystem::path(argv[1])/(label+(axisOnly?"_axis0_compact.txt":forward?"_native_forward.txt":"_native_inverse.txt")),axisOnly?258:512);
        if(forward)for(size_t i=1;i<input.size();i+=2)if(input[i]!=0)throw std::runtime_error("Forward input must be real");
        IQM::Bin::VulkanInstance instance(true);const auto& device=*instance.device();
        const auto properties=instance.physicalDevice()->getProperties2<vk::PhysicalDeviceProperties2,vk::PhysicalDeviceFloatControlsProperties>();
        const auto& controls=properties.get<vk::PhysicalDeviceFloatControlsProperties>();
        std::cout<<"float_controls preserve32="<<controls.shaderDenormPreserveFloat32<<" flush32="<<controls.shaderDenormFlushToZeroFloat32<<" rte32="<<controls.shaderRoundingModeRTEFloat32<<'\n';
        if(!controls.shaderDenormPreserveFloat32||!controls.shaderRoundingModeRTEFloat32)
            throw std::runtime_error("FFT256 experiment requires Float32 denormal preservation and RTE support");
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
        for(unsigned axis=0;axis<(axisOnly?1u:2u);++axis){
            const Push push{256,256,axis==0?3u:4u,0};
            instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[axis]},{});
            instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,push);
            instance.cmdBuf()->dispatch((push.batches+31)/32,1,1);
            vk::MemoryBarrier between{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=axis==0?vk::AccessFlagBits::eShaderRead:vk::AccessFlagBits::eHostRead};
            if(axis==0)between.dstAccessMask|=vk::AccessFlagBits::eHostRead;
            instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,axis==0?(vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eHost):vk::PipelineStageFlagBits::eHost,{}, {between},{},{});
        }
        instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();
        vk::SubmitInfo submit{.commandBufferCount=1,.pCommandBuffers=&cmd};vk::raii::Fence fence(device,vk::FenceCreateInfo{});
        instance.queue()->submit(submit,fence);instance.waitForFence(fence);
        if(axisOnly){
            const auto* actual=static_cast<const float*>(temporary.memory.mapMemory(0,bytes));size_t mismatches=0;double maximum=0;
            std::ofstream dump(output_dir/"axis0_compact.txt");dump<<"256 258\n"<<std::setprecision(9);
            for(size_t i=0;i<expected.size();++i){dump<<actual[i]<<'\n';if(!std::isfinite(actual[i])||actual[i]!=expected[i]){
                if(mismatches<5)std::cout<<"index="<<i<<" actual="<<std::setprecision(9)<<actual[i]<<" expected="<<expected[i]<<'\n';
                ++mismatches;maximum=std::max(maximum,std::abs(double(actual[i])-expected[i]));
            }}temporary.memory.unmapMemory();
            std::cout<<"forward_axis values="<<expected.size()<<" mismatches="<<mismatches<<" max="<<std::setprecision(17)<<maximum<<'\n';return mismatches?1:0;
        }
        const auto* actual=static_cast<const float*>(output.memory.mapMemory(0,bytes));size_t mismatches=0;
        for(size_t i=0;i<expected.size();++i){ const size_t complex=i/2,channel=i%2; const float value=actual[2*((complex%256)*256+complex/256)+channel]; if(!std::isfinite(value)||value!=expected[i]){if(mismatches<5)std::cout<<"index="<<i<<" actual="<<value<<" expected="<<expected[i]<<char(10);++mismatches;}}
        std::ofstream dump(output_dir/(forward?"forward_row_major.txt":"inverse_row_major.txt"));dump<<"256 512\n"<<std::setprecision(std::numeric_limits<float>::max_digits10);
        for(size_t i=0;i<expected.size();++i){const size_t complex=i/2,channel=i%2;dump<<actual[2*((complex%256)*256+complex/256)+channel]<<'\n';}
        output.memory.unmapMemory();
        const auto* axis0=static_cast<const float*>(temporary.memory.mapMemory(0,bytes));
        std::ofstream first(output_dir/"axis0_storage.txt");first<<"256 "<<(forward?258:512)<<'\n'<<std::setprecision(std::numeric_limits<float>::max_digits10);
        for(size_t i=0;i<(forward?66048:expected.size());++i)first<<axis0[i]<<'\n';temporary.memory.unmapMemory();
        std::cout<<direction<<" values="<<expected.size()<<" mismatches="<<mismatches<<'\n';return mismatches?1:0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 2;}
}
