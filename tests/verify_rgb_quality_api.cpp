#include <IQM/hdrvdp3/rgb_quality.h>
#include <IQM/hdrvdp3/pathway_runtime.h>
#include <vulkan_instance.h>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace fs=std::filesystem;
template<class T>std::vector<T> read(const fs::path& p,size_t n){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()!=std::streamoff(n*sizeof(T)))throw std::runtime_error("Invalid fixture "+p.string());std::vector<T> v(n);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),n*sizeof(T));if(!f)throw std::runtime_error("Read failed");return v;}
std::vector<float> rows(const std::vector<float>& c,unsigned side,unsigned planes=1){std::vector<float> r(c.size());for(unsigned k=0;k<planes;++k)for(unsigned y=0;y<side;++y)for(unsigned x=0;x<side;++x)r[k*side*side+y*side+x]=c[k*side*side+x*side+y];return r;}
int main(int argc,char** argv){try{
 if(argc!=9&&argc!=5)throw std::runtime_error("Usage: native runtime raw-data [projection adaptation JND RGB-comparison] fresh-output");
 const fs::path native(argv[1]),raw(argv[3]),out(argv[argc-1]);if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 using namespace IQM::HDRVDP3;RGBQualityAssets assets;
 if(argc==5)assets=load_rgb_pathway_runtime(argv[2],96,96);
 else{assets={load_pathway_runtime(argv[2],96,96),argv[4],argv[5],argv[6]};assets.quality.comparison=argv[7];}
 IQM::Bin::VulkanInstance instance(true);constexpr unsigned n=96*96;size_t total=0,checks=0;
 std::ofstream log(out/"results.tsv");log<<"stage\tvalues\tbit_errors\n";
 auto check=[&](const std::string& tag,const std::vector<float>& a,const std::vector<float>& e){if(a.size()!=e.size())throw std::runtime_error("Shape mismatch");size_t errors=0;for(size_t i=0;i<a.size();++i)errors+=std::bit_cast<uint32_t>(a[i])!=std::bit_cast<uint32_t>(e[i]);log<<tag<<'\t'<<a.size()<<'\t'<<errors<<'\n';log.flush();std::ofstream dump(out/(tag+".f32"),std::ios::binary);dump.write(reinterpret_cast<const char*>(a.data()),a.size()*4);if(errors)throw std::runtime_error(tag+" mismatches "+std::to_string(errors));total+=a.size();++checks;};
 for(std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"}){
  auto spectra=load_rgb_spectral_samples(raw/"log_cone_smith_pokorny_1975.csv",raw/"cie_scotopic_lum.txt",raw/("emission_spectra_"+display+".csv"));
  auto ref=rows(read<float>(native/(display+"_1_input.f32"),3*n),96,3),test=rows(read<float>(native/(display+"_2_input.f32"),3*n),96,3);
  RGBQualityTrace trace;auto scores=evaluate_pathway_rgb_quality(instance,ref,test,96,96,spectra,assets,&trace);
  for(unsigned image=0;image<2;++image){std::string tag=display+"_"+std::to_string(image+1);auto prefix=native/tag;
   check(tag+"_optical",trace.optical[image],rows(read<float>(prefix.string()+"_optical.f32",3*n),96,3));
   check(tag+"_LMSR",trace.lmsr[image],rows(read<float>(prefix.string()+"_response.f32",4*n),96,4));
   check(tag+"_adaptation_input",trace.adaptationInput[image],rows(read<float>(prefix.string()+"_adaptation_input.f32",n),96));
   check(tag+"_adaptation",trace.quality.adaptation[image],rows(read<float>(prefix.string()+"_adaptation.f32",n),96));
   check(tag+"_P",trace.quality.P[image],rows(read<float>(prefix.string()+"_P.f32",n),96));
   std::ifstream scalar(prefix.string()+"_pupil.txt");double pupil;if(!(scalar>>pupil))throw std::runtime_error("Missing pupil");check(tag+"_pupil",{trace.pupil[image][3]},{float(pupil)});
   auto after=rows(read<float>(prefix.string()+"_post_pupil.f32",4*n),96,4),channels=rows(read<float>(prefix.string()+"_jnd.f32",4*n),96,4);
   for(unsigned c=0;c<3;++c){std::vector<float> a(n),v(n);for(unsigned i=0;i<n;++i){a[i]=trace.jnd[image][i].lmr_after[c];v[i]=trace.jnd[image][i].channels[c];}unsigned sensor=c==2?3:c;check(tag+"_after"+std::to_string(c),a,{after.begin()+sensor*n,after.begin()+(sensor+1)*n});check(tag+"_jnd"+std::to_string(c),v,{channels.begin()+c*n,channels.begin()+(c+1)*n});}
  }
  check(display+"_quality",{scores.Q,scores.Q_JOD},read<float>(native/(display+"_quality.f32"),2));std::vector<float> errors;
  for(unsigned b=0;b<5;++b){unsigned side=b<2?96:96/(1u<<(b-1));check(display+"_band"+std::to_string(b+1),trace.quality.masking.bands[b].masked,rows(read<float>(native/(display+"_band"+std::to_string(b+1)+"_masked.f32"),side*side),side));errors.push_back(trace.quality.masking.bands[b].error);}
  errors.push_back(scores.error);check(display+"_errors",errors,read<float>(native/(display+"_errors.f32"),6));
 }
 if(total!=949134)throw std::runtime_error("Coverage mismatch");std::ofstream(out/"PASS.txt")<<checks<<" checks / "<<total<<" Float32 words bitwise. Production RGB API; three spectral configurations of one RGB96 pair.\n";std::cout<<"RGB production API PASS "<<checks<<" checks / "<<total<<" words\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
