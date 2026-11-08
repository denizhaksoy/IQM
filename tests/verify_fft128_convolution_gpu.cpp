#include "fft128_single_provider_gpu.h"
#include "fft128_single_gpu.h"
#include "fft_filter_single_gpu.h"
#include <IQM/hdrvdp3/fftw_single_provider.h>
#include <vulkan_instance.h>
#include <bit>
#include <fstream>
#include <iostream>
#include <random>
#include <cmath>
int main(int argc,char**argv){try{
 if(argc!=6)throw std::runtime_error("Usage: fft.spv filter.spv crop.spv fftw.dll fresh-out");std::filesystem::path out=argv[5];if(std::filesystem::exists(out)||!std::filesystem::create_directories(out))throw std::runtime_error("Fresh output required");IQM::Bin::VulkanInstance gpu(true,"",true);IQM::HDRVDP3::FFTWProviderSingle fft(argv[4]);std::mt19937 random(7129);std::ofstream rows(out/"results.jsonl");uint64_t total=0,mismatches=0;
 for(unsigned mode=0;mode<4;++mode){std::vector<float> input(128*128);std::vector<double> filter(input.size());for(size_t i=0;i<input.size();++i){input[i]=mode==0?1.f:mode==1?(i==67?1.f:0.f):float(int(random()%20001)-10000)/71.f;const double x=double(i/128)/128.,y=double(i%128)/128.;filter[i]=mode==0?1.:mode==1?0.:mode==2?std::exp(-100.*(x*x+y*y)):std::exp(-500.*(x*x+y*y));}
 const unsigned w=mode==0?128:mode==1?64:mode==2?63:64,h=mode==0?128:mode==1?64:mode==2?65:64;
 auto expected=fft.convolve(input,filter,128,128,w,h);IQM::HDRVDP3::Experimental::FFT128ProviderGPU provider{gpu,argv[1],argv[2],argv[3]};auto actual=provider.backend().convolve(input,filter,128,128,w,h);if(actual.size()!=expected.size())throw std::runtime_error("Shape");size_t bits=0;for(size_t i=0;i<actual.size();++i)if(std::bit_cast<uint32_t>(actual[i])!=std::bit_cast<uint32_t>(expected[i]))++bits;rows<<"{\"mode\":"<<mode<<",\"values\":"<<actual.size()<<",\"bit_mismatches\":"<<bits<<"}\n";rows.flush();std::cout<<"convolution mode="<<mode<<" bits="<<bits<<std::endl;total+=actual.size();mismatches+=bits;
 }
 bool rejected=false;try{IQM::HDRVDP3::Experimental::FFT128ProviderGPU provider{gpu,argv[1],argv[2],argv[3]};std::vector<float> wrong(64*64);provider.backend().forward(wrong,64,64);}catch(const std::invalid_argument&){rejected=true;}if(!rejected)throw std::runtime_error("Unsupported FFT shape must fail");
 std::ofstream(out/"summary.json")<<"{\"exact\":"<<(mismatches?"false":"true")<<",\"cases\":4,\"values\":"<<total<<",\"bit_mismatches\":"<<mismatches<<",\"scope\":\"GPU forward/filter/inverse/crop128 only; model coefficients supplied by CPU\"}";return mismatches?1:0;
 }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 2;}}
