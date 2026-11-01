#include <IQM/hdrvdp3/sp0_double.h>
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace fs=std::filesystem;
std::vector<double> read(const fs::path&p,size_t n){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()!=std::streamoff(n*8))throw std::runtime_error("Invalid capture "+p.string());std::vector<double>v(n);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),n*8);return v;}
std::vector<double> rows(std::vector<double> v,unsigned w,unsigned h){auto out=v;for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x)out[y*w+x]=v[x*h+y];return out;}
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("Usage: native fresh-output");fs::path native=argv[1],out=argv[2];if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");size_t count=0,errors=0;std::ofstream log(out/"results.tsv");auto check=[&](std::string name,const std::vector<double>&a,const std::vector<double>&e){size_t bad=0;for(size_t i=0;i<a.size();++i){bad+=std::bit_cast<uint64_t>(a[i])!=std::bit_cast<uint64_t>(e[i]);++count;}errors+=bad;log<<name<<'\t'<<bad<<'/'<<a.size()<<'\n';};
 auto f=IQM::HDRVDP3::create_sp0_model_double();auto expectedFilters=read(native/"filters.f64",380);size_t offset=0;for(auto p:{&f.lo0,&f.high,&f.lo,&f.orientation}){unsigned k=unsigned(std::sqrt(p->size()));std::vector<double> part(expectedFilters.begin()+offset,expectedFilters.begin()+offset+p->size());check("filter",*p,rows(part,k,k));offset+=p->size();}
 for(std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"})for(int side=1;side<=2;++side){auto prefix=(native/(display+"_"+std::to_string(side))).string();auto pyr=IQM::HDRVDP3::build_sp0_pyramid_double(rows(read(prefix+"_P.f64",9216),96,96));for(size_t b=0;b<pyr.bands.size();++b){auto&band=pyr.bands[b];check(display+" band"+std::to_string(b+1),band.values,rows(read(prefix+"_band"+std::to_string(b+1)+".f64",band.values.size()),band.width,band.height));}check(display+" base_pre",pyr.base_pre,rows(read(prefix+"_base_pre.f64",144),12,12));check(display+" mean",{pyr.base_mean},read(prefix+"_base_mean.f64",1));}
 size_t rejected=0;for(auto v:{std::vector<double>{},std::vector<double>(9215,1),std::vector<double>(9216,std::numeric_limits<double>::quiet_NaN())}){bool caught=false;try{IQM::HDRVDP3::build_sp0_pyramid_double(v);}catch(const std::invalid_argument&){caught=true;}if(!caught)throw std::runtime_error("Invalid pyramid input accepted");++rejected;}
 std::cout<<errors<<" bit errors /"<<count<<" Double pyramid words;"<<rejected<<" rejections\n";if(errors)return 2;std::ofstream(out/"PASS.txt")<<count<<" Double words bitwise;3invalidchecks;independent filters/pyramid from captured P;not original-input or GPU/fullscore acceptance.\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
