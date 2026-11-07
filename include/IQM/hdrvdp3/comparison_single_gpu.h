#pragma once
#include <span>
#include <string>
#include <vector>
namespace IQM::Bin {class VulkanInstance;}
namespace IQM::HDRVDP3 {
// Experimental standalone stage. Requires an enabled shaderFloat64 device.
// Three planar native RGB channels, all finite and >=1e-6f, up to 4K.
// Returns a Single binary mask; no resizing or quality evaluation included.
std::vector<float> comparison_single_gpu(const IQM::Bin::VulkanInstance& instance,
 const std::string& shader,std::span<const float> reference,std::span<const float> test);
}
