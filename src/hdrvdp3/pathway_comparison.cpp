#include <IQM/hdrvdp3/pathway_comparison.h>
#include <IQM/hdrvdp3/resize.h>
#include <vulkan_res.h>
#include <IQM/base/vulkan_runtime.h>
#include <vulkan.h>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <memory>
#include <stdexcept>
using IQM::GPU::VulkanRuntime;
namespace {
struct Buffer{vk::raii::DeviceMemory memory=nullptr;vk::raii::Buffer handle=nullptr;};
static Buffer buffer(const IQM::VulkanInstance& instance,unsigned bytes){
    auto pair=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),bytes,
        vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
    Buffer b;b.handle=std::move(pair.first);b.memory=std::move(pair.second);b.handle.bindMemory(b.memory,0);return b;
}
static std::vector<uint32_t> code(const std::filesystem::path& path){
    std::ifstream f(path,std::ios::binary|std::ios::ate);auto length=f.tellg();if(length<=0||length%4)throw std::runtime_error("Invalid SPIR-V");
    std::vector<uint32_t> v(size_t(length)/4);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),length);
    if(!f||v[0]!=0x07230203u)throw std::runtime_error("Invalid shader words");return v;
}
static void plan(std::vector<uint32_t>& data,const IQM::HDRVDP3::ResizeAxisPlan& p,int n){
    for(int d=0;d<n;++d){if(p.normalization[d]!=1)throw std::runtime_error("Nonunit resize normalization");double absolute=0;for(int k=0;k<p.taps;++k){const auto i=size_t(d)*p.taps+k;double f=std::ldexp(p.weights[i],20);absolute+=std::abs(p.weights[i]);if(!std::isfinite(f)||f!=std::trunc(f)||std::abs(f)>1048576)throw std::runtime_error("Inexact Q20 weight");data.push_back(uint32_t(p.indices[i]));data.push_back(std::bit_cast<uint32_t>(int32_t(f)));}if(absolute>2)throw std::runtime_error("Accumulation bound exceeded");}
}
}
namespace IQM::HDRVDP3 {
#if defined(IQM_HDRVDP3_SQUARE_QUALITY_DIAGNOSTIC)
#define run_pathway_comparison run_square_comparison_diagnostic
#endif
static PathwayComparisonResult comparison_impl(const IQM::VulkanInstance& instance,const std::vector<float>& ref,const std::vector<float>& tst,unsigned w,unsigned h,unsigned nw,unsigned nh,const std::filesystem::path& shader,unsigned channels){
#if defined(IQM_HDRVDP3_SQUARE128_QUALITY_DIAGNOSTIC)
 const bool shapeOK=h==128&&w==128;
#elif defined(IQM_HDRVDP3_SQUARE80_QUALITY_DIAGNOSTIC)
 const bool shapeOK=h==80&&w==80;
#elif defined(IQM_HDRVDP3_SQUARE_QUALITY_DIAGNOSTIC)
 const bool shapeOK=h==96&&w==96;
#else
 const bool shapeOK=((h==128&&w==128)||(h==80&&w==80)||(h==96&&w==96)||(h==80&&w==96)||(h==96&&w==80));
#endif
 if(!shapeOK||!nh||!nw||h%nh||w%nw||h/nh!=w/nw||!(h/nh==1||h/nh==2||h/nh==4||h/nh==8))throw std::invalid_argument("Unvalidated comparison shape");
 const unsigned n=h*w,first=h*nw,last=nh*nw;if(ref.size()!=channels*n||tst.size()!=channels*n)throw std::invalid_argument("Comparison input size");
 std::vector<uint32_t> reference,test;for(unsigned i=0;i<channels*n;++i){if(!std::isfinite(ref[i])||(channels==3?ref[i]<0:ref[i]<=0)||!std::isfinite(tst[i])||(channels==3&&tst[i]<0))throw std::invalid_argument("Comparison input outside domain");reference.push_back(std::bit_cast<uint32_t>(ref[i]));test.push_back(std::bit_cast<uint32_t>(tst[i]));}
const auto& device=*instance.device();const auto props=instance.physicalDevice()->getProperties2<vk::PhysicalDeviceProperties2,vk::PhysicalDeviceFloatControlsProperties>();const auto& controls=props.get<vk::PhysicalDeviceFloatControlsProperties>();if(!controls.shaderDenormPreserveFloat32||!controls.shaderRoundingModeRTEFloat32)throw std::runtime_error("Float32 modes unsupported");
 auto module=VulkanRuntime::createShaderModule(device,code(shader));auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(32));auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
  auto hp=IQM::HDRVDP3::make_resize_axis_plan(w,nw),vp=IQM::HDRVDP3::make_resize_axis_plan(h,nh);std::array<std::vector<uint32_t>,4> data;data[0]=reference;data[0].insert(data[0].end(),test.begin(),test.end());data[1].resize(n);plan(data[1],hp,nw);data[2].resize(first);plan(data[2],vp,nh);data[3].resize(2*last);
  std::vector<Buffer> buffers;for(auto& v:data){buffers.push_back(buffer(instance,unsigned(v.size()*4)));void* p=buffers.back().memory.mapMemory(0,v.size()*4);std::memcpy(p,v.data(),v.size()*4);buffers.back().memory.unmapMemory();}
  auto pool=VulkanRuntime::createDescPool(device,3,{{vk::DescriptorType::eStorageBuffer,6}});const vk::DescriptorSetLayout raw[]={*setLayout,*setLayout,*setLayout};auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=3,.pSetLayouts=raw});for(unsigned j=0;j<3;++j){const std::vector<vk::DescriptorBufferInfo>a{{*buffers[j].handle,0,data[j].size()*4}},b{{*buffers[j+1].handle,0,data[j+1].size()*4}};device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[j],0,a),VulkanRuntime::createWriteSet(*sets[j],1,b)},{});}
  instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
  struct Push{uint32_t count,mode,w,h,nw,nh,taps,offset;};const Push pushes[]={{n,channels==3?3u:0u,w,h,w,h,0,0},{first,1,w,h,nw,h,uint32_t(hp.taps),n},{last,2,nw,h,nw,nh,uint32_t(vp.taps),first}};
  for(unsigned j=0;j<3;++j){instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[j]},{});instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,pushes[j]);instance.cmdBuf()->dispatch((pushes[j].count+63)/64,1,1);vk::MemoryBarrier barrier{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eHostRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eHost,{}, {barrier},{},{});}
  instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);

 PathwayComparisonResult result;result.binary.resize(n);result.resized.resize(last);
 const auto* mask=static_cast<const uint32_t*>(buffers[1].memory.mapMemory(0,n*4));std::memcpy(result.binary.data(),mask,n*4);buffers[1].memory.unmapMemory();
 const auto* actual=static_cast<const uint32_t*>(buffers[3].memory.mapMemory(0,last*8));for(unsigned i=0;i<last;++i)result.resized[i]=std::bit_cast<double>(uint64_t(actual[2*i])|(uint64_t(actual[2*i+1])<<32));buffers[3].memory.unmapMemory();instance.cmdBuf()->reset();return result;
}
PathwayComparisonResult run_pathway_comparison(const IQM::VulkanInstance& instance,const std::vector<float>& ref,const std::vector<float>& tst,unsigned w,unsigned h,unsigned nw,unsigned nh,const std::filesystem::path& shader){return comparison_impl(instance,ref,tst,w,h,nw,nh,shader,1);}
PathwayComparisonResult run_pathway_comparison_rgb(const IQM::VulkanInstance& instance,const std::vector<float>& ref,const std::vector<float>& tst,unsigned w,unsigned h,unsigned nw,unsigned nh,const std::filesystem::path& shader){if(w!=96||h!=96)throw std::invalid_argument("Unvalidated RGB comparison shape");return comparison_impl(instance,ref,tst,w,h,nw,nh,shader,3);}
}
