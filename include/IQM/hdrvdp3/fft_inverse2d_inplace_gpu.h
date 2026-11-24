#pragma once
#include <IQM/hdrvdp3/fft_inverse4320_inplace_gpu.h>
namespace IQM::HDRVDP3 {
struct FFTInverse2DInplaceShaders{Inverse4320InplaceShaders axis4320;Inverse7680Shaders axis7680;};
// Fixed7680x4320 full complex C2C inverse, no normalization/crop/filter.
// Input/output views use complex samples. Set xStride4320,yStride1 for canonical
// full planes; shared FFT2DComplexView defaults belong to the R2C half plane.
// Intermediate >=7680*4320*8 bytes, four workspaces each >=
// max(chunks.real4320*4320,chunks.complex7680*7680)*8 bytes. "real4320" is
// the shared scheduling field name; BOTH axes in this graph are COMPLEX.
// Same residency/ownership/lifetime contract as the axis interface above.
class ResidentFFT2DInverseInplace {
public:
 ResidentFFT2DInverseInplace(const vk::raii::Device&,const vk::raii::PhysicalDevice&,const FFTInverse2DBuffers&,const FFTInverse2DInplaceShaders&,FFT2DChunks={});
 ~ResidentFFT2DInverseInplace();ResidentFFT2DInverseInplace(ResidentFFT2DInverseInplace&&)noexcept;ResidentFFT2DInverseInplace&operator=(ResidentFFT2DInverseInplace&&)noexcept;
 ResidentFFT2DInverseInplace(const ResidentFFT2DInverseInplace&)=delete;ResidentFFT2DInverseInplace&operator=(const ResidentFFT2DInverseInplace&)=delete;
 void record(vk::CommandBuffer)const;void record4320(vk::CommandBuffer)const;void record7680(vk::CommandBuffer)const;uint32_t dispatchCount()const noexcept;
private:struct Impl;std::unique_ptr<Impl>impl_;
};
}
