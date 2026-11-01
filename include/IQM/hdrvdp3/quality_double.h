#pragma once
#include <span>
#include <vector>
#include <array>
namespace IQM::HDRVDP3 {
struct QualityBandDouble {
    std::vector<double> mask_noise,excitation,difference,psychometric,masked;
    double error=0;
};
// Default quality/sp0 profile. Inputs are row-major, already resized stage
// boundaries: test-reference band, self/neighbor masks, neural noise, comparison.
QualityBandDouble evaluate_quality_band_double(std::span<const double> band_difference,
    std::span<const double> self,std::span<const double> neighbor,
    std::span<const double> noise,std::span<const double> comparison,
    unsigned width,unsigned height);
std::array<double,6> quality_mask_parameters_double();
// Default quality mutual masking:3x3 box mean with zero border.
std::vector<double> mutual_mask_double(std::span<const double> reference,
    std::span<const double> test,unsigned width,unsigned height);
std::vector<double> quality_neighbor_mask_double(
    std::span<const std::vector<double>> self_masks,unsigned band);
struct QualityScoresDouble {double error,Q,Q_JOD;};
QualityScoresDouble quality_scores_double(std::span<const double> band_errors);
std::vector<double> quality_log_adaptation_double(std::span<const double> reference,
    std::span<const double> test);
std::vector<double> quality_comparison_double(std::span<const double> reference_rgb,
    std::span<const double> test_rgb,unsigned band);
struct QualityCSFDouble {
    std::vector<double> log_adaptation,sensitivity,noise;
    std::array<double,256> table;
    double base_luminance=0;
};
// base_frequency is selected from the actual base difference FFT, with DC zero.
QualityCSFDouble quality_csf_double(std::span<const double> mean_log_adaptation,
    unsigned band,double base_frequency=0);
}
