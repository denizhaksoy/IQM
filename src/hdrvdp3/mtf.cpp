/*
 * Image Quality Metrics - HDR-VDP-3 Optical MTF Filter
 * Deniz Aksoy - 2026
 */

#include <IQM/hdrvdp3/mtf.h>
#include <cmath>
#include <algorithm>

namespace IQM {
namespace HDRVDP3 {

// Exact coefficients from hdrvdp_parse_options.m:148-150
// par = [0.061466549455263, 0.99727370023777070];
// mtf_params_a = [par(2)*0.426, par(2)*0.574, (1-par(2))*par(1), (1-par(2))*(1-par(1))];
// mtf_params_b = [0.028, 0.37, 37, 360];

static constexpr double DEFAULT_PAR1 = 0.061466549455263;
static constexpr double DEFAULT_PAR2 = 0.99727370023777070;

static constexpr double DEFAULT_A[4] = {
    DEFAULT_PAR2 * 0.426,
    DEFAULT_PAR2 * 0.574,
    (1.0 - DEFAULT_PAR2) * DEFAULT_PAR1,
    (1.0 - DEFAULT_PAR2) * (1.0 - DEFAULT_PAR1)
};

static constexpr double DEFAULT_B[4] = {
    0.028,
    0.37,
    37.0,
    360.0
};

// Verify sum(a) == 1.0 at compile time
static constexpr double SUM_A = DEFAULT_A[0] + DEFAULT_A[1] + DEFAULT_A[2] + DEFAULT_A[3];
static_assert(SUM_A > 1.0 - 1e-15 && SUM_A < 1.0 + 1e-15,
              "MTF coefficients must sum to 1.0");

void get_default_mtf_coefficients(double a_out[4], double b_out[4]) {
    for (int i = 0; i < 4; ++i) {
        a_out[i] = DEFAULT_A[i];
        b_out[i] = DEFAULT_B[i];
    }
}

double compute_mtf_at_rho(double rho, const double a[4], const double b[4]) {
    double mtf = 0.0;
    for (int k = 0; k < 4; ++k) {
        mtf += a[k] * std::exp(-b[k] * rho);
    }
    return mtf;
}

void generate_mtf_filter(double* out, std::size_t width, std::size_t height, double pix_per_deg) {
    // Frequency grid indexing matches create_cycdeg_image.m exactly:
    // KX0 = mod(0.5 + (0:(W-1))/W, 1) - 0.5
    // KX1 = KX0 * pix_per_deg
    // rho = sqrt(KX1^2 + KY1^2)
    // DC is at (0,0) - NOT fftshifted
    
    for (std::size_t y = 0; y < height; ++y) {
        double ky = std::fmod(0.5 + static_cast<double>(y) / static_cast<double>(height), 1.0) - 0.5;
        ky *= pix_per_deg;
        
        for (std::size_t x = 0; x < width; ++x) {
            double kx = std::fmod(0.5 + static_cast<double>(x) / static_cast<double>(width), 1.0) - 0.5;
            kx *= pix_per_deg;
            
            double rho = std::sqrt(kx * kx + ky * ky);
            out[y * width + x] = compute_mtf_at_rho(rho, DEFAULT_A, DEFAULT_B);
        }
    }
}

void generate_mtf_filter(float* out, std::size_t width, std::size_t height, double pix_per_deg, bool as_float) {
    // Generate in double precision first
    std::vector<double> temp(width * height);
    generate_mtf_filter(temp.data(), width, height, pix_per_deg);
    
    if (as_float) {
        // Convert to float32
        for (std::size_t i = 0; i < width * height; ++i) {
            out[i] = static_cast<float>(temp[i]);
        }
    } else {
        // This shouldn't be called with float* and as_float=false
        // but handle gracefully by copying as double to float
        for (std::size_t i = 0; i < width * height; ++i) {
            out[i] = static_cast<float>(temp[i]);
        }
    }
}

} // namespace HDRVDP3
} // namespace IQM