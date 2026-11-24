#include <IQM/hdrvdp3/sp0_single_gpu.h>
#include <IQM/hdrvdp3/sp0_resident_gpu.h>
#include <IQM/hdrvdp3/sp0_double.h>
#include <IQM/base/vulkan_runtime.h>
#include <vulkan_res.h>
#include <vulkan_instance.h>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <vector>
namespace IQM::HDRVDP3 {
static GpuFloatBuffer uploadFloats(const IQM::VulkanInstance& inst,const std::vector<float>& v){
  auto b=IQM::Bin::VulkanResource::createBuffer(*inst.device(),*inst.physicalDevice(),unsigned(v.size()*4),vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
  b.first.bindMemory(b.second,0); void* p=b.second.mapMemory(0,v.size()*4); std::memcpy(p,v.data(),v.size()*4); b.second.unmapMemory();
  return GpuFloatBuffer{std::move(b.first),std::move(b.second),unsigned(v.size()*4),unsigned(v.size())};
}
static GpuFloatBuffer deviceBuf(const IQM::VulkanInstance& inst,size_t count){
  auto b=IQM::Bin::VulkanResource::createBuffer(*inst.device(),*inst.physicalDevice(),unsigned(count*4),vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eDeviceLocal);
  b.first.bindMemory(b.second,0);
  return GpuFloatBuffer{std::move(b.first),std::move(b.second),unsigned(count*4),unsigned(count)};
}
struct IsoCmd {
  vk::raii::CommandPool pool;
  vk::raii::CommandBuffer cmd;
  IsoCmd(const IQM::VulkanInstance& inst):pool(nullptr),cmd(nullptr){
    auto qf = inst.physicalDevice()->getQueueFamilyProperties();
    uint32_t qi=0; for(uint32_t i=0;i<qf.size();++i) if(qf[i].queueFlags & vk::QueueFlagBits::eCompute){ qi=i; break; }
    pool=vk::raii::CommandPool(*inst.device(),vk::CommandPoolCreateInfo{.flags=vk::CommandPoolCreateFlagBits::eResetCommandBuffer,.queueFamilyIndex=qi});
    cmd=std::move(vk::raii::CommandBuffers(*inst.device(),vk::CommandBufferAllocateInfo{.commandPool=*pool,.commandBufferCount=1}).front());
  }
  void submitWait(const IQM::VulkanInstance& inst){ cmd.end(); vk::CommandBuffer c=*cmd; vk::raii::Fence f(*inst.device(),vk::FenceCreateInfo{}); inst.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&c},f); inst.waitForFence(f); cmd.reset(); }
};
static std::vector<float> readbackIso(const IQM::VulkanInstance& inst,IsoCmd& iso,vk::Buffer buf,size_t count){
  auto staging=IQM::Bin::VulkanResource::createBuffer(*inst.device(),*inst.physicalDevice(),unsigned(count*4),vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent); staging.first.bindMemory(staging.second,0);
  iso.cmd.begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eTransferRead};
  iso.cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eTransfer,{},{before},{},{});
  iso.cmd.copyBuffer(buf,*staging.first,{vk::BufferCopy{0,0,count*4}});
  vk::MemoryBarrier hb{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};
  iso.cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},{hb},{},{});
  iso.submitWait(inst);
  std::vector<float> out(count); void* p=staging.second.mapMemory(0,count*4); std::memcpy(out.data(),p,count*4); staging.second.unmapMemory(); return out;
}
SP0PyramidGpuSingle build_sp0_pyramid_single_gpu(const IQM::VulkanInstance& inst,
    const std::string& corrdnShader,const std::string& baseMeanShader,
    vk::Buffer jndResponse,unsigned w,unsigned h,double ppd,bool readHost){
  if(!jndResponse||w<16||h<16) throw std::invalid_argument("sp0 gpu bad input");
  unsigned maxLevels=0; for(unsigned a=w,b=h;a>=13&&b>=13;a/=2,b/=2) ++maxLevels;
  unsigned levels=std::min(unsigned(std::ceil(std::log2(ppd))-2),maxLevels);
  auto f=create_sp0_model_double();
  auto toFloat=[&](const std::vector<double>& d){ std::vector<float> r(d.size()); for(size_t i=0;i<d.size();++i) r[i]=float(d[i]); return r; };
  auto lo0=uploadFloats(inst,toFloat(f.lo0));
  auto hi0=uploadFloats(inst,toFloat(f.high));
  auto lo=uploadFloats(inst,toFloat(f.lo));
  auto orient=uploadFloats(inst,toFloat(f.orientation));
  auto workA=deviceBuf(inst,size_t(w)*h);
  auto workB=deviceBuf(inst,size_t(w)*h);
  std::vector<GpuFloatBuffer> bandBufs;
  unsigned bw=w,bh=h; for(size_t b=0;b<size_t(levels)+2;++b){ bandBufs.push_back(deviceBuf(inst,size_t(bw)*bh)); if(b>=1){bw=(bw+1)/2; bh=(bh+1)/2;} }
  auto baseMean=deviceBuf(inst,1);
  SP0ResidentBuffers bufs; bufs.input={jndResponse,0,size_t(w)*h*4}; bufs.lo0Filter={*lo0.buffer,0,lo0.bytes}; bufs.hi0Filter={*hi0.buffer,0,hi0.bytes}; bufs.loFilter={*lo.buffer,0,lo.bytes}; bufs.orientFilter={*orient.buffer,0,orient.bytes}; bufs.workA={*workA.buffer,0,workA.bytes}; bufs.workB={*workB.buffer,0,workB.bytes}; bufs.baseMean={*baseMean.buffer,0,4};
  for(auto& b:bandBufs) bufs.bandRegions.push_back({*b.buffer,0,b.bytes});
  SP0ResidentShaders shaders{.corrdn=corrdnShader,.baseMean=baseMeanShader};
  ResidentSP0Pyramid pyr(*inst.device(),*inst.physicalDevice(),shaders,bufs,w,h,ppd);
  IsoCmd iso(inst);
  iso.cmd.begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  pyr.record(*iso.cmd);
  iso.submitWait(inst);
  SP0PyramidGpuSingle out; out.levels=levels; out.bands=std::move(bandBufs); out.baseMean=std::move(baseMean);
  out.dispatches=pyr.dispatchCount();
  if(!readHost) return out;
  SP0PyramidSingle host; host.bands.resize(out.bands.size());
  bw=w; bh=h; for(size_t b=0;b<out.bands.size();++b){ host.bands[b].width=bw; host.bands[b].height=bh; host.bands[b].values=readbackIso(inst,iso,*out.bands[b].buffer,size_t(bw)*bh); ++out.hostReadbacks; if(b>=1){bw=(bw+1)/2; bh=(bh+1)/2;} }
  auto bm=readbackIso(inst,iso,*out.baseMean.buffer,1); host.base_mean=bm[0]; ++out.hostReadbacks;
  out.host=std::move(host);
  return out;
}
}