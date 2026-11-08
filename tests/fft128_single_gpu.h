#pragma once
#include <span>
#include <string>
#include <vector>
namespace IQM::Bin {class VulkanInstance;}
namespace IQM::HDRVDP3::Experimental {
// Isolated128x128 implementation using the existing FFTW-derived test shader.
// Forward:16384 real column-major values ->32768 interleaved complex values.
// Inverse:32768 interleaved values ->32768 normalized complex values.
std::vector<float> fft128_gpu(const IQM::Bin::VulkanInstance&,const std::string&,
 std::span<const float>,bool forward);
}
