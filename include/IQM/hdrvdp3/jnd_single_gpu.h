#pragma once
#include <IQM/hdrvdp3/pq_display_single_gpu.h>
#include <vulkan/vulkan_raii.hpp>
#include <vector>
namespace IQM { class VulkanInstance; }
namespace IQM::HDRVDP3 {
GpuFloatBuffer rgb_to_jnd_response_single_gpu_device(const IQM::VulkanInstance& instance,
    const std::string& shader, vk::Buffer lmsrClamped, vk::Buffer adaptation,
    unsigned width, unsigned height, double area, double age);
std::vector<float> rgb_to_jnd_response_single_gpu_readback(const IQM::VulkanInstance& instance,
    const std::string& shader, vk::Buffer lmsrClamped, vk::Buffer adaptation,
    unsigned width, unsigned height, double area, double age);
}
