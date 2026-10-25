#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>
namespace IQM { class VulkanInstance; }
namespace IQM::HDRVDP3 {
struct PathwaySPYRAssets { std::filesystem::path fma,separate,mean,difference,residual; };
struct PathwaySPYRFilters { std::vector<uint32_t> lo0,lo,high,orientation; };
struct PathwaySPYRResult { std::array<std::vector<uint32_t>,11> stages; };
// Synchronous exclusive instance use; two finite Float32 P maps,80x96/96x80.
// Fixed sp0 filter words and compatible explicit shaders; no native data reads.
// Stages0..7 are bands;8 mean/DC;9 difference pack;10 residual FFT/selection.
PathwaySPYRResult run_pathway_spyr(const IQM::VulkanInstance& instance,
 const std::vector<float>& reference,const std::vector<float>& test,
 unsigned width,unsigned height,const PathwaySPYRFilters& filters,
 const PathwaySPYRAssets& assets);
}
