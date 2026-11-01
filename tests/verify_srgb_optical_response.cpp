#include <IQM/hdrvdp3/srgb_display.h>
#include <bit>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace fs=std::filesystem;
std::vector<double> read(const fs::path&p,size_t n){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()!=std::streamoff(n*8))throw std::runtime_error("Invalid capture "+p.string());std::vector<double>v(n);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),n*8);return v;}
int main(int argc,char**argv){try{
 if(argc!=4)throw std::runtime_error("Usage: native raw-data fresh-output");fs::path native=argv[1],data=argv[2],out=argv[3];if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");size_t count=0,rejected=0;constexpr size_t n=9216;
 for(std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"}){
  auto samples=IQM::HDRVDP3::load_rgb_spectral_samples(data/"log_cone_smith_pokorny_1975.csv",data/"cie_scotopic_lum.txt",data/("emission_spectra_"+display+".csv"));
  for(int side=1;side<=2;++side){auto prefix=(native/(display+"_"+std::to_string(side))).string();auto optical=read(prefix+"_optical.f64",3*n);auto actual=IQM::HDRVDP3::project_optical_rgb_double(optical,samples,24);auto filtered=read(prefix+"_conv4_output.f64",n);auto adaptation=IQM::HDRVDP3::finish_local_adaptation_double(filtered);
   for(auto item:{std::pair{"response",&actual.response},std::pair{"clamped",&actual.clamped},std::pair{"adaptation_input",&actual.adaptation_input},std::pair{"conv4_input",&actual.adaptation_log},std::pair{"adaptation",&adaptation}}){auto expected=read(prefix+"_"+item.first+".f64",item.second->size());for(size_t i=0;i<expected.size();++i){if(std::bit_cast<uint64_t>(expected[i])!=std::bit_cast<uint64_t>((*item.second)[i]))throw std::runtime_error(display+" "+item.first+" mismatch "+std::to_string(i));++count;}}
  }
  for(auto input:{std::vector<double>{},std::vector<double>{1,1},std::vector<double>{-1,0,0},std::vector<double>{std::numeric_limits<double>::quiet_NaN(),0,0},std::vector<double>{std::numeric_limits<double>::infinity(),0,0}}){bool caught=false;try{IQM::HDRVDP3::project_optical_rgb_double(input,samples,24);}catch(const std::invalid_argument&){caught=true;}if(!caught)throw std::runtime_error("Invalid optical accepted");++rejected;}
 }
 for(auto input:{std::vector<double>{},std::vector<double>{std::numeric_limits<double>::quiet_NaN()},std::vector<double>{std::numeric_limits<double>::infinity()},std::vector<double>{1000},std::vector<double>{-1000}}){bool caught=false;try{IQM::HDRVDP3::finish_local_adaptation_double(input);}catch(const std::invalid_argument&){caught=true;}if(!caught)throw std::runtime_error("Invalid adaptation accepted");++rejected;}
 if(count!=608256||rejected!=20)throw std::runtime_error("Coverage");std::ofstream(out/"PASS.txt")<<count<<" Double words bitwise;20 input rejection checks. Captured optical and Gaussian output used as isolated stage boundaries;NOT independent optical/adaptation convolution or image-to-score acceptance.\n";std::cout<<"PASS "<<count<<" Double words;"<<rejected<<" rejections\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
