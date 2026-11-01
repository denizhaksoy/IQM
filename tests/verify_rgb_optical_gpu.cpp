#include <vulkan_instance.h>
#include <IQM/hdrvdp3/mixed_filter.h>
#include <IQM/hdrvdp3/spectral.h>
#include <IQM/hdrvdp3/pathway_runtime.h>
#include "diagnostic_float_kernel.h"
#include <iostream>
namespace fs=std::filesystem;
template<class T> std::vector<T> readCapture(const fs::path& p,size_t count){
 std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()!=std::streamoff(count*sizeof(T)))throw std::runtime_error("Invalid capture "+p.string());
 std::vector<T> v(count);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),count*sizeof(T));if(!f)throw std::runtime_error("Read failed");return v;
}
int main(int argc,char** argv){try{
 if(argc<6||argc>10||argc==9)throw std::runtime_error("Usage: native-captures runtime-assets spectral-data projection-shader fresh-output [adaptation-shader [RGB-map-shader [runtime RGB-comparison-shader]]]");
 fs::path root(argv[1]),assets(argv[2]),raw(argv[3]),out(argv[5]);if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 using namespace IQM::HDRVDP3;IQM::Bin::VulkanInstance instance(true);constexpr unsigned n=96*96;size_t total=0;
 MixedFilterAssets filter{assets/"forward.spv",assets/"inverse.spv",assets/"io.spv"};
 std::ofstream log(out/"results.tsv");log<<"stage\tvalues\tbit_errors\n";
 auto check=[&](const std::string& tag,const std::vector<float>& actual,const std::vector<float>& expected){size_t errors=0;if(actual.size()!=expected.size())throw std::runtime_error("Shape mismatch");for(size_t i=0;i<actual.size();++i)errors+=std::bit_cast<uint32_t>(actual[i])!=std::bit_cast<uint32_t>(expected[i]);log<<tag<<'\t'<<actual.size()<<'\t'<<errors<<'\n';log.flush();std::ofstream dump(out/(tag+".f32"),std::ios::binary);dump.write(reinterpret_cast<const char*>(actual.data()),actual.size()*4);if(errors)throw std::runtime_error(tag+" bit_errors="+std::to_string(errors));total+=actual.size();};
 // Invalid inputs must fail before any shader/command-buffer work.
 for(auto mode:{MixedFilterMode::OpticalMTF,MixedFilterMode::LocalAdaptation,MixedFilterMode::OpticalRGB}){
  std::vector<float> invalid(n,1);invalid[0]=mode==MixedFilterMode::OpticalRGB?-1:0;bool rejected=false;
  try{run_mixed_filter(instance,invalid,96,96,mode,filter);}catch(const std::invalid_argument&){rejected=true;}
  if(!rejected)throw std::runtime_error("Input domain guard failed");
 }
 for(std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"}){
  std::array<std::vector<float>,2> originalRows,computedP,computedAdaptation;
  auto samples=load_rgb_spectral_samples(raw/"log_cone_smith_pokorny_1975.csv",raw/"cie_scotopic_lum.txt",raw/("emission_spectra_"+display+".csv"));auto calibration=calibrate_rgb_spectral_age(samples,24);
  for(int side=1;side<=2;++side){std::string tag=display+"_"+std::to_string(side);auto prefix=root/tag;auto input=readCapture<float>(prefix.string()+"_input.f32",3*n);auto opticalExpected=readCapture<float>(prefix.string()+"_optical.f32",3*n);std::vector<float> optical;
   originalRows[side-1].resize(3*n);for(unsigned c=0;c<3;++c)for(unsigned y=0;y<96;++y)for(unsigned x=0;x<96;++x)originalRows[side-1][c*n+y*96+x]=input[c*n+x*96+y];
   for(unsigned channel=0;channel<3;++channel){std::vector<float> plane(n);for(unsigned y=0;y<96;++y)for(unsigned x=0;x<96;++x)plane[y*96+x]=input[channel*n+x*96+y];auto result=run_mixed_filter(instance,plane,96,96,MixedFilterMode::OpticalRGB,filter);std::vector<float> column(n);for(unsigned y=0;y<96;++y)for(unsigned x=0;x<96;++x)column[x*96+y]=result.output[y*96+x];check(tag+"_optical"+std::to_string(channel),column,std::vector<float>(opticalExpected.begin()+channel*n,opticalExpected.begin()+(channel+1)*n));optical.insert(optical.end(),column.begin(),column.end());}
   for(unsigned sensor=0;sensor<4;++sensor)for(unsigned channel=0;channel<3;++channel)optical.push_back(float(calibration.matrix[channel][sensor]));
   auto response=runFloatKernel(instance,argv[4],optical,4*n,4*n,n);check(tag+"_LMSR",response,readCapture<float>(prefix.string()+"_response.f32",4*n));
   if(argc>=7){
    auto sum=runFloatKernel(instance,argv[6],response,n,n,n);check(tag+"_adaptation_input",sum,readCapture<float>(prefix.string()+"_adaptation_input.f32",n));
    std::vector<float> row(n);for(unsigned y=0;y<96;++y)for(unsigned x=0;x<96;++x)row[y*96+x]=sum[x*96+y];
    auto adaptation=run_mixed_filter(instance,row,96,96,MixedFilterMode::LocalAdaptation,filter);
    std::vector<float> column(n);for(unsigned y=0;y<96;++y)for(unsigned x=0;x<96;++x)column[x*96+y]=adaptation.output[y*96+x];
    check(tag+"_adaptation",column,readCapture<float>(prefix.string()+"_adaptation.f32",n));
    if(argc>=8){
     MapGPU map(instance,argv[7],MapGPUProfile::RGBResponse);
     auto pupil=map.pupil_trace(adaptation.output,96,96,double(n)/900,24);
     std::ifstream scalar(prefix.string()+"_pupil.txt");double expectedPupil;if(!(scalar>>expectedPupil))throw std::runtime_error("Missing pupil");
     check(tag+"_pupil",{pupil[3]},{float(expectedPupil)});
     auto jnd=map.jnd_response_lmsr(response,create_jnd_lut(),pupil[3]);
     auto afterExpected=readCapture<float>(prefix.string()+"_post_pupil.f32",4*n);
     auto jndExpected=readCapture<float>(prefix.string()+"_jnd.f32",4*n);
     for(unsigned c=0;c<3;++c){std::vector<float> after(n),channel(n);for(unsigned i=0;i<n;++i){after[i]=jnd[i].lmr_after[c];channel[i]=jnd[i].channels[c];}unsigned native=c==2?3:c;
      check(tag+"_post_pupil"+std::to_string(c),after,std::vector<float>(afterExpected.begin()+native*n,afterExpected.begin()+(native+1)*n));
      check(tag+"_jnd"+std::to_string(c),channel,std::vector<float>(jndExpected.begin()+c*n,jndExpected.begin()+(c+1)*n));
     }
     std::vector<float> P;for(const auto& v:jnd)P.push_back(v.response);check(tag+"_P",P,readCapture<float>(prefix.string()+"_P.f32",n));
     computedAdaptation[side-1]=adaptation.output;computedP[side-1].resize(n);
     for(unsigned y=0;y<96;++y)for(unsigned x=0;x<96;++x)computedP[side-1][y*96+x]=P[x*96+y];
    }
   }
  }
  if(argc==10){auto qualityAssets=load_pathway_runtime(argv[8],96,96);qualityAssets.comparison=argv[9];PathwayQualityTrace trace;
   auto scores=evaluate_pathway_rgb_response_quality(instance,originalRows[0],originalRows[1],computedP,computedAdaptation,96,96,qualityAssets,&trace);
   check(display+"_quality",{scores.Q,scores.Q_JOD},readCapture<float>(root/(display+"_quality.f32"),2));
   std::vector<float> errors;
   for(unsigned band=0;band<5;++band){unsigned size=band<2?96:96/(1u<<(band-1));auto native=readCapture<float>(root/(display+"_band"+std::to_string(band+1)+"_masked.f32"),size*size);std::vector<float> row(size*size);for(unsigned y=0;y<size;++y)for(unsigned x=0;x<size;++x)row[y*size+x]=native[x*size+y];check(display+"_band"+std::to_string(band+1)+"_masked",trace.masking.bands[band].masked,row);errors.push_back(trace.masking.bands[band].error);}
   errors.push_back(scores.error);check(display+"_band_errors",errors,readCapture<float>(root/(display+"_errors.f32"),6));
  }
 }
 std::ofstream(out/"PASS.txt")<<total<<" Float32 words bitwise; original RGB -> GPU optical/LMSR"
    <<(argc>=7?" -> adaptation":"")<<(argc>=8?" -> pupil/JND/P":"")<<(argc==10?" -> Q/Q_JOD.":". Not final quality-score acceptance.")<<'\n';
 std::cout<<"RGB chain PASS: "<<total<<" words bitwise\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
