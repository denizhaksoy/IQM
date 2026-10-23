// Built only by the explicit isolated128 diagnostic CMake option.
#include <vulkan_instance.h>
#include <IQM/hdrvdp3/evaluation.h>
#include <bit>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv){try{
    if(argc!=11)throw std::runtime_error("Usage:ref.pfm test.pfm fft.spv math_dir map14.spv base82.spv spectral_dir fresh_export native_Q native_JOD");
    using namespace IQM::HDRVDP3;
    const auto ref=load_luminance_pfm(argv[1]),test=load_luminance_pfm(argv[2]);
    if(ref.width!=128||ref.height!=128||test.width!=128||test.height!=128)throw std::runtime_error("Diagnostic128 inputs required");
    EvaluationOptions options;options.fft.referenceShaderPath=argv[3];options.fft.referenceMathDirectory=argv[4];
    options.mapShader=argv[5];options.baseFFTShader=argv[6];options.spectralDataDirectory=argv[7];
    IQM::Bin::VulkanInstance instance(true);const auto result=evaluate_quality(instance,ref,test,options,EvaluationTrace{argv[8]});
    auto equal=[](float value,const char* expected){return std::bit_cast<uint32_t>(value)==std::bit_cast<uint32_t>(std::stof(expected));};
    if(!equal(result.scores.Q,argv[9])||!equal(result.scores.Q_JOD,argv[10]))throw std::runtime_error("Diagnostic128 Q/Q_JOD differ from official native reference");
    if(result.bandErrors.size()!=5)throw std::runtime_error("Diagnostic128/30ppd requires5 bands and16-square residual");
    std::cout<<std::setprecision(9)<<"Diagnostic128 Q="<<result.scores.Q<<" Q_JOD="<<result.scores.Q_JOD<<" error="<<result.scores.error<<"; Q/Q_JOD native bits exact; all-stage validation still required\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
