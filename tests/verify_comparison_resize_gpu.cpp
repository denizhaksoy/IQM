#include <vulkan_instance.h>
#include <IQM/hdrvdp3/comparison_resize_gpu.h>
#include <IQM/hdrvdp3/resize.h>
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
namespace fs=std::filesystem;using namespace IQM::HDRVDP3;
std::vector<float> load(const fs::path& file,size_t n){if(fs::file_size(file)!=n*4)throw std::runtime_error("Native tensor size");std::vector<float> values(n);std::ifstream f(file,std::ios::binary);f.read(reinterpret_cast<char*>(values.data()),n*4);if(!f)throw std::runtime_error("Native tensor read");return values;}
int main(int argc,char**argv){try{
 if(argc!=3&&argc!=4)throw std::runtime_error("Usage: resize-shader fresh-out [4K-native-capture-root]");
 fs::path out=argv[2];if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 IQM::Bin::VulkanInstance instance(true,"",true);std::ofstream report(out/"results.jsonl");uint64_t total=0;unsigned cases=0;
 auto check=[&](const std::string& name,const std::vector<double>& mask,unsigned w,unsigned h,unsigned nw,unsigned nh){
  auto expected=resize_bicubic(mask,int(h),int(w),int(nh),int(nw));auto actual=resize_comparison_gpu(instance,argv[1],mask,h,w,nh,nw);size_t errors=0,first=0;
  if(actual.values.size()!=expected.size()||actual.dispatches!=unsigned(w!=nw)+unsigned(h!=nh))throw std::runtime_error("Resize shape/dispatch count");
  for(size_t i=0;i<expected.size();++i)if(std::bit_cast<uint64_t>(expected[i])!=std::bit_cast<uint64_t>(actual.values[i])){if(!errors)first=i;++errors;}
  report<<"{\"case\":\""<<name<<"\",\"width\":"<<w<<",\"height\":"<<h<<",\"target_width\":"<<nw<<",\"target_height\":"<<nh<<",\"values\":"<<expected.size()<<",\"bit_mismatches\":"<<errors<<",\"dispatches\":"<<actual.dispatches<<"}\n";report.flush();
  std::cout<<name<<": "<<errors<<" / "<<expected.size()<<'\n';if(errors){std::cerr<<"first="<<first<<" expected="<<std::hex<<std::bit_cast<uint64_t>(expected[first])<<" actual="<<std::bit_cast<uint64_t>(actual.values[first])<<'\n';throw std::runtime_error("Double resize bit mismatch");}total+=expected.size();++cases;
 };
 struct Shape{unsigned w,h,nw,nh;};std::mt19937 random(17017);
 for(auto s:{Shape{17,33,9,17},Shape{33,17,17,9},Shape{31,47,59,83},Shape{47,31,16,13},Shape{33,65,33,17},Shape{65,33,17,33},Shape{16,16,16,16}}){
  std::vector<double> mask(size_t(s.w)*s.h);for(auto& v:mask)v=double(random()&1u);
  const auto name=std::to_string(s.w)+"x"+std::to_string(s.h)+"_to_"+std::to_string(s.nw)+"x"+std::to_string(s.nh);
  check("binary_"+name,mask,s.w,s.h,s.nw,s.nh);mask.assign(mask.size(),0.);check("zero_"+name,mask,s.w,s.h,s.nw,s.nh);mask.assign(mask.size(),1.);check("one_"+name,mask,s.w,s.h,s.nw,s.nh);
 }
 if(argc==4){constexpr unsigned w=3840,h=2160;constexpr size_t n=size_t(w)*h;auto r=load(fs::path(argv[3])/"reference_native.f32",3*n),t=load(fs::path(argv[3])/"test_native.f32",3*n);std::vector<double> mask(n,0.);
  for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x)for(size_t c=0;c<3;++c){const size_t i=c*n+size_t(x)*h+y;if(std::abs(t[i]-r[i])/r[i]>.001f)mask[size_t(y)*w+x]=1.;}
  for(auto [nw,nh]:{std::pair{3840u,2160u},{1920u,1080u},{960u,540u},{480u,270u},{240u,135u}})check("real_mask_"+std::to_string(nw)+"x"+std::to_string(nh),mask,w,h,nw,nh);
 }
 std::ofstream(out/"acceptance.json")<<"{\"exact\":true,\"cases\":"<<cases<<",\"values\":"<<total<<",\"scope\":\"standalone Double binary comparison resize\"}";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
