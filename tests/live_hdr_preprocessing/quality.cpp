#include "../single_fftw_backend.h"
#include <IQM/hdrvdp3/general_quality_single.h>
#include <IQM/hdrvdp3/linear_color_single.h>
#include <fstream>
#include <iostream>
#include <bit>
#include <iomanip>
namespace fs=std::filesystem;using namespace IQM::HDRVDP3;
std::vector<float> load(fs::path p){auto bytes=fs::file_size(p);if(bytes%4)throw std::runtime_error("Bad Single bytes");std::vector<float> v(bytes/4);std::ifstream f(p,std::ios::binary);f.read(reinterpret_cast<char*>(v.data()),bytes);if(!f)throw std::runtime_error("Read failed");return v;}
int main(int argc,char**argv){try{
 if(argc!=10||fs::exists(argv[9]))throw std::runtime_error("Usage: linear-root own-fft-root own-pyramid-root data-root fftw-float-dll width height ppd fresh-root");
 unsigned w=std::stoul(argv[6]),h=std::stoul(argv[7]);double ppd=std::stod(argv[8]);fs::path out=argv[9];fs::create_directories(out);
 auto data=fs::path(argv[4]);auto xyz=load_xyz_display_samples(data/"ciexyz31.csv",data/"emission_spectra_oled.csv");
 auto side=[&](const char* name){QualitySideSingle s;s.native_rgb=prepare_linear_color_single(load(fs::path(argv[1])/(std::string(name)+".f32")),xyz,LinearColorEncoding::BT2020);s.adaptation=load(fs::path(argv[2])/name/"adaptation.f32");unsigned bw=w,bh=h;
 for(size_t b=1;;++b){auto file=fs::path(argv[3])/name/("band"+std::to_string(b)+".f32");if(!fs::exists(file))break;if(b>2){bw=(bw+1)/2;bh=(bh+1)/2;}s.pyramid.bands.push_back({bw,bh,load(file)});}return s;};
 auto r=side("reference"),t=side("test");SingleFFTW fft(argv[5]);auto result=evaluate_quality_single(r,t,w,h,ppd,fft.backend());
 std::ofstream f(out/"score.json");f.precision(17);f<<"{\"scope\":\"independent C++ Single quality from independently computed persisted stages\",\"Q_text\":\""<<result.scores.Q<<"\",\"Q_JOD_text\":\""<<result.scores.Q_JOD<<"\",\"Q_hex\":\""<<std::hex<<std::setw(8)<<std::setfill('0')<<std::bit_cast<uint32_t>(result.scores.Q)<<"\",\"Q_JOD_hex\":\""<<std::setw(8)<<std::bit_cast<uint32_t>(result.scores.Q_JOD)<<std::dec<<"\",\"base_frequency\":"<<result.base_frequency<<",\"band_errors\":[";
 for(size_t b=0;b<result.band_errors.size();++b){if(b)f<<',';f<<result.band_errors[b];}f<<"]}\n";if(!f)throw std::runtime_error("Output write failed");std::cout<<std::setprecision(17)<<"Q="<<result.scores.Q<<" Q_JOD="<<result.scores.Q_JOD<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
