#pragma once
#include <filesystem>
#include <vector>
namespace IQM::HDRVDP3 {
struct RGBImage {
 unsigned width=0,height=0;
 std::vector<float> pixels; // Three top-down row-major planes, native primaries.
};
// Color PF, three-line header, interleaved bottom-up raster. Scale magnitude
// applied once in binary32; endian sign honored. Finite nonnegative domain.
// No gamma, color-space, grayscale conversion or implicit resizing.
RGBImage load_rgb_pfm(const std::filesystem::path& filename);
}
