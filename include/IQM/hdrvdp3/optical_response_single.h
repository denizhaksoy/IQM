#pragma once
#include <IQM/hdrvdp3/spectral.h>
#include <span>
namespace IQM::HDRVDP3 {
struct RGBOpticalResponseSingle {
    std::vector<float> clamped,adaptation_input,adaptation_log;
};
// Input must already have undergone per-primary optical filtering.
// Output planes are L,M,S,rod, then adaptation L+M and its natural logarithm.
RGBOpticalResponseSingle project_optical_rgb_single(std::span<const float> optical,
    const RGBSpectralSamples& display,double age);
}
