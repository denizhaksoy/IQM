#include <IQM/hdrvdp3/mixed_filter.h>
#include <IQM/hdrvdp3/mtf.h>
#include <IQM/base/vulkan_runtime.h>
#include <vulkan_res.h>
#include <vulkan.h>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <memory>
#include <stdexcept>
using IQM::GPU::VulkanRuntime;
using IQM::VulkanInstance;
using IQM::Bin::VulkanResource;
using Path=std::filesystem::path;
namespace {
struct Buffer{vk::raii::DeviceMemory memory=nullptr;vk::raii::Buffer handle=nullptr;unsigned bytes;};
static Buffer buffer(const VulkanInstance& instance,unsigned bytes){
    auto pair=VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),bytes,
        vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
    Buffer b;b.handle=std::move(pair.first);b.memory=std::move(pair.second);b.bytes=bytes;b.handle.bindMemory(b.memory,0);return b;
}
static void upload(Buffer& b,const std::vector<float>& data){
    if(data.size()*4!=b.bytes)throw std::runtime_error("Upload size");auto* p=b.memory.mapMemory(0,b.bytes);std::memcpy(p,data.data(),b.bytes);b.memory.unmapMemory();
}
struct Kernel{
    vk::raii::DescriptorSetLayout setLayout=nullptr;vk::raii::DescriptorPool pool=nullptr;
    vk::raii::PipelineLayout layout=nullptr;vk::raii::Pipeline pipeline=nullptr;vk::raii::DescriptorSet set=nullptr;
    Kernel(const VulkanInstance& instance,const Path& path,const std::vector<vk::DescriptorType>& types,unsigned pushBytes=16){
        const auto& device=*instance.device();std::ifstream f(path,std::ios::binary|std::ios::ate);auto size=f.tellg();
        if(size<=0||size%4)throw std::runtime_error("Invalid shader:"+path.string());std::vector<uint32_t> code(size_t(size)/4);
        f.seekg(0);if(!f.read(reinterpret_cast<char*>(code.data()),size))throw std::runtime_error("Truncated shader");
        auto module=VulkanRuntime::createShaderModule(device,code);
        std::vector<std::pair<vk::DescriptorType,unsigned>> bindings;
        for(auto t:types)bindings.emplace_back(t,1);
        setLayout=VulkanRuntime::createDescLayout(device,bindings);
        pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,3},{vk::DescriptorType::eStorageImage,2}});
        layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(pushBytes));
        pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
        vk::DescriptorSetLayout raw=*setLayout;
        auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});set=std::move(sets[0]);
    }
    void bind(const VulkanInstance& instance,unsigned binding,Buffer& b){
        const std::vector<vk::DescriptorBufferInfo> info{{*b.handle,0,b.bytes}};
        instance.device()->updateDescriptorSets({VulkanRuntime::createWriteSet(*set,binding,info)},{});
    }
    void run(vk::CommandBuffer cmd,std::array<uint32_t,4> push,unsigned x,unsigned y=1){
        cmd.bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*set},{});
        cmd.pushConstants(*layout,vk::ShaderStageFlagBits::eCompute,0,16,push.data());cmd.dispatch(x,y,1);
        vk::MemoryBarrier barrier{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eHostRead|vk::AccessFlagBits::eTransferRead};
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eHost|vk::PipelineStageFlagBits::eTransfer,{}, {barrier},{},{});
    }
};
}
namespace IQM::HDRVDP3 {
MixedFilterResult
#if defined(IQM_HDRVDP3_SQUARE_FILTER_DIAGNOSTIC)
run_square_filter_diagnostic(const IQM::VulkanInstance& instance,
#else
run_mixed_filter(const IQM::VulkanInstance& instance,
#endif
 const std::vector<float>& input,unsigned w,unsigned h,MixedFilterMode mode,
 const MixedFilterAssets& assets,bool trace) {
#if defined(IQM_HDRVDP3_SQUARE128_FILTER_DIAGNOSTIC)
 const bool reference256=true;
 const bool square=h==128&&w==128;
#else
 const bool reference256=assets.reference256Graph;
 const bool square=(h==80&&w==80)||(h==96&&w==96)||(h==128&&w==128);
#endif
 if(!((h==80&&w==96)||(h==96&&w==80)||square)||input.size()!=size_t(h)*w)
 throw std::invalid_argument("Unvalidated mixed filter dimensions");
 if(reference256!=(h==128&&w==128))throw std::invalid_argument("FFT asset ABI does not match geometry");
 if(mode!=MixedFilterMode::OpticalMTF&&mode!=MixedFilterMode::LocalAdaptation&&mode!=MixedFilterMode::OpticalRGB)
 throw std::invalid_argument("Invalid mixed filter mode");
 for(float v:input)if(!std::isfinite(v)||(mode==MixedFilterMode::OpticalRGB?v<0:v<=0))
 throw std::invalid_argument("Invalid input domain for mixed filter mode");
 const unsigned fh=2*h,fw=2*w,n=fh*fw,pixels=h*w;
 const bool mtf=mode!=MixedFilterMode::LocalAdaptation;
std::vector<float> filter(n);
 if(mtf)IQM::HDRVDP3::generate_mtf_filter(filter.data(),fw,fh,30);
 else{const double sigma=std::pow(10.0,-0.781367)*30;for(unsigned y=0;y<fh;++y)for(unsigned x=0;x<fw;++x){const double kx=(std::fmod(0.5+double(x)/fw,1.0)-0.5)*(2*3.14159265358979323846),ky=(std::fmod(0.5+double(y)/fh,1.0)-0.5)*(2*3.14159265358979323846);filter[y*fw+x]=float(std::exp(-0.5*(kx*kx+ky*ky)*sigma*sigma));}}
 std::vector<float> forwardInitial(3*n,0);for(unsigned y=0;y<fh;++y)for(unsigned x=0;x<fw;++x)forwardInitial[2*n+x*fh+y]=filter[y*fw+x];
 const auto& device=*instance.device();const auto props=instance.physicalDevice()->getProperties2<vk::PhysicalDeviceProperties2,vk::PhysicalDeviceFloatControlsProperties>();const auto& controls=props.get<vk::PhysicalDeviceFloatControlsProperties>();if(!controls.shaderDenormPreserveFloat32||!controls.shaderRoundingModeRTEFloat32)throw std::runtime_error("Float32 modes unsupported");
 auto original=buffer(instance,pixels*4),prepared=buffer(instance,n*8),scratch=buffer(instance,n*8),forward=buffer(instance,n*12),filtered=buffer(instance,n*8),inverseScratch=buffer(instance,n*8),inverse=buffer(instance,n*8),result=buffer(instance,pixels*4);upload(original,input);upload(forward,forwardInitial);
 const auto sb=vk::DescriptorType::eStorageBuffer;Kernel prep(instance,assets.io,{sb,sb}),f0(instance,assets.forward,{sb,sb}),f1(instance,assets.forward,{sb,sb}),mul(instance,assets.io,{sb,sb}),i0(instance,assets.inverse,{sb,sb}),i1(instance,assets.inverse,{sb,sb}),finish(instance,assets.io,{sb,sb});
 prep.bind(instance,0,original);prep.bind(instance,1,prepared);f0.bind(instance,0,prepared);f0.bind(instance,1,scratch);f1.bind(instance,0,scratch);f1.bind(instance,1,forward);mul.bind(instance,0,forward);mul.bind(instance,1,filtered);i0.bind(instance,0,filtered);i0.bind(instance,1,inverseScratch);i1.bind(instance,0,inverseScratch);i1.bind(instance,1,inverse);finish.bind(instance,0,inverse);finish.bind(instance,1,result);
 std::unique_ptr<Kernel> narrowForward,narrowFiltered,narrowInverse;
 if(square){
  narrowForward=std::make_unique<Kernel>(instance,assets.io,std::vector<vk::DescriptorType>{sb,sb});
  narrowFiltered=std::make_unique<Kernel>(instance,assets.io,std::vector<vk::DescriptorType>{sb,sb});
  narrowInverse=std::make_unique<Kernel>(instance,assets.io,std::vector<vk::DescriptorType>{sb,sb});
  narrowForward->bind(instance,0,forward);narrowForward->bind(instance,1,forward);
  narrowFiltered->bind(instance,0,filtered);narrowFiltered->bind(instance,1,filtered);
  narrowInverse->bind(instance,0,inverse);narrowInverse->bind(instance,1,inverse);
 }

 auto cmd=**instance.cmdBuf();cmd.begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};cmd.pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
const unsigned forwardFirst=reference256?8:3,forwardSecond=reference256?9:4;
 prep.run(cmd,{mtf?0u:1u,h,w,n},(n+63)/64);f0.run(cmd,{fh,fw,forwardFirst,0},(fw+31)/32);f1.run(cmd,{fw,fh/2+1,forwardSecond,0},(fh/2+32)/32);
 if(square)narrowForward->run(cmd,{5,fh,fw,n},1);
 mul.run(cmd,{4,fh,fw,n},(n+63)/64);
 if(square)narrowFiltered->run(cmd,{5,fh,fw,n},1);
 i0.run(cmd,{fh,fw,3,0},(fw+31)/32);i1.run(cmd,{fw,fh,4,1},(fh+31)/32);
 if(square)narrowInverse->run(cmd,{5,fh,fw,n},1);
 finish.run(cmd,{mtf?2u:3u,h,w,pixels},(pixels+63)/64);
 cmd.end();vk::CommandBuffer raw=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&raw},fence);instance.waitForFence(fence);

 auto download=[](Buffer& b,unsigned rows,unsigned cols,bool transpose){
 std::vector<float> values(size_t(rows)*cols);
 const auto* data=static_cast<const float*>(b.memory.mapMemory(0,b.bytes));
 for(size_t i=0;i<values.size();++i){size_t j=i;if(transpose){const size_t k=i/2;j=2*((k%(cols/2))*rows+k/(cols/2))+i%2;}values[i]=data[j];}
 b.memory.unmapMemory();return values;};
 MixedFilterResult out;out.output=download(result,h,w,false);
 if(trace){out.prepared=download(prepared,fh,2*fw,false);out.forward=download(forward,fh,2*fw,true);out.filtered=download(filtered,fh,2*fw,false);out.inverse=download(inverse,fh,2*fw,true);out.modelFilter=filter;}
 return out;
}
}
