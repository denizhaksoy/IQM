#include <vulkan_instance.h>
#include <IQM/hdrvdp3/projection_single_gpu.h>
#include <IQM/base/vulkan_runtime.h>
#include <bit>
#include <fstream>
#include <IQM/hdrvdp3/spectral.h>
#include <iostream>
#include <filesystem>
namespace fs=std::filesystem;using namespace IQM::HDRVDP3;
std::vector<float> load(const fs::path& p,size_t n){if(fs::file_size(p)!=n*4)throw std::runtime_error("Input tensor shape");std::vector<float> v(n);std::ifstream f(p,std::ios::binary);f.read(reinterpret_cast<char*>(v.data()),n*4);if(!f)throw std::runtime_error("Tensor read failed");return v;}
int main(int argc,char**argv){try{
 if(argc!=7&&argc!=8)throw std::runtime_error("Usage: independent-optical-root official-capture-root model-data shader 3840x2160 fresh-out [linear-root]");
 // This diagnostic intentionally locks the accepted 3840x2160 geometry.
 fs::path independent=argv[1],official=argv[2],data=argv[3],shader=argv[4],out=argv[6];
 if(std::string(argv[5])!="3840x2160"||fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Requires 3840x2160 and fresh output");
 constexpr uint32_t w=3840,h=2160,n=w*h;auto samples=load_rgb_spectral_samples(data/"log_cone_smith_pokorny_1975.csv",data/"cie_scotopic_lum.txt",data/"emission_spectra_oled.csv");auto calibration=calibrate_rgb_spectral_age(samples,24.);
 IQM::Bin::VulkanInstance instance(true);const auto properties=instance.physicalDevice()->getProperties();std::ofstream(out/"device.txt")<<properties.deviceName.data()<<'\n';std::ofstream result(out/"results.jsonl");uint64_t total=0;
 auto compare=[&](const char* side,const char* stage,const std::vector<float>& actual,const std::vector<float>& reference,unsigned channels){uint64_t errors=0;for(unsigned c=0;c<channels;++c)for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x)errors+=std::bit_cast<uint32_t>(actual[size_t(c)*n+size_t(y)*w+x])!=std::bit_cast<uint32_t>(reference[size_t(c)*n+size_t(x)*h+y]);result<<"{\"stage\":\""<<stage<<"\",\"side\":\""<<side<<"\",\"values\":"<<channels*n<<",\"bit_mismatches\":"<<errors<<"}\n";result.flush();std::cout<<side<<' '<<stage<<": "<<errors<<" bit differences / "<<channels*n<<'\n';if(errors)throw std::runtime_error("GPU matrix stage not bitwise equal");total+=channels*n;};
 for(const char* side:{"reference","test"}){
 if(argc==8){auto input=load(fs::path(argv[7])/(std::string(side)+".f32"),3*n);auto xyz=load_xyz_display_samples(data/"ciexyz31.csv",data/"emission_spectra_oled.csv");auto color=linear_color_transform(xyz,LinearColorEncoding::BT2020);std::vector<float> coefficients;for(unsigned c=0;c<3;++c)for(unsigned p=0;p<3;++p)coefficients.push_back(float(color[c][p]));auto expected=load(official/(std::string(side)+"_native.f32"),3*n);auto actual=project_rgb_single_gpu(instance,shader.string(),input,coefficients,SingleProjectionKind::NativeColor);compare(side,"native",actual,expected,3);}
 auto input=load(independent/side/"optical.f32",3*n);auto reference=load(official/(std::string(side)+"_lmsr.f32"),4*n);
 std::vector<float> coefficients;for(unsigned c=0;c<4;++c)for(unsigned p=0;p<3;++p)coefficients.push_back(float(calibration.matrix[p][c]));
 auto actual=project_rgb_single_gpu(instance,shader.string(),input,coefficients,SingleProjectionKind::LMSR);compare(side,"lmsr",actual,reference,4);
 }
 std::ofstream(out/"PASS.txt")<<total<<" Single GPU matrix values bitwise equal; stage-only acceptance, not full GPU pipeline.\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
