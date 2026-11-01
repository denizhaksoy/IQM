// Isolate Double LMSR and local-adaptation scalar arithmetic from FFT errors.
#include <IQM/hdrvdp3/spectral.h>
#include <array>
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace fs=std::filesystem;
template<class T>std::vector<T> read(const fs::path& p,size_t count){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()!=std::streamoff(count*sizeof(T)))throw std::runtime_error("Capture shape "+p.string());std::vector<T> v(count);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),count*sizeof(T));return v;}
int main(int argc,char**argv){try{
 if(argc!=4)throw std::runtime_error("Usage: native raw-data fresh-output");fs::path native=argv[1],data=argv[2],out=argv[3];if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 constexpr size_t n=9216;size_t matrixErrors=0,logErrors=0,expErrors=0;std::array<size_t,4> errors{};
 for(std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"}){
  const auto samples=IQM::HDRVDP3::load_rgb_spectral_samples(data/"log_cone_smith_pokorny_1975.csv",data/"cie_scotopic_lum.txt",data/("emission_spectra_"+display+".csv"));const auto calibration=IQM::HDRVDP3::calibrate_rgb_spectral_age(samples,24);
  for(int side=1;side<=2;++side){const auto prefix=(native/(display+"_"+std::to_string(side))).string();auto optical=read<double>(prefix+"_optical.f64",3*n);auto matrix=read<double>(prefix+"_matrix.f64",12);auto response=read<double>(prefix+"_response.f64",4*n);auto adaptationInput=read<double>(prefix+"_adaptation_input.f64",n);auto logged=read<double>(prefix+"_conv4_input.f64",n);auto convolved=read<double>(prefix+"_conv4_output.f64",n);auto adaptation=read<double>(prefix+"_adaptation.f64",n);
   for(size_t c=0;c<4;++c)for(size_t k=0;k<3;++k)matrixErrors+=std::bit_cast<uint64_t>(calibration.matrix[k][c])!=std::bit_cast<uint64_t>(matrix[c*3+k]);
   for(size_t c=0;c<4;++c)for(size_t i=0;i<n;++i){double x=optical[i],y=optical[n+i],z=optical[2*n+i],r=calibration.matrix[0][c],g=calibration.matrix[1][c],b=calibration.matrix[2][c];double p=x*r,q=y*g,s=z*b;std::array<double,4> candidates{(p+q)+s,p+(q+s),std::fma(z,b,std::fma(y,g,p)),std::fma(x,r,std::fma(y,g,s))};for(size_t k=0;k<4;++k)errors[k]+=std::bit_cast<uint64_t>(candidates[k])!=std::bit_cast<uint64_t>(response[c*n+i]);}
   for(size_t i=0;i<n;++i){logErrors+=std::bit_cast<uint64_t>(std::log(std::max(adaptationInput[i],1e-6)))!=std::bit_cast<uint64_t>(logged[i]);expErrors+=std::bit_cast<uint64_t>(std::exp(convolved[i]))!=std::bit_cast<uint64_t>(adaptation[i]);}
  }
 }
 std::ofstream log(out/"results.txt");log<<"Independent LMSR matrix bit errors "<<matrixErrors<<" /72\n";for(size_t k=0;k<4;++k)log<<"projection candidate "<<k<<" bit errors "<<errors[k]<<" /221184\n";log<<"MSVC log errors "<<logErrors<<" /55296; exp errors "<<expErrors<<" /55296\n";log.close();std::cout<<std::ifstream(out/"results.txt").rdbuf();return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
