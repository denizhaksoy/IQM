#include <IQM/hdrvdp3/pathway_runtime.h>
#include <vulkan_instance.h>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace fs=std::filesystem;
std::vector<float> read(const fs::path& p,size_t n){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()!=std::streamoff(n*4))throw std::runtime_error("Invalid fixture "+p.string());std::vector<float> v(n);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),n*4);return v;}
std::vector<float> rows(std::vector<float> col,unsigned planes){std::vector<float> out(col.size());for(unsigned c=0;c<planes;++c)for(unsigned y=0;y<96;++y)for(unsigned x=0;x<96;++x)out[c*9216+y*96+x]=col[c*9216+x*96+y];return out;}
int main(int argc,char** argv){try{
 if(argc!=6&&argc!=7)throw std::runtime_error("Usage: native RGB-runtime spectral-data conversion-shader fresh-output [encoding]");fs::path native(argv[1]),raw(argv[3]),out(argv[5]);if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");using namespace IQM::HDRVDP3;
 LinearColorEncoding encoding=LinearColorEncoding::BT709;if(argc==7){std::string name=argv[6];if(name=="rgb-bt.2020")encoding=LinearColorEncoding::BT2020;else if(name=="xyz")encoding=LinearColorEncoding::XYZ;else if(name!="rgb-bt.709")throw std::runtime_error("Unknown encoding");}
 auto assets=load_rgb_pathway_runtime(argv[2],96,96);IQM::Bin::VulkanInstance instance(true);size_t total=0;std::ofstream log(out/"results.tsv");log<<"stage\tvalues\tbit_errors\n";
 auto check=[&](const std::string& tag,const std::vector<float>& a,const std::vector<float>& e){if(a.size()!=e.size())throw std::runtime_error("Shape mismatch");size_t errors=0;for(size_t i=0;i<a.size();++i)errors+=std::bit_cast<uint32_t>(a[i])!=std::bit_cast<uint32_t>(e[i]);log<<tag<<'\t'<<a.size()<<'\t'<<errors<<'\n';log.flush();std::ofstream f(out/(tag+".f32"),std::ios::binary);f.write(reinterpret_cast<const char*>(a.data()),a.size()*4);if(errors)throw std::runtime_error(tag+" differs");total+=a.size();};
 for(std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"}){auto emission=raw/("emission_spectra_"+display+".csv");auto xyz=load_xyz_display_samples(raw/"ciexyz31.csv",emission);auto spectra=load_rgb_spectral_samples(raw/"log_cone_smith_pokorny_1975.csv",raw/"cie_scotopic_lum.txt",emission);
  auto ref=rows(read(native/(display+"_1_input.f32"),3*9216),3),test=rows(read(native/(display+"_2_input.f32"),3*9216),3);RGB709QualityTrace trace;
  auto score=argc==6?evaluate_pathway_rgb709_quality(instance,ref,test,96,96,xyz,spectra,assets,argv[4],&trace):evaluate_pathway_linear_color_quality(instance,ref,test,96,96,xyz,spectra,assets,argv[4],encoding,&trace);
  for(unsigned image=0;image<2;++image){auto tag=display+"_"+std::to_string(image+1),prefix=(native/tag).string();check(tag+"_raw",trace.raw[image],rows(read(prefix+"_raw.f32",3*9216),3));check(tag+"_clamped",trace.converted[image],rows(read(prefix+"_clamped.f32",3*9216),3));check(tag+"_P",trace.native.quality.P[image],rows(read(prefix+"_P.f32",9216),1));check(tag+"_adaptation",trace.native.quality.adaptation[image],rows(read(prefix+"_adaptation.f32",9216),1));}
  check(display+"_scores",{score.Q,score.Q_JOD},read(native/(display+"_quality.f32"),2));
  if(argc==7){std::vector<float> errors;for(const auto& band:trace.native.quality.masking.bands)errors.push_back(band.error);errors.push_back(score.error);check(display+"_errors",errors,read(native/(display+"_errors.f32"),6));}
 }
 if(total!=(argc==7?442392u:442374u))throw std::runtime_error("Coverage mismatch");std::ofstream(out/"PASS.txt")<<total<<" Float32 words bitwise;production linear-color API encoding="<<(argc==7?argv[6]:"rgb-bt.709")<<";originalinput->independentmatrix->GPUconversion/clamp->Q/JOD.\n";std::cout<<"Production linear-color API PASS "<<total<<" words bitwise\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
