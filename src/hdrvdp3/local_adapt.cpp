/*

 * Image Quality Metrics - HDR-VDP-3 Local Adaptation

 * Deniz Aksoy - 2026

 */

#include <fstream>

#include <vkFFT.h>

#include <stdexcept>

#include <string>

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

// ============================================================================

// FFT-based Local Adaptation

// ============================================================================

struct LocalAdaptFFT::Impl {

void recordPrepare(const vk::raii::CommandBuffer& commandBuffer) {

    vk::CommandBuffer cmd = *commandBuffer;

    cmd.bindPipeline(

        vk::PipelineBindPoint::eCompute,

        *preparePipeline);

    cmd.bindDescriptorSets(

        vk::PipelineBindPoint::eCompute,

        *preparePipelineLayout,

        0,

        {*prepareDescriptorSets[0]},

        {});

    std::array<uint32_t, 2> pushConstants{

        fftWidth,

        fftHeight

    };

    cmd.pushConstants(

        *preparePipelineLayout,

        vk::ShaderStageFlagBits::eCompute,

        0,

        static_cast<uint32_t>(sizeof(pushConstants)),

        pushConstants.data());

    cmd.dispatch(

        (fftWidth + 15) / 16,

        (fftHeight + 15) / 16,

        1);

}

void recordFinish(const vk::raii::CommandBuffer& commandBuffer) {

    vk::CommandBuffer cmd = *commandBuffer;

    cmd.bindPipeline(

        vk::PipelineBindPoint::eCompute,

        *finishPipeline);

    cmd.bindDescriptorSets(

        vk::PipelineBindPoint::eCompute,

        *finishPipelineLayout,

        0,

        {*finishDescriptorSets[0]},

        {});

    std::array<uint32_t, 2> pushConstants{

        fftWidth,

        fftHeight

    };

    cmd.pushConstants(

        *finishPipelineLayout,

        vk::ShaderStageFlagBits::eCompute,

        0,

        static_cast<uint32_t>(sizeof(pushConstants)),

        pushConstants.data());

    // Output image is 64x64.

    constexpr uint32_t outputWidth = 64;

    constexpr uint32_t outputHeight = 64;

    cmd.dispatch(

        (outputWidth + 15) / 16,

        (outputHeight + 15) / 16,

        1);

}

    void createBuffer(

    vk::DeviceSize size,

    vk::BufferUsageFlags usage,

    vk::MemoryPropertyFlags properties,

    vk::raii::Buffer& buffer,

    vk::raii::DeviceMemory& memory

) {

    vk::BufferCreateInfo bufferInfo{

        .size = size,

        .usage = usage,

        .sharingMode = vk::SharingMode::eExclusive

    };

    vk::raii::Buffer newBuffer(device, bufferInfo);

    auto requirements = newBuffer.getMemoryRequirements();

    uint32_t memoryType = findMemoryType(

        physicalDevice,

        requirements.memoryTypeBits,

        properties

    );

    if (memoryType == UINT32_MAX) {

        throw std::runtime_error(

            "LocalAdaptFFT: suitable Vulkan memory type not found"

        );

    }

    vk::MemoryAllocateInfo allocInfo{

        .allocationSize = requirements.size,

        .memoryTypeIndex = memoryType

    };

    vk::raii::DeviceMemory newMemory(device, allocInfo);

    newBuffer.bindMemory(newMemory, 0);

    buffer = std::move(newBuffer);

    memory = std::move(newMemory);

}

    void initializeBuffers(uint32_t width, uint32_t height) {

    if (width == 0 || height == 0) {

        throw std::invalid_argument(

            "LocalAdaptFFT: FFT dimensions must be non-zero"

        );

    }

    const vk::DeviceSize pixelCount =

        static_cast<vk::DeviceSize>(width) * height;

    const vk::DeviceSize complexBufferSize =

        pixelCount * 2 * sizeof(float);

    const vk::DeviceSize realBufferSize =

        pixelCount * sizeof(float);

    createBuffer(

        complexBufferSize,

        vk::BufferUsageFlagBits::eStorageBuffer |

        vk::BufferUsageFlagBits::eTransferSrc |

        vk::BufferUsageFlagBits::eTransferDst,

        vk::MemoryPropertyFlagBits::eDeviceLocal,

        workBuffer,

        workMemory

    );

    createBuffer(

        realBufferSize,

        vk::BufferUsageFlagBits::eStorageBuffer |

        vk::BufferUsageFlagBits::eTransferDst,

        vk::MemoryPropertyFlagBits::eDeviceLocal,

        kernelBuffer,

        kernelMemory

    );

    createBuffer(

        complexBufferSize,

        vk::BufferUsageFlagBits::eStorageBuffer |

        vk::BufferUsageFlagBits::eTransferSrc |

        vk::BufferUsageFlagBits::eTransferDst,

        vk::MemoryPropertyFlagBits::eDeviceLocal,

        outputBuffer,

        outputMemory

    );

    fftWidth = width;

    fftHeight = height;

}

    const vk::raii::Device& device;

    const vk::raii::PhysicalDevice& physicalDevice;

    VkFFTApplication fftApp{};

    bool fftInitialized = false;

    // Stable Vulkan handles: VkFFT stores pointers to these values.

    VkDevice fftDeviceHandle = VK_NULL_HANDLE;

    VkPhysicalDevice fftPhysicalDeviceHandle = VK_NULL_HANDLE;

    VkQueue fftQueueHandle = VK_NULL_HANDLE;

    VkCommandPool fftCommandPoolHandle = VK_NULL_HANDLE;

    VkFence fftFenceHandle = VK_NULL_HANDLE;

    VkBuffer fftWorkBufferHandle = VK_NULL_HANDLE;

    uint64_t fftWorkBufferSize = 0;

    // FFT dimensions

    uint32_t fftWidth = 0;

    uint32_t fftHeight = 0;

    // Memory must be declared BEFORE its associated buffer.

    // C++ destroys members in reverse declaration order.

    // FFT working buffer (complex float2)

    vk::raii::DeviceMemory workMemory = nullptr;

    vk::raii::Buffer workBuffer = nullptr;

    // Gaussian frequency-domain filter (float)

    vk::raii::DeviceMemory kernelMemory = nullptr;

    vk::raii::Buffer kernelBuffer = nullptr;

    // Filtered complex spectrum (float2)

    vk::raii::DeviceMemory outputMemory = nullptr;

    vk::raii::Buffer outputBuffer = nullptr;

    // Host-visible staging buffer must outlive command-buffer execution.

    vk::raii::DeviceMemory kernelStagingMemory = nullptr;

    vk::raii::Buffer kernelStagingBuffer = nullptr;

    // Gaussian multiplication compute pipeline

    vk::raii::DescriptorPool descriptorPool = nullptr;

    vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;

    vk::raii::PipelineLayout pipelineLayout = nullptr;

    vk::raii::Pipeline computePipeline = nullptr;

    std::vector<vk::raii::DescriptorSet> descriptorSets;

    // FFT prepare: input image -> complex work buffer

    vk::raii::DescriptorSetLayout prepareDescriptorSetLayout = nullptr;

    vk::raii::PipelineLayout preparePipelineLayout = nullptr;

    vk::raii::Pipeline preparePipeline = nullptr;

    vk::raii::DescriptorPool prepareDescriptorPool = nullptr;

    std::vector<vk::raii::DescriptorSet> prepareDescriptorSets;

    // FFT finish: complex work buffer -> output image

    vk::raii::DescriptorSetLayout finishDescriptorSetLayout = nullptr;

    vk::raii::PipelineLayout finishPipelineLayout = nullptr;

    vk::raii::Pipeline finishPipeline = nullptr;

    vk::raii::DescriptorPool finishDescriptorPool = nullptr;

    std::vector<vk::raii::DescriptorSet> finishDescriptorSets;

    // Record a host-to-device upload of the Gaussian frequency response.

    // Call after initializeBuffers(), before the FFT multiplication dispatch.

    // The caller owns command-buffer submission and GPU synchronization.

    void recordGaussianKernelUpload(

        const vk::raii::CommandBuffer& commandBuffer,

        float ppd

    ) {

        if (!std::isfinite(ppd) || ppd <= 0.0f) {

            throw std::invalid_argument("LocalAdaptFFT: ppd must be positive and finite");

        }

        if (fftWidth == 0 || fftHeight == 0 || *kernelBuffer == VK_NULL_HANDLE) {

            throw std::logic_error("LocalAdaptFFT: initializeBuffers must be called first");

        }

        const float sigma = std::pow(10.0f, -0.781367f) * ppd;

        const auto kernel = generateGaussianKernel2D(sigma, fftWidth, fftHeight);

        const vk::DeviceSize bytes =

            static_cast<vk::DeviceSize>(kernel.size()) * sizeof(float);

        // Do not replace a staging buffer referenced by an in-flight command.

        // Reuse this allocation for uploads of the same dimensions.

        if (*kernelStagingBuffer == VK_NULL_HANDLE) {

            createBuffer(

                bytes,

                vk::BufferUsageFlagBits::eTransferSrc,

                vk::MemoryPropertyFlagBits::eHostVisible |

                    vk::MemoryPropertyFlagBits::eHostCoherent,

                kernelStagingBuffer,

                kernelStagingMemory

            );

        }

        void* mapped = kernelStagingMemory.mapMemory(0, bytes, {});

        std::memcpy(mapped, kernel.data(), static_cast<size_t>(bytes));

        kernelStagingMemory.unmapMemory();

        const vk::BufferCopy copyRegion{0, 0, bytes};

        commandBuffer.copyBuffer(*kernelStagingBuffer, *kernelBuffer, {copyRegion});

        const vk::BufferMemoryBarrier barrier{

            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,

            .dstAccessMask = vk::AccessFlagBits::eShaderRead,

            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

            .buffer = *kernelBuffer,

            .offset = 0,

            .size = bytes

        };

        commandBuffer.pipelineBarrier(

            vk::PipelineStageFlagBits::eTransfer,

            vk::PipelineStageFlagBits::eComputeShader,

            {}, {}, {barrier}, {}

        );

    }

    void initializeMultiplyPipeline() {

    // Three storage buffers: FFT input, Gaussian kernel, filtered output.

    std::array<vk::DescriptorSetLayoutBinding, 3> bindings{{

        {0, vk::DescriptorType::eStorageBuffer, 1,

         vk::ShaderStageFlagBits::eCompute},

        {1, vk::DescriptorType::eStorageBuffer, 1,

         vk::ShaderStageFlagBits::eCompute},

        {2, vk::DescriptorType::eStorageBuffer, 1,

         vk::ShaderStageFlagBits::eCompute}

    }};

    vk::DescriptorSetLayoutCreateInfo layoutInfo{

        .bindingCount = static_cast<uint32_t>(bindings.size()),

        .pBindings = bindings.data()

    };

    descriptorSetLayout =

        vk::raii::DescriptorSetLayout(device, layoutInfo);

    vk::PushConstantRange pushRange{

        .stageFlags = vk::ShaderStageFlagBits::eCompute,

        .offset = 0,

        .size = 2 * sizeof(uint32_t)

    };

    vk::DescriptorSetLayout rawLayout = *descriptorSetLayout;

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{

        .setLayoutCount = 1,

        .pSetLayouts = &rawLayout,

        .pushConstantRangeCount = 1,

        .pPushConstantRanges = &pushRange

    };

    pipelineLayout =

        vk::raii::PipelineLayout(device, pipelineLayoutInfo);

    // Descriptor pool: one set containing three storage buffers.

    vk::DescriptorPoolSize poolSize{

        .type = vk::DescriptorType::eStorageBuffer,

        .descriptorCount = 3

    };

    vk::DescriptorPoolCreateInfo poolInfo{

        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 1,

        .poolSizeCount = 1,

        .pPoolSizes = &poolSize

    };

    descriptorPool = vk::raii::DescriptorPool(device, poolInfo);

    // Allocate one descriptor set.

    vk::DescriptorSetAllocateInfo allocInfo{

        .descriptorPool = *descriptorPool,

        .descriptorSetCount = 1,

        .pSetLayouts = &rawLayout

    };

    auto allocatedSets = vk::raii::DescriptorSets(device, allocInfo);

    descriptorSets.clear();

    for (auto& set : allocatedSets) {

        descriptorSets.push_back(std::move(set));

    }

    // Buffer sizes for the 128x128 FFT.

    const vk::DeviceSize pixelCount =

        static_cast<vk::DeviceSize>(fftWidth) * fftHeight;

    const vk::DeviceSize complexBytes =

        pixelCount * 2 * sizeof(float);

    const vk::DeviceSize realBytes =

        pixelCount * sizeof(float);

    std::array<vk::DescriptorBufferInfo, 3> bufferInfos{{

        {*workBuffer, 0, complexBytes},

        {*kernelBuffer, 0, realBytes},

        {*outputBuffer, 0, complexBytes}

    }};

    std::array<vk::WriteDescriptorSet, 3> writes{};

    for (uint32_t i = 0; i < 3; ++i) {

        writes[i].dstSet = *descriptorSets[0];

        writes[i].dstBinding = i;

        writes[i].descriptorCount = 1;

        writes[i].descriptorType = vk::DescriptorType::eStorageBuffer;

        writes[i].pBufferInfo = &bufferInfos[i];

    }

    device.updateDescriptorSets(writes, {});

    // Load compiled Gaussian multiplication shader.

    const std::string shaderPath = "shaders/hdrvdp3/mtf_multiply.spv";

    std::ifstream shaderFile(shaderPath, std::ios::binary | std::ios::ate);

    if (!shaderFile) {

        throw std::runtime_error(

            "LocalAdaptFFT: cannot open shader: " + shaderPath);

    }

    const std::streamsize shaderSize = shaderFile.tellg();

    if (shaderSize <= 0 ||

        shaderSize % sizeof(uint32_t) != 0) {

        throw std::runtime_error(

            "LocalAdaptFFT: invalid SPIR-V shader size");

    }

    std::vector<uint32_t> shaderCode(

        static_cast<size_t>(shaderSize) / sizeof(uint32_t));

    shaderFile.seekg(0);

    shaderFile.read(

        reinterpret_cast<char*>(shaderCode.data()),

        shaderSize);

    if (!shaderFile) {

        throw std::runtime_error(

            "LocalAdaptFFT: failed to read shader");

    }

    vk::ShaderModuleCreateInfo shaderInfo{

        .codeSize = static_cast<size_t>(shaderSize),

        .pCode = shaderCode.data()

    };

    vk::raii::ShaderModule shaderModule(device, shaderInfo);

    vk::PipelineShaderStageCreateInfo stageInfo{

        .stage = vk::ShaderStageFlagBits::eCompute,

        .module = *shaderModule,

        .pName = "main"

    };

    vk::ComputePipelineCreateInfo pipelineInfo{

        .stage = stageInfo,

        .layout = *pipelineLayout

    };

    computePipeline = vk::raii::Pipeline(

        device,

        nullptr,

        pipelineInfo);

}

void initializeImagePipelines() {

    auto loadShader = [&](const std::string& path) {

        std::ifstream file(path, std::ios::binary | std::ios::ate);

        if (!file) {

            throw std::runtime_error(

                "LocalAdaptFFT: cannot open shader: " + path);

        }

        const std::streamsize size = file.tellg();

        if (size <= 0 || size % sizeof(uint32_t) != 0) {

            throw std::runtime_error(

                "LocalAdaptFFT: invalid SPIR-V size: " + path);

        }

        std::vector<uint32_t> code(

            static_cast<size_t>(size) / sizeof(uint32_t));

        file.seekg(0);

        file.read(reinterpret_cast<char*>(code.data()), size);

        if (!file) {

            throw std::runtime_error(

                "LocalAdaptFFT: failed to read shader: " + path);

        }

        vk::ShaderModuleCreateInfo info{

            .codeSize = static_cast<size_t>(size),

            .pCode = code.data()

        };

        return vk::raii::ShaderModule(device, info);

    };

    auto createImagePipeline = [&](

        const std::string& shaderPath,

        vk::raii::DescriptorSetLayout& setLayout,

        vk::raii::PipelineLayout& layout,

        vk::raii::Pipeline& pipeline,

        vk::raii::DescriptorPool& pool,

        std::vector<vk::raii::DescriptorSet>& sets

    ) {

        std::array<vk::DescriptorSetLayoutBinding, 2> bindings{{

            {0, vk::DescriptorType::eStorageImage, 1,

                vk::ShaderStageFlagBits::eCompute},

            {1, vk::DescriptorType::eStorageBuffer, 1,

                vk::ShaderStageFlagBits::eCompute}

        }};

        // Finish shader has the reverse descriptor types.

        if (shaderPath.find("finish") != std::string::npos) {

            bindings[0].descriptorType =

                vk::DescriptorType::eStorageBuffer;

            bindings[1].descriptorType =

                vk::DescriptorType::eStorageImage;

        }

        vk::DescriptorSetLayoutCreateInfo setInfo{

            .bindingCount = static_cast<uint32_t>(bindings.size()),

            .pBindings = bindings.data()

        };

        setLayout = vk::raii::DescriptorSetLayout(device, setInfo);

        vk::PushConstantRange pushRange{

            .stageFlags = vk::ShaderStageFlagBits::eCompute,

            .offset = 0,

            .size = 2 * sizeof(uint32_t)

        };

        vk::DescriptorSetLayout rawLayout = *setLayout;

        vk::PipelineLayoutCreateInfo layoutInfo{

            .setLayoutCount = 1,

            .pSetLayouts = &rawLayout,

            .pushConstantRangeCount = 1,

            .pPushConstantRanges = &pushRange

        };

        layout = vk::raii::PipelineLayout(device, layoutInfo);

        std::array<vk::DescriptorPoolSize, 2> poolSizes{{

            {vk::DescriptorType::eStorageImage, 1},

            {vk::DescriptorType::eStorageBuffer, 1}

        }};

        vk::DescriptorPoolCreateInfo poolInfo{

            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = 1,

            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),

            .pPoolSizes = poolSizes.data()

        };

        pool = vk::raii::DescriptorPool(device, poolInfo);

        vk::DescriptorSetAllocateInfo allocInfo{

            .descriptorPool = *pool,

            .descriptorSetCount = 1,

            .pSetLayouts = &rawLayout

        };

        auto allocatedSets = vk::raii::DescriptorSets(

            device, allocInfo);

        sets.clear();

        for (auto& set : allocatedSets) {

            sets.push_back(std::move(set));

        }

        auto shaderModule = loadShader(shaderPath);

        vk::PipelineShaderStageCreateInfo stageInfo{

            .stage = vk::ShaderStageFlagBits::eCompute,

            .module = *shaderModule,

            .pName = "main"

        };

        vk::ComputePipelineCreateInfo pipelineInfo{

            .stage = stageInfo,

            .layout = *layout

        };

        pipeline = vk::raii::Pipeline(

            device, nullptr, pipelineInfo);

    };

    createImagePipeline(

        "shaders/hdrvdp3/hdrvdp3_fft_prepare.spv",

        prepareDescriptorSetLayout,

        preparePipelineLayout,

        preparePipeline,

        prepareDescriptorPool,

        prepareDescriptorSets

    );

    createImagePipeline(

        "shaders/hdrvdp3/hdrvdp3_fft_finish.spv",

        finishDescriptorSetLayout,

        finishPipelineLayout,

        finishPipeline,

        finishDescriptorPool,

        finishDescriptorSets

    );

}

void updateImageDescriptors(const LocalAdaptFFTInput& input) {

    if (!input.ivInput || !input.ivOutput) {

        throw std::invalid_argument(

            "LocalAdaptFFT: input or output image view is null");

    }

    if (prepareDescriptorSets.empty() ||

        finishDescriptorSets.empty()) {

        throw std::logic_error(

            "LocalAdaptFFT: image descriptor sets not initialized");

    }

    const vk::DeviceSize bufferSize =

        static_cast<vk::DeviceSize>(fftWidth) *

        fftHeight * 2 * sizeof(float);

    vk::DescriptorImageInfo inputImageInfo{

        .imageView = **input.ivInput,

        .imageLayout = vk::ImageLayout::eGeneral

    };

    vk::DescriptorImageInfo outputImageInfo{

        .imageView = **input.ivOutput,

        .imageLayout = vk::ImageLayout::eGeneral

    };

    vk::DescriptorBufferInfo prepareBufferInfo{

        .buffer = *workBuffer,

        .offset = 0,

        .range = bufferSize

    };

    vk::DescriptorBufferInfo finishBufferInfo{

        .buffer = *workBuffer,

        .offset = 0,

        .range = bufferSize

    };

    std::array<vk::WriteDescriptorSet, 4> writes{};

    writes[0].dstSet = *prepareDescriptorSets[0];

    writes[0].dstBinding = 0;

    writes[0].descriptorCount = 1;

    writes[0].descriptorType = vk::DescriptorType::eStorageImage;

    writes[0].pImageInfo = &inputImageInfo;

    writes[1].dstSet = *prepareDescriptorSets[0];

    writes[1].dstBinding = 1;

    writes[1].descriptorCount = 1;

    writes[1].descriptorType = vk::DescriptorType::eStorageBuffer;

    writes[1].pBufferInfo = &prepareBufferInfo;

    writes[2].dstSet = *finishDescriptorSets[0];

    writes[2].dstBinding = 0;

    writes[2].descriptorCount = 1;

    writes[2].descriptorType = vk::DescriptorType::eStorageBuffer;

    writes[2].pBufferInfo = &finishBufferInfo;

    writes[3].dstSet = *finishDescriptorSets[0];

    writes[3].dstBinding = 1;

    writes[3].descriptorCount = 1;

    writes[3].descriptorType = vk::DescriptorType::eStorageImage;

    writes[3].pImageInfo = &outputImageInfo;

    device.updateDescriptorSets(writes, {});

}

    void recordGaussianMultiply(

    const vk::raii::CommandBuffer& commandBuffer

) {

    if (*computePipeline == VK_NULL_HANDLE ||

        descriptorSets.empty()) {

        throw std::logic_error(

            "LocalAdaptFFT: multiply pipeline not initialized");

    }

    vk::CommandBuffer cmd = *commandBuffer;

    cmd.bindPipeline(

        vk::PipelineBindPoint::eCompute,

        *computePipeline);

    cmd.bindDescriptorSets(

        vk::PipelineBindPoint::eCompute,

        *pipelineLayout,

        0,

        {*descriptorSets[0]},

        {});

    std::array<uint32_t, 2> pushConstants{

        fftWidth,

        fftHeight

    };

    cmd.pushConstants(

        *pipelineLayout,

        vk::ShaderStageFlagBits::eCompute,

        0,

        static_cast<uint32_t>(sizeof(pushConstants)),

        pushConstants.data());

    cmd.dispatch(

        (fftWidth + 15) / 16,

        (fftHeight + 15) / 16,

        1);

}

    // Initialize the VkFFT plan after allocating the working buffers.

    // Do not reallocate workBuffer or destroy the Vulkan context while this

    // plan is alive. The caller must synchronize any in-flight GPU work.

    void initializeFFT(

        const vk::raii::Queue& queue,

        const vk::raii::CommandPool& commandPool

    ) {

        if (fftInitialized) {

            throw std::logic_error("LocalAdaptFFT: VkFFT already initialized");

        }

        if (fftWidth == 0 || fftHeight == 0 ||

            *workBuffer == VK_NULL_HANDLE) {

            throw std::logic_error("LocalAdaptFFT: initializeBuffers must run first");

        }

        fftDeviceHandle = static_cast<VkDevice>(*device);

        fftPhysicalDeviceHandle = static_cast<VkPhysicalDevice>(*physicalDevice);

        fftQueueHandle = static_cast<VkQueue>(*queue);

        fftCommandPoolHandle = static_cast<VkCommandPool>(*commandPool);

        fftFenceHandle = VK_NULL_HANDLE;

        fftWorkBufferHandle = static_cast<VkBuffer>(*workBuffer);

        fftWorkBufferSize = static_cast<uint64_t>(fftWidth) * fftHeight *

                            2u * sizeof(float);

        VkFFTConfiguration config{};

        config.FFTdim = 2;

        config.size[0] = fftWidth;

        config.size[1] = fftHeight;

        config.numberBatches = 1;

        config.bufferNum = 1;

        config.buffer = &fftWorkBufferHandle;

        config.bufferSize = &fftWorkBufferSize;

        config.physicalDevice = &fftPhysicalDeviceHandle;

        config.device = &fftDeviceHandle;

        config.queue = &fftQueueHandle;

        config.commandPool = &fftCommandPoolHandle;

        config.fence = &fftFenceHandle;

        config.isCompilerInitialized = 0;

        config.normalize = 1;

        config.performR2C = 0;  // Complex-to-complex, float2 per pixel.

        config.doublePrecision = 0;

        config.reorderFourStep = 1;

        config.useLUT = -1;

        config.aimThreads = 128;

        config.makeForwardPlanOnly = 0;

        config.makeInversePlanOnly = 0;

        const VkFFTResult result = initializeVkFFT(&fftApp, config);

        if (result != VKFFT_SUCCESS) {

            // VkFFT may have partially allocated resources on failure.

            // Do not mark the plan initialized; investigate the returned code.

            throw std::runtime_error(

                "LocalAdaptFFT: initializeVkFFT failed with code " +

                std::to_string(static_cast<int>(result)));

        }

        fftInitialized = true;

    }

    explicit Impl(

        const vk::raii::Device& dev,

        const vk::raii::PhysicalDevice& physDev

    )

        : device(dev),

          physicalDevice(physDev)

    {

    }

    ~Impl() {

        // The caller must ensure GPU work has completed before destruction.

        if (fftInitialized) {

            deleteVkFFT(&fftApp);

            fftInitialized = false;

        }

        // Release image-pipeline descriptor sets before their pools.

        prepareDescriptorSets.clear();

        finishDescriptorSets.clear();

        preparePipeline = nullptr;

        finishPipeline = nullptr;

        preparePipelineLayout = nullptr;

        finishPipelineLayout = nullptr;

        prepareDescriptorSetLayout = nullptr;

        finishDescriptorSetLayout = nullptr;

        prepareDescriptorPool = nullptr;

        finishDescriptorPool = nullptr;

        // Descriptor sets must be released before their pool.

        descriptorSets.clear();

        computePipeline = nullptr;

        pipelineLayout = nullptr;

        descriptorSetLayout = nullptr;

        descriptorPool = nullptr;

        kernelStagingBuffer = nullptr;

        kernelStagingMemory = nullptr;

        // Destroy buffers before freeing the memory bound to them.

        outputBuffer = nullptr;

        outputMemory = nullptr;

        kernelBuffer = nullptr;

        kernelMemory = nullptr;

        workBuffer = nullptr;

        workMemory = nullptr;

    }

};

LocalAdaptFFT::LocalAdaptFFT(

    const vk::raii::Device& device,

    const vk::raii::PhysicalDevice& physicalDevice

)

    : impl(std::make_unique<Impl>(device, physicalDevice))

{

}

LocalAdaptFFT::~LocalAdaptFFT() = default;

// Record the complete FFT-based local adaptation pass into the caller's

// already-recording command buffer. The caller owns submission and synchronization.

void LocalAdaptFFT::compute(const LocalAdaptFFTInput& input) {

    if (!input.cmdBuf || !input.cmdPool || !input.queue ||

        !input.ivInput || !input.ivOutput) {

        throw std::invalid_argument("LocalAdaptFFT: missing Vulkan input resource");

    }

    if (!std::isfinite(input.ppd) || input.ppd <= 0.0f) {

        throw std::invalid_argument("LocalAdaptFFT: ppd must be positive and finite");

    }

    // The first integration test uses a 64x64 R32_SFLOAT input/output

    // and a 128x128 complex FFT. Both images must already be in GENERAL.

    if (!impl->fftInitialized) {

        impl->initializeBuffers(128, 128);

        impl->initializeMultiplyPipeline();

        impl->initializeImagePipelines();

        impl->initializeFFT(*input.queue, *input.cmdPool);

    }

    // Descriptor updates and staging-buffer writes require the previous use

    // of this LocalAdaptFFT instance to have finished on the GPU.

    impl->updateImageDescriptors(input);

    const vk::raii::CommandBuffer& commandBuffer = *input.cmdBuf;

    vk::CommandBuffer cmd = *input.cmdBuf;

    const vk::DeviceSize complexBufferSize =

        static_cast<vk::DeviceSize>(impl->fftWidth) * impl->fftHeight *

        2 * sizeof(float);

    auto bufferBarrier = [&](vk::Buffer buffer,

                             vk::AccessFlags srcAccess,

                             vk::AccessFlags dstAccess,

                             vk::PipelineStageFlags srcStage,

                             vk::PipelineStageFlags dstStage) {

        vk::BufferMemoryBarrier barrier{

            .srcAccessMask = srcAccess,

            .dstAccessMask = dstAccess,

            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

            .buffer = buffer,

            .offset = 0,

            .size = complexBufferSize

        };

        cmd.pipelineBarrier(srcStage, dstStage, {}, {}, {barrier}, {});

    };

    // Kernel upload includes its own transfer-write -> compute-read barrier.

    impl->recordGaussianKernelUpload(commandBuffer, input.ppd);

    // Image -> padded log-domain complex work buffer.

    impl->recordPrepare(commandBuffer);

    bufferBarrier(*impl->workBuffer,

                  vk::AccessFlagBits::eShaderWrite,

                  vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,

                  vk::PipelineStageFlagBits::eComputeShader,

                  vk::PipelineStageFlagBits::eComputeShader);

    VkCommandBuffer rawCmd = static_cast<VkCommandBuffer>(*commandBuffer);

    VkBuffer rawWorkBuffer = static_cast<VkBuffer>(*impl->workBuffer);

    VkFFTLaunchParams launchParams{};

    launchParams.commandBuffer = &rawCmd;

    launchParams.buffer = &rawWorkBuffer;

    VkFFTResult result = VkFFTAppend(&impl->fftApp, -1, &launchParams);

    if (result != VKFFT_SUCCESS) {

        throw std::runtime_error("LocalAdaptFFT: forward FFT failed: " +

                                 std::to_string(static_cast<int>(result)));

    }

    bufferBarrier(*impl->workBuffer,

                  vk::AccessFlagBits::eShaderWrite,

                  vk::AccessFlagBits::eShaderRead,

                  vk::PipelineStageFlagBits::eComputeShader,

                  vk::PipelineStageFlagBits::eComputeShader);

    // Multiply FFT coefficients by the frequency-domain Gaussian.

    impl->recordGaussianMultiply(commandBuffer);

    bufferBarrier(*impl->outputBuffer,

                  vk::AccessFlagBits::eShaderWrite,

                  vk::AccessFlagBits::eTransferRead,

                  vk::PipelineStageFlagBits::eComputeShader,

                  vk::PipelineStageFlagBits::eTransfer);

    // The VkFFT inverse plan operates in-place on workBuffer.

    vk::BufferCopy copyRegion{

        .srcOffset = 0,

        .dstOffset = 0,

        .size = complexBufferSize

    };

    cmd.copyBuffer(*impl->outputBuffer, *impl->workBuffer, {copyRegion});

    bufferBarrier(*impl->workBuffer,

                  vk::AccessFlagBits::eTransferWrite,

                  vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,

                  vk::PipelineStageFlagBits::eTransfer,

                  vk::PipelineStageFlagBits::eComputeShader);

    result = VkFFTAppend(&impl->fftApp, 1, &launchParams);

    if (result != VKFFT_SUCCESS) {

        throw std::runtime_error("LocalAdaptFFT: inverse FFT failed: " +

                                 std::to_string(static_cast<int>(result)));

    }

    bufferBarrier(*impl->workBuffer,

                  vk::AccessFlagBits::eShaderWrite,

                  vk::AccessFlagBits::eShaderRead,

                  vk::PipelineStageFlagBits::eComputeShader,

                  vk::PipelineStageFlagBits::eComputeShader);

    // Top-left 64x64 crop and exp(real(IFFT)).

    impl->recordFinish(commandBuffer);

    // Make output image writes visible to subsequent compute or transfer reads.

    vk::MemoryBarrier outputBarrier{

        .srcAccessMask = vk::AccessFlagBits::eShaderWrite,

        .dstAccessMask = vk::AccessFlagBits::eShaderRead |

                         vk::AccessFlagBits::eTransferRead

    };

    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,

                        vk::PipelineStageFlagBits::eComputeShader |

                            vk::PipelineStageFlagBits::eTransfer,

                        {}, {outputBarrier}, {}, {});

}

} // namespace HDRVDP3

} // namespace IQM