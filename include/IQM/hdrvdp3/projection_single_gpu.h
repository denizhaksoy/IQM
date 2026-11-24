#pragma once
#include <span>
#include <string>
#include <vector>
#include <vulkan/vulkan_raii.hpp>
namespace IQM { class VulkanInstance; }
namespace IQM::HDRVDP3 { struct GpuFloatBuffer; }
namespace IQM::HDRVDP3 {
enum class SingleProjectionKind {NativeColor,LMSR};
// Explicit Vulkan elementwise matrix stage, up to3840x2160 pixels.
// RGB is three planes. Coefficients are output-major (three per output).
// NativeColor returns3 planes with1e-6 floor; LMSR returns4 clamped planes.
// Preserves plane layout; no gamma, optical filtering or implicit CPU fallback.
std::vector<float> project_rgb_single_gpu(const IQM::VulkanInstance& instance,
    const std::string& shader,std::span<const float> rgb,
    std::span<const float> coefficients,SingleProjectionKind kind);
// Device-resident variant: rgbInput is a device-local/storage buffer containing 3*pixelCount floats.
// Output is returned as host vector for compatibility; use project_rgb_single_gpu_device for resident output.
std::vector<float> project_rgb_single_gpu_from_device(const IQM::VulkanInstance& instance,
    const std::string& shader,vk::Buffer rgbInput,unsigned pixelCount,
    std::span<const float> coefficients,SingleProjectionKind kind);
// Device-resident projection. Output buffer is device-local/storage; no host readback.
GpuFloatBuffer project_rgb_single_gpu_device(const IQM::VulkanInstance& instance,
    const std::string& shader,vk::Buffer rgbInput,unsigned pixelCount,
    std::span<const float> coefficients,SingleProjectionKind kind);
}
