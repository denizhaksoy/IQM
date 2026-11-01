#pragma once
#include <IQM/hdrvdp3/pathway_quality.h>
#include <IQM/hdrvdp3/rgb_quality.h>
namespace IQM::HDRVDP3 {
// V1 rectangular/V2 square96/V3 square80(map13),V4 square128(map14);30ppd/age24/D65/sp0.
// Paths are confined
// to the package; SHA256 content digests and model/SPIR-V structure validated.
PathwayQualityAssets load_pathway_runtime(const std::filesystem::path& directory,
 unsigned width,unsigned height);
// Separate RGB overlay manifest plus a confined, validated v2 downstream base.
RGBQualityAssets load_rgb_pathway_runtime(const std::filesystem::path& directory,
 unsigned width,unsigned height);
struct RGB709Runtime {RGBQualityAssets native;std::filesystem::path conversion;};
RGB709Runtime load_rgb709_pathway_runtime(const std::filesystem::path& directory,
 unsigned width,unsigned height);
using LinearColorRuntime = RGB709Runtime;
LinearColorRuntime load_linear_color_pathway_runtime(const std::filesystem::path& directory,
 unsigned width,unsigned height,LinearColorEncoding encoding);
}
