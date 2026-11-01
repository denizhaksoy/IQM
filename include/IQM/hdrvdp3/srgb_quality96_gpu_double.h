#pragma once
#include <IQM/hdrvdp3/srgb_quality96_double.h>
#include <vulkan/vulkan_raii.hpp>
#include <memory>
namespace IQM::HDRVDP3 {
// The caller owns device/physicalDevice/queue and must keep them alive and
// synchronize other uses of that queue. float64Enabled describes the logical
// device feature enabled at creation;physical support alone is insufficient.
class SRGBQuality96GPUDouble {
public:
    using Shaders=std::array<std::vector<uint32_t>,7>; // forward axes,multiply,inverse axes,crop,base12
    struct Convolution {std::vector<double> cropped,inverse,spectrum;};
    SRGBQuality96GPUDouble(const vk::raii::PhysicalDevice&,const vk::raii::Device&,
        const vk::raii::Queue&,uint32_t queueFamily,bool float64Enabled,const Shaders&,std::span<const uint32_t> projectionShader={},std::span<const uint32_t> mutualShader={},std::span<const uint32_t> neighborShader={},std::span<const uint32_t> pyramidShader={},std::span<const uint32_t> comparisonShader={},std::span<const uint32_t> jndShader={},std::span<const uint32_t> csfShader={},std::span<const uint32_t> qualityShader={},std::span<const uint32_t> logarithmShader={});
    ~SRGBQuality96GPUDouble();
    SRGBQuality96GPUDouble(const SRGBQuality96GPUDouble&)=delete;
    SRGBQuality96GPUDouble& operator=(const SRGBQuality96GPUDouble&)=delete;
    // Traces are opt-in; normal metric callbacks return only cropped output.
    Convolution convolve(std::span<const double>,std::span<const double>,bool traces=false);
    // Positive normal finite arguments only, 1..36864 elements. Interleaved ln/log10.
    std::vector<double> logarithms(std::span<const double>);
    // CPU CSF models/base scalar, JND reduction/exp and table slopes remain explicit.
    // CPU pow/psychometric/error powers;GPU map arithmetic and normalized pooling sum.
    QualityBandDouble quality(std::span<const double>,std::span<const double>,std::span<const double>,std::span<const double>,std::span<const double>,unsigned,unsigned,std::vector<double>* denominator=nullptr,double* pooling_sum=nullptr);
    QualityCSFDouble csf(std::span<const double>,unsigned band,double base_frequency);
    RGBJNDResponseDouble jnd(const JNDLUT&,std::span<const double>,std::span<const double>);
    std::vector<double> comparison(std::span<const double>,std::span<const double>,unsigned band);
    SP0PyramidDouble pyramid(std::span<const double>);
    std::vector<double> neighbor(std::span<const std::vector<double>>,unsigned band);
    std::vector<double> mutual(std::span<const double>,std::span<const double>,unsigned width,unsigned height);
    RGBOpticalResponseDouble project(std::span<const double>,const RGBSpectralSamples&);
    std::vector<double> base_fft(std::span<const double>);
    // Callbacks borrow this object. Do not retain them beyond its lifetime.
    SRGBQuality96BackendDouble backend();
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
