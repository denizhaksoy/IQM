#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <IQM/base/vulkan_runtime.h>
#include <IQM/hdrvdp3/sp0_single.h>
#include <IQM/hdrvdp3/sp0_double.h>
#include <IQM/hdrvdp3/sp0_resident_gpu.h>
#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#ifdef MemoryBarrier
#undef MemoryBarrier
#endif
#endif
namespace fs=std::filesystem;
struct Cmp { uint64_t mismatches=0,compared=0,nanCount=0; uint64_t maxUlp=0; double maxAbs=0,maxRel=0; size_t first=size_t(-1); float fg=0,fr=0; };
static uint32_t f2u(float v){ uint32_t u; std::memcpy(&u,&v,4); return u; }
static uint64_t ulpDiff(float a,float b){ uint32_t ai=f2u(a), bi=f2u(b); auto ord=[](uint32_t u)->int32_t{ return (u&0x80000000u)? -int32_t(~u+1u) : int32_t(u^0x80000000u); }; int64_t d=int64_t(ord(ai))-int64_t(ord(bi)); return uint64_t(d<0?-d:d); }
static Cmp compare(std::span<const float> g,std::span<const float> r,bool exactOnly){
    Cmp c; c.compared=(std::min)(g.size(),r.size());
    for(size_t i=0;i<c.compared;++i){ float gv=g[i],rv=r[i]; if(!std::isfinite(gv)||!std::isfinite(rv)){ c.nanCount++; c.mismatches++; if(c.first==size_t(-1)){c.first=i;c.fg=gv;c.fr=rv;} continue; }
        double err=std::abs(double(gv)-double(rv)); double rel=err/(std::abs(double(rv))+1e-30); uint64_t ulp=ulpDiff(gv,rv);
        if(err>c.maxAbs)c.maxAbs=err; if(rel>c.maxRel)c.maxRel=rel; if(ulp>c.maxUlp)c.maxUlp=ulp;
        bool bad = exactOnly ? (f2u(gv)!=f2u(rv)) : (err>0.0);
        if(bad){ c.mismatches++; if(c.first==size_t(-1)){c.first=i;c.fg=gv;c.fr=rv;} } }
    return c;
}
static void printCmp(const std::string& name,const Cmp& c){ std::cout<<name<<" compared="<<c.compared<<" mismatches="<<c.mismatches<<" maxAbs="<<std::setprecision(9)<<c.maxAbs<<" maxRel="<<c.maxRel<<" maxUlp="<<c.maxUlp<<" nanInf="<<c.nanCount; if(c.first!=size_t(-1)) std::cout<<" first="<<c.first<<" gpu="<<std::hexfloat<<c.fg<<" ref="<<c.fr<<std::defaultfloat; else std::cout<<std::defaultfloat; std::cout<<"\n"; }
static std::string sha256File(const fs::path& p){
#ifdef _WIN32
    std::ifstream f(p,std::ios::binary); std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)),{});
    BCRYPT_ALG_HANDLE alg=nullptr; BCRYPT_HASH_HANDLE h=nullptr; DWORD objLen=0,cb=0,hashLen=0;
    BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0);
    BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,(PUCHAR)&objLen,sizeof(DWORD),&cb,0);
    BCryptGetProperty(alg,BCRYPT_HASH_LENGTH,(PUCHAR)&hashLen,sizeof(DWORD),&cb,0);
    std::vector<uint8_t> obj(objLen), hash(hashLen);
    BCryptCreateHash(alg,&h,obj.data(),objLen,nullptr,0,0);
    BCryptHashData(h,data.data(),(ULONG)data.size(),0);
    BCryptFinishHash(h,hash.data(),hashLen,0);
    BCryptDestroyHash(h); BCryptCloseAlgorithmProvider(alg,0);
    std::ostringstream os; os<<std::hex<<std::setfill('0'); for(auto b:hash) os<<std::setw(2)<<(int)b; return os.str();
#else
    (void)p; return "unsupported";
#endif
}
struct Case { std::string name; unsigned w,h; double ppd; std::vector<float> input; };
static Case makeGradient(unsigned w,unsigned h,double ppd){ Case c{"gradient",w,h,ppd,{}}; c.input.assign(size_t(w)*h,0.f); for(unsigned y=0;y<h;++y) for(unsigned x=0;x<w;++x){ float fx=float(x)/float(w-1), fy=float(y)/float(h-1); c.input[size_t(y)*w+x]=0.01f+10.f*fx+5.f*fy+0.1f*fx*fy; } return c; }
static Case makeChecker(unsigned w,unsigned h,double ppd){ Case c{"checker",w,h,ppd,{}}; c.input.assign(size_t(w)*h,0.f); for(unsigned y=0;y<h;++y) for(unsigned x=0;x<w;++x){ bool hi=((x/8)+(y/8))%2==0; c.input[size_t(y)*w+x]=hi?100.f:0.5f; } return c; }
static Case makeImpulse(unsigned w,unsigned h,double ppd){ Case c{"impulse",w,h,ppd,{}}; c.input.assign(size_t(w)*h,0.1f); c.input[size_t(h/2)*w+w/2]=1000.f; return c; }
static Case makeRandom(unsigned w,unsigned h,double ppd,uint32_t seed){ Case c{"random_"+std::to_string(w)+"x"+std::to_string(h),w,h,ppd,{}}; c.input.resize(size_t(w)*h); std::mt19937 rng(seed); std::uniform_real_distribution<float> dist(1e-4f,200.f); for(auto& v:c.input) v=dist(rng); return c; }
static Case makeJndLike(unsigned w,unsigned h,double ppd,uint32_t seed){ Case c{"jndlike_"+std::to_string(w)+"x"+std::to_string(h),w,h,ppd,{}}; c.input.resize(size_t(w)*h); std::mt19937 rng(seed); std::lognormal_distribution<float> dist(2.0f,1.5f); for(auto& v:c.input) v=std::clamp(dist(rng),1e-5f,5000.f); return c; }
struct Buf { vk::raii::DeviceMemory m{nullptr}; vk::raii::Buffer h{nullptr}; };
static Buf makeBuf(const vk::raii::Device& dev,const vk::raii::PhysicalDevice& phys,size_t bytes,vk::MemoryPropertyFlags mem){
    auto [hb,mb]=IQM::Bin::VulkanResource::createBuffer(dev,phys,bytes,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst,mem);
    hb.bindMemory(mb,0); return Buf{std::move(mb),std::move(hb)};
}
static std::vector<float> readbackBuf(IQM::Bin::VulkanInstance& instance,const vk::raii::Device& dev,const vk::raii::PhysicalDevice& phys,const Buf& db,size_t bytes){
    auto rb=makeBuf(dev,phys,bytes,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
    auto& cmd=*instance.cmdBuf(); cmd.reset(); cmd.begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    cmd.copyBuffer(*db.h,*rb.h,{vk::BufferCopy{0,0,bytes}});
    vk::MemoryBarrier mb{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},{mb},{},{});
    cmd.end(); vk::raii::Fence f(dev,vk::FenceCreateInfo{}); instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&*cmd},*f); instance.waitForFence(f);
    std::vector<float> out(bytes/4); void* p=rb.m.mapMemory(0,bytes); std::memcpy(out.data(),p,bytes); rb.m.unmapMemory(); return out;
}
static void uploadBuf(IQM::Bin::VulkanInstance& instance,const vk::raii::Device& dev,const vk::raii::PhysicalDevice& phys,Buf& db,const void* data,size_t bytes){
    auto st=makeBuf(dev,phys,bytes,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
    void* p=st.m.mapMemory(0,bytes); std::memcpy(p,data,bytes); st.m.unmapMemory();
    auto& cmd=*instance.cmdBuf(); cmd.reset(); cmd.begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    cmd.copyBuffer(*st.h,*db.h,{vk::BufferCopy{0,0,bytes}}); cmd.end();
    vk::raii::Fence f(dev,vk::FenceCreateInfo{}); instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&*cmd},*f); instance.waitForFence(f);
}
static void barrierAfterRecord(vk::CommandBuffer cmd){
    vk::MemoryBarrier mb{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eTransferRead};
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eTransfer, {}, {mb}, {}, {});
}
static fs::path findSpv(const fs::path& dir, std::initializer_list<const char*> names){ for(auto n:names){ fs::path p=dir/n; if(fs::exists(p)) return p; p=dir/"hdrvdp3"/n; if(fs::exists(p)) return p; } return {}; }

int main(int argc,char** argv){
    if(argc<3){ std::cerr<<"Usage: "<<argv[0]<<" <shader_or_runtime_dir> <output_dir> [--repeats N]\n"; return 1; }
    fs::path shaderDir=argv[1], outDir=argv[2]; fs::create_directories(outDir);
    int repeats=10; for(int i=3;i<argc;++i){ if(std::string(argv[i])=="--repeats" && i+1<argc) repeats=std::max(1,std::atoi(argv[++i])); }
    bool exactOnly=false; for(int i=3;i<argc;++i){ if(std::string(argv[i])=="--exact") exactOnly=true; }
    try{
        IQM::Bin::VulkanInstance instance(true,"NVIDIA",true);
        if(!*instance.debugMessenger) throw std::runtime_error("validation required");
        auto& phys=*instance.physicalDevice(); auto& dev=*instance.device();
        std::cout<<"DEVICE="<<phys.getProperties().deviceName.data()<<"\n";
        auto corrdnSpv=findSpv(shaderDir,{"sp0_corrdn.spv","sp0_single_corrdn.spv"});
        auto baseSpv=findSpv(shaderDir,{"sp0_base_mean.spv","sp0_single_base_mean.spv"});
        if(corrdnSpv.empty()||baseSpv.empty()) throw std::runtime_error("SP0 spv not found in "+shaderDir.string());
        std::cout<<"CORRDN_SPV="<<corrdnSpv<<" sha256="<<sha256File(corrdnSpv)<<" bytes="<<fs::file_size(corrdnSpv)<<"\n";
        std::cout<<"BASEMEAN_SPV="<<baseSpv<<" sha256="<<sha256File(baseSpv)<<" bytes="<<fs::file_size(baseSpv)<<"\n";

        auto filters=IQM::HDRVDP3::create_sp0_model_double();
        auto toFloat=[&](const std::vector<double>& d){ std::vector<float> f(d.size()); for(size_t i=0;i<d.size();++i) f[i]=float(d[i]); return f; };
        auto lo0F=toFloat(filters.lo0), hi0F=toFloat(filters.high), loF=toFloat(filters.lo), orientF=toFloat(filters.orientation);
        auto bLo0=makeBuf(dev,phys,lo0F.size()*4,vk::MemoryPropertyFlagBits::eDeviceLocal);
        auto bHi0=makeBuf(dev,phys,hi0F.size()*4,vk::MemoryPropertyFlagBits::eDeviceLocal);
        auto bLo=makeBuf(dev,phys,loF.size()*4,vk::MemoryPropertyFlagBits::eDeviceLocal);
        auto bOrient=makeBuf(dev,phys,orientF.size()*4,vk::MemoryPropertyFlagBits::eDeviceLocal);
        uploadBuf(instance,dev,phys,bLo0,lo0F.data(),lo0F.size()*4); uploadBuf(instance,dev,phys,bHi0,hi0F.data(),hi0F.size()*4);
        uploadBuf(instance,dev,phys,bLo,loF.data(),loF.size()*4); uploadBuf(instance,dev,phys,bOrient,orientF.data(),orientF.size()*4);

        std::vector<Case> cases{makeGradient(128,128,60.0),makeChecker(128,128,60.0),makeImpulse(128,128,60.0),makeRandom(257,129,60.0,1),makeRandom(384,216,62.729130063099447,2),makeJndLike(33,17,30.0,7),makeJndLike(59,41,30.0,9)};
        IQM::HDRVDP3::SP0ResidentShaders shaders{corrdnSpv, baseSpv};

        uint64_t totalCompared=0,totalMismatch=0,totalNan=0,totalMaxUlp=0; double worstAbs=0,worstRel=0; int exitCode=0;
        for(const auto& tc:cases){
            std::cout<<"CASE="<<tc.name<<" "<<tc.w<<"x"<<tc.h<<" ppd="<<tc.ppd<<"\n";
            auto cpu=IQM::HDRVDP3::build_sp0_pyramid_single(std::span<const float>(tc.input.data(),tc.input.size()),tc.w,tc.h,tc.ppd);
            size_t inBytes=tc.input.size()*4;
            auto bIn=makeBuf(dev,phys,inBytes,vk::MemoryPropertyFlagBits::eDeviceLocal);
            uploadBuf(instance,dev,phys,bIn,tc.input.data(),inBytes);

            std::vector<std::vector<std::vector<float>>> allRuns(repeats);
            std::vector<float> allMean(repeats, NAN);
            std::vector<uint32_t> allDispatch(repeats, 0);
            for(int r=0;r<repeats;++r){
                auto bWorkA=makeBuf(dev,phys,inBytes,vk::MemoryPropertyFlagBits::eDeviceLocal);
                auto bWorkB=makeBuf(dev,phys,inBytes,vk::MemoryPropertyFlagBits::eDeviceLocal);
                auto bMean=makeBuf(dev,phys,4,vk::MemoryPropertyFlagBits::eDeviceLocal);
                std::vector<Buf> bandBufs; std::vector<IQM::HDRVDP3::FFTBufferRegion> bandRegs;
                for(size_t b=0;b<cpu.bands.size();++b){ size_t bytes=cpu.bands[b].values.size()*4; bandBufs.push_back(makeBuf(dev,phys,bytes,vk::MemoryPropertyFlagBits::eDeviceLocal)); bandRegs.push_back({*bandBufs.back().h,0,bytes}); }
                IQM::HDRVDP3::SP0ResidentBuffers bufs; bufs.input={*bIn.h,0,inBytes}; bufs.lo0Filter={*bLo0.h,0,lo0F.size()*4}; bufs.hi0Filter={*bHi0.h,0,hi0F.size()*4}; bufs.loFilter={*bLo.h,0,loF.size()*4}; bufs.orientFilter={*bOrient.h,0,orientF.size()*4}; bufs.workA={*bWorkA.h,0,inBytes}; bufs.workB={*bWorkB.h,0,inBytes}; bufs.bandRegions=bandRegs; bufs.baseMean={*bMean.h,0,4};
                IQM::HDRVDP3::ResidentSP0Pyramid gpu(dev,phys,shaders,bufs,tc.w,tc.h,tc.ppd);
                auto& cmd=*instance.cmdBuf(); cmd.reset(); cmd.begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit}); gpu.record(cmd);
                barrierAfterRecord(cmd);
                cmd.end(); vk::raii::Fence fence(dev,vk::FenceCreateInfo{}); instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&*cmd},*fence); instance.waitForFence(fence);
                allDispatch[r]=gpu.dispatchCount();
                allRuns[r].resize(cpu.bands.size());
                for(size_t b=0;b<cpu.bands.size();++b) allRuns[r][b]=readbackBuf(instance,dev,phys,bandBufs[b],cpu.bands[b].values.size()*4);
                auto meanGot=readbackBuf(instance,dev,phys,bMean,4); allMean[r]=meanGot.empty()?NAN:meanGot[0];
            }

            bool casePass=true;
            // Determinism across repeats
            bool det=true;
            for(int r=1;r<repeats;++r){
                if(allDispatch[r]!=allDispatch[0]) det=false;
                if(f2u(allMean[r])!=f2u(allMean[0])) det=false;
                for(size_t b=0;b<cpu.bands.size();++b){ if(allRuns[r][b].size()!=allRuns[0][b].size()) {det=false; continue;} for(size_t i=0;i<allRuns[0][b].size();++i) if(f2u(allRuns[r][b][i])!=f2u(allRuns[0][b][i])) {det=false; break;} }
            }
            std::cout<<"  determinism="<<(det?"PASS":"FAIL")<<" repeats="<<repeats<<" dispatches="<<allDispatch[0]<<"\n";
            if(!det) { casePass=false; exitCode=1; }

            for(size_t b=0;b<cpu.bands.size();++b){
                auto cmp=compare(allRuns[0][b],cpu.bands[b].values,exactOnly);
                std::ostringstream nm; nm<<"  band"<<b<<"("<<cpu.bands[b].width<<"x"<<cpu.bands[b].height<<")";
                printCmp(nm.str(),cmp);
                totalCompared+=cmp.compared; totalMismatch+=cmp.mismatches; totalNan+=cmp.nanCount;
                worstAbs=(std::max)(worstAbs,cmp.maxAbs); worstRel=(std::max)(worstRel,cmp.maxRel); totalMaxUlp=(std::max)(totalMaxUlp,cmp.maxUlp);
                if(cmp.mismatches||cmp.nanCount) casePass=false;
            }
            float gpuMean=allMean[0];
            double meanErr=std::abs(double(gpuMean)-double(cpu.base_mean));
            uint64_t meanUlp=ulpDiff(gpuMean,cpu.base_mean);
            std::cout<<"  base_mean gpu="<<std::hexfloat<<gpuMean<<" ref="<<cpu.base_mean<<std::defaultfloat<<" absErr="<<std::setprecision(9)<<meanErr<<" ulp="<<meanUlp<<"\n";
            totalCompared++; if(f2u(gpuMean)!=f2u(cpu.base_mean)){ totalMismatch++; casePass=false; worstAbs=(std::max)(worstAbs,meanErr); totalMaxUlp=(std::max)(totalMaxUlp,meanUlp); }
            std::cout<<(casePass?"CASE_PASS":"CASE_FAIL")<<"\n";
            if(!casePass) exitCode=1;
        }
        std::cout<<"SUMMARY compared="<<totalCompared<<" mismatches="<<totalMismatch<<" nanInf="<<totalNan<<" maxAbs="<<std::setprecision(9)<<worstAbs<<" maxRel="<<worstRel<<" maxUlp="<<totalMaxUlp<<"\n";
        std::cout<<"VULKAN_VALIDATION_ERRORS=none\n";
        std::cout<<(exitCode==0?"ALL_PASS":"ALL_FAIL")<<"\n";
        return exitCode;
    } catch (const std::exception& e) {
        std::cerr<<"EXCEPTION: "<<e.what()<<"\n";
        return 1;
    }
}
