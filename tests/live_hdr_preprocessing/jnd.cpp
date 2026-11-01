#include <IQM/hdrvdp3/jnd.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <bit>
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace fs=std::filesystem;using namespace IQM::HDRVDP3;
std::vector<float> load(fs::path p){auto bytes=fs::file_size(p);if(bytes%4)throw std::runtime_error("Bad Single bytes");std::vector<float> v(bytes/4);std::ifstream f(p,std::ios::binary);f.read(reinterpret_cast<char*>(v.data()),bytes);if(!f)throw std::runtime_error("Read failed");return v;}
int main(int argc,char**argv){try{
 if(argc!=8||fs::exists(argv[7]))throw std::runtime_error("Usage: independent-root official-root side width height ppd fresh-root");
 const unsigned w=std::stoul(argv[4]),h=std::stoul(argv[5]);size_t n=size_t(w)*h;double ppd=std::stod(argv[6]);fs::path base=argv[1],out=argv[7];fs::create_directories(out);
 auto lmsr=load(base/"lmsr.f32"),adaptation=load(base/"adaptation.f32");
 auto result=rgb_to_jnd_response_single(create_jnd_lut(),lmsr,adaptation,w,h,double(n)/(ppd*ppd),24.);
 auto reference=load(fs::path(argv[2])/(std::string(argv[3])+"_response.f32"));if(reference.size()!=n)throw std::runtime_error("Reference shape");
 uint64_t errors=0;uint32_t ulp=0;double abs=0;for(unsigned x=0;x<w;++x)for(unsigned y=0;y<h;++y){float a=result.response[size_t(y)*w+x],r=reference[size_t(x)*h+y];auto ab=std::bit_cast<uint32_t>(a),rb=std::bit_cast<uint32_t>(r);errors+=ab!=rb;ulp=std::max(ulp,ab>rb?ab-rb:rb-ab);abs=std::max(abs,std::abs(double(a)-r));}
 std::ofstream json(out/"comparison.json");json.precision(17);json<<"{\"values\":"<<n<<",\"bit_mismatches\":"<<errors<<",\"max_ulp\":"<<ulp<<",\"max_abs\":"<<abs<<",\"pupil_reduction\":"<<result.pupil_reduction<<"}\n";
 std::ofstream binary(out/"response.f32",std::ios::binary);binary.write(reinterpret_cast<const char*>(result.response.data()),n*4);if(!json||!binary)throw std::runtime_error("Output write failed");
 std::cout<<errors<<" bit differences / "<<n<<" response values\n";return errors?2:0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
