#include <IQM/hdrvdp3/mixed_filter.h>
#include <IQM/hdrvdp3/map_gpu.h>
#include <IQM/hdrvdp3/sp0_model.h>
#include <vulkan_instance.h>
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#ifndef IQM_SQUARE_SIDE
#define IQM_SQUARE_SIDE 96
#endif
constexpr unsigned SquareSide=IQM_SQUARE_SIDE;
namespace fs=std::filesystem;
#if defined(IQM_SQUARE_PUBLIC_CALLS)
#define run_square_filter_diagnostic run_mixed_filter
#define run_square_spyr_diagnostic run_pathway_spyr
#endif
namespace IQM::HDRVDP3 {
// This symbol exists only in the isolated diagnostic build. The production
// function and public geometry guards keep their original supported shapes.
MixedFilterResult run_square_filter_diagnostic(const IQM::VulkanInstance&,
 const std::vector<float>&,unsigned,unsigned,MixedFilterMode,const MixedFilterAssets&,bool);
PathwaySPYRResult run_square_spyr_diagnostic(const IQM::VulkanInstance&,
 const std::vector<float>&,const std::vector<float>&,unsigned,unsigned,
 const PathwaySPYRFilters&,const PathwaySPYRAssets&);
}
static std::vector<float> read(const fs::path& path,unsigned h,unsigned w){
 std::ifstream f(path);unsigned rh,rw;if(!(f>>rh>>rw)||rh!=h||rw!=w)throw std::runtime_error("Fixture shape:"+path.string());
 std::vector<float> result(size_t(h)*w);for(float& v:result){double d;if(!(f>>d)||!std::isfinite(d))throw std::runtime_error("Invalid native value");v=float(d);}
 std::string extra;if(f>>extra)throw std::runtime_error("Trailing native data");return result;
}
int main(int argc,char** argv){try{
 if(argc!=7&&argc!=8&&argc!=14)throw std::runtime_error("Usage:native forward inverse io map fresh-output [visual [spyrFMA separate mean difference residual spyr-goldens]]");
 const bool spyr=argc==14,visual=argc>=8;
 const fs::path native(argv[1]),out(argv[6]);if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 using namespace IQM::HDRVDP3;
 const MixedFilterAssets assets{argv[2],argv[3],argv[4]};IQM::Bin::VulkanInstance instance(true);
 MapGPU gpu(instance,argv[5],SquareSide==128?MapGPUProfile::Extended128Components:MapGPUProfile::Bounded);
 const auto lut=visual?create_jnd_lut():JNDLUT{};
 std::array<std::vector<float>,2> adaptations;
 std::array<std::vector<float>,2> Pmaps;
 // Only original images enter before the GPU calls. Goldens are read below.
 const auto reference=read(native/"ref_luminance.txt",SquareSide,SquareSide),test=read(native/"test_luminance.txt",SquareSide,SquareSide);
 std::ofstream report(out/"results.tsv");report<<"stage\tvalues\tnumeric_errors\tbit_errors\n";size_t checks=0,values=0;
 auto check=[&](const std::string& name,const std::vector<float>& actual,const std::vector<float>& expected){
  if(actual.size()!=expected.size())throw std::runtime_error("Stage size:"+name);
  size_t numeric=0,bits=0;for(size_t i=0;i<actual.size();++i){numeric+=!std::isfinite(actual[i])||actual[i]!=expected[i];bits+=std::bit_cast<uint32_t>(actual[i])!=std::bit_cast<uint32_t>(expected[i]);}
  std::ofstream dump(out/(name+".f32"),std::ios::binary);dump.write(reinterpret_cast<const char*>(actual.data()),actual.size()*4);
  report<<name<<'\t'<<actual.size()<<'\t'<<numeric<<'\t'<<bits<<'\n';report.flush();
  if(numeric||bits)throw std::runtime_error("Square chain mismatch:"+name+" bits="+std::to_string(bits));++checks;values+=actual.size();
 };
 for(const std::string label:{"ref","test"}){
  const auto& input=label=="ref"?reference:test;
  const auto mtf=run_square_filter_diagnostic(instance,input,SquareSide,SquareSide,MixedFilterMode::OpticalMTF,assets,true);
  const auto adaptationInput=gpu.adaptation_input(mtf.output);
  const auto la=run_square_filter_diagnostic(instance,adaptationInput,SquareSide,SquareSide,MixedFilterMode::LocalAdaptation,assets,true);
  std::array<float,4> pupil{};std::vector<float> P;
  if(visual){
   pupil=gpu.pupil_trace(la.output,SquareSide,SquareSide,double(SquareSide)*SquareSide/(30.0*30),24);
   const auto response=gpu.jnd_response(mtf.output,lut,pupil[3]);
   for(const auto& pixel:response)P.push_back(pixel.response);
   adaptations[label=="ref"?0:1]=la.output;
   Pmaps[label=="ref"?0:1]=P;
  }
  for(unsigned chain=0;chain<2;++chain){
   const auto& data=chain?la:mtf;const std::string tag=label+(chain?"_la":"_mtf");
   check(tag+"_model",data.modelFilter,read(native/(chain?"la_filter_double.txt":"mtf_filter_double.txt"),(2*SquareSide),(2*SquareSide)));
   check(tag+"_prepared",data.prepared,read(native/(tag+"_stage0.txt"),(2*SquareSide),(4*SquareSide)));
   check(tag+"_forward",data.forward,read(native/(label+(chain?"_la_native_forward.txt":"_native_forward.txt")),(2*SquareSide),(4*SquareSide)));
   check(tag+"_filtered",data.filtered,read(native/(tag+"_stage2.txt"),(2*SquareSide),(4*SquareSide)));
   check(tag+"_inverse",data.inverse,read(native/(label+(chain?"_la_native_inverse.txt":"_native_inverse.txt")),(2*SquareSide),(4*SquareSide)));
   check(tag+"_output",data.output,read(native/(label+(chain?"_adaptation.txt":"_retinal.txt")),SquareSide,SquareSide));
  }
  check(label+"_adapt_input",adaptationInput,read(native/(label+"_adapt_input.txt"),SquareSide,SquareSide));
  if(visual){
   const fs::path expected(argv[7]);const std::string name=label=="ref"?"reference":"test";
   check(label+"_pupil",std::vector<float>(pupil.begin(),pupil.end()),read(expected/(name+"_pupil.txt"),1,4));
   check(label+"_P",P,read(expected/(name+"_P.txt"),SquareSide,SquareSide));
  }
 }
 if(visual){
  const auto mean=gpu.mean_adaptation(adaptations[0],adaptations[1]);const auto log=gpu.log_adaptation(mean);
  check("mean_adaptation",mean,read(fs::path(argv[7])/"mean_adaptation.txt",SquareSide,SquareSide));
  check("log_adaptation",log,read(fs::path(argv[7])/"log_adaptation.txt",SquareSide,SquareSide));
 }
 if(spyr){
  const PathwaySPYRAssets spyrAssets{argv[8],argv[9],argv[10],argv[11],argv[12]};
  const auto pyramid=run_square_spyr_diagnostic(instance,Pmaps[0],Pmaps[1],SquareSide,SquareSide,create_sp0_model(),spyrAssets);
  for(unsigned stage=0;stage<11;++stage){
   const auto& actual=pyramid.stages[stage];std::ifstream file(fs::path(argv[13])/("stage"+std::to_string(stage)+".txt"));unsigned h,w;
   if(!(file>>h>>w)||h!=1||w!=actual.size())throw std::runtime_error("SPYR expected shape");
   std::vector<float> expected;for(unsigned k=0;k<w;++k){uint32_t word;if(!(file>>word))throw std::runtime_error("Truncated SPYR golden");expected.push_back(std::bit_cast<float>(word));}
   std::string extra;if(file>>extra)throw std::runtime_error("Trailing SPYR golden");
   std::vector<float> floats;for(uint32_t word:actual)floats.push_back(std::bit_cast<float>(word));
   check("spyr_stage"+std::to_string(stage),floats,expected);
  }
 }
 if(checks!=(spyr?43u:visual?32u:26u)||values!=(spyr?154u*SquareSide*SquareSide+8u+473u*(SquareSide/8)*(SquareSide/8)+3u:visual?154u*SquareSide*SquareSide+8u:150u*SquareSide*SquareSide))throw std::runtime_error("Incomplete square chain coverage");
 std::ofstream(out/"PASS.txt")<<checks<<" stages/"<<values<<" words bitwise;original images ->GPU MTF ->GPU L+M ->GPU adaptation;visualP/log="<<visual<<";host stage-boundary copies,no CPU intermediate arithmetic;not complete96x96 quality acceptance.\n";
 std::cout<<"Square filter chain PASS:"<<checks<<" stages,"<<values<<" words\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
