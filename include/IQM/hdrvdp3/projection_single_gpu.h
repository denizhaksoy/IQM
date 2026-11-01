#pragma once
#include <span>
#include <string>
#include <vector>
namespace IQM { class VulkanInstance; }
namespace IQM::HDRVDP3 {
enum class SingleProjectionKind {NativeColor,LMSR};
// Explicit Vulkan elementwise matrix stage, up to3840x2160 pixels.
// RGB is three planes. Coefficients are output-major (three per output).
// NativeColor returns3 planes with1e-6 floor; LMSR returns4 clamped planes.
// Preserves plane layout; no gamma, optical filtering or implicit CPU fallback.
std::vector<float> project_rgb_single_gpu(const IQM::VulkanInstance& instance,
    const std::string& shader,std::span<const float> rgb,
    std::span<const float> coefficients,SingleProjectionKind kind);
}
