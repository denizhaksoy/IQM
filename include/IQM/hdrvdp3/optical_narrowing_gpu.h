#pragma once
#include <IQM/hdrvdp3/optical_resident_gpu.h>
#include <IQM/hdrvdp3/narrowing_resident_gpu.h>
#include <IQM/hdrvdp3/fft_inverse2d_inplace_gpu.h>
namespace IQM::HDRVDP3 {
struct OpticalNarrowingBuffers{OpticalResidentBuffers base;FFTBufferRegion alternateInverse,fftStatus,productStatus;};
struct OpticalNarrowingShaders{OpticalResidentShaders base;Inverse4320InplaceShaders inverse4320;std::filesystem::path narrowing,select;};
// Fixed Phase1E optical profile, extended with actual real-storage/in-place graph.
// Both native inverse graphs execute sequentially, then GPU selects using P's
// classification. Correctness first: no CPU status branch, readback or fallback.
// Three additional caller-owned disjoint regions: full complex inverse +2 words.
// Original Phase1E class and its explicit rejection semantics remain unchanged.
class ResidentOpticalNarrowing {
public:
 ResidentOpticalNarrowing(const vk::raii::Device&,const vk::raii::PhysicalDevice&,const OpticalNarrowingBuffers&,const OpticalNarrowingShaders&);
 ~ResidentOpticalNarrowing();ResidentOpticalNarrowing(ResidentOpticalNarrowing&&)noexcept;ResidentOpticalNarrowing&operator=(ResidentOpticalNarrowing&&)noexcept;
 ResidentOpticalNarrowing(const ResidentOpticalNarrowing&)=delete;ResidentOpticalNarrowing&operator=(const ResidentOpticalNarrowing&)=delete;
 void record(vk::CommandBuffer)const;void recordStage(vk::CommandBuffer,OpticalStage)const;
 uint32_t dispatchCount()const noexcept;
private:struct Impl;std::unique_ptr<Impl>impl_;
};
}
