#pragma once
#include <IQM/hdrvdp3/spectral.h>
#include <span>
namespace IQM::HDRVDP3 {
// Linear absolute RGB/XYZ -> native display primaries, with gamut floor 1e-6.
// Three row-major planes in and out. Single matrix-multiply semantics;
// spectral calibration and matrix solution remain Double, as in v3.0.7.
std::vector<float> prepare_linear_color_single(std::span<const float> input,
    const XYZDisplaySamples& display, LinearColorEncoding encoding);
}
