// Isolated128-pixel MTF/LA chain. Does not change the bounded public API.
#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <IQM/base/vulkan_runtime.h>
#include <IQM/hdrvdp3/mtf.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>
using IQM::GPU::VulkanRuntime;
using IQM::Bin::VulkanInstance;
using IQM::Bin::VulkanResource;
using Path=std::filesystem::path;
struct Buffer{vk::raii::DeviceMemory memory=nullptr;vk::raii::Buffer handle=nullptr;unsigned bytes;};
static Buffer buffer(const VulkanInstance& instance,unsigned bytes){
    auto pair=VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),bytes,
        vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
    Buffer b;b.handle=std::move(pair.first);b.memory=std::move(pair.second);b.bytes=bytes;b.handle.bindMemory(b.memory,0);return b;
}
static std::vector<float> read(const Path& path,unsigned h,unsigned w){
    std::ifstream f(path);unsigned rh,rw;if(!(f>>rh>>rw)||rh!=h||rw!=w)throw std::runtime_error("Invalid shape:"+path.string());
    std::vector<float> result(h*w);for(auto& v:result){double d;if(!(f>>d)||!std::isfinite(d))throw std::runtime_error("Invalid input");v=float(d);}
    std::string extra;if(f>>extra)throw std::runtime_error("Trailing values");return result;
}
static void upload(Buffer& b,const std::vector<float>& data){
    if(data.size()*4!=b.bytes)throw std::runtime_error("Upload size");auto* p=b.memory.mapMemory(0,b.bytes);std::memcpy(p,data.data(),b.bytes);b.memory.unmapMemory();
}
struct Kernel{
    vk::raii::DescriptorSetLayout setLayout=nullptr;vk::raii::DescriptorPool pool=nullptr;
    vk::raii::PipelineLayout layout=nullptr;vk::raii::Pipeline pipeline=nullptr;vk::raii::DescriptorSet set=nullptr;
    Kernel(const VulkanInstance& instance,const Path& path,const std::vector<vk::DescriptorType>& types){
        const auto& device=*instance.device();std::ifstream f(path,std::ios::binary|std::ios::ate);auto size=f.tellg();
        if(size<=0||size%4)throw std::runtime_error("Invalid shader:"+path.string());std::vector<uint32_t> code(size_t(size)/4);
        f.seekg(0);if(!f.read(reinterpret_cast<char*>(code.data()),size))throw std::runtime_error("Truncated shader");
        auto module=VulkanRuntime::createShaderModule(device,code);
        std::vector<std::pair<vk::DescriptorType,unsigned>> bindings;
        for(auto t:types)bindings.emplace_back(t,1);
        setLayout=VulkanRuntime::createDescLayout(device,bindings);
        pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,3},{vk::DescriptorType::eStorageImage,2}});
        layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(16));
        pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
        vk::DescriptorSetLayout raw=*setLayout;
        auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});set=std::move(sets[0]);
    }
    void bind(const VulkanInstance& instance,unsigned binding,Buffer& b){
        const std::vector<vk::DescriptorBufferInfo> info{{*b.handle,0,b.bytes}};
        instance.device()->updateDescriptorSets({VulkanRuntime::createWriteSet(*set,binding,info)},{});
    }
    void bind(const VulkanInstance& instance,unsigned binding,const vk::raii::ImageView& image){
        const auto info=VulkanRuntime::createImageInfos({&image});instance.device()->updateDescriptorSets({VulkanRuntime::createWriteSet(*set,binding,info)},{});
    }
    void run(vk::CommandBuffer cmd,std::array<uint32_t,4> push,unsigned x,unsigned y=1){
        cmd.bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*set},{});
        cmd.pushConstants(*layout,vk::ShaderStageFlagBits::eCompute,0,16,push.data());cmd.dispatch(x,y,1);
        vk::MemoryBarrier barrier{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eHostRead|vk::AccessFlagBits::eTransferRead};
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eHost|vk::PipelineStageFlagBits::eTransfer,{}, {barrier},{},{});
    }
};
static size_t compare(Buffer& b,const std::vector<float>& expected,const Path& dump,unsigned h,unsigned w,bool transpose){
    const auto* actual=static_cast<const float*>(b.memory.mapMemory(0,b.bytes));size_t bad=0;double maximum=0;
    std::ofstream f(dump);f<<h<<' '<<w<<'\n'<<std::setprecision(9);
    for(size_t i=0;i<expected.size();++i){
        const auto ci=i/2;const size_t index=transpose?2*((ci%256)*256+ci/256)+i%2:i;
        const float v=actual[index];f<<v<<'\n';
        if(!std::isfinite(v)||v!=expected[i]){if(bad<3)std::cout<<dump.filename()<<" index="<<i<<" gpu="<<v<<" native="<<expected[i]<<'\n';++bad;maximum=std::max(maximum,std::abs(double(v)-expected[i]));}
    }b.memory.unmapMemory();std::cout<<dump.filename()<<" values="<<expected.size()<<" mismatches="<<bad<<" max="<<maximum<<'\n';return bad;
}
static int base16(const Path& source,const Path& shader,const Path& output){
    if(!std::filesystem::create_directory(output))throw std::runtime_error("Fresh base16 export required");
    const auto input=read(source/"input.txt",64,512),expected=read(source/"expected.txt",64,769);
    VulkanInstance instance(true);const auto& device=*instance.device();
    const auto props=instance.physicalDevice()->getProperties2<vk::PhysicalDeviceProperties2,vk::PhysicalDeviceFloatControlsProperties>();
    const auto& controls=props.get<vk::PhysicalDeviceFloatControlsProperties>();
    if(!controls.shaderDenormPreserveFloat32||!controls.shaderRoundingModeRTEFloat32)throw std::runtime_error("Required float controls unsupported");
    auto in=buffer(instance,unsigned(input.size()*4)),out=buffer(instance,unsigned(expected.size()*4));upload(in,input);
    const auto sb=vk::DescriptorType::eStorageBuffer;Kernel kernel(instance,shader,{sb,sb});kernel.bind(instance,0,in);kernel.bind(instance,1,out);
    vk::CommandBuffer cmd=**instance.cmdBuf();cmd.begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    vk::MemoryBarrier host{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {host},{},{});
    kernel.run(cmd,{16,64,0,0},2);cmd.end();vk::SubmitInfo submit{.commandBufferCount=1,.pCommandBuffers=&cmd};
    vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(submit,fence);instance.waitForFence(fence);
    const auto bad=compare(out,expected,output/"base16_output.txt",64,769,false);
    std::cout<<"base16 values=49216 mismatches="<<bad<<'\n';return bad?1:0;
}
int main(int argc,char** argv){try{
    if(argc==5&&std::string(argv[1])=="--base16")return base16(argv[2],argv[3],argv[4]);
    if(argc!=8)throw std::runtime_error("Usage: fixture forward.spv inverse.spv math_dir ref|test mtf|la fresh_export");
    const Path source(argv[1]),math(argv[4]),output(argv[7]);const std::string label(argv[5]),mode(argv[6]);
    if((label!="ref"&&label!="test")||(mode!="mtf"&&mode!="la"))throw std::runtime_error("Invalid mode/label");
    if(!std::filesystem::create_directory(output))throw std::runtime_error("Fresh export directory required");
    const bool mtf=mode=="mtf";const std::string tag=label+"_"+mode;
    const auto input=read(source/(label+(mtf?"_luminance.txt":"_adapt_input.txt")),128,128);
    auto filter=read(source/(mode+"_filter_double.txt"),256,256);std::vector<float> generated(65536);
    if(mtf)IQM::HDRVDP3::generate_mtf_filter(generated.data(),256,256,30);
    else{
        const double sigma=std::pow(10.0,-0.781367)*30;
        for(unsigned y=0;y<256;++y)for(unsigned x=0;x<256;++x){
            const double kx=(std::fmod(0.5+double(x)/256,1.0)-0.5)*(2*3.14159265358979323846);
            const double ky=(std::fmod(0.5+double(y)/256,1.0)-0.5)*(2*3.14159265358979323846);
            generated[y*256+x]=float(std::exp(-0.5*(kx*kx+ky*ky)*sigma*sigma));
        }
    }
    size_t filterBad=0;for(size_t i=0;i<filter.size();++i)if(filter[i]!=generated[i])++filterBad;
    std::cout<<"filter float32 values=65536 mismatches="<<filterBad<<'\n';if(filterBad)throw std::runtime_error("Generated filter mismatch");
    VulkanInstance instance(true);const auto& device=*instance.device();
    const auto props=instance.physicalDevice()->getProperties2<vk::PhysicalDeviceProperties2,vk::PhysicalDeviceFloatControlsProperties>();
    const auto& controls=props.get<vk::PhysicalDeviceFloatControlsProperties>();
    if(!controls.shaderDenormPreserveFloat32||!controls.shaderRoundingModeRTEFloat32)throw std::runtime_error("Required float controls unsupported");
    auto imageInfo=vk::ImageCreateInfo{.imageType=vk::ImageType::e2D,.format=vk::Format::eR32Sfloat,.extent={128,128,1},.mipLevels=1,.arrayLayers=1,
        .samples=vk::SampleCountFlagBits::e1,.tiling=vk::ImageTiling::eOptimal,
        .usage=vk::ImageUsageFlagBits::eStorage|vk::ImageUsageFlagBits::eTransferSrc|vk::ImageUsageFlagBits::eTransferDst,.initialLayout=vk::ImageLayout::eUndefined};
    auto imageIn=VulkanResource::createImage(device,*instance.physicalDevice(),imageInfo),imageOut=VulkanResource::createImage(device,*instance.physicalDevice(),imageInfo);
    auto uploadBuffer=buffer(instance,65536),download=buffer(instance,65536),kernel=buffer(instance,262144);
    upload(uploadBuffer,input);upload(kernel,generated);
    auto prepared=buffer(instance,524288),scratch=buffer(instance,524288),forward=buffer(instance,524288),filtered=buffer(instance,524288),inverseScratch=buffer(instance,524288),inverse=buffer(instance,524288);
    const auto sb=vk::DescriptorType::eStorageBuffer,si=vk::DescriptorType::eStorageImage;
    Kernel prep(instance,math/"prepare.spv",{si,sb}),f0(instance,argv[2],{sb,sb}),f1(instance,argv[2],{sb,sb}),mul(instance,math/"multiply.spv",{sb,sb,sb}),i0(instance,argv[3],{sb,sb}),i1(instance,argv[3],{sb,sb}),finish(instance,math/"finish.spv",{sb,si});
    prep.bind(instance,0,imageIn.imageView);prep.bind(instance,1,prepared);
    f0.bind(instance,0,prepared);f0.bind(instance,1,scratch);f1.bind(instance,0,scratch);f1.bind(instance,1,forward);
    mul.bind(instance,0,forward);mul.bind(instance,1,kernel);mul.bind(instance,2,filtered);
    i0.bind(instance,0,filtered);i0.bind(instance,1,inverseScratch);i1.bind(instance,0,inverseScratch);i1.bind(instance,1,inverse);
    finish.bind(instance,0,inverse);finish.bind(instance,1,imageOut.imageView);
    vk::CommandBuffer cmd=**instance.cmdBuf();cmd.begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    const vk::ImageSubresourceRange range{vk::ImageAspectFlagBits::eColor,0,1,0,1};
    std::array<vk::ImageMemoryBarrier,2> init{};
    for(unsigned j=0;j<2;++j)init[j]=vk::ImageMemoryBarrier{.dstAccessMask=vk::AccessFlagBits::eTransferWrite|vk::AccessFlagBits::eShaderWrite,.oldLayout=vk::ImageLayout::eUndefined,.newLayout=vk::ImageLayout::eGeneral,
        .srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.image=j?*imageOut.image:*imageIn.image,.subresourceRange=range};
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,vk::PipelineStageFlagBits::eTransfer|vk::PipelineStageFlagBits::eComputeShader,{}, {},{},init);
    vk::BufferImageCopy copy{.imageSubresource={vk::ImageAspectFlagBits::eColor,0,0,1},.imageExtent={128,128,1}};
    cmd.copyBufferToImage(*uploadBuffer.handle,*imageIn.image,vk::ImageLayout::eGeneral,{copy});
    vk::MemoryBarrier uploaded{.srcAccessMask=vk::AccessFlagBits::eTransferWrite|vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer|vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {uploaded},{},{});
    const std::array<uint32_t,4> params{256,256,mtf?1u:0u,1};
    prep.run(cmd,params,16,16);f0.run(cmd,{256,256,3,0},8);f1.run(cmd,{256,129,4,0},5);
    mul.run(cmd,{256,256,0,0},16,16);i0.run(cmd,{256,256,3,0},8);i1.run(cmd,{256,256,4,0},8);finish.run(cmd,params,8,8);
    cmd.copyImageToBuffer(*imageOut.image,vk::ImageLayout::eGeneral,*download.handle,{copy});
    vk::MemoryBarrier downloaded{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{}, {downloaded},{},{});
    cmd.end();vk::SubmitInfo submit{.commandBufferCount=1,.pCommandBuffers=&cmd};vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(submit,fence);instance.waitForFence(fence);
    size_t bad=0;
    bad+=compare(prepared,read(source/(tag+"_stage0.txt"),256,512),output/"stage0.txt",256,512,false);
    bad+=compare(forward,read(source/(label+(mtf?"_native_forward.txt":"_la_native_forward.txt")),256,512),output/"stage1_row_major.txt",256,512,true);
    bad+=compare(filtered,read(source/(tag+"_stage2.txt"),256,512),output/"stage2.txt",256,512,false);
    bad+=compare(inverse,read(source/(label+(mtf?"_native_inverse.txt":"_la_native_inverse.txt")),256,512),output/"stage3_row_major.txt",256,512,true);
    bad+=compare(download,read(source/(label+(mtf?"_retinal.txt":"_adaptation.txt")),128,128),output/"output.txt",128,128,false);
    std::cout<<"filter_chain GPU values=540672 mismatches="<<bad<<'\n';return bad?1:0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
