#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <IQM/base/vulkan_runtime.h>
#include <bit>
#include <cfenv>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <stdexcept>
#if defined(_M_IX86)||defined(_M_X64)||defined(__i386__)||defined(__x86_64__)
#include <xmmintrin.h>
#endif
using IQM::GPU::VulkanRuntime;
struct Buffer{vk::raii::DeviceMemory memory=nullptr;vk::raii::Buffer handle=nullptr;};
static Buffer buffer(const IQM::Bin::VulkanInstance& instance,unsigned bytes){
    auto pair=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),bytes,
        vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
    Buffer b;b.handle=std::move(pair.first);b.memory=std::move(pair.second);b.handle.bindMemory(b.memory,0);return b;
}
static std::vector<uint32_t> read(const std::filesystem::path& path){
    std::ifstream file(path);unsigned h,w;if(!(file>>h>>w)||!h||!w||size_t(h)*w>1000000)throw std::runtime_error("Invalid native shape");
    std::vector<uint32_t> data(size_t(h)*w);for(auto& v:data){double value;if(!(file>>value))throw std::runtime_error("Invalid native float");v=std::bit_cast<uint32_t>(float(value));}
    return data;
}
int main(int argc,char** argv){try{
    const bool square160=argc==5&&std::string(argv[4])=="--square160";
    const bool square=square160||(argc==5&&std::string(argv[4])=="--square192");
    if(argc!=4&&!square)throw std::runtime_error("Usage:shader.spv native-pairs-manifest fresh-output");
    if(std::fegetround()!=FE_TONEAREST)throw std::runtime_error("Host needs RTE");
#if defined(_M_IX86)||defined(_M_X64)||defined(__i386__)||defined(__x86_64__)
    if(_mm_getcsr()&((1u<<15)|(1u<<6)))throw std::runtime_error("Host needs gradual underflow");
#endif
    const std::filesystem::path output(argv[3]);if(std::filesystem::exists(output)||!std::filesystem::create_directories(output))throw std::runtime_error("Fresh output required");
    std::ifstream shader(argv[1],std::ios::binary|std::ios::ate);auto length=shader.tellg();if(length<=0||length%4)throw std::runtime_error("Invalid shader");
    std::vector<uint32_t> code(size_t(length)/4);shader.seekg(0);shader.read(reinterpret_cast<char*>(code.data()),length);if(!shader||code[0]!=0x07230203u)throw std::runtime_error("Invalid SPIR-V");
    IQM::Bin::VulkanInstance instance(true);const auto& device=*instance.device();
    auto module=VulkanRuntime::createShaderModule(device,code);
    auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
    auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(16));auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
    std::ofstream results(output/"results.tsv");results<<"case\tvalues\tnumeric_mismatches\tbit_mismatches\n";size_t total=0,nativeTotal=0,batches=0;
    auto check=[&](const std::string& tag,const std::vector<uint32_t>& input,const std::vector<uint32_t>& expected,bool syntheticBatch){
        if(input.size()!=expected.size()||input.empty())throw std::runtime_error("Fixture count mismatch");
        unsigned bytes=unsigned(input.size()*4);auto src=buffer(instance,bytes),dst=buffer(instance,bytes);
        void* mapped=src.memory.mapMemory(0,bytes);std::memcpy(mapped,input.data(),bytes);src.memory.unmapMemory();
        auto pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,2}});const vk::DescriptorSetLayout raw=*setLayout;
        auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});
        const std::vector<vk::DescriptorBufferInfo> a{{*src.handle,0,bytes}},b{{*dst.handle,0,bytes}};
        device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,a),VulkanRuntime::createWriteSet(*sets[0],1,b)},{});
        instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
        instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
        instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});
        struct Push{uint32_t count,a,b,c;};instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,Push{uint32_t(input.size()),0,0,0});instance.cmdBuf()->dispatch((uint32_t(input.size())+63)/64,1,1);
        vk::MemoryBarrier after{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};
        instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eHost,{}, {after},{},{});
        instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);
        const auto* actual=static_cast<const uint32_t*>(dst.memory.mapMemory(0,bytes));size_t numeric=0,bits=0;
        for(size_t i=0;i<input.size();++i){numeric+=std::bit_cast<float>(actual[i])!=std::bit_cast<float>(expected[i]);bits+=actual[i]!=expected[i];}
        std::ofstream dump(output/(tag+".u32"),std::ios::binary);dump.write(reinterpret_cast<const char*>(actual),bytes);
        dst.memory.unmapMemory();instance.cmdBuf()->reset();results<<tag<<'\t'<<input.size()<<'\t'<<numeric<<'\t'<<bits<<'\n';results.flush();
        if(numeric||bits)throw std::runtime_error("Normalization mismatch:"+tag);
        if(syntheticBatch)total+=input.size();else nativeTotal+=input.size();++batches;
    };
    std::vector<uint32_t> synthetic;uint32_t rng=0x30720160u;
    for(uint32_t i=0;i<1048576;++i){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;synthetic.push_back((rng&0x807fffffu)|((i%255)<<23));}
    const std::vector<uint32_t> fractions=square160?std::vector<uint32_t>{0u,1u,12799u,12800u,12801u,38399u,38400u,38401u,0x47ffffu,0x480000u,0x480001u,0x7fffffu}:square?std::vector<uint32_t>{0u,1u,18431u,18432u,18433u,55295u,55296u,55297u,0x0fffffu,0x100000u,0x100001u,0x7fffffu}:std::vector<uint32_t>{0u,1u,15359u,15360u,15361u,46079u,46080u,46081u,0x6fffffu,0x700000u,0x700001u,0x7fffffu};
    for(uint32_t exponent=0;exponent<255;++exponent)for(uint32_t sign:{0u,0x80000000u})for(uint32_t fraction:fractions)synthetic.push_back(sign|(exponent<<23)|fraction);
    std::vector<uint32_t> expected;volatile float denominator=square160?25600.0f:square?36864.0f:30720.0f;
    for(uint32_t word:synthetic){float value=std::bit_cast<float>(word)/denominator;expected.push_back(std::bit_cast<uint32_t>(value));}
    std::ofstream(output/"synthetic_input.u32",std::ios::binary).write(reinterpret_cast<const char*>(synthetic.data()),std::streamsize(synthetic.size()*4));
    std::ofstream(output/"synthetic_expected.u32",std::ios::binary).write(reinterpret_cast<const char*>(expected.data()),std::streamsize(expected.size()*4));
    check("synthetic",synthetic,expected,true);
    std::ifstream manifest(argv[2]);if(!manifest)throw std::runtime_error("Missing native manifest");std::string tag,inputPath,expectedPath;
    while(manifest>>tag>>std::quoted(inputPath)>>std::quoted(expectedPath))check(tag,read(inputPath),read(expectedPath),false);
    if(!manifest.eof()||batches!=(square?5u:25u)||nativeTotal!=(square160?204800u:square?294912u:1474560u))throw std::runtime_error("Native coverage incomplete");
    std::cout<<"normalization synthetic_bitwise="<<total<<" native_bitwise="<<nativeTotal<<" mismatches=0\n";
    std::ofstream(output/"PASS.txt")<<"Fixed"<<(square160?25600:square?36864:30720)<<" division bitwise exact over1054696 synthetic finite IEEE probes and"<<(square?4:24)<<" native inverse outputs.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
