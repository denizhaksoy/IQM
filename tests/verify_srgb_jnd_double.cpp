#include <IQM/hdrvdp3/jnd.h>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace fs=std::filesystem;
std::vector<double> read(const fs::path&p,size_t n){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()!=std::streamoff(n*8))throw std::runtime_error("Invalid capture "+p.string());std::vector<double>v(n);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),n*8);return v;}
std::vector<double> rows(std::vector<double> v){auto out=v;for(size_t c=0;c<v.size()/9216;++c)for(size_t y=0;y<96;++y)for(size_t x=0;x<96;++x)out[c*9216+y*96+x]=v[c*9216+x*96+y];return out;}
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("Usage: native fresh-output");fs::path native=argv[1],out=argv[2];if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");const auto lut=IQM::HDRVDP3::create_jnd_lut();size_t count=0,errors=0;std::ofstream log(out/"results.tsv");
 auto check=[&](const std::string& name,const std::vector<double>& a,const std::vector<double>& e){size_t bad=0;for(size_t i=0;i<a.size();++i){bad+=std::bit_cast<uint64_t>(a[i])!=std::bit_cast<uint64_t>(e[i]);++count;}errors+=bad;log<<name<<'\t'<<bad<<'/'<<a.size()<<'\n';};
 for(std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"})for(int side=1;side<=2;++side){auto p=(native/(display+"_"+std::to_string(side))).string();auto table=read(p+"_lut.f64",6144);std::vector<double> actualTable;for(auto values:{&lut.log_luminance,&lut.cone,&lut.rod})actualTable.insert(actualTable.end(),values->begin(),values->end());check(display+" lut",actualTable,table);
  auto actual=IQM::HDRVDP3::rgb_to_jnd_response_double(lut,rows(read(p+"_clamped.f64",36864)),rows(read(p+"_adaptation.f64",9216)),96,96,96.*96./30./30.,24);
  check(display+" geo",{actual.geometric_mean},read(p+"_geomean.f64",1));check(display+" pupil",{actual.pupil_reduction},read(p+"_pupil_reduction.f64",1));
  check(display+" post",actual.post_pupil,rows(read(p+"_post_pupil.f64",36864)));check(display+" jnd",actual.channels,rows(read(p+"_jnd.f64",36864)));check(display+" P",actual.response,rows(read(p+"_P.f64",9216)));
 }
 size_t rejected=0;auto bad=[&](auto action){bool caught=false;try{action();}catch(const std::invalid_argument&){caught=true;}if(!caught)throw std::runtime_error("Invalid JND input accepted");++rejected;};
 std::vector<double> lmsr(16,1),adapt(4,1);auto run=[&](const IQM::HDRVDP3::JNDLUT& table,unsigned width,double area){return IQM::HDRVDP3::rgb_to_jnd_response_double(table,lmsr,adapt,width,2,area,24);};
 bad([&]{run(lut,0,1);});bad([&]{run(lut,3,1);});bad([&]{run(lut,2,0);});
 lmsr[0]=0;bad([&]{run(lut,2,1);});lmsr[0]=1;adapt[0]=-1;bad([&]{run(lut,2,1);});adapt[0]=1;
 auto broken=lut;broken.log_luminance[1]=broken.log_luminance[0];bad([&]{run(broken,2,1);});
 std::cout<<"Double JND "<<errors<<" bit errors /"<<count<<";"<<rejected<<" rejections\n";if(errors)return 2;std::ofstream(out/"PASS.txt")<<count<<" Double words bitwise;6 invalid input checks;independent LUT/pupil/JND,using captured clamped LMSR/adaptation stage inputs. Not full image-to-score acceptance.\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
