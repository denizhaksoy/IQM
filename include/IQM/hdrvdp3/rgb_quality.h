#pragma once
#include <IQM/hdrvdp3/pathway_quality.h>
#include <IQM/hdrvdp3/map_gpu.h>
#include <IQM/hdrvdp3/spectral.h>
namespace IQM::HDRVDP3 {
struct RGBQualityAssets {
    PathwayQualityAssets quality; // v2 downstream/map13, RGB comparison shader
    std::filesystem::path projection,adaptation,jnd; // JND requires ABI17
};
struct RGBQualityTrace {
    // All images/planes are row-major, reference then test.
    std::array<std::vector<float>,2> optical,lmsr,adaptationInput;
    std::array<std::array<float,4>,2> pupil;
    std::array<std::vector<JNDResponseSingle>,2> jnd;
    PathwayQualityTrace quality;
};
// Synchronous/exclusive instance use. Original finite nonnegative Float32
// RGB96, three row-major planes, rgb-native,30ppd,age24,default quality/sp0.
// Explicit shared-grid sensor/emission samples; no reference intermediate reads.
QualityScores evaluate_pathway_rgb_quality(const IQM::VulkanInstance& instance,
    const std::vector<float>& reference,const std::vector<float>& test,
    unsigned width,unsigned height,const RGBSpectralSamples& spectra,
    const RGBQualityAssets& assets,RGBQualityTrace* trace=nullptr);
struct RGB709QualityTrace {
    ColorMatrix3 transform{};
    std::array<std::vector<float>,2> raw,converted; // Row-major planar RGB.
    RGBQualityTrace native;
};
// LinearBT.70996,30ppd/age24; explicit conversion shader plus native downstream
// assets. XYZ and LMSR samples must describe the same display primaries/grid.
QualityScores evaluate_pathway_rgb709_quality(const IQM::VulkanInstance& instance,
    const std::vector<float>& reference,const std::vector<float>& test,
    unsigned width,unsigned height,const XYZDisplaySamples& xyz,
    const RGBSpectralSamples& spectra,const RGBQualityAssets& assets,
    const std::filesystem::path& conversion,RGB709QualityTrace* trace=nullptr);
using LinearColorQualityTrace = RGB709QualityTrace;
QualityScores evaluate_pathway_linear_color_quality(const IQM::VulkanInstance& instance,
    const std::vector<float>& reference,const std::vector<float>& test,
    unsigned width,unsigned height,const XYZDisplaySamples& xyz,
    const RGBSpectralSamples& spectra,const RGBQualityAssets& assets,
    const std::filesystem::path& conversion,LinearColorEncoding encoding,
    LinearColorQualityTrace* trace=nullptr);
}
