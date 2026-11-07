#include <vulkan_instance.h>
#include <IQM/hdrvdp3/comparison_single_gpu.h>
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
namespace fs=std::filesystem;
std::vector<float> load(const fs::path& file){auto bytes=fs::file_size(file);if(bytes%4)throw std::runtime_error("Tensor length");std::vector<float> v(bytes/4);std::ifstream f(file,std::ios::binary);f.read(reinterpret_cast<char*>(v.data()),bytes);if(!f)throw std::runtime_error("Tensor read");return v;}
int main(int argc,char**argv){try{
 if(argc!=3&&argc!=5)throw std::runtime_error("Usage: shader fresh-out [reference-native.f32 test-native.f32]; optional tensors planar, identical layout");
 const fs::path out=argv[2];if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 IQM::Bin::VulkanInstance instance(true,"",true);
 std::ofstream report(out/"results.jsonl");uint64_t total=0;
 auto check=[&](const char* label,const std::vector<float>& r,const std::vector<float>& t){
  auto actual=IQM::HDRVDP3::comparison_single_gpu(instance,argv[1],r,t);const size_t n=r.size()/3;size_t mismatches=0;
  for(size_t i=0;i<n;++i){float expected=0;for(size_t c=0;c<3;++c){float delta=t[c*n+i]-r[c*n+i];float ratio=std::abs(delta)/r[c*n+i];if(ratio>.001f)expected=1;}mismatches+=std::bit_cast<uint32_t>(expected)!=std::bit_cast<uint32_t>(actual[i]);}
  report<<"{\"case\":\""<<label<<"\",\"pixels\":"<<n<<",\"bit_mismatches\":"<<mismatches<<"}\n";report.flush();std::cout<<label<<": "<<mismatches<<" / "<<n<<'\n';if(mismatches)throw std::runtime_error("Mask mismatch");total+=n;
 };
 // Each channel/sign gets its own near-threshold pixel. Sweep many Float32
 // exponents/significands and adjacent test values, including identical RGB.
 std::vector<float> refs,tests;std::mt19937 random(17011);
 for(unsigned k=0;k<100000;++k){
  float r=std::bit_cast<float>(uint32_t(0x358637bdu+random()%(0x7f000000u-0x358637bdu)));
  if(r<1e-6f||!std::isfinite(r))continue;
  float threshold=r*.001f;
  for(int sign:{-1,1}){float center=r+sign*threshold;for(int offset=-2;offset<=2;++offset){float t=center;for(int j=0;j<std::abs(offset);++j)t=std::nextafter(t,offset<0?0.f:std::numeric_limits<float>::infinity());if(!std::isfinite(t)||t<1e-6f)continue;refs.push_back(r);tests.push_back(t);}}
 }
 const size_t n=refs.size();std::vector<float> r(3*n),t(3*n);
 for(size_t i=0;i<n;++i)for(size_t c=0;c<3;++c){r[c*n+i]=refs[i];t[c*n+i]=(c==i%3)?tests[i]:refs[i];}
 check("threshold_neighbours",r,t);
 r.assign(3*65,1.f);t=r;check("identical_tail",r,t);
 r.assign(3*65,1e-6f);t.assign(3*65,std::numeric_limits<float>::max());check("extremes_tail",r,t);
 if(argc==5)check("real_native",load(argv[3]),load(argv[4]));
 std::ofstream(out/"summary.json")<<"{\"exact\":true,\"pixels\":"<<total<<",\"scope\":\"standalone mask only\"}";
 return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
