#pragma once
#include <IQM/hdrvdp3/fft_single.h>
namespace IQM::HDRVDP3 {
// Three native Single row-major RGB planes. Default hdrvdp MTF and symmetric
// post-padding to twice each dimension. Explicit FFT provider, no fallback.
std::vector<float> filter_optical_rgb_single(std::span<const float> native,
    unsigned width,unsigned height,double ppd,const FFTBackendSingle& backend);
// Adaptation-input L+M -> log, replicate postpad, FFT Gaussian, exponentiate.
// Initial domain ppd26..256: no unimplemented spatial-Gaussian branch.
std::vector<float> local_adaptation_single(std::span<const float> luminance,
    unsigned width,unsigned height,double ppd,const FFTBackendSingle& backend);
}
