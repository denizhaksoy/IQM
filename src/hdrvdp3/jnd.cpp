
#include "IQM/hdrvdp3/jnd.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace IQM::HDRVDP3 {
namespace {

constexpr double LOG10_FACTOR = 2.302585092994045684;

double interpolate(
    const std::vector<double>& x,
    const std::vector<double>& y,
    double value
) {
    if (value <= x.front())
        return y.front();

    if (value >= x.back())
        return y.back();

    auto it = std::lower_bound(x.begin(), x.end(), value);
    const auto i = static_cast<std::size_t>(it - x.begin());

    const double t =
        (value - x[i - 1]) / (x[i] - x[i - 1]);

    return y[i - 1] + t * (y[i] - y[i - 1]);
}

double joint_sensitivity(
    double luminance,
    const JNDParameters& p
) {
    const auto& a = p.csf_sa;

    return a[0] * std::pow(
        std::pow(a[1] / luminance, a[2]) + 1.0,
        -a[3]
    );
}

double rod_sensitivity(
    double luminance,
    const JNDParameters& p
) {
    const auto& a = p.csf_sr;

    const double ratio = std::abs(
        std::log10(luminance / a[0])
    );

    const double sensitivity =
        luminance > a[0]
        ? std::exp(-std::pow(ratio, a[4]) / a[3])
        : std::exp(-std::pow(ratio, a[2]) / a[1]);

    return sensitivity * std::pow(10.0, a[5]);
}

} // namespace

JNDLUT create_jnd_lut(const JNDParameters& params) {
    JNDLUT result;

    std::vector<double> luminance(JND_LUT_SIZE);
    std::vector<double> combined(JND_LUT_SIZE);
    std::vector<double> rods(JND_LUT_SIZE);
    std::vector<double> cones(JND_LUT_SIZE);

    const double rod_scale =
        std::pow(10.0, params.rod_sensitivity);

    const double correction_scale =
        std::pow(10.0, params.sensitivity_correction);

    for (std::size_t i = 0; i < JND_LUT_SIZE; ++i) {
        const double log_l =
            -5.0 + 10.0 * static_cast<double>(i)
            / static_cast<double>(JND_LUT_SIZE - 1);

        result.log_luminance[i] = log_l;
        luminance[i] = std::pow(10.0, log_l);

        combined[i] = joint_sensitivity(
            luminance[i], params
        );

        rods[i] = rod_sensitivity(
            luminance[i], params
        ) * rod_scale;
    }

    std::vector<double> cone_source(JND_LUT_SIZE);

    for (std::size_t j = 0; j < JND_LUT_SIZE; ++j) {
        cone_source[j] = std::max(
            combined[j] - rods[j], 1e-3
        );
    }

    for (std::size_t i = 0; i < JND_LUT_SIZE; ++i) {
        const double query = std::min(
            luminance[i] * 2.0,
            luminance.back()
        );

        cones[i] = 0.5 * interpolate(
            luminance, cone_source, query
        );
    }

    result.cone[0] = 0.0;
    result.rod[0] = 0.0;

    for (std::size_t i = 1; i < JND_LUT_SIZE; ++i) {
        const double step =
            result.log_luminance[i]
            - result.log_luminance[i - 1];

        result.cone[i] = result.cone[i - 1]
            + 0.5 * (cones[i - 1] + cones[i])
            * LOG10_FACTOR * step;

        result.rod[i] = result.rod[i - 1]
            + 0.5 * (rods[i - 1] + rods[i])
            * LOG10_FACTOR * step;
    }

    for (std::size_t i = 0; i < JND_LUT_SIZE; ++i) {
        result.cone[i] *= correction_scale;
        result.rod[i] *= correction_scale;
    }

    return result;
}

double sample_jnd(
    const JNDLUT& lut,
    double luminance,
    bool rod_pathway
) {
    const double log_l = std::log10(
        std::clamp(luminance, 1e-5, 1e5)
    );

    const double index =
        (log_l + 5.0) * (JND_LUT_SIZE - 1) / 10.0;

    const auto i = std::min(
        static_cast<std::size_t>(index),
        JND_LUT_SIZE - 2
    );

    const double t = std::clamp(
        index - static_cast<double>(i), 0.0, 1.0
    );

    const auto& values = rod_pathway ? lut.rod : lut.cone;

    return values[i] + t * (values[i + 1] - values[i]);
}

double luminance_to_jnd_response(
    const JNDLUT& lut,
    double luminance
) {
    constexpr double L_COEFF = 0.652931408350278;
    constexpr double M_COEFF = 0.347068591649722;
    constexpr double R_COEFF = 1.017130067953930;

    const double L = sample_jnd(
        lut, std::clamp(luminance * L_COEFF, 1e-8, 1e10), false
    );

    const double M = sample_jnd(
        lut, std::clamp(luminance * M_COEFF, 1e-8, 1e10), false
    );

    const double R = sample_jnd(
        lut, std::clamp(luminance * R_COEFF, 1e-8, 1e10), true
    );

    return L + M + R;
}

double pupil_d_stanley_davies(double luminance, double area)
{
    const double x = std::pow(
        luminance * area / 846.0, 0.41
    );

    return 7.75 - 5.75 * (x / (x + 2.0));
}

double pupil_d_unified(double luminance, double area, double age)
{
    const double y = std::clamp(age, 20.0, 83.0);

    const double d_sd =
        pupil_d_stanley_davies(luminance, area);

    return d_sd
        + (y - 28.58) * (0.02132 - 0.009562 * d_sd);
}

double pupil_luminance_reduction(
    double adapted_luminance,
    double area,
    double age)
{
    const double d_ref =
        pupil_d_unified(adapted_luminance, area, 28.0);

    const double d_age =
        pupil_d_unified(adapted_luminance, area, age);

    return (d_age * d_age) / (d_ref * d_ref);
}

} // namespace IQM::HDRVDP3
