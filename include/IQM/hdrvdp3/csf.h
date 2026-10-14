#pragma once

#include <array>

namespace IQM::HDRVDP3 {

struct CSFParameters {
    double age = 24.0;
    bool enable_aesl = true;
};

using CSFLUT = std::array<double, 256>;

double compute_mtf(double rho);

double compute_ncsf(
    double rho,
    double luminance,
    const CSFParameters& params = {}
);

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

} // namespace IQM::HDRVDP3