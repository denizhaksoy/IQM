#include <vulkan_instance.h>
#include "diagnostic_float_kernel.h"
#include <iostream>
namespace fs=std::filesystem;
template<class T> std::vector<T> load(const fs::path& p,size_t count){
 std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()!=std::streamoff(count*sizeof(T)))throw std::runtime_error("Invalid capture "+p.string());
 std::vector<T> values(count);f.seekg(0);f.read(reinterpret_cast<char*>(values.data()),count*sizeof(T));if(!f)throw std::runtime_error("Read failure");return values;
}
int main(int argc,char** argv){try{
 if(argc!=4)throw std::runtime_error("Usage: native-captures shader fresh-output");
 fs::path root(argv[1]),out(argv[3]);if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 IQM::Bin::VulkanInstance instance(true);constexpr uint32_t n=96*96;size_t total=0;
 std::ofstream results(out/"results.tsv");results<<"display\tside\tvalues\tbit_errors\n";
 for(std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"})for(int side=1;side<=2;++side){
  auto prefix=root/(display+"_"+std::to_string(side));auto optical=load<float>(prefix.string()+"_optical.f32",3*n);auto matrix=load<double>(prefix.string()+"_matrix.f64",12);auto expected=load<float>(prefix.string()+"_response.f32",4*n);
  for(double v:matrix)optical.push_back(float(v));
  auto actual=runFloatKernel(instance,argv[2],optical,4*n,4*n,n);
  size_t errors=0;for(size_t i=0;i<actual.size();++i)if(std::bit_cast<uint32_t>(actual[i])!=std::bit_cast<uint32_t>(expected[i]))++errors;
  results<<display<<'\t'<<side<<'\t'<<actual.size()<<'\t'<<errors<<'\n';results.flush();
  std::ofstream dump(out/(display+"_"+std::to_string(side)+".f32"),std::ios::binary);dump.write(reinterpret_cast<const char*>(actual.data()),actual.size()*4);
  if(errors)throw std::runtime_error("GPU LMSR bits differ: "+std::to_string(errors));total+=actual.size();
 }
 std::ofstream(out/"PASS.txt")<<total<<" Float32 words bitwise exact. Native optical inputs;projection-only GPU acceptance,not full RGB image-to-score.\n";
 std::cout<<"RGB GPU projection PASS: "<<total<<" Float32 words bitwise\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
