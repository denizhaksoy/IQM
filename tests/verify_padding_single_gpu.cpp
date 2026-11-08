#include <vulkan_instance.h>
#include <IQM/hdrvdp3/padding_single_gpu.h>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("Usage: shader fresh-out");std::filesystem::path out=argv[2];if(std::filesystem::exists(out)||!std::filesystem::create_directories(out))throw std::runtime_error("Fresh output required");
 IQM::Bin::VulkanInstance instance(true,"",true);std::mt19937 random(7062);std::ofstream rows(out/"results.jsonl");uint64_t total=0;unsigned cases=0;
 for(auto [w,h]:{std::pair{1u,1u},{1u,65u},{65u,1u},{17u,29u},{29u,17u},{80u,96u},{96u,80u},{127u,95u},{95u,127u},{3840u,2160u}}){
  std::vector<float> values(size_t(w)*h);for(float& v:values){uint32_t bits=random();if((bits&0x7f800000u)==0x7f800000u)bits^=0x00800000u;v=std::bit_cast<float>(bits);}values[0]=std::bit_cast<float>(0x80000000u);if(values.size()>1)values[1]=std::bit_cast<float>(1u);
  for(bool symmetric:{false,true}){auto actual=IQM::HDRVDP3::padding_single_gpu(instance,argv[1],values,w,h,symmetric);if(actual.size()!=4*values.size())throw std::runtime_error("Shape");
   for(unsigned x=0;x<2*w;++x)for(unsigned y=0;y<2*h;++y){unsigned sx=x<w?x:(symmetric?2*w-1-x:w-1),sy=y<h?y:(symmetric?2*h-1-y:h-1);if(std::bit_cast<uint32_t>(actual[size_t(x)*(2*h)+y])!=std::bit_cast<uint32_t>(values[size_t(sy)*w+sx]))throw std::runtime_error("Padding bit mismatch");}
   total+=actual.size();++cases;rows<<"{\"width\":"<<w<<",\"height\":"<<h<<",\"symmetric\":"<<(symmetric?"true":"false")<<",\"exact\":true}\n";rows.flush();std::cout<<"EXACT "<<w<<"x"<<h<<" symmetric="<<symmetric<<std::endl;
  }
 }
 std::ofstream(out/"summary.json")<<"{\"exact\":true,\"cases\":"<<cases<<",\"values\":"<<total<<",\"scope\":\"standalone padding/layout only\"}";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
