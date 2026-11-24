#pragma once
#include <IQM/hdrvdp3/fft_gpu_single.h>
namespace IQM::HDRVDP3 {
// Reproduce Octave's all_elements_are_real/real-storage imaginary-zero semantics.
// Contiguous interleaved f32 complex tensor, modified in place. If all imaginary
// magnitudes are zero, canonicalize every imaginary word to +0; otherwise copy
// nothing/change nothing. Real words always remain bit-identical (even NaNs).
// status is a separate 4-byte GPU word: 0=real, 1=nonreal. Reset on GPU per record.
// Caller owns device-local storage and command submission/lifetime. Status needs
// STORAGE_BUFFER|TRANSFER_DST usage. No CPU tensor processing, readback or wait.
// This helper does NOT implement the required real-input inverse FFT graph.
class ResidentComplexNarrowing {
public:
 ResidentComplexNarrowing(const vk::raii::Device&,const vk::raii::PhysicalDevice&,
                         FFTBufferRegion tensor,FFTBufferRegion status,
                         uint32_t complexCount,const std::filesystem::path& shader);
 ~ResidentComplexNarrowing();
 ResidentComplexNarrowing(ResidentComplexNarrowing&&)noexcept;
 ResidentComplexNarrowing&operator=(ResidentComplexNarrowing&&)noexcept;
 ResidentComplexNarrowing(const ResidentComplexNarrowing&)=delete;
 ResidentComplexNarrowing&operator=(const ResidentComplexNarrowing&)=delete;
 void record(vk::CommandBuffer)const;
 static constexpr uint32_t dispatchCount()noexcept{return 2;}
private:struct Impl;std::unique_ptr<Impl>impl_;
};
}
