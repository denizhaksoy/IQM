#include "../single_fftw_backend.h"
#include <IQM/hdrvdp3/optical_single.h>
#include <IQM/hdrvdp3/linear_color_single.h>
#include <IQM/hdrvdp3/optical_response_single.h>
#include <fstream>
#include <iostream>
#include <bit>
#include <cmath>
namespace fs=std::filesystem;using namespace IQM::HDRVDP3;
std::vector<float> load(fs::path p){auto bytes=fs::file_size(p);if(bytes%4)throw std::runtime_error("Bad Single bytes");std::vector<float> v(bytes/4);std::ifstream f(p,std::ios::binary);f.read(reinterpret_cast<char*>(v.data()),bytes);if(!f)throw std::runtime_error("Read failed");return v;}
void save(fs::path p,std::span<const float> v){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(v.data()),v.size_bytes());if(!f)throw std::runtime_error("Write failed");}
int main(int argc,char**argv){try{
 if(argc!=10||fs::exists(argv[9]))throw std::runtime_error("Usage: linear.f32 official-dir side data-root fftw-float-dll width height ppd fresh-out");
 const size_t w=std::stoul(argv[6]),h=std::stoul(argv[7]),n=w*h;const double ppd=std::stod(argv[8]);fs::path data=argv[4],out=argv[9];fs::create_directories(out);
 auto input=load(argv[1]);if(input.size()!=3*n)throw std::runtime_error("Input shape");
 auto xyz=load_xyz_display_samples(data/"ciexyz31.csv",data/"emission_spectra_oled.csv");
 auto native=prepare_linear_color_single(input,xyz,LinearColorEncoding::BT2020);input.clear();input.shrink_to_fit();
 SingleFFTW fft(argv[5]);auto optical=filter_optical_rgb_single(native,unsigned(w),unsigned(h),ppd,fft.backend());native.clear();native.shrink_to_fit();
 auto spectral=load_rgb_spectral_samples(data/"log_cone_smith_pokorny_1975.csv",data/"cie_scotopic_lum.txt",data/"emission_spectra_oled.csv");
 auto lmsr=project_optical_rgb_single(optical,spectral,24.);
 auto adaptation=local_adaptation_single(lmsr.adaptation_input,unsigned(w),unsigned(h),ppd,fft.backend());
 std::ofstream results(out/"comparisons.jsonl");uint64_t total=0,errors=0;
 auto compare=[&](const char* name,std::span<const float> actual,size_t channels){auto ref=load(fs::path(argv[2])/(std::string(argv[3])+"_"+name+".f32"));if(ref.size()!=actual.size())throw std::runtime_error("Reference shape");uint64_t count=0;uint32_t ulp=0;double abs=0;
 for(size_t c=0;c<channels;++c)for(size_t y=0;y<h;++y)for(size_t x=0;x<w;++x){float a=actual[c*n+y*w+x],r=ref[c*n+x*h+y];auto ab=std::bit_cast<uint32_t>(a),rb=std::bit_cast<uint32_t>(r);count+=ab!=rb;ulp=std::max(ulp,ab>rb?ab-rb:rb-ab);abs=std::max(abs,std::abs(double(a)-r));}
 results.precision(17);results<<"{\"stage\":\""<<name<<"\",\"values\":"<<actual.size()<<",\"bit_mismatches\":"<<count<<",\"max_ulp\":"<<ulp<<",\"max_abs\":"<<abs<<"}\n";std::cout<<name<<": "<<count<<" / "<<actual.size()<<'\n';total+=actual.size();errors+=count;save(out/(std::string(name)+".f32"),actual);};
 compare("optical",optical,3);compare("lmsr",lmsr.clamped,4);compare("adaptation",adaptation,1);
 std::ofstream(out/"result.txt")<<errors<<" bit differences / "<<total<<" Single values\n";return errors?2:0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
