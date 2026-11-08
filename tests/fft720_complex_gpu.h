#pragma once
#include <span>
#include <string>
#include <vector>
namespace IQM::Bin{class VulkanInstance;}
namespace IQM::HDRVDP3{
std::vector<float> fft720_complex_gpu(const IQM::Bin::VulkanInstance&,const std::string&,std::span<const float>,bool inverse);
}
