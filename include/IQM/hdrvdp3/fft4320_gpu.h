#pragma once
#include <IQM/hdrvdp3/fft_gpu_single.h>
namespace IQM::HDRVDP3 {
enum class FFT4320Mode { RealForward, ComplexForward, ComplexInverse };
// Strides/base are explicit scalar float32 words (not complex samples).
// RealForward input has4320 real samples; output2161 complex samples (2 words).
// Complex modes input/output4320 complex samples. All transforms unnormalized.
struct FFT4320View { FFTBufferRegion region;uint32_t firstScalar=0,sampleStride=0,batchStride=0; };
struct FFT4320Buffers { FFT4320View input,output;std::array<FFTBufferRegion,4> workspace; };
struct FFT4320Shaders {std::filesystem::path io,realOuter,complexOuter;ResidentFFTShaders children;};
// Pinned FFTW3.3.10 radix6/720 graph only; no generic4320 replacement/C2R/2D.
// Caller buffer/job lifetime, no underlying-memory alias and same-queue ownership
// requirements match ResidentFFT. No map/staging/readback/CPU FFT/submit/wait.
class ResidentFFT4320 {
public:
 ResidentFFT4320(const vk::raii::Device&,const vk::raii::PhysicalDevice&,FFT4320Mode,
                 uint32_t batches,const FFT4320Buffers&,const FFT4320Shaders&);
 ~ResidentFFT4320();
 ResidentFFT4320(ResidentFFT4320&&) noexcept;
 ResidentFFT4320& operator=(ResidentFFT4320&&) noexcept;
 ResidentFFT4320(const ResidentFFT4320&)=delete;
 ResidentFFT4320& operator=(const ResidentFFT4320&)=delete;
 void record(vk::CommandBuffer) const;
 // Resident stage composition/debugging: child result in workspace[1].
 // recordOuter consumes that GPU buffer; neither method reads it on the host.
 void recordChildren(vk::CommandBuffer) const;
 void recordOuter(vk::CommandBuffer) const;
 uint32_t dispatchCount() const noexcept;
private:
 struct Impl;std::unique_ptr<Impl> impl_;
};
}
