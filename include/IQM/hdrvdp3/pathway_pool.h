#pragma once
#include <filesystem>
#include <vector>
namespace IQM { class VulkanInstance; }
namespace IQM::HDRVDP3 {
struct PathwayPoolAssets { std::filesystem::path fma,separate; };
struct PathwayPoolResult { std::vector<float> raw,pooled; };
// Synchronous/exclusive command buffer use. Row-major,already normalized/DC-
// processed bands,matching validated80x96/96x80 pyramid dimensions only.
PathwayPoolResult run_pathway_pool(const IQM::VulkanInstance& instance,
 const std::vector<float>& reference,const std::vector<float>& test,
 unsigned width,unsigned height,const PathwayPoolAssets& assets);
}
