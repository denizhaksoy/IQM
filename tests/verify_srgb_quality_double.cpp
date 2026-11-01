#include <IQM/hdrvdp3/quality_double.h>
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <limits>
namespace fs=std::filesystem;
std::vector<double> read(const fs::path&p,size_t n){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()!=std::streamoff(n*8))throw std::runtime_error("Invalid capture "+p.string());std::vector<double>v(n);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),n*8);return v;}
std::vector<double> rows(std::vector<double> v,unsigned n){auto out=v;for(size_t c=0;c<v.size()/(n*n);++c)for(unsigned y=0;y<n;++y)for(unsigned x=0;x<n;++x)out[c*n*n+y*n+x]=v[c*n*n+x*n+y];return out;}
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("Usage: native fresh-output");fs::path native=argv[1],out=argv[2];if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");size_t count=0,errors=0;std::ofstream log(out/"results.tsv");auto check=[&](std::string name,const std::vector<double>&a,const std::vector<double>&e){size_t bad=0;for(size_t i=0;i<a.size();++i){bad+=std::bit_cast<uint64_t>(a[i])!=std::bit_cast<uint64_t>(e[i]);++count;}errors+=bad;log<<name<<'\t'<<bad<<'/'<<a.size()<<'\n';};
 for(std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"}){std::vector<double> bandErrors;double accumulated=0;std::vector<std::vector<double>> masks;
  const auto referenceRGB=rows(read(native/(display+"_1_input.f64"),27648),96),testRGB=rows(read(native/(display+"_2_input.f64"),27648),96);
  const auto logMean=IQM::HDRVDP3::quality_log_adaptation_double(rows(read(native/(display+"_1_adaptation.f64"),9216),96),rows(read(native/(display+"_2_adaptation.f64"),9216),96));
  for(unsigned b=1;b<=5;++b){unsigned w=b<3?96:96/(1u<<(b-2));auto reference=rows(read(native/(display+"_1_band"+std::to_string(b)+".f64"),size_t(w)*w),w),test=rows(read(native/(display+"_2_band"+std::to_string(b)+".f64"),size_t(w)*w),w);masks.push_back(IQM::HDRVDP3::mutual_mask_double(reference,test,w,w));}
  for(unsigned b=1;b<=5;++b){const unsigned w=b<3?96:96/(1u<<(b-2));const size_t n=size_t(w)*w;const auto prefix=(native/(display+"_quality_band"+std::to_string(b))).string();auto load=[&](std::string field){return rows(read(prefix+"_"+field+".f64",n),w);};
   auto reference=rows(read(native/(display+"_1_band"+std::to_string(b)+".f64"),n),w),test=rows(read(native/(display+"_2_band"+std::to_string(b)+".f64"),n),w);check("self_from_bands",IQM::HDRVDP3::mutual_mask_double(reference,test,w,w),load("self"));
   const auto neighbor=IQM::HDRVDP3::quality_neighbor_mask_double(masks,b-1);check("neighbor_from_self",neighbor,load("neighbor"));
   std::vector<double> difference(n);for(size_t i=0;i<n;++i)difference[i]=test[i]-reference[i];
   const auto base=b==5?read(prefix+"_base_metadata.f64",3):std::vector<double>{0,0,0};
   const auto csf=IQM::HDRVDP3::quality_csf_double(logMean,b-1,base[1]);check("CSF_table",{csf.table.begin(),csf.table.end()},read(prefix+"_csf_table.f64",256));check("log_adaptation",csf.log_adaptation,load("log_adaptation"));check("sensitivity",csf.sensitivity,load("csf"));check("neural_noise",csf.noise,load("noise"));if(b==5)check("base_luminance",{csf.base_luminance},{base[2]});
   const auto comparison=IQM::HDRVDP3::quality_comparison_double(referenceRGB,testRGB,b-1);check("comparison",comparison,load("comparison"));
   const auto actual=IQM::HDRVDP3::evaluate_quality_band_double(difference,masks[b-1],neighbor,csf.noise,comparison,w,w);
   const auto p=IQM::HDRVDP3::quality_mask_parameters_double();check("parameters",{p.begin(),p.end()},read(prefix+"_parameters.f64",6));
   for(auto field:{std::pair{"mask_noise",&actual.mask_noise},std::pair{"excitation",&actual.excitation},std::pair{"difference",&actual.difference},std::pair{"psychometric",&actual.psychometric},std::pair{"masked",&actual.masked}})check(display+" b"+std::to_string(b)+" "+field.first,*field.second,load(field.first));
   check("error",{actual.error},read(prefix+"_error.f64",1));bandErrors.push_back(actual.error);accumulated+=actual.error/5.;check("accumulated",{accumulated},read(prefix+"_accumulated.f64",1));
  }
  const auto scores=IQM::HDRVDP3::quality_scores_double(bandErrors);check(display+" scores",{scores.error,scores.Q,scores.Q_JOD},read(native/(display+"_quality.f64"),3));
 }
 size_t rejected=0;auto bad=[&](auto action){bool caught=false;try{action();}catch(const std::invalid_argument&){caught=true;}if(!caught)throw std::runtime_error("Invalid quality input accepted");++rejected;};
 std::vector<double> one{1},zero{0},negative{-1},nan{std::numeric_limits<double>::quiet_NaN()};
 bad([&]{IQM::HDRVDP3::quality_scores_double({});});bad([&]{IQM::HDRVDP3::quality_scores_double(negative);});
 bad([&]{IQM::HDRVDP3::evaluate_quality_band_double(one,one,one,one,one,0,1);});
 bad([&]{IQM::HDRVDP3::evaluate_quality_band_double(one,negative,one,one,one,1,1);});
 bad([&]{IQM::HDRVDP3::evaluate_quality_band_double(one,one,one,zero,one,1,1);});
 bad([&]{IQM::HDRVDP3::evaluate_quality_band_double(one,one,one,one,nan,1,1);});
 bad([&]{IQM::HDRVDP3::mutual_mask_double(one,{},1,1);});bad([&]{IQM::HDRVDP3::quality_neighbor_mask_double({},0);});
 bad([&]{IQM::HDRVDP3::quality_log_adaptation_double({},{});});bad([&]{IQM::HDRVDP3::quality_comparison_double(one,one,0);});bad([&]{IQM::HDRVDP3::quality_csf_double({},0);});const std::vector<double> nanMap(9216,std::numeric_limits<double>::quiet_NaN());bad([&]{IQM::HDRVDP3::quality_csf_double(nanMap,0);});
 std::vector<double> infinity{std::numeric_limits<double>::infinity()};const auto limit=IQM::HDRVDP3::evaluate_quality_band_double(one,one,one,infinity,one,1,1);const std::vector<double> limitErrors{limit.error};const auto limitScore=IQM::HDRVDP3::quality_scores_double(limitErrors);if(limit.error!=0||limitScore.Q!=10||limitScore.Q_JOD!=10)throw std::runtime_error("Infinite neural-noise limit wrong");
 std::cout<<errors<<" bit errors /"<<count<<" Double quality words;"<<rejected<<" rejections;infinite-noise limit PASS\n";if(errors)return 2;std::ofstream(out/"PASS.txt")<<count<<" Double words exact;12 invalidchecks+infinite-noise limit. Native band/adaptation/RGB/base-frequency boundary inputs;own masks/CSF/comparison/scoring. Stage test,not independent original-input score/GPU acceptance.\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

