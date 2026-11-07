#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <IQM/hdrvdp3/fftw_single_provider.h>
#include <IQM/hdrvdp3/general_quality_single.h>
#include <IQM/hdrvdp3/pq_single.h>
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
#endif
#endif
#endif
namespace fs=std::filesystem;using namespace IQM::HDRVDP3;
void help(){std::cout<<"IQM HDR-VDP-3 Single quality (explicit FFTW)\n"
 "Usage: <executable> --input test.png --ref reference.png\n"
 "  --encoding pq-bt2020-rgb16 --ppd NUMBER --model-data DIR\n"
 "  --fftw PATH_TO_FLOAT_FFTW_DLL --output FRESH_DIRECTORY\n"
 "Profile: quality/sp0, age24, OLED, ambient200lux, reflectivity0.005,\n"
 "peak2000cd/m2 before ambient. RGB16 PNG codes must already be PQ BT2020.\n"
 "Distorted input is bicubic-resized in PQ to the reference size and clamped.\n"
 "CPU executable: no Vulkan initialization or implicit fallback.\n";
#ifdef IQM_HDRVDP3_HYBRID
std::cout<<"Hybrid executable additionally REQUIRES --projection-shader PATH:\nVulkan native/LMSR matrices; CPU FFT/adaptation/JND/pyramid/quality.\n";
#ifdef IQM_HDRVDP3_COMPARISON
std::cout<<"Experimental mask profile also REQUIRES --comparison-shader PATH and shaderFloat64.\nNative comparison mask executes on Vulkan; remaining quality arithmetic stays CPU.\n";
#ifdef IQM_HDRVDP3_MUTUAL
std::cout<<"Mutual profile additionally REQUIRES --mutual-shader PATH; per-band mutual activity executes on Vulkan.\n";
std::cout<<"Optional --comparison-resize-shader PATH also runs Double comparison-mask resizing on Vulkan.\n";
#endif
#endif
#endif
}
int main(int argc,char**argv){try{
 if(argc==2&&std::string(argv[1])=="--help"){help();return 0;}
 std::vector<std::string> keys{"--input","--ref","--encoding","--ppd","--model-data","--fftw","--output"};
#ifdef IQM_HDRVDP3_HYBRID
 keys.push_back("--projection-shader");
#ifdef IQM_HDRVDP3_COMPARISON
 keys.push_back("--comparison-shader");
#ifdef IQM_HDRVDP3_MUTUAL
 keys.push_back("--mutual-shader");
#endif
#endif
#endif
 std::unordered_map<std::string,std::string> options;
 auto acceptedKeys=keys;
#ifdef IQM_HDRVDP3_MUTUAL
 acceptedKeys.push_back("--comparison-resize-shader");
#endif
 for(int i=1;i<argc;i+=2){std::string key=argv[i];if(std::find(acceptedKeys.begin(),acceptedKeys.end(),key)==acceptedKeys.end()||i+1>=argc||!argv[i+1][0]||std::string(argv[i+1]).starts_with("--")||!options.emplace(key,argv[i+1]).second)throw std::invalid_argument("Unknown, missing or duplicate argument: "+key);}
 for(const auto& key:keys)if(!options.contains(key))throw std::invalid_argument("Required argument: "+key);
 if(options.at("--encoding")!="pq-bt2020-rgb16")throw std::invalid_argument("Only explicit pq-bt2020-rgb16 encoding is supported");
 size_t parsed=0;double ppd=std::stod(options.at("--ppd"),&parsed);if(parsed!=options.at("--ppd").size()||!std::isfinite(ppd)||ppd<26||ppd>256)throw std::invalid_argument("PPD must be finite in [26,256]");
 fs::path data=options.at("--model-data"),out=options.at("--output");if(fs::exists(out)||!fs::create_directories(out))throw std::invalid_argument("Fresh output directory required");
 FFTWProviderSingle fft(options.at("--fftw"));auto backend=fft.backend();
 const SingleMatrixBackend* matrices=nullptr;
 const char* backendName="cpu-fftw";
 unsigned matrixDispatches=0;
 const SingleComparisonBackend* comparison=nullptr;
 unsigned comparisonDispatches=0;
 const SingleMutualBackend* mutual=nullptr;
 unsigned mutualDispatches=0;
 const SingleComparisonResizeBackend* comparisonResize=nullptr;
 unsigned comparisonResizeCalls=0,comparisonResizeDispatches=0;
#ifdef IQM_HDRVDP3_HYBRID
#ifdef IQM_HDRVDP3_COMPARISON
 auto gpu=std::make_unique<IQM::Bin::VulkanInstance>(true,"",true);
 SingleComparisonBackend gpuComparison{[&](auto r,auto t){auto values=comparison_single_gpu(*gpu,options.at("--comparison-shader"),r,t);++comparisonDispatches;return values;}};
 comparison=&gpuComparison;
#ifdef IQM_HDRVDP3_MUTUAL
 SingleMutualBackend gpuMutual{[&](auto r,auto t,unsigned w,unsigned h){auto values=mutual_single_gpu(*gpu,options.at("--mutual-shader"),r,t,w,h);++mutualDispatches;return values;}};
 mutual=&gpuMutual;
 SingleComparisonResizeBackend gpuResize{[&](auto mask,unsigned h,unsigned w,unsigned nh,unsigned nw){++comparisonResizeCalls;auto result=resize_comparison_gpu(*gpu,options.at("--comparison-resize-shader"),mask,h,w,nh,nw);comparisonResizeDispatches+=result.dispatches;return std::move(result.values);}};
 if(options.contains("--comparison-resize-shader"))comparisonResize=&gpuResize;
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
#endif
#endif
#endif
 auto xyz=load_xyz_display_samples(data/"ciexyz31.csv",data/"emission_spectra_oled.csv");
 auto spectral=load_rgb_spectral_samples(data/"log_cone_smith_pokorny_1975.csv",data/"cie_scotopic_lum.txt",data/"emission_spectra_oled.csv");
 unsigned w=0,h=0;auto start=std::chrono::steady_clock::now();std::ofstream log(out/"progress.log");
 auto prepare=[&](const std::string& file,const char* side){
   log<<side<<" start\n"<<std::flush;std::cout<<side<<' '<<backendName<<" Single\n"<<std::flush;
   if(!stbi_is_16_bit(file.c_str()))throw std::invalid_argument("Explicit RGB16 PNG required");int iw,ih,channels;
   std::unique_ptr<stbi_us,decltype(&stbi_image_free)> raw(stbi_load_16(file.c_str(),&iw,&ih,&channels,3),stbi_image_free);
   if(!raw||iw<16||ih<16||channels!=3)throw std::invalid_argument("RGB16 image decode/shape failed");
   unsigned sw=unsigned(iw),sh=unsigned(ih);size_t sn=size_t(sw)*sh;if(!w){w=sw;h=sh;}size_t n=size_t(w)*h;
   std::vector<float> encoded(3*sn);for(size_t c=0;c<3;++c)for(size_t i=0;i<sn;++i)encoded[c*sn+i]=normalize_pq_u16_single(raw.get()[3*i+c]);raw.reset();
   if(sw!=w||sh!=h){std::vector<float> resized(3*n);for(size_t c=0;c<3;++c){std::vector<float> plane(encoded.begin()+c*sn,encoded.begin()+(c+1)*sn);auto values=resize_bicubic(plane,int(sh),int(sw),int(h),int(w));for(size_t i=0;i<n;++i)resized[c*n+i]=std::clamp(values[i],0.f,1.f);}encoded=std::move(resized);}
   auto linear=pq_to_display_single(encoded,{200.,.005,2000.});encoded.clear();encoded.shrink_to_fit();
   auto result=prepare_quality_side_single(linear,w,h,ppd,xyz,spectral,backend,matrices);log<<side<<" ready\n"<<std::flush;return result;
 };
 auto reference=prepare(options.at("--ref"),"reference"),test=prepare(options.at("--input"),"test");
 auto result=evaluate_quality_single(reference,test,w,h,ppd,backend,comparison,mutual,comparisonResize);
 double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
 std::ofstream f(out/"score.json");f.precision(17);f<<"{\"scope\":\"IQM public Single quality with explicit backend\",\"backend\":\""<<backendName<<"\",\"encoding\":\"pq-bt2020-rgb16\",\"width\":"<<w<<",\"height\":"<<h<<",\"ppd\":"<<ppd<<",\"gpu_matrix_dispatches\":"<<matrixDispatches<<",\"age\":24,\"display\":\"oled\",\"ambient_lux\":200,\"reflectivity\":0.005,\"peak\":2000,\"fftw_threads\":3,\"fftw_planner\":\"estimate\",\"elapsed_seconds\":"<<elapsed<<",\"Q_text\":\""<<result.scores.Q<<"\",\"Q_JOD_text\":\""<<result.scores.Q_JOD<<"\",\"Q_hex\":\""<<std::hex<<std::setw(8)<<std::setfill('0')<<std::bit_cast<uint32_t>(result.scores.Q)<<"\",\"Q_JOD_hex\":\""<<std::setw(8)<<std::bit_cast<uint32_t>(result.scores.Q_JOD)<<std::dec<<"\",\"base_frequency\":"<<result.base_frequency<<"}\n";
 if(!f)throw std::runtime_error("Score output failed");
 std::ofstream stages(out/"gpu_stages.json");stages<<"{\"matrix_dispatches\":"<<matrixDispatches<<",\"comparison_dispatches\":"<<comparisonDispatches<<",\"mutual_dispatches\":"<<mutualDispatches<<",\"comparison_resize_calls\":"<<comparisonResizeCalls<<",\"comparison_resize_dispatches\":"<<comparisonResizeDispatches<<",\"full_gpu\":false}";
 if(!stages)throw std::runtime_error("GPU stage report failed");
 std::cout<<std::setprecision(17)<<"Q="<<result.scores.Q<<" Q_JOD="<<result.scores.Q_JOD<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<"HDR-VDP-3 quality error: "<<e.what()<<'\n';return 1;}}
