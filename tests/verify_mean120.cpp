#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <IQM/base/vulkan_runtime.h>
#include <bit>
#include <algorithm>
#include <cmath>
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
    std::vector<uint32_t> data(size_t(h)*w);for(auto& v:data){if(!(file>>v))throw std::runtime_error("Invalid native word");}
    return data;
}
int main(int argc,char** argv){try{
    if(argc!=4)throw std::runtime_error("Usage:shader.spv native-pairs-manifest fresh-output");
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
    auto check=[&](const std::string& tag,const std::vector<uint32_t>& input,const std::vector<uint32_t>& expected,bool syntheticBatch){
        if(input.empty()||input.size()%120||expected.size()!=input.size()/120*121)throw std::runtime_error("Fixture count mismatch");
        for(size_t offset=0;offset<input.size();offset+=120){
            int minimum=1000,maximum=-1000;double sum=0;
            for(size_t i=0;i<120;++i){float v=std::bit_cast<float>(input[offset+i]);
                if(!std::isfinite(v))throw std::runtime_error("Nonfinite mean input");
                if(v!=0){int e=std::ilogb(std::abs(v));minimum=std::min(minimum,e);maximum=std::max(maximum,e);}
                sum+=double(v);
            }
            if(maximum!=-1000&&(minimum < -90||maximum>100||maximum-minimum>16))throw std::runtime_error("Mean120 exponent domain exceeded");
            volatile double divisor=120.0;float mean=float(sum/divisor);
            size_t base=offset/120*121;
            if(std::bit_cast<uint32_t>(mean)!=expected[base])throw std::runtime_error("Native mean differs from double-accumulation contract");
            for(size_t i=0;i<120;++i){volatile float v=std::bit_cast<float>(input[offset+i]);float centered=v-mean;
                if(std::bit_cast<uint32_t>(centered)!=expected[base+1+i])throw std::runtime_error("Native DC contract differs");}
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
        struct Push{uint32_t count,a,b,c;};instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,Push{uint32_t(input.size()/120),0,0,0});instance.cmdBuf()->dispatch((uint32_t(input.size()/120)+63)/64,1,1);
        vk::MemoryBarrier after{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};
        instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eHost,{}, {after},{},{});
        instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);
        const auto* actual=static_cast<const uint32_t*>(dst.memory.mapMemory(0,bytes));size_t numeric=0,bits=0;
        for(size_t i=0;i<expected.size();++i){numeric+=std::bit_cast<float>(actual[i])!=std::bit_cast<float>(expected[i]);bits+=actual[i]!=expected[i];}
        std::ofstream dump(output/(tag+".u32"),std::ios::binary);dump.write(reinterpret_cast<const char*>(actual),bytes);
        dst.memory.unmapMemory();instance.cmdBuf()->reset();results<<tag<<'\t'<<expected.size()<<'\t'<<numeric<<'\t'<<bits<<'\n';results.flush();
        if(numeric||bits)throw std::runtime_error("Mean/DC mismatch:"+tag);
        if(syntheticBatch)total+=expected.size();else nativeTotal+=expected.size();++batches;
    };
    std::vector<uint32_t> synthetic,expected;uint32_t rng=0x120dc017u;
    for(unsigned group=0;group<8192;++group){
        std::vector<float> row;double sum=0;int low=-90+int(group%175);
        for(unsigned i=0;i<120;++i){
            rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;
            uint32_t word=(rng&0x807fffffu)|(uint32_t(low+int(rng%17)+127)<<23);
            if(group==0)word=0;if(group==1)word=0x80000000u;
            if(group==2)word=0x3f800000u;
            if(group==3)word=(i&1?0xbf800000u:0x3f800000u);
            float v=std::bit_cast<float>(word);row.push_back(v);synthetic.push_back(word);sum+=double(v);
        }
        volatile double divisor=120.0;float mean=float(sum/divisor);expected.push_back(std::bit_cast<uint32_t>(mean));
        for(float v:row){volatile float x=v;float centered=x-mean;expected.push_back(std::bit_cast<uint32_t>(centered));}
    }
    std::ofstream(output/"synthetic_input.u32",std::ios::binary).write(reinterpret_cast<const char*>(synthetic.data()),std::streamsize(synthetic.size()*4));
    std::ofstream(output/"synthetic_expected.u32",std::ios::binary).write(reinterpret_cast<const char*>(expected.data()),std::streamsize(expected.size()*4));
    check("synthetic",synthetic,expected,true);
    std::ifstream manifest(argv[2]);if(!manifest)throw std::runtime_error("Missing native manifest");std::string tag,inputPath,expectedPath;
    while(manifest>>tag>>std::quoted(inputPath)>>std::quoted(expectedPath))check(tag,read(inputPath),read(expectedPath),false);
    if(!manifest.eof()||batches!=13||nativeTotal!=1452)throw std::runtime_error("Native coverage incomplete");
    std::cout<<"mean120/DC synthetic_bitwise="<<total<<" native_bitwise="<<nativeTotal<<" mismatches=0\n";
    std::ofstream(output/"PASS.txt")<<"Mean120 and DC subtraction exact on bounded synthetic probes and12 official SPYR residual fixtures.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
