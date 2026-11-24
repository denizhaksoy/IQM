#pragma once
#include <IQM/hdrvdp3/fft_gpu_single.h>
namespace IQM::HDRVDP3 {
struct ResidualGpuBuffers {
    FFTBufferRegion reference, test, spectrum, selection, frequency;
};
// Caller owns buffers, queue, pool and synchronization. Construct outside recording.
// All resources must survive GPU execution. Requires enabled shaderFloat64.
class ResidentResidualSingle {
public:
    ResidentResidualSingle(const vk::raii::Device&, const vk::raii::PhysicalDevice&,
        vk::Queue, vk::CommandPool, const ResidualGpuBuffers&,
        const std::filesystem::path& runtime, unsigned width, unsigned height,
        unsigned bandCount, double ppd, bool float64Enabled);
    ~ResidentResidualSingle();
    void record(vk::CommandBuffer);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
