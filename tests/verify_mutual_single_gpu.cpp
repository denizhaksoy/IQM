#include <vulkan_instance.h>
#include <IQM/hdrvdp3/mutual_single_gpu.h>
#include <IQM/hdrvdp3/general_quality_single.h>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
namespace fs=std::filesystem;using namespace IQM::HDRVDP3;
std::vector<float> load(const fs::path& file,unsigned w,unsigned h){const size_t n=size_t(w)*h;if(fs::file_size(file)!=n*4)throw std::runtime_error("Band size");std::vector<float> col(n),row(n);std::ifstream f(file,std::ios::binary);f.read(reinterpret_cast<char*>(col.data()),n*4);if(!f)throw std::runtime_error("Band read");for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x)row[size_t(y)*w+x]=col[size_t(x)*h+y];return row;}
int main(int argc,char**argv){try{
 if(argc!=3&&argc!=4)throw std::runtime_error("Usage: shader fresh-out [4K-reference-capture-root]");
 fs::path out=argv[2];if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 IQM::Bin::VulkanInstance instance(true,"",true);std::ofstream report(out/"results.jsonl");uint64_t total=0;unsigned cases=0;
 auto check=[&](const std::string& name,const std::vector<float>& r,const std::vector<float>& t,unsigned w,unsigned h){
  auto expected=mutual_mask_single(r,t,w,h),actual=mutual_single_gpu(instance,argv[1],r,t,w,h);size_t mismatches=0,first=0;
  for(size_t i=0;i<actual.size();++i)if(std::bit_cast<uint32_t>(actual[i])!=std::bit_cast<uint32_t>(expected[i])){if(!mismatches)first=i;++mismatches;}
  report<<"{\"case\":\""<<name<<"\",\"width\":"<<w<<",\"height\":"<<h<<",\"values\":"<<actual.size()<<",\"bit_mismatches\":"<<mismatches<<"}\n";report.flush();
  std::cout<<name<<": "<<mismatches<<" / "<<actual.size()<<'\n';if(mismatches){std::cerr<<"first="<<first<<" cpu="<<std::hex<<std::bit_cast<uint32_t>(expected[first])<<" gpu="<<std::bit_cast<uint32_t>(actual[first])<<'\n';throw std::runtime_error("Mutual mask mismatch");}total+=actual.size();++cases;
 };
 std::mt19937 random(17013);
 for(auto [w,h]:{std::pair{1u,1u},{1u,65u},{65u,1u},{17u,15u},{15u,17u},{31u,32u},{32u,31u},{33u,33u},{63u,65u},{65u,63u},{64u,64u}}){
  std::vector<float> r(size_t(w)*h),t(r.size());for(size_t i=0;i<r.size();++i){r[i]=std::bit_cast<float>(uint32_t(0x30000000u+random()%0x18000000u))*(i%2?-1.f:1.f);t[i]=std::bit_cast<float>(uint32_t(0x30000000u+random()%0x18000000u))*(i%3?-1.f:1.f);}
  check("normal_"+std::to_string(w)+"x"+std::to_string(h),r,t,w,h);
  r.assign(r.size(),0.f);t=r;check("zero_"+std::to_string(w)+"x"+std::to_string(h),r,t,w,h);
 }
 for(auto [w,h]:{std::pair{257u,193u},{193u,257u}}){
  std::vector<float> r(size_t(w)*h),t(r.size());for(size_t i=0;i<r.size();++i){r[i]=std::bit_cast<float>(uint32_t(1u+random()%0x7f7fffffu))*(i%2?-1.f:1.f);t[i]=std::bit_cast<float>(uint32_t(1u+random()%0x7f7fffffu))*(i%3?-1.f:1.f);}check("all_exponents_"+std::to_string(w)+"x"+std::to_string(h),r,t,w,h);
 }
 for(uint32_t bits:{0x007fffffu,0x00800000u,0x00800001u,0x7f7fffffu}){std::vector<float> v(33*33,std::bit_cast<float>(bits));check("edge_bits_"+std::to_string(bits),v,v,33,33);}
 if(argc==4){unsigned w=3840,h=2160;for(unsigned b=1;b<=6;++b){if(b>2){w=(w+1)/2;h=(h+1)/2;}check("real_band"+std::to_string(b),load(fs::path(argv[3])/("reference_band"+std::to_string(b)+".f32"),w,h),load(fs::path(argv[3])/("test_band"+std::to_string(b)+".f32"),w,h),w,h);}}
 // Keep IEEE subnormal coverage explicit: a failure prevents acceptance.
 std::vector<float> tiny(33*33);for(size_t i=0;i<tiny.size();++i)tiny[i]=std::bit_cast<float>(uint32_t(1+i%0xffffff));check("subnormal_probe",tiny,tiny,33,33);
 std::ofstream(out/"acceptance.json")<<"{\"exact\":true,\"values\":"<<total<<",\"cases\":"<<cases<<",\"scope\":\"standalone mutual activity only\"}";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
