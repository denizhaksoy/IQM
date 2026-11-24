#pragma once

#include <IQM/hdrvdp3/fft_gpu_single.h>
#include <memory>
#include <vector>

namespace IQM::HDRVDP3 {

enum class CSFKernelSingle {
    Log,
    Resize,
    Generate,
    Sample,
    Neighbor,
    MaskedDifference,
    Comparison,
    ComparisonResize,
    PoolingTerms,
    PoolingReduce,
    FinalScore,
    RGBResize
};

class CSFDispatchSingle {
public:
    CSFDispatchSingle(
        const vk::raii::Device& device,
        const vk::raii::PhysicalDevice& physical,
        const std::filesystem::path& runtime,
        CSFKernelSingle kind,
        const std::vector<FFTBufferRegion>& regions,
        const std::vector<uint32_t>& push,
        unsigned groupsX,
        unsigned groupsY,
        bool float64Enabled
    );

    ~CSFDispatchSingle();

    CSFDispatchSingle(const CSFDispatchSingle&) = delete;
    CSFDispatchSingle& operator=(
        const CSFDispatchSingle&) = delete;

    void record(vk::CommandBuffer cmd);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}