#pragma once
#include <IQM/hdrvdp3/fft_gpu_single.h>
#include <array>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>
namespace IQM::HDRVDP3 {
struct SP0BandGpu { unsigned width=0,height=0; FFTBufferRegion region{}; };
struct SP0PyramidGpuResult { std::vector<SP0BandGpu> bands; FFTBufferRegion baseMean{}; unsigned levels=0; };
struct SP0ResidentShaders { std::filesystem::path corrdn; std::filesystem::path baseMean; };
// Caller-owned device-local buffers. Filter buffers must contain float filter coefficients in
// row-major fy*k+fx order matching sp0_single.cpp (double literals cast to float).
// workA/workB are scratch buffers sized for max(w*h) floats. bandRegions has levels+2 entries
// sized for each band output. baseMean is 1 float.
struct SP0ResidentBuffers {
    FFTBufferRegion input;
    FFTBufferRegion lo0Filter;   // 49 floats
    FFTBufferRegion hi0Filter;   // 81 floats
    FFTBufferRegion loFilter;    // 169 floats
    FFTBufferRegion orientFilter;// 81 floats
    FFTBufferRegion workA;
    FFTBufferRegion workB;
    std::vector<FFTBufferRegion> bandRegions; // levels+2
    FFTBufferRegion baseMean; // 1 float
};
class ResidentSP0Pyramid {
public:
    ResidentSP0Pyramid(const vk::raii::Device&,const vk::raii::PhysicalDevice&,const SP0ResidentShaders&,const SP0ResidentBuffers&,unsigned w,unsigned h,double ppd);
    ~ResidentSP0Pyramid(); ResidentSP0Pyramid(ResidentSP0Pyramid&&)noexcept; ResidentSP0Pyramid&operator=(ResidentSP0Pyramid&&)noexcept;
    ResidentSP0Pyramid(const ResidentSP0Pyramid&)=delete; ResidentSP0Pyramid&operator=(const ResidentSP0Pyramid&)=delete;
    void record(vk::CommandBuffer)const;
    uint32_t dispatchCount()const noexcept;
    const SP0PyramidGpuResult& result()const;
private:
    struct Impl; std::unique_ptr<Impl> impl_;
};
}
