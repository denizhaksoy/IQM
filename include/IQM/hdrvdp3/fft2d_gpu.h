#pragma once
#include <IQM/hdrvdp3/fft4320_gpu.h>
namespace IQM::HDRVDP3 {
// Pinned FFTW(7680,4320), threads3/flags82. x-major storage, y contiguous.
// Input scalar words: first + x*xStride + y*yStride (4320 real samples).
struct FFT2DRealView { FFTBufferRegion region; uint32_t first=0,xStride=4320,yStride=1; };
// Half output complex samples: first + kx*xStride + ky*yStride, ky0..2160.
struct FFT2DComplexView { FFTBufferRegion region; uint32_t first=0,xStride=2161,yStride=1; };
struct FFT2DBuffers {
 FFT2DRealView input; FFT2DComplexView output;
 FFTBufferRegion intermediate; // >=7680*2161*8 bytes; canonical half plane
 std::array<FFTBufferRegion,4> workspace; // reused sequentially across chunks
};
struct FFT2DChunks {uint32_t real4320=1024,complex7680=512;};
struct FFT2DShaders {FFT4320Shaders axis4320;ResidentFFTShaders axis7680;};
// Caller owns all device buffers, command recording/submission and lifetimes.
// No CPU tensor computation, buffer allocation, map, readback, submit or wait.
// Unnormalized R2C half-spectrum ONLY, no inverse, filter or HDR preprocessing.
class ResidentFFT2DForward {
public:
 ResidentFFT2DForward(const vk::raii::Device&,const vk::raii::PhysicalDevice&,
                     const FFT2DBuffers&,const FFT2DShaders&,FFT2DChunks={});
 ~ResidentFFT2DForward();
 ResidentFFT2DForward(ResidentFFT2DForward&&) noexcept;
 ResidentFFT2DForward& operator=(ResidentFFT2DForward&&) noexcept;
 ResidentFFT2DForward(const ResidentFFT2DForward&)=delete;
 ResidentFFT2DForward& operator=(const ResidentFFT2DForward&)=delete;
 void record(vk::CommandBuffer) const;
 void recordRealAxis(vk::CommandBuffer) const;
 void recordComplexAxis(vk::CommandBuffer) const;
 uint32_t dispatchCount()const noexcept;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
