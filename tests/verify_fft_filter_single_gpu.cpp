#include "fft_filter_single_gpu.h"
#include <vulkan_instance.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <bit>
#include <cmath>
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("Usage: shader fresh-out");std::filesystem::path out=argv[2];if(std::filesystem::exists(out)||!std::filesystem::create_directories(out))throw std::runtime_error("Fresh output required");IQM::Bin::VulkanInstance gpu(true,"",true);std::mt19937 random(7182);uint64_t total=0;std::ofstream rows(out/"results.jsonl");
 for(unsigned n:{1u,33u,65u,16384u,100003u}){std::vector<float> input(2*n);std::vector<double> coefficients(n);for(float& v:input){uint32_t bits=random();if((bits&0x7f800000u)==0x7f800000u)bits^=0x00800000u;v=std::bit_cast<float>(bits);}for(unsigned i=0;i<n;++i){const float base=std::bit_cast<float>(uint32_t(random()%0x3f800001u));coefficients[i]=std::nextafter(double(base),i%2?1.:0.);if(i%3==0)coefficients[i]=-coefficients[i];}coefficients[0]=std::bit_cast<double>(uint64_t(0x8000000000000000ull));if(n>2){coefficients[1]=std::ldexp(1.,-150);coefficients[2]=std::ldexp(3.,-150);input[2]=1.f;input[3]=-1.f;}
 const auto actual=IQM::HDRVDP3::fft_filter_single_gpu(gpu,argv[1],input,coefficients);if(actual.size()!=input.size())throw std::runtime_error("Shape");size_t mismatches=0;for(size_t i=0;i<input.size();++i){float coefficient=float(coefficients[i/2]),expected=input[i]*coefficient;if(std::bit_cast<uint32_t>(expected)!=std::bit_cast<uint32_t>(actual[i])){if(mismatches<4)std::cout<<"mismatch i="<<i<<" expected="<<std::hex<<std::bit_cast<uint32_t>(expected)<<" actual="<<std::bit_cast<uint32_t>(actual[i])<<std::dec<<std::endl;++mismatches;}}
 rows<<"{\"complex_pixels\":"<<n<<",\"mismatches\":"<<mismatches<<"}\n";rows.flush();if(mismatches)throw std::runtime_error("FFT filter mismatch");total+=input.size();std::cout<<"EXACT "<<n<<std::endl;
 }
 std::ofstream(out/"summary.json")<<"{\"exact\":true,\"cases\":5,\"values\":"<<total<<",\"scope\":\"isolated complex/filter multiply only\"}";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
