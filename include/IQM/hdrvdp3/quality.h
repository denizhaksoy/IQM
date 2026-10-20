#pragma once
#include <vector>
namespace IQM::HDRVDP3 {
// CPU reference-validation helpers; FFT coefficients are not parity outputs.
double dominant_frequency(const std::vector<float>& difference,unsigned width,unsigned height,double ppd);
double dominant_frequency_from_index(std::size_t index,unsigned width,unsigned height,double ppd);
double dominant_frequency_from_spectrum(const std::vector<float>& interleaved,unsigned width,unsigned height,double ppd,std::size_t* selected_index=nullptr);
float masked_difference(float difference,float sensitivity,float mask,double p,double pf,double base_noise=-1.0,float* raw_difference=nullptr);
float quality_band_error(const std::vector<float>& difference,const std::vector<double>& comparison_mask,unsigned width,unsigned height);
struct QualityScores { float error,Q,Q_JOD; };
QualityScores quality_scores(const std::vector<float>& band_errors);
}
