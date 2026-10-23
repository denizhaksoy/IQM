// Test-only caller;public evaluate_quality keeps its square-only contract.
#include <vulkan_instance.h>
#include <IQM/hdrvdp3/evaluation.h>
#include <bit>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
namespace IQM::HDRVDP3 {
EvaluationResult diagnostic_quality_rectangular(const IQM::VulkanInstance&,
    const LuminanceImage&,const LuminanceImage&,const EvaluationOptions&,const EvaluationTrace&);
}
int main(int argc,char** argv){try{
    if(argc!=11)throw std::runtime_error("Usage:ref.pfm test.pfm fft64256.spv math-dir map16.spv base85.spv spectral-dir fresh-export native-Q native-JOD");
    using namespace IQM::HDRVDP3;const auto ref=load_luminance_pfm(argv[1]),test=load_luminance_pfm(argv[2]);
    if(!((ref.width==128&&ref.height==64)||(ref.width==64&&ref.height==128)))throw std::runtime_error("Rectangular diagnostic dimensions required");
    EvaluationOptions options;options.fft.referenceShaderPath=argv[3];options.fft.referenceMathDirectory=argv[4];
    options.mapShader=argv[5];options.baseFFTShader=argv[6];options.spectralDataDirectory=argv[7];
    IQM::Bin::VulkanInstance instance(true);
    const auto result=diagnostic_quality_rectangular(instance,ref,test,options,EvaluationTrace{argv[8]});
    auto equal=[](float value,const char* expected){return std::bit_cast<uint32_t>(value)==std::bit_cast<uint32_t>(std::stof(expected));};
    if(!equal(result.scores.Q,argv[9])||!equal(result.scores.Q_JOD,argv[10]))throw std::runtime_error("Rectangular Q/Q_JOD differ from official reference");
    if(result.bandErrors.size()!=5)throw std::runtime_error("Rectangular30ppd requires5 bands");
    std::cout<<std::setprecision(17)<<"Rectangular diagnostic Q="<<result.scores.Q<<" Q_JOD="<<result.scores.Q_JOD<<";native bits exact;all-stage checks still required\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
