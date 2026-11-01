#pragma once
#include <IQM/hdrvdp3/spectral.h>
#include <span>

namespace IQM::HDRVDP3 {
// CPU preprocessing only. Three planar encoded Single channels in [0,1].
// Outputs remain Double; they must not be passed through a Float32 pathway
// when claiming parity with the native srgb-display prediction.
struct SRGBDisplayTrace {
    std::vector<double> display_rgb, raw_native_rgb, native_rgb;
};
SRGBDisplayTrace prepare_srgb_display(std::span<const float> encoded,
    const XYZDisplaySamples& display);
// Stage boundary only: input is already optically filtered planar native RGB.
// Keeping this separate prevents treating unfiltered RGB as retinal response.
struct RGBOpticalResponseDouble {
    std::vector<double> response, clamped, adaptation_input, adaptation_log;
};
RGBOpticalResponseDouble project_optical_rgb_double(std::span<const double> optical,
    const RGBSpectralSamples& display, double age);
// Input is the Double Gaussian-filtered log luminance, not raw luminance.
std::vector<double> finish_local_adaptation_double(std::span<const double> filtered_log);
}
