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
    if(argc!=5)throw std::runtime_error("Usage:fma.spv separate.spv pool.txt fresh-output");
    const std::filesystem::path output(argv[4]);if(std::filesystem::exists(output)||!std::filesystem::create_directories(output))throw std::runtime_error("Fresh output required");
    IQM::Bin::VulkanInstance instance(true);const auto& device=*instance.device();
    const auto props=instance.physicalDevice()->getProperties2<vk::PhysicalDeviceProperties2,vk::PhysicalDeviceFloatControlsProperties>();const auto& controls=props.get<vk::PhysicalDeviceFloatControlsProperties>();
    if(!controls.shaderDenormPreserveFloat32||!controls.shaderRoundingModeRTEFloat32)throw std::runtime_error("Float32 modes unsupported");
    auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
    auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(16));
    std::vector<vk::raii::ShaderModule> modules;std::vector<vk::raii::Pipeline> pipelines;
    for(int j=1;j<=2;++j){modules.push_back(VulkanRuntime::createShaderModule(device,code(argv[j])));pipelines.push_back(VulkanRuntime::createComputePipeline(device,modules.back(),layout));}
    std::ifstream fixture(argv[3]);if(!fixture)throw std::runtime_error("Missing fixture");
    std::ofstream results(output/"results.tsv");results<<"case\th\tw\tvalues\tnumeric_errors\tbit_errors\n";
    unsigned h,w,cases=0;float normalization;size_t total=0;
    while(fixture>>h>>w>>normalization){
        if(normalization!=1||!((h==80&&w==96)||(h==96&&w==80)||(h==40&&w==48)||(h==48&&w==40)||(h==20&&w==24)||(h==24&&w==20)||(h==10&&w==12)||(h==12&&w==10)))throw std::runtime_error("Unvalidated pool shape/normalization");
        const unsigned n=h*w,bytes=2*n*4;std::vector<uint32_t> input(2*n),expected(2*n);
        for(auto* v:{&input,&expected})for(auto& u:*v){float f;if(!(fixture>>f)||!std::isfinite(f))throw std::runtime_error("Invalid fixture float");u=std::bit_cast<uint32_t>(f);}
        auto src=buffer(instance,bytes),dst=buffer(instance,bytes);void* ptr=src.memory.mapMemory(0,bytes);std::memcpy(ptr,input.data(),bytes);src.memory.unmapMemory();
        auto pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,2}});const vk::DescriptorSetLayout raw=*setLayout;
        auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});
        const std::vector<vk::DescriptorBufferInfo> a{{*src.handle,0,bytes}},b{{*dst.handle,0,bytes}};
        device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,a),VulkanRuntime::createWriteSet(*sets[0],1,b)},{});
        instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
        instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
        instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});
        struct Push{uint32_t count,w,h,reserved;};instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,Push{n,w,h,0});
        for(unsigned j=0;j<2;++j){instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipelines[j]);instance.cmdBuf()->dispatch((n+63)/64,1,1);}
        vk::MemoryBarrier after{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};
        instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eHost,{}, {after},{},{});
        instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);
        const auto* actual=static_cast<const uint32_t*>(dst.memory.mapMemory(0,bytes));size_t numeric=0,bits=0;
        for(size_t k=0;k<expected.size();++k){numeric+=std::bit_cast<float>(actual[k])!=std::bit_cast<float>(expected[k]);bits+=actual[k]!=expected[k];}
        std::ofstream dump(output/("case"+std::to_string(cases)+".u32"),std::ios::binary);dump.write(reinterpret_cast<const char*>(actual),bytes);dst.memory.unmapMemory();
        results<<cases<<'\t'<<h<<'\t'<<w<<'\t'<<expected.size()<<'\t'<<numeric<<'\t'<<bits<<'\n';results.flush();instance.cmdBuf()->reset();
        if(numeric)throw std::runtime_error("Pool mismatch case="+std::to_string(cases));total+=expected.size();++cases;
    }
    if(!fixture.eof()||cases!=10||total!=71520)throw std::runtime_error("Incomplete pool coverage");
    std::ofstream(output/"PASS.txt")<<"Actual pathway mutual masking/pool:10 bands,71520 values numerically exact.\n";std::cout<<"pool cases="<<cases<<" values="<<total<<" mismatches=0\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
