#include "fft128_single_gpu.h"
#include <IQM/hdrvdp3/fftw_single_provider.h>
#include <vulkan_instance.h>
#include <bit>
#include <fstream>
#include <iostream>
#include <random>
#include <cmath>
int main(int argc,char**argv){try{
 if(argc!=4)throw std::runtime_error("Usage: shader fftw.dll fresh-out");std::filesystem::path out=argv[3];if(std::filesystem::exists(out)||!std::filesystem::create_directories(out))throw std::runtime_error("Fresh output required");
 IQM::Bin::VulkanInstance gpu(true,"",true);IQM::HDRVDP3::FFTWProviderSingle fft(argv[2]);std::mt19937 random(7128);std::ofstream rows(out/"results.jsonl");uint64_t total=0,mismatches=0;unsigned count=0;
 auto compare=[&](const std::string& label,const std::vector<float>& expected,const std::vector<float>& actual){if(expected.size()!=actual.size())throw std::runtime_error("Shape");size_t bits=0,numeric=0,first=0;for(size_t i=0;i<actual.size();++i){if(std::bit_cast<uint32_t>(expected[i])!=std::bit_cast<uint32_t>(actual[i])){if(!bits)first=i;++bits;}if(expected[i]!=actual[i])++numeric;}rows<<"{\"case\":\""<<label<<"\",\"values\":"<<actual.size()<<",\"bit_mismatches\":"<<bits<<",\"numeric_mismatches\":"<<numeric<<",\"first\":"<<first<<"}\n";rows.flush();std::cout<<label<<" bit="<<bits<<" numeric="<<numeric<<std::endl;if(bits){std::ofstream a(out/(label+"_expected.f32"),std::ios::binary),b(out/(label+"_actual.f32"),std::ios::binary);a.write(reinterpret_cast<const char*>(expected.data()),expected.size()*4);b.write(reinterpret_cast<const char*>(actual.data()),actual.size()*4);}total+=actual.size();mismatches+=bits;++count;};
 for(unsigned mode=0;mode<8;++mode){std::vector<float> input(128*128);for(size_t i=0;i<input.size();++i){if(mode==0)input[i]=1;else if(mode==1)input[i]=i==67?1:0;else if(mode==2)input[i]=float(int(i%137)-68)/31.f;else{const float scale=mode==3?.001f:mode==4?1.f:mode==5?10.f:mode==6?10000.f:1e-15f;input[i]=float(int(random()%20001)-10000)*scale;}}
 auto reference=fft.forward(input,128,128),actual=IQM::HDRVDP3::Experimental::fft128_gpu(gpu,argv[1],input,true);compare("forward_"+std::to_string(mode),reference,actual);
 IQM::HDRVDP3::FFTWProviderSingle::Memory src(fft,reference.size()*4),dst(fft,reference.size()*4);std::memcpy(src.data,reference.data(),reference.size()*4);fft.forget();auto plan=fft.complexPlan(128,128,static_cast<IQM::HDRVDP3::FFTWProviderSingle::Complex*>(src.data),static_cast<IQM::HDRVDP3::FFTWProviderSingle::Complex*>(dst.data),1,64|16|2);if(!plan)throw std::runtime_error("Inverse plan");fft.execute(plan);fft.destroy(plan);std::vector<float> expected(reference.size());const auto* values=static_cast<float*>(dst.data);for(size_t i=0;i<expected.size();++i)expected[i]=values[i]/16384.f;compare("inverse_"+std::to_string(mode),expected,IQM::HDRVDP3::Experimental::fft128_gpu(gpu,argv[1],reference,false));
 }
 std::ofstream(out/"summary.json")<<"{\"exact\":"<<(mismatches?"false":"true")<<",\"cases\":"<<count<<",\"values\":"<<total<<",\"bit_mismatches\":"<<mismatches<<",\"scope\":\"isolated128 FFT; not full metric or4K\"}";return mismatches?1:0;
 }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 2;}}
