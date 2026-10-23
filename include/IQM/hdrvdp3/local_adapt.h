/*
 * Image Quality Metrics - HDR-VDP-3 Local Adaptation
 * Deniz Aksoy - 2026
 */

#ifndef HDRVDP3_LOCAL_ADAPT_H
#define HDRVDP3_LOCAL_ADAPT_H

#include <IQM/base/vulkan_runtime.h>
#include <vulkan/vulkan.hpp>
#include <vector>
#include <memory>
#include <array>
#include <string>

namespace IQM {
class VulkanInstance;
namespace HDRVDP3 {

// ============================================================================
// Original separable Local Adaptation (preserved for comparison)
// ============================================================================

struct LocalAdaptInput {
    const vk::raii::Device* device;
    const vk::raii::CommandBuffer* cmdBuf;
    const vk::raii::ImageView* ivInput;       // 64x64 R32_SFLOAT
    const vk::raii::ImageView* ivLogClamp;    // 64x64 R32_SFLOAT (temp)
    const vk::raii::ImageView* ivPadded;      // 128x128 R32_SFLOAT (temp)
    const vk::raii::ImageView* ivGaussH;      // 128x128 R32_SFLOAT (temp)
    const vk::raii::ImageView* ivGaussV;      // 128x128 R32_SFLOAT (temp)
    const vk::raii::ImageView* ivOutput;      // 64x64 R32_SFLOAT
    float ppd;                                // pixels per degree
};

class LocalAdapt {
public:
    explicit LocalAdapt(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice);
    void compute(const LocalAdaptInput& input) const;
    const vk::raii::DeviceMemory& getKernelMemory() const { return kernelMemory; }
    int getKernelSize() const { return kernelSize; }

private:
    vk::raii::DescriptorPool descPool;
    vk::raii::PipelineLayout layout = VK_NULL_HANDLE;
    vk::raii::Pipeline pipelineLogClamp = VK_NULL_HANDLE;
    vk::raii::Pipeline pipelinePad = VK_NULL_HANDLE;
    vk::raii::Pipeline pipelineGaussH = VK_NULL_HANDLE;
    vk::raii::Pipeline pipelineGaussV = VK_NULL_HANDLE;
    vk::raii::Pipeline pipelineCropExp = VK_NULL_HANDLE;
    vk::raii::DescriptorSetLayout descSetLayout = VK_NULL_HANDLE;
    std::vector<vk::raii::DescriptorSet> descSets;
    vk::raii::Buffer kernelBuffer = VK_NULL_HANDLE;
    vk::raii::DeviceMemory kernelMemory = VK_NULL_HANDLE;
    int kernelSize = 31;
};

// ============================================================================
// FFT-based Local Adaptation (matching fast_gauss.m for sigma >= 4.3)
// Also supports optical MTF with symmetric post-padding and output clamping.
// ============================================================================

enum class FFTFilterMode : uint32_t { LocalAdaptation = 0, OpticalMTF = 1 };

struct LocalAdaptFFTInput {
    const vk::raii::Device* device;
    const vk::raii::PhysicalDevice* physicalDevice;
    const vk::raii::CommandBuffer* cmdBuf;
    const vk::raii::CommandPool* cmdPool;
    const vk::raii::Queue* queue;
    const vk::raii::ImageView* ivInput;       // 64x64 R32_SFLOAT
    const vk::raii::ImageView* ivOutput;      // 64x64 R32_SFLOAT
    float ppd;
    FFTFilterMode mode = FFTFilterMode::LocalAdaptation;
    bool columnFirst = false; // Align forward/inverse logical column traversal.
    bool useTwiddleLUT = false; // Numerical experiment: precomputed roots of unity.
    // Explicit test-only backend: runtime SPIR-V generated from the licensed
    // FFTW parity fixtures. Empty retains VkFFT. Requires columnFirst=true.
    // Rectangular64x128/128x64 uses explicit combined ABI64256,ppd30 and
    // reference math assets;this low-level contract does not widen quality API.
    std::string referenceShaderPath;
    // Optional explicit numerical math shaders, reference backend only.
    std::string referenceMathDirectory;
    // Optional transfer-destination buffer: four 128x128 complex float snapshots
    // (prepared, forward FFT, filtered FFT, inverse FFT). Caller synchronizes.
    const vk::raii::Buffer* traceBuffer = nullptr;
    uint32_t width = 64;
    uint32_t height = 64;
};

class LocalAdaptFFT {
public:
    LocalAdaptFFT(
        const vk::raii::Device& device,
        const vk::raii::PhysicalDevice& physicalDevice
    );

    ~LocalAdaptFFT();

    LocalAdaptFFT(const LocalAdaptFFT&) = delete;
    LocalAdaptFFT& operator=(const LocalAdaptFFT&) = delete;

    void compute(const LocalAdaptFFTInput& input);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace HDRVDP3
} // namespace IQM

#endif // HDRVDP3_LOCAL_ADAPT_H
