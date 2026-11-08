#pragma once
#include <span>
#include <string>
#include <vector>
namespace IQM::Bin {class VulkanInstance;}
namespace IQM::HDRVDP3 {
// Row-major plane -> column-major 2W x 2H postpad, symmetric or replicate.
// Finite signed Float32 values, bit-preserving, <=4K input; stage-only API.
std::vector<float> padding_single_gpu(const IQM::Bin::VulkanInstance&,
 const std::string&,std::span<const float>,unsigned width,unsigned height,bool symmetric);
}
