#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <IQM/base/vulkan_runtime.h>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using IQM::GPU::VulkanRuntime;
struct Buffer{vk::raii::DeviceMemory memory=nullptr;vk::raii::Buffer handle=nullptr;};
static Buffer buffer(const IQM::Bin::VulkanInstance& instance,unsigned bytes){
    auto pair=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),bytes,
        vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
    Buffer result;result.handle=std::move(pair.first);result.memory=std::move(pair.second);result.handle.bindMemory(result.memory,0);return result;
}
static std::vector<uint32_t> words(const std::filesystem::path& path,unsigned h,unsigned w){
    std::ifstream in(path);unsigned actualH,actualW;
    if(!(in>>actualH>>actualW)||actualH!=h||actualW!=w)throw std::runtime_error("Unexpected fixture shape");
    std::vector<uint32_t> result(size_t(h)*w);
    for(auto& value:result){uint64_t wide;if(!(in>>wide)||wide>UINT32_MAX)throw std::runtime_error("Invalid fixture word");value=uint32_t(wide);}
    std::string extra;if(in>>extra)throw std::runtime_error("Trailing fixture data");return result;
}
int main(int argc,char** argv){try{
    if(argc!=4)throw std::runtime_error("Usage:native-directory shader.spv fresh-export");
    const std::filesystem::path native(argv[1]),out(argv[3]);
    if(std::filesystem::exists(out)||!std::filesystem::create_directories(out))throw std::runtime_error("Fresh output directory required");
    std::ifstream shader(argv[2],std::ios::binary|std::ios::ate);auto size=shader.tellg();
    if(size<=0||size%4)throw std::runtime_error("Invalid shader size");
    std::vector<uint32_t> code(size_t(size)/4);shader.seekg(0);shader.read(reinterpret_cast<char*>(code.data()),size);
    if(!shader||code[0]!=0x07230203u)throw std::runtime_error("Invalid SPIR-V");
    IQM::Bin::VulkanInstance instance(true);const auto& device=*instance.device();
    auto module=VulkanRuntime::createShaderModule(device,code);
    auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
    auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(16));
    auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
    std::ifstream manifest(native/"padding_manifest.txt");if(!manifest)throw std::runtime_error("Missing manifest");
    std::ofstream records(out/"results.tsv");records<<"case\twords\tbit_mismatches\tnegative_zero_words\n";
    size_t cases=0,total=0,negativeZeros=0;std::string tag,expectedFile;unsigned h,w,pattern,mode,columnOrder;
    while(manifest>>tag>>h>>w>>pattern>>mode>>columnOrder>>expectedFile){
        if(!h||!w||h>2048||w>2048||pattern<1||pattern>3||mode<1||mode>2||columnOrder>1)throw std::runtime_error("Invalid padding contract");
        auto input=words(native/tag/("input"+std::to_string(pattern)+".txt"),h,w);
        auto expected=words(native/tag/expectedFile,2*h,4*w);
        unsigned inputBytes=unsigned(input.size()*4),outputBytes=unsigned(expected.size()*4);
        auto source=buffer(instance,inputBytes),output=buffer(instance,outputBytes);
        auto mapped=source.memory.mapMemory(0,inputBytes);std::memcpy(mapped,input.data(),inputBytes);source.memory.unmapMemory();
        mapped=output.memory.mapMemory(0,outputBytes);std::memset(mapped,0xcd,outputBytes);output.memory.unmapMemory();
        auto pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,2}});
        const vk::DescriptorSetLayout raw=*setLayout;
        auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});
        const std::vector<vk::DescriptorBufferInfo> src{{*source.handle,0,inputBytes}},dst{{*output.handle,0,outputBytes}};
        device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,src),VulkanRuntime::createWriteSet(*sets[0],1,dst)},{});
        instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eShaderWrite};
        instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
        instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);
        instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});
        struct Push{uint32_t width,height,mode,columnOrder;};
        instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,Push{w,h,mode,columnOrder});
        instance.cmdBuf()->dispatch((4*w*h+63)/64,1,1);
        vk::MemoryBarrier after{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};
        instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eHost,{}, {after},{},{});
        instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});
        instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);
        const auto* actual=static_cast<const uint32_t*>(output.memory.mapMemory(0,outputBytes));size_t mismatches=0,zeros=0;
        std::ofstream dump(out/(tag+"_"+expectedFile));dump<<2*h<<' '<<4*w<<'\n';
        for(size_t i=0;i<expected.size();++i){dump<<actual[i]<<'\n';mismatches+=actual[i]!=expected[i];zeros+=actual[i]==0x80000000u;}
        output.memory.unmapMemory();instance.cmdBuf()->reset();
        records<<tag<<'_'<<expectedFile<<'\t'<<expected.size()<<'\t'<<mismatches<<'\t'<<zeros<<'\n';records.flush();
        if(mismatches)throw std::runtime_error("Padding/layout bit mismatch:"+tag+"/"+expectedFile);
        ++cases;total+=expected.size();negativeZeros+=zeros;
    }
    if(!manifest.eof()||cases!=96||!negativeZeros)throw std::runtime_error("Incomplete fixture coverage");
    std::cout<<"padding cases="<<cases<<" words="<<total<<" bit_mismatches=0 negative_zero_words="<<negativeZeros<<'\n';
    std::ofstream(out/"PASS.txt")<<"Native padarray and bothcomplex layouts bitwise exact;not full general quality.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
