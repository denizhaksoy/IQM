/*
 * Image Quality Metrics - HDR-VDP-3 Optical MTF Filter
 * Petr Volf - 2026
 */

#ifndef HDRVDP3_MTF_H
#define HDRVDP3_MTF_H

#include <vector>
#include <cstddef>

namespace IQM {
namespace HDRVDP3 {

/**
 * Generate the HDR-VDP-3 default 'hdrvdp' MTF filter in the frequency domain.
 *
 * The filter is computed using the exact formulation from the reference
 * implementation (hdrvdp_parse_options.m, create_cycdeg_image.m, hdrvdp_mtf.m).
 *
 * The MTF is a sum of 4 exponentials:
 *   MTF(rho) = sum_{k=0..3} a[k] * exp(-b[k] * rho)
 *
 * where rho is the radial spatial frequency in cycles/degree.
 *
 * The frequency grid follows the create_cycdeg_image.m convention:
 * - DC component is at index (0, 0) (not fftshifted)
 * - x, y indices: 0 .. width-1, 0 .. height-1
 * - Frequency at index (y, x) = sqrt( (kx)^2 + (ky)^2 )
 *   where kx = (mod(0.5 + x/width, 1) - 0.5) * pix_per_deg
 *         ky = (mod(0.5 + y/height, 1) - 0.5) * pix_per_deg
 *
 * @param out          Output buffer, size = width * height. Must be pre-allocated.
 * @param width        Filter width in frequency domain (e.g., 128)
 * @param height       Filter height in frequency domain (e.g., 128)
 * @param pix_per_deg  Pixels per degree (e.g., 30.0)
 * @param as_float     If true, convert double-precision results to float32
 */
void generate_mtf_filter(float* out, std::size_t width, std::size_t height, double pix_per_deg, bool as_float = true);

/**
 * Overload for double-precision output.
 */
void generate_mtf_filter(double* out, std::size_t width, std::size_t height, double pix_per_deg);

/**
 * Get the default MTF coefficients (a and b arrays) used by the 'hdrvdp' mode.
 * Useful for verification/debugging.
 *
 * @param a_out  Output array of 4 doubles for a coefficients
 * @param b_out  Output array of 4 doubles for b coefficients
 */
void get_default_mtf_coefficients(double a_out[4], double b_out[4]);

/**
 * Compute MTF value at a single radial frequency rho (cycles/degree).
 * Uses double precision internally.
 */
double compute_mtf_at_rho(double rho, const double a[4], const double b[4]);

} // namespace HDRVDP3
} // namespace IQM

#endif // HDRVDP3_MTF_H