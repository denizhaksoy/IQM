#pragma once

#include <array>

namespace IQM::HDRVDP3 {

struct CSFParameters {
    double age = 24.0;
    bool enable_aesl = true;
};

using CSFLUT = std::array<double, 256>;
struct CSFSingleLUT {
    std::array<float,256> log_luminance{},sensitivity{};
    std::array<float,255> slope{};
};
CSFSingleLUT create_csf_single_lut(const CSFLUT& lut);
struct CSFParameterSingleLUT {
    std::array<float,7> log_luminance{};
    std::array<float,28> values{};
    std::array<float,24> slopes{};
};
CSFParameterSingleLUT create_csf_parameter_single_lut();
double evaluate_ncsf_parameters(double rho,const std::array<float,4>& parameters,const CSFParameters& params = {});

double compute_mtf(double rho);

double compute_ncsf(
    double rho,
    double luminance,
    const CSFParameters& params = {}
);
// Official scalar nCSF with a single luminance query: interp1 rounds the
// interpolated parameters to single before storing them in a double matrix.
double compute_ncsf_single_luminance(double rho,float luminance,const CSFParameters& params = {});

double compute_band_frequency(
    int band_index,
    double ppd
);

CSFLUT create_csf_lut(
    double band_frequency,
    const CSFParameters& params = {}
);

double sample_csf_lut(
    const CSFLUT& lut,
    double luminance
);

// Sample an already logarithmic adaptation map without an exp/log roundtrip.
double sample_csf_lut_log(const CSFLUT& lut, double log_luminance);
// Octave interp1 with a single-precision query uses single arithmetic in ppval.
float sample_csf_lut_log_single(const CSFLUT& lut, float log_luminance);

} // namespace IQM::HDRVDP3
