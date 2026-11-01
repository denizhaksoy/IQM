#include <vulkan_instance.h>
#include "diagnostic_float_kernel.h"
#include <IQM/hdrvdp3/pathway_runtime.h>
#include <iostream>
namespace fs=std::filesystem;
template<class T>std::vector<T> read(const fs::path& p,size_t n){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()!=std::streamoff(n*sizeof(T)))throw std::runtime_error("Invalid fixture "+p.string());std::vector<T> v(n);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),n*sizeof(T));if(!f)throw std::runtime_error("Read failed");return v;}
int main(int argc,char** argv){try{
 if(argc<4||argc>6)throw std::runtime_error("Usage: native shader fresh-output [raw-spectral-data [RGB-runtime]]");fs::path native(argv[1]),out(argv[3]);if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 IQM::Bin::VulkanInstance instance(true);constexpr unsigned n=96*96;size_t total=0;std::ofstream report(out/"results.tsv");report<<"display\tside\tvalues\tbit_errors\n";
 for(std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"}){
 std::array<std::vector<float>,2> converted;
 for(unsigned side=1;side<=2;++side){std::string tag=display+"_"+std::to_string(side);auto prefix=native/tag;
  auto input=read<float>(prefix.string()+"_input.f32",3*n);
  if(argc>=5){fs::path raw(argv[4]);auto samples=IQM::HDRVDP3::load_xyz_display_samples(raw/"ciexyz31.csv",raw/("emission_spectra_"+display+".csv"));auto m=IQM::HDRVDP3::rgb709_transform(samples);for(unsigned col=0;col<3;++col)for(unsigned row=0;row<3;++row)input.push_back(float(m[row][col]));}
  else for(double v:read<double>(prefix.string()+"_matrix.f64",9))input.push_back(float(v));
  auto expected=read<float>(prefix.string()+"_raw.f32",3*n),clamped=read<float>(prefix.string()+"_clamped.f32",3*n);expected.insert(expected.end(),clamped.begin(),clamped.end());
  auto actual=runFloatKernel(instance,argv[2],input,6*n,3*n,n);size_t errors=0;for(size_t i=0;i<actual.size();++i)errors+=std::bit_cast<uint32_t>(actual[i])!=std::bit_cast<uint32_t>(expected[i]);
  report<<display<<'\t'<<side<<'\t'<<actual.size()<<'\t'<<errors<<'\n';report.flush();std::ofstream dump(out/(tag+".f32"),std::ios::binary);dump.write(reinterpret_cast<const char*>(actual.data()),actual.size()*4);if(errors)throw std::runtime_error("GPU709 bits differ: "+std::to_string(errors));total+=actual.size();
  converted[side-1].resize(3*n);for(unsigned c=0;c<3;++c)for(unsigned y=0;y<96;++y)for(unsigned x=0;x<96;++x)converted[side-1][c*n+y*96+x]=actual[3*n+c*n+x*96+y];
 }
 if(argc==6){using namespace IQM::HDRVDP3;fs::path raw(argv[4]);auto spectra=load_rgb_spectral_samples(raw/"log_cone_smith_pokorny_1975.csv",raw/"cie_scotopic_lum.txt",raw/("emission_spectra_"+display+".csv"));auto assets=load_rgb_pathway_runtime(argv[5],96,96);RGBQualityTrace trace;
  auto scores=evaluate_pathway_rgb_quality(instance,converted[0],converted[1],96,96,spectra,assets,&trace);auto expected=read<float>(native/(display+"_quality.f32"),2);
  std::vector<float> actual{scores.Q,scores.Q_JOD};size_t errors=0;
  for(unsigned i=0;i<2;++i)errors+=std::bit_cast<uint32_t>(actual[i])!=std::bit_cast<uint32_t>(expected[i]);
  for(unsigned image=0;image<2;++image)for(const std::string field:{"adaptation","P"}){auto gold=read<float>(native/(display+"_"+std::to_string(image+1)+"_"+field+".f32"),n);const auto& a=field=="P"?trace.quality.P[image]:trace.quality.adaptation[image];for(unsigned y=0;y<96;++y)for(unsigned x=0;x<96;++x)errors+=std::bit_cast<uint32_t>(a[y*96+x])!=std::bit_cast<uint32_t>(gold[x*96+y]);total+=n;}
  report<<display<<"\tquality/P/adaptation\t"<<4*n+2<<'\t'<<errors<<'\n';report.flush();if(errors)throw std::runtime_error("Independent709 quality/P/adaptation differs");total+=2;
  std::ofstream scoresFile(out/(display+"_scores.f32"),std::ios::binary);scoresFile.write(reinterpret_cast<const char*>(actual.data()),8);
 }
 }
 std::ofstream(out/"PASS.txt")<<total<<" Float32 words bitwise;"<<(argc>=5?"independent raw spectral transform matrix":"native-captured transform matrix")<<(argc==6?";original709 -> GPU conversion/clamp -> productionnativeRGBquality/Q/JOD. Test orchestration,not public709 entry.":";projection/clamp only.")<<'\n';std::cout<<"GPU709 PASS: "<<total<<" words bitwise\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
