#include <vulkan_instance.h>
#include <IQM/hdrvdp3/evaluation.h>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace IQM::HDRVDP3;
static void poison_environment() {
    for(const char* name:{"IQM_FFT_AXIS_ORDER","IQM_FFT_TWIDDLES","IQM_FFT_BACKEND",
        "IQM_FFT_REFERENCE_SHADER","IQM_FFT_REFERENCE_SHADER64","IQM_FFT_MATH_SHADER_DIR",
        "IQM_MAP_BACKEND","IQM_MAP_SHADER","IQM_BASE_FFT_SHADER","IQM_PIX_PER_DEGREE",
        "IQM_OBSERVER_AGE","IQM_SPECTRAL_DATA","IQM_SPECTRAL_SAMPLES",
        "IQM_LUMINANCE_REFERENCE","IQM_LUMINANCE_TEST","IQM_PREPROCESSING"}) {
#ifdef _WIN32
        if(_putenv_s(name,"invalid_environment_must_not_be_read"))throw std::runtime_error("Cannot set test environment");
#else
        if(setenv(name,"invalid_environment_must_not_be_read",1))throw std::runtime_error("Cannot set test environment");
#endif
    }
}
int main(int argc,char** argv) {
    try {
        poison_environment();
        IQM::Bin::VulkanInstance instance(true);
        if(argc==3&&std::string(argv[1])=="--invalid-inputs") {
            const std::filesystem::path root(argv[2]);
            if(std::filesystem::exists(root))throw std::runtime_error("Fresh test directory required");
            LuminanceImage image{32,32,std::vector<float>(1024,10.f)};
            EvaluationOptions options;
            options.fft.referenceShaderPath="not_accessed";
            options.fft.referenceMathDirectory="not_accessed";
            options.mapShader="not_accessed";options.baseFFTShader="not_accessed";
            EvaluationTrace trace{root};
            unsigned rejected=0;
            auto reject=[&](const LuminanceImage& ref,const LuminanceImage& test,const EvaluationOptions& cfg,const EvaluationTrace& output) {
                try {evaluate_quality(instance,ref,test,cfg,output);}
                catch(const std::invalid_argument&){++rejected;return;}
                throw std::runtime_error("Expected invalid_argument");
            };
            auto bad=image;bad.width=31;reject(bad,image,options,trace);
            bad=image;bad.height=64;reject(image,bad,options,trace);
            bad=image;bad.pixels.pop_back();reject(image,bad,options,trace);
            bad=image;bad.pixels[0]=0;reject(bad,image,options,trace);
            bad.pixels[0]=-1;reject(image,bad,options,trace);
            bad.pixels[0]=std::numeric_limits<float>::infinity();reject(bad,image,options,trace);
            bad.pixels[0]=std::numeric_limits<float>::quiet_NaN();reject(image,bad,options,trace);
            auto cfg=options;cfg.pixelsPerDegree=31;reject(image,image,cfg,trace);
            cfg=options;cfg.observerAge=25;reject(image,image,cfg,trace);
            cfg=options;cfg.fft.columnFirst=false;reject(image,image,cfg,trace);
            cfg=options;cfg.fft.useTwiddleLUT=true;reject(image,image,cfg,trace);
            cfg=options;cfg.mapShader.clear();reject(image,image,cfg,trace);
            reject(image,image,options,EvaluationTrace{});
            if(std::filesystem::exists(root))throw std::runtime_error("Invalid inputs created output directory");
            std::filesystem::create_directories(root);
            reject(image,image,options,trace);
            if(rejected!=14)throw std::runtime_error("Incomplete negative cases");
            std::cout<<"API rejected="<<rejected<<"; no GPU dispatch or output overwrite\n";
            return 0;
        }
        if(argc!=12)throw std::invalid_argument("Usage: ref.pfm test.pfm fft.spv math_dir map.spv base_fft.spv spectral_dir fresh_export expected_Q expected_JOD expected_error");
        const auto reference=load_luminance_pfm(argv[1]),test=load_luminance_pfm(argv[2]);
        EvaluationOptions options;options.fft.referenceShaderPath=argv[3];options.fft.referenceMathDirectory=argv[4];
        options.mapShader=argv[5];options.baseFFTShader=argv[6];options.spectralDataDirectory=argv[7];
        const auto result=evaluate_quality(instance,reference,test,options,EvaluationTrace{argv[8]});
        auto equal=[](float actual,const char* expected){return std::bit_cast<uint32_t>(actual)==std::bit_cast<uint32_t>(std::stof(expected));};
        if(!equal(result.scores.Q,argv[9])||!equal(result.scores.Q_JOD,argv[10])||!equal(result.scores.error,argv[11]))
            throw std::runtime_error("API returned scores differ from pinned native reference");
        const size_t bands=reference.width==32?4:5;
        if(result.bandErrors.size()!=bands)throw std::runtime_error("API returned band count mismatch");
        std::cout<<"API returned scores bitwise exact; bands="<<bands<<"; poisoned environment ignored\n";
        return 0;
    } catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
