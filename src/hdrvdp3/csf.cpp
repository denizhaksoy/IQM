
#include <IQM/hdrvdp3/csf.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace IQM::HDRVDP3 {

namespace {

constexpr std::array<double, 7> csf_lums = {
    0.0002, 0.002, 0.02, 0.2, 2.0, 20.0, 150.0
};

// Columns correspond to p1, p2, p3, p4.
// The first column of the original Octave csf_params table
// is not used by hdrvdp_ncsf().
constexpr std::array<std::array<double, 4>, 7> csf_params = {{
    {1.26181, 4.27832, 0.361902, 3.11914},
    {0.893585, 4.27832, 0.361902, 2.18938},
    {0.84864, 3.57253, 0.530355, 3.12486},
    {0.699243, 3.94545, 0.68608, 4.41846},
    {0.530826, 4.25337, 0.866916, 4.65117},
    {0.459297, 3.78765, 0.981028, 4.33546},
    {0.312626, 4.15264, 0.952367, 3.22389}
}};

std::array<double, 4> interpolate_parameters(double luminance) {
    const double log_lum = std::log10(
        std::clamp(luminance, csf_lums.front(), csf_lums.back())
    );

    for (std::size_t i = 0; i + 1 < csf_lums.size(); ++i) {
        const double x0 = std::log10(csf_lums[i]);
        const double x1 = std::log10(csf_lums[i + 1]);

        if (log_lum <= x1) {
            const double t = (log_lum - x0) / (x1 - x0);

            std::array<double, 4> result{};

            for (std::size_t k = 0; k < 4; ++k) {
                result[k] =
                    csf_params[i][k] * (1.0 - t) +
                    csf_params[i + 1][k] * t;
            }

            return result;
        }
    }

    return csf_params.back();
}

double compute_aesl(double rho, double age) {
    constexpr double aesl_slope_freq = -2.711;
    constexpr double aesl_base = -0.125539;

    const double gamma = std::pow(10.0, aesl_base);
    const double age_difference = std::max(0.0, age - 24.0);

    return std::pow(
        10.0,
        -std::pow(
            10.0,
            aesl_slope_freq * std::log2(rho + gamma)
        ) * age_difference
    );
}

} // namespace

double compute_mtf(double rho) {
    constexpr double par1 = 0.061466549455263;
    constexpr double par2 = 0.99727370023777070;

    constexpr std::array<double, 4> a = {
        par2 * 0.426,
        par2 * 0.574,
        (1.0 - par2) * par1,
        (1.0 - par2) * (1.0 - par1)
    };

    constexpr std::array<double, 4> b = {
        0.028, 0.37, 37.0, 360.0
    };

    double mtf = 0.0;

    for (std::size_t i = 0; i < 4; ++i) {
        mtf += a[i] * std::exp(-b[i] * rho);
    }

    return mtf;
}

double compute_ncsf(
    double rho,
    double luminance,
    const CSFParameters& params
) {
    if (rho <= 1e-4) {
        return 0.0;
    }

    const auto p = interpolate_parameters(luminance);

    const double term1 =
        1.0 + std::pow(p[0] * rho, p[1]);

    const double term2 =
        1.0 - std::exp(-std::pow(rho / 7.0, 2.0));

    const double denominator = std::sqrt(
        term1 * std::pow(term2, -p[2])
    );

    double sensitivity = p[3] / denominator;

    sensitivity /= compute_mtf(rho);

    if (params.enable_aesl) {
        sensitivity *= compute_aesl(rho, params.age);
    }

    return sensitivity;
}

double compute_band_frequency(
    int band_index,
    double ppd
) {
    return std::ldexp(ppd, -band_index);
}

CSFLUT create_csf_lut(
    double band_frequency,
    const CSFParameters& params
) {
    CSFLUT lut{};

    constexpr double log_min = -5.0;
    constexpr double log_max = 5.0;

    for (std::size_t i = 0; i < lut.size(); ++i) {
        const double t =
            static_cast<double>(i) / (lut.size() - 1);

        const double luminance =
            std::pow(10.0, log_min + t * (log_max - log_min));

        lut[i] = compute_ncsf(
            band_frequency, luminance, params
        );
    }

    return lut;
}

double sample_csf_lut(
    const CSFLUT& lut,
    double luminance
) {
    const double log_luminance = std::log10(
        std::clamp(luminance, 1e-5, 1e5)
    );

    const double position =
        (log_luminance + 5.0) / 10.0 * (lut.size() - 1);

    const std::size_t lower =
        static_cast<std::size_t>(position);

    const std::size_t upper =
        std::min(lower + 1, lut.size() - 1);

    const double t = position - lower;

    return lut[lower] * (1.0 - t) + lut[upper] * t;
}

} // namespace IQM::HDRVDP3
