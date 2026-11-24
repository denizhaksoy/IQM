#pragma once
#include <IQM/hdrvdp3/fft_inverse_gpu.h>
namespace IQM::HDRVDP3 {
struct Inverse4320InplaceShaders {std::filesystem::path copy;std::array<std::filesystem::path,4> kernels;};
// Actual inverse2D CT20/CT12/CT9/n1_2 graph, unnormalized. Not standalone4320.
// All view offsets/strides are COMPLEX samples; three workspace regions each
// >= batches*4320*8 bytes; batch1..4096. No CPU FFT, map/staging/readback,
// submit/wait or buffer allocation. Caller guarantees device-local buffers,
// no underlying allocation alias, same-queue ownership and job/buffer lifetime.
class ResidentFFTInverse4320Inplace {
public:
 ResidentFFTInverse4320Inplace(const vk::raii::Device&,const vk::raii::PhysicalDevice&,uint32_t batches,const ResidentFFTBuffers&,const Inverse4320InplaceShaders&);
 ~ResidentFFTInverse4320Inplace();ResidentFFTInverse4320Inplace(ResidentFFTInverse4320Inplace&&)noexcept;ResidentFFTInverse4320Inplace&operator=(ResidentFFTInverse4320Inplace&&)noexcept;
 ResidentFFTInverse4320Inplace(const ResidentFFTInverse4320Inplace&)=delete;ResidentFFTInverse4320Inplace&operator=(const ResidentFFTInverse4320Inplace&)=delete;
 void record(vk::CommandBuffer)const;uint32_t dispatchCount()const noexcept;
 // Resident diagnostic composition: kernel0=gather,1=t1bv9,2=t1bv12,3=t3bv20.
 // Uses the same bound workspace regions; no host operation.
 void recordKernel(vk::CommandBuffer,uint32_t kernel)const;
private:struct Impl;std::unique_ptr<Impl>impl_;
};
}
