#include <array>
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace fs=std::filesystem;
template<class T> std::vector<T> read(const fs::path& p,size_t count){
 std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()!=std::streamoff(count*sizeof(T)))throw std::runtime_error("Invalid capture: "+p.string());
 std::vector<T> v(count);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),count*sizeof(T));if(!f)throw std::runtime_error("Read failure");return v;
}
int main(int argc,char** argv){try{
 if(argc!=3)throw std::runtime_error("Usage: native-captures fresh-output");
 fs::path input(argv[1]),out(argv[2]);if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 constexpr size_t n=96*96;std::array<size_t,5> errors{};
 std::ofstream log(out/"candidates.tsv");log<<"display\tside\tseparate_left\tseparate_right\tfma_forward\tfma_reverse\tdouble_accum\n";
 for(std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"})for(int side=1;side<=2;++side){
  auto prefix=input/(display+"_"+std::to_string(side));auto a=read<float>(prefix.string()+"_optical.f32",3*n);auto m=read<double>(prefix.string()+"_matrix.f64",12);auto expected=read<float>(prefix.string()+"_response.f32",4*n);std::array<size_t,5> local{};
  for(size_t ch=0;ch<4;++ch)for(size_t i=0;i<n;++i){
   float x=a[i],y=a[n+i],z=a[2*n+i],r=float(m[3*ch]),g=float(m[3*ch+1]),b=float(m[3*ch+2]);
   float p=x*r,q=y*g,s=z*b;
   std::array<float,5> values{(p+q)+s,p+(q+s),std::fma(z,b,std::fma(y,g,p)),std::fma(x,r,std::fma(y,g,s)),float((double(x)*m[3*ch]+double(y)*m[3*ch+1])+double(z)*m[3*ch+2])};
   for(size_t k=0;k<5;++k)if(std::bit_cast<uint32_t>(values[k])!=std::bit_cast<uint32_t>(expected[ch*n+i]))++local[k];
  }
  log<<display<<'\t'<<side;for(size_t k=0;k<5;++k){errors[k]+=local[k];log<<'\t'<<local[k];}log<<'\n';
 }
 for(size_t k=0;k<5;++k)std::cout<<"candidate"<<k<<" bit_errors="<<errors[k]<<" /221184\n";
 std::ofstream(out/"scope.txt")<<"Arithmetic characterization only. Captured native optical inputs;not an independent image-to-score or GPU test.\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
