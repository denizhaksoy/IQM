#include <IQM/hdrvdp3/sp0_single.h>
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
 if(argc!=8||fs::exists(argv[7]))throw std::runtime_error("Usage: response.f32 official-root side width height ppd fresh-root");
 unsigned w=std::stoul(argv[4]),h=std::stoul(argv[5]);double ppd=std::stod(argv[6]);fs::path out=argv[7];fs::create_directories(out);
 auto result=build_sp0_pyramid_single(load(argv[1]),w,h,ppd);uint64_t total=0,errors=0;std::ofstream json(out/"comparison.jsonl");
 for(size_t b=0;b<result.bands.size();++b){auto& band=result.bands[b];auto ref=load(fs::path(argv[2])/(std::string(argv[3])+"_band"+std::to_string(b+1)+".f32"));if(ref.size()!=band.values.size())throw std::runtime_error("Band shape");uint64_t count=0;double abs=0;
 for(unsigned x=0;x<band.width;++x)for(unsigned y=0;y<band.height;++y){float a=band.values[size_t(y)*band.width+x],r=ref[size_t(x)*band.height+y];count+=std::bit_cast<uint32_t>(a)!=std::bit_cast<uint32_t>(r);abs=std::max(abs,std::abs(double(a)-r));}
 json.precision(17);json<<"{\"band\":"<<b+1<<",\"width\":"<<band.width<<",\"height\":"<<band.height<<",\"values\":"<<ref.size()<<",\"bit_mismatches\":"<<count<<",\"max_abs\":"<<abs<<"}\n";
 std::ofstream binary(out/("band"+std::to_string(b+1)+".f32"),std::ios::binary);binary.write(reinterpret_cast<const char*>(band.values.data()),band.values.size()*4);if(!binary)throw std::runtime_error("Write failed");total+=ref.size();errors+=count;std::cout<<"band"<<b+1<<": "<<count<<" / "<<ref.size()<<'\n';}
 std::ofstream(out/"result.txt")<<errors<<" bit differences / "<<total<<" Single values\n";return errors?2:0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
