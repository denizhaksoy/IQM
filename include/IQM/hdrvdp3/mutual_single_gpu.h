#pragma once
#include <span>
#include <string>
#include <vector>
namespace IQM::Bin {class VulkanInstance;}
namespace IQM::HDRVDP3 {
// Experimental Single mutual activity correlation, row-major finite band inputs.
std::vector<float> mutual_single_gpu(const IQM::Bin::VulkanInstance& instance,
 const std::string& shader,std::span<const float> reference,std::span<const float> test,
 unsigned width,unsigned height);
}
