#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <IQM/base/vulkan_runtime.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>
using IQM::GPU::VulkanRuntime;
#ifndef IQM_BASE_COUNT
#define IQM_BASE_COUNT 120
#endif
#if !defined(IQM_BASE_GENERAL)
constexpr unsigned BaseCount=IQM_BASE_COUNT,MeanStride=BaseCount+1,FFTWords=2*BaseCount,OutputWords=3*BaseCount+1;
static_assert(BaseCount==120||BaseCount==144);
#endif
struct Buffer{vk::raii::DeviceMemory memory=nullptr;vk::raii::Buffer handle=nullptr;};
static Buffer buffer(const IQM::Bin::VulkanInstance& instance,unsigned bytes){
    auto pair=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),bytes,
        vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
    Buffer b;b.handle=std::move(pair.first);b.memory=std::move(pair.second);b.handle.bindMemory(b.memory,0);return b;
}
static std::vector<uint32_t> words(const std::filesystem::path& path){
    std::ifstream f(path);unsigned h,w;if(!(f>>h>>w)||!h||!w||size_t(h)*w>1000000)throw std::runtime_error("Invalid word fixture");
    std::vector<uint32_t> v(size_t(h)*w);for(auto& x:v)if(!(f>>x))throw std::runtime_error("Truncated word fixture");
    std::string extra;if(f>>extra)throw std::runtime_error("Trailing fixture words");return v;
}
static std::vector<uint32_t> code(const std::filesystem::path& path){
    std::ifstream f(path,std::ios::binary|std::ios::ate);auto length=f.tellg();if(length<=0||length%4)throw std::runtime_error("Invalid SPIR-V");
    std::vector<uint32_t> v(size_t(length)/4);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),length);
    if(!f||v[0]!=0x07230203u)throw std::runtime_error("Invalid shader words");return v;
}
int main(int argc,char** argv){try{
#if defined(IQM_BASE256_DIAGNOSTIC)
    const bool extraCount=argc==8;const unsigned expectedCases=extraCount?unsigned(std::stoul(argv[7])):5u;
    if(expectedCases!=5&&expectedCases!=6)throw std::runtime_error("Unvalidated case count");
#else
    const bool extraCount=false;
#endif
    const bool difference=(argc==7||extraCount)&&std::string(argv[6])=="--pathway-difference";
    const bool pathway=(argc==7||extraCount)&&(std::string(argv[6])=="--pathway"||difference);
    if(argc!=6&&!pathway)throw std::runtime_error("Usage:mean.spv pack.spv fft.spv manifest fresh-output [--pathway|--pathway-difference]");
    const std::filesystem::path output(argv[5]);if(std::filesystem::exists(output)||!std::filesystem::create_directories(output))throw std::runtime_error("Fresh output required");
    IQM::Bin::VulkanInstance instance(true);const auto& device=*instance.device();
    const auto properties=instance.physicalDevice()->getProperties2<vk::PhysicalDeviceProperties2,vk::PhysicalDeviceFloatControlsProperties>();
    const auto& controls=properties.get<vk::PhysicalDeviceFloatControlsProperties>();
    if(!controls.shaderDenormPreserveFloat32||!controls.shaderRoundingModeRTEFloat32)throw std::runtime_error("Float32 modes unsupported");
    auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
    auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(16));
    std::vector<vk::raii::ShaderModule> modules;std::vector<vk::raii::Pipeline> pipelines;
    for(int i=1;i<=3;++i){modules.push_back(VulkanRuntime::createShaderModule(device,code(argv[i])));pipelines.push_back(VulkanRuntime::createComputePipeline(device,modules.back(),layout));}
    std::ifstream manifest(argv[4]);if(!manifest)throw std::runtime_error("Missing manifest");
    std::ofstream results(output/"results.tsv");results<<"case\tmean_bit_errors\tpack_bit_errors\tfft_numeric_errors\tmag_index_bit_errors\tfft_zero_sign_differences\n";
    std::string tag,inputPath,meanPath,packPath,expectedPath;unsigned h,w,total=0,orientations=0;
    while(manifest>>tag>>h>>w>>std::quoted(inputPath)>>std::quoted(meanPath)>>std::quoted(packPath)>>std::quoted(expectedPath)){
        #if defined(IQM_BASE_GENERAL)
        #if defined(IQM_BASE256_DIAGNOSTIC)
        if(!difference||h!=16||w!=16)
#else
        if(!difference||!((h==10||h==12)&&(w==10||w==12)))
#endif
        throw std::runtime_error("Unvalidated general shape/mode");
        const unsigned BaseCount=h*w,MeanStride=BaseCount+1,FFTWords=2*BaseCount,OutputWords=3*BaseCount+1;
#else
        if(!(BaseCount==144?(h==12&&w==12):((h==10&&w==12)||(h==12&&w==10))))throw std::runtime_error("Unvalidated shape");
#endif
        const auto input=words(inputPath),mean=words(meanPath),pack=words(packPath),expected=words(expectedPath);
        const unsigned groups=pathway?2:6;
        const unsigned outputs=difference?1:groups;
        if(input.size()!=groups*BaseCount||mean.size()!=groups*MeanStride||pack.size()!=outputs*FFTWords||expected.size()!=outputs*OutputWords)throw std::runtime_error("Incomplete pipeline coverage");
        for(unsigned batch=0;batch<groups;++batch){int minimum=1000,maximum=-1000;
            for(unsigned i=0;i<BaseCount;++i){float v=std::bit_cast<float>(input[batch*BaseCount+i]);if(!std::isfinite(v))throw std::runtime_error("Nonfinite mean input");
                if(v!=0){int e=std::ilogb(std::abs(v));minimum=std::min(minimum,e);maximum=std::max(maximum,e);}}
            if(maximum!=-1000&&(minimum < -90||maximum>100||maximum-minimum>16))throw std::runtime_error("Mean domain exceeded");
        }
        for(unsigned batch=0;batch<outputs;++batch)for(unsigned i=0;i<FFTWords;++i){float v=std::bit_cast<float>(expected[batch*OutputWords+i]);if(!std::isfinite(v)||(v!=0&&(std::ilogb(std::abs(v)) < -90||std::ilogb(std::abs(v))>100)))throw std::runtime_error("Hypot domain exceeded");}
        const unsigned sizes[]={unsigned(input.size()*4),unsigned(mean.size()*4),unsigned(pack.size()*4),unsigned(expected.size()*4)};
        std::array<Buffer,4> buffers{buffer(instance,sizes[0]),buffer(instance,sizes[1]),buffer(instance,sizes[2]),buffer(instance,sizes[3])};
        auto* mapped=buffers[0].memory.mapMemory(0,sizes[0]);std::memcpy(mapped,input.data(),sizes[0]);buffers[0].memory.unmapMemory();
        auto pool=VulkanRuntime::createDescPool(device,3,{{vk::DescriptorType::eStorageBuffer,6}});const vk::DescriptorSetLayout raw[]={*setLayout,*setLayout,*setLayout};
        auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=3,.pSetLayouts=raw});
        for(unsigned stage=0;stage<3;++stage){const std::vector<vk::DescriptorBufferInfo> a{{*buffers[stage].handle,0,sizes[stage]}},b{{*buffers[stage+1].handle,0,sizes[stage+1]}};
            device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[stage],0,a),VulkanRuntime::createWriteSet(*sets[stage],1,b)},{});}
        instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
        instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
        struct Push{uint32_t groups,h,w,reserved;};
        for(unsigned stage=0;stage<3;++stage){
            const unsigned stageGroups=stage==0?groups:outputs;
            #if defined(IQM_BASE_GENERAL)
            const Push push{stageGroups,stage==0?BaseCount:h,w,0};
#else
            const Push push{stageGroups,h,w,0};
#endif
            instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipelines[stage]);
            instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[stage]},{});
            instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,push);
            instance.cmdBuf()->dispatch(((stage==1?stageGroups*BaseCount:stageGroups)+63)/64,1,1);
            vk::MemoryBarrier barrier{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=stage==2?vk::AccessFlagBits::eHostRead:vk::AccessFlagBits::eShaderRead};
            instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,stage==2?vk::PipelineStageFlagBits::eHost:vk::PipelineStageFlagBits::eComputeShader,{}, {barrier},{},{});
        }
        instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});
        instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);
        size_t meanErrors=0,packErrors=0,fftErrors=0,magErrors=0,zeroDifferences=0;
        for(unsigned stage=1;stage<=3;++stage){const auto* actual=static_cast<const uint32_t*>(buffers[stage].memory.mapMemory(0,sizes[stage]));
            const auto& target=stage==1?mean:stage==2?pack:expected;
            for(size_t i=0;i<target.size();++i){if(stage==1)meanErrors+=actual[i]!=target[i];else if(stage==2)packErrors+=actual[i]!=target[i];
                else if(i%OutputWords<FFTWords){fftErrors+=std::bit_cast<float>(actual[i])!=std::bit_cast<float>(target[i]);zeroDifferences+=actual[i]!=target[i];}
                else magErrors+=actual[i]!=target[i];}
            std::ofstream dump(output/(tag+"_stage"+std::to_string(stage)+".u32"),std::ios::binary);dump.write(reinterpret_cast<const char*>(actual),sizes[stage]);buffers[stage].memory.unmapMemory();
        }
        results<<tag<<'\t'<<meanErrors<<'\t'<<packErrors<<'\t'<<fftErrors<<'\t'<<magErrors<<'\t'<<zeroDifferences<<'\n';results.flush();instance.cmdBuf()->reset();
        #if defined(IQM_BASE_GENERAL)
        if(meanErrors||packErrors||fftErrors||magErrors||zeroDifferences)
#else
        if(meanErrors||packErrors||fftErrors||magErrors||(BaseCount==144&&zeroDifferences))
#endif
        throw std::runtime_error("GPU pipeline mismatch:"+tag);
        total+=groups;++orientations;
    }
    #if defined(IQM_BASE_GENERAL)
    if(!manifest.eof()||total!=
#if defined(IQM_BASE256_DIAGNOSTIC)
        2u*expectedCases||orientations!=expectedCases
#else
        40u||orientations!=20u
#endif
        )throw std::runtime_error("General pipeline coverage incomplete");
    constexpr bool strictZero=true;
#else
    if(!manifest.eof()||total!=(BaseCount==144?10u:pathway?4u:12u)||orientations!=(BaseCount==144?5u:2u))throw std::runtime_error("Pipeline coverage incomplete");
    constexpr bool strictZero=BaseCount==144;
#endif
    std::cout<<"GPU mean/DC -> packing -> FFT/magnitude/index: cases="<<total<<" mismatches=0\n";
    std::ofstream(output/"PASS.txt")<<"Three GPU compute stages without host intermediate computation; input bands="<<total<<"; pathway="<<pathway<<"; difference="<<difference<<". "<<(strictZero?"All FFT signed-zero bits asserted.":"FFT zero-sign bits excluded.")<<"\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
