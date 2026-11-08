#pragma once
#include <span>
#include <string>
#include <vector>
namespace IQM::Bin {class VulkanInstance;}
namespace IQM::HDRVDP3 {
// Interleaved complex Single spectrum times Single-rounded Double real filter.
std::vector<float> fft_filter_single_gpu(const IQM::Bin::VulkanInstance&,
 const std::string&,std::span<const float>,std::span<const double>);
}

namespace IQM::HDRVDP3 {
std::vector<float> fft128_crop_single_gpu(const IQM::Bin::VulkanInstance&,
 const std::string&,std::span<const float>,unsigned,unsigned);
}
