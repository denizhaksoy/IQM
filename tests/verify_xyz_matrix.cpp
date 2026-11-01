#include <IQM/hdrvdp3/spectral.h>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace fs=std::filesystem;
int main(int argc,char** argv){try{
 if(argc!=4&&argc!=5)throw std::runtime_error("Usage: native raw-data fresh-output [encoding]");fs::path native(argv[1]),raw(argv[2]),out(argv[3]);if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 using namespace IQM::HDRVDP3;LinearColorEncoding encoding=LinearColorEncoding::BT709;
 if(argc==5){std::string name=argv[4];if(name=="rgb-bt.2020")encoding=LinearColorEncoding::BT2020;else if(name=="xyz")encoding=LinearColorEncoding::XYZ;else if(name!="rgb-bt.709")throw std::runtime_error("Unknown encoding");}
 size_t count=0;auto check=[&](double a,std::ifstream& f){double e;if(!f.read(reinterpret_cast<char*>(&e),8)||std::bit_cast<uint64_t>(a)!=std::bit_cast<uint64_t>(e))throw std::runtime_error("XYZ bits differ at "+std::to_string(count));++count;};
 for(std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"}){auto samples=IQM::HDRVDP3::load_xyz_display_samples(raw/"ciexyz31.csv",raw/("emission_spectra_"+display+".csv"));std::ifstream expected(native/(display+"_samples.f64"),std::ios::binary);
  for(size_t i=0;i<420;++i){check(samples.wavelength[i],expected);for(double x:samples.xyz[i])check(x,expected);for(double x:samples.emission[i])check(x,expected);}
  auto actual=IQM::HDRVDP3::calibrate_display_xyz(samples);std::ifstream matrices(native/(display+"_matrices.f64"),std::ios::binary);
  for(const auto* matrix:{&actual.integrals,&actual.native2XYZ})for(unsigned col=0;col<3;++col)for(unsigned row=0;row<3;++row)check((*matrix)[row][col],matrices);
  auto transform=IQM::HDRVDP3::linear_color_transform(samples,encoding);for(unsigned col=0;col<3;++col)for(unsigned row=0;row<3;++row)check(transform[row][col],matrices);
  std::ofstream transformDump(out/(display+"_transform.f64"),std::ios::binary);for(unsigned col=0;col<3;++col)for(unsigned row=0;row<3;++row){double v=transform[row][col];transformDump.write(reinterpret_cast<const char*>(&v),8);}
  std::ofstream dump(out/(display+"_A.f64"),std::ios::binary);for(unsigned col=0;col<3;++col)for(unsigned row=0;row<3;++row){double x=actual.native2XYZ[row][col];dump.write(reinterpret_cast<const char*>(&x),8);}
 }
 if(count!=8901)throw std::runtime_error("Coverage mismatch");std::ofstream(out/"PASS.txt")<<count<<" Double values bitwise exact;XYZ loader/integrals/normalization and independent color solve;encoding="<<(argc==5?argv[4]:"rgb-bt.709")<<'\n';std::cout<<"Linear color calibration PASS "<<count<<" Double words\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
