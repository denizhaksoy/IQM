#include <IQM/hdrvdp3/srgb_display.h>
#include <bit>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace fs=std::filesystem;
template<class T> std::vector<T> read(const fs::path& p,size_t count){
 std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()!=std::streamoff(count*sizeof(T)))throw std::runtime_error("Invalid capture "+p.string());
 std::vector<T> v(count);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),count*sizeof(T));if(!f)throw std::runtime_error("Read failure");return v;
}
int main(int argc,char** argv){try{
 if(argc!=4)throw std::runtime_error("Usage: native raw-data fresh-output");
 fs::path native=argv[1],data=argv[2],out=argv[3];if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 size_t values=0,rejected=0;
 for(std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"}){
  const auto samples=IQM::HDRVDP3::load_xyz_display_samples(data/"ciexyz31.csv",data/("emission_spectra_"+display+".csv"));
  for(int side=1;side<=2;++side){const auto prefix=(native/(display+"_"+std::to_string(side))).string();auto input=read<float>(prefix+"_input.f32",27648);
   const auto result=IQM::HDRVDP3::prepare_srgb_display(input,samples);
   for(const auto& item:{std::pair{"eotf",&result.display_rgb},std::pair{"raw",&result.raw_native_rgb},std::pair{"v",&result.native_rgb}}){auto expected=read<double>(prefix+"_"+item.first+".f64",27648);
    for(size_t i=0;i<expected.size();++i){if(std::bit_cast<uint64_t>((*item.second)[i])!=std::bit_cast<uint64_t>(expected[i]))throw std::runtime_error(display+" side"+std::to_string(side)+" "+item.first+" bit mismatch at "+std::to_string(i));++values;}
    std::ofstream f(out/(display+"_"+std::to_string(side)+"_"+item.first+".f64"),std::ios::binary);f.write(reinterpret_cast<const char*>(item.second->data()),std::streamsize(item.second->size()*8));
   }
  }
  for(auto input:{std::vector<float>{},std::vector<float>{0,0},std::vector<float>{-1,0,0},std::vector<float>{1.01f,0,0},std::vector<float>{std::numeric_limits<float>::quiet_NaN(),0,0},std::vector<float>{std::numeric_limits<float>::infinity(),0,0}}){bool caught=false;try{IQM::HDRVDP3::prepare_srgb_display(input,samples);}catch(const std::invalid_argument&){caught=true;}if(!caught)throw std::runtime_error("Invalid input accepted");++rejected;}
 }
 if(values!=497664||rejected!=18)throw std::runtime_error("Coverage count");
 std::ofstream(out/"PASS.txt")<<values<<" Double words bitwise exact;"<<rejected<<" invalid inputs rejected. Independent original Single input to EOTF/raw/clamped native RGB,three spectra. CPU preprocessing only,no full score/GPU acceptance.\n";
 std::cout<<"PASS "<<values<<" Double words;"<<rejected<<" rejections\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
