#pragma once
#include <filesystem>
#include <IQM/hdrvdp3/pq_display_single_gpu.h>
#include <vulkan/vulkan_raii.hpp>
#include <vector>
namespace IQM { class VulkanInstance; }
namespace IQM::HDRVDP3 {
GpuFloatBuffer local_adaptation_resident_gpu(const IQM::VulkanInstance& instance,
    vk::Buffer inputAdaptation, unsigned width, unsigned height, double ppd,
    const std::filesystem::path& runtime);
GpuFloatBuffer local_adaptation_single_gpu(const IQM::VulkanInstance& instance,
    vk::Buffer inputAdaptation, unsigned width, unsigned height, float ppd);
std::vector<float> local_adaptation_single_gpu_readback(const IQM::VulkanInstance& instance,
    vk::Buffer inputAdaptation, unsigned width, unsigned height, float ppd);
}
