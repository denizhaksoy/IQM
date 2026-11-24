#pragma once
#include <IQM/hdrvdp3/optical_narrowing_gpu.h>
namespace IQM::HDRVDP3 {
struct AdaptationPixelView{FFTBufferRegion region;uint32_t first=0,sampleStride=1,rowStride=3840;};
struct LocalAdaptationBuffers{OpticalNarrowingBuffers convolution;AdaptationPixelView luminance,output;FFTBufferRegion crop;};
enum class AdaptationStage{Log,Pad,Forward,Expand,Multiply,Inverse,Normalize,Crop,Exp};
// Profile3840x2160 Single, ppd62.729130063099447, Gaussian sigma10.377727547380433.
// Caller supplies exact static f32 Gaussian coefficients; CPU preparation is
// explicit outside this API, not per-frame fallback. No MTF output clamp.
// convolution.base.input=contiguous log plane; convolution.base.output=output.region.
// crop=unclamped row-major log convolution. Inputs finite/nonnegative; clamped
// log and cropped log exp domain[-32,32]. Outside domain produces NaN, no fallback.
// All tensors caller-owned device-local; record has no allocation/map/readback/
// CPU tensor math/submit/wait. Input/output views allow padded rows/sample stride.
class ResidentLocalAdaptation{
public:
 ResidentLocalAdaptation(const vk::raii::Device&,const vk::raii::PhysicalDevice&,const LocalAdaptationBuffers&,const OpticalNarrowingShaders&,const std::filesystem::path& ioShader,const std::filesystem::path& fftIoShader);
 ~ResidentLocalAdaptation();ResidentLocalAdaptation(ResidentLocalAdaptation&&)noexcept;ResidentLocalAdaptation&operator=(ResidentLocalAdaptation&&)noexcept;
 ResidentLocalAdaptation(const ResidentLocalAdaptation&)=delete;ResidentLocalAdaptation&operator=(const ResidentLocalAdaptation&)=delete;
 void record(vk::CommandBuffer)const;void recordStage(vk::CommandBuffer,AdaptationStage)const;uint32_t dispatchCount()const noexcept;
private:struct Impl;std::unique_ptr<Impl>impl_;
};
}
