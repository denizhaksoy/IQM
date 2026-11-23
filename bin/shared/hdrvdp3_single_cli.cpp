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
#include <vulkan_res.h>
#include <vulkan/vulkan.hpp>
#include <IQM/hdrvdp3/resize.h>
#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <fstream>
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
#endif
#endif
#endif
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
 for(const auto& key:keys)if(!options.contains(key))throw std::invalid_argument("Required argument: "+key);
 if(options.at("--encoding")!="pq-bt2020-rgb16")throw std::invalid_argument("Only explicit pq-bt2020-rgb16 encoding is supported");
 size_t parsed=0;double ppd=std::stod(options.at("--ppd"),&parsed);if(parsed!=options.at("--ppd").size()||!std::isfinite(ppd)||ppd<26||ppd>256)throw std::invalid_argument("PPD must be finite in [26,256]");
 fs::path data=options.at("--model-data"),out=options.at("--output");if(fs::exists(out)||!fs::create_directories(out))throw std::invalid_argument("Fresh output directory required");
 FFTWProviderSingle fft(options.at("--fftw"));auto backend=fft.backend();
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
  auto fnv=[&](std::span<const float> v){uint64_t h=1469598103934665603ull; for(float x:v){uint32_t b=std::bit_cast<uint32_t>(x); h^=b; h*=1099511628211ull;} return h;};
  auto dump=[&](const char* side,const char* stage,std::span<const float> v){std::ofstream d(out/(std::string("diag_")+side+"_"+stage+"_run"+std::to_string(diagRun)+".txt")); d<<"n="<<v.size()<<" fnv="<<std::hex<<fnv(v)<<std::dec<<"\n"; for(size_t i=0;i<v.size();++i){ d<<std::hex<<std::bit_cast<uint32_t>(v[i]); if(i+1<v.size()) d<<","; } d<<std::dec<<"\n"; for(size_t i=0;i<v.size();++i) d<<std::setprecision(9)<<v[i]<<"\n"; };
  auto prepare=[&](const std::string& file,const char* side){
    log<<side<<" start\n"<<std::flush;std::cout<<side<<' '<<(fullGpu?"full-gpu-optical":"cpu-prep")<<' '<<backendName<<" Single\n"<<std::flush;
    if(!stbi_is_16_bit(file.c_str()))throw std::invalid_argument("Explicit RGB16 PNG required");int iw,ih,channels;
    std::unique_ptr<stbi_us,decltype(&stbi_image_free)> raw(stbi_load_16(file.c_str(),&iw,&ih,&channels,3),stbi_image_free);
    if(!raw||iw<16||ih<16||channels!=3)throw std::invalid_argument("RGB16 image decode/shape failed");
    unsigned sw=unsigned(iw),sh=unsigned(ih);size_t sn=size_t(sw)*sh;if(!w){w=sw;h=sh;}size_t n=size_t(w)*h;
    std::vector<float> encoded(3*sn);for(size_t c=0;c<3;++c)for(size_t i=0;i<sn;++i)encoded[c*sn+i]=normalize_pq_u16_single(raw.get()[3*i+c]);raw.reset();
    if(sw!=w||sh!=h){std::vector<float> resized(3*n);for(size_t c=0;c<3;++c){std::vector<float> plane(encoded.begin()+c*sn,encoded.begin()+(c+1)*sn);auto values=resize_bicubic(plane,int(sh),int(sw),int(h),int(w));for(size_t i=0;i<n;++i)resized[c*n+i]=std::clamp(values[i],0.f,1.f);}encoded=std::move(resized);}
    if(fullGpu){
#ifdef IQM_HDRVDP3_HYBRID
      // PQ encoded -> GPU PQ display (device-resident display-linear RGB)
      auto gpuLinear=pq_to_display_single_gpu(*gpu,options.at("--pq-display-shader"),encoded,{200.,.005,2000.});++gpuPqDispatches;
      encoded.clear();encoded.shrink_to_fit();
      // Display-linear RGB -> GPU NativeColor projection (device-resident native RGB)
      const auto color=linear_color_transform(xyz,LinearColorEncoding::BT2020);
      std::vector<float> nativeCoeffs;for(unsigned c=0;c<3;++c)for(unsigned p=0;p<3;++p)nativeCoeffs.push_back(float(color[c][p]));
      auto gpuNative=project_rgb_single_gpu_device(*gpu,options.at("--projection-shader"),*gpuLinear.buffer,n,nativeCoeffs,SingleProjectionKind::NativeColor);++matrixDispatches;
      // Native RGB -> GPU Optical MTF (device-resident optical RGB)
      auto gpuOptical=filter_optical_rgb_single_gpu(*gpu,*gpuNative.buffer,w,h,float(ppd));++gpuOpticalDispatches;
      // Optical RGB -> GPU LMSR projection (device-resident)
      const auto lmsrModel=calibrate_rgb_spectral_age(spectral,24.); std::vector<float> lmsrCoeffs; for(unsigned c=0;c<4;++c)for(unsigned p=0;p<3;++p)lmsrCoeffs.push_back(float(lmsrModel.matrix[p][c]));
      auto gpuLmsr=project_rgb_single_gpu_device(*gpu,options.at("--projection-shader"),*gpuOptical.buffer,n,lmsrCoeffs,SingleProjectionKind::LMSR);++matrixDispatches;
            // Device-resident L+M, local adaptation, JND; legacy readback for SP0/quality.
      auto readback=[&](const GpuFloatBuffer& b){ auto staging=IQM::Bin::VulkanResource::createBuffer(*gpu->device(),*gpu->physicalDevice(),b.bytes,vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent); staging.first.bindMemory(staging.second,0); gpu->cmdBuf()->reset(); gpu->cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit}); vk::BufferMemoryBarrier rb{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eTransferRead,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=*b.buffer,.offset=0,.size=b.bytes}; gpu->cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eTransfer,{},{},{rb},{}); gpu->cmdBuf()->copyBuffer(*b.buffer,*staging.first,vk::ArrayProxy<const vk::BufferCopy>{vk::BufferCopy{0,0,b.bytes}}); vk::MemoryBarrier hr{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead}; gpu->cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},{hr},{},{}); gpu->cmdBuf()->end(); vk::CommandBuffer c=*gpu->cmdBuf(); vk::raii::Fence f(*gpu->device(),vk::FenceCreateInfo{}); gpu->queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&c},f); gpu->waitForFence(f); std::vector<float> out(b.count); void* p=staging.second.mapMemory(0,b.bytes); std::memcpy(out.data(),p,b.bytes); staging.second.unmapMemory(); gpu->cmdBuf()->reset(); return out; };
      auto gpuAdaptIn=adaptation_input_single_gpu_device(*gpu,options.at("--adaptation-input-shader"),*gpuLmsr.buffer,unsigned(n)); ++adaptationInputDispatches;
      auto gpuAdapt=local_adaptation_single_gpu(*gpu,*gpuAdaptIn.buffer,w,h,float(ppd)); ++gpuAdaptDispatches;
      QualitySideSingle result;
      result.native_rgb=readback(gpuNative);
      result.adaptation=readback(gpuAdapt);
      auto lmsrHost=readback(gpuLmsr);
      const size_t nn=size_t(w)*h; if(lmsrHost.size()!=4*nn) throw std::runtime_error("LMSR matrix provider shape mismatch");
      if(diagRun){ dump(side,"lmsr",readback(gpuLmsr)); dump(side,"adapt",readback(gpuAdapt)); std::ofstream ds(out/(std::string("diag_")+side+"_jnd_params_run"+std::to_string(diagRun)+".txt")); ds<<std::setprecision(17)<<"w="<<w<<" h="<<h<<" nn="<<nn<<" ppd="<<ppd<<" area="<<(double(nn)/(ppd*ppd))<<" age=24\n"; }
      auto gpuJndBuf=rgb_to_jnd_response_single_gpu_device(*gpu,options.at("--jnd-shader"),*gpuLmsr.buffer,*gpuAdapt.buffer,w,h,double(nn)/(ppd*ppd),24.); ++gpuJndDispatches;
      const bool gpuSp0Enabled = !cpuSp0;
      if(gpuSp0Enabled){
        auto gpuSp0=build_sp0_pyramid_single_gpu(*gpu,options.at("--sp0-corrdn-shader"),options.at("--sp0-base-mean-shader"),*gpuJndBuf.buffer,w,h,ppd);
        gpuSp0Dispatches += gpuSp0.dispatches;
        gpuSp0HostReadbacks += gpuSp0.hostReadbacks;
        result.pyramid=std::move(gpuSp0.host);
        if(diagRun){ auto jndHost=readback(gpuJndBuf); dump(side,"jnd",jndHost); dump(side,"sp0_base",std::span<const float>(&result.pyramid.base_mean,1)); for(size_t bi=0;bi<result.pyramid.bands.size();++bi){ dump(side,(std::string("sp0_b")+std::to_string(bi)).c_str(), result.pyramid.bands[bi].values); } std::ofstream sd(out/(std::string("diag_")+side+"_sp0_gpu_stats_run"+std::to_string(diagRun)+".txt")); sd<<"dispatches="<<gpuSp0.dispatches<<" host_readbacks="<<gpuSp0.hostReadbacks<<"\n"; }
      } else {
        auto jndHost=readback(gpuJndBuf);
        if(diagRun){ dump(side,"jnd",jndHost); }
        result.pyramid=build_sp0_pyramid_single(jndHost,w,h,ppd);
        if(diagRun){ dump(side,"sp0_base",std::span<const float>(&result.pyramid.base_mean,1)); for(size_t bi=0;bi<result.pyramid.bands.size();++bi){ dump(side,(std::string("sp0_b")+std::to_string(bi)).c_str(), result.pyramid.bands[bi].values); } }
      }
      log<<side<<" ready\n"<<std::flush;return result;
#else
      throw std::invalid_argument("--full-gpu requires a Vulkan-enabled HDRVDP3 build");
#endif
    }
    auto linear=pq_to_display_single(encoded,{200.,.005,2000.});encoded.clear();encoded.shrink_to_fit();
    auto result=prepare_quality_side_single(linear,w,h,ppd,xyz,spectral,backend,matrices,adaptationInput,padding);log<<side<<" ready\n"<<std::flush;return result;
  };
 auto reference=prepare(options.at("--ref"),"reference"),test=prepare(options.at("--input"),"test");
 auto result=evaluate_quality_single(reference,test,w,h,ppd,backend,comparison,mutual,comparisonResize);
 double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
 std::ofstream f(out/"score.json");f.precision(17);f<<"{\"scope\":\"IQM public Single quality with explicit backend\",\"backend\":\""<<backendName<<"\",\"encoding\":\"pq-bt2020-rgb16\",\"width\":"<<w<<",\"height\":"<<h<<",\"ppd\":"<<ppd<<",\"gpu_matrix_dispatches\":"<<matrixDispatches<<",\"age\":24,\"display\":\"oled\",\"ambient_lux\":200,\"reflectivity\":0.005,\"peak\":2000,\"fftw_threads\":3,\"fftw_planner\":\"estimate\",\"elapsed_seconds\":"<<elapsed<<",\"Q_text\":\""<<result.scores.Q<<"\",\"Q_JOD_text\":\""<<result.scores.Q_JOD<<"\",\"Q_hex\":\""<<std::hex<<std::setw(8)<<std::setfill('0')<<std::bit_cast<uint32_t>(result.scores.Q)<<"\",\"Q_JOD_hex\":\""<<std::setw(8)<<std::bit_cast<uint32_t>(result.scores.Q_JOD)<<std::dec<<"\",\"base_frequency\":"<<result.base_frequency<<"}\n";
 if(!f)throw std::runtime_error("Score output failed");
  std::ofstream stages(out/"gpu_stages.json");stages<<"{\"matrix_dispatches\":"<<matrixDispatches<<",\"comparison_dispatches\":"<<comparisonDispatches<<",\"mutual_dispatches\":"<<mutualDispatches<<",\"comparison_resize_calls\":"<<comparisonResizeCalls<<",\"comparison_resize_dispatches\":"<<comparisonResizeDispatches<<",\"adaptation_input_dispatches\":"<<adaptationInputDispatches<<",\"padding_dispatches\":"<<paddingDispatches<<",\"pq_display_dispatches\":"<<gpuPqDispatches<<",\"optical_dispatches\":"<<gpuOpticalDispatches<<",\"adaptation_dispatches\":"<<gpuAdaptDispatches<<",\"jnd_dispatches\":"<<gpuJndDispatches<<",\"sp0_dispatches\":"<<gpuSp0Dispatches<<",\"sp0_host_readbacks\":"<<gpuSp0HostReadbacks<<",\"full_gpu_stage1\":"<<(fullGpu?"true":"false")<<",\"full_gpu\":false}";
 if(!stages)throw std::runtime_error("GPU stage report failed");
 std::cout<<std::setprecision(17)<<"Q="<<result.scores.Q<<" Q_JOD="<<result.scores.Q_JOD<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<"HDR-VDP-3 quality error: "<<e.what()<<'\n';return 1;}}
