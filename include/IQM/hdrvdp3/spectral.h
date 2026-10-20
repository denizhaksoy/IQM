#pragma once
#include <array>
#include <vector>
#include <filesystem>

namespace IQM::HDRVDP3 {
// Inputs already resampled by the official spectral loader, in double.
// Calibration consumes these values; the separate loader below reads raw data.
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
// Read original numeric data files and reproduce the native420-point grid.
SpectralSamples load_d65_spectral_samples(const std::filesystem::path& cone,
    const std::filesystem::path& rod,const std::filesystem::path& emission);
}
