#pragma once
#include <span>
#include <string>
#include <vector>
namespace IQM::Bin{class VulkanInstance;}
namespace IQM::HDRVDP3{
std::vector<float> fft_t1fv6_gpu(const IQM::Bin::VulkanInstance&,const std::string&,std::span<const float>);
}
