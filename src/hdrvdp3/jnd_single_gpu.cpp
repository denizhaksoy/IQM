#include <IQM/hdrvdp3/jnd_single_gpu.h>
#include <IQM/hdrvdp3/jnd.h>
#include <IQM/base/vulkan_runtime.h>
#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <span>
#include <stdexcept>
#include <vector>
namespace IQM::HDRVDP3 {
namespace {
GpuFloatBuffer rgb_to_jnd_response_single_gpu_device_diag(const IQM::VulkanInstance& instance,
    const std::string& shader, vk::Buffer lmsrClamped, vk::Buffer adaptation,
    unsigned width, unsigned height, double area, double age, int diagRun){
  using IQM::GPU::VulkanRuntime;
  const size_t n=size_t(width)*height;
  auto readAdapt=[&](){ auto staging=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),n*4,vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent); staging.first.bindMemory(staging.second,0); instance.cmdBuf()->reset(); instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit}); vk::BufferMemoryBarrier rb{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eTransferRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=adaptation,.offset=0,.size=n*4}; instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eTransfer,{},{},{rb},{}); instance.cmdBuf()->copyBuffer(adaptation,*staging.first,{vk::BufferCopy{0,0,n*4}}); vk::MemoryBarrier hb{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead}; instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},{hb},{},{}); instance.cmdBuf()->end(); vk::CommandBuffer c=*instance.cmdBuf(); vk::raii::Fence f(*instance.device(),vk::FenceCreateInfo{}); instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&c},f); instance.waitForFence(f); std::vector<float> out(n); void* p=staging.second.mapMemory(0,n*4); std::memcpy(out.data(),p,n*4); staging.second.unmapMemory(); instance.cmdBuf()->reset(); return out; };
  auto adapt=readAdapt();
  const float pupil=pupil_luminance_reduction_single(adapt,width,height,area,age);
  auto lut=create_jnd_lut();
  std::vector<float> lutLog(2048),lutCone(2048),lutRod(2048); for(size_t i=0;i<2048;++i){ lutLog[i]=float(lut.log_luminance[i]); lutCone[i]=float(lut.cone[i]); lutRod[i]=float(lut.rod[i]); }
  float low=float(std::pow(10.,lut.log_luminance.front())); float high=float(std::pow(10.,lut.log_luminance.back()));
  auto makeBuf=[&](size_t bytes,vk::BufferUsageFlags u,vk::MemoryPropertyFlags m){ auto b=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),bytes,u,m); b.first.bindMemory(b.second,0); return b; };
  auto upload=[&](const std::vector<float>& v){ auto b=makeBuf(v.size()*4,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent); void* p=b.second.mapMemory(0,v.size()*4); std::memcpy(p,v.data(),v.size()*4); b.second.unmapMemory(); return b; };
  auto bLog=upload(lutLog), bCone=upload(lutCone), bRod=upload(lutRod);
  auto bOutCh=makeBuf(3*n*4,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc,vk::MemoryPropertyFlagBits::eDeviceLocal);
  auto bOutResp=makeBuf(n*4,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eDeviceLocal);
  static int jndCall=0; const int call=jndCall++;
  std::string shaderPath=shader; { std::filesystem::path p(shader); auto cand=p.parent_path()/"jnd_diag.spv"; if(std::filesystem::exists(cand)) shaderPath=cand.string(); }
  std::ifstream file(shaderPath,std::ios::binary|std::ios::ate); auto len=file.tellg(); if(len<=0||len%4) throw std::runtime_error("jnd shader"); std::vector<uint32_t> code(size_t(len)/4); file.seekg(0); file.read(reinterpret_cast<char*>(code.data()),len); if(!file||code[0]!=0x07230203u) throw std::runtime_error("jnd SPIR-V");
  const auto& device=*instance.device(); auto module=VulkanRuntime::createShaderModule(device,code);
  const uint32_t dbgCount=6u*32u; const size_t dbgBytes=dbgCount*4u;
  auto dbgBuf=makeBuf(dbgBytes,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eDeviceLocal);
  std::vector<std::pair<vk::DescriptorType,uint32_t>> bindings{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}};
  auto setLayout=VulkanRuntime::createDescLayout(device,bindings);
  auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(32));
  auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
  auto pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,7u}});
  const vk::DescriptorSetLayout raw=*setLayout; auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});
  std::vector<vk::DescriptorBufferInfo> infos{{lmsrClamped,0,4*n*4},{*bLog.first,0,2048*4},{*bCone.first,0,2048*4},{*bRod.first,0,2048*4},{*bOutCh.first,0,3*n*4},{*bOutResp.first,0,n*4},{*dbgBuf.first,0,dbgBytes}};
  std::vector<vk::WriteDescriptorSet> writes; writes.reserve(infos.size());
  for(uint32_t i=0;i<infos.size();++i) writes.push_back({.dstSet=*sets[0],.dstBinding=i,.descriptorCount=1,.descriptorType=vk::DescriptorType::eStorageBuffer,.pBufferInfo=&infos[i]});
  device.updateDescriptorSets(writes,{});
  auto dumpBuf=[&](vk::Buffer buf,size_t bytes,const char* name){
    auto staging=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),bytes,vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent); staging.first.bindMemory(staging.second,0);
    instance.cmdBuf()->reset(); instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    vk::BufferMemoryBarrier pre{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eTransferRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=buf,.offset=0,.size=bytes};
    instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eTransfer,{},{},{pre},{});
    instance.cmdBuf()->copyBuffer(buf,*staging.first,{vk::BufferCopy{0,0,bytes}});
    vk::MemoryBarrier hb{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};
    instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},{hb},{},{});
    instance.cmdBuf()->end(); vk::CommandBuffer cc=*instance.cmdBuf(); vk::raii::Fence ff(*instance.device(),vk::FenceCreateInfo{}); instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cc},ff); instance.waitForFence(ff);
    std::vector<uint8_t> out(bytes); void* p=staging.second.mapMemory(0,bytes); std::memcpy(out.data(),p,bytes); staging.second.unmapMemory(); instance.cmdBuf()->reset();
    std::filesystem::path outd=getenv("IQM_HDRVDP3_DIAG_OUT")?std::filesystem::path(getenv("IQM_HDRVDP3_DIAG_OUT")):std::filesystem::path(".");
    std::ofstream d(outd/(std::string("diag_jnd_")+name+"_call"+std::to_string(call)+"_run"+std::to_string(diagRun)+".bin"),std::ios::binary); d.write((char*)out.data(),out.size());
  };
  dumpBuf(lmsrClamped,4*n*4,"lmsr_pre"); dumpBuf(adaptation,n*4,"adapt_pre"); dumpBuf(*bLog.first,2048*4,"lutLog"); dumpBuf(*bCone.first,2048*4,"lutCone"); dumpBuf(*bRod.first,2048*4,"lutRod");
  { std::filesystem::path outd=getenv("IQM_HDRVDP3_DIAG_OUT")?std::filesystem::path(getenv("IQM_HDRVDP3_DIAG_OUT")):std::filesystem::path("."); std::ofstream m(outd/("diag_jnd_meta_call"+std::to_string(call)+"_run"+std::to_string(diagRun)+".txt")); m<<std::setprecision(17)
    <<"call="<<call<<" run="<<diagRun<<" width="<<width<<" height="<<height<<" n="<<n<<" area="<<area<<" age="<<age<<" pupil="<<pupil<<" low="<<low<<" high="<<high<<" lutSize=2048\n"; }
  instance.cmdBuf()->reset(); instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  instance.cmdBuf()->fillBuffer(*bOutResp.first,0,n*4,0x7FC0FFFFu);
  vk::BufferMemoryBarrier sent{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eShaderRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=*bOutResp.first,.offset=0,.size=n*4};
  instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eComputeShader,{},{},{sent},{});
  std::vector<vk::BufferMemoryBarrier> inReady{{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=lmsrClamped,.offset=0,.size=4*n*4}};
  instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader,{},{},inReady,{});
  instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);
  instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});
  struct Push{uint32_t count,pixels; float pupil,low,high; uint32_t lutSize,r0,r1;} pc{uint32_t(n),uint32_t(n),pupil,low,high,2048,0,0}; instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,pc);
  instance.cmdBuf()->dispatch((uint32_t(n)+63)/64,1,1);
  vk::BufferMemoryBarrier outReady{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eTransferRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=*bOutResp.first,.offset=0,.size=n*4};
  instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eTransfer,{},{},{outReady},{});
  instance.cmdBuf()->end(); vk::CommandBuffer c=*instance.cmdBuf(); vk::raii::Fence f(*instance.device(),vk::FenceCreateInfo{}); instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&c},f); instance.waitForFence(f);
  auto staging=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),n*4+dbgBytes,vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent); staging.first.bindMemory(staging.second,0);
  instance.cmdBuf()->reset(); instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  vk::BufferMemoryBarrier pre{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eTransferRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=*bOutResp.first,.offset=0,.size=n*4};
  vk::BufferMemoryBarrier preDbg{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eTransferRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=*dbgBuf.first,.offset=0,.size=dbgBytes};
  instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eTransfer,{},{},{pre,preDbg},{});
  instance.cmdBuf()->copyBuffer(*bOutResp.first,*staging.first,{vk::BufferCopy{0,0,n*4}});
  instance.cmdBuf()->copyBuffer(*dbgBuf.first,*staging.first,{vk::BufferCopy{0,n*4,dbgBytes}});
  vk::MemoryBarrier hb{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};
  instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},{hb},{},{});
  instance.cmdBuf()->end(); vk::CommandBuffer cc=*instance.cmdBuf(); vk::raii::Fence ff(*instance.device(),vk::FenceCreateInfo{}); instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cc},ff); instance.waitForFence(ff);
  std::vector<float> out(n); std::vector<uint32_t> dbg(dbgCount); void* p=staging.second.mapMemory(0,n*4+dbgBytes); std::memcpy(out.data(),p,n*4); std::memcpy(dbg.data(),(char*)p+n*4,dbgBytes); staging.second.unmapMemory(); instance.cmdBuf()->reset();
  auto fnv=[](std::span<const float> v){uint64_t h=1469598103934665603ull; for(float x:v){uint32_t b=std::bit_cast<uint32_t>(x); h^=b; h*=1099511628211ull;} return h;};
  uint32_t sentCount=0, sentPat=0x7FC0FFFFu; int firstSent=-1; for(size_t i=0;i<out.size();++i){ if(std::bit_cast<uint32_t>(out[i])==sentPat){ ++sentCount; if(firstSent<0) firstSent=(int)i; } }
  std::filesystem::path outd=getenv("IQM_HDRVDP3_DIAG_OUT")?std::filesystem::path(getenv("IQM_HDRVDP3_DIAG_OUT")):std::filesystem::path(".");
  std::ofstream d(outd/("diag_jnd_post_call"+std::to_string(call)+"_run"+std::to_string(diagRun)+".txt"));
  d<<"n="<<out.size()<<" fnv="<<std::hex<<fnv(out)<<" sentinel_count="<<std::dec<<sentCount<<" first_sentinel="<<firstSent<<"\n"; for(size_t i=0;i<out.size();++i){ d<<std::hex<<std::bit_cast<uint32_t>(out[i]); if(i+1<out.size()) d<<","; } d<<std::dec<<"\n"; for(float v:out) d<<std::setprecision(9)<<v<<"\n";
  std::ofstream dd(outd/("diag_jnd_dbg_call"+std::to_string(call)+"_run"+std::to_string(diagRun)+".txt"));
  dd<<"slots=6 stride=32\n";
  for(int s=0;s<6;++s){ uint32_t* bb=&dbg[s*32]; auto fv=[&](int k){float v; std::memcpy(&v,&bb[k],4); return v;}; dd<<"slot="<<s<<" idx="<<bb[0]<<" ch="<<bb[1]<<" lo="<<bb[2]<<" lutI="<<bb[3]<<" x0="<<std::setprecision(9)<<fv(4)<<" x1="<<fv(5)<<" y0="<<fv(6)<<" y1="<<fv(7)<<" slope="<<fv(8)<<" off="<<fv(9)<<" res="<<fv(10)<<" lmsr="<<fv(11)<<" pupil="<<fv(12)<<" pup="<<fv(13)<<" q="<<fv(14)<<" outCh="<<fv(15)<<" resp="<<fv(16)<<"\n"; }
  return GpuFloatBuffer{std::move(bOutResp.first),std::move(bOutResp.second),unsigned(n*4),unsigned(n)};
}
} // namespace

GpuFloatBuffer rgb_to_jnd_response_single_gpu_device(const IQM::VulkanInstance& instance,
    const std::string& shader, vk::Buffer lmsrClamped, vk::Buffer adaptation,
    unsigned width, unsigned height, double area, double age){
  using IQM::GPU::VulkanRuntime;
  if(!lmsrClamped||!adaptation||!width||!height) throw std::invalid_argument("jnd gpu: bad input");
  const char* diagEnv=getenv("IQM_HDRVDP3_DIAG_RUN");
  const int diagRun=(diagEnv?std::atoi(diagEnv):0);
  if(diagRun)
    return rgb_to_jnd_response_single_gpu_device_diag(instance,shader,lmsrClamped,adaptation,width,height,area,age,diagRun);

  const size_t n=size_t(width)*height;
  auto readAdapt=[&](){ auto staging=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),n*4,vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent); staging.first.bindMemory(staging.second,0); instance.cmdBuf()->reset(); instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit}); vk::BufferMemoryBarrier rb{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eTransferRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=adaptation,.offset=0,.size=n*4}; instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eTransfer,{},{},{rb},{}); instance.cmdBuf()->copyBuffer(adaptation,*staging.first,{vk::BufferCopy{0,0,n*4}}); vk::MemoryBarrier hb{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead}; instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},{hb},{},{}); instance.cmdBuf()->end(); vk::CommandBuffer c=*instance.cmdBuf(); vk::raii::Fence f(*instance.device(),vk::FenceCreateInfo{}); instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&c},f); instance.waitForFence(f); std::vector<float> out(n); void* p=staging.second.mapMemory(0,n*4); std::memcpy(out.data(),p,n*4); staging.second.unmapMemory(); instance.cmdBuf()->reset(); return out; };
  auto adapt=readAdapt();
  const float pupil=pupil_luminance_reduction_single(adapt,width,height,area,age);
  auto lut=create_jnd_lut();
  std::vector<float> lutLog(2048),lutCone(2048),lutRod(2048); for(size_t i=0;i<2048;++i){ lutLog[i]=float(lut.log_luminance[i]); lutCone[i]=float(lut.cone[i]); lutRod[i]=float(lut.rod[i]); }
  float low=float(std::pow(10.,lut.log_luminance.front())); float high=float(std::pow(10.,lut.log_luminance.back()));
  auto makeBuf=[&](size_t bytes,vk::BufferUsageFlags u,vk::MemoryPropertyFlags m){ auto b=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),bytes,u,m); b.first.bindMemory(b.second,0); return b; };
  const char* stagingEnv=getenv("IQM_HDRVDP3_JND_LUT_STAGING");
  const bool lutStaging=(stagingEnv?std::atoi(stagingEnv):0)!=0;
  auto uploadHost=[&](const std::vector<float>& v){ auto b=makeBuf(v.size()*4,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent); void* p=b.second.mapMemory(0,v.size()*4); std::memcpy(p,v.data(),v.size()*4); b.second.unmapMemory(); return b; };
  using BufPair=std::pair<vk::raii::Buffer,vk::raii::DeviceMemory>;
  BufPair bLog{nullptr,nullptr}, bCone{nullptr,nullptr}, bRod{nullptr,nullptr};
  std::vector<BufPair> stagingBufs; stagingBufs.reserve(3);
  if(lutStaging){
    auto uploadStaged=[&](const std::vector<float>& v){
      auto st=makeBuf(v.size()*4,vk::BufferUsageFlagBits::eTransferSrc,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
      void* p=st.second.mapMemory(0,v.size()*4); std::memcpy(p,v.data(),v.size()*4); st.second.unmapMemory();
      auto dev=makeBuf(v.size()*4,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eDeviceLocal);
      stagingBufs.push_back(std::move(st));
      return dev;
    };
    bLog=uploadStaged(lutLog); bCone=uploadStaged(lutCone); bRod=uploadStaged(lutRod);
  } else {
    bLog=uploadHost(lutLog); bCone=uploadHost(lutCone); bRod=uploadHost(lutRod);
  }
  auto bOutCh=makeBuf(3*n*4,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc,vk::MemoryPropertyFlagBits::eDeviceLocal);
  auto bOutResp=makeBuf(n*4,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eDeviceLocal);

  std::ifstream file(shader,std::ios::binary|std::ios::ate); auto len=file.tellg(); if(len<=0||len%4) throw std::runtime_error("jnd shader"); std::vector<uint32_t> code(size_t(len)/4); file.seekg(0); file.read(reinterpret_cast<char*>(code.data()),len); if(!file||code[0]!=0x07230203u) throw std::runtime_error("jnd SPIR-V");
  const auto& device=*instance.device(); auto module=VulkanRuntime::createShaderModule(device,code);
  std::vector<std::pair<vk::DescriptorType,uint32_t>> bindings{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}};
  auto setLayout=VulkanRuntime::createDescLayout(device,bindings);
  auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(32));
  auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
  auto pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,6u}});
  const vk::DescriptorSetLayout raw=*setLayout; auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});
  std::vector<vk::DescriptorBufferInfo> infos{{lmsrClamped,0,4*n*4},{*bLog.first,0,2048*4},{*bCone.first,0,2048*4},{*bRod.first,0,2048*4},{*bOutCh.first,0,3*n*4},{*bOutResp.first,0,n*4}};
  std::vector<vk::WriteDescriptorSet> writes; writes.reserve(infos.size());
  for(uint32_t i=0;i<infos.size();++i) writes.push_back({.dstSet=*sets[0],.dstBinding=i,.descriptorCount=1,.descriptorType=vk::DescriptorType::eStorageBuffer,.pBufferInfo=&infos[i]});
  device.updateDescriptorSets(writes,{});

  instance.cmdBuf()->reset(); instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  if(lutStaging){
    const vk::Buffer dsts[3]={*bLog.first,*bCone.first,*bRod.first};
    for(size_t i=0;i<3;++i) instance.cmdBuf()->copyBuffer(*stagingBufs[i].first,dsts[i],{vk::BufferCopy{0,0,2048*4}});
    std::vector<vk::BufferMemoryBarrier> lutReady;
    for(size_t i=0;i<3;++i) lutReady.push_back({.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=dsts[i],.offset=0,.size=2048*4});
    lutReady.push_back({.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=lmsrClamped,.offset=0,.size=4*n*4});
    instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer|vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader,{},{},lutReady,{});
  } else {
    vk::BufferMemoryBarrier inReady{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=lmsrClamped,.offset=0,.size=4*n*4};
    instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader,{},{},{inReady},{});
  }
  instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);
  instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});
  struct Push{uint32_t count,pixels; float pupil,low,high; uint32_t lutSize,r0,r1;} pc{uint32_t(n),uint32_t(n),pupil,low,high,2048,0,0}; instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,pc);
  instance.cmdBuf()->dispatch((uint32_t(n)+63)/64,1,1);
  vk::BufferMemoryBarrier outReady{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eTransferRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=*bOutResp.first,.offset=0,.size=n*4};
  instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eTransfer,{},{},{outReady},{});
  instance.cmdBuf()->end(); vk::CommandBuffer c=*instance.cmdBuf(); vk::raii::Fence f(*instance.device(),vk::FenceCreateInfo{}); instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&c},f); instance.waitForFence(f);
  instance.cmdBuf()->reset();
  return GpuFloatBuffer{std::move(bOutResp.first),std::move(bOutResp.second),unsigned(n*4),unsigned(n)};
}
std::vector<float> rgb_to_jnd_response_single_gpu_readback(const IQM::VulkanInstance& instance,
    const std::string& shader, vk::Buffer lmsrClamped, vk::Buffer adaptation,
    unsigned width, unsigned height, double area, double age){
  auto buf=rgb_to_jnd_response_single_gpu_device(instance,shader,lmsrClamped,adaptation,width,height,area,age);
  auto staging=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),buf.bytes,vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent); staging.first.bindMemory(staging.second,0);
  instance.cmdBuf()->reset(); instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  instance.cmdBuf()->copyBuffer(*buf.buffer,*staging.first,{vk::BufferCopy{0,0,buf.bytes}});
  vk::MemoryBarrier hb{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};
  instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},{hb},{},{});
  instance.cmdBuf()->end(); vk::CommandBuffer c=*instance.cmdBuf(); vk::raii::Fence f(*instance.device(),vk::FenceCreateInfo{}); instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&c},f); instance.waitForFence(f);
  std::vector<float> out(buf.count); void* p=staging.second.mapMemory(0,buf.bytes); std::memcpy(out.data(),p,buf.bytes); staging.second.unmapMemory(); instance.cmdBuf()->reset(); return out;
}
} // namespace IQM::HDRVDP3
