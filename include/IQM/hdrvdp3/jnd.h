
#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace IQM::HDRVDP3 {

constexpr std::size_t JND_LUT_SIZE = 2048;

struct JNDLUT {
    std::array<double, JND_LUT_SIZE> log_luminance{};
    std::array<double, JND_LUT_SIZE> cone{};
    std::array<double, JND_LUT_SIZE> rod{};
};

struct JNDParameters {
    std::array<double, 4> csf_sa{
        315.98, 6.7977, 1.6008, 0.25534
    };

    std::array<double, 6> csf_sr{
        1.1732, 1.32, 1.095, 0.5547, 2.9899, 1.8
    };

    double rod_sensitivity = 0.0;
    double sensitivity_correction = 0.203943775672;
};

JNDLUT create_jnd_lut(
    const JNDParameters& params = JNDParameters{}
);

double sample_jnd(
    const JNDLUT& lut,
    double luminance,
    bool rod_pathway
);

double luminance_to_jnd_response(
    const JNDLUT& lut,
    double luminance
);

double pupil_luminance_reduction(
    double adapted_luminance,
    double area,
    double age
);

inline constexpr std::array<double,3> D65_AGE24_LMR{0.65293140835027819,0.34706859164972181,1.0171300679539299};
// Native single pathway with explicitly calibrated spectral coefficients.
struct JNDResponseSingle {
    std::array<float, 3> lmr_before{}, lmr_after{}, log_query{}, channels{};
    float response = 0.0f;
};
JNDResponseSingle luminance_to_jnd_response_single(
    const JNDLUT& lut, float luminance, float pupil_reduction,
    const std::array<double,3>& matrix = D65_AGE24_LMR);
float pupil_luminance_reduction_single(
    const std::vector<float>& adaptation, unsigned width, unsigned height,
    double area, double age);

} // namespace IQM::HDRVDP3

