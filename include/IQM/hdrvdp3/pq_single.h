#pragma once
#include <cstdint>
#include <span>
#include <vector>

namespace IQM::HDRVDP3 {
// Explicit display parameters. Inputs are normalized, nonlinear BT.2020 RGB;
// callers must decode YCbCr and perform any required spatial resize first.
struct PQDisplaySingle {
    double ambient_lux;
    double reflectivity;
    double peak_cd_m2;
};
// Original uint16 -> Single normalization used by the tested Octave image package.
float normalize_pq_u16_single(std::uint16_t code);
// Elementwise ST2084 -> absolute linear RGB, add reflected ambient, clamp peak.
// Preserves input layout; does not apply a luma transform or change dimensions.
std::vector<float> pq_to_display_single(std::span<const float> encoded,
                                      const PQDisplaySingle& display);
}
