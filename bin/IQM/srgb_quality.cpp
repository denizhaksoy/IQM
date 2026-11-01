#include "srgb_quality.h"
#include "vulkan_instance.h"
#include <IQM/hdrvdp3/srgb_quality96_runtime_double.h>
#include <IQM/hdrvdp3/rgb_image.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <chrono>
namespace IQM::Bin {
int run_srgb_quality96_cli(const Args& args){using namespace IQM::HDRVDP3;namespace fs=std::filesystem;const auto& cli=*args.hdrvdp3Quality;
 auto reference=load_rgb_pfm(args.refPath),test=load_rgb_pfm(args.inputPath);if(reference.width!=96||reference.height!=96||test.width!=96||test.height!=96)throw std::invalid_argument("sRGB quality supports96x96 colorPF inputs only");
 auto column=[](const RGBImage& im){std::vector<float> v(27648);for(size_t c=0;c<3;++c)for(size_t x=0;x<96;++x)for(size_t y=0;y<96;++y){float a=im.pixels[c*9216+y*96+x];if(a>1)throw std::invalid_argument("sRGB quality requires encoded RGB in[0,1]");v[c*9216+x*96+y]=a;}return v;};auto r=column(reference),t=column(test);
 const fs::path output=cli.exportDirectory?fs::path(*cli.exportDirectory):fs::path("validation")/("srgb_quality_cli_"+std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));if(fs::exists(output))throw std::invalid_argument("Fresh sRGB export directory required");auto runtime=load_srgb_quality96_runtime_double(cli.runtimeDirectory,*cli.rgbDisplay);
 VulkanInstance instance(args.enableValidation,cli.device.value_or(""),true);SRGBQuality96GPUDouble gpu(*instance.physicalDevice(),*instance.device(),*instance.queue(),instance.queueFamilyIndex,instance.float64Enabled,runtime.shaders,runtime.projection,runtime.mutual,runtime.neighbor,runtime.pyramid,runtime.comparison,runtime.jnd,runtime.csf,runtime.quality,runtime.logarithm);auto result=evaluate_srgb_quality96_double(r,t,runtime.xyz,runtime.spectral,gpu.backend());if(!fs::create_directories(output))throw std::runtime_error("Cannot create sRGB export directory");
 std::ofstream quality(output/"quality.txt");quality<<std::setprecision(17);for(auto value:result.band_errors)quality<<value<<' ';quality<<'\n'<<result.scores.error<<' '<<result.scores.Q<<' '<<result.scores.Q_JOD<<'\n';if(!quality)throw std::runtime_error("Cannot write sRGB quality");
 for(auto item:{std::pair{"reference_rgb",&reference.pixels},std::pair{"test_rgb",&test.pixels}}){std::ofstream f(output/(std::string(item.first)+".f32"),std::ios::binary);f.write(reinterpret_cast<const char*>(item.second->data()),item.second->size()*4);if(!f)throw std::runtime_error("Cannot write sRGB input export");}
 std::ofstream profile(output/"profile.txt");profile<<"IQM_HDRVDP3_DOUBLE96_CLI 1\nencoding srgb-display\ndisplay "<<*cli.rgbDisplay<<"\ndimensions96x96\nppd30\nage24\ninput-export-layout planar-row-major-f32\nFFT GPU Float64;remaining visual/quality CPU\nGPU spectral-projection "<<(!runtime.projection.empty()?"yes":"no")<<"\nGPU mutual-masking "<<(!runtime.mutual.empty()?"yes":"no")<<"\nGPU neighbor-masking "<<(!runtime.neighbor.empty()?"yes":"no")<<"\nGPU pyramid "<<(!runtime.pyramid.empty()?"yes":"no")<<"\nGPU comparison "<<(!runtime.comparison.empty()?"yes":"no")<<"\nGPU JND-linear "<<(!runtime.jnd.empty()?(!runtime.logarithm.empty()?"yes;GPU log queries;CPU reduction/exp/pupil/slopes":"yes;CPU log/exp queries"):"no")<<"\nGPU CSF-linear "<<(!runtime.csf.empty()?"yes;CPU model/base nonlinear scalar":"no")<<"\nGPU quality-arithmetic "<<(!runtime.quality.empty()?"yes;CPU pow/psychometric/final scalar":"no")<<"\nGPU logarithm "<<(!runtime.logarithm.empty()?"yes;JND adaptation/query logs;CPU exp/pow retained":"no")<<"\ndevice "<<instance.selectedDevice<<"\nruntime "<<fs::absolute(cli.runtimeDirectory).string()<<'\n';if(!profile)throw std::runtime_error("Cannot write sRGB profile");std::cout<<std::setprecision(17)<<"Quality API returned Q="<<result.scores.Q<<" Q_JOD="<<result.scores.Q_JOD<<"\nQuality export directory: "<<fs::absolute(output).string()<<'\n';return 0;
}
}
