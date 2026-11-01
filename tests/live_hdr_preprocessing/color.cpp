#include <IQM/hdrvdp3/linear_color_single.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <bit>
#include <cmath>
#include <stdexcept>
namespace fs=std::filesystem;using namespace IQM::HDRVDP3;
std::vector<float> load(fs::path p){auto bytes=fs::file_size(p);if(bytes%4)throw std::runtime_error("Bad Single bytes");std::vector<float> v(bytes/4);std::ifstream f(p,std::ios::binary);f.read(reinterpret_cast<char*>(v.data()),bytes);if(!f)throw std::runtime_error("Read failed");return v;}
int main(int argc,char**argv){try{
 if(argc!=7||fs::exists(argv[6]))throw std::runtime_error("Usage: input.f32 native.f32 data-root width height fresh.json");
 auto input=load(argv[1]),reference=load(argv[2]);size_t w=std::stoul(argv[4]),h=std::stoul(argv[5]),n=w*h;
 if(input.size()!=3*n||reference.size()!=3*n)throw std::runtime_error("Shape mismatch");
 fs::path data=argv[3];auto samples=load_xyz_display_samples(data/"ciexyz31.csv",data/"emission_spectra_oled.csv");
 auto actual=prepare_linear_color_single(input,samples,LinearColorEncoding::BT2020);
 size_t errors=0;uint32_t maxUlp=0;double maxAbs=0;
 for(size_t c=0;c<3;++c)for(size_t y=0;y<h;++y)for(size_t x=0;x<w;++x){auto a=actual[c*n+y*w+x],r=reference[c*n+x*h+y];auto ab=std::bit_cast<uint32_t>(a),rb=std::bit_cast<uint32_t>(r);errors+=ab!=rb;maxUlp=std::max(maxUlp,ab>rb?ab-rb:rb-ab);maxAbs=std::max(maxAbs,std::abs(double(a)-r));}
 std::ofstream out(argv[6]);out.precision(17);out<<"{\"values\":"<<actual.size()<<",\"bit_mismatches\":"<<errors<<",\"max_ulp\":"<<maxUlp<<",\"max_abs\":"<<maxAbs<<"}\n";
 if(!out)throw std::runtime_error("Write failed");std::cout<<errors<<" bit differences / "<<actual.size()<<'\n';return errors?2:0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
