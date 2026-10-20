#pragma once
#include <IQM/hdrvdp3/csf.h>
#include <IQM/hdrvdp3/quality.h>
#include <IQM/hdrvdp3/jnd.h>
#include <array>
#include <memory>
#include <string>
#include <vector>
namespace IQM { class VulkanInstance; }
namespace IQM::HDRVDP3 {
struct MaskingMapsGPU { std::vector<float> raw,pooled; };
struct BaseSelectionGPU { std::vector<float> magnitudes; std::size_t index; };
// Synchronous parity backend. Host prepares filter/LUT coefficients;
// GPU evaluates adaptation preparation/pupil, resize/CSF, masking/D and quality.
// Shader must be generated with tests/generate_power_shaders.ps1 (ABI13).
class MapGPU {
public:
    MapGPU(const IQM::VulkanInstance& instance,const std::string& shader_path);
    ~MapGPU();
    std::vector<float> resize(const std::vector<float>& input,int h,int w,int nh,int nw);
    std::vector<float> sample_csf(const std::vector<float>& log_luminance,const CSFLUT& lut);
    std::vector<float> clamp_log(const std::vector<float>& input);
    std::vector<float> neighbor_mask(const std::vector<float>& previous,const std::vector<float>& next);
    std::vector<float> masking_denominator(const std::vector<float>& self,const std::vector<float>& neighbor,float q,float self_scale,float neighbor_scale);
    std::vector<float> difference(const std::vector<float>& difference,const std::vector<float>& sensitivity,const std::vector<float>& mask,float p,float base_neural=-1.0f);
    std::vector<float> psychometric(const std::vector<float>& input,float exponent);
    float quality_band_error(const std::vector<float>& difference,const std::vector<double>& comparison_mask,unsigned width,unsigned height);
    QualityScores quality_scores(const std::vector<float>& band_errors);
    std::array<float,4> pupil_trace(const std::vector<float>& adaptation,unsigned width,unsigned height,double area,double age);
    std::vector<float> mean_adaptation(const std::vector<float>& reference,const std::vector<float>& test);
    std::vector<float> adaptation_input(const std::vector<float>& retinal,const std::array<double,3>& matrix = D65_AGE24_LMR);
    std::vector<JNDResponseSingle> jnd_response(const std::vector<float>& luminance,const JNDLUT& lut,float pupil_reduction,const std::array<double,3>& matrix = D65_AGE24_LMR);
    std::vector<float> log_adaptation(const std::vector<float>& input);
    std::vector<float> normalized_difference(const std::vector<float>& reference,const std::vector<float>& test,float normalization);
    std::vector<double> comparison_mask(const std::vector<float>& reference,const std::vector<float>& test);
    std::vector<float> remove_residual_dc(const std::vector<float>& input);
    // Returns once-rounded mean log10 luminance and its native single power10.
    std::array<float,2> base_adaptation(const std::vector<float>& log_luminance);
    BaseSelectionGPU base_selection(const std::vector<float>& interleaved);
    // Each result is clamped log10 luminance followed by four native parameters.
    std::vector<std::array<float,5>> base_csf_parameters(const std::vector<float>& luminance);
    // Exact double result for binary64x64 masks and square8/16/32/64 outputs.
    std::vector<double> resize_comparison_mask(const std::vector<double>& input,int h,int w,int nh,int nw);
    std::vector<float> base_spectrum(const std::vector<float>& input,const std::string& shader_path);
    MaskingMapsGPU mutual_masking(const std::vector<float>& reference,const std::vector<float>& test,int h,int w,float normalization);
private:
    struct State;
    std::unique_ptr<State> state;
};
}
