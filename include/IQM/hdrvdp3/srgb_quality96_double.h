#pragma once
#include <IQM/hdrvdp3/quality_double.h>
#include <IQM/hdrvdp3/srgb_display.h>
#include <IQM/hdrvdp3/spectral.h>
#include <functional>
#include <IQM/hdrvdp3/jnd.h>
#include <IQM/hdrvdp3/sp0_double.h>
namespace IQM::HDRVDP3 {
// Explicitly limited to quality/sp0,96x96,30ppd,age24. No implicit FFT fallback.
struct SRGBQuality96BackendDouble {
    // Column-major padded192sq and real filter -> column-major cropped96sq.
    std::function<std::vector<double>(std::span<const double>,std::span<const double>)> convolve;
    // Interleaved complex12sq -> same layout, DC zero, wholly-real imag +0.
    std::function<std::vector<double>(std::span<const double>)> base_fft;
    // Optional raw optical RGB->clamped response;must include optical clamp.
    std::function<RGBOpticalResponseDouble(std::span<const double>,const RGBSpectralSamples&)> project;
    std::function<std::vector<double>(std::span<const double>,std::span<const double>,unsigned,unsigned)> mutual;
    std::function<std::vector<double>(std::span<const std::vector<double>>,unsigned)> neighbor;
    std::function<SP0PyramidDouble(std::span<const double>)> pyramid;
    std::function<std::vector<double>(std::span<const double>,std::span<const double>,unsigned)> comparison;
    // GPU linear JND stages may retain explicit CPU log/exp query preparation.
    std::function<RGBJNDResponseDouble(const JNDLUT&,std::span<const double>,std::span<const double>)> jnd;
    std::function<QualityCSFDouble(std::span<const double>,unsigned,double)> csf;
    std::function<QualityBandDouble(std::span<const double>,std::span<const double>,std::span<const double>,std::span<const double>,std::span<const double>,unsigned,unsigned)> quality;

};
struct SRGBQuality96ResultDouble {
    QualityScoresDouble scores;
    std::array<double,5> band_errors;
};
// Original planar column-major encoded Single RGB in [0,1]. Model samples
// must describe the same display. No native captures or filesystem access.
SRGBQuality96ResultDouble evaluate_srgb_quality96_double(
    std::span<const float> reference,std::span<const float> test,
    const XYZDisplaySamples& xyz,const RGBSpectralSamples& spectral,
    const SRGBQuality96BackendDouble& backend);
}
