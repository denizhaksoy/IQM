// Stage-isolation diagnostic: persisted reference stages, NOT independent end-to-end.
#include "single_fftw_backend.h"
#include <vulkan_instance.h>
#include <IQM/hdrvdp3/general_quality_single.h>
#include <IQM/hdrvdp3/comparison_single_gpu.h>
#include <IQM/hdrvdp3/mutual_single_gpu.h>
#include <IQM/hdrvdp3/comparison_resize_gpu.h>
#include <bit>
#include <fstream>
#include <iostream>
namespace fs=std::filesystem;using namespace IQM::HDRVDP3;
std::vector<float> load(const fs::path& file,unsigned w,unsigned h,unsigned channels=1){
 const size_t n=size_t(w)*h;if(fs::file_size(file)!=n*channels*4)throw std::runtime_error("Capture shape");
 std::vector<float> col(n*channels),row(col.size());std::ifstream f(file,std::ios::binary);f.read(reinterpret_cast<char*>(col.data()),col.size()*4);if(!f)throw std::runtime_error("Capture read");
 for(unsigned c=0;c<channels;++c)for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x)row[c*n+size_t(y)*w+x]=col[c*n+size_t(x)*h+y];return row;
}
int main(int argc,char**argv){try{
 if(argc<6||argc>8)throw std::runtime_error("Usage: captured-stage-root shader fftw-single-dll ppd fresh-out [mutual-shader [resize-shader]] (4K only)");
 fs::path root=argv[1],out=argv[5];if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 auto side=[&](const std::string& name){QualitySideSingle s;s.native_rgb=load(root/(name+"_native.f32"),3840,2160,3);s.adaptation=load(root/(name+"_adaptation.f32"),3840,2160);unsigned w=3840,h=2160;
  for(unsigned b=1;b<=6;++b){if(b>2){w=(w+1)/2;h=(h+1)/2;}s.pyramid.bands.push_back({w,h,load(root/(name+"_band"+std::to_string(b)+".f32"),w,h)});}return s;};
 auto r=side("reference"),t=side("test");SingleFFTW fft(argv[3]);
 IQM::Bin::VulkanInstance instance(true,"",true);unsigned calls=0;
 SingleComparisonBackend gpu{[&](auto a,auto b){++calls;return comparison_single_gpu(instance,argv[2],a,b);}};
 unsigned mutualCalls=0;SingleMutualBackend gpuMutual{[&](auto a,auto b,unsigned w,unsigned h){++mutualCalls;return mutual_single_gpu(instance,argv[6],a,b,w,h);}};
 unsigned resizeCalls=0,resizeDispatches=0;SingleComparisonResizeBackend gpuResize{[&](auto mask,unsigned h,unsigned w,unsigned nh,unsigned nw){++resizeCalls;auto r=resize_comparison_gpu(instance,argv[7],mask,h,w,nh,nw);resizeDispatches+=r.dispatches;return std::move(r.values);}};
 auto cpu=evaluate_quality_single(r,t,3840,2160,std::stod(argv[4]),fft.backend());
 auto actual=evaluate_quality_single(r,t,3840,2160,std::stod(argv[4]),fft.backend(),&gpu,argc>=7?&gpuMutual:nullptr,argc==8?&gpuResize:nullptr);
 if(mutualCalls!=(argc>=7?6u:0u))throw std::runtime_error("Mutual dispatch count mismatch");
 if(resizeCalls!=(argc==8?6u:0u)||resizeDispatches!=(argc==8?8u:0u))throw std::runtime_error("Resize dispatch count mismatch");
 if(calls!=1||cpu.band_errors.size()!=actual.band_errors.size()||std::bit_cast<uint64_t>(cpu.base_frequency)!=std::bit_cast<uint64_t>(actual.base_frequency))throw std::runtime_error("Stage mismatch");
 for(size_t b=0;b<cpu.band_errors.size();++b)if(std::bit_cast<uint32_t>(cpu.band_errors[b])!=std::bit_cast<uint32_t>(actual.band_errors[b]))throw std::runtime_error("Band error mismatch");
 for(auto pair:{std::pair{cpu.scores.Q,actual.scores.Q},std::pair{cpu.scores.Q_JOD,actual.scores.Q_JOD}})if(std::bit_cast<uint32_t>(pair.first)!=std::bit_cast<uint32_t>(pair.second))throw std::runtime_error("Score mismatch");
 std::ofstream report(out/"acceptance.json");report<<"{\"exact\":true,\"scope\":\"GPU comparison/mutual providers in quality stage using persisted reference tensors, not independent end-to-end\",\"dispatches\":"<<calls<<",\"mutual_dispatches\":"<<mutualCalls<<",\"resize_calls\":"<<resizeCalls<<",\"resize_dispatches\":"<<resizeDispatches<<",\"Q_hex\":\""<<std::hex<<std::bit_cast<uint32_t>(actual.scores.Q)<<"\",\"Q_JOD_hex\":\""<<std::bit_cast<uint32_t>(actual.scores.Q_JOD)<<"\"}";
 std::cout<<"PASS six band errors, base frequency, Q and Q_JOD bitwise; comparison dispatches="<<calls<<" mutual dispatches="<<mutualCalls<<" resize dispatches="<<resizeDispatches<<'\n';return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
