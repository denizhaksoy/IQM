#pragma once
#include <span>
#include <string>
#include <vector>
namespace IQM::Bin {class VulkanInstance;}
namespace IQM::HDRVDP3 {
// Standalone adaptation L+M stage, finite clamped planes [1e-8,1e10], <=4K.
// GPU Float32 addition; synchronous host transfers, not a resident pipeline.
std::vector<float> adaptation_input_single_gpu(const IQM::Bin::VulkanInstance&,
 const std::string& shader,std::span<const float> l,std::span<const float> m);
}
