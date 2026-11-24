#pragma once
#include <IQM/hdrvdp3/fft2d_gpu.h>
namespace IQM::HDRVDP3 {
struct Inverse7680Shaders {std::filesystem::path copy;std::array<std::filesystem::path,4> kernels;};
// Actual inverse2D CT20/CT16/CT6/n1_4 graph, unnormalized. Not standalone7680.
// All view offsets/strides are COMPLEX samples; three workspace regions each
// >= batches*7680*8 bytes; batch1..4096. No CPU FFT, map/staging/readback,
// submit/wait or buffer allocation. Caller guarantees device-local buffers,
// no underlying allocation alias, same-queue ownership and job/buffer lifetime.
class ResidentFFTInverse7680 {
public:
 ResidentFFTInverse7680(const vk::raii::Device&,const vk::raii::PhysicalDevice&,uint32_t batches,const ResidentFFTBuffers&,const Inverse7680Shaders&);
 ~ResidentFFTInverse7680();ResidentFFTInverse7680(ResidentFFTInverse7680&&)noexcept;ResidentFFTInverse7680&operator=(ResidentFFTInverse7680&&)noexcept;
 ResidentFFTInverse7680(const ResidentFFTInverse7680&)=delete;ResidentFFTInverse7680&operator=(const ResidentFFTInverse7680&)=delete;
 void record(vk::CommandBuffer)const;uint32_t dispatchCount()const noexcept;
 // Resident diagnostic composition: kernel0=gather,1=t1bv6,2=t3bv16,3=t3bv20.
 // Uses the same bound workspace regions; no host operation.
 void recordKernel(vk::CommandBuffer,uint32_t kernel)const;
private:struct Impl;std::unique_ptr<Impl>impl_;
};
struct FFTInverse2DBuffers {FFT2DComplexView input,output;FFTBufferRegion intermediate;std::array<FFTBufferRegion,4>workspace;};
// axis4320.children.fft720 must bind fft720_inverse_context.comp's SPIR-V;
// original Phase1A720 is preserved but differs on the tested signed-zero case.
struct FFTInverse2DShaders {FFT4320Shaders axis4320;Inverse7680Shaders axis7680;};
// Fixed7680x4320 full complex C2C inverse, no normalization/crop/filter.
// Input/output views use complex samples. Set xStride4320,yStride1 for canonical
// full planes; shared FFT2DComplexView defaults belong to the R2C half plane.
// Intermediate >=7680*4320*8 bytes, four workspaces each >=
// max(chunks.real4320*4320,chunks.complex7680*7680)*8 bytes. "real4320" is
// the shared scheduling field name; BOTH axes in this graph are COMPLEX.
// Same residency/ownership/lifetime contract as the axis interface above.
class ResidentFFT2DInverse {
public:
 ResidentFFT2DInverse(const vk::raii::Device&,const vk::raii::PhysicalDevice&,const FFTInverse2DBuffers&,const FFTInverse2DShaders&,FFT2DChunks={});
 ~ResidentFFT2DInverse();ResidentFFT2DInverse(ResidentFFT2DInverse&&)noexcept;ResidentFFT2DInverse&operator=(ResidentFFT2DInverse&&)noexcept;
 ResidentFFT2DInverse(const ResidentFFT2DInverse&)=delete;ResidentFFT2DInverse&operator=(const ResidentFFT2DInverse&)=delete;
 void record(vk::CommandBuffer)const;void record4320(vk::CommandBuffer)const;void record7680(vk::CommandBuffer)const;uint32_t dispatchCount()const noexcept;
private:struct Impl;std::unique_ptr<Impl>impl_;
};
}
