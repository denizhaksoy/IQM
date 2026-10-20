/*
 * Image Quality Metrics - HDR-VDP-3 Steerable Pyramid
 * Deniz Aksoy - 2026
 */

#ifndef HDRVDP3_SPYR_H
#define HDRVDP3_SPYR_H

#include <iostream>
#include <fstream>
#include <limits>
#include <iomanip>
#include <cmath>  // Dosyanın başına ekle
#include <algorithm>
#include <IQM/hdrvdp3/corrdn.h>
#include <vector>
#include <memory>
#include <functional>

namespace IQM {
    class VulkanInstance;
namespace HDRVDP3 {

struct SpyrBand {
    std::vector<float> data;
    unsigned width = 0;
    unsigned height = 0;
};

struct SpyrResult {
    std::vector<SpyrBand> bands;
    std::vector<std::pair<unsigned, unsigned>> pind;  // {height, width} for each band
    unsigned pyr_height = 0;
};

class Spyr {
public:
    // Filter coefficients for sp0Filters (row-major, as used by shader)
    struct Filters {
        const float* lo0filt;   // 7x7 = 49
        const float* hi0filt;   // 9x9 = 81
        const float* lofilt;    // 13x13 = 169
        const float* bfilts;    // 81*8 = 648 (8 orientations of 9x9)
        unsigned num_orientations = 1;  // For sp0Filters, only 1 orientation (harmonics = [0])
    };

    explicit Spyr(const vk::raii::Device& device, const Filters& filters);

    // Compute max pyramid height for given image size
    static unsigned max_pyr_height(unsigned width, unsigned height, unsigned filter_size);

private:
    static unsigned max_pyr_height_rec(unsigned width, unsigned height, unsigned filter_size);

public:
    // Compute full steerable pyramid
    // Input image must be in row-major order (W*H floats)
    // Returns bands in order: [hi0, orient_bands..., residual_lowpass].
    // HDR-VDP callers cap maximum_height with ceil(log2(ppd))-2; the default
    // retains the unrestricted pyramid behavior for generic callers.
    // The corrDnFn is a callable that performs a single corrDn operation:
    //   std::vector<float> corrDnFn(const IQM::VulkanInstance& instance, CorrDn& corrdn,
    //                                const float* filter_coeffs, int filter_size,
    //                                const std::vector<float>& input_data,
    //                                unsigned width, unsigned height, int step)
    template <typename CorrDnFn>
    SpyrResult compute(const std::vector<float>& input, unsigned width, unsigned height,
                       const IQM::VulkanInstance& instance, CorrDnFn&& corrDnFn,
                       const std::string& export_prefix = "",
                       unsigned maximum_height = std::numeric_limits<unsigned>::max()) const;

private:
    Filters m_filters;
    std::unique_ptr<CorrDn> m_corrdn;
};

template <typename CorrDnFn>
SpyrResult Spyr::compute(const std::vector<float>& input, unsigned width, unsigned height,
                         const IQM::VulkanInstance& instance, CorrDnFn&& corrDnFn,
                         const std::string& export_prefix, unsigned maximum_height) const {
    SpyrResult result;
    result.pyr_height = std::min(maximum_height, max_pyr_height(width, height, 13));

    // Step 1: Initial highpass (hi0filt)
    {
        auto hi0_output = corrDnFn(instance, *m_corrdn, m_filters.hi0filt, 9,
                                   input, width, height, 1, false);

        SpyrBand band;
        band.width = width;
        band.height = height;
        band.data = std::move(hi0_output);
        result.bands.push_back(std::move(band));
        result.pind.emplace_back(height, width);
    }

    // Step 2: Initial lowpass (lo0filt)
    std::vector<float> current_lowpass = corrDnFn(instance, *m_corrdn, m_filters.lo0filt, 7,
                                                  input, width, height, 1, false);


    // Step 3: For each pyramid level, compute orientation bands and downsample
    unsigned current_w = width;
    unsigned current_h = height;

    for (unsigned level = 0; level < result.pyr_height; ++level) {
        // Compute orientation bands for this level (sp0Filters has 1 orientation)
        for (unsigned orient = 0; orient < m_filters.num_orientations; ++orient) {
            const float* orient_filter = m_filters.bfilts + orient * 81;
            const bool use_non_fma = (std::max(current_h,current_w) < 32);

            auto band_output = corrDnFn(
                instance, *m_corrdn, orient_filter, 9,
                current_lowpass, current_w, current_h, 1,
                use_non_fma
            );

            SpyrBand band;
            band.width = current_w;
            band.height = current_h;
            band.data = std::move(band_output);
            result.bands.push_back(std::move(band));
            result.pind.emplace_back(current_h, current_w);
        }

        // Downsample lowpass for next level (after every orientation level)
        unsigned next_w = (current_w + 1) / 2;  // ceil(width/2)
        unsigned next_h = (current_h + 1) / 2;  // ceil(height/2)

        current_lowpass = corrDnFn(
            instance, *m_corrdn, m_filters.lofilt, 13,
            current_lowpass, current_w, current_h, 2,
            std::max(current_h,current_w) < 32
        );

        current_w = next_w;
        current_h = next_h;


    }

    // Final residual lowpass
    SpyrBand residual;
    residual.width = current_w;
    residual.height = current_h;
    residual.data = std::move(current_lowpass);

    result.pind.emplace_back(residual.height, residual.width);
    result.bands.push_back(std::move(residual));

    // TEMPORARY DEBUG: Export raw steerable pyramid bands
for (size_t b = 0; b < result.bands.size(); ++b) {
    if (export_prefix.empty()) break;
    const auto& band = result.bands[b];

    const std::string filename =
        export_prefix + std::to_string(b) + ".txt";

    std::ofstream file(filename);

    if (!file) {
        throw std::runtime_error("Cannot open: " + filename);
    }

    file << band.height << " " << band.width << "\n";
    file << std::setprecision(std::numeric_limits<float>::max_digits10);

    for (float value : band.data) {
        file << value << "\n";
    }

    std::cout << "Saved GPU Band " << b
              << ": " << band.width << "x" << band.height
              << '\n';
}

    return result;
}

} // namespace HDRVDP3
} // namespace IQM

#endif // HDRVDP3_SPYR_H
