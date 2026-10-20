#include <IQM/hdrvdp3/jnd.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace IQM::HDRVDP3 {
namespace {
float pupil_diameter(float luminance, double area, double age) {
    float x = std::pow((luminance * static_cast<float>(area)) / 846.0f, 0.41f);
    float sd = 7.75f - 5.75f * (x / (x + 2.0f));
    float correction = static_cast<float>(std::clamp(age, 20.0, 83.0) - 28.58)
        * (0.02132f - 0.009562f * sd);
    return sd + correction;
}
float sample_single(const JNDLUT& lut, float query, bool rod) {
    // Octave lookup converts a double grid to single for a single query.
    // Comparing against double nodes selects the wrong interval at rounded knots.
    auto it = std::upper_bound(lut.log_luminance.begin(), lut.log_luminance.end(),query,
        [](float q,double node) { return q<static_cast<float>(node); });
    const auto i = std::clamp<std::size_t>(it - lut.log_luminance.begin(), 1,
                                          JND_LUT_SIZE - 1) - 1;
    const auto& table = rod ? lut.rod : lut.cone;
    // interp1 builds double slopes; ppval evaluates against a single query.
    const float slope = static_cast<float>((table[i + 1] - table[i]) /
        (lut.log_luminance[i + 1] - lut.log_luminance[i]));
    const float offset = query - static_cast<float>(lut.log_luminance[i]);
    const float product = slope * offset;
    return product + static_cast<float>(table[i]);
}
}

float pupil_luminance_reduction_single(const std::vector<float>& adaptation,
    unsigned width, unsigned height, double area, double age) {
    if (!width || !height || adaptation.size() != static_cast<std::size_t>(width) * height)
        throw std::invalid_argument("Adaptation dimensions mismatch");
    float log_sum = 0.0f;
    // Octave geomean(L_adapt(:)): column-major sequential float accumulation.
    for (unsigned x = 0; x < width; ++x) {
        for (unsigned y = 0; y < height; ++y) {
            const float value = adaptation[static_cast<std::size_t>(y) * width + x];
            if (!(value > 0.0f) || !std::isfinite(value))
                throw std::invalid_argument("Nonpositive or nonfinite adaptation luminance");
            log_sum += std::log(value);
        }
    }
    const float geo = std::exp(log_sum / static_cast<float>(adaptation.size()));
    const float d_ref = pupil_diameter(geo, area, 28.0);
    const float d_age = pupil_diameter(geo, area, age);
    return (d_age * d_age) / (d_ref * d_ref);
}

JNDResponseSingle luminance_to_jnd_response_single(
    const JNDLUT& lut, float luminance, float pupil_reduction,const std::array<double,3>& matrix) {
    JNDResponseSingle result;
    for (unsigned ch = 0; ch < 3; ++ch) {
        result.lmr_before[ch] = luminance * static_cast<float>(matrix[ch]);
        result.lmr_after[ch] = std::clamp(result.lmr_before[ch], 1e-8f, 1e10f) * pupil_reduction;
        result.log_query[ch] = std::log10(std::clamp(result.lmr_after[ch], 1e-5f, 1e5f));
        result.channels[ch] = sample_single(lut, result.log_query[ch], ch == 2);
    }
    result.response = (result.channels[0] + result.channels[1]) + result.channels[2];
    return result;
}
}
