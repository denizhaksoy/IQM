/*
 * Image Quality Metrics - HDR-VDP-3 Spatial Correlation
 * Petr Volf - 2026
 */

#include <IQM/hdrvdp3/corrdn.h>
#include <IQM/base/vulkan_runtime.h>
#include <cstring>

static std::vector<uint32_t> src =
#include <hdrvdp3/hdrvdp3_corrdn.inc>
;

using IQM::GPU::VulkanRuntime;

namespace IQM {
namespace HDRVDP3 {

constexpr int MAX_FILTER_SIZE = 169;  // 13x13

CorrDn::CorrDn(const vk::raii::Device& device):
    descPool(VulkanRuntime::createDescPool(device, 8, {
        vk::DescriptorPoolSize{.type = vk::DescriptorType::eStorageImage, .descriptorCount = 8},
        vk::DescriptorPoolSize{.type = vk::DescriptorType::eStorageBuffer, .descriptorCount = 8}
    }))
{
    const auto sm = VulkanRuntime::createShaderModule(device, src);

    this->descSetLayout = std::move(VulkanRuntime::createDescLayout(device, {
        {vk::DescriptorType::eStorageImage, 1},
        {vk::DescriptorType::eStorageImage, 1},
        {vk::DescriptorType::eStorageBuffer, 1},
    }));

    const std::vector<vk::DescriptorSetLayout> descLayouts = {
        *this->descSetLayout,
    };

    vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo = {
        .descriptorPool = this->descPool,
        .descriptorSetCount = static_cast<uint32_t>(descLayouts.size()),
        .pSetLayouts = descLayouts.data()
    };

    this->descSet = std::move(vk::raii::DescriptorSets{device, descriptorSetAllocateInfo}.front());

    const auto pushConstantRanges = VulkanRuntime::createPushConstantRange(sizeof(int) * 5);
    this->layout = VulkanRuntime::createPipelineLayout(device, descLayouts, pushConstantRanges);
    this->pipeline = VulkanRuntime::createComputePipeline(device, sm, this->layout);
}

void CorrDn::compute(const CorrDnInput& input) const {
    auto imgIn = VulkanRuntime::createImageInfos({input.ivInput});
    auto imgOut = VulkanRuntime::createImageInfos({input.ivOutput});

    std::vector<vk::DescriptorBufferInfo> bufInfo = {
        vk::DescriptorBufferInfo{
            .buffer = *input.filterBuffer,
            .offset = 0,
            .range = VK_WHOLE_SIZE,
        }
    };

    auto writeSetIn = VulkanRuntime::createWriteSet(
        this->descSet,
        0,
        imgIn
    );

    auto writeSetOut = VulkanRuntime::createWriteSet(
        this->descSet,
        1,
        imgOut
    );

    auto writeSetFilter = VulkanRuntime::createWriteSet(
        this->descSet,
        2,
        bufInfo
    );

    input.device->updateDescriptorSets({
        writeSetIn, writeSetOut, writeSetFilter,
    }, nullptr);

    input.cmdBuf->bindPipeline(vk::PipelineBindPoint::eCompute, this->pipeline);
    input.cmdBuf->bindDescriptorSets(vk::PipelineBindPoint::eCompute, this->layout, 0, {this->descSet}, {});

    struct PushConstants {
        int filter_size;
        int step_x;
        int step_y;
        int start_x;
        int start_y;
    } push_consts;

    push_consts.filter_size = input.filter_size;
    push_consts.step_x = input.step_x;
    push_consts.step_y = input.step_y;
    push_consts.start_x = input.start_x;
    push_consts.start_y = input.start_y;

    input.cmdBuf->pushConstants<PushConstants>(this->layout, vk::ShaderStageFlagBits::eCompute, 0, push_consts);

    // Shader works in 16x16 tiles
    auto [groupsX, groupsY] = VulkanRuntime::compute2DGroupCounts(input.width, input.height, 16);

    input.cmdBuf->dispatch(groupsX, groupsY, 1);

    vk::MemoryBarrier memoryBarrier = {
        .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eTransferRead,
    };

    input.cmdBuf->pipelineBarrier(
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eComputeShader | vk::PipelineStageFlagBits::eTransfer,
        vk::DependencyFlagBits::eDeviceGroup, {memoryBarrier}, {}, {}
    );
}

} // namespace HDRVDP3
} // namespace IQM