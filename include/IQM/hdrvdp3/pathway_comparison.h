#pragma once
#include <cstdint>
#include <filesystem>
#include <vector>
namespace IQM { class VulkanInstance; }
namespace IQM::HDRVDP3 {
struct PathwayComparisonResult { std::vector<uint32_t> binary; std::vector<double> resized; };
// Synchronous/exclusive instance use. Source80x96/96x80,uniform target
// downsampling1/2/4/8. Positive finite reference,finite test,explicit shader.
// Q20/Q40 integer resize; no fixture reads,implicit resizing or public guard changes.
PathwayComparisonResult run_pathway_comparison(const IQM::VulkanInstance& instance,
 const std::vector<float>& reference,const std::vector<float>& test,
 unsigned width,unsigned height,unsigned targetWidth,unsigned targetHeight,
 const std::filesystem::path& shader);
}
