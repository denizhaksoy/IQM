#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <IQM/base/vulkan_runtime.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

using IQM::GPU::VulkanRuntime;
using IQM::Bin::VulkanResource;
struct Buffer {
    vk::raii::DeviceMemory memory=nullptr;
    vk::raii::Buffer handle=nullptr;
};
static Buffer buffer(const IQM::Bin::VulkanInstance& instance,unsigned bytes) {
    auto allocation=VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),bytes,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
    Buffer b; b.handle=std::move(allocation.first); b.memory=std::move(allocation.second);
    b.handle.bindMemory(b.memory,0); return b;
}
static std::vector<float> read(const std::filesystem::path& path,unsigned& batches,unsigned& n) {
    std::ifstream f(path); unsigned width;
    if(!(f>>batches>>width) || width%2 || !batches) throw std::runtime_error("Invalid input dimensions");
    n=width/2; std::vector<float> values(batches*width);
    for(float& v:values) if(!(f>>v) || !std::isfinite(v)) throw std::runtime_error("Invalid input value");
    return values;
}
int main(int argc,char** argv) {
    if(argc!=3 && argc!=4) { std::cerr<<"Usage: IQM-fft-codelet-test <reference dir> <shader.spv> [--second-axis|--inverse2d|--r2c|--forward-second|--forward-tail|--forward2d]\n"; return 2; }
    const bool second=argc==4 && std::string(argv[3])=="--second-axis";
    const bool size64=argc==4 && (std::string(argv[3])=="--inverse64"||std::string(argv[3])=="--forward64");
    const bool full=argc==4 && (std::string(argv[3])=="--inverse2d"||std::string(argv[3])=="--inverse64");
    const bool realForward=argc==4 && std::string(argv[3])=="--r2c";
    const bool forwardSecond=argc==4 && std::string(argv[3])=="--forward-second";
    const bool forwardTail=argc==4 && std::string(argv[3])=="--forward-tail";
    const bool forwardFull=argc==4 && (std::string(argv[3])=="--forward2d"||std::string(argv[3])=="--forward64");
    const bool math=argc==4 && std::string(argv[3])=="--float-math";
    const bool multiply=argc==4 && std::string(argv[3])=="--float-multiply";
    if(argc==4 && !second && !full && !realForward && !forwardSecond && !forwardTail && !forwardFull && !math && !multiply) return 2;
    const bool twoPass=full||forwardFull;
    const unsigned selectedGraph=multiply?11:math?10:forwardFull?8:forwardTail?7:forwardSecond?6:realForward?5:full?3:second?2:1;
    try {
        IQM::Bin::VulkanInstance instance(true);
        const auto& device=*instance.device();
        std::ifstream shaderFile(argv[2],std::ios::binary|std::ios::ate);
        const auto bytes=shaderFile.tellg();
        if(bytes<=0 || bytes%4) throw std::runtime_error("Invalid shader binary");
        std::vector<uint32_t> code(static_cast<size_t>(bytes)/4); shaderFile.seekg(0);
        if(!shaderFile.read(reinterpret_cast<char*>(code.data()),bytes)) throw std::runtime_error("Truncated shader");
        auto module=VulkanRuntime::createShaderModule(device,code);
        auto pool=VulkanRuntime::createDescPool(device,2,{{vk::DescriptorType::eStorageBuffer,4}});
        auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
        auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(16));
        auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
        vk::DescriptorSetLayout rawLayout=*setLayout;
        const vk::DescriptorSetLayout rawLayouts[]={rawLayout,rawLayout};
        auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=2,.pSetLayouts=rawLayouts});
        bool selectedPathMatches=true;
        for(unsigned size:{8u,16u,64u,128u}) {
            if(selectedGraph==1 && size==64) continue;
            if(selectedGraph!=1 && size!=(size64?64u:128u)) continue;
            unsigned batches,n,refBatches,refN;
            auto input=read(std::filesystem::path(argv[1])/("input"+std::to_string(size)+".txt"),batches,n);
            const auto expected=read(std::filesystem::path(argv[1])/((selectedGraph>=5?"forward":"inverse")+std::to_string(size)+".txt"),refBatches,refN);
            if(n!=size || refN!=n || refBatches!=batches) throw std::runtime_error("Reference shape mismatch");
            if(twoPass && batches%size) throw std::runtime_error("2D fixtures must contain complete square matrices");
            if(realForward || forwardFull) for(size_t i=1;i<input.size();i+=2)
                if(input[i]!=0) throw std::runtime_error("Real forward FFT requires zero imaginary input");
            const unsigned bufferBytes=static_cast<unsigned>(input.size()*sizeof(float));
            auto in=buffer(instance,bufferBytes),out=buffer(instance,bufferBytes),temp=buffer(instance,bufferBytes);
            void* mapped=in.memory.mapMemory(0,bufferBytes); std::memcpy(mapped,input.data(),bufferBytes); in.memory.unmapMemory();
            std::vector<vk::DescriptorBufferInfo> inputInfo{{*in.handle,0,bufferBytes}},outputInfo{{*out.handle,0,bufferBytes}},tempInfo{{*temp.handle,0,bufferBytes}};
            device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,inputInfo),VulkanRuntime::createWriteSet(*sets[0],1,twoPass?tempInfo:outputInfo),VulkanRuntime::createWriteSet(*sets[1],0,tempInfo),VulkanRuntime::createWriteSet(*sets[1],1,outputInfo)},{});
            bool selectedSizeMatches=false;
            for(unsigned graph=selectedGraph==1?0:selectedGraph;graph<(selectedGraph==1?2u:selectedGraph+1);++graph) for(unsigned fused=0;fused<(selectedGraph==1?3u:1u);++fused) {
                instance.cmdBuf()->reset();
                instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
                vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
                instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
                instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);
                instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});
                struct Push {uint32_t n,batches,graph,fused;} push{n,batches,graph,fused};
                instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,push);
                instance.cmdBuf()->dispatch((batches+31)/32,1,1);
                if(twoPass) {
                    vk::MemoryBarrier between{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
                    instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader,{}, {between},{},{});
                    instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[1]},{});
                    push.graph=forwardFull?9:4;
                    if(forwardFull) push.batches=(batches/size)*(size/2+1);
                    instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,push);
                    instance.cmdBuf()->dispatch((push.batches+31)/32,1,1);
                }
                vk::MemoryBarrier after{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};
                instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eHost,{}, {after},{},{});
                instance.cmdBuf()->end();
                vk::CommandBuffer cmd=*instance.cmdBuf(); vk::SubmitInfo submit{.commandBufferCount=1,.pCommandBuffers=&cmd};
                vk::raii::Fence fence(device,vk::FenceCreateInfo{}); instance.queue()->submit(submit,fence); instance.waitForFence(fence);
                const float* actual=static_cast<const float*>(out.memory.mapMemory(0,bufferBytes));
                size_t mismatches=0,first=0; double maxError=0,sumSquared=0;
                size_t mathCounts[2]={0,0},exportCounts[2]={0,0};
                for(size_t i=0;i<expected.size();++i) {
                    if(!std::isfinite(actual[i])) throw std::runtime_error("Nonfinite GPU output");
                    const double delta=static_cast<double>(actual[i])-expected[i];
                    if(delta!=0) {
                        if(math) {
                            ++mathCounts[i%2];
                            if(i/2>=4 && i/2<24580) ++exportCounts[i%2];
                            if(mismatches<8) std::cout<<std::setprecision(17)<<"math index="<<i/2<<" component="<<i%2<<" input="<<input[i]<<" actual="<<actual[i]<<" expected="<<expected[i]<<'\n';
                        }
                        if(!mismatches) first=i; ++mismatches;
                    }
                    maxError=std::max(maxError,std::abs(delta)); sumSquared+=delta*delta;
                }
                out.memory.unmapMemory();
                if(math) std::cout<<"math log mismatches="<<mathCounts[0]<<" exported="<<exportCounts[0]
                    <<" exp mismatches="<<mathCounts[1]<<" exported="<<exportCounts[1]<<'\n';
                if(graph==selectedGraph && fused==0) selectedSizeMatches=mismatches==0;
                std::cout<<std::setprecision(17)<<(selectedGraph>=5?"forward n=":"inverse n=")<<n<<" batches="<<batches<<" graph="<<graph<<" fused="<<fused
                    <<" max="<<maxError<<" RMSE="<<std::sqrt(sumSquared/expected.size())<<" mismatches="<<mismatches<<" first="<<first<<'\n';
            }
            selectedPathMatches&=selectedSizeMatches;
        }
        std::cout<<"Selected FFT path graph="<<selectedGraph<<" fused=0: "<<(selectedPathMatches?"PASS":"FAIL")<<'\n';
        return selectedPathMatches?0:1;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 2;}
}
