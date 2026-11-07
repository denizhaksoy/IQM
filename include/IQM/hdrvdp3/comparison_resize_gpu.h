#pragma once
#include <span>
#include <string>
#include <vector>
namespace IQM::Bin {class VulkanInstance;}
namespace IQM::HDRVDP3 {
struct ComparisonResizeGPUResult {std::vector<double> values;unsigned dispatches=0;};
// Experimental binary comparison-mask resize. Double image arithmetic and
// CPU-prepared existing axis plans. Requires enabled shaderFloat64, <=4K count.
ComparisonResizeGPUResult resize_comparison_gpu(const IQM::Bin::VulkanInstance& instance,
 const std::string& shader,std::span<const double> mask,
 unsigned height,unsigned width,unsigned targetHeight,unsigned targetWidth);
}
