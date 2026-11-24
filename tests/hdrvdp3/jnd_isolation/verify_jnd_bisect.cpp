#include <IQM/base/vulkan_runtime.h>
#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <bit>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
namespace fs=std::filesystem;
using IQM::GPU::VulkanRuntime;
static uint64_t fnvU32(const std::vector<uint32_t>& v){uint64_t h=1469598103934665603ull; for(uint32_t b:v){ h^=b; h*=1099511628211ull;} return h;}
static std::string hx(uint32_t v){ std::ostringstream o; o<<"0x"<<std::hex<<std::setw(8)<<std::setfill('0')<<v; return o.str(); }
struct Buf { vk::raii::Buffer buf{nullptr}; vk::raii::DeviceMemory mem{nullptr}; };
static Buf makeBuf(const IQM::VulkanInstance& inst,size_t bytes,vk::BufferUsageFlags u,vk::MemoryPropertyFlags m){ auto b=IQM::Bin::VulkanResource::createBuffer(*inst.device(),*inst.physicalDevice(),(unsigned)bytes,u,m); b.first.bindMemory(b.second,0); return {std::move(b.first),std::move(b.second)}; }
static void upload(const IQM::VulkanInstance& inst, Buf& dst, const void* data, size_t bytes){ auto st=makeBuf(inst,bytes,vk::BufferUsageFlagBits::eTransferSrc,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent); void* p=st.mem.mapMemory(0,bytes); std::memcpy(p,data,bytes); st.mem.unmapMemory(); inst.cmdBuf()->reset(); inst.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit}); inst.cmdBuf()->copyBuffer(*st.buf,*dst.buf,{vk::BufferCopy{0,0,bytes}}); inst.cmdBuf()->end(); vk::CommandBuffer c=*inst.cmdBuf(); vk::raii::Fence f(*inst.device(),vk::FenceCreateInfo{}); inst.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&c},f); inst.waitForFence(f); inst.cmdBuf()->reset(); }
static std::vector<uint32_t> rbU32(const IQM::VulkanInstance& inst, vk::Buffer buf, size_t bytes){ auto st=makeBuf(inst,bytes,vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent); inst.cmdBuf()->reset(); inst.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit}); vk::BufferMemoryBarrier pre{.srcAccessMask=vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eTransferRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=buf,.offset=0,.size=bytes}; inst.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,vk::PipelineStageFlagBits::eTransfer,{},{},{pre},{}); inst.cmdBuf()->copyBuffer(buf,*st.buf,{vk::BufferCopy{0,0,bytes}}); vk::MemoryBarrier hb{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead}; inst.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},{hb},{},{}); inst.cmdBuf()->end(); vk::CommandBuffer c=*inst.cmdBuf(); vk::raii::Fence f(*inst.device(),vk::FenceCreateInfo{}); inst.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&c},f); inst.waitForFence(f); std::vector<uint32_t> out(bytes/4); void* p=st.mem.mapMemory(0,bytes); std::memcpy(out.data(),p,bytes); st.mem.unmapMemory(); inst.cmdBuf()->reset(); return out; }
struct R { bool stable=true,correct=true; std::vector<uint64_t> hashes; size_t badRun=size_t(-1),badIdx=size_t(-1); uint32_t got=0,exp=0; size_t sentinelCount=0,zeroCount=0; };
static void printR(const char* name,const R& r){ std::cout<<name<<" STABLE="<<r.stable<<" CORRECT="<<r.correct<<" runs="<<r.hashes.size(); if(!r.stable||!r.correct) std::cout<<" badRun="<<r.badRun<<" badIdx="<<r.badIdx<<" got="<<hx(r.got)<<" exp="<<hx(r.exp); std::cout<<" sentinel="<<r.sentinelCount<<" zero="<<r.zeroCount<<std::endl; for(size_t i=0;i<r.hashes.size();++i) std::cout<<" "<<name<<".run"<<i<<"="<<std::hex<<std::setw(16)<<std::setfill('0')<<r.hashes[i]<<std::dec<<std::endl; }
struct SimpleCfg { bool fresh=false; bool hostVisibleInput=false; bool updateDescEach=false; bool sentinelOut=false; uint32_t sentinel=0xCDCDCDCDu; };
static R runSimple(const IQM::VulkanInstance& inst, const fs::path& spv, uint32_t count, const std::vector<uint32_t>& inData, uint32_t fillVal, bool fillInput, int runs, const SimpleCfg& cfg){
  bool fresh=cfg.fresh;
  std::ifstream f(spv,std::ios::binary|std::ios::ate); auto n=f.tellg(); std::vector<uint32_t> code(size_t(n)/4); f.seekg(0); f.read((char*)code.data(),n);
  const auto& dev=*inst.device();
  auto module=VulkanRuntime::createShaderModule(dev,code);
  auto setLayout=VulkanRuntime::createDescLayout(dev,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
  auto layout=VulkanRuntime::createPipelineLayout(dev,{*setLayout},VulkanRuntime::createPushConstantRange(16));
  auto pipe=VulkanRuntime::createComputePipeline(dev,module,layout);
  struct Res{ vk::raii::DescriptorPool pool{nullptr}; vk::raii::DescriptorSets sets{nullptr}; Buf in,out; };
  auto mk=[&](){ Res r; r.pool=VulkanRuntime::createDescPool(dev,1,{{vk::DescriptorType::eStorageBuffer,2u}}); const vk::DescriptorSetLayout raw=*setLayout; r.sets=vk::raii::DescriptorSets(dev,vk::DescriptorSetAllocateInfo{.descriptorPool=*r.pool,.descriptorSetCount=1,.pSetLayouts=&raw});
    auto inFlags = cfg.hostVisibleInput ? (vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent) : vk::MemoryPropertyFlagBits::eDeviceLocal;
    vk::BufferUsageFlags inUsage = vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eTransferSrc;
    r.in=makeBuf(inst,size_t(count)*4,inUsage,inFlags);
    r.out=makeBuf(inst,size_t(count)*4,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eDeviceLocal);
    std::vector<vk::DescriptorBufferInfo> infos{{*r.in.buf,0,size_t(count)*4},{*r.out.buf,0,size_t(count)*4}}; std::vector<vk::WriteDescriptorSet> w; w.reserve(2); for(uint32_t i=0;i<2;++i) w.push_back({.dstSet=*r.sets[0],.dstBinding=i,.descriptorCount=1,.descriptorType=vk::DescriptorType::eStorageBuffer,.pBufferInfo=&infos[i]}); dev.updateDescriptorSets(w,{}); return r; };
  auto uploadIn=[&](Res& r){ if(fillInput) return; if(cfg.hostVisibleInput){ void* p=r.in.mem.mapMemory(0,inData.size()*4); std::memcpy(p,inData.data(),inData.size()*4); r.in.mem.unmapMemory(); } else { upload(inst,r.in,inData.data(),inData.size()*4); } };
  Res res=mk();
  if(!fillInput && !inData.empty()) uploadIn(res);
  R outR; std::vector<uint32_t> expected(count);
  bool isConst = spv.filename().string()=="const.spv";
  if(isConst) std::fill(expected.begin(),expected.end(),0x3F800000u); else if(fillInput) std::fill(expected.begin(),expected.end(),fillVal); else for(uint32_t i=0;i<count;++i) expected[i]=inData[i];
  for(int run=0; run<runs; ++run){
    if(fresh){ res=mk(); if(!fillInput && !inData.empty()) uploadIn(res); }
    if(cfg.updateDescEach){ std::vector<vk::DescriptorBufferInfo> infos{{*res.in.buf,0,size_t(count)*4},{*res.out.buf,0,size_t(count)*4}}; std::vector<vk::WriteDescriptorSet> w; w.reserve(2); for(uint32_t i=0;i<2;++i) w.push_back({.dstSet=*res.sets[0],.dstBinding=i,.descriptorCount=1,.descriptorType=vk::DescriptorType::eStorageBuffer,.pBufferInfo=&infos[i]}); dev.updateDescriptorSets(w,{}); }
    if(!fillInput && !isConst){
      auto inChk=rbU32(inst,*res.in.buf,size_t(count)*4);
      uint64_t ih=fnvU32(inChk); bool inOk=true; for(size_t i=0;i<inChk.size();++i) if(inChk[i]!=inData[i]){ inOk=false; std::cout<<"INCHK_FAIL run="<<run<<" idx="<<i<<" got="<<hx(inChk[i])<<" exp="<<hx(inData[i])<<" hash="<<std::hex<<ih<<std::dec<<std::endl; break; }
      if(run==0) std::cout<<"INCHK_FIRST hash="<<std::hex<<ih<<std::dec<<" ok="<<inOk<<" inH=0x"<<std::hex<<(uint64_t)(VkBuffer)*res.in.buf<<" outH=0x"<<(uint64_t)(VkBuffer)*res.out.buf<<std::dec<<std::endl;
    }
    inst.cmdBuf()->reset(); inst.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    if(cfg.sentinelOut){
      inst.cmdBuf()->fillBuffer(*res.out.buf,0,size_t(count)*4,cfg.sentinel);
      vk::BufferMemoryBarrier sb{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eShaderRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=*res.out.buf,.offset=0,.size=size_t(count)*4};
      inst.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eComputeShader,{},{},{sb},{});
    }
    if(fillInput){ inst.cmdBuf()->fillBuffer(*res.in.buf,0,size_t(count)*4,fillVal); vk::BufferMemoryBarrier b{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=*res.in.buf,.offset=0,.size=size_t(count)*4}; inst.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eComputeShader,{},{},{b},{}); }
    else { vk::AccessFlags srcA = cfg.hostVisibleInput ? vk::AccessFlagBits::eHostWrite : (vk::AccessFlagBits::eTransferWrite|vk::AccessFlagBits::eHostWrite); vk::PipelineStageFlags srcS = cfg.hostVisibleInput ? vk::PipelineStageFlagBits::eHost : (vk::PipelineStageFlagBits::eTransfer|vk::PipelineStageFlagBits::eHost); vk::BufferMemoryBarrier b{.srcAccessMask=srcA,.dstAccessMask=vk::AccessFlagBits::eShaderRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=*res.in.buf,.offset=0,.size=size_t(count)*4}; inst.cmdBuf()->pipelineBarrier(srcS,vk::PipelineStageFlagBits::eComputeShader,{},{},{b},{}); }
    inst.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipe);
    inst.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*res.sets[0]},{});
    struct P{uint32_t count,inCount,r0,r1;} pc{count,count,0,0};
    inst.cmdBuf()->pushConstants<P>(*layout,vk::ShaderStageFlagBits::eCompute,0,pc);
    inst.cmdBuf()->dispatch((count+63)/64,1,1);
    vk::BufferMemoryBarrier ob{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eTransferRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=*res.out.buf,.offset=0,.size=size_t(count)*4}; inst.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eTransfer,{},{},{ob},{});
    inst.cmdBuf()->end(); vk::CommandBuffer c=*inst.cmdBuf(); vk::raii::Fence f(*inst.device(),vk::FenceCreateInfo{}); inst.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&c},f); inst.waitForFence(f);
    auto got=rbU32(inst,*res.out.buf,size_t(count)*4);
    uint64_t h=fnvU32(got); outR.hashes.push_back(h);
    outR.sentinelCount=0; outR.zeroCount=0; for(auto v:got){ if(v==cfg.sentinel) outR.sentinelCount++; if(v==0) outR.zeroCount++; }
    for(size_t i=0;i<got.size();++i){ if(got[i]!=expected[i]){ outR.correct=false; outR.stable=false; outR.badRun=run; outR.badIdx=i; outR.got=got[i]; outR.exp=expected[i]; break; } }
    if(run>0 && h!=outR.hashes[0]) outR.stable=false;
  }  return outR;
}
int main(int argc,char** argv){
  try{
    if(argc<3){ std::cerr<<"usage: verify-jnd-bisect <shader-dir-ignored> <out-dir>\n"; return 2; }
    fs::path outDir=argv[2]; fs::create_directories(outDir);
    IQM::Bin::VulkanInstance inst(true,"",false);
    std::cout<<"DEVICE_OK"<<std::endl;
    fs::path sdir=outDir/"shaders"; fs::create_directories(sdir);
    auto write=[&](fs::path p,const char* c){ std::ofstream f(p,std::ios::binary); f<<c; };
    write(sdir/"const.comp","#version 450\nlayout(local_size_x=64) in;\nlayout(set=0,binding=0,std430) readonly buffer InB{uint v[];} inb;\nlayout(set=0,binding=1,std430) writeonly buffer OutB{uint v[];} outb;\nlayout(push_constant) uniform P{uint count;uint inCount;uint r0;uint r1;} pc;\nvoid main(){uint i=gl_GlobalInvocationID.x; if(i>=pc.count) return; outb.v[i]=0x3F800000u;}\n");
    write(sdir/"copy.comp","#version 450\nlayout(local_size_x=64) in;\nlayout(set=0,binding=0,std430) readonly buffer InB{uint v[];} inb;\nlayout(set=0,binding=1,std430) writeonly buffer OutB{uint v[];} outb;\nlayout(push_constant) uniform P{uint count;uint inCount;uint r0;uint r1;} pc;\nvoid main(){uint i=gl_GlobalInvocationID.x; if(i>=pc.count) return; outb.v[i]=inb.v[i];}\n");
    const char* glslang="C:\\VulkanSDK\\1.4.363.0\\Bin\\glslangValidator.exe";
    auto compile=[&](fs::path src, fs::path dst){ std::string cmd=std::string("cmd /c \"\"")+glslang+"\" -V \""+src.string()+"\" -o \""+dst.string()+"\"\""; std::cout<<"CMD "<<cmd<<std::endl; int rc=std::system(cmd.c_str()); std::cout<<"RC "<<rc<<std::endl; if(rc!=0) throw std::runtime_error("compile"); };
    if(!fs::exists(sdir/"const.spv")) compile(sdir/"const.comp",sdir/"const.spv"); if(!fs::exists(sdir/"copy.spv")) compile(sdir/"copy.comp",sdir/"copy.spv");
    uint32_t N=561; std::vector<uint32_t> in(N); for(uint32_t i=0;i<N;++i) in[i]=0x12340000u+i;
    auto rB=runSimple(inst,sdir/"copy.spv",N,in,0,false,20,SimpleCfg{}); printR("COPY_REUSED_DEVLOCAL",rB);
    auto rBfreshDesc=runSimple(inst,sdir/"copy.spv",N,in,0,false,20,SimpleCfg{.fresh=true}); printR("COPY_FRESH_DESC_BUF",rBfreshDesc);
    // Variant A: reuse device-local buffers, fresh descriptor pool+set per dispatch
    // Custom A
    {
      R r; std::vector<uint32_t> expected(N); for(uint32_t i=0;i<N;++i) expected[i]=0x12340000u+i;
      auto inB=makeBuf(inst,size_t(N)*4,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eTransferSrc,vk::MemoryPropertyFlagBits::eDeviceLocal);
      auto outB=makeBuf(inst,size_t(N)*4,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eDeviceLocal);
      upload(inst,inB,in.data(),in.size()*4);
      auto module=[&](){ std::ifstream f(sdir/"copy.spv",std::ios::binary|std::ios::ate); auto n=f.tellg(); std::vector<uint32_t> v(size_t(n)/4); f.seekg(0); f.read((char*)v.data(),n); return v; }();
      auto mod=VulkanRuntime::createShaderModule(*inst.device(),module);
      auto setLayout=VulkanRuntime::createDescLayout(*inst.device(),{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
      auto layout=VulkanRuntime::createPipelineLayout(*inst.device(),{*setLayout},VulkanRuntime::createPushConstantRange(16));
      auto pipe=VulkanRuntime::createComputePipeline(*inst.device(),mod,layout);
      for(int run=0;run<20;++run){
        auto pool=VulkanRuntime::createDescPool(*inst.device(),1,{{vk::DescriptorType::eStorageBuffer,2u}});
        const vk::DescriptorSetLayout raw=*setLayout; auto sets=vk::raii::DescriptorSets(*inst.device(),vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});
        std::vector<vk::DescriptorBufferInfo> infos{{*inB.buf,0,size_t(N)*4},{*outB.buf,0,size_t(N)*4}}; std::vector<vk::WriteDescriptorSet> w; w.reserve(2); for(uint32_t i=0;i<2;++i) w.push_back({.dstSet=*sets[0],.dstBinding=i,.descriptorCount=1,.descriptorType=vk::DescriptorType::eStorageBuffer,.pBufferInfo=&infos[i]}); inst.device()->updateDescriptorSets(w,{});
        inst.cmdBuf()->reset(); inst.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        vk::BufferMemoryBarrier b{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=*inB.buf,.offset=0,.size=size_t(N)*4}; inst.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eComputeShader,{},{},{b},{});
        inst.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipe); inst.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{}); struct P{uint32_t c,i,r0,r1;} pc{N,N,0,0}; inst.cmdBuf()->pushConstants<P>(*layout,vk::ShaderStageFlagBits::eCompute,0,pc); inst.cmdBuf()->dispatch((N+63)/64,1,1);
        vk::BufferMemoryBarrier ob{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eTransferRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=*outB.buf,.offset=0,.size=size_t(N)*4}; inst.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eTransfer,{},{},{ob},{});
        inst.cmdBuf()->end(); vk::CommandBuffer cb=*inst.cmdBuf(); vk::raii::Fence f(*inst.device(),vk::FenceCreateInfo{}); inst.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cb},f); inst.waitForFence(f);
        auto got=rbU32(inst,*outB.buf,size_t(N)*4); uint64_t h=fnvU32(got); r.hashes.push_back(h); if(run==0) std::cout<<"VAR_A inH=0x"<<std::hex<<(uint64_t)(VkBuffer)*inB.buf<<" outH=0x"<<(uint64_t)(VkBuffer)*outB.buf<<std::dec<<std::endl;
        for(size_t i=0;i<got.size();++i){ if(got[i]!=expected[i]){ r.correct=false; r.stable=false; r.badRun=run; r.badIdx=i; r.got=got[i]; r.exp=expected[i]; break; } } if(run>0 && h!=r.hashes[0]) r.stable=false;
      }
      printR("VAR_A_FRESH_DESC_REUSE_BUF",r);
    }
    // Variant B: reuse buffers+desc, separate upload and compute submissions with fence wait
    {
      R r; std::vector<uint32_t> expected(N); for(uint32_t i=0;i<N;++i) expected[i]=0x12340000u+i;
      auto inB=makeBuf(inst,size_t(N)*4,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eTransferSrc,vk::MemoryPropertyFlagBits::eDeviceLocal);
      auto outB=makeBuf(inst,size_t(N)*4,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eDeviceLocal);
      auto st=makeBuf(inst,size_t(N)*4,vk::BufferUsageFlagBits::eTransferSrc,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
      void* p=st.mem.mapMemory(0,in.size()*4); std::memcpy(p,in.data(),in.size()*4); st.mem.unmapMemory();
      auto module=[&](){ std::ifstream f(sdir/"copy.spv",std::ios::binary|std::ios::ate); auto n=f.tellg(); std::vector<uint32_t> v(size_t(n)/4); f.seekg(0); f.read((char*)v.data(),n); return v; }();
      auto mod=VulkanRuntime::createShaderModule(*inst.device(),module);
      auto setLayout=VulkanRuntime::createDescLayout(*inst.device(),{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
      auto layout=VulkanRuntime::createPipelineLayout(*inst.device(),{*setLayout},VulkanRuntime::createPushConstantRange(16));
      auto pipe=VulkanRuntime::createComputePipeline(*inst.device(),mod,layout);
      auto pool=VulkanRuntime::createDescPool(*inst.device(),1,{{vk::DescriptorType::eStorageBuffer,2u}});
      const vk::DescriptorSetLayout raw=*setLayout; auto sets=vk::raii::DescriptorSets(*inst.device(),vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});
      std::vector<vk::DescriptorBufferInfo> infos{{*inB.buf,0,size_t(N)*4},{*outB.buf,0,size_t(N)*4}}; std::vector<vk::WriteDescriptorSet> w; w.reserve(2); for(uint32_t i=0;i<2;++i) w.push_back({.dstSet=*sets[0],.dstBinding=i,.descriptorCount=1,.descriptorType=vk::DescriptorType::eStorageBuffer,.pBufferInfo=&infos[i]}); inst.device()->updateDescriptorSets(w,{});
      for(int run=0;run<20;++run){
        // upload submission
        inst.cmdBuf()->reset(); inst.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit}); inst.cmdBuf()->copyBuffer(*st.buf,*inB.buf,{vk::BufferCopy{0,0,size_t(N)*4}}); inst.cmdBuf()->end(); vk::CommandBuffer cb1=*inst.cmdBuf(); vk::raii::Fence f1(*inst.device(),vk::FenceCreateInfo{}); inst.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cb1},f1); inst.waitForFence(f1);
        // compute submission
        inst.cmdBuf()->reset(); inst.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit}); vk::BufferMemoryBarrier b{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=*inB.buf,.offset=0,.size=size_t(N)*4}; inst.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eComputeShader,{},{},{b},{}); inst.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipe); inst.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{}); struct P{uint32_t c,i,r0,r1;} pc{N,N,0,0}; inst.cmdBuf()->pushConstants<P>(*layout,vk::ShaderStageFlagBits::eCompute,0,pc); inst.cmdBuf()->dispatch((N+63)/64,1,1); vk::BufferMemoryBarrier ob{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eTransferRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=*outB.buf,.offset=0,.size=size_t(N)*4}; inst.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eTransfer,{},{},{ob},{}); inst.cmdBuf()->end(); vk::CommandBuffer cb2=*inst.cmdBuf(); vk::raii::Fence f2(*inst.device(),vk::FenceCreateInfo{}); inst.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cb2},f2); inst.waitForFence(f2);
        auto got=rbU32(inst,*outB.buf,size_t(N)*4); uint64_t h=fnvU32(got); r.hashes.push_back(h); for(size_t i=0;i<got.size();++i){ if(got[i]!=expected[i]){ r.correct=false; r.stable=false; r.badRun=run; r.badIdx=i; r.got=got[i]; r.exp=expected[i]; break; } } if(run>0 && h!=r.hashes[0]) r.stable=false;
      }
      printR("VAR_B_SEPARATE_UPLOAD_SUBMIT",r);
    }
    // Variant C: host-visible coherent input, same good dev-local descriptor/buffer strategy as passing case? passing case is COPY_REUSED_DEVLOCAL
    auto rC=runSimple(inst,sdir/"copy.spv",N,in,0,false,20,SimpleCfg{.hostVisibleInput=true}); printR("VAR_C_HOSTVISIBLE_REUSED",rC);
    auto rA=runSimple(inst,sdir/"const.spv",N,in,0,false,20,SimpleCfg{}); printR("CONST_REUSED",rA);
    auto rCfill=runSimple(inst,sdir/"copy.spv",N,{},0xDEADBEEFu,true,20,SimpleCfg{}); printR("FILL_COPY_REUSED",rCfill);
    bool ok=rA.stable&&rA.correct&&rB.stable&&rB.correct&&rCfill.stable&&rCfill.correct;
    std::cout<<"BASIC_OK="<<ok<<std::endl; return ok?0:1;
  } catch(const std::exception& e){ std::cerr<<"EXC "<<e.what()<<std::endl; return 1; }
}
