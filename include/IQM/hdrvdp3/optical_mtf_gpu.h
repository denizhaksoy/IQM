#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <span>
#include <vector>
#include <filesystem>
namespace IQM { class VulkanInstance; }
namespace IQM::HDRVDP3 {
struct GpuFloatBuffer;
// Production-equivalent GPU Optical MTF: input/output are device-local row-major 3-plane float buffers.
// Implements filter_optical_rgb_single semantics: symmetric post-pad, generate_mtf_filter, C2C FFT,
// 1/N normalization (VkFFT normalize=1), crop top-left, clamp [1e-5,1e10].
GpuFloatBuffer filter_optical_rgb_single_gpu(const IQM::VulkanInstance& instance,
    vk::Buffer inputRgb, unsigned width, unsigned height, float ppd);
// Resident 4K Optical MTF path; static MTF coefficients are prepared on the host.
// Input/output remain device-local planar RGB float buffers.
GpuFloatBuffer filter_optical_rgb_resident_gpu(const IQM::VulkanInstance& instance,
    vk::Buffer inputRgb, unsigned width, unsigned height, double ppd,
    const std::filesystem::path& runtime);
// Readback helper for bounded parity tests only.
std::vector<float> filter_optical_rgb_single_gpu_readback(const IQM::VulkanInstance& instance,
    vk::Buffer inputRgb, unsigned width, unsigned height, float ppd);
}