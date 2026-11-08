#include "fft_n1_4_gpu.h"
#include <IQM/hdrvdp3/fftw_single_provider.h>
#include <vulkan_instance.h>
#include <bit>
#include <fstream>
#include <iostream>
#include <random>
int main(int argc,char**argv){try{
 if(argc!=4)throw std::runtime_error("Usage: shader fftw.dll fresh-out");std::filesystem::path out=argv[3];if(std::filesystem::exists(out)||!std::filesystem::create_directories(out))throw std::runtime_error("Fresh output required");IQM::Bin::VulkanInstance gpu(true,"",true);IQM::HDRVDP3::FFTWProviderSingle fft(argv[2]);using Complex=IQM::HDRVDP3::FFTWProviderSingle::Complex;auto make=fft.symbol<void*(*)(int,Complex*,Complex*,int,unsigned)>("fftwf_plan_dft_1d");auto print=fft.symbol<char*(*)(void*)>("fftwf_sprint_plan");IQM::HDRVDP3::FFTWProviderSingle::Memory input(fft,8*4),output(fft,8*4);std::mt19937 random(7404);std::ofstream rows(out/"results.jsonl");uint64_t count=0,values=0,mismatches=0;
 for(bool inverse:{false,true}){fft.forget();void* plan=make(4,static_cast<Complex*>(input.data),static_cast<Complex*>(output.data),inverse?1:-1,82);if(!plan)throw std::runtime_error("Oracle plan");char* p=print(plan);if(!p||std::string(p).find("n1_4")==std::string::npos)throw std::runtime_error("Wrong oracle codelet");std::ofstream(out/(inverse?"inverse.plan.txt":"forward.plan.txt"))<<p;
 for(unsigned batches:{1u,63u,64u,65u,513u,4096u}){std::vector<float> samples(8*batches),expected(samples.size());for(unsigned b=0;b<batches;++b){for(unsigned i=0;i<8;++i){const float scale=b%4==0?.001f:b%4==1?1.f:b%4==2?1e4f:1e-20f;samples[8*b+i]=b==0?1.f:b==1?(i==2?1.f:0.f):b==2?float(int(i)-3)/7.f:float(int(random()%20001)-10000)*scale;}std::memcpy(input.data,samples.data()+8*b,8*4);fft.execute(plan);std::memcpy(expected.data()+8*b,output.data,8*4);}
 const auto actual=IQM::HDRVDP3::fft_n1_4_gpu(gpu,argv[1],samples,inverse);if(actual.size()!=expected.size())throw std::runtime_error("Shape");size_t bits=0;for(size_t i=0;i<expected.size();++i)if(std::bit_cast<uint32_t>(expected[i])!=std::bit_cast<uint32_t>(actual[i]))++bits;rows<<"{\"inverse\":"<<(inverse?"true":"false")<<",\"batches\":"<<batches<<",\"bit_mismatches\":"<<bits<<"}\n";rows.flush();std::cout<<"n1_4 inverse="<<inverse<<" batches="<<batches<<" bits="<<bits<<std::endl;count+=batches;values+=actual.size();mismatches+=bits;
 }fft.destroy(plan);}
 std::ofstream(out/"summary.json")<<"{\"exact\":"<<(mismatches?"false":"true")<<",\"transforms\":"<<count<<",\"values\":"<<values<<",\"bit_mismatches\":"<<mismatches<<",\"scope\":\"isolated n1_4 leaf, not4KFFT\"}";return mismatches?1:0;
 }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 2;}}
