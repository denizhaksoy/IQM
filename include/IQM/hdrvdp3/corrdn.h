/*
 * Image Quality Metrics - HDR-VDP-3 Spatial Correlation
 * Deniz Aksoy - 2026
 */

#ifndef HDRVDP3_CORRDN_H
#define HDRVDP3_CORRDN_H

#include <IQM/base/vulkan_runtime.h>
#include <vulkan/vulkan.hpp>

namespace IQM {
namespace HDRVDP3 {

struct CorrDnInput {
    const vk::raii::Device* device;
    const vk::raii::CommandBuffer* cmdBuf;
    const vk::raii::ImageView* ivInput;
    const vk::raii::ImageView* ivOutput;
    const vk::raii::Buffer* filterBuffer;
    unsigned width, height;
    int filter_size;
    int step_x;
    int step_y;
    int start_x;
    int start_y;
    bool use_non_fma = false;
};

class CorrDn {
public:
    explicit CorrDn(const vk::raii::Device& device);
    void compute(const CorrDnInput& input) const;

private:
    vk::raii::DescriptorPool descPool;
    vk::raii::PipelineLayout layout = VK_NULL_HANDLE;
    vk::raii::Pipeline pipeline = VK_NULL_HANDLE;
    vk::raii::Pipeline pipelineNonFma = VK_NULL_HANDLE;
    vk::raii::DescriptorSetLayout descSetLayout = VK_NULL_HANDLE;
    vk::raii::DescriptorSet descSet = VK_NULL_HANDLE;
};

} // namespace HDRVDP3
} // namespace IQM

#endif // HDRVDP3_CORRDN_H