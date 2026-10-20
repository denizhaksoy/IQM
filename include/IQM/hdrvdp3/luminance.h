#pragma once
#include <filesystem>
#include <vector>

namespace IQM::HDRVDP3 {
struct LuminanceImage {
    unsigned width=0,height=0;
    std::vector<float> pixels; // Top-to-bottom row-major absolute cd/m^2.
};
// Grayscale Pf, three-line header, bottom-to-top raster (Netpbm convention).
// Header scale magnitude is applied once in binary32. No resize/tone mapping.
// The current metric domain requires strictly positive finite luminance.
LuminanceImage load_luminance_pfm(const std::filesystem::path& filename);
}
