#include <IQM/hdrvdp3/pathway_quality.h>
#include <IQM/hdrvdp3/sp0_model.h>
#include <vulkan_instance.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>
#ifndef IQM_SQUARE_SIDE
#define IQM_SQUARE_SIDE 96
#endif
namespace fs=std::filesystem;
namespace IQM::HDRVDP3 {
QualityScores evaluate_square_quality_diagnostic(const IQM::VulkanInstance&,const std::vector<float>&,const std::vector<float>&,unsigned,unsigned,const PathwayQualityAssets&,PathwayQualityTrace*);
}
static IQM::HDRVDP3::PathwayQualityAssets load_assets(const fs::path& root){
 IQM::HDRVDP3::PathwayQualityAssets a;
 a.filter={root/"forward.spv",root/"inverse.spv",root/"io.spv"};
 a.spyr={root/"spyr_fma.spv",root/"spyr_separate.spv",root/"mean.spv",root/"difference.spv",root/"residual.spv"};
 a.filters=IQM::HDRVDP3::create_sp0_model();a.pool={root/"pool_fma.spv",root/"pool_separate.spv"};
 a.comparison=root/"comparison.spv";a.masking={root/"map.spv",root/"product.spv",root/"base.spv"};return a;
}

template<class T>std::vector<T> read(const fs::path& p,unsigned* width=nullptr,unsigned* height=nullptr){std::ifstream f(p);unsigned h,w;if(!(f>>h>>w)||!h||!w||size_t(h)*w>1000000)throw std::runtime_error("Bad native shape:"+p.string());if(width)*width=w;if(height)*height=h;std::vector<T> v(size_t(h)*w);for(auto& x:v)if(!(f>>x)||!std::isfinite(double(x)))throw std::runtime_error("Bad finite native data:"+p.string());std::string extra;if(f>>extra)throw std::runtime_error("Trailing fixture");return v;}
int main(int argc,char** argv){try{
 if(argc!=4)throw std::runtime_error("Usage:native-case runtime fresh-output");const fs::path native(argv[1]),out(argv[3]);if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");unsigned w,h,tw,th;const auto ref=read<float>(native/"reference.txt",&w,&h),test=read<float>(native/"test.txt",&tw,&th);if(w!=tw||h!=th||w!=IQM_SQUARE_SIDE||h!=IQM_SQUARE_SIDE)throw std::runtime_error("Input geometry mismatch");const auto assets=load_assets(argv[2]);IQM::Bin::VulkanInstance instance(true);IQM::HDRVDP3::PathwayQualityTrace trace;const auto scores=IQM::HDRVDP3::evaluate_square_quality_diagnostic(instance,ref,test,w,h,assets,&trace);
 std::ofstream results(out/"results.tsv");results<<"stage\tvalues\tnumeric_errors\tbit_errors\n";size_t checks=0,total=0;
 auto check=[&]<class T>(const std::string& name,const std::vector<T>& actual,const std::vector<T>& expected){if(actual.size()!=expected.size())throw std::runtime_error("Shape mismatch:"+name);using U=std::conditional_t<sizeof(T)==4,uint32_t,uint64_t>;size_t numeric=0,bits=0,first=actual.size();for(size_t i=0;i<actual.size();++i){numeric+=actual[i]!=expected[i];if(std::bit_cast<U>(actual[i])!=std::bit_cast<U>(expected[i])){++bits;if(first==actual.size())first=i;}}for(unsigned side=0;side<2;++side){const auto& v=side?expected:actual;std::ofstream f(out/(name+(side?"_expected.bin":".bin")),std::ios::binary);f.write(reinterpret_cast<const char*>(v.data()),v.size()*sizeof(T));}results<<name<<'\t'<<actual.size()<<'\t'<<numeric<<'\t'<<bits<<'\n';results.flush();++checks;total+=actual.size();if(bits){std::cerr<<std::setprecision(17)<<name<<" first="<<first<<" actual="<<actual[first]<<" native="<<expected[first]<<" numeric="<<numeric<<" bits="<<bits<<'\n';throw std::runtime_error("Strict native parity failure");}};
 check("reference_P",trace.P[0],read<float>(native/"P_reference.txt"));check("test_P",trace.P[1],read<float>(native/"P_test.txt"));check("log",trace.logAdaptation,read<float>(native/"log_La.txt"));
 const std::array<unsigned,5> widths{w,w,w/2,w/4,w/8},heights{h,h,h/2,h/4,h/8};std::vector<float> errors;
 for(unsigned b=0;b<5;++b){const auto prefix="band"+std::to_string(b+1);auto field=[&](const std::string& n){return read<float>(native/(prefix+"_mask_"+n+".txt"));};auto nr=read<float>(native/(prefix+"_o1_reference.txt")),nt=read<float>(native/(prefix+"_o1_test.txt"));const size_t n=size_t(widths[b])*heights[b];const auto& words=trace.spyr.stages[b<4?b+4:8];if(words.size()!=(b<4?2*n:2*(n+1)))throw std::runtime_error("SPYR size");std::vector<float> ar,at;for(size_t i=0;i<n;++i){ar.push_back(std::bit_cast<float>(words[(b<4?0:1)+i]));at.push_back(std::bit_cast<float>(words[(b<4?n:n+2)+i]));}check(prefix+"_reference",ar,nr);check(prefix+"_test",at,nt);auto raw=nr,diff=nr;for(size_t i=0;i<n;++i){raw[i]=std::min(std::abs(nr[i]),std::abs(nt[i]));diff[i]=nt[i]-nr[i];}check(prefix+"_raw",trace.pool[b].raw,raw);check(prefix+"_pool",trace.pool[b].pooled,field("self"));check(prefix+"_comparison",trace.comparison[b].resized,read<double>(native/(prefix+"_mask_comparison.txt")));const auto& a=trace.masking.bands[b];check(prefix+"_neighbor",a.neighbor,field("neighbor"));check(prefix+"_denominator",a.denominator,field("denominator"));check(prefix+"_difference",a.difference,diff);check(prefix+"_CSF",a.sensitivity,read<float>(native/(prefix+"_csf.txt")));check(prefix+"_D",a.D,field("difference"));check(prefix+"_masked",a.masked,field("quality_difference"));check(prefix+"_psychometric",a.psychometric,read<float>(native/(prefix+"_psychometric.txt")));errors.push_back(a.error);
 }
 check("band_errors",errors,read<float>(native/"band_errors.txt"));check("Q_error",std::vector<float>{scores.error},read<float>(native/"Q_error.txt"));check("scores",std::vector<float>{scores.Q,scores.Q_JOD},read<float>(native/"scores.txt"));
 std::ifstream f(native/"base_selection.txt");unsigned bh,bw;double index,frequency,luminance;std::string noise,extra;if(!(f>>bh>>bw>>index>>frequency>>luminance>>noise)||bh!=1||bw!=4||(f>>extra))throw std::runtime_error("Bad native base selection");const double ncsf=noise=="Inf"?std::numeric_limits<double>::infinity():std::stod(noise);if(!std::isfinite(ncsf)&&!(frequency==0&&std::isinf(ncsf)&&ncsf>0))throw std::runtime_error("Unsupported native base limit");check("base_noise",std::vector<double>(trace.masking.baseNoise.begin(),trace.masking.baseNoise.end()),std::vector<double>{index-1,frequency,ncsf});const auto logs=read<float>(native/"band5_log_resized.txt");check("base_log",trace.masking.baseLog,logs);double sum=0;for(float x:logs)sum+=double(x);check("base_adaptation",trace.masking.baseAdaptation,std::vector<float>{float(sum/double(logs.size())),float(luminance)});
 if(checks!=69)throw std::runtime_error("Incomplete contrast checks");std::ofstream(out/"PASS.txt")<<"69 native contrast stage checks;"<<total<<" values bitwise exact,including D/errors/baseNoise/Q.\n";std::ofstream(out/"scores.json")<<std::setprecision(17)<<"{\"Q\":"<<scores.Q<<",\"Q_JOD\":"<<scores.Q_JOD<<",\"checks\":"<<checks<<",\"values\":"<<total<<"}";std::cout<<"Contrast native bits PASS:69checks,"<<total<<"values\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
