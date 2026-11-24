#include <IQM/hdrvdp3/jnd_single_gpu.h>
#include <IQM/hdrvdp3/jnd.h>
#include <IQM/base/vulkan_runtime.h>
#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace fs=std::filesystem;
using IQM::GPU::VulkanRuntime;
static uint64_t fnv(std::span<const float> v){uint64_t h=1469598103934665603ull; for(float x:v){uint32_t b=std::bit_cast<uint32_t>(x); h^=b; h*=1099511628211ull;} return h;}
static uint64_t fnvU32(std::span<const uint32_t> v){uint64_t h=1469598103934665603ull; for(uint32_t b:v){ h^=b; h*=1099511628211ull;} return h;}
static std::string hexu32(uint32_t v){ std::ostringstream o; o<<"0x"<<std::hex<<std::setw(8)<<std::setfill('0')<<v; return o.str(); }
static void writeBin(const fs::path& p, std::span<const float> v){ std::ofstream f(p,std::ios::binary); f.write((const char*)v.data(), v.size()*4); }
static std::vector<float> readBin(const fs::path& p){ std::ifstream f(p,std::ios::binary|std::ios::ate); auto n=f.tellg(); std::vector<float> v(size_t(n)/4); f.seekg(0); f.read((char*)v.data(),n); return v; }
struct Buf { vk::raii::Buffer buf{nullptr}; vk::raii::DeviceMemory mem{nullptr}; size_t bytes=0; };
static Buf makeBuf(const IQM::VulkanInstance& inst,size_t bytes,vk::BufferUsageFlags u,vk::MemoryPropertyFlags m){
  auto b=IQM::Bin::VulkanResource::createBuffer(*inst.device(),*inst.physicalDevice(),(unsigned)bytes,u,m); b.first.bindMemory(b.second,0); return {std::move(b.first),std::move(b.second),bytes};
}
static void uploadRaw(const IQM::VulkanInstance& inst, Buf& dst, const void* data, size_t bytes){
  auto st=makeBuf(inst,bytes,vk::BufferUsageFlagBits::eTransferSrc,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
  void* p=st.mem.mapMemory(0,bytes); std::memcpy(p,data,bytes); st.mem.unmapMemory();
  inst.cmdBuf()->reset(); inst.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  inst.cmdBuf()->copyBuffer(*st.buf,*dst.buf,{vk::BufferCopy{0,0,bytes}});
  vk::MemoryBarrier hb{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead|vk::AccessFlagBits::eHostWrite};
  inst.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},{hb},{},{});
  inst.cmdBuf()->end(); vk::CommandBuffer c=*inst.cmdBuf(); vk::raii::Fence f(*inst.device(),vk::FenceCreateInfo{}); inst.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&c},f); inst.waitForFence(f); inst.cmdBuf()->reset();
}
static std::vector<uint32_t> readbackU32(const IQM::VulkanInstance& inst, vk::Buffer buf, size_t bytes){
  auto st=makeBuf(inst,bytes,vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
  inst.cmdBuf()->reset(); inst.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  vk::BufferMemoryBarrier pre{.srcAccessMask=vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eTransferRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=buf,.offset=0,.size=bytes};
  inst.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,vk::PipelineStageFlagBits::eTransfer,{},{},{pre},{});
  inst.cmdBuf()->copyBuffer(buf,*st.buf,{vk::BufferCopy{0,0,bytes}});
  vk::MemoryBarrier hb{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};
  inst.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},{hb},{},{});
  inst.cmdBuf()->end(); vk::CommandBuffer c=*inst.cmdBuf(); vk::raii::Fence f(*inst.device(),vk::FenceCreateInfo{}); inst.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&c},f); inst.waitForFence(f);
  std::vector<uint32_t> out(bytes/4); void* p=st.mem.mapMemory(0,bytes); std::memcpy(out.data(),p,bytes); st.mem.unmapMemory(); inst.cmdBuf()->reset(); return out;
}
static void writeText(const fs::path& p, const std::string& s){ std::ofstream f(p,std::ios::binary); f<<s; }

struct TestResult { bool stable=true; bool correct=true; uint64_t firstHash=0; size_t firstBadRun=size_t(-1), firstIdx=size_t(-1); uint32_t got=0, expected=0; std::vector<uint64_t> hashes; };

static TestResult runShaderTest(const IQM::VulkanInstance& inst, const std::string& spvPath, uint32_t count, uint32_t inCount,
  const std::vector<uint32_t>& input, const std::vector<uint32_t>& expected, int runs, bool freshEach, bool fillInputWithCmd){
  auto code=[&](){ std::ifstream f(spvPath,std::ios::binary|std::ios::ate); auto n=f.tellg(); std::vector<uint32_t> v(size_t(n)/4); f.seekg(0); f.read((char*)v.data(),n); return v; }();
  const auto& dev=*inst.device();
  auto module=VulkanRuntime::createShaderModule(dev,code);
  std::vector<std::pair<vk::DescriptorType,uint32_t>> bindings={{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}};
  auto setLayout=VulkanRuntime::createDescLayout(dev,bindings);
  auto layout=VulkanRuntime::createPipelineLayout(dev,{*setLayout},VulkanRuntime::createPushConstantRange(16));
  auto pipeline=VulkanRuntime::createComputePipeline(dev,module,layout);
  auto makeResources=[&](){
    auto pool=VulkanRuntime::createDescPool(dev,1,{{vk::DescriptorType::eStorageBuffer,2u}});
    const vk::DescriptorSetLayout raw=*setLayout; auto sets=vk::raii::DescriptorSets(dev,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});
    auto inBuf=makeBuf(inst,size_t(inCount)*4,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eDeviceLocal);
    auto outBuf=makeBuf(inst,size_t(count)*4,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eDeviceLocal);
    std::vector<vk::DescriptorBufferInfo> infos{{*inBuf.buf,0,size_t(inCount)*4},{*outBuf.buf,0,size_t(count)*4}};
    std::vector<vk::WriteDescriptorSet> writes; for(uint32_t i=0;i<2;++i) writes.push_back(VulkanRuntime::createWriteSet(*sets[0],i,std::vector<vk::DescriptorBufferInfo>{infos[i]})); dev.updateDescriptorSets(writes,{});
    struct R{ decltype(pool) pool; decltype(sets) sets; Buf in; Buf out; } r{std::move(pool),std::move(sets),std::move(inBuf),std::move(outBuf)};
    return r;
  };
  auto res=makeResources();
  if(!fillInputWithCmd && !input.empty()) uploadRaw(inst,res.in,input.data(),input.size()*4);
  TestResult tr;
  for(int r=0;r<runs;++r){
    if(freshEach) res=makeResources();
    inst.cmdBuf()->reset(); inst.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    if(fillInputWithCmd){
      inst.cmdBuf()->fillBuffer(*res.in.buf,0,size_t(inCount)*4,0xDEADBEEFu);
      vk::BufferMemoryBarrier b{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=*res.in.buf,.offset=0,.size=size_t(inCount)*4};
      inst.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eComputeShader,{},{},{b},{});
    } else {
      vk::BufferMemoryBarrier b{.srcAccessMask=vk::AccessFlagBits::eTransferWrite|vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=*res.in.buf,.offset=0,.size=size_t(inCount)*4};
      inst.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer|vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{},{},{b},{});
    }
    inst.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);
    inst.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*res.sets[0]},{});
    struct P{uint32_t count; uint32_t inCount; uint32_t r0; uint32_t r1;} pc{count,inCount,0,0};
    inst.cmdBuf()->pushConstants<P>(*layout,vk::ShaderStageFlagBits::eCompute,0,pc);
    inst.cmdBuf()->dispatch((count+63)/64,1,1);
    vk::BufferMemoryBarrier ob{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eTransferRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=*res.out.buf,.offset=0,.size=size_t(count)*4};
    inst.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eTransfer,{},{},{ob},{});
    inst.cmdBuf()->end(); vk::CommandBuffer c=*inst.cmdBuf(); vk::raii::Fence f(*inst.device(),vk::FenceCreateInfo{}); inst.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&c},f); inst.waitForFence(f);
    auto out=readbackU32(inst,*res.out.buf,size_t(count)*4);
    uint64_t h=fnvU32(out); tr.hashes.push_back(h);
    if(r==0){ tr.firstHash=h; }
    else if(h!=tr.firstHash && tr.stable){ tr.stable=false; tr.firstBadRun=r; for(size_t i=0;i<out.size();++i){ if(out[i]!=expected[i%expected.size()]){ tr.firstIdx=i; tr.got=out[i]; tr.expected=expected[i%expected.size()]; break; } } }
    for(size_t i=0;i<out.size();++i){ uint32_t e=expected[i%expected.size()]; if(out[i]!=e){ tr.correct=false; if(tr.firstIdx==size_t(-1)){ tr.firstIdx=i; tr.got=out[i]; tr.expected=e; } break; } }
  }
  return tr;
}
static void report(std::ostream& os, const char* name, const TestResult& tr, int runs){
  os<<name<<": runs="<<runs<<" stable="<<tr.stable<<" correct="<<tr.correct<<" firstHash="<<std::hex<<std::setw(16)<<std::setfill('0')<<tr.firstHash<<std::dec;
  if(!tr.stable||!tr.correct) os<<" firstBadRun="<<tr.firstBadRun<<" firstIdx="<<tr.firstIdx<<" got="<<hexu32(tr.got)<<" expected="<<hexu32(tr.expected);
  os<<"\n";
  for(size_t i=0;i<tr.hashes.size();++i) os<<"  run"<<i<<".fnv="<<std::hex<<std::setw(16)<<std::setfill('0')<<tr.hashes[i]<<std::dec<<"\n";
  std::cout<<name<<" STABLE="<<tr.stable<<" CORRECT="<<tr.correct<<"\n";
}

static std::pair<vk::raii::Buffer,vk::raii::DeviceMemory> uploadF32(const IQM::VulkanInstance& inst, std::span<const float> data){
  size_t bytes=data.size()*4;
  auto b=IQM::Bin::VulkanResource::createBuffer(*inst.device(),*inst.physicalDevice(),(unsigned)bytes,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
  b.first.bindMemory(b.second,0); void* p=b.second.mapMemory(0,bytes); std::memcpy(p,data.data(),bytes); b.second.unmapMemory(); return b;
}
int main(int argc,char** argv){
  try{
    std::string device="NVIDIA RTX A500 Laptop GPU"; std::vector<std::string> pos; for(int i=1;i<argc;++i){ std::string a=argv[i]; if(a=="--device" && i+1<argc){ device=argv[++i]; } else { pos.push_back(a); } }
    if(pos.size()<4){ std::cerr<<"usage: verify-jnd-isolation [--device NAME] <shader-dir> <input-dir> <out-dir> <runs>\n"; return 2; }
    fs::path shaderDir=pos[0], inDir=pos[1], outDir=pos[2]; int runs=std::atoi(pos[3].c_str());
    fs::create_directories(outDir);
    IQM::Bin::VulkanInstance inst(true,device,false);
    std::cout<<"DEVICE="<<inst.physicalDevice()->getProperties().deviceName.data()<<"\n";
    auto lmsr=readBin(inDir/"lmsr.bin"); auto adapt=readBin(inDir/"adapt.bin");
    std::ifstream meta(inDir/"meta.txt"); unsigned W=0,H=0; double area=0,age=24; std::string k; while(meta>>k){ if(k=="w") meta>>W; else if(k=="h") meta>>H; else if(k=="area") meta>>area; else if(k=="age") meta>>age; }
    auto lut=IQM::HDRVDP3::create_jnd_lut();
    auto cpuRef=IQM::HDRVDP3::rgb_to_jnd_response_single(lut,std::span<const float>(lmsr.data(),lmsr.size()),std::span<const float>(adapt.data(),adapt.size()),W,H,area,age);
    std::vector<uint32_t> cpuBits(cpuRef.response.size()); for(size_t i=0;i<cpuRef.response.size();++i) cpuBits[i]=std::bit_cast<uint32_t>(cpuRef.response[i]);
    uint64_t cpuHash=fnvU32(cpuBits);
    auto bLmsr=uploadF32(inst,lmsr); auto bAdapt=uploadF32(inst,adapt);
    fs::path shader=shaderDir/"jnd.spv";
    std::vector<uint64_t> hashes;
    std::vector<float> firstOut;
    for(int r=0;r<runs;++r){
      auto out=IQM::HDRVDP3::rgb_to_jnd_response_single_gpu_readback(inst,shader.string(),*bLmsr.first,*bAdapt.first,W,H,area,age);
      std::vector<uint32_t> bits(out.size()); for(size_t i=0;i<out.size();++i) bits[i]=std::bit_cast<uint32_t>(out[i]);
      uint64_t h=fnvU32(bits); hashes.push_back(h);
      std::cout<<"run"<<r<<"="<<std::hex<<std::setw(16)<<std::setfill('0')<<h<<std::dec<<" cpu_match="<<(h==cpuHash?1:0)<<std::endl;
      if(r==0) firstOut=out;
    }
    bool stable=true; for(size_t i=1;i<hashes.size();++i) if(hashes[i]!=hashes[0]) stable=false;
    size_t mismatchCount=0, firstMismatch=size_t(-1); double maxAbs=0.0; uint32_t maxUlp=0;
    for(size_t i=0;i<firstOut.size() && i<cpuRef.response.size(); ++i){
      uint32_t gb=std::bit_cast<uint32_t>(firstOut[i]), cb=std::bit_cast<uint32_t>(cpuRef.response[i]);
      if(gb!=cb){
        mismatchCount++; if(firstMismatch==size_t(-1)) firstMismatch=i;
        double absErr=std::abs((double)firstOut[i]-(double)cpuRef.response[i]); if(absErr>maxAbs) maxAbs=absErr;
        auto ou=[](uint32_t u)->uint32_t{ return (u&0x80000000u)?~u:(u|0x80000000u); };
        uint32_t ulp = ou(gb)>ou(cb)? ou(gb)-ou(cb): ou(cb)-ou(gb);
        if(ulp>maxUlp) maxUlp=ulp;
      }
    }
    const size_t N=size_t(W)*H;
    float lowF=float(std::pow(10.,lut.log_luminance.front()));
    float highF=float(std::pow(10.,lut.log_luminance.back()));
    auto cpuSample=[&](float query,bool rod)->float{
      auto it=std::upper_bound(lut.log_luminance.begin(),lut.log_luminance.end(),query,[](float q,double node){return q<static_cast<float>(node);});
      auto i=std::clamp<size_t>(it-lut.log_luminance.begin(),1,IQM::HDRVDP3::JND_LUT_SIZE-1)-1;
      const auto& table=rod?lut.rod:lut.cone;
      float slope=static_cast<float>((table[i+1]-table[i])/(lut.log_luminance[i+1]-lut.log_luminance[i]));
      float offset=query-static_cast<float>(lut.log_luminance[i]);
      return slope*offset+static_cast<float>(table[i]);
    };
    auto gpuFormulaSample=[&](float query,bool rod)->float{
      auto it=std::upper_bound(lut.log_luminance.begin(),lut.log_luminance.end(),query,[](float q,double node){return q<static_cast<float>(node);});
      auto i=std::clamp<size_t>(it-lut.log_luminance.begin(),1,IQM::HDRVDP3::JND_LUT_SIZE-1)-1;
      float x0=(float)lut.log_luminance[i], x1=(float)lut.log_luminance[i+1];
      float y0=(float)(rod?lut.rod[i]:lut.cone[i]), y1=(float)(rod?lut.rod[i+1]:lut.cone[i+1]);
      float slope=(y1-y0)/(x1-x0);
      float offset=query-x0;
      return slope*offset+y0;
    };
    auto log10cpu=[](float v){ return std::log10(v); };
    auto log10gpu=[](float v){ return std::log(v)*0.4342944920063018798828125f; };
    size_t simMismatch=0; double simMaxAbs=0; uint32_t simMaxUlp=0; size_t simFirst=size_t(-1);
    for(size_t idx=0; idx<N; ++idx){
      float l=gpuFormulaSample(log10gpu(std::clamp(lmsr[0*N+idx]*cpuRef.pupil_reduction,lowF,highF)),false);
      float m=gpuFormulaSample(log10gpu(std::clamp(lmsr[1*N+idx]*cpuRef.pupil_reduction,lowF,highF)),false);
      float r=gpuFormulaSample(log10gpu(std::clamp(lmsr[3*N+idx]*cpuRef.pupil_reduction,lowF,highF)),true);
      float resp=(l+m)+r;
      uint32_t gb=std::bit_cast<uint32_t>(resp), cb=std::bit_cast<uint32_t>(cpuRef.response[idx]);
      if(gb!=cb){ simMismatch++; if(simFirst==size_t(-1)) simFirst=idx; double a=std::abs((double)resp-(double)cpuRef.response[idx]); if(a>simMaxAbs) simMaxAbs=a; auto ou=[](uint32_t u)->uint32_t{ return (u&0x80000000u)?~u:(u|0x80000000u);}; uint32_t ulp=ou(gb)>ou(cb)?ou(gb)-ou(cb):ou(cb)-ou(gb); if(ulp>simMaxUlp) simMaxUlp=ulp; }
    }
    std::cout<<"STABLE="<<stable
             <<" CPU_HASH="<<std::hex<<std::setw(16)<<std::setfill('0')<<cpuHash<<std::dec
             <<" MISMATCH_COUNT="<<mismatchCount
             <<" MAX_ABS="<<std::setprecision(17)<<maxAbs
             <<" MAX_ULP="<<maxUlp
             <<" FIRST_MISMATCH="<<(firstMismatch==size_t(-1)?std::string("-1"):std::to_string(firstMismatch))<<std::endl;
        std::cout<<"SIM_GPUFORMULA_MISMATCH="<<simMismatch<<" SIM_MAX_ABS="<<std::setprecision(17)<<simMaxAbs<<" SIM_MAX_ULP="<<simMaxUlp<<" SIM_FIRST="<<(simFirst==size_t(-1)?std::string("-1"):std::to_string(simFirst))<<"\n";
    size_t onlyActual=0, onlySim=0, both=0;
    for(size_t idx=0; idx<N; ++idx){
      uint32_t ab=std::bit_cast<uint32_t>(firstOut[idx]), cb=std::bit_cast<uint32_t>(cpuRef.response[idx]);
      float l=gpuFormulaSample(log10gpu(std::clamp(lmsr[0*N+idx]*cpuRef.pupil_reduction,lowF,highF)),false);
      float m=gpuFormulaSample(log10gpu(std::clamp(lmsr[1*N+idx]*cpuRef.pupil_reduction,lowF,highF)),false);
      float r=gpuFormulaSample(log10gpu(std::clamp(lmsr[3*N+idx]*cpuRef.pupil_reduction,lowF,highF)),true);
      uint32_t sb=std::bit_cast<uint32_t>((l+m)+r);
      bool aDiff=ab!=cb, sDiff=sb!=cb;
      if(aDiff&&sDiff) both++; else if(aDiff) onlyActual++; else if(sDiff) onlySim++;
    }
    std::cout<<"CLASS both="<<both<<" onlyActual="<<onlyActual<<" onlySim="<<onlySim<<"\n";
        // dump CPU and GPU for three-way compare
    { std::ofstream fc(outDir/"cpu_resp.bin",std::ios::binary); fc.write((char*)cpuRef.response.data(),cpuRef.response.size()*4); }
    { std::ofstream fg(outDir/"gpu_resp.bin",std::ios::binary); fg.write((char*)firstOut.data(),firstOut.size()*4); }
    std::cout<<"DUMPED cpu_resp.bin gpu_resp.bin\n";std::cout<<"LUT_last_log="<<std::setprecision(17)<<(double)lut.log_luminance.back()<<" lut_last_cone="<<(double)lut.cone.back()<<" lut_last_rod="<<(double)lut.rod.back()<<"\n";
    auto dumpIdx=[&](size_t idx){
      if(idx>=firstOut.size()) return;
      std::cout<<"IDX="<<idx<<"\n";
      for(int c=0;c<3;++c){
        size_t lmsrPlane=(c==2?3:c);
        float src=lmsr[lmsrPlane*N+idx];
        float pupilVal=src*cpuRef.pupil_reduction;
        float clamped=std::clamp(pupilVal,lowF,highF);
        float qCpu=log10cpu(clamped), qGpu=log10gpu(clamped);
        bool rod=(c==2);
        float sCpu=cpuSample(qCpu,rod), sGpuFormula=gpuFormulaSample(qGpu,rod);
        std::cout<<" c"<<c<<" src="<<hexu32(std::bit_cast<uint32_t>(src))
                 <<" pupil="<<hexu32(std::bit_cast<uint32_t>(pupilVal))
                 <<" clamp="<<hexu32(std::bit_cast<uint32_t>(clamped))
                 <<" qCpu="<<hexu32(std::bit_cast<uint32_t>(qCpu))
                 <<" qGpu="<<hexu32(std::bit_cast<uint32_t>(qGpu))
                 <<" qDiff="<<(std::bit_cast<uint32_t>(qCpu)!=std::bit_cast<uint32_t>(qGpu))
                 <<" sCpu="<<hexu32(std::bit_cast<uint32_t>(sCpu))
                 <<" sGpuF="<<hexu32(std::bit_cast<uint32_t>(sGpuFormula))
                 <<" sDiffCpuGpu="<<(std::bit_cast<uint32_t>(sCpu)!=std::bit_cast<uint32_t>(sGpuFormula))<<"\n";
      }
      std::cout<<" respCpu="<<hexu32(std::bit_cast<uint32_t>(cpuRef.response[idx]))
               <<" respGpu="<<hexu32(std::bit_cast<uint32_t>(firstOut[idx]))<<"\n";
    };
    if(firstMismatch!=size_t(-1)){
      std::cout<<"FIRST_CPU="<<std::setprecision(9)<<cpuRef.response[firstMismatch]
               <<" FIRST_GPU="<<std::setprecision(9)<<firstOut[firstMismatch]
               <<" FIRST_CPU_HEX="<<hexu32(std::bit_cast<uint32_t>(cpuRef.response[firstMismatch]))
               <<" FIRST_GPU_HEX="<<hexu32(std::bit_cast<uint32_t>(firstOut[firstMismatch]))<<std::endl;
      dumpIdx(firstMismatch);
    }
    return stable?0:1;
  } catch(const std::exception& e){ std::cerr<<"EXC "<<e.what()<<std::endl; return 1; }
}