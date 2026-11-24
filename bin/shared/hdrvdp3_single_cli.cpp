#include <IQM/hdrvdp3/resize.h>
#include <IQM/hdrvdp3/csf_single_gpu.h>
#include <IQM/hdrvdp3/csf.h>
#include <type_traits>
#include <cstring>
#include <IQM/hdrvdp3/residual_single_gpu.h>
#include <cstdlib>
#include <array>
#include <functional>
#include "hdrvdp3_single_cli.h"
#include <stb_image.h>
#include <IQM/hdrvdp3/fftw_single_provider.h>
#include <IQM/hdrvdp3/general_quality_single.h>
#include <IQM/hdrvdp3/linear_color_single.h>
#include <IQM/hdrvdp3/optical_single.h>
#include <IQM/hdrvdp3/optical_response_single.h>
#include <IQM/hdrvdp3/jnd.h>
#include <IQM/hdrvdp3/pq_single.h>
#include <IQM/hdrvdp3/pq_display_single_gpu.h>
#include <IQM/hdrvdp3/optical_mtf_gpu.h>
#include <IQM/hdrvdp3/local_adapt_single_gpu.h>
#include <IQM/hdrvdp3/adaptation_input_single_gpu_device.h>
#include <IQM/hdrvdp3/jnd_single_gpu.h>
#include <IQM/hdrvdp3/sp0_single_gpu.h>
#include <IQM/hdrvdp3/mutual_mask_resident_gpu.h>
#include <optional>
#include "vulkan_res.h"
#include <vulkan/vulkan.hpp>
#include <IQM/hdrvdp3/resize.h>
#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <unordered_map>
#ifdef IQM_HDRVDP3_HYBRID
#include "../IQM/vulkan_instance.h"
#ifdef IQM_HDRVDP3_COMPARISON
#include <IQM/hdrvdp3/comparison_single_gpu.h>
#ifdef IQM_HDRVDP3_MUTUAL
#include <IQM/hdrvdp3/mutual_single_gpu.h>
#include <IQM/hdrvdp3/comparison_resize_gpu.h>
#include <IQM/hdrvdp3/adaptation_input_single_gpu.h>
#include <IQM/hdrvdp3/padding_single_gpu.h>
#ifdef MemoryBarrier
#undef MemoryBarrier
#endif

#endif
#endif
#endif
namespace IQM::HDRVDP3 {
QualitySideSingle prepare_quality_side_single_diagnostic(std::span<const float>,unsigned,unsigned,double,const XYZDisplaySamples&,const RGBSpectralSamples&,const FFTBackendSingle&,const SingleMatrixBackend*,const SingleAdaptationInputBackend*,const SinglePaddingBackend*,const std::function<void(const char*,std::span<const float>)>&);
}
namespace fs=std::filesystem;using namespace IQM::HDRVDP3;
static void single_quality_help(){std::cout<<"IQM HDR-VDP-3 Single quality (explicit FFTW)\n"
 "Usage: <executable> --input test.png --ref reference.png\n"
 "  --encoding pq-bt2020-rgb16 --ppd NUMBER --model-data DIR\n"
 "  --fftw PATH_TO_FLOAT_FFTW_DLL --output FRESH_DIRECTORY\n"
 "Profile: quality/sp0, age24, OLED, ambient200lux, reflectivity0.005,\n"
 "peak2000cd/m2 before ambient. RGB16 PNG codes must already be PQ BT2020.\n"
 "Distorted input is bicubic-resized in PQ to the reference size and clamped.\n"
 "--full-gpu: experimental stage 1 moves PQ decode+native matrix projection to Vulkan;\ndownstream stages remain existing validated CPU/GPU-hybrid; boundary readback only at legacy contract.\n";
#ifdef IQM_HDRVDP3_HYBRID
std::cout<<"Hybrid executable additionally REQUIRES --projection-shader PATH:\nVulkan native/LMSR matrices; CPU FFT/adaptation/JND/pyramid/quality.\n";
#ifdef IQM_HDRVDP3_COMPARISON
std::cout<<"Experimental mask profile also REQUIRES --comparison-shader PATH and shaderFloat64.\nNative comparison mask executes on Vulkan; remaining quality arithmetic stays CPU.\n";
#ifdef IQM_HDRVDP3_MUTUAL
std::cout<<"Mutual profile additionally REQUIRES --mutual-shader PATH; per-band mutual activity executes on Vulkan.\n";
std::cout<<"Optional --padding-shader PATH runs optical/adaptation padding on Vulkan.\n";
std::cout<<"Optional --adaptation-input-shader PATH runs L+M on Vulkan.\n";
std::cout<<"Optional --comparison-resize-shader PATH also runs Double comparison-mask resizing on Vulkan.\n";
#endif
#endif
#endif
}
int IQM::Bin::run_hdrvdp3_single_cli(int argc,const char* argv[]){try{
 if(argc==2&&std::string(argv[1])=="--help"){single_quality_help();return 0;}
 std::vector<std::string> keys{"--input","--ref","--encoding","--ppd","--model-data","--fftw","--output"};
#ifdef IQM_HDRVDP3_HYBRID
 keys.push_back("--projection-shader");
#ifdef IQM_HDRVDP3_COMPARISON
#ifdef IQM_HDRVDP3_MUTUAL
 keys.push_back("--mutual-shader");
#endif
#endif
#endif
 std::unordered_map<std::string,std::string> options;
 auto acceptedKeys=keys;
#ifdef IQM_HDRVDP3_MUTUAL
  acceptedKeys.push_back("--comparison-shader");
 acceptedKeys.push_back("--comparison-resize-shader");
 acceptedKeys.push_back("--adaptation-input-shader");
   acceptedKeys.push_back("--pq-display-shader");
  acceptedKeys.push_back("--full-gpu");
acceptedKeys.push_back("--padding-shader");
  acceptedKeys.push_back("--jnd-shader");
  acceptedKeys.push_back("--sp0-corrdn-shader");
  acceptedKeys.push_back("--sp0-base-mean-shader");
  acceptedKeys.push_back("--cpu-sp0");
#endif
  bool fullGpu=false;
  bool cpuSp0=false;
  for(int i=1;i<argc;i++){
    std::string key=argv[i];
    if(key=="--full-gpu"){ if(fullGpu)throw std::invalid_argument("Duplicate argument: --full-gpu"); fullGpu=true; continue; }
    if(key=="--cpu-sp0"){ if(cpuSp0)throw std::invalid_argument("Duplicate argument: --cpu-sp0"); cpuSp0=true; continue; }
    if(std::find(acceptedKeys.begin(),acceptedKeys.end(),key)==acceptedKeys.end()||i+1>=argc||!argv[i+1][0]||std::string(argv[i+1]).starts_with("--")||!options.emplace(key,argv[i+1]).second)throw std::invalid_argument("Unknown, missing or duplicate argument: "+key);
    ++i;
  }
 for(const auto& key:keys)
    if(key!="--fftw" && !options.contains(key))
        throw std::invalid_argument("Required argument: "+key);
 if(options.at("--encoding")!="pq-bt2020-rgb16")throw std::invalid_argument("Only explicit pq-bt2020-rgb16 encoding is supported");
 size_t parsed=0;double ppd=std::stod(options.at("--ppd"),&parsed);if(parsed!=options.at("--ppd").size()||!std::isfinite(ppd)||ppd<26||ppd>256)throw std::invalid_argument("PPD must be finite in [26,256]");
 fs::path data=options.at("--model-data"),out=options.at("--output");if(fs::exists(out)||!fs::create_directories(out))throw std::invalid_argument("Fresh output directory required");
 auto envEnabled=[](const char* name){
    const char* value=std::getenv(name);
    return value && std::string(value)=="1";
};

const bool residentQualityNoFftw=
    fullGpu && !cpuSp0 &&
    envEnabled("IQM_HDRVDP3_RESIDENT_MUTUAL") &&
    envEnabled("IQM_HDRVDP3_RESIDENT_RESIDUAL") &&
    envEnabled("IQM_HDRVDP3_RESIDENT_CSF") &&
    envEnabled("IQM_HDRVDP3_RESIDENT_MASKING") &&
    envEnabled("IQM_HDRVDP3_RESIDENT_COMPARISON") &&
    envEnabled("IQM_HDRVDP3_RESIDENT_POOLING") &&
    envEnabled("IQM_HDRVDP3_RESIDENT_FINAL_SCORE") &&
    !envEnabled("IQM_HDRVDP3_RESIDUAL_PROBE") &&
    !envEnabled("IQM_HDRVDP3_RESIDUAL_COMPARE") &&
    !envEnabled("IQM_HDRVDP3_CSF_PROBE") &&
    !envEnabled("IQM_HDRVDP3_CSF_RESIZE_PROBE") &&
    !envEnabled("IQM_HDRVDP3_CSF_4K_PROBE") &&
    (!std::getenv("IQM_HDRVDP3_DIAG_RUN") ||
     std::atoi(std::getenv("IQM_HDRVDP3_DIAG_RUN"))==0);

std::unique_ptr<FFTWProviderSingle> fft;
FFTBackendSingle backend{};

if(residentQualityNoFftw){
    backend.convolve=[](
        std::span<const float>,
        std::span<const double>,
        unsigned,unsigned,unsigned,unsigned
    )->std::vector<float>{
        throw std::runtime_error(
            "CPU FFT convolution forbidden in resident GPU quality");
    };

    backend.forward=[](
        std::span<const float>,
        unsigned,unsigned
    )->std::vector<float>{
        throw std::runtime_error(
            "CPU FFT forward forbidden in resident GPU quality");
    };
}else{
    if(!options.contains("--fftw"))
        throw std::invalid_argument(
            "--fftw is required for hybrid and diagnostic execution");

    fft=std::make_unique<FFTWProviderSingle>(
        options.at("--fftw"));

    backend=fft->backend();
}

std::ofstream fftwReport(out/"fftw_dependency.txt");

fftwReport
    <<"resident_quality_no_fftw="<<residentQualityNoFftw<<'\n'
    <<"fftw_provider_created="<<(fft!=nullptr)<<'\n'
    <<"fftw_argument_supplied="<<options.contains("--fftw")<<'\n'
    <<"cpu_fft_fallback_forbidden="<<residentQualityNoFftw<<'\n';

fftwReport.close();

if(!fftwReport)
    throw std::runtime_error("FFTW dependency report failed");
 const SingleMatrixBackend* matrices=nullptr;
 std::string backendName="cpu-fftw";
 unsigned matrixDispatches=0;
 unsigned adaptationInputDispatches=0;
 const SingleAdaptationInputBackend* adaptationInput=nullptr;
 const SinglePaddingBackend* padding=nullptr;unsigned paddingDispatches=0;
 const SingleComparisonBackend* comparison=nullptr;
 unsigned comparisonDispatches=0;
 const SingleMutualBackend* mutual=nullptr;
 unsigned mutualDispatches=0;
 const SingleComparisonResizeBackend* comparisonResize=nullptr;
 unsigned comparisonResizeCalls=0,comparisonResizeDispatches=0;
#ifdef IQM_HDRVDP3_HYBRID
#ifdef IQM_HDRVDP3_COMPARISON
 auto gpu=std::make_unique<IQM::Bin::VulkanInstance>(true,"",true);
    SingleComparisonBackend gpuComparison;
  const bool hasCmpShader = options.contains("--comparison-shader");
  std::cerr<<"[DBG] single_cli has_comparison_shader="<<(hasCmpShader?"1":"0")<<"\n";
  if(hasCmpShader){
   std::cerr<<"[DBG] constructing SingleComparisonBackend\n";
   gpuComparison=SingleComparisonBackend{[&](auto r,auto t){std::cerr<<"[DBG] comparison lambda invoked\n"; auto values=comparison_single_gpu(*gpu,options.at("--comparison-shader"),r,t);++comparisonDispatches;return values;}};
   comparison=&gpuComparison;
  } else { std::cerr<<"[DBG] comparison backend NOT constructed\n"; }
#ifdef IQM_HDRVDP3_MUTUAL
 SingleMutualBackend gpuMutual{[&](auto r,auto t,unsigned w,unsigned h){auto values=mutual_single_gpu(*gpu,options.at("--mutual-shader"),r,t,w,h);++mutualDispatches;return values;}};
 mutual=&gpuMutual;
 SingleComparisonResizeBackend gpuResize{[&](auto mask,unsigned h,unsigned w,unsigned nh,unsigned nw){++comparisonResizeCalls;auto result=resize_comparison_gpu(*gpu,options.at("--comparison-resize-shader"),mask,h,w,nh,nw);comparisonResizeDispatches+=result.dispatches;return std::move(result.values);}};
 if(options.contains("--comparison-resize-shader"))comparisonResize=&gpuResize;
 SingleAdaptationInputBackend gpuAdaptationInput{[&](auto l,auto m){auto values=adaptation_input_single_gpu(*gpu,options.at("--adaptation-input-shader"),l,m);++adaptationInputDispatches;return values;}};
 if(options.contains("--adaptation-input-shader"))adaptationInput=&gpuAdaptationInput;
 SinglePaddingBackend gpuPadding{[&](auto input,unsigned w,unsigned h,bool symmetric){auto values=padding_single_gpu(*gpu,options.at("--padding-shader"),input,w,h,symmetric);++paddingDispatches;return values;}};
 if(options.contains("--padding-shader"))padding=&gpuPadding;
#endif
#else
 auto gpu=std::make_unique<IQM::Bin::VulkanInstance>(true);
#endif
// Opt-in isolated diagnostic; no production score is emitted.
 if(const char* probe=std::getenv("IQM_HDRVDP3_RESIDUAL_PROBE");probe && std::string(probe)=="1"){
   constexpr unsigned rw=240,rh=135,n=rw*rh;
   const auto& d=*gpu->device(); const auto& p=*gpu->physicalDevice();
   auto alloc=[&](unsigned bytes){auto a=IQM::Bin::VulkanResource::createBuffer(d,p,bytes,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);a.first.bindMemory(a.second,0);return a;};
   auto r=alloc(n*4),t=alloc(n*4),s=alloc(n*8),sel=alloc(12),freq=alloc(8);
   auto runtime=fs::path(options.at("--projection-shader")).parent_path();
   ResidentResidualSingle job(d,p,*(*gpu->queue()),*(*gpu->cmdPool()),{{*r.first,0,n*4},{*t.first,0,n*4},{*s.first,0,n*8},{*sel.first,0,12},{*freq.first,0,8}},runtime,rw,rh,6,ppd,gpu->float64Enabled);
   std::ofstream report(out/"residual_probe.txt");report<<std::setprecision(17);
   unsigned failures=0;
   for(unsigned c=0;c<5;++c){
     std::vector<float> a(n,0),b(n),col(n);
     for(unsigned y=0;y<rh;++y)for(unsigned x=0;x<rw;++x){unsigned i=y*rw+x; double phase=6.2831853071795864769*(double(7*x)/rw+double(3*y)/rh);b[i]=c==0?0.f:c==1?2.f:c==2?float(std::cos(phase)):c==3?float(std::sin(6.2831853071795864769*11*y/rh)):float((int((i*1664525u+1013904223u)>>16)%1024)-512)/512.f;col[x*rh+y]=b[i]-a[i];}
     auto oracle=backend.forward(col,rw,rh);unsigned expected=0;float maxv=0;
     for(unsigned i=1;i<n;++i){float m=std::hypot(oracle[2*i],oracle[2*i+1]);if(m>maxv){maxv=m;expected=i;}}
     double base=std::ldexp(1.,-5)*ppd/2;auto axis=[&](unsigned k,unsigned z){return ((std::fmod(.5+double(k)/z,1.)-.5)*(base*2))*2;};double fx=axis(expected/rh,rw),fy=axis(expected%rh,rh),expectedFreq=std::sqrt(fx*fx+fy*fy);
     auto upload=[&](auto& mem,const auto& v){void* ptr=mem.mapMemory(0,n*4);std::memcpy(ptr,v.data(),n*4);mem.unmapMemory();};upload(r.second,a);upload(t.second,b);
     std::array<uint32_t,3> previous{};double previousFreq=0;
     for(unsigned rep=0;rep<3;++rep){
       gpu->cmdBuf()->reset();gpu->cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
       vk::MemoryBarrier hb{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};gpu->cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{},{hb},{},{});
       job.record(*gpu->cmdBuf());
       vk::MemoryBarrier hr{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};gpu->cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eHost,{},{hr},{},{});
       gpu->cmdBuf()->end();vk::CommandBuffer cmd=*gpu->cmdBuf();vk::raii::Fence fence(d,vk::FenceCreateInfo{});gpu->queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);gpu->waitForFence(fence);
       std::array<uint32_t,3> actual{};double f=0;void* ptr=sel.second.mapMemory(0,12);std::memcpy(actual.data(),ptr,12);sel.second.unmapMemory();ptr=freq.second.mapMemory(0,8);std::memcpy(&f,ptr,8);freq.second.unmapMemory();
       bool stable=rep==0||(actual==previous&&std::bit_cast<uint64_t>(f)==std::bit_cast<uint64_t>(previousFreq));bool ok=actual[0]==expected&&actual[1]==0&&std::bit_cast<uint64_t>(f)==std::bit_cast<uint64_t>(expectedFreq)&&stable;
       report<<"case="<<c<<" repeat="<<rep<<" cpu_index="<<expected<<" gpu_index="<<actual[0]<<" cpu_frequency="<<expectedFreq<<" gpu_frequency="<<f<<" invalid="<<actual[1]<<" stable="<<stable<<" pass="<<ok<<'\n'<<std::flush;failures+=!ok;previous=actual;previousFreq=f;
     }
   }
   report<<"FAILURES="<<failures<<'\n';std::cout<<"RESIDUAL_PROBE_FAILURES="<<failures<<'\n';return failures?1:0;
 }

 if(const char* probe=std::getenv("IQM_HDRVDP3_CSF_PROBE");
    probe && std::string(probe)=="1"){

    if(!gpu || !gpu->float64Enabled)
        throw std::runtime_error("CSF probe requires Float64");

    const auto& d=*gpu->device();
    const auto& p=*gpu->physicalDevice();

    auto runtime=fs::path(
        options.at("--projection-shader")).parent_path();

    auto alloc=[&](unsigned bytes){
        auto a=IQM::Bin::VulkanResource::createBuffer(
            d,p,bytes,
            vk::BufferUsageFlagBits::eStorageBuffer |
            vk::BufferUsageFlagBits::eTransferSrc |
            vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);
        a.first.bindMemory(a.second,0);
        return a;
    };

    auto upload=[&](auto& b,const auto& values){
        size_t bytes=values.size()*sizeof(values[0]);
        void* ptr=b.second.mapMemory(0,bytes);
        std::memcpy(ptr,values.data(),bytes);
        b.second.unmapMemory();
    };

    auto download=[&](auto& b,auto& values){
        size_t bytes=values.size()*sizeof(values[0]);
        void* ptr=b.second.mapMemory(0,bytes);
        std::memcpy(values.data(),ptr,bytes);
        b.second.unmapMemory();
    };

    auto region=[](auto& b,unsigned bytes){
        return FFTBufferRegion{*b.first,0,bytes};
    };

    auto dispatch=[&](
        CSFKernelSingle kind,
        const std::vector<FFTBufferRegion>& regions,
        const std::vector<uint32_t>& push,
        unsigned gx,unsigned gy){

        CSFDispatchSingle job(
            d,p,runtime,kind,regions,push,gx,gy,
            gpu->float64Enabled);

        gpu->cmdBuf()->reset();

        gpu->cmdBuf()->begin(vk::CommandBufferBeginInfo{
            .flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

        job.record(*gpu->cmdBuf());

        vk::MemoryBarrier hr{
            .srcAccessMask=vk::AccessFlagBits::eShaderWrite,
            .dstAccessMask=vk::AccessFlagBits::eHostRead};

        gpu->cmdBuf()->pipelineBarrier(
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eHost,
            {},{hr},{},{});

        gpu->cmdBuf()->end();

        vk::CommandBuffer cmd=*gpu->cmdBuf();
        vk::raii::Fence fence(d,vk::FenceCreateInfo{});

        gpu->queue()->submit(vk::SubmitInfo{
            .commandBufferCount=1,
            .pCommandBuffers=&cmd},fence);

        gpu->waitForFence(fence);
        gpu->cmdBuf()->reset();
    };

    if(const char* resizeProbe =
        std::getenv("IQM_HDRVDP3_CSF_RESIZE_PROBE");
        resizeProbe && std::string(resizeProbe)=="1") {

        std::ofstream resizeReport(out/"csf_resize_parity.txt");

        if(!resizeReport)
            throw std::runtime_error("Resize report open failed");

        resizeReport << std::setprecision(17);

        struct ResizeCase {
            const char* name;
            unsigned w,h,nw,nh;
        };

        const ResizeCase cases[] = {
            {"identity",32,32,32,32},
            {"downsample",64,48,32,24},
            {"upsample",32,24,64,48},
            {"odd_dimensions",33,17,19,11},
            {"horizontal_only",64,32,32,32},
            {"vertical_only",32,64,32,32},
            {"4k_pyramid",3840,2160,1920,1080},
            {"4k_residual",3840,2160,240,135},
            {"4k_level2",3840,2160,960,540},
            {"4k_level3",3840,2160,480,270},
            {"cross_1920_to_960",1920,1080,960,540},
            {"cross_960_to_480",960,540,480,270},
            {"cross_480_to_240",480,270,240,135},
            {"cross_240_to_480",240,135,480,270},
            {"cross_480_to_960",480,270,960,540},
            {"cross_960_to_1920",960,540,1920,1080},
            {"cross_1920_to_3840",1920,1080,3840,2160},
            {"cross_3840_to_1920",3840,2160,1920,1080}
        };

        size_t totalValues=0;
        size_t totalMismatch=0;
        size_t totalNonfinite=0;
        unsigned failures=0;
        unsigned passed=0;

        for(const auto& c:cases) {

            std::cout<<"RESIZE_CASE="<<c.name<<std::endl;

            std::vector<float> input(size_t(c.w)*c.h);

            uint32_t state=123456789u;

            for(auto& v:input) {
                state=1664525u*state+1013904223u;
                v=float(int(state%20001u)-10000)/2000.0f;
            }

            auto cpu=resize_bicubic(
                input,c.h,c.w,c.nh,c.nw);

            const unsigned inputBytes=
                unsigned(input.size()*sizeof(float));

            const unsigned tempBytes=
                unsigned(size_t(c.nw)*c.h*sizeof(float));

            const unsigned outputBytes=
                unsigned(size_t(c.nw)*c.nh*sizeof(float));

            auto inputBuffer=alloc(inputBytes);
            auto tempBuffer=alloc(tempBytes);
            auto outputBuffer=alloc(outputBytes);

            upload(inputBuffer,input);

            auto runAxis=[&](
                auto& src,auto& dst,
                unsigned iw,unsigned ih,
                unsigned ow,unsigned oh,
                bool horizontal) {

                unsigned n=horizontal?iw:ih;
                unsigned m=horizontal?ow:oh;

                auto plan=make_resize_axis_plan(
                    int(n),int(m));

                std::vector<int32_t> indices(
                    plan.indices.begin(),
                    plan.indices.end());

                std::vector<float> weights(
                    plan.weights.begin(),
                    plan.weights.end());

                std::vector<float> norm(
                    plan.normalization.begin(),
                    plan.normalization.end());

                unsigned ib=unsigned(
                    indices.size()*sizeof(int32_t));

                unsigned wb=unsigned(
                    weights.size()*sizeof(float));

                unsigned nb=unsigned(
                    norm.size()*sizeof(float));

                auto indexBuffer=alloc(ib);
                auto weightBuffer=alloc(wb);
                auto normBuffer=alloc(nb);

                upload(indexBuffer,indices);
                upload(weightBuffer,weights);
                upload(normBuffer,norm);

                dispatch(
                    CSFKernelSingle::Resize,
                    {
                        region(src,unsigned(
                            size_t(iw)*ih*sizeof(float))),
                        region(indexBuffer,ib),
                        region(weightBuffer,wb),
                        region(normBuffer,nb),
                        region(dst,unsigned(
                            size_t(ow)*oh*sizeof(float)))
                    },
                    {
                        iw,ih,ow,oh,
                        uint32_t(plan.taps),
                        horizontal?1u:0u
                    },
                    (ow+15u)/16u,
                    (oh+15u)/16u
                );
            };

            runAxis(
                inputBuffer,tempBuffer,
                c.w,c.h,c.nw,c.h,true);

            runAxis(
                tempBuffer,outputBuffer,
                c.nw,c.h,c.nw,c.nh,false);

            std::vector<float> gpuValues(
                size_t(c.nw)*c.nh);

            download(outputBuffer,gpuValues);

            if(cpu.size()!=gpuValues.size())
                throw std::runtime_error(
                    "Resize comparison shape mismatch");

            size_t mismatch=0;
            size_t nonfinite=0;
            double maxAbs=0;
            double maxRel=0;

            for(size_t i=0;i<cpu.size();++i) {

                float a=cpu[i];
                float b=gpuValues[i];

                if(std::memcmp(&a,&b,sizeof(float))!=0)
                    ++mismatch;

                if(!std::isfinite(a)||!std::isfinite(b)){
                    ++nonfinite;
                    continue;
                }

                double err=std::abs(double(a)-double(b));

                maxAbs=std::max(maxAbs,err);

                maxRel=std::max(
                    maxRel,
                    err/std::max(std::abs(double(a)),1e-30));
            }

            bool pass=mismatch==0 && nonfinite==0;

            totalValues+=cpu.size();
            totalMismatch+=mismatch;
            totalNonfinite+=nonfinite;

            if(pass) ++passed;
            else ++failures;

            resizeReport
                <<c.name
                <<" input="<<c.w<<"x"<<c.h
                <<" output="<<c.nw<<"x"<<c.nh
                <<" values="<<cpu.size()
                <<" mismatch="<<mismatch
                <<" nonfinite="<<nonfinite
                <<" max_abs="<<maxAbs
                <<" max_rel="<<maxRel
                <<" PASS="<<pass<<'\n';

            resizeReport.flush();
        }

        resizeReport
            <<"TOTAL_CASES=18\n"
            <<"PASSED_CASES="<<passed<<'\n'
            <<"FAILED_CASES="<<failures<<'\n'
            <<"TOTAL_VALUES="<<totalValues<<'\n'
            <<"TOTAL_MISMATCH="<<totalMismatch<<'\n'
            <<"TOTAL_NONFINITE="<<totalNonfinite<<'\n';

        resizeReport.flush();

        std::cout<<"CSF_RESIZE_FAILURES="
                 <<failures<<std::endl;

        return failures?1:0;
    }

    std::ofstream report(out/"csf_parity.txt");

    if(!report)
        throw std::runtime_error("CSF report open failed");

    report<<std::setprecision(17);

    unsigned failures=0;
    size_t totalValues=0,totalMismatch=0;

    auto compare=[&](const std::string& name,
                     const auto& cpu,const auto& gpuValues){

        if(cpu.size()!=gpuValues.size())
            throw std::runtime_error("CSF comparison shape");

        size_t mismatch=0,nonfinite=0;
        double maxAbs=0,maxRel=0;

        for(size_t i=0;i<cpu.size();++i){
            using T=std::remove_cvref_t<decltype(cpu[i])>;

            T a=cpu[i],b=gpuValues[i];

            bool same=std::memcmp(&a,&b,sizeof(T))==0;

            if(!same) ++mismatch;

            if(!std::isfinite(a)||!std::isfinite(b)){
                ++nonfinite;
                continue;
            }

            double err=std::abs(double(a)-double(b));

            maxAbs=std::max(maxAbs,err);
            maxRel=std::max(maxRel,
                err/std::max(std::abs(double(a)),1e-30));
        }

        totalValues+=cpu.size();
        totalMismatch+=mismatch;

        bool pass=mismatch==0 && nonfinite==0;

        if(!pass) ++failures;

        report<<name
            <<" values="<<cpu.size()
            <<" mismatch="<<mismatch
            <<" nonfinite="<<nonfinite
            <<" max_abs="<<maxAbs
            <<" max_rel="<<maxRel
            <<" PASS="<<pass<<'\n';
    };

    constexpr unsigned n=1024;

    std::vector<float> ref(n),test(n),cpuLog(n),gpuLog(n);

    for(unsigned i=0;i<n;++i){
        ref[i]=std::pow(10.f,-6.f+12.f*float(i)/float(n-1));
        test[i]=ref[i]*(0.5f+float(i%17)/16.f);

        cpuLog[i]=std::log10(std::clamp(
            (test[i]+ref[i])/2.f,1e-5f,1e5f));
    }

    auto a=alloc(n*4),b=alloc(n*4),log=alloc(n*4);

    upload(a,ref);
    upload(b,test);

    dispatch(CSFKernelSingle::Log,
        {region(a,n*4),region(b,n*4),region(log,n*4)},
        {n},(n+63)/64,1);

    download(log,gpuLog);

    compare("log_luminance",cpuLog,gpuLog);

    // Identical CPU-generated inputs isolate downstream kernels.
    upload(log,cpuLog);

    auto freq=alloc(8);
    auto grid=alloc(256*4);
    auto sensitivity=alloc(256*4);
    auto slopes=alloc(255*4);
    auto noise=alloc(8);
    auto sampled=alloc(n*4);

    const std::array<double,1> residual{
        0.032671421907864179
    };

    upload(freq,residual);

    for(unsigned band=0;band<6;++band){

        double rho=std::ldexp(1.,-int(band))*ppd/2;

        uint64_t bits=std::bit_cast<uint64_t>(rho);

        std::vector<uint32_t> push{
            uint32_t(bits),
            uint32_t(bits>>32),
            32,32,0
        };

        dispatch(CSFKernelSingle::Generate,
            {region(log,n*4),region(freq,8),
             region(grid,256*4),
             region(sensitivity,256*4),
             region(slopes,255*4),region(noise,8)},
            push,4,1);

        auto cpuTable=create_csf_lut(rho);
        auto cpuSingle=create_csf_single_lut(cpuTable);

        std::array<float,256> gpuGrid{},gpuSensitivity{};
        std::array<float,255> gpuSlopes{};

        download(grid,gpuGrid);
        download(sensitivity,gpuSensitivity);
        download(slopes,gpuSlopes);

        std::string prefix="band_"+std::to_string(band);

        compare(prefix+"_grid",
            cpuSingle.log_luminance,gpuGrid);

        compare(prefix+"_sensitivity",
            cpuSingle.sensitivity,gpuSensitivity);

        compare(prefix+"_slopes",
            cpuSingle.slope,gpuSlopes);

        // Isolate sampling from LUT generation errors.
        upload(grid,cpuSingle.log_luminance);
        upload(sensitivity,cpuSingle.sensitivity);
        upload(slopes,cpuSingle.slope);

        dispatch(CSFKernelSingle::Sample,
            {region(log,n*4),region(grid,256*4),
             region(sensitivity,256*4),
             region(slopes,255*4),region(sampled,n*4)},
            {n},(n+63)/64,1);

        std::vector<float> cpuSample(n),gpuSample(n);

        for(unsigned i=0;i<n;++i)
            cpuSample[i]=sample_csf_lut_log_single(
                cpuTable,cpuLog[i]);

        download(sampled,gpuSample);

        compare(prefix+"_sample",
            cpuSample,gpuSample);
    }

    // Neural noise: preserve column-major mean order.
    double sum=0;

    for(unsigned x=0;x<32;++x)
        for(unsigned y=0;y<32;++y)
            sum+=cpuLog[y*32+x];

    float meanLog=float(sum/double(n));
    float lum=std::pow(10.f,meanLog);

    std::array<double,1> cpuNoise{
        1./compute_ncsf_single_luminance(
            residual[0],lum)
    };

    std::array<double,1> gpuNoise{};

    dispatch(CSFKernelSingle::Generate,
        {region(log,n*4),region(freq,8),
         region(grid,256*4),region(sensitivity,256*4),
         region(slopes,255*4),region(noise,8)},
        {0,0,32,32,1},1,1);

    download(noise,gpuNoise);

    compare("residual_neural_noise",cpuNoise,gpuNoise);

    report<<"cpu_noise="<<cpuNoise[0]<<'\n'
          <<"gpu_noise="<<gpuNoise[0]<<'\n'
          <<"TOTAL_VALUES="<<totalValues<<'\n'
          <<"TOTAL_MISMATCH="<<totalMismatch<<'\n'
          <<"FAILED_COMPARISONS="<<failures<<'\n';

    report.flush();

    if(!report)
        throw std::runtime_error("CSF report write failed");

    std::cout<<"CSF_PROBE_FAILURES="<<failures<<'\n';

    return failures?1:0;
}
 std::ofstream(out/"device.txt")<<gpu->physicalDevice()->getProperties().deviceName.data()<<'\n';
 SingleMatrixBackend gpuMatrices{[&](auto rgb,auto coefficients,auto kind){auto values=project_rgb_single_gpu(*gpu,options.at("--projection-shader"),rgb,coefficients,kind);++matrixDispatches;return values;}};
 matrices=&gpuMatrices;backendName="hybrid-vulkan-matrices-cpu-fftw";
#ifdef IQM_HDRVDP3_COMPARISON
 backendName="experimental-vulkan-matrices-mask-cpu-fftw";
#ifdef IQM_HDRVDP3_MUTUAL
 backendName="experimental-vulkan-matrices-comparison-mutual-cpu-fftw";
 if(comparisonResize)backendName="experimental-vulkan-matrices-comparison-mutual-resize-cpu-fftw";
 if(adaptationInput)backendName=comparisonResize?"experimental-vulkan-matrices-comparison-mutual-resize-adaptation-input-cpu-fftw":"experimental-vulkan-matrices-comparison-mutual-adaptation-input-cpu-fftw";
 if(padding)backendName+="-gpu-padding";
#endif
#endif
#endif
 auto xyz=load_xyz_display_samples(data/"ciexyz31.csv",data/"emission_spectra_oled.csv");
 auto spectral=load_rgb_spectral_samples(data/"log_cone_smith_pokorny_1975.csv",data/"cie_scotopic_lum.txt",data/"emission_spectra_oled.csv");
  unsigned w=0,h=0;auto start=std::chrono::steady_clock::now();std::ofstream log(out/"progress.log");
  if(fullGpu){
#ifndef IQM_HDRVDP3_HYBRID
    throw std::invalid_argument("--full-gpu requires a Vulkan-enabled HDRVDP3 build");
#else
    if(!options.contains("--projection-shader"))throw std::invalid_argument("--full-gpu requires --projection-shader");
    if(!options.contains("--pq-display-shader"))throw std::invalid_argument("--full-gpu requires --pq-display-shader");
    if(!options.contains("--jnd-shader"))throw std::invalid_argument("--full-gpu requires --jnd-shader");
    if(!options.contains("--sp0-corrdn-shader"))throw std::invalid_argument("--full-gpu requires --sp0-corrdn-shader");
    if(!options.contains("--sp0-base-mean-shader"))throw std::invalid_argument("--full-gpu requires --sp0-base-mean-shader");
    // Stage 1: PQ decode + native projection on GPU. Downstream GPU mask/resize backends still use existing per-stage host interfaces; reject to avoid implying full device-resident pipeline.
    // keep comparison/mutual/resize backends; stage 1 only replaces PQ decode + projection input residency
#endif
  }
  unsigned gpuPqDispatches=0,gpuOpticalDispatches=0,gpuAdaptDispatches=0,gpuJndDispatches=0,gpuSp0Dispatches=0,gpuSp0HostReadbacks=0;
  int diagRun=0; const char* diagEnv=getenv("IQM_HDRVDP3_DIAG_RUN"); if(diagEnv) diagRun=std::atoi(diagEnv);
  auto dump=[&](const char* side,const char* stage,std::span<const float> v){
    std::ofstream d(out/(std::string("diag_")+side+"_"+stage+".f32"),std::ios::binary);
    d.write(reinterpret_cast<const char*>(v.data()),static_cast<std::streamsize>(v.size_bytes()));
    if(!d) throw std::runtime_error("Diagnostic export failed");
  };
  unsigned residentMutualDispatches=0, residentMutualInputUploads=0;
  unsigned residentMutualOutputReadbacks=0;
  bool residentMutual=false;
  const char* residualEnv=std::getenv("IQM_HDRVDP3_RESIDENT_RESIDUAL");
  const bool residentResidual=residualEnv && std::string(residualEnv)=="1";
  double residualFrequency=0;

  const char* csfResidentEnv=
      std::getenv("IQM_HDRVDP3_RESIDENT_CSF");

  const bool residentCsf=
      csfResidentEnv &&
      std::string(csfResidentEnv)=="1";

  const char* maskingEnv=
      std::getenv("IQM_HDRVDP3_RESIDENT_MASKING");

  const bool residentMasking=
      maskingEnv && std::string(maskingEnv)=="1";

  if(residentMasking && !residentCsf)
      throw std::runtime_error(
          "Resident masking requires resident CSF");

  unsigned residentMaskingDispatches=0;

  const char* comparisonResidentEnv=
      std::getenv("IQM_HDRVDP3_RESIDENT_COMPARISON");

  const bool residentComparison=
      comparisonResidentEnv &&
      std::string(comparisonResidentEnv)=="1";

  if(residentComparison && !residentMasking)
      throw std::runtime_error(
          "Resident comparison requires resident masking");

  unsigned residentCsfAdaptationBuffers=0;
  unsigned residentCsfFrequencyBuffers=0;
  unsigned residualExecutions=0;
#ifdef IQM_HDRVDP3_HYBRID
#ifdef IQM_HDRVDP3_MUTUAL
  const char* residentEnv=getenv("IQM_HDRVDP3_RESIDENT_MUTUAL");
  residentMutual=fullGpu && !cpuSp0 && residentEnv && std::string(residentEnv)=="1";
  if(residentMutual && !gpu->float64Enabled) throw std::runtime_error("Resident mutual requires shaderFloat64");
#endif
  std::optional<SP0PyramidGpuSingle> refPyramid,testPyramid;

  std::optional<GpuFloatBuffer>
      refAdaptationResident,
      testAdaptationResident;

  std::optional<GpuFloatBuffer>
      residualFrequencyResident;

  std::vector<GpuFloatBuffer> residentMutualBuffers;
  std::vector<GpuFloatBuffer> residentRawDifferences;

  std::optional<GpuFloatBuffer>
      refNativeResident,
      testNativeResident,
      comparisonMaskResident;

  std::vector<GpuFloatBuffer> comparisonBandMasks;

  std::vector<size_t> comparisonBandSlots;

  const char* poolingResidentEnv=
      std::getenv("IQM_HDRVDP3_RESIDENT_POOLING");

  const bool residentPooling=
      poolingResidentEnv &&
      std::string(poolingResidentEnv)=="1";

  if(residentPooling && !residentComparison)
      throw std::runtime_error(
          "Resident pooling requires resident comparison");

  std::vector<GpuFloatBuffer> residentBandErrors;

  const char* finalScoreResidentEnv=
      std::getenv("IQM_HDRVDP3_RESIDENT_FINAL_SCORE");

  const bool residentFinalScore=
      finalScoreResidentEnv &&
      std::string(finalScoreResidentEnv)=="1";

  if(residentFinalScore && !residentPooling)
      throw std::runtime_error(
          "Resident final score requires resident pooling");

  std::optional<GpuFloatBuffer> residentQualityScores;

  std::vector<GpuFloatBuffer> residentCsfSensitivity;
  std::optional<GpuFloatBuffer> residentCsfNoise;

  unsigned residentCsfDispatches=0;
  unsigned residentCsfPlanUploads=0;
      auto readback=[&](const GpuFloatBuffer& b){ auto staging=IQM::Bin::VulkanResource::createBuffer(*gpu->device(),*gpu->physicalDevice(),b.bytes,vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent); staging.first.bindMemory(staging.second,0); gpu->cmdBuf()->reset(); gpu->cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit}); vk::BufferMemoryBarrier rb{.srcAccessMask=vk::AccessFlagBits::eMemoryWrite,.dstAccessMask=vk::AccessFlagBits::eTransferRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=*b.buffer,.offset=0,.size=b.bytes}; gpu->cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,vk::PipelineStageFlagBits::eTransfer,{},{},{rb},{}); gpu->cmdBuf()->copyBuffer(*b.buffer,*staging.first,vk::ArrayProxy<const vk::BufferCopy>{vk::BufferCopy{0,0,b.bytes}}); vk::MemoryBarrier hr{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead}; gpu->cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},{hr},{},{}); gpu->cmdBuf()->end(); vk::CommandBuffer c=*gpu->cmdBuf(); vk::raii::Fence f(*gpu->device(),vk::FenceCreateInfo{}); gpu->queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&c},f); gpu->waitForFence(f); std::vector<float> out(b.count); void* p=staging.second.mapMemory(0,b.bytes); std::memcpy(out.data(),p,b.bytes); staging.second.unmapMemory(); gpu->cmdBuf()->reset(); return out; };
#endif
  if(residentResidual && !residentMutual)
      throw std::runtime_error("Resident residual requires resident mutual and GPU SP0");
  if(residentCsf && !residentResidual)
      throw std::runtime_error(
          "Resident CSF requires resident residual");

  auto prepare=[&](const std::string& file,const char* side){
    log<<side<<" start\n"<<std::flush;std::cout<<side<<' '<<(fullGpu?"full-gpu-optical":"cpu-prep")<<' '<<backendName<<" Single\n"<<std::flush;
    if(!stbi_is_16_bit(file.c_str()))throw std::invalid_argument("Explicit RGB16 PNG required");int iw,ih,channels;
    std::unique_ptr<stbi_us,decltype(&stbi_image_free)> raw(stbi_load_16(file.c_str(),&iw,&ih,&channels,3),stbi_image_free);
    if(!raw||iw<16||ih<16||channels!=3)throw std::invalid_argument("RGB16 image decode/shape failed");
    unsigned sw=unsigned(iw),sh=unsigned(ih);size_t sn=size_t(sw)*sh;if(!w){w=sw;h=sh;}size_t n=size_t(w)*h;
        const bool gpuNormalize=residentQualityNoFftw;
    const bool gpuInputResize=
        residentQualityNoFftw && (sw!=w || sh!=h);

    std::vector<float> encoded(3*sn);

    for(size_t c=0;c<3;++c)
        for(size_t i=0;i<sn;++i)
            encoded[c*sn+i]=gpuNormalize
                ? float(raw.get()[3*i+c])
                : normalize_pq_u16_single(raw.get()[3*i+c]);

    raw.reset();

    log<<side
       <<" pq_normalization_backend="
       <<(gpuNormalize?"gpu":"cpu")
       <<'\n'<<std::flush;
    if(!gpuInputResize && (sw!=w||sh!=h)){std::vector<float> resized(3*n);for(size_t c=0;c<3;++c){std::vector<float> plane(encoded.begin()+c*sn,encoded.begin()+(c+1)*sn);auto values=resize_bicubic(plane,int(sh),int(sw),int(h),int(w));for(size_t i=0;i<n;++i)resized[c*n+i]=std::clamp(values[i],0.f,1.f);}encoded=std::move(resized);}
    if(fullGpu){
#ifdef IQM_HDRVDP3_HYBRID

      // PQ encoded -> GPU PQ display (device-resident display-linear RGB)
      auto gpuLinear=[&]() -> GpuFloatBuffer {

    if(!gpuInputResize) {
        log<<side<<" input_resize_backend=none\n"
           <<std::flush;

        return pq_to_display_single_gpu(
            *gpu,
            options.at("--pq-display-shader"),
            encoded,
            {200.,.005,2000.},
            gpuNormalize);
    }

    const auto& d=*gpu->device();
    const auto& p=*gpu->physicalDevice();

    const auto runtime=fs::path(
        options.at("--projection-shader")
    ).parent_path();

    if(!gpu->float64Enabled)
        throw std::runtime_error(
            "GPU input resize requires Float64");

    const auto limits=p.getProperties().limits;

    auto allocateRGB=[&](size_t bytes,bool host=false) {

        if(!bytes ||
           bytes>limits.maxStorageBufferRange ||
           bytes>0xffffffffull)
            throw std::runtime_error(
                "RGB resize storage limit");

        auto flags=host
            ? vk::MemoryPropertyFlags(
                vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent)
            : vk::MemoryPropertyFlags(
                vk::MemoryPropertyFlagBits::eDeviceLocal);

        auto v=IQM::Bin::VulkanResource::createBuffer(
            d,p,unsigned(bytes),
            vk::BufferUsageFlagBits::eStorageBuffer |
            vk::BufferUsageFlagBits::eTransferSrc |
            vk::BufferUsageFlagBits::eTransferDst,
            flags);

        v.first.bindMemory(v.second,0);

        return GpuFloatBuffer{
            std::move(v.first),
            std::move(v.second),
            unsigned(bytes),
            unsigned(bytes/4u)
        };
    };

    auto uploadRGB=[&](
        GpuFloatBuffer& buffer,const auto& values) {

        if(values.size()*sizeof(values[0])!=buffer.bytes)
            throw std::runtime_error(
                "RGB resize upload shape mismatch");

        void* ptr=buffer.memory.mapMemory(
            0,buffer.bytes);

        std::memcpy(
            ptr,values.data(),buffer.bytes);

        buffer.memory.unmapMemory();
    };

    auto regionRGB=[](const GpuFloatBuffer& buffer) {
        return FFTBufferRegion{
            *buffer.buffer,0,buffer.bytes
        };
    };

    unsigned resizeDispatches=0;
    unsigned resizePlanUploads=0;

    auto dispatchRGB=[&](
        const GpuFloatBuffer& source,
        const GpuFloatBuffer& indices,
        const GpuFloatBuffer& weights,
        const GpuFloatBuffer& norms,
        const GpuFloatBuffer& destination,
        unsigned iw,unsigned ih,
        unsigned ow,unsigned oh,
        unsigned taps,unsigned mode,
        bool clampOutput) {

        const unsigned count=destination.count;
        const unsigned groups=(count+63u)/64u;

        const unsigned gx=std::min(
            groups,
            std::min(
                32768u,
                limits.maxComputeWorkGroupCount[0]));

        if(!gx)
            throw std::runtime_error(
                "Invalid RGB resize dispatch");

        const unsigned gy=(groups+gx-1u)/gx;

        if(gy>limits.maxComputeWorkGroupCount[1])
            throw std::runtime_error(
                "RGB resize dispatch limit");

        CSFDispatchSingle job(
            d,p,runtime,
            CSFKernelSingle::RGBResize,
            {
                regionRGB(source),
                regionRGB(indices),
                regionRGB(weights),
                regionRGB(norms),
                regionRGB(destination)
            },
            {
                count,iw,ih,ow,oh,taps,mode,
                clampOutput?1u:0u
            },
            gx,gy,gpu->float64Enabled);

        gpu->cmdBuf()->reset();

        gpu->cmdBuf()->begin(
            vk::CommandBufferBeginInfo{
                .flags=vk::CommandBufferUsageFlagBits::
                    eOneTimeSubmit});

        vk::MemoryBarrier before{
            .srcAccessMask=
                vk::AccessFlagBits::eHostWrite |
                vk::AccessFlagBits::eShaderWrite,
            .dstAccessMask=
                vk::AccessFlagBits::eShaderRead
        };

        gpu->cmdBuf()->pipelineBarrier(
            vk::PipelineStageFlagBits::eHost |
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eComputeShader,
            {},{before},{},{});

        job.record(*gpu->cmdBuf());

        vk::MemoryBarrier after{
            .srcAccessMask=
                vk::AccessFlagBits::eShaderWrite,
            .dstAccessMask=
                vk::AccessFlagBits::eShaderRead |
                vk::AccessFlagBits::eTransferRead
        };

        gpu->cmdBuf()->pipelineBarrier(
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eComputeShader |
            vk::PipelineStageFlagBits::eTransfer,
            {},{after},{},{});

        gpu->cmdBuf()->end();

        vk::CommandBuffer cmd=*gpu->cmdBuf();

        vk::raii::Fence fence(
            d,vk::FenceCreateInfo{});

        gpu->queue()->submit(
            vk::SubmitInfo{
                .commandBufferCount=1,
                .pCommandBuffers=&cmd},
            fence);

        gpu->waitForFence(fence);
        gpu->cmdBuf()->reset();

        ++resizeDispatches;
    };

    // Initial input upload and GPU normalization.

    auto input=allocateRGB(
        encoded.size()*sizeof(float),true);

    uploadRGB(input,encoded);

    auto dummyIndices=allocateRGB(4u,true);
    auto dummyWeights=allocateRGB(4u,true);
    auto dummyNorms=allocateRGB(4u,true);

    uploadRGB(dummyIndices,std::vector<int32_t>{0});
    uploadRGB(dummyWeights,std::vector<float>{0.f});
    uploadRGB(dummyNorms,std::vector<float>{1.f});

    auto normalized=allocateRGB(
        encoded.size()*sizeof(float));

    dispatchRGB(
        input,dummyIndices,dummyWeights,dummyNorms,normalized,
        sw,sh,sw,sh,0u,0u,false);

    auto resizeAxis=[&](
        const GpuFloatBuffer& source,
        unsigned iw,unsigned ih,
        unsigned ow,unsigned oh,
        bool horizontal) {

        auto plan=make_resize_axis_plan(
            int(horizontal?iw:ih),
            int(horizontal?ow:oh));

        std::vector<int32_t> indices(
            plan.indices.begin(),
            plan.indices.end());

        std::vector<float> weights(
            plan.weights.begin(),
            plan.weights.end());

        std::vector<float> norms(
            plan.normalization.begin(),
            plan.normalization.end());

        auto ib=allocateRGB(
            indices.size()*sizeof(int32_t),true);

        auto wb=allocateRGB(
            weights.size()*sizeof(float),true);

        auto nb=allocateRGB(
            norms.size()*sizeof(float),true);

        uploadRGB(ib,indices);
        uploadRGB(wb,weights);
        uploadRGB(nb,norms);

        resizePlanUploads+=3;

        auto destination=allocateRGB(
            size_t(ow)*oh*3u*sizeof(float));

        dispatchRGB(
            source,ib,wb,nb,destination,
            iw,ih,ow,oh,
            unsigned(plan.taps),
            horizontal?1u:2u,
            false);

        return destination;
    };

    // Preserve horizontal -> vertical order.

    auto horizontal=(sw==w)
        ? std::move(normalized)
        : resizeAxis(normalized,sw,sh,w,sh,true);

    auto vertical=(sh==h)
        ? std::move(horizontal)
        : resizeAxis(horizontal,w,sh,w,h,false);

    auto clamped=allocateRGB(
        size_t(w)*h*3u*sizeof(float));

    dispatchRGB(
        vertical,dummyIndices,dummyWeights,dummyNorms,clamped,
        w,h,w,h,0u,3u,true);

    // Optional validation only: compare GPU input resize
    // against the existing CPU implementation.

    if(envEnabled("IQM_HDRVDP3_INPUT_RESIZE_VERIFY")) {

        auto actual=readback(clamped);

        size_t compared=0;
        size_t mismatches=0;
        size_t nonfinite=0;

        double maxAbs=0.0;
        uint32_t maxUlp=0;

        for(size_t channel=0;channel<3;++channel) {

            std::vector<float> plane(sn);

            for(size_t i=0;i<sn;++i) {
                plane[i]=normalize_pq_u16_single(
                    uint16_t(encoded[channel*sn+i]));
            }

            auto expected=resize_bicubic(
                plane,int(sh),int(sw),int(h),int(w));

            for(size_t i=0;i<n;++i) {

                float a=actual[channel*n+i];
                float e=std::clamp(expected[i],0.f,1.f);

                ++compared;

                if(!std::isfinite(a) ||
                   !std::isfinite(e)) {
                    ++nonfinite;
                    continue;
                }

                uint32_t ab=std::bit_cast<uint32_t>(a);
                uint32_t eb=std::bit_cast<uint32_t>(e);

                if(ab!=eb)
                    ++mismatches;

                uint32_t ulp=ab>eb ? ab-eb : eb-ab;

                maxUlp=std::max(maxUlp,ulp);

                maxAbs=std::max(
                    maxAbs,
                    std::abs(double(a)-double(e)));
            }
        }

        std::ofstream report(
            out/(std::string(side)+
                 "_input_resize_parity.txt"));

        report<<std::setprecision(17)
              <<"source_width="<<sw<<'\n'
              <<"source_height="<<sh<<'\n'
              <<"target_width="<<w<<'\n'
              <<"target_height="<<h<<'\n'
              <<"compared="<<compared<<'\n'
              <<"bitwise_mismatches="<<mismatches<<'\n'
              <<"nonfinite="<<nonfinite<<'\n'
              <<"max_abs_error="<<maxAbs<<'\n'
              <<"max_ulp="<<maxUlp<<'\n'
              <<"diagnostic_host_readbacks=1\n"
              <<"status="
              <<((mismatches==0 && nonfinite==0)
                    ?"PASS":"FAIL")
              <<'\n';

        report.close();

        log<<side<<" input_resize_diagnostic_readbacks=1\n"
           <<std::flush;
    }

    // Production path: resident resize output to PQ.

    auto display=pq_to_display_single_gpu_resident(
        *gpu,
        options.at("--pq-display-shader"),
        clamped,
        {200.,.005,2000.});

    log<<side<<" input_resize_backend=gpu\n"
       <<side<<" input_resize_source_width="<<sw<<'\n'
       <<side<<" input_resize_source_height="<<sh<<'\n'
       <<side<<" input_resize_target_width="<<w<<'\n'
       <<side<<" input_resize_target_height="<<h<<'\n'
       <<side<<" input_resize_dispatches="
       <<resizeDispatches<<'\n'
       <<side<<" input_resize_plan_uploads="
       <<resizePlanUploads<<'\n'
       <<side<<" input_resize_host_readbacks="<<(envEnabled("IQM_HDRVDP3_INPUT_RESIZE_VERIFY")?1:0)<<'\n'
       <<side<<" input_resize_to_pq_reuploads=0\n"
       <<std::flush;

    return display;

}();

++gpuPqDispatches;
      if(diagRun) dump(side,"pq",readback(gpuLinear));
      encoded.clear();encoded.shrink_to_fit();
      // Display-linear RGB -> GPU NativeColor projection (device-resident native RGB)
      const auto color=linear_color_transform(xyz,LinearColorEncoding::BT2020);
      std::vector<float> nativeCoeffs;for(unsigned c=0;c<3;++c)for(unsigned p=0;p<3;++p)nativeCoeffs.push_back(float(color[c][p]));
      auto gpuNative=project_rgb_single_gpu_device(*gpu,options.at("--projection-shader"),*gpuLinear.buffer,n,nativeCoeffs,SingleProjectionKind::NativeColor);++matrixDispatches;
      if(diagRun) dump(side,"native",readback(gpuNative));
      // Native RGB -> GPU Optical MTF (device-resident optical RGB)
      const bool residentOptical = w == 3840 && h == 2160 && ppd == 62.729130063099447;
      if (w == 3840 && h == 2160 && !residentOptical)
          throw std::invalid_argument("Resident 4K optical requires PPD 62.729130063099447");
      const auto opticalRuntime = fs::absolute(fs::path(options.at("--projection-shader"))).parent_path();
      log << side << " optical_backend=" << (residentOptical ? "resident-4k" : "legacy") << '\n' << std::flush;
      auto gpuOptical = residentOptical
          ? filter_optical_rgb_resident_gpu(*gpu, *gpuNative.buffer, w, h, ppd, opticalRuntime)
          : filter_optical_rgb_single_gpu(*gpu, *gpuNative.buffer, w, h, float(ppd));
      ++gpuOpticalDispatches; // wrapper calls, not individual Vulkan dispatches
      log << side << " optical_complete\n" << std::flush;
      if(diagRun) dump(side,"optical",readback(gpuOptical));
      // Optical RGB -> GPU LMSR projection (device-resident)
      const auto lmsrModel=calibrate_rgb_spectral_age(spectral,24.); std::vector<float> lmsrCoeffs; for(unsigned c=0;c<4;++c)for(unsigned p=0;p<3;++p)lmsrCoeffs.push_back(float(lmsrModel.matrix[p][c]));
      auto gpuLmsr=project_rgb_single_gpu_device(*gpu,options.at("--projection-shader"),*gpuOptical.buffer,n,lmsrCoeffs,SingleProjectionKind::LMSR);++matrixDispatches;
            // Device-resident L+M, local adaptation, JND; legacy readback for SP0/quality.

      auto gpuAdaptIn=adaptation_input_single_gpu_device(*gpu,options.at("--adaptation-input-shader"),*gpuLmsr.buffer,unsigned(n)); ++adaptationInputDispatches;
      if(diagRun) dump(side,"lm",readback(gpuAdaptIn));
      log << side << " adaptation_backend=" << (residentOptical ? "resident-4k" : "legacy") << '\n' << std::flush;
      auto gpuAdapt = residentOptical
          ? local_adaptation_resident_gpu(*gpu,*gpuAdaptIn.buffer,w,h,ppd,opticalRuntime)
          : local_adaptation_single_gpu(*gpu,*gpuAdaptIn.buffer,w,h,float(ppd));
      ++gpuAdaptDispatches; // wrapper calls
      log << side << " adaptation_complete\n" << std::flush;
      QualitySideSingle result;
      if(!residentFinalScore || diagRun) result.native_rgb=readback(gpuNative);

      if(residentComparison){

          auto& slot=std::string(side)=="reference"
              ? refNativeResident
              : testNativeResident;

          if(slot)
              throw std::runtime_error(
                  "Duplicate resident native RGB");

          slot.emplace(std::move(gpuNative));
      }
      const size_t nn=size_t(w)*h;

      std::vector<float> lmsrHost;

      if(!residentFinalScore || diagRun){
          result.adaptation=readback(gpuAdapt);
          lmsrHost=readback(gpuLmsr);

          if(lmsrHost.size()!=4*nn)
              throw std::runtime_error(
                  "LMSR matrix provider shape mismatch");
      }
      if(diagRun){ dump(side,"lmsr",lmsrHost); dump(side,"adapt",result.adaptation); std::ofstream ds(out/(std::string("diag_")+side+"_jnd_params_run"+std::to_string(diagRun)+".txt")); ds<<std::setprecision(17)<<"w="<<w<<" h="<<h<<" nn="<<nn<<" ppd="<<ppd<<" area="<<(double(nn)/(ppd*ppd))<<" age=24\n"; }
      auto gpuJndBuf=rgb_to_jnd_response_single_gpu_device(*gpu,options.at("--jnd-shader"),*gpuLmsr.buffer,*gpuAdapt.buffer,w,h,double(nn)/(ppd*ppd),24.); ++gpuJndDispatches;
      const bool gpuSp0Enabled = !cpuSp0;
      if(gpuSp0Enabled){
        auto gpuSp0=build_sp0_pyramid_single_gpu(*gpu,options.at("--sp0-corrdn-shader"),options.at("--sp0-base-mean-shader"),*gpuJndBuf.buffer,w,h,ppd,!residentMutual);
        gpuSp0Dispatches += gpuSp0.dispatches;
        gpuSp0HostReadbacks += gpuSp0.hostReadbacks;
        if(residentMutual){
          if(diagRun) dump(side,"jnd",readback(gpuJndBuf));
          auto& slot=std::string(side)=="reference"?refPyramid:testPyramid;
          slot.emplace(std::move(gpuSp0));
        }else{
        result.pyramid=std::move(gpuSp0.host);
        if(diagRun){ auto jndHost=readback(gpuJndBuf); dump(side,"jnd",jndHost); dump(side,"sp0_base",std::span<const float>(&result.pyramid.base_mean,1)); for(size_t bi=0;bi<result.pyramid.bands.size();++bi){ dump(side,(std::string("sp0_b")+std::to_string(bi)).c_str(), result.pyramid.bands[bi].values); } std::ofstream sd(out/(std::string("diag_")+side+"_sp0_gpu_stats_run"+std::to_string(diagRun)+".txt")); sd<<"dispatches="<<gpuSp0.dispatches<<" host_readbacks="<<gpuSp0.hostReadbacks<<"\n"; }
        }
      } else {
        auto jndHost=readback(gpuJndBuf);
        if(diagRun){ dump(side,"jnd",jndHost); }
        result.pyramid=build_sp0_pyramid_single(jndHost,w,h,ppd);
        if(diagRun){ dump(side,"sp0_base",std::span<const float>(&result.pyramid.base_mean,1)); for(size_t bi=0;bi<result.pyramid.bands.size();++bi){ dump(side,(std::string("sp0_b")+std::to_string(bi)).c_str(), result.pyramid.bands[bi].values); } }
      }
      if(residentCsf){
          auto& adaptSlot=
              std::string(side)=="reference"
              ? refAdaptationResident
              : testAdaptationResident;

          if(adaptSlot)
              throw std::runtime_error(
                  "Duplicate resident adaptation");

          adaptSlot.emplace(std::move(gpuAdapt));

          ++residentCsfAdaptationBuffers;
      }

      log<<side<<" ready\n"<<std::flush;return result;
#else
      throw std::invalid_argument("--full-gpu requires a Vulkan-enabled HDRVDP3 build");
#endif
    }
    auto linear=pq_to_display_single(encoded,{200.,.005,2000.});encoded.clear();encoded.shrink_to_fit();
    if(diagRun) dump(side,"pq",linear);
    auto result=diagRun ? prepare_quality_side_single_diagnostic(linear,w,h,ppd,xyz,spectral,backend,matrices,adaptationInput,padding,[&](const char* stage,std::span<const float> v){dump(side,stage,v);}) : prepare_quality_side_single(linear,w,h,ppd,xyz,spectral,backend,matrices,adaptationInput,padding);
    if(diagRun){dump(side,"sp0_base",std::span<const float>(&result.pyramid.base_mean,1));for(size_t bi=0;bi<result.pyramid.bands.size();++bi)dump(side,(std::string("sp0_b")+std::to_string(bi)).c_str(),result.pyramid.bands[bi].values);}log<<side<<" ready\n"<<std::flush;return result;
  };
 auto reference=prepare(options.at("--ref"),"reference"),test=prepare(options.at("--input"),"test");
#if defined(IQM_HDRVDP3_HYBRID) && defined(IQM_HDRVDP3_MUTUAL)
 std::vector<std::vector<float>> residentMasks;
 size_t nextMask=0;
 SingleMutualBackend residentProvider;
 if(residentMutual){
   if(!refPyramid || !testPyramid) throw std::runtime_error("Missing resident pyramids");
   auto& r=*refPyramid; auto& t=*testPyramid;
   if(r.bands.size()!=t.bands.size()) throw std::runtime_error("Resident pyramid count mismatch");
   unsigned bw=w,bh=h;
   for(size_t b=0;b<r.bands.size();++b){
     auto& a=r.bands[b]; auto& z=t.bands[b];
     if(a.count!=size_t(bw)*bh || z.count!=a.count) throw std::runtime_error("Resident band shape mismatch");
     auto mem=IQM::Bin::VulkanResource::createBuffer(*gpu->device(),*gpu->physicalDevice(),a.bytes,
       vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc,
       vk::MemoryPropertyFlagBits::eDeviceLocal);
     mem.first.bindMemory(mem.second,0);
     GpuFloatBuffer mask{std::move(mem.first),std::move(mem.second),a.bytes,a.count};
     ResidentMutualMaskSingle job(*gpu->device(),*gpu->physicalDevice(),options.at("--mutual-shader"),
       {*a.buffer,0,a.bytes},{*z.buffer,0,z.bytes},{*mask.buffer,0,mask.bytes},bw,bh);
     gpu->cmdBuf()->reset();
     gpu->cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
     vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
     gpu->cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader,{},{before},{},{});
     job.record(*gpu->cmdBuf());
     gpu->cmdBuf()->end();
     vk::CommandBuffer c=*gpu->cmdBuf();
     vk::raii::Fence fence(*gpu->device(),vk::FenceCreateInfo{});
     gpu->queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&c},fence);
     gpu->waitForFence(fence);
     ++residentMutualDispatches; ++mutualDispatches;
     if(!residentMasking || diagRun){
         residentMasks.push_back(readback(mask));
         ++residentMutualOutputReadbacks;

         if(diagRun)
             dump("mutual",
                 (std::string("b")+std::to_string(b)).c_str(),
                 residentMasks.back());
     }

     if(residentMasking)
         residentMutualBuffers.emplace_back(std::move(mask));
     if(b>=1){bw=(bw+1)/2;bh=(bh+1)/2;}
   }
   log<<"resident_mutual_complete\\n"<<std::flush;
   const char* hostOracleEnv=
       std::getenv("IQM_HDRVDP3_RESIDUAL_COMPARE");

   const bool retainHostPyramid=
       !residentFinalScore || diagRun ||
       (hostOracleEnv &&
        std::string(hostOracleEnv)=="1");

   auto materialize=[&](SP0PyramidGpuSingle& p,QualitySideSingle& side,const char* name){
     unsigned bw=w,bh=h;
     for(size_t b=0;b<p.bands.size();++b){
       std::vector<float> values;

       if(retainHostPyramid){
           values=readback(p.bands[b]);
           ++gpuSp0HostReadbacks;
       }else{
           if(p.bands[b].bytes!=
              size_t(bw)*bh*sizeof(float))
               throw std::runtime_error(
                   "Resident SP0 geometry mismatch");
       }
       if(diagRun) dump(name,(std::string("sp0_b")+std::to_string(b)).c_str(),values);
       side.pyramid.bands.emplace_back();
       auto& band=side.pyramid.bands.back(); band.width=bw; band.height=bh; band.values=std::move(values);
       if(b>=1){bw=(bw+1)/2;bh=(bh+1)/2;}
     }
     if(retainHostPyramid){
         auto bm=readback(p.baseMean);
         ++gpuSp0HostReadbacks;
         side.pyramid.base_mean=bm.at(0);
         if(diagRun) dump(name,"sp0_base",bm);
     }
   };
   materialize(r,reference,"reference"); materialize(t,test,"test");
   if(residentResidual){
      const auto& rb=reference.pyramid.bands.back();
      const auto& tb=test.pyramid.bands.back();
      const auto& a=r.bands.back();
      const auto& z=t.bands.back();

      if(rb.width!=240 || rb.height!=135 ||
         tb.width!=rb.width || tb.height!=rb.height ||
         r.bands.size()!=6 || t.bands.size()!=6 ||
         a.bytes!=240u*135u*4u || z.bytes!=a.bytes)
          throw std::runtime_error("Unsupported resident residual geometry");

      const auto& d=*gpu->device();
      const auto& p=*gpu->physicalDevice();

      auto alloc=[&](unsigned bytes,vk::MemoryPropertyFlags flags){
          auto v=IQM::Bin::VulkanResource::createBuffer(
              d,p,bytes,
              vk::BufferUsageFlagBits::eStorageBuffer |
              vk::BufferUsageFlagBits::eTransferSrc |
              vk::BufferUsageFlagBits::eTransferDst,
              flags);
          v.first.bindMemory(v.second,0);
          return v;
      };

      auto spectrum=alloc(240u*135u*8u,
          vk::MemoryPropertyFlagBits::eDeviceLocal);

      auto selection=alloc(12,
          vk::MemoryPropertyFlagBits::eHostVisible |
          vk::MemoryPropertyFlagBits::eHostCoherent);

      auto frequency=alloc(8,
          vk::MemoryPropertyFlagBits::eHostVisible |
          vk::MemoryPropertyFlagBits::eHostCoherent);

      auto runtime=fs::path(
          options.at("--projection-shader")).parent_path();

      ResidentResidualSingle job(
          d,p,*(*gpu->queue()),*(*gpu->cmdPool()),
          {{*a.buffer,0,a.bytes},
           {*z.buffer,0,z.bytes},
           {*spectrum.first,0,240u*135u*8u},
           {*selection.first,0,12},
           {*frequency.first,0,8}},
          runtime,rb.width,rb.height,
          unsigned(r.bands.size()),ppd,gpu->float64Enabled);

      gpu->cmdBuf()->reset();

      gpu->cmdBuf()->begin(vk::CommandBufferBeginInfo{
          .flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

      vk::MemoryBarrier before{
          .srcAccessMask=vk::AccessFlagBits::eShaderWrite,
          .dstAccessMask=vk::AccessFlagBits::eShaderRead};

      gpu->cmdBuf()->pipelineBarrier(
          vk::PipelineStageFlagBits::eComputeShader,
          vk::PipelineStageFlagBits::eComputeShader,
          {},{before},{},{});

      job.record(*gpu->cmdBuf());

      vk::MemoryBarrier hr{
          .srcAccessMask=vk::AccessFlagBits::eShaderWrite,
          .dstAccessMask=vk::AccessFlagBits::eHostRead};

      gpu->cmdBuf()->pipelineBarrier(
          vk::PipelineStageFlagBits::eComputeShader,
          vk::PipelineStageFlagBits::eHost,
          {},{hr},{},{});

      gpu->cmdBuf()->end();

      vk::CommandBuffer cmd=*gpu->cmdBuf();
      vk::raii::Fence fence(d,vk::FenceCreateInfo{});

      gpu->queue()->submit(vk::SubmitInfo{
          .commandBufferCount=1,
          .pCommandBuffers=&cmd},fence);

      gpu->waitForFence(fence);

      std::array<uint32_t,3> selected{};

      void* ptr=selection.second.mapMemory(0,12);
      std::memcpy(selected.data(),ptr,12);
      selection.second.unmapMemory();

      ptr=frequency.second.mapMemory(0,8);
      std::memcpy(&residualFrequency,ptr,8);
      frequency.second.unmapMemory();

      gpu->cmdBuf()->reset();

      if(selected[1]!=0 ||
         selected[0]>=240u*135u ||
         !std::isfinite(residualFrequency) ||
         residualFrequency<0)
          throw std::runtime_error("Invalid GPU residual output");

      ++residualExecutions;

      const char* compareEnv=
          std::getenv("IQM_HDRVDP3_RESIDUAL_COMPARE");

      if(compareEnv && std::string(compareEnv)=="1"){
          const unsigned rw=rb.width,rh=rb.height;
          const size_t n=size_t(rw)*rh;

          std::vector<float> col(n);

          for(unsigned x=0;x<rw;++x)
              for(unsigned y=0;y<rh;++y)
                  col[size_t(x)*rh+y]=
                      tb.values[size_t(y)*rw+x]-
                      rb.values[size_t(y)*rw+x];

          auto oracle=backend.forward(col,rw,rh);

          if(oracle.size()!=2*n)
              throw std::runtime_error("Residual oracle shape");

          oracle[0]=oracle[1]=0;

          size_t expected=0;
          float maximum=0;

          for(size_t i=0;i<n;++i){
              float mag=std::hypot(
                  oracle[2*i],oracle[2*i+1]);

              if(mag>maximum){
                  maximum=mag;
                  expected=i;
              }
          }

          const double base=
              std::ldexp(1.,-int(r.bands.size()-1))*ppd/2;

          auto axis=[&](unsigned k,unsigned size){
              return ((std::fmod(
                  .5+double(k)/size,1.)-.5)*(base*2))*2;
          };

          double fx=axis(unsigned(expected/rh),rw);
          double fy=axis(unsigned(expected%rh),rh);
          double expectedFrequency=std::sqrt(fx*fx+fy*fy);

          bool indexMatch=expected==selected[0];

          bool frequencyMatch=
              std::bit_cast<uint64_t>(expectedFrequency)==
              std::bit_cast<uint64_t>(residualFrequency);

          std::ofstream parity(out/"residual_parity.txt");

          parity<<std::setprecision(17)
              <<"cpu_index="<<expected<<'\n'
              <<"gpu_index="<<selected[0]<<'\n'
              <<"cpu_frequency="<<expectedFrequency<<'\n'
              <<"gpu_frequency="<<residualFrequency<<'\n'
              <<"index_match="<<indexMatch<<'\n'
              <<"frequency_bitwise_match="<<frequencyMatch<<'\n'
              <<"validation_cpu_fft_executions=1\n"
              <<"PASS="<<(indexMatch&&frequencyMatch)<<'\n';

          parity.flush();

          if(!parity)
              throw std::runtime_error("Residual parity report failed");

          if(!indexMatch || !frequencyMatch)
              throw std::runtime_error("Residual parity mismatch");
      }

      std::ofstream report(out/"resident_residual.txt");

      report<<std::setprecision(17)
          <<"gpu_index="<<selected[0]<<'\n'
          <<"gpu_frequency="<<residualFrequency<<'\n'
          <<"gpu_fft_executions="<<residualExecutions<<'\n'
          <<"residual_input_uploads=0\n"
          <<"frequency_readback_bytes=8\n"
          <<"selection_readback_bytes=12\n";

      if(!report)
          throw std::runtime_error("Residual report failed");

       if(residentCsf){

           residualFrequencyResident.emplace(
               GpuFloatBuffer{
                   std::move(frequency.first),
                   std::move(frequency.second),
                   8u,2u
               });

           ++residentCsfFrequencyBuffers;
       }
    }

    if(residentCsf){

       const size_t expected=size_t(w)*h;

       if(!refAdaptationResident ||
          !testAdaptationResident ||
          !residualFrequencyResident ||
          residentCsfAdaptationBuffers!=2 ||
          residentCsfFrequencyBuffers!=1)
           throw std::runtime_error(
               "Missing resident CSF input buffers");

       if(refAdaptationResident->count!=expected ||
          testAdaptationResident->count!=expected ||
          refAdaptationResident->bytes!=expected*4 ||
          testAdaptationResident->bytes!=expected*4 ||
          residualFrequencyResident->bytes!=8)
           throw std::runtime_error(
               "Resident CSF input shape mismatch");

              const auto& d=*gpu->device();
       const auto& p=*gpu->physicalDevice();

       const auto runtime=fs::path(
           options.at("--projection-shader")).parent_path();

       auto allocateCsf=[&](unsigned bytes,bool host=false){
           auto flags=host
               ? vk::MemoryPropertyFlags(
                   vk::MemoryPropertyFlagBits::eHostVisible |
                   vk::MemoryPropertyFlagBits::eHostCoherent)
               : vk::MemoryPropertyFlags(
                   vk::MemoryPropertyFlagBits::eDeviceLocal);

           auto v=IQM::Bin::VulkanResource::createBuffer(
               d,p,bytes,
               vk::BufferUsageFlagBits::eStorageBuffer |
               vk::BufferUsageFlagBits::eTransferSrc |
               vk::BufferUsageFlagBits::eTransferDst,
               flags);

           v.first.bindMemory(v.second,0);

           return GpuFloatBuffer{
               std::move(v.first),
               std::move(v.second),
               bytes,bytes/4u
           };
       };

       auto csfRegion=[](const GpuFloatBuffer& b){
           return FFTBufferRegion{
               *b.buffer,0,b.bytes
           };
       };

       auto uploadPlan=[&](
           GpuFloatBuffer& b,const auto& values){

           if(values.size()*sizeof(values[0])!=b.bytes)
               throw std::runtime_error(
                   "CSF plan upload shape mismatch");

           void* ptr=b.memory.mapMemory(0,b.bytes);

           std::memcpy(ptr,values.data(),b.bytes);

           b.memory.unmapMemory();

           ++residentCsfPlanUploads;
       };

       auto dispatchCsf=[&](
           CSFKernelSingle kind,
           std::vector<FFTBufferRegion> regions,
           std::vector<uint32_t> constants,
           unsigned gx,unsigned gy){

           CSFDispatchSingle job(
               d,p,runtime,kind,
               regions,constants,gx,gy,
               gpu->float64Enabled);

           gpu->cmdBuf()->reset();

           gpu->cmdBuf()->begin(
               vk::CommandBufferBeginInfo{
                   .flags=vk::CommandBufferUsageFlagBits::
                       eOneTimeSubmit});

           job.record(*gpu->cmdBuf());

           gpu->cmdBuf()->end();

           vk::CommandBuffer cmd=*gpu->cmdBuf();

           vk::raii::Fence fence(
               d,vk::FenceCreateInfo{});

           gpu->queue()->submit(
               vk::SubmitInfo{
                   .commandBufferCount=1,
                   .pCommandBuffers=&cmd},
               fence);

           gpu->waitForFence(fence);

           gpu->cmdBuf()->reset();

           ++residentCsfDispatches;
       };

       auto dispatchLinear=[&](
           CSFKernelSingle kind,
           std::vector<FFTBufferRegion> regions,
           unsigned count){

           const unsigned groups=(count+63u)/64u;

           const unsigned gx=std::min(
               groups,
               std::min(
                   32768u,
                   p.getProperties().limits.
                       maxComputeWorkGroupCount[0]));

           if(!gx)
               throw std::runtime_error(
                   "Invalid CSF dispatch size");

           const unsigned gy=(groups+gx-1u)/gx;

           dispatchCsf(
               kind,std::move(regions),
               {count},gx,gy);
       };

       auto resizeAxisCsf=[&](
           const GpuFloatBuffer& src,
           GpuFloatBuffer& dst,
           unsigned iw,unsigned ih,
           unsigned ow,unsigned oh,
           bool horizontal){

           const unsigned n=horizontal?iw:ih;
           const unsigned m=horizontal?ow:oh;

           auto plan=make_resize_axis_plan(
               int(n),int(m));

           std::vector<int32_t> indices(
               plan.indices.begin(),
               plan.indices.end());

           std::vector<float> weights(
               plan.weights.begin(),
               plan.weights.end());

           std::vector<float> normalization(
               plan.normalization.begin(),
               plan.normalization.end());

           auto ib=allocateCsf(
               unsigned(indices.size()*4),true);

           auto wb=allocateCsf(
               unsigned(weights.size()*4),true);

           auto nb=allocateCsf(
               unsigned(normalization.size()*4),true);

           uploadPlan(ib,indices);
           uploadPlan(wb,weights);
           uploadPlan(nb,normalization);

           dispatchCsf(
               CSFKernelSingle::Resize,
               {
                   csfRegion(src),
                   csfRegion(ib),
                   csfRegion(wb),
                   csfRegion(nb),
                   csfRegion(dst)
               },
               {
                   iw,ih,ow,oh,
                   uint32_t(plan.taps),
                   horizontal?1u:0u
               },
               (ow+15u)/16u,
               (oh+15u)/16u);
       };

       const unsigned pixels=w*h;

       auto fullLog=allocateCsf(pixels*4u);

       dispatchLinear(
           CSFKernelSingle::Log,
           {
               csfRegion(*refAdaptationResident),
               csfRegion(*testAdaptationResident),
               csfRegion(fullLog)
           },
           pixels);

       log<<"resident_csf_log_complete\n"<<std::flush;

       residentCsfNoise.emplace(allocateCsf(8));

       residentCsfSensitivity.reserve(
           reference.pyramid.bands.size());

       for(size_t bi=0;
           bi<reference.pyramid.bands.size();++bi){

           const auto& band=
               reference.pyramid.bands[bi];

           const unsigned bw=band.width;
           const unsigned bh=band.height;
           const unsigned bn=bw*bh;

           std::optional<GpuFloatBuffer> resized;

           const GpuFloatBuffer* logInput=&fullLog;

           if(bw!=w || bh!=h){

               auto temp=allocateCsf(bw*h*4u);

               resized.emplace(
                   allocateCsf(bn*4u));

               resizeAxisCsf(
                   fullLog,temp,
                   w,h,bw,h,true);

               resizeAxisCsf(
                   temp,*resized,
                   bw,h,bw,bh,false);

               logInput=&*resized;
           }

           auto grid=allocateCsf(256u*4u);
           auto sensitivity=allocateCsf(256u*4u);
           auto slopes=allocateCsf(255u*4u);

           const double rho=
               std::ldexp(1.,-int(bi))*ppd/2;

           const uint64_t bits=
               std::bit_cast<uint64_t>(rho);

           std::vector<FFTBufferRegion> generateRegions{
               csfRegion(*logInput),
               csfRegion(*residualFrequencyResident),
               csfRegion(grid),
               csfRegion(sensitivity),
               csfRegion(slopes),
               csfRegion(*residentCsfNoise)
           };

           dispatchCsf(
               CSFKernelSingle::Generate,
               generateRegions,
               {
                   uint32_t(bits),
                   uint32_t(bits>>32),
                   bw,bh,0u
               },
               4,1);

           residentCsfSensitivity.emplace_back(
               allocateCsf(bn*4u));

           dispatchLinear(
               CSFKernelSingle::Sample,
               {
                   csfRegion(*logInput),
                   csfRegion(grid),
                   csfRegion(sensitivity),
                   csfRegion(slopes),
                   csfRegion(
                       residentCsfSensitivity.back())
               },
               bn);

           if(bi+1==reference.pyramid.bands.size()){

               dispatchCsf(
                   CSFKernelSingle::Generate,
                   generateRegions,
                   {0u,0u,bw,bh,1u},
                   1,1);
           }

           log<<"resident_csf_band="
              <<bi<<" complete\n"<<std::flush;
       }

       std::ofstream csfReport(
           out/"resident_csf_compute.txt");

       csfReport
           <<"csf_dispatches="
           <<residentCsfDispatches<<'\n'
           <<"sensitivity_buffers="
           <<residentCsfSensitivity.size()<<'\n'
           <<"base_noise_buffers=1\n"
           <<"adaptation_reuploads=0\n"
           <<"frequency_reuploads=0\n"
           <<"resize_plan_uploads="
           <<residentCsfPlanUploads<<'\n'
           <<"csf_output_readbacks=0\n"
           <<"quality_consumer_connected=0\n";

       if(!csfReport)
           throw std::runtime_error(
               "Resident CSF report failed");

       if(residentMasking){

           const size_t bandCount=r.bands.size();

           if(residentMutualBuffers.size()!=bandCount ||
              residentCsfSensitivity.size()!=bandCount ||
              !residentCsfNoise)
               throw std::runtime_error(
                   "Resident masking input count mismatch");

           const unsigned dispatchStart=residentCsfDispatches;
           const unsigned uploadStart=residentCsfPlanUploads;

           auto dispatchMaskLinear=[&](
               CSFKernelSingle kind,
               std::vector<FFTBufferRegion> regions,
               std::vector<uint32_t> constants,
               unsigned count){

               const unsigned groups=(count+63u)/64u;

               const unsigned gx=std::min(
                   groups,std::min(
                       32768u,p.getProperties().limits.
                           maxComputeWorkGroupCount[0]));

               if(!gx)
                   throw std::runtime_error(
                       "Invalid masking dispatch geometry");

               const unsigned gy=(groups+gx-1u)/gx;

               dispatchCsf(
                   kind,std::move(regions),
                   std::move(constants),gx,gy);
           };

           const double maskingP=std::pow(10.,.3424);
           const float maskingQ=float(
               std::pow(10.,.108934275615));

           const float maskingKs=float(
               std::pow(10.,1.41846291721));

           const float maskingKn=float(
               std::pow(10.,.136877512403));

           const uint64_t pBits=
               std::bit_cast<uint64_t>(maskingP);

           residentRawDifferences.reserve(bandCount);

           size_t totalRawValues=0;

           for(size_t b=0;b<bandCount;++b){

               const auto& geometry=
                   reference.pyramid.bands[b];

               const unsigned mw=geometry.width;
               const unsigned mh=geometry.height;
               const unsigned mn=mw*mh;

               const bool hasPrevious=b>0;
               const bool hasNext=b+2<bandCount;

               std::optional<GpuFloatBuffer> previous;
               std::optional<GpuFloatBuffer> next;

               auto dummyPrevious=allocateCsf(4u);
               auto dummyNext=allocateCsf(4u);

               auto resizeNeighbor=[&](
                   size_t k,
                   std::optional<GpuFloatBuffer>& dst)
                   ->const GpuFloatBuffer& {

                   const auto& source=
                       residentMutualBuffers[k];

                   const auto& sg=
                       reference.pyramid.bands[k];

                   if(sg.width==mw && sg.height==mh)
                       return source;

                   auto temp=allocateCsf(
                       mw*sg.height*4u);

                   dst.emplace(allocateCsf(mn*4u));

                   resizeAxisCsf(
                       source,temp,
                       sg.width,sg.height,
                       mw,sg.height,true);

                   resizeAxisCsf(
                       temp,*dst,
                       mw,sg.height,
                       mw,mh,false);

                   return *dst;
               };

               const auto& prevInput=hasPrevious
                   ? resizeNeighbor(b-1,previous)
                   : dummyPrevious;

               const auto& nextInput=hasNext
                   ? resizeNeighbor(b+1,next)
                   : dummyNext;

               auto neighbor=allocateCsf(mn*4u);

               dispatchMaskLinear(
                   CSFKernelSingle::Neighbor,
                   {
                       csfRegion(prevInput),
                       csfRegion(nextInput),
                       csfRegion(neighbor)
                   },
                   {
                       mn,
                       hasPrevious?1u:0u,
                       hasNext?1u:0u
                   },
                   mn);

               residentRawDifferences.emplace_back(
                   allocateCsf(mn*4u));

               dispatchMaskLinear(
                   CSFKernelSingle::MaskedDifference,
                   {
                       csfRegion(r.bands[b]),
                       csfRegion(t.bands[b]),
                       csfRegion(residentMutualBuffers[b]),
                       csfRegion(neighbor),
                       csfRegion(residentCsfSensitivity[b]),
                       csfRegion(*residentCsfNoise),
                       csfRegion(residentRawDifferences.back())
                   },
                   {
                       uint32_t(pBits),
                       uint32_t(pBits>>32),
                       mn,
                       b+1==bandCount?1u:0u,
                       std::bit_cast<uint32_t>(maskingQ),
                       std::bit_cast<uint32_t>(maskingKs),
                       std::bit_cast<uint32_t>(maskingKn)
                   },
                   mn);

               totalRawValues+=mn;

               log<<"resident_masking_band="
                  <<b<<" complete\n"<<std::flush;
           }

           residentMaskingDispatches=
               residentCsfDispatches-dispatchStart;

           std::ofstream maskingReport(
               out/"resident_masking_compute.txt");

           maskingReport
               <<"mutual_buffers_retained="
               <<residentMutualBuffers.size()<<'\n'
               <<"raw_difference_buffers="
               <<residentRawDifferences.size()<<'\n'
               <<"raw_difference_values="
               <<totalRawValues<<'\n'
               <<"masking_dispatches="
               <<residentMaskingDispatches<<'\n'
               <<"resize_plan_uploads="
               <<residentCsfPlanUploads-uploadStart<<'\n'
               <<"sp0_input_reuploads=0\n"
               <<"mutual_input_reuploads=0\n"
               <<"csf_input_reuploads=0\n"
               <<"raw_difference_readbacks=0\n"
               <<"quality_consumer_connected=0\n"
               <<"full_gpu=0\n";

           if(!maskingReport)
               throw std::runtime_error(
                   "Resident masking report failed");
       }

       if(residentComparison){

           const unsigned pixels=w*h;

           if(!refNativeResident ||
              !testNativeResident ||
              refNativeResident->bytes!=pixels*12u ||
              testNativeResident->bytes!=pixels*12u)
               throw std::runtime_error(
                   "Resident native RGB shape mismatch");

           const unsigned dispatchStart=
               residentCsfDispatches;

           const unsigned uploadStart=
               residentCsfPlanUploads;

           auto dispatchComparison=[
               &](
               CSFKernelSingle kind,
               std::vector<FFTBufferRegion> regions,
               std::vector<uint32_t> constants,
               unsigned count){

               const unsigned groups=(count+63u)/64u;

               const unsigned gx=std::min(
                   groups,
                   std::min(
                       32768u,
                       p.getProperties().limits.
                           maxComputeWorkGroupCount[0]));

               if(!gx)
                   throw std::runtime_error(
                       "Invalid comparison dispatch size");

               const unsigned gy=(groups+gx-1u)/gx;

               dispatchCsf(
                   kind,regions,constants,gx,gy);
           };

           comparisonMaskResident.emplace(
               allocateCsf(pixels*8u));

           dispatchComparison(
               CSFKernelSingle::Comparison,
               {
                   csfRegion(*refNativeResident),
                   csfRegion(*testNativeResident),
                   csfRegion(*comparisonMaskResident)
               },
               {pixels},
               pixels);

           auto resizeComparisonAxis=[
               &](
               const GpuFloatBuffer& source,
               GpuFloatBuffer& destination,
               unsigned iw,unsigned ih,
               unsigned ow,unsigned oh,
               bool horizontal){

               const unsigned from=horizontal?iw:ih;
               const unsigned to=horizontal?ow:oh;

               const auto plan=make_resize_axis_plan(
                   int(from),int(to));

               std::vector<double> packed(
                   size_t(to)*(2*plan.taps+1));

               for(unsigned coordinate=0;
                   coordinate<to;
                   ++coordinate){

                   const size_t offset=
                       size_t(coordinate)*
                       (2*plan.taps+1);

                   packed[offset]=
                       plan.normalization[coordinate];

                   for(int tap=0;
                       tap<plan.taps;
                       ++tap){

                       const size_t k=
                           size_t(coordinate)*
                           plan.taps+tap;

                       packed[offset+1+2*tap]=
                           double(plan.indices[k]);

                       packed[offset+2+2*tap]=
                           plan.weights[k];
                   }
               }

               auto planBuffer=allocateCsf(
                   unsigned(packed.size()*8u),true);

               uploadPlan(planBuffer,packed);

               dispatchComparison(
                   CSFKernelSingle::ComparisonResize,
                   {
                       csfRegion(source),
                       csfRegion(planBuffer),
                       csfRegion(destination)
                   },
                   {
                       ow*oh,
                       iw,ih,ow,oh,
                       uint32_t(plan.taps),
                       horizontal?1u:0u,
                       0u
                   },
                   ow*oh);
           };

           size_t totalBandValues=0;

           comparisonBandMasks.reserve(
               reference.pyramid.bands.size());

           comparisonBandSlots.reserve(
               reference.pyramid.bands.size());

           for(const auto& band:
               reference.pyramid.bands){

               const unsigned bw=band.width;
               const unsigned bh=band.height;

               totalBandValues+=size_t(bw)*bh;

               if(bw==w && bh==h){

                   comparisonBandSlots.push_back(
                       size_t(-1));

                   continue;
               }

               std::optional<GpuFloatBuffer> horizontal;

               const GpuFloatBuffer* source=
                   &*comparisonMaskResident;

               if(bw!=w){

                   horizontal.emplace(
                       allocateCsf(bw*h*8u));

                   resizeComparisonAxis(
                       *source,*horizontal,
                       w,h,bw,h,true);

                   source=&*horizontal;
               }

               comparisonBandSlots.push_back(
                   comparisonBandMasks.size());

               if(bh!=h){

                   comparisonBandMasks.emplace_back(
                       allocateCsf(bw*bh*8u));

                   resizeComparisonAxis(
                       *source,
                       comparisonBandMasks.back(),
                       bw,h,bw,bh,false);

               }else{

                   comparisonBandMasks.emplace_back(
                       std::move(*horizontal));
               }
           }

           const unsigned comparisonDispatchEnd=
               residentCsfDispatches;

           if(residentPooling){

               const size_t count=
                   reference.pyramid.bands.size();

               if(!count ||
                  residentRawDifferences.size()!=count ||
                  comparisonBandSlots.size()!=count)
                   throw std::runtime_error(
                       "Resident pooling band count mismatch");

               const unsigned poolingDispatchStart=
                   residentCsfDispatches;

               size_t poolingValues=0;

               residentBandErrors.reserve(count);

               for(size_t b=0;b<count;++b){

                   const auto& band=
                       reference.pyramid.bands[b];

                   const unsigned pw=band.width;
                   const unsigned ph=band.height;

                   const size_t n64=size_t(pw)*ph;

                   if(!n64 || n64>0x1fffffffu)
                       throw std::runtime_error(
                           "Invalid resident pooling geometry");

                   const unsigned n=
                       static_cast<unsigned>(n64);

                   const auto& raw=
                       residentRawDifferences[b];

                   const size_t slot=
                       comparisonBandSlots[b];

                   const GpuFloatBuffer* mask=nullptr;

                   if(slot==size_t(-1)){

                       if(!comparisonMaskResident)
                           throw std::runtime_error(
                               "Missing resident comparison mask");

                       mask=&*comparisonMaskResident;

                   }else{

                       if(slot>=comparisonBandMasks.size())
                           throw std::runtime_error(
                               "Invalid comparison band slot");

                       mask=&comparisonBandMasks[slot];
                   }

                   if(raw.bytes!=n64*sizeof(float) ||
                      mask->bytes!=n64*sizeof(double))
                       throw std::runtime_error(
                           "Resident pooling buffer size mismatch");

                   auto terms=allocateCsf(
                       n*sizeof(float));

                   dispatchComparison(
                       CSFKernelSingle::PoolingTerms,
                       {
                           csfRegion(raw),
                           csfRegion(*mask),
                           csfRegion(terms)
                       },
                       {n},
                       n);

                   residentBandErrors.emplace_back(
                       allocateCsf(sizeof(float)));

                   dispatchCsf(
                       CSFKernelSingle::PoolingReduce,
                       {
                           csfRegion(terms),
                           csfRegion(
                               residentBandErrors.back())
                       },
                       {pw,ph},
                       1,1);

                   poolingValues+=n64;

                   log<<"resident_pooling_band="
                      <<b<<" complete\n"
                      <<std::flush;
               }

               std::ofstream poolingReport(
                   out/"resident_pooling_compute.txt");

               poolingReport
                   <<"pooling_bands="<<count<<'\n'
                   <<"pooling_input_values="
                   <<poolingValues<<'\n'
                   <<"pooling_dispatches="
                   <<residentCsfDispatches-
                       poolingDispatchStart<<'\n'
                   <<"band_error_buffers="
                   <<residentBandErrors.size()<<'\n'
                   <<"band_error_bytes="
                   <<residentBandErrors.size()*
                       sizeof(float)<<'\n'
                   <<"raw_difference_input_reuploads=0\n"
                   <<"comparison_mask_input_reuploads=0\n"
                   <<"pooling_input_readbacks=0\n"
                   <<"band_error_readbacks=0\n"
                   <<"final_score_consumer_connected=0\n"
                   <<"full_gpu=0\n";

               if(!poolingReport)
                   throw std::runtime_error(
                       "Resident pooling report failed");

               if(residentFinalScore){

                   if(residentBandErrors.size()!=6u)
                       throw std::runtime_error(
                           "Resident final score requires six bands");

                   std::vector<FFTBufferRegion> scoreRegions;
                   scoreRegions.reserve(7);

                   for(const auto& bandError:
                       residentBandErrors){

                       if(bandError.bytes!=sizeof(float))
                           throw std::runtime_error(
                               "Invalid resident band error size");

                       scoreRegions.push_back(
                           csfRegion(bandError));
                   }

                   residentQualityScores.emplace(
                       allocateCsf(3u*sizeof(float)));

                   scoreRegions.push_back(
                       csfRegion(*residentQualityScores));

                   const unsigned finalDispatchStart=
                       residentCsfDispatches;

                   dispatchCsf(
                       CSFKernelSingle::FinalScore,
                       scoreRegions,
                       {6u},
                       1u,1u);

                   std::ofstream finalReport(
                       out/"resident_final_score_compute.txt");

                   finalReport
                       <<"final_score_input_bands="
                       <<residentBandErrors.size()<<'\n'
                       <<"final_score_input_bytes="
                       <<residentBandErrors.size()*
                           sizeof(float)<<'\n'
                       <<"final_score_dispatches="
                       <<residentCsfDispatches-
                           finalDispatchStart<<'\n'
                       <<"final_score_output_values=3\n"
                       <<"final_score_output_bytes="
                       <<residentQualityScores->bytes<<'\n'
                       <<"band_error_input_reuploads=0\n"
                       <<"band_error_input_readbacks=0\n"
                       <<"final_score_output_readbacks=0\n"
                       <<"public_score_consumer_connected=0\n"
                       <<"full_gpu=0\n";

                   if(!finalReport)
                       throw std::runtime_error(
                           "Resident final score report failed");

                   log<<"resident_final_score_complete\n"
                      <<std::flush;
               }
           }

           std::ofstream report(
               out/"resident_comparison_compute.txt");

           report
               <<"native_rgb_buffers_retained=2\n"
               <<"native_rgb_input_reuploads=0\n"
               <<"comparison_mask_values="<<pixels<<'\n'
               <<"comparison_mask_bytes="
               <<comparisonMaskResident->bytes<<'\n'
               <<"comparison_band_slots="
               <<comparisonBandSlots.size()<<'\n'
               <<"comparison_resized_buffers="
               <<comparisonBandMasks.size()<<'\n'
               <<"comparison_band_values="
               <<totalBandValues<<'\n'
               <<"comparison_dispatches="
               <<comparisonDispatchEnd-dispatchStart<<'\n'
               <<"resize_plan_uploads="
               <<residentCsfPlanUploads-uploadStart<<'\n'
               <<"resident_comparison_readbacks=0\n"
               <<"pooling_consumer_connected=0\n"
               <<"full_gpu=0\n";

           if(!report)
               throw std::runtime_error(
                   "Resident comparison report failed");

           log<<"resident_comparison_complete\n"
              <<std::flush;
       }

       std::ofstream csfLifetime(
           out/"resident_csf_lifetime.txt");

       csfLifetime
           <<"adaptation_buffers_retained=2\n"
           <<"adaptation_values_per_buffer="
           <<expected<<'\n'
           <<"residual_frequency_buffers_retained=1\n"
           <<"residual_frequency_bytes=8\n"
           <<"adaptation_reuploads=0\n"
           <<"frequency_reuploads=0\n"
           <<"csf_compute_integrated=1\n"
           <<"full_gpu=0\n"
           <<"PASS=1\n";

       if(!csfLifetime)
           throw std::runtime_error(
               "Resident CSF lifetime report failed");
    }

    refPyramid.reset(); testPyramid.reset();
   residentProvider.mutual=[&](auto a,auto z,unsigned bw,unsigned bh){
     if(nextMask>=residentMasks.size() || a.size()!=size_t(bw)*bh || z.size()!=a.size() || residentMasks[nextMask].size()!=a.size())
       throw std::runtime_error("Resident mutual consumer mismatch");
     return std::move(residentMasks[nextMask++]);
   };
   mutual=&residentProvider;
 }
#endif
 if(residentResidual && residualExecutions!=1)
     throw std::runtime_error("Resident residual execution missing");

 std::vector<std::vector<float>> csfHostSensitivity;
 std::vector<std::vector<float>> maskingHostRaw;
 double csfHostNoise=0.;

 if(residentCsf && !residentMasking){

     if(residentCsfSensitivity.size()!=reference.pyramid.bands.size() ||
        !residentCsfNoise)
         throw std::runtime_error("Resident CSF outputs missing");

     auto downloadCsf=[&](const GpuFloatBuffer& b,void* destination){

         auto staging=IQM::Bin::VulkanResource::createBuffer(
             *gpu->device(),*gpu->physicalDevice(),b.bytes,
             vk::BufferUsageFlagBits::eTransferDst,
             vk::MemoryPropertyFlagBits::eHostVisible |
             vk::MemoryPropertyFlagBits::eHostCoherent);

         staging.first.bindMemory(staging.second,0);

         gpu->cmdBuf()->reset();

         gpu->cmdBuf()->begin(
             vk::CommandBufferBeginInfo{
                 .flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

         vk::BufferMemoryBarrier rb{
             .srcAccessMask=vk::AccessFlagBits::eMemoryWrite,
             .dstAccessMask=vk::AccessFlagBits::eTransferRead,
             .srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
             .dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
             .buffer=*b.buffer,
             .offset=0,
             .size=b.bytes};

         gpu->cmdBuf()->pipelineBarrier(
             vk::PipelineStageFlagBits::eAllCommands,
             vk::PipelineStageFlagBits::eTransfer,
             {},{},{rb},{});

         gpu->cmdBuf()->copyBuffer(
             *b.buffer,*staging.first,
             vk::ArrayProxy<const vk::BufferCopy>{
                 vk::BufferCopy{0,0,b.bytes}});

         vk::MemoryBarrier hr{
             .srcAccessMask=vk::AccessFlagBits::eTransferWrite,
             .dstAccessMask=vk::AccessFlagBits::eHostRead};

         gpu->cmdBuf()->pipelineBarrier(
             vk::PipelineStageFlagBits::eTransfer,
             vk::PipelineStageFlagBits::eHost,
             {},{hr},{},{});

         gpu->cmdBuf()->end();

         vk::CommandBuffer cmd=*gpu->cmdBuf();

         vk::raii::Fence fence(
             *gpu->device(),vk::FenceCreateInfo{});

         gpu->queue()->submit(
             vk::SubmitInfo{
                 .commandBufferCount=1,
                 .pCommandBuffers=&cmd},fence);

         gpu->waitForFence(fence);

         void* ptr=staging.second.mapMemory(0,b.bytes);

         std::memcpy(destination,ptr,b.bytes);

         staging.second.unmapMemory();

         gpu->cmdBuf()->reset();
     };

     size_t sensitivityValues=0;

     for(const auto& b:residentCsfSensitivity){

         if(b.bytes!=size_t(b.count)*sizeof(float))
             throw std::runtime_error("CSF output byte mismatch");

         csfHostSensitivity.emplace_back(b.count);

         downloadCsf(
             b,csfHostSensitivity.back().data());

         sensitivityValues+=b.count;
     }

     if(residentCsfNoise->bytes!=sizeof(double))
         throw std::runtime_error("CSF noise byte mismatch");

     downloadCsf(*residentCsfNoise,&csfHostNoise);

     std::ofstream report(
         out/"resident_csf_consumer.txt");

     report<<std::setprecision(17)
         <<"sensitivity_readbacks="
         <<csfHostSensitivity.size()<<'\n'
         <<"sensitivity_values="
         <<sensitivityValues<<'\n'
         <<"noise_readbacks=1\n"
         <<"base_noise="<<csfHostNoise<<'\n'
         <<"output_readback_bytes="
         <<sensitivityValues*sizeof(float)+sizeof(double)<<'\n'
         <<"cpu_csf_recomputation=0\n"
         <<"full_gpu=0\n";

     if(!report)
         throw std::runtime_error("CSF consumer report failed");
 }

 if(residentMasking && !residentFinalScore){

     if(residentRawDifferences.size()!=
         reference.pyramid.bands.size())
         throw std::runtime_error(
             "Resident raw difference buffers missing");

     size_t rawValues=0;

     for(const auto& buffer:residentRawDifferences){

         maskingHostRaw.emplace_back(readback(buffer));

         rawValues+=maskingHostRaw.back().size();
     }

     std::ofstream report(
         out/"resident_masking_consumer.txt");

     report
         <<"raw_difference_readbacks="
         <<maskingHostRaw.size()<<'\n'
         <<"raw_difference_values="
         <<rawValues<<'\n'
         <<"raw_difference_readback_bytes="
         <<rawValues*sizeof(float)<<'\n'
         <<"csf_sensitivity_readbacks=0\n"
         <<"csf_noise_readbacks=0\n"
         <<"cpu_csf_recomputation=0\n"
         <<"cpu_masking_recomputation=0\n"
         <<"cpu_pooling=1\n"
         <<"full_gpu=0\n";

     if(!report)
         throw std::runtime_error(
             "Resident masking consumer report failed");
 }

 GeneralQualityResultSingle result{};

 if(residentFinalScore){

     if(!residentQualityScores ||
        residentQualityScores->bytes!=3u*sizeof(float) ||
        residentQualityScores->count!=3u)
         throw std::runtime_error(
             "Resident final score output missing");

     const auto values=readback(*residentQualityScores);

     if(values.size()!=3u)
         throw std::runtime_error(
             "Resident final score shape mismatch");

     for(float value:values)
         if(!std::isfinite(value))
             throw std::runtime_error(
                 "Nonfinite resident final score");

     if(values[0]<0.f)
         throw std::runtime_error(
             "Negative resident quality error");

     result.scores={values[0],values[1],values[2]};
     result.base_frequency=residualFrequency;

      backendName="vulkan-resident-quality-score";

      const bool resizeDiagnostic=
          envEnabled("IQM_HDRVDP3_INPUT_RESIZE_VERIFY");

      const bool residentQualityProduction=
          residentQualityNoFftw && !resizeDiagnostic;

      std::ofstream audit(out/"quality_execution_scope.txt");

      audit
          <<"resident_quality_path="<<residentQualityNoFftw<<'\n'
          <<"production_mode="<<residentQualityProduction<<'\n'
          <<"resize_diagnostic_enabled="<<resizeDiagnostic<<'\n'
          <<"reference_width="<<w<<'\n'
          <<"reference_height="<<h<<'\n'
          <<"cpu_fft_provider_created="<<(fft!=nullptr)<<'\n'
          <<"cpu_quality_evaluator_calls=0\n"
          <<"cpu_pooling=0\n"
          <<"cpu_final_score_mapping=0\n"
          <<"cpu_image_decoding=1\n"
          <<"cpu_geometry_plan_preparation=1\n"
          <<"cpu_fixed_parameter_preparation=1\n"
          <<"residual_metadata_readback_bytes=20\n"
          <<"final_score_readback_bytes=12\n"
          <<"sp0_host_readbacks="<<gpuSp0HostReadbacks<<'\n'
          <<"mutual_host_readbacks="<<residentMutualOutputReadbacks<<'\n'
          <<"official_reference_parity_verified=0\n"
          <<"general_resolution_support_verified=0\n"
          <<"full_gpu_acceptance_pending=1\n";

      audit.flush();

      if(!audit)
          throw std::runtime_error(
              "Quality execution scope report failed");

     std::ofstream report(
         out/"resident_final_score_consumer.txt");

     report<<std::setprecision(17)
         <<"error="<<values[0]<<'\n'
         <<"Q="<<values[1]<<'\n'
         <<"Q_JOD="<<values[2]<<'\n'
         <<"final_score_readbacks=1\n"
         <<"final_score_readback_bytes=12\n"
         <<"band_error_readbacks=0\n"
         <<"raw_difference_readbacks=0\n"
         <<"cpu_quality_evaluator_calls=0\n"
         <<"cpu_pooling=0\n"
         <<"cpu_final_score_mapping=0\n"
         <<"public_score_consumer_connected=1\n"
         <<"full_gpu=0\n";

     report.flush();

     if(!report)
         throw std::runtime_error(
             "Resident final score consumer report failed");

     log<<"resident_public_score_complete\n"
        <<std::flush;

 }else{

     result=evaluate_quality_single(
         reference,test,w,h,ppd,backend,
         comparison,mutual,comparisonResize,
         residentResidual ? &residualFrequency : nullptr,
         (residentCsf && !residentMasking) ? &csfHostSensitivity : nullptr,
         (residentCsf && !residentMasking) ? &csfHostNoise : nullptr,
         residentMasking ? &maskingHostRaw : nullptr);
 }
#if defined(IQM_HDRVDP3_HYBRID) && defined(IQM_HDRVDP3_MUTUAL)
 if(residentMutual && !residentMasking && nextMask!=residentMasks.size()) throw std::runtime_error("Unused resident masks");
#endif
 double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
 std::ofstream f(out/"score.json");f.precision(17);f<<"{\"scope\":\"IQM public Single quality with explicit backend\",\"backend\":\""<<backendName<<"\",\"encoding\":\"pq-bt2020-rgb16\",\"width\":"<<w<<",\"height\":"<<h<<",\"ppd\":"<<ppd<<",\"gpu_matrix_dispatches\":"<<matrixDispatches<<",\"age\":24,\"display\":\"oled\",\"ambient_lux\":200,\"reflectivity\":0.005,\"peak\":2000,\"fftw_threads\":"<<(fft?3:0)<<",\"fftw_planner\":\""<<(fft?"estimate":"none")<<"\",\"elapsed_seconds\":"<<elapsed<<",\"Q_text\":\""<<result.scores.Q<<"\",\"Q_JOD_text\":\""<<result.scores.Q_JOD<<"\",\"Q_hex\":\""<<std::hex<<std::setw(8)<<std::setfill('0')<<std::bit_cast<uint32_t>(result.scores.Q)<<"\",\"Q_JOD_hex\":\""<<std::setw(8)<<std::bit_cast<uint32_t>(result.scores.Q_JOD)<<std::dec<<"\",\"base_frequency\":"<<result.base_frequency<<"}\n";
 if(!f)throw std::runtime_error("Score output failed");
  std::ofstream stages(out/"gpu_stages.json");stages<<"{\"matrix_dispatches\":"<<matrixDispatches<<",\"comparison_dispatches\":"<<comparisonDispatches<<",\"mutual_dispatches\":"<<mutualDispatches<<",\"comparison_resize_calls\":"<<comparisonResizeCalls<<",\"comparison_resize_dispatches\":"<<comparisonResizeDispatches<<",\"adaptation_input_dispatches\":"<<adaptationInputDispatches<<",\"padding_dispatches\":"<<paddingDispatches<<",\"pq_display_dispatches\":"<<gpuPqDispatches<<",\"optical_dispatches\":"<<gpuOpticalDispatches<<",\"adaptation_dispatches\":"<<gpuAdaptDispatches<<",\"jnd_dispatches\":"<<gpuJndDispatches<<",\"sp0_dispatches\":"<<gpuSp0Dispatches<<",\"sp0_host_readbacks\":"<<gpuSp0HostReadbacks<<",\"resident_mutual\":"<<(residentMutual?"true":"false")<<",\"resident_mutual_dispatches\":"<<residentMutualDispatches<<",\"resident_mutual_input_uploads\":"<<residentMutualInputUploads<<",\"resident_mutual_output_readbacks\":"<<residentMutualOutputReadbacks<<",\"full_gpu_stage1\":"<<(fullGpu?"true":"false")<<",\"full_gpu\":false}";
 if(!stages)throw std::runtime_error("GPU stage report failed");
 std::cout<<std::setprecision(17)<<"Q="<<result.scores.Q<<" Q_JOD="<<result.scores.Q_JOD<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<"HDR-VDP-3 quality error: "<<e.what()<<'\n';return 1;}}