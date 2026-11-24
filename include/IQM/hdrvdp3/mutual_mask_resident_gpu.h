#pragma once
#include <IQM/hdrvdp3/fft_gpu_single.h>
#include <filesystem>
#include <memory>
#include <vulkan/vulkan_raii.hpp>
namespace IQM::HDRVDP3 {
// Buffer-resident mutual/self mask for official general_single_pathway semantics.
// Inputs/outputs are row-major float bands with identical width/height.
// Production record() performs compute + compute barrier only; no host readback.
class ResidentMutualMaskSingle {
public:
    ResidentMutualMaskSingle(const vk::raii::Device&, const vk::raii::PhysicalDevice&,
        const std::filesystem::path& shaderSpv,
        FFTBufferRegion reference, FFTBufferRegion test, FFTBufferRegion output,
        unsigned width, unsigned height);
    ~ResidentMutualMaskSingle();
    ResidentMutualMaskSingle(ResidentMutualMaskSingle&&) noexcept;
    ResidentMutualMaskSingle& operator=(ResidentMutualMaskSingle&&) noexcept;
    ResidentMutualMaskSingle(const ResidentMutualMaskSingle&)=delete;
    ResidentMutualMaskSingle& operator=(const ResidentMutualMaskSingle&)=delete;
    void record(vk::CommandBuffer) const;
    uint32_t dispatchCount() const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
