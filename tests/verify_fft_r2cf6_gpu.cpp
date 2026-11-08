#include "fft_r2cf6_gpu.h"
#include <IQM/hdrvdp3/fftw_single_provider.h>
#include <vulkan_instance.h>
#include <bit>
#include <fstream>
#include <iostream>
#include <random>
int main(int argc,char**argv){try{
 if(argc!=4)throw std::runtime_error("Usage: shader fftw.dll fresh-out");std::filesystem::path out=argv[3];if(std::filesystem::exists(out)||!std::filesystem::create_directories(out))throw std::runtime_error("Fresh output required");IQM::Bin::VulkanInstance gpu(true,"",true);IQM::HDRVDP3::FFTWProviderSingle fft(argv[2]);auto realPlan=fft.symbol<void*(*)(int,float*,IQM::HDRVDP3::FFTWProviderSingle::Complex*,unsigned)>("fftwf_plan_dft_r2c_1d");auto print=fft.symbol<char*(*)(void*)>("fftwf_sprint_plan");IQM::HDRVDP3::FFTWProviderSingle::Memory input(fft,6*4),output(fft,4*8);fft.forget();void* plan=realPlan(6,static_cast<float*>(input.data),static_cast<IQM::HDRVDP3::FFTWProviderSingle::Complex*>(output.data),82);if(!plan)throw std::runtime_error("Oracle plan");char* printed=print(plan);if(!printed)throw std::runtime_error("Oracle plan text");std::string text=printed;std::ofstream(out/"oracle.plan.txt")<<text;if(text.find("r2cf_6")==std::string::npos)throw std::runtime_error("Oracle did not select r2cf_6");
 std::mt19937 random(7606);std::ofstream rows(out/"results.jsonl");uint64_t count=0,values=0,mismatches=0;
 for(unsigned batches:{1u,63u,64u,65u,513u,4096u}){std::vector<float> samples(6*batches),expected(8*batches);for(unsigned b=0;b<batches;++b){for(unsigned i=0;i<6;++i){const float scale=b%4==0?.001f:b%4==1?1.f:b%4==2?1e4f:1e-20f;samples[6*b+i]=b==0?1.f:b==1?(i==2?1.f:0.f):b==2?float(int(i)-3)/7.f:float(int(random()%20001)-10000)*scale;}std::memcpy(input.data,samples.data()+6*b,6*4);fft.execute(plan);std::memcpy(expected.data()+8*b,output.data,4*8);}
 auto actual=IQM::HDRVDP3::fft_r2cf6_gpu(gpu,argv[1],samples);if(actual.size()!=expected.size())throw std::runtime_error("Shape");size_t bits=0;for(size_t i=0;i<expected.size();++i)if(std::bit_cast<uint32_t>(expected[i])!=std::bit_cast<uint32_t>(actual[i])){if(bits<4)std::cout<<"i="<<i<<" expected="<<std::hex<<std::bit_cast<uint32_t>(expected[i])<<" actual="<<std::bit_cast<uint32_t>(actual[i])<<std::dec<<std::endl;++bits;}rows<<"{\"batches\":"<<batches<<",\"bit_mismatches\":"<<bits<<"}\n";rows.flush();std::cout<<"r2cf6 batches="<<batches<<" bit="<<bits<<std::endl;count+=batches;values+=actual.size();mismatches+=bits;
 }fft.destroy(plan);std::ofstream(out/"summary.json")<<"{\"exact\":"<<(mismatches?"false":"true")<<",\"transforms\":"<<count<<",\"values\":"<<values<<",\"bit_mismatches\":"<<mismatches<<",\"scope\":\"isolated r2cf6 leaf, not4KFFT\"}";return mismatches?1:0;
 }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 2;}}
