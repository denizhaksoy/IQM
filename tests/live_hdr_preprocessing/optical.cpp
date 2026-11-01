#include <IQM/hdrvdp3/optical_response_single.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <bit>
#include <cmath>
#include <stdexcept>
namespace fs=std::filesystem;using namespace IQM::HDRVDP3;
std::vector<float> load(fs::path p){auto bytes=fs::file_size(p);if(bytes%4)throw std::runtime_error("Bad Single bytes");std::vector<float> v(bytes/4);std::ifstream f(p,std::ios::binary);f.read(reinterpret_cast<char*>(v.data()),bytes);if(!f)throw std::runtime_error("Read failed");return v;}
int main(int argc,char**argv){try{
 if(argc!=7||fs::exists(argv[6]))throw std::runtime_error("Usage: optical.f32 lmsr.f32 data-root width height fresh.json");
 auto input=load(argv[1]),reference=load(argv[2]);size_t w=std::stoul(argv[4]),h=std::stoul(argv[5]),n=w*h;
 if(input.size()!=3*n||reference.size()!=4*n)throw std::runtime_error("Shape mismatch");
 std::vector<float> rows(input.size());for(size_t c=0;c<3;++c)for(size_t y=0;y<h;++y)for(size_t x=0;x<w;++x)rows[c*n+y*w+x]=input[c*n+x*h+y];
 fs::path data=argv[3];auto samples=load_rgb_spectral_samples(data/"log_cone_smith_pokorny_1975.csv",data/"cie_scotopic_lum.txt",data/"emission_spectra_oled.csv");
 auto result=project_optical_rgb_single(rows,samples,24.);
 size_t errors=0;uint32_t maxUlp=0;double maxAbs=0;
 for(size_t c=0;c<4;++c)for(size_t y=0;y<h;++y)for(size_t x=0;x<w;++x){auto a=result.clamped[c*n+y*w+x],r=reference[c*n+x*h+y];auto ab=std::bit_cast<uint32_t>(a),rb=std::bit_cast<uint32_t>(r);errors+=ab!=rb;maxUlp=std::max(maxUlp,ab>rb?ab-rb:rb-ab);maxAbs=std::max(maxAbs,std::abs(double(a)-r));}
 std::ofstream out(argv[6]);out.precision(17);out<<"{\"scope\":\"stage isolation using official optical inputs, not independent full pipeline\",\"values\":"<<result.clamped.size()<<",\"bit_mismatches\":"<<errors<<",\"max_ulp\":"<<maxUlp<<",\"max_abs\":"<<maxAbs<<"}\n";
 if(!out)throw std::runtime_error("Write failed");std::cout<<errors<<" bit differences / "<<result.clamped.size()<<'\n';return errors?2:0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
