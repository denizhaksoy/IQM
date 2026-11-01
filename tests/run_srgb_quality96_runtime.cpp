#include <IQM/hdrvdp3/srgb_quality96_runtime_double.h>
#include <vulkan_instance.h>
#include <fstream>
#include <iostream>
#include <iomanip>
using namespace IQM::HDRVDP3;namespace fs=std::filesystem;
std::vector<float> image(const fs::path&p){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()!=27648*4)throw std::runtime_error("Expected96x96 planar encodedRGB f32");std::vector<float> v(27648);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),v.size()*4);return v;}
int main(int argc,char**argv){try{
 if(argc==4&&std::string(argv[1])=="--load-only"){auto runtime=load_srgb_quality96_runtime_double(argv[2],argv[3]);std::cout<<"Runtime loaded\n";return 0;}
 if(argc!=6)throw std::invalid_argument("Usage: runtime display reference.f32 test.f32 fresh-output");auto runtime=load_srgb_quality96_runtime_double(argv[1],argv[2]);fs::path out=argv[5];if(fs::exists(out)||!fs::create_directories(out))throw std::invalid_argument("Fresh output required");auto r=image(argv[3]),t=image(argv[4]);IQM::Bin::VulkanInstance instance(true,"NVIDIA",true);SRGBQuality96GPUDouble gpu(*instance.physicalDevice(),*instance.device(),*instance.queue(),instance.queueFamilyIndex,instance.float64Enabled,runtime.shaders);auto scores=evaluate_srgb_quality96_double(r,t,runtime.xyz,runtime.spectral,gpu.backend()).scores;std::ofstream f(out/"scores.json");f<<std::setprecision(17)<<"{\"error\":"<<scores.error<<",\"Q\":"<<scores.Q<<",\"Q_JOD\":"<<scores.Q_JOD<<"}\n";std::cout<<std::setprecision(17)<<"Q="<<scores.Q<<" Q_JOD="<<scores.Q_JOD<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
