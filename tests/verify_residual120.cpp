#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <IQM/base/vulkan_runtime.h>
#include <bit>
#include <cfenv>
#include <cmath>
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
    const bool pathway=argc==5&&std::string(argv[4])=="--pathway";
    const bool extended=argc==5&&(std::string(argv[4])=="--extended"||pathway);
    if(argc!=4&&!extended)throw std::runtime_error("Usage:shader.spv native-pairs-manifest fresh-output [--extended]");
    if(std::fegetround()!=FE_TONEAREST)throw std::runtime_error("Host needs RTE");
#if defined(_M_IX86)||defined(_M_X64)||defined(__i386__)||defined(__x86_64__)
    if(_mm_getcsr()&((1u<<15)|(1u<<6)))throw std::runtime_error("Host needs gradual underflow");
#endif
    const std::filesystem::path output(argv[3]);if(std::filesystem::exists(output)||!std::filesystem::create_directories(output))throw std::runtime_error("Fresh output required");
    std::ifstream shader(argv[1],std::ios::binary|std::ios::ate);auto length=shader.tellg();if(length<=0||length%4)throw std::runtime_error("Invalid shader");
    std::vector<uint32_t> code(size_t(length)/4);shader.seekg(0);shader.read(reinterpret_cast<char*>(code.data()),length);if(!shader||code[0]!=0x07230203u)throw std::runtime_error("Invalid SPIR-V");
    IQM::Bin::VulkanInstance instance(true);const auto& device=*instance.device();
    const auto properties=instance.physicalDevice()->getProperties2<vk::PhysicalDeviceProperties2,vk::PhysicalDeviceFloatControlsProperties>();
    const auto& controls=properties.get<vk::PhysicalDeviceFloatControlsProperties>();
    if(!controls.shaderDenormPreserveFloat32||!controls.shaderRoundingModeRTEFloat32)throw std::runtime_error("Required Float32 modes unsupported");
    auto module=VulkanRuntime::createShaderModule(device,code);
    auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
    auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(16));auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
    std::ofstream results(output/"results.tsv");results<<"case\tvalues\tnumeric_mismatches\tbit_mismatches\n";size_t total=0,nativeTotal=0,batches=0;
    auto check=[&](const std::string& tag,const std::vector<uint32_t>& input,const std::vector<uint32_t>& expected,unsigned h,unsigned w){
        if(input.empty()||input.size()%240||expected.size()!=input.size()/240*(extended?361:240)||!((h==10&&w==12)||(h==12&&w==10)))throw std::runtime_error("Fixture count mismatch");
        if(extended)for(size_t batch=0;batch<input.size()/240;++batch)for(size_t k=0;k<240;++k){
            float v=std::bit_cast<float>(expected[batch*361+k]);
            if(!std::isfinite(v)||(v!=0&&(std::ilogb(std::abs(v)) < -90||std::ilogb(std::abs(v))>100)))throw std::runtime_error("Hypot domain exceeded");
        }
        unsigned bytes=unsigned(expected.size()*4);auto src=buffer(instance,bytes),dst=buffer(instance,bytes);
        void* mapped=src.memory.mapMemory(0,bytes);std::memcpy(mapped,input.data(),input.size()*4);src.memory.unmapMemory();
        auto pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,2}});const vk::DescriptorSetLayout raw=*setLayout;
        auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});
        const std::vector<vk::DescriptorBufferInfo> a{{*src.handle,0,bytes}},b{{*dst.handle,0,bytes}};
        device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,a),VulkanRuntime::createWriteSet(*sets[0],1,b)},{});
        instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
        instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
        instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});
        struct Push{uint32_t count,a,b,c;};instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,Push{uint32_t(input.size()/240),h,w,0});instance.cmdBuf()->dispatch((uint32_t(input.size()/240)+63)/64,1,1);
        vk::MemoryBarrier after{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};
        instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eHost,{}, {after},{},{});
        instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);
        const auto* actual=static_cast<const uint32_t*>(dst.memory.mapMemory(0,bytes));size_t numeric=0,bits=0,magBits=0;
        for(size_t i=0;i<expected.size();++i){numeric+=std::bit_cast<float>(actual[i])!=std::bit_cast<float>(expected[i]);bits+=actual[i]!=expected[i];
            if(extended&&i%361>=240&&actual[i]!=expected[i])++magBits;}
        std::ofstream dump(output/(tag+".u32"),std::ios::binary);dump.write(reinterpret_cast<const char*>(actual),bytes);
        dst.memory.unmapMemory();instance.cmdBuf()->reset();results<<tag<<'\t'<<expected.size()<<'\t'<<numeric<<'\t'<<bits<<'\n';results.flush();
        if(magBits)throw std::runtime_error("Magnitude/index bit mismatch:"+tag+" count="+std::to_string(magBits));
        if(numeric)throw std::runtime_error("Residual FFT mismatch:"+tag);
        nativeTotal+=expected.size();total+=bits;++batches;
    };
    std::ifstream manifest(argv[2]);if(!manifest)throw std::runtime_error("Missing native manifest");std::string tag,inputPath,expectedPath;unsigned h,w;
    while(manifest>>tag>>h>>w>>std::quoted(inputPath)>>std::quoted(expectedPath))check(tag,read(inputPath),read(expectedPath),h,w);
    if(!manifest.eof()||batches!=2||nativeTotal!=(pathway?722u:extended?27436u:18240u))throw std::runtime_error("Native coverage incomplete");
    std::cout<<"residual120 numerical_values="<<nativeTotal<<" signed_zero_or_bit_differences="<<total<<" mismatches=0\n";
    std::ofstream(output/"PASS.txt")<<(pathway?"Two full-pathway base differences:480 FFT floats numerically exact;240 magnitudes and2 indices bitwise exact. FFT signed-zero bits excluded.\n":extended?"Residual10x12/12x10:76 matrices;18240 FFT floats numerically exact;9120 magnitudes and76 indices bitwise exact. FFT signed-zero bits not asserted.\n":"Residual10x12/12x10:76 matrices,18240 Float32 components numerically exact; signed-zero bits not asserted.\n");return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
