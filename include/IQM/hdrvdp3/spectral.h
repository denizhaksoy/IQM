#pragma once
#include <array>
#include <vector>

namespace IQM::HDRVDP3 {
// Inputs already resampled by the official spectral loader, in double.
// This API does not yet load raw spectral files or change the metric defaults.
struct SpectralSamples {
    std::vector<double> wavelength, emission;
    std::vector<std::array<double,4>> sensitivity;
};
struct SpectralCalibration {
    std::array<double,26> optical_density_delta{};
    std::vector<double> interpolated_density, transmission, filtered_emission;
    std::array<double,4> integrals{}, matrix{};
};
SpectralCalibration calibrate_spectral_age(const SpectralSamples& samples,double age);
}
