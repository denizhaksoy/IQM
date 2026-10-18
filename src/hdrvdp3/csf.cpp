
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

// Symmetric logarithmic sampling: measure each node from its nearest endpoint.
// This preserves the two-endpoint rounding used by native logspace(-5,5,256).
double csf_grid_exponent(std::size_t i) {
    const std::size_t distance=std::min(i,std::size_t(255)-i);
    const double magnitude=5.0-static_cast<double>(distance)*(10.0/255.0);
    return i<128?-magnitude:magnitude;
}

std::array<double, 4> interpolate_parameters(double luminance) {
    const double log_lum = std::log10(
        std::clamp(luminance, csf_lums.front(), csf_lums.back())
    );

    for (std::size_t i = 0; i + 1 < csf_lums.size(); ++i) {
        const double x0 = std::log10(csf_lums[i]);
        const double x1 = std::log10(csf_lums[i + 1]);

        if (log_lum <= x1) {
            std::array<double, 4> result{};

            for (std::size_t k = 0; k < 4; ++k) {
                const double slope=(csf_params[i+1][k]-csf_params[i][k])/(x1-x0);
                result[k]=slope*(log_lum-x0)+csf_params[i][k];
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
        -(std::pow(10.0,aesl_slope_freq)*std::log2(rho+gamma))*age_difference
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

static double evaluate_ncsf(
    double rho,
    const std::array<double,4>& p,
    const CSFParameters& params
) {
    if (rho <= 1e-4) {
        return 0.0;
    }

    const double term1 =
        1.0 + std::pow(p[0] * rho, p[1]);

    const double term2 =
        1.0 - std::exp(-std::pow(rho / 7.0, 2.0));

    const double denominator = std::pow(
        term1 / std::pow(term2, p[2]),0.5
    );

    // Preserve the reference's divisions: reciprocal/multiply rewrites change
    // double rounding even when their final single outputs happen to agree.
    double sensitivity = p[3] / denominator;

    sensitivity /= compute_mtf(rho);

    if (params.enable_aesl) {
        sensitivity *= compute_aesl(rho, params.age);
    }

    return sensitivity;
}

CSFParameterSingleLUT create_csf_parameter_single_lut() {
    CSFParameterSingleLUT lut;
    for(std::size_t i=0;i<7;++i) {
        lut.log_luminance[i]=static_cast<float>(std::log10(csf_lums[i]));
        for(std::size_t k=0;k<4;++k) {
            lut.values[4*i+k]=static_cast<float>(csf_params[i][k]);
            if(i<6) lut.slopes[4*i+k]=static_cast<float>((csf_params[i+1][k]-csf_params[i][k])/(std::log10(csf_lums[i+1])-std::log10(csf_lums[i])));
        }
    }
    return lut;
}
double evaluate_ncsf_parameters(double rho,const std::array<float,4>& parameters,const CSFParameters& params) {
    return evaluate_ncsf(rho,{parameters[0],parameters[1],parameters[2],parameters[3]},params);
}
double compute_ncsf(double rho,double luminance,const CSFParameters& params) {
    return evaluate_ncsf(rho,interpolate_parameters(luminance),params);
}

double compute_ncsf_single_luminance(double rho,float luminance,const CSFParameters& params) {
    std::array<double,7> grid{};
    for(std::size_t i=0;i<grid.size();++i) grid[i]=std::log10(csf_lums[i]);
    const float q=std::clamp(std::log10(luminance),static_cast<float>(grid.front()),static_cast<float>(grid.back()));
    const auto next=std::upper_bound(grid.begin(),grid.end(),q,
        [](float query,double node){return query<static_cast<float>(node);});
    const auto upper=std::clamp<std::size_t>(next-grid.begin(),1,grid.size()-1);
    const auto lower=upper-1;
    std::array<double,4> p{};
    for(std::size_t k=0;k<p.size();++k) {
        const float slope=static_cast<float>((csf_params[upper][k]-csf_params[lower][k])/(grid[upper]-grid[lower]));
        const float offset=q-static_cast<float>(grid[lower]);
        const float product=slope*offset;
        p[k]=product+static_cast<float>(csf_params[lower][k]);
    }
    return evaluate_ncsf(rho,p,params);
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

    for (std::size_t i = 0; i < lut.size(); ++i) {
        const double luminance = std::pow(10.0, csf_grid_exponent(i));

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

    return sample_csf_lut_log(lut, log_luminance);
}

static const std::array<double,256>& csf_log_grid() {
    // Interpolate on log10 of the actual luminances used to build the LUT.
    // They are only approximately uniformly spaced after pow/log rounding.
    static const auto grid = [] {
        std::array<double, 256> result{};
        for (std::size_t i = 0; i < result.size(); ++i) {
            result[i] = std::log10(std::pow(10.0, csf_grid_exponent(i)));
        }
        return result;
    }();
    return grid;
}

double sample_csf_lut_log(const CSFLUT& lut, double log_luminance) {
    const auto& grid=csf_log_grid();
    const double query = std::clamp(log_luminance, grid.front(), grid.back());
    const auto next = std::upper_bound(grid.begin(), grid.end(), query);
    const std::size_t upper = std::min(static_cast<std::size_t>(next-grid.begin()), grid.size()-1);
    const std::size_t lower = upper-1;
    const double slope = (lut[upper]-lut[lower])/(grid[upper]-grid[lower]);
    return slope*(query-grid[lower])+lut[lower];
}

float sample_csf_lut_log_single(const CSFLUT& lut, float log_luminance) {
    const auto& grid=csf_log_grid();
    const float query=std::clamp(log_luminance,static_cast<float>(grid.front()),static_cast<float>(grid.back()));
    // Octave lookup converts the grid to single when the query is single.
    const auto next=std::upper_bound(grid.begin(),grid.end(),query,
        [](float q,double node) { return q<static_cast<float>(node); });
    const std::size_t upper=std::min(static_cast<std::size_t>(next-grid.begin()),grid.size()-1);
    const std::size_t lower=upper-1;
    const double slope=(lut[upper]-lut[lower])/(grid[upper]-grid[lower]);
    const float offset=query-static_cast<float>(grid[lower]);
    const float product=static_cast<float>(slope)*offset;
    return product+static_cast<float>(lut[lower]);
}

CSFSingleLUT create_csf_single_lut(const CSFLUT& lut) {
    const auto& grid=csf_log_grid(); CSFSingleLUT result;
    for(std::size_t i=0;i<lut.size();++i) {
        result.log_luminance[i]=static_cast<float>(grid[i]);
        result.sensitivity[i]=static_cast<float>(lut[i]);
        if(i+1<lut.size()) result.slope[i]=static_cast<float>((lut[i+1]-lut[i])/(grid[i+1]-grid[i]));
    }
    return result;
}

} // namespace IQM::HDRVDP3
