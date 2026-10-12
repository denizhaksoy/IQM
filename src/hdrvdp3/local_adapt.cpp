/*
 * Image Quality Metrics - HDR-VDP-3 Local Adaptation
 * Petr Volf - 2026
 */

#include <IQM/hdrvdp3/local_adapt.h>
#include <IQM/base/vulkan_runtime.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <cstring>
#include <array>
#include <numeric>

namespace IQM {
namespace HDRVDP3 {

static int computeKernelSize(float sigma) {
    // Gaussian kernel radius should cover enough of the Gaussian for good accuracy
    // with circular convolution on 128x128. The convergence experiment showed:
    // - sigma=4.96 (ppd=30): 51 taps optimal (31 too small, >51 no float32 improvement)
    // - Higher sigma: need larger kernel
    // Use radius = ceil(5 * sigma) clamped to [15, 40] -> ksize in [31, 81]
    int radius = static_cast<int>(std::ceil(5.0f * sigma));
    int ksize = 2 * radius + 1;
    ksize = std::max(31, ksize);
    ksize = std::min(81, ksize);
    // Ensure odd
    if (ksize % 2 == 0) ksize++;
    return ksize;
}

static std::vector<float> generateGaussianKernel2D(float sigma, unsigned width, unsigned height) {
    // Generate 2D frequency-domain Gaussian kernel matching fast_gauss.m exactly
    // KX0 = mod(0.5 + (0:(M-1))/M, 1) - 0.5
    // KX1 = KX0 * (2*pi)
    // KY0 = mod(0.5 + (0:(N-1))/N, 1) - 0.5
    // KY1 = KY0 * (2*pi)
    // K = exp(-0.5 * (XX.^2 + YY.^2) * sigma^2)
    //
    // This creates a 2D filter in the frequency domain (NOT fftshifted)
    // DC component is at index (0, 0)
    
    const float two_pi = 2.0f * 3.14159265358979323846f;
    const float sigma2 = sigma * sigma;
    
    std::vector<float> kernel(width * height);
    
    for (unsigned y = 0; y < height; ++y) {
        float ky0 = std::fmod(0.5f + static_cast<float>(y) / static_cast<float>(height), 1.0f) - 0.5f;
        float ky1 = ky0 * two_pi;
        
        for (unsigned x = 0; x < width; ++x) {
            float kx0 = std::fmod(0.5f + static_cast<float>(x) / static_cast<float>(width), 1.0f) - 0.5f;
            float kx1 = kx0 * two_pi;
            
            float val = std::exp(-0.5f * (kx1 * kx1 + ky1 * ky1) * sigma2);
            kernel[y * width + x] = val;
        }
    }
    
    return kernel;
}

static uint32_t findMemoryType(
    const vk::raii::PhysicalDevice& physicalDevice,
    uint32_t typeBits,
    vk::MemoryPropertyFlags requirementsMask)
{
    auto memoryProperties = physicalDevice.getMemoryProperties();
    uint32_t typeIndex = static_cast<uint32_t>(~0);
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if ((typeBits & 1) && ((memoryProperties.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask)) {
            typeIndex = i;
            break;
        }
        typeBits >>= 1;
    }
    return typeIndex;
}

static std::vector<float> generateGaussianKernel1D(float sigma, int ksize) {
    const int radius = ksize / 2;
    
    // Generate 1D Gaussian kernel (matching the 1D slice used for separable convolution)
    // For separable convolution, we need a 1D kernel that sums to 1
    std::vector<float> kernel1D(ksize);
    float sum1D = 0.0f;
    for (int i = 0; i < ksize; ++i) {
        float dx = static_cast<float>(i - radius);
        float val = std::exp(-0.5f * dx * dx / (sigma * sigma));
        kernel1D[i] = val;
        sum1D += val;
    }
    for (float& v : kernel1D) {
        v /= sum1D;
    }
    
    return kernel1D;
}

LocalAdapt::LocalAdapt(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice)
    : descPool(IQM::GPU::VulkanRuntime::createDescPool(device, 5, {
        vk::DescriptorPoolSize{.type = vk::DescriptorType::eStorageImage, .descriptorCount = 10},
        vk::DescriptorPoolSize{.type = vk::DescriptorType::eStorageBuffer, .descriptorCount = 5},
    }))
{
    // Generate Gaussian kernels for sigma = 10^-0.781367 * ppd
    // For ppd=30, sigma = 4.9631, ksize = 51 (based on convergence experiment)
    const float sigma = std::pow(10.0f, -0.781367f) * 30.0f;
    const int ksize = computeKernelSize(sigma);
    auto kernel1D = generateGaussianKernel1D(sigma, ksize);

    // Create 1D kernel buffer for shaders (dynamic size based on sigma)
    const unsigned kernel1DBufferSize = ksize * sizeof(float);
    
    vk::BufferCreateInfo bufferCreateInfo{
        .size = kernel1DBufferSize,
        .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
    };
    vk::raii::Buffer buffer{device, bufferCreateInfo};
    auto memReqs = buffer.getMemoryRequirements();
    const auto memType = findMemoryType(
        physicalDevice,
        memReqs.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    vk::MemoryAllocateInfo memoryAllocateInfo{
        .allocationSize = memReqs.size,
        .memoryTypeIndex = memType
    };

    vk::raii::DeviceMemory memory{device, memoryAllocateInfo};
    buffer.bindMemory(memory, 0);
    
    void* mapped = memory.mapMemory(0, kernel1DBufferSize, {});
    std::memcpy(mapped, kernel1D.data(), kernel1DBufferSize);
    memory.unmapMemory();
    
    kernelBuffer = std::move(buffer);
    kernelMemory = std::move(memory);
    kernelSize = ksize;  // Store for push constant

    // Descriptor pool - need enough sets for 5 descriptor sets + kernel buffer
    descPool = IQM::GPU::VulkanRuntime::createDescPool(device, 5, {
        vk::DescriptorPoolSize{.type = vk::DescriptorType::eStorageImage, .descriptorCount = 10}, // 2 per set * 5 sets
        vk::DescriptorPoolSize{.type = vk::DescriptorType::eStorageBuffer, .descriptorCount = 5}, // 1 per set
    });

    // Descriptor set layout with explicit binding indices:
    // binding 0: input image (STORAGE_IMAGE)
    // binding 1: output image (STORAGE_IMAGE)
    // binding 6: kernel buffer (STORAGE_BUFFER)
    vk::DescriptorSetLayoutBinding bindings[] = {
        { .binding = 0, .descriptorType = vk::DescriptorType::eStorageImage, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute },
        { .binding = 1, .descriptorType = vk::DescriptorType::eStorageImage, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute },
        { .binding = 6, .descriptorType = vk::DescriptorType::eStorageBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute },
    };
    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = 3,
        .pBindings = bindings,
    };
    descSetLayout = vk::raii::DescriptorSetLayout{device, layoutInfo};

    // Create 5 descriptor sets (one per pass)
    std::array<vk::DescriptorSetLayout, 5> setLayouts = {*descSetLayout, *descSetLayout, *descSetLayout, *descSetLayout, *descSetLayout};
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = descPool,
        .descriptorSetCount = 5,
        .pSetLayouts = setLayouts.data(),
    };
    descSets = std::move(vk::raii::DescriptorSets{device, allocInfo});

    // Pipeline layout with push constants for ppd and kernel_size
    struct PushConstants {
        float ppd;
        int kernel_size;
    };
    vk::PushConstantRange pushConstantRange{
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        .size = sizeof(PushConstants),
    };
    layout = IQM::GPU::VulkanRuntime::createPipelineLayout(device, {*descSetLayout}, {pushConstantRange});

    // Load shaders
    static std::vector<uint32_t> logClampSrc =
#include <hdrvdp3/hdrvdp3_local_adapt_logclamp.inc>
    ;
    static std::vector<uint32_t> padSrc =
#include <hdrvdp3/hdrvdp3_local_adapt_pad.inc>
    ;
    static std::vector<uint32_t> gaussHSrc =
#include <hdrvdp3/hdrvdp3_local_adapt_gaussian_h.inc>
    ;
    static std::vector<uint32_t> gaussVSrc =
#include <hdrvdp3/hdrvdp3_local_adapt_gaussian_v.inc>
    ;
    static std::vector<uint32_t> cropExpSrc =
#include <hdrvdp3/hdrvdp3_local_adapt_crop_exp.inc>
    ;

    auto logClampModule = IQM::GPU::VulkanRuntime::createShaderModule(device, logClampSrc);
    auto padModule = IQM::GPU::VulkanRuntime::createShaderModule(device, padSrc);
    auto gaussHModule = IQM::GPU::VulkanRuntime::createShaderModule(device, gaussHSrc);
    auto gaussVModule = IQM::GPU::VulkanRuntime::createShaderModule(device, gaussVSrc);
    auto cropExpModule = IQM::GPU::VulkanRuntime::createShaderModule(device, cropExpSrc);

    pipelineLogClamp = IQM::GPU::VulkanRuntime::createComputePipeline(device, logClampModule, layout);
    pipelinePad = IQM::GPU::VulkanRuntime::createComputePipeline(device, padModule, layout);
    pipelineGaussH = IQM::GPU::VulkanRuntime::createComputePipeline(device, gaussHModule, layout);
    pipelineGaussV = IQM::GPU::VulkanRuntime::createComputePipeline(device, gaussVModule, layout);
    pipelineCropExp = IQM::GPU::VulkanRuntime::createComputePipeline(device, cropExpModule, layout);

    // Update each descriptor set with kernel buffer at binding 6
    vk::DescriptorBufferInfo kernelBufInfo{
        .buffer = kernelBuffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };
    for (size_t i = 0; i < 5; ++i) {
        vk::WriteDescriptorSet kernelWrite{
            .dstSet = descSets[i],
            .dstBinding = 6,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &kernelBufInfo,
        };
        device.updateDescriptorSets({kernelWrite}, {});
    }
}

void LocalAdapt::compute(const LocalAdaptInput& input) const {
    vk::CommandBuffer cmdBuf = *input.cmdBuf;

    struct PushConstants {
        float ppd;
        int kernel_size;
    };
    PushConstants pc{input.ppd, kernelSize};
    cmdBuf.pushConstants<PushConstants>(layout, vk::ShaderStageFlagBits::eCompute, 0, {pc});

    // Helper lambda to update descriptor set bindings 0 and 1
    auto updateBindings01 = [&](const vk::raii::DescriptorSet& descSet, 
                                 const vk::raii::ImageView* inView, 
                                 const vk::raii::ImageView* outView,
                                 const vk::raii::Device* device) {
        auto inInfo = IQM::GPU::VulkanRuntime::createImageInfos({inView});
        auto outInfo = IQM::GPU::VulkanRuntime::createImageInfos({outView});
        vk::WriteDescriptorSet writes[] = {
            { .dstSet = descSet, .dstBinding = 0, .dstArrayElement = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageImage, .pImageInfo = inInfo.data() },
            { .dstSet = descSet, .dstBinding = 1, .dstArrayElement = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageImage, .pImageInfo = outInfo.data() },
        };
        device->updateDescriptorSets(writes, {});
    };

    vk::MemoryBarrier barrier{
        .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead,
    };

    // Pass 1: Log-clamp (64x64 -> 64x64) - descSets[0]
    updateBindings01(descSets[0], input.ivInput, input.ivLogClamp, input.device);
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, layout, 0, {descSets[0]}, {});
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, pipelineLogClamp);
    cmdBuf.dispatch(4, 4, 1);

    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eComputeShader,
        {}, {barrier}, {}, {}
    );

    // Pass 2: Pad 64x64 -> 128x128 (replicate on bottom/right) - descSets[1]
    updateBindings01(descSets[1], input.ivLogClamp, input.ivPadded, input.device);
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, layout, 0, {descSets[1]}, {});
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, pipelinePad);
    cmdBuf.dispatch(8, 8, 1);

    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eComputeShader,
        {}, {barrier}, {}, {}
    );

    // Pass 3: Horizontal Gaussian (128x128 -> 128x128) - descSets[2]
    updateBindings01(descSets[2], input.ivPadded, input.ivGaussH, input.device);
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, layout, 0, {descSets[2]}, {});
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, pipelineGaussH);
    cmdBuf.dispatch(8, 8, 1);

    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eComputeShader,
        {}, {barrier}, {}, {}
    );

    // Pass 4: Vertical Gaussian (128x128 -> 128x128) - descSets[3]
    updateBindings01(descSets[3], input.ivGaussH, input.ivGaussV, input.device);
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, layout, 0, {descSets[3]}, {});
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, pipelineGaussV);
    cmdBuf.dispatch(8, 8, 1);

    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eComputeShader,
        {}, {barrier}, {}, {}
    );

    // Pass 5: Crop 128x128 -> 64x64 and Exp - descSets[4]
    updateBindings01(descSets[4], input.ivGaussV, input.ivOutput, input.device);
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, layout, 0, {descSets[4]}, {});
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, pipelineCropExp);
    cmdBuf.dispatch(4, 4, 1);

    // Final barrier for output
    vk::MemoryBarrier finalBarrier{
        .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eTransferRead,
    };
    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eComputeShader | vk::PipelineStageFlagBits::eTransfer,
        {}, {finalBarrier}, {}, {}
    );
}

} // namespace HDRVDP3
} // namespace IQM