#pragma once
#include <span>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan_raii.hpp>
namespace IQM { class VulkanInstance; }
namespace IQM::HDRVDP3 {
struct PQDisplaySingle;
struct GpuFloatBuffer {
 vk::raii::Buffer buffer{nullptr};
 vk::raii::DeviceMemory memory{nullptr};
 unsigned bytes=0;
 unsigned count=0;
};
// Elementwise ST2084 normalized PQ [0,1] -> absolute linear RGB + ambient, clamped to peak+reflected.
// Device-resident output; no host readback. Caller provides already-normalized float input.
GpuFloatBuffer pq_to_display_single_gpu(const IQM::VulkanInstance& instance,
 const std::string& shader,std::span<const float> encoded,const PQDisplaySingle& display);
// Convenience readback for parity testing only.
std::vector<float> pq_to_display_single_gpu_readback(const IQM::VulkanInstance& instance,
 const std::string& shader,std::span<const float> encoded,const PQDisplaySingle& display);
}
