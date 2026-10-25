#pragma once
#include <filesystem>
#include <vector>
namespace IQM { class VulkanInstance; }
namespace IQM::HDRVDP3 {
struct MixedFilterAssets { std::filesystem::path forward, inverse, io; };
enum class MixedFilterMode { OpticalMTF, LocalAdaptation };
struct MixedFilterResult {
 std::vector<float> output;
 // Optional row-major interleaved complex diagnostic stages.
 std::vector<float> prepared, forward, filtered, inverse, modelFilter;
};
// Synchronous/exclusive instance command buffer. Positive finite Float32
// luminance,80x96/96x80,30ppd only. Explicit compatible mixed-axis assets.
// No native fixture reads, implicit resizing or public quality guard changes.
MixedFilterResult run_mixed_filter(const IQM::VulkanInstance& instance,
 const std::vector<float>& input,unsigned width,unsigned height,
 MixedFilterMode mode,const MixedFilterAssets& assets,bool trace=false);
}
