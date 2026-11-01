#pragma once
#include <filesystem>
#include <vector>
namespace IQM { class VulkanInstance; }
namespace IQM::HDRVDP3 {
struct MixedFilterAssets { std::filesystem::path forward, inverse, io; bool reference256Graph=false; };
enum class MixedFilterMode { OpticalMTF, LocalAdaptation, OpticalRGB };
struct MixedFilterResult {
 std::vector<float> output;
 // Prepared/filtered:column-major complex;forward/inverse:row-major complex.
 std::vector<float> prepared, forward, filtered, inverse, modelFilter;
};
// Synchronous/exclusive instance command buffer. Positive finite Float32
// luminance,80x96/96x80/96x96/80x80/128x128,30ppd only. Explicit compatible assets.
// No native fixture reads, implicit resizing or public quality guard changes.
// OpticalRGB explicitly accepts finite nonnegative primary values. Its clamp
// is applied after MTF filtering, using the same optical shader path.
MixedFilterResult run_mixed_filter(const IQM::VulkanInstance& instance,
 const std::vector<float>& input,unsigned width,unsigned height,
 MixedFilterMode mode,const MixedFilterAssets& assets,bool trace=false);
}
