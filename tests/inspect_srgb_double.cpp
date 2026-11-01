#include <IQM/hdrvdp3/spectral.h>
#include <array>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace fs=std::filesystem;
template<class T> std::vector<T> read(const fs::path& p,size_t count){
 std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()!=std::streamoff(count*sizeof(T)))throw std::runtime_error("Invalid capture "+p.string());
 std::vector<T> v(count);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),count*sizeof(T));if(!f)throw std::runtime_error("Read failure");return v;
}
int main(int argc,char** argv){try{
 if(argc!=4)throw std::runtime_error("Usage: native raw-data fresh-output");
 fs::path native=argv[1],data=argv[2],out=argv[3];if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 constexpr size_t n=9216;std::array<size_t,4> eErrors{},mErrors{};size_t matrixErrors=0;
 std::ofstream log(out/"candidates.tsv");log<<"display\tside\te_single\te_double\te_pow_double\te_fma\tm_left\tm_right\tm_fma_forward\tm_fma_reverse\n";
 for(std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"}){
  auto samples=IQM::HDRVDP3::load_xyz_display_samples(data/"ciexyz31.csv",data/("emission_spectra_"+display+".csv"));auto matrix=IQM::HDRVDP3::rgb709_transform(samples);
  for(int side=1;side<=2;++side){auto prefix=(native/(display+"_"+std::to_string(side))).string();auto input=read<float>(prefix+"_input.f32",3*n);auto expected=read<double>(prefix+"_eotf.f64",3*n);auto raw=read<double>(prefix+"_raw.f64",3*n);auto captured=read<double>(prefix+"_matrix.f64",9);std::array<size_t,4> eLocal{},mLocal{};
   for(size_t c=0;c<3;++c)for(size_t k=0;k<3;++k)matrixErrors+=std::bit_cast<uint64_t>(matrix[c][k])!=std::bit_cast<uint64_t>(captured[c+3*k]);
   for(size_t i=0;i<3*n;++i){float x=input[i];float base=(x+float(.055))/float(1.055);float low=x/float(12.92);float high=std::pow(base,float(2.4));
    double s=x<=.04045?low:high;double d=x<=.04045?double(x)/12.92:std::pow((double(x)+.055)/1.055,2.4);double pd=x<=.04045?low:float(std::pow(double(base),double(float(2.4))));
    std::array<double,4> candidates{99*s+1,99*d+1,99*pd+1,std::fma(99.,s,1.)};for(size_t k=0;k<4;++k)eLocal[k]+=std::bit_cast<uint64_t>(candidates[k])!=std::bit_cast<uint64_t>(expected[i]);
   }
   // Captured EOTF isolates matrix arithmetic; this is characterization only.
   for(size_t c=0;c<3;++c)for(size_t i=0;i<n;++i){double x=expected[i],y=expected[n+i],z=expected[2*n+i],r=matrix[c][0],g=matrix[c][1],b=matrix[c][2];double p=x*r,q=y*g,s=z*b;
    std::array<double,4> candidates{(p+q)+s,p+(q+s),std::fma(z,b,std::fma(y,g,p)),std::fma(x,r,std::fma(y,g,s))};for(size_t k=0;k<4;++k)mLocal[k]+=std::bit_cast<uint64_t>(candidates[k])!=std::bit_cast<uint64_t>(raw[c*n+i]);
   }
   log<<display<<'\t'<<side;for(size_t k=0;k<4;++k){eErrors[k]+=eLocal[k];log<<'\t'<<eLocal[k];}for(size_t k=0;k<4;++k){mErrors[k]+=mLocal[k];log<<'\t'<<mLocal[k];}log<<'\n';
  }
 }
 std::cout<<"Independent matrix bit errors "<<matrixErrors<<"\n";for(size_t k=0;k<4;++k)std::cout<<"candidate "<<k<<" EOTF errors "<<eErrors[k]<<" matrix errors "<<mErrors[k]<<" /165888\n";
 std::ofstream(out/"scope.txt")<<"CPU arithmetic characterization only;matrix evaluated using captured EOTF,not independent preprocessing acceptance.\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
