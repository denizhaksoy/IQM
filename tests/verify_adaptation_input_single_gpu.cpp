#include <vulkan_instance.h>
#include <IQM/hdrvdp3/adaptation_input_single_gpu.h>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <cmath>
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("Usage: shader fresh-out");std::filesystem::path out=argv[2];if(std::filesystem::exists(out)||!std::filesystem::create_directories(out))throw std::runtime_error("Fresh output required");
 IQM::Bin::VulkanInstance instance(true,"",true);std::ofstream report(out/"results.jsonl");std::mt19937 random(7011);uint64_t total=0;
 for(unsigned n:{1u,63u,64u,65u,127u,128u,129u,100003u,3840u*2160u}){
  std::vector<float> l(n),m(n);const uint32_t low=std::bit_cast<uint32_t>(1e-8f),high=std::bit_cast<uint32_t>(1e10f);
  for(unsigned i=0;i<n;++i){l[i]=std::bit_cast<float>(low+random()%(high-low+1));m[i]=std::bit_cast<float>(low+random()%(high-low+1));}
  l[0]=1e-8f;m[0]=1e10f;if(n>1){l[1]=1e10f;m[1]=1e10f;}
  auto actual=IQM::HDRVDP3::adaptation_input_single_gpu(instance,argv[1],l,m);if(actual.size()!=n)throw std::runtime_error("Shape");
  for(unsigned i=0;i<n;++i){float expected=l[i]+m[i];if(std::bit_cast<uint32_t>(expected)!=std::bit_cast<uint32_t>(actual[i]))throw std::runtime_error("Bit mismatch at "+std::to_string(i));}
  total+=n;report<<"{\"pixels\":"<<n<<",\"exact\":true}\n";report.flush();std::cout<<"EXACT "<<n<<std::endl;
 }
 bool rejected=false;try{std::vector<float> l{0.f},m{1.f};IQM::HDRVDP3::adaptation_input_single_gpu(instance,argv[1],l,m);}catch(const std::invalid_argument&){rejected=true;}if(!rejected)throw std::runtime_error("Invalid domain not rejected");
 std::ofstream(out/"summary.json")<<"{\"exact\":true,\"values\":"<<total<<",\"cases\":9,\"scope\":\"standalone L+M only, not full quality\"}";
 return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
