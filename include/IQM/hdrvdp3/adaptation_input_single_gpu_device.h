#pragma once
#include <IQM/hdrvdp3/pq_display_single_gpu.h>
#include <vulkan/vulkan_raii.hpp>
namespace IQM { class VulkanInstance; }
namespace IQM::HDRVDP3 {
GpuFloatBuffer adaptation_input_single_gpu_device(const IQM::VulkanInstance& instance,
    const std::string& shader, vk::Buffer lmsrClamped, unsigned pixelCount);
}
