#include <fstream>
#include <iostream>
#include <filesystem>
#include <stdexcept>
#include <IQM/hdrvdp3/srgb_quality96_double.h>
#include <IQM/hdrvdp3/srgb_quality96_gpu_double.h>
#include <vulkan_instance.h>
#include <bit>
#include <cmath>
#include <limits>
#include <iomanip>
namespace fs=std::filesystem;using namespace IQM::HDRVDP3;
template<class T>std::vector<T> read(const fs::path&p,size_t n){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()!=std::streamoff(n*sizeof(T)))throw std::runtime_error("Input shape "+p.string());std::vector<T> v(n);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),n*sizeof(T));return v;}
int main(int argc,char**argv){try{
 if(argc!=7)throw std::runtime_error("Usage: cpu-root encoded-root chain-root base-root raw-data fresh-output");fs::path cpu=argv[1],encoded=argv[2],chain=argv[3],base=argv[4],data=argv[5],out=argv[6];if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 const std::array<fs::path,6> shaders{chain/"dependency_shaders/forward-first.spv",chain/"dependency_shaders/forward-second.spv",chain/"multiply_pack/multiply.spv",chain/"dependency_shaders/inverse-first.spv",chain/"dependency_shaders/inverse-second.spv",chain/"crop_pack/multiply.spv"};SRGBQuality96GPUDouble::Shaders code;std::array<fs::path,7> shaderPaths;for(size_t i=0;i<6;++i)shaderPaths[i]=shaders[i];shaderPaths[6]=base/"repro/fft12_double.spv";
 for(size_t i=0;i<7;++i){auto n=fs::file_size(shaderPaths[i]);if(n%4)throw std::runtime_error("Shader size");code[i]=read<uint32_t>(shaderPaths[i],n/4);}
 IQM::Bin::VulkanInstance instance(true,"NVIDIA",true);SRGBQuality96GPUDouble gpu(*instance.physicalDevice(),*instance.device(),*instance.queue(),instance.queueFamilyIndex,instance.float64Enabled,code);
 size_t words=0;auto compare=[&](std::span<const double>a,std::span<const double>b){if(a.size()!=b.size())throw std::runtime_error("Comparison shape");for(size_t i=0;i<a.size();++i)if(std::bit_cast<uint64_t>(a[i])!=std::bit_cast<uint64_t>(b[i]))throw std::runtime_error("API bit mismatch at "+std::to_string(i));words+=a.size();};
 std::ofstream log(out/"scores.tsv");log<<"pair\tdisplay\tQ\tQ_JOD\n"<<std::setprecision(17);
 for(std::string tag:{"stress","ramps","identity"})for(std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"}){
  fs::path native=cpu/(tag=="stress"?"native":tag=="ramps"?"native_ramps":"native_identity_v2"),inputs=tag=="identity"?cpu/"input_identity":encoded/(tag=="stress"?"native_stress":"native");
  auto r=read<float>(inputs/(display+"_1_input.f32"),27648),t=read<float>(inputs/(display+"_2_input.f32"),27648);auto xyz=load_xyz_display_samples(data/"ciexyz31.csv",data/("emission_spectra_"+display+".csv"));auto spectral=load_rgb_spectral_samples(data/"log_cone_smith_pokorny_1975.csv",data/"cie_scotopic_lum.txt",data/("emission_spectra_"+display+".csv"));unsigned calls=0,baseCalls=0;
  SRGBQuality96BackendDouble backend;
  backend.convolve=[&](std::span<const double>padded,std::span<const double>filter){const auto prefix=native/(display+"_"+std::to_string(calls/4+1)+"_conv"+std::to_string(calls%4+1));++calls;compare(padded,read<double>(prefix.string()+"_padded.f64",36864));compare(filter,read<double>(prefix.string()+"_filter.f64",36864));auto result=gpu.convolve(padded,filter,true);compare(result.inverse,read<double>(prefix.string()+"_inverse.f64",36864));compare(result.cropped,read<double>(prefix.string()+"_output.f64",9216));compare(result.spectrum,read<double>(prefix.string()+"_fft.f64",73728));return result.cropped;};
  backend.base_fft=[&](std::span<const double>v){++baseCalls;auto result=gpu.base_fft(v);compare(result,read<double>(native/(display+"_quality_band5_base_fft.f64"),288));return result;};
  auto result=evaluate_srgb_quality96_double(r,t,xyz,spectral,backend);if(calls!=8||baseCalls!=1)throw std::runtime_error("Backend call coverage");compare(std::array<double,3>{result.scores.error,result.scores.Q,result.scores.Q_JOD},read<double>(native/(display+"_quality.f64"),3));for(unsigned b=0;b<5;++b)compare(std::array<double,1>{result.band_errors[b]},read<double>(native/(display+"_quality_band"+std::to_string(b+1)+"_error.f64"),1));log<<tag<<'\t'<<display<<'\t'<<result.scores.Q<<'\t'<<result.scores.Q_JOD<<'\n';log.flush();std::cout<<tag<<' '<<display<<" scores exact\n";
 }
 auto xyz=load_xyz_display_samples(data/"ciexyz31.csv",data/"emission_spectra_oled.csv");auto spectral=load_rgb_spectral_samples(data/"log_cone_smith_pokorny_1975.csv",data/"cie_scotopic_lum.txt",data/"emission_spectra_oled.csv");std::vector<float> valid(27648,.5f);unsigned invalidChecks=0,calls=0;
 SRGBQuality96BackendDouble backend;backend.convolve=[&](auto,auto){++calls;return std::vector<double>(9216,1.);};backend.base_fft=[](auto){return std::vector<double>(288,0.);};
 auto reject=[&](auto action){bool rejected=false;try{action();}catch(const std::invalid_argument&){rejected=true;}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("Invalid API input accepted");++invalidChecks;};
 reject([&]{evaluate_srgb_quality96_double({},valid,xyz,spectral,backend);});auto bad=valid;bad.pop_back();reject([&]{evaluate_srgb_quality96_double(valid,bad,xyz,spectral,backend);});
 for(float value:{-1.f,1.01f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){bad=valid;bad[9]=value;reject([&]{evaluate_srgb_quality96_double(bad,valid,xyz,spectral,backend);});}
 reject([&]{evaluate_srgb_quality96_double(valid,valid,xyz,spectral,{});});if(calls)throw std::runtime_error("Invalid images reached backend");
 backend.convolve=[](auto,auto){return std::vector<double>();};reject([&]{evaluate_srgb_quality96_double(valid,valid,xyz,spectral,backend);});backend.convolve=[](auto,auto){return std::vector<double>(9216,std::numeric_limits<double>::infinity());};reject([&]{evaluate_srgb_quality96_double(valid,valid,xyz,spectral,backend);});
 reject([&]{SRGBQuality96GPUDouble unsupported(*instance.physicalDevice(),*instance.device(),*instance.queue(),instance.queueFamilyIndex,false,code);});
 reject([&]{SRGBQuality96GPUDouble wrongFamily(*instance.physicalDevice(),*instance.device(),*instance.queue(),std::numeric_limits<uint32_t>::max(),true,code);});
 auto broken=code;broken[0].clear();reject([&]{SRGBQuality96GPUDouble wrongShader(*instance.physicalDevice(),*instance.device(),*instance.queue(),instance.queueFamilyIndex,true,broken);});
 std::vector<double> padded(36864,0.),filter(36864,1.),baseValues(288,0.);
 reject([&]{gpu.convolve({},filter);});padded[0]=std::numeric_limits<double>::quiet_NaN();reject([&]{gpu.convolve(padded,filter);});padded[0]=0.;reject([&]{gpu.convolve(padded,{});});filter[0]=std::numeric_limits<double>::infinity();reject([&]{gpu.convolve(padded,filter);});reject([&]{gpu.base_fft({});});baseValues[0]=std::numeric_limits<double>::infinity();reject([&]{gpu.base_fft(baseValues);}); std::ofstream(out/"PASS.txt")<<words<<" Double words bitwise exact;9 production GPU backend API configurations;"<<invalidChecks<<" rejection checks;API calculation has no native file access\n";std::cout<<words<<" exact words;"<<invalidChecks<<" rejection checks\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
