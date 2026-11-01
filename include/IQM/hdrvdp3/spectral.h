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
using RGBSpectralSamples = std::array<SpectralSamples,3>;
using ColorMatrix3 = std::array<std::array<double,3>,3>;
struct XYZDisplaySamples {
    std::vector<double> wavelength;
    std::vector<std::array<double,3>> xyz,emission;
};
struct XYZDisplayCalibration {ColorMatrix3 integrals{},native2XYZ{};};
XYZDisplaySamples load_xyz_display_samples(const std::filesystem::path& xyz,
    const std::filesystem::path& emission);
XYZDisplayCalibration calibrate_display_xyz(const XYZDisplaySamples& samples);
// Native-reference3x3 pivoted solve; matrix output rows are native primaries.
ColorMatrix3 rgb709_transform(const XYZDisplaySamples& samples);
enum class LinearColorEncoding { BT709, BT2020, XYZ };
ColorMatrix3 linear_color_transform(const XYZDisplaySamples& samples,LinearColorEncoding encoding);
struct RGBSpectralCalibration {
    std::array<std::array<double,4>,3> integrals{}, matrix{};
};
SpectralCalibration calibrate_spectral_age(const SpectralSamples& samples,double age);
// Shared sensor/grid, three display primaries; normalize all RGB L+M together.
RGBSpectralCalibration calibrate_rgb_spectral_age(const RGBSpectralSamples& samples,double age);
// Read original numeric data files and reproduce the native420-point grid.
SpectralSamples load_d65_spectral_samples(const std::filesystem::path& cone,
    const std::filesystem::path& rod,const std::filesystem::path& emission);
RGBSpectralSamples load_rgb_spectral_samples(const std::filesystem::path& cone,
    const std::filesystem::path& rod,const std::filesystem::path& emission);
}
