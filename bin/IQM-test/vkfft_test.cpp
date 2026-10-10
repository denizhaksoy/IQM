/*
 * Image Quality Metrics - VkFFT GPU FFT Round-Trip Test
 * Tests actual GPU FFT2 followed by GPU IFFT2 using VkFFT
 */

#include <IQM/base/vulkan_runtime.h>
#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vkFFT.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <iomanip>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

using namespace IQM::GPU;
using namespace IQM::Bin;

static constexpr unsigned WIDTH = 128;
static constexpr unsigned HEIGHT = 128;
static constexpr unsigned INNER_WIDTH = 64;
static constexpr unsigned INNER_HEIGHT = 64;
static constexpr unsigned NUM_CHANNELS = 2;
static constexpr size_t BUFFER_SIZE = WIDTH * HEIGHT * NUM_CHANNELS * sizeof(float);

inline VkBuffer toVkBuffer(const vk::raii::Buffer& raiiBuffer) {
    return static_cast<VkBuffer>(*raiiBuffer);
}

inline VkCommandBuffer toVkCommandBuffer(const vk::raii::CommandBuffer& raiiCmdBuf) {
    return static_cast<VkCommandBuffer>(*raiiCmdBuf);
}

inline VkQueue toVkQueue(const vk::raii::Queue& raiiQueue) {
    return static_cast<VkQueue>(*raiiQueue);
}

inline VkCommandPool toVkCommandPool(const vk::raii::CommandPool& raiiCmdPool) {
    return static_cast<VkCommandPool>(*raiiCmdPool);
}

inline VkFence toVkFence(const vk::raii::Fence& raiiFence) {
    return static_cast<VkFence>(*raiiFence);
}

inline VkPhysicalDevice toVkPhysicalDevice(const vk::raii::PhysicalDevice& raiiPhysDev) {
    return static_cast<VkPhysicalDevice>(*raiiPhysDev);
}

inline VkDevice toVkDevice(const vk::raii::Device& raiiDevice) {
    return static_cast<VkDevice>(*raiiDevice);
}

inline VkDebugUtilsMessengerEXT toVkDebugMessenger(const vk::raii::DebugUtilsMessengerEXT& raiiMessenger) {
    return static_cast<VkDebugUtilsMessengerEXT>(*raiiMessenger);
}

void printValidationStatus(const VulkanInstance& instance) {
    std::cout << "=== Vulkan Validation Layer Status ===" << std::endl;
    bool hasDebugMessenger = toVkDebugMessenger(instance.debugMessenger) != VK_NULL_HANDLE;
    if (hasDebugMessenger) {
        std::cout << "Validation layer: ENABLED (VK_LAYER_KHRONOS_validation)" << std::endl;
        std::cout << "Debug messenger: ACTIVE" << std::endl;
    } else {
        std::cout << "Validation layer: DISABLED" << std::endl;
    }
    std::cout << "Selected device: " << instance.selectedDevice << std::endl;
    std::cout << std::endl;
}

std::vector<float> generateDeterministicPattern(unsigned width, unsigned height) {
    std::vector<float> pattern(width * height);
    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            float fx = static_cast<float>(x) / static_cast<float>(width);
            float fy = static_cast<float>(y) / static_cast<float>(height);
            pattern[y * width + x] = std::sin(fx * 8.0f * M_PI) * std::cos(fy * 6.0f * M_PI) * 0.5f + 0.5f;
        }
    }
    return pattern;
}

std::vector<float> createPaddedInput(const std::vector<float>& inner, unsigned innerW, unsigned innerH, unsigned outerW, unsigned outerH) {
    std::vector<float> padded(outerW * outerH * NUM_CHANNELS, 0.0f);
    for (unsigned y = 0; y < innerH; ++y) {
        for (unsigned x = 0; x < innerW; ++x) {
            float val = inner[y * innerW + x];
            padded[(y * outerW + x) * NUM_CHANNELS + 0] = val;
            padded[(y * outerW + x) * NUM_CHANNELS + 1] = 0.0f;
        }
    }
    return padded;
}

std::pair<float, float> computeErrors(const std::vector<float>& reference, const std::vector<float>& reconstructed, 
                                       unsigned width, unsigned height, unsigned channels,
                                       unsigned cropW = 0, unsigned cropH = 0) {
    float maxAbsErr = 0.0f;
    float sumSqErr = 0.0f;
    size_t count = 0;
    
    unsigned compareW = (cropW > 0) ? cropW : width;
    unsigned compareH = (cropH > 0) ? cropH : height;
    
    for (unsigned y = 0; y < compareH; ++y) {
        for (unsigned x = 0; x < compareW; ++x) {
            for (unsigned c = 0; c < channels; ++c) {
                size_t idx = (y * width + x) * channels + c;
                float ref = reference[idx];
                float rec = reconstructed[idx];
                float err = std::abs(ref - rec);
                maxAbsErr = std::max(maxAbsErr, err);
                sumSqErr += err * err;
                count++;
            }
        }
    }
    
    float rmse = (count > 0) ? std::sqrt(sumSqErr / count) : 0.0f;
    return {maxAbsErr, rmse};
}

void printSampleValues(const std::vector<float>& data, unsigned width, unsigned height, unsigned channels, 
                       const char* label, unsigned numSamples = 10) {
    std::cout << label << " (first " << numSamples << " samples):" << std::endl;
    for (unsigned i = 0; i < std::min(numSamples, width * height); ++i) {
        unsigned y = i / width;
        unsigned x = i % width;
        std::cout << "  [" << y << "," << x << "] = (" 
                  << data[(y * width + x) * channels + 0] << ", " 
                  << data[(y * width + x) * channels + 1] << ")" << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "=========================================" << std::endl;
    std::cout << "VkFFT GPU FFT2 + IFFT2 Round-Trip Test" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "VkFFT version: " << VkFFTGetVersion() << std::endl;
    std::cout << "Test size: " << WIDTH << "x" << HEIGHT << " (padded from " << INNER_WIDTH << "x" << INNER_HEIGHT << ")" << std::endl;
    std::cout << std::endl;

    VulkanInstance instance(true);
    printValidationStatus(instance);

    const vk::raii::Device& device = *instance.device();
    const vk::raii::PhysicalDevice& physicalDevice = *instance.physicalDevice();
    const vk::raii::Queue& queue = *instance.queue();
    const vk::raii::CommandPool& commandPool = *instance.cmdPool();
    const vk::raii::CommandBuffer& cmdBuf = *instance.cmdBuf();

    vk::raii::Fence fence(device, vk::FenceCreateInfo{});

    // Raw handles for VkFFT integration
    VkDevice vkDev = toVkDevice(device);
    VkPhysicalDevice vkPhysDev = toVkPhysicalDevice(physicalDevice);
    VkQueue vkQueue = toVkQueue(queue);
    VkCommandPool vkCmdPool = toVkCommandPool(commandPool);
    VkFence vkFence = toVkFence(fence);
    VkCommandBuffer vkCmdBuf = toVkCommandBuffer(cmdBuf);

    std::cout << "=== Generating Test Data ===" << std::endl;
    auto innerPattern = generateDeterministicPattern(INNER_WIDTH, INNER_HEIGHT);
    auto paddedInput = createPaddedInput(innerPattern, INNER_WIDTH, INNER_HEIGHT, WIDTH, HEIGHT);
    std::cout << "Generated " << INNER_WIDTH << "x" << INNER_HEIGHT << " deterministic pattern, padded to " << WIDTH << "x" << HEIGHT << std::endl;
    printSampleValues(paddedInput, WIDTH, HEIGHT, NUM_CHANNELS, "Input (padded)");

    std::cout << "=== Creating GPU Buffers ===" << std::endl;
    // Input buffer - holds original data (for comparison later)
    auto [inputBuffer, inputMemory] = VulkanResource::createBuffer(
        device, physicalDevice, BUFFER_SIZE,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    inputBuffer.bindMemory(inputMemory, 0);

    // Work buffer - used by VkFFT for in-place FFT transforms
    auto [workBuffer, workMemory] = VulkanResource::createBuffer(
        device, physicalDevice, BUFFER_SIZE,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    workBuffer.bindMemory(workMemory, 0);

    // Output buffer - holds final inverse FFT result
    auto [outputBuffer, outputMemory] = VulkanResource::createBuffer(
        device, physicalDevice, BUFFER_SIZE,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    outputBuffer.bindMemory(outputMemory, 0);

    // Staging buffer - for host-device transfers (needs both SRC and DST)
    auto [stagingBuffer, stagingMemory] = VulkanResource::createBuffer(
        device, physicalDevice, BUFFER_SIZE,
        vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    stagingBuffer.bindMemory(stagingMemory, 0);

    // Upload input data to input buffer
    void* stagedData = stagingMemory.mapMemory(0, BUFFER_SIZE);
    std::memcpy(stagedData, paddedInput.data(), BUFFER_SIZE);
    stagingMemory.unmapMemory();

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    cmdBuf.begin(beginInfo);
    vk::BufferCopy copyRegion{0, 0, BUFFER_SIZE};
    cmdBuf.copyBuffer(stagingBuffer, inputBuffer, copyRegion);
    
    vk::BufferMemoryBarrier barrier{
        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = inputBuffer,
        .offset = 0,
        .size = BUFFER_SIZE
    };
    cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, nullptr, {barrier}, nullptr);
    cmdBuf.end();

    VkSubmitInfo submitInfo1{};
    submitInfo1.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo1.commandBufferCount = 1;
    submitInfo1.pCommandBuffers = &vkCmdBuf;
    vkQueueSubmit(vkQueue, 1, &submitInfo1, vkFence);
    vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkDev, 1, &vkFence);
    std::cout << "Input data uploaded to GPU" << std::endl;

    // Copy input to work buffer for VkFFT processing
    cmdBuf.begin(beginInfo);
    cmdBuf.copyBuffer(inputBuffer, workBuffer, copyRegion);
    
    vk::BufferMemoryBarrier workBarrier{
        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = workBuffer,
        .offset = 0,
        .size = BUFFER_SIZE
    };
    cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, nullptr, {workBarrier}, nullptr);
    cmdBuf.end();

    VkSubmitInfo submitInfoCopy{};
    submitInfoCopy.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfoCopy.commandBufferCount = 1;
    submitInfoCopy.pCommandBuffers = &vkCmdBuf;
    vkQueueSubmit(vkQueue, 1, &submitInfoCopy, vkFence);
    vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkDev, 1, &vkFence);
    std::cout << "Input copied to work buffer" << std::endl;

    std::cout << "=== Initializing VkFFT ===" << std::endl;
    VkFFTConfiguration config = {};
    config.FFTdim = 2;
    config.size[0] = WIDTH;
    config.size[1] = HEIGHT;
    config.numberBatches = 1;
    config.bufferNum = 1;
    
    VkBuffer vkWorkBuffer = toVkBuffer(workBuffer);
    uint64_t bufferSize[1] = {BUFFER_SIZE};
    config.buffer = &vkWorkBuffer;
    config.bufferSize = bufferSize;
    
    config.physicalDevice = &vkPhysDev;
    config.device = &vkDev;
    config.queue = &vkQueue;
    config.commandPool = &vkCmdPool;
    config.fence = &vkFence;
    config.isCompilerInitialized = 0;
    config.normalize = 1;
    config.performR2C = 0;
    config.doublePrecision = 0;
    config.reorderFourStep = 1;
    config.useLUT = -1;
    config.aimThreads = 128;
    config.makeForwardPlanOnly = 0;
    config.makeInversePlanOnly = 0;

    VkFFTApplication app = {};
    VkFFTResult resFFT = initializeVkFFT(&app, config);
    if (resFFT != VKFFT_SUCCESS) {
        std::cerr << "Failed to initialize VkFFT: " << getVkFFTErrorString(resFFT) << std::endl;
        return 1;
    }
    std::cout << "VkFFT initialized successfully" << std::endl;

    VkFFTLaunchParams launchParams = {};
    launchParams.buffer = &vkWorkBuffer;
    launchParams.commandBuffer = &vkCmdBuf;

    std::cout << "=== Running Forward FFT2 ===" << std::endl;
    cmdBuf.begin(beginInfo);
    resFFT = VkFFTAppend(&app, -1, &launchParams);
    if (resFFT != VKFFT_SUCCESS) {
        std::cerr << "Forward FFT failed: " << getVkFFTErrorString(resFFT) << std::endl;
        deleteVkFFT(&app);
        return 1;
    }
    cmdBuf.end();
    
    VkSubmitInfo submitInfo2{};
    submitInfo2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo2.commandBufferCount = 1;
    submitInfo2.pCommandBuffers = &vkCmdBuf;
    vkQueueSubmit(vkQueue, 1, &submitInfo2, vkFence);
    vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkDev, 1, &vkFence);
    std::cout << "Forward FFT2 completed" << std::endl;

    std::cout << "=== Running Inverse FFT2 ===" << std::endl;
    // Inverse FFT uses the same work buffer (in-place)
    cmdBuf.begin(beginInfo);
    resFFT = VkFFTAppend(&app, 1, &launchParams);
    if (resFFT != VKFFT_SUCCESS) {
        std::cerr << "Inverse FFT failed: " << getVkFFTErrorString(resFFT) << std::endl;
        deleteVkFFT(&app);
        return 1;
    }
    cmdBuf.end();
    
    VkSubmitInfo submitInfo3{};
    submitInfo3.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo3.commandBufferCount = 1;
    submitInfo3.pCommandBuffers = &vkCmdBuf;
    vkQueueSubmit(vkQueue, 1, &submitInfo3, vkFence);
    vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkDev, 1, &vkFence);
    std::cout << "Inverse FFT2 completed" << std::endl;

    // Copy work buffer to output buffer for download
    cmdBuf.begin(beginInfo);
    cmdBuf.copyBuffer(workBuffer, outputBuffer, copyRegion);
    cmdBuf.end();
    
    VkSubmitInfo submitInfoCopy2{};
    submitInfoCopy2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfoCopy2.commandBufferCount = 1;
    submitInfoCopy2.pCommandBuffers = &vkCmdBuf;
    vkQueueSubmit(vkQueue, 1, &submitInfoCopy2, vkFence);
    vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkDev, 1, &vkFence);
    std::cout << "Work buffer copied to output buffer" << std::endl;

    std::cout << "=== Downloading Results ===" << std::endl;
    cmdBuf.begin(beginInfo);
    cmdBuf.copyBuffer(outputBuffer, stagingBuffer, copyRegion);
    
    vk::BufferMemoryBarrier downloadBarrier{
        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
        .dstAccessMask = vk::AccessFlagBits::eHostRead,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = stagingBuffer,
        .offset = 0,
        .size = BUFFER_SIZE
    };
    cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eHost, {}, nullptr, {downloadBarrier}, nullptr);
    cmdBuf.end();
    
    VkSubmitInfo submitInfo4{};
    submitInfo4.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo4.commandBufferCount = 1;
    submitInfo4.pCommandBuffers = &vkCmdBuf;
    vkQueueSubmit(vkQueue, 1, &submitInfo4, vkFence);
    vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkDev, 1, &vkFence);
    
    std::vector<float> reconstructed(BUFFER_SIZE / sizeof(float));
    stagedData = stagingMemory.mapMemory(0, BUFFER_SIZE);
    std::memcpy(reconstructed.data(), stagedData, BUFFER_SIZE);
    stagingMemory.unmapMemory();
    std::cout << "Results downloaded from GPU" << std::endl;

    printSampleValues(reconstructed, WIDTH, HEIGHT, NUM_CHANNELS, "Output (reconstructed)");

    std::cout << "=== Error Analysis ===" << std::endl;
    auto [maxAbsErrFull, rmseFull] = computeErrors(paddedInput, reconstructed, WIDTH, HEIGHT, NUM_CHANNELS);
    auto [maxAbsErrCrop, rmseCrop] = computeErrors(paddedInput, reconstructed, WIDTH, HEIGHT, NUM_CHANNELS, INNER_WIDTH, INNER_HEIGHT);
    
    std::cout << std::fixed << std::setprecision(8);
    std::cout << "Full 128x128 comparison:" << std::endl;
    std::cout << "  MaxAbsErr: " << maxAbsErrFull << std::endl;
    std::cout << "  RMSE:      " << rmseFull << std::endl;
    std::cout << std::endl;
    std::cout << "Cropped 64x64 (top-left) comparison:" << std::endl;
    std::cout << "  MaxAbsErr: " << maxAbsErrCrop << std::endl;
    std::cout << "  RMSE:      " << rmseCrop << std::endl;
    std::cout << std::endl;

    std::cout << "=== Representative Pixel Values ===" << std::endl;
    std::cout << "  Input[0,0]:      (" << paddedInput[0] << ", " << paddedInput[1] << ")" << std::endl;
    std::cout << "  Output[0,0]:     (" << reconstructed[0] << ", " << reconstructed[1] << ")" << std::endl;
    std::cout << "  Input[32,32]:    (" << paddedInput[(32*WIDTH+32)*2] << ", " << paddedInput[(32*WIDTH+32)*2+1] << ")" << std::endl;
    std::cout << "  Output[32,32]:   (" << reconstructed[(32*WIDTH+32)*2] << ", " << reconstructed[(32*WIDTH+32)*2+1] << ")" << std::endl;
    std::cout << "  Input[63,63]:    (" << paddedInput[(63*WIDTH+63)*2] << ", " << paddedInput[(63*WIDTH+63)*2+1] << ")" << std::endl;
    std::cout << "  Output[63,63]:   (" << reconstructed[(63*WIDTH+63)*2] << ", " << reconstructed[(63*WIDTH+63)*2+1] << ")" << std::endl;
    std::cout << "  Input[64,0] (pad): (" << paddedInput[(64*WIDTH+0)*2] << ", " << paddedInput[(64*WIDTH+0)*2+1] << ")" << std::endl;
    std::cout << "  Output[64,0]:    (" << reconstructed[(64*WIDTH+0)*2] << ", " << reconstructed[(64*WIDTH+0)*2+1] << ")" << std::endl;
    std::cout << std::endl;

    bool success = (maxAbsErrFull < 1e-4f) && (rmseFull < 1e-5f);
    std::cout << "=== Test Result ===" << std::endl;
    if (success) {
        std::cout << "PASSED: Reconstruction accuracy within tolerance" << std::endl;
    } else {
        std::cout << "FAILED: Reconstruction error exceeds tolerance" << std::endl;
        std::cout << "  Expected MaxAbsErr < 1e-4, got " << maxAbsErrFull << std::endl;
        std::cout << "  Expected RMSE < 1e-5, got " << rmseFull << std::endl;
    }

    deleteVkFFT(&app);
    std::cout << "=== Cleanup Complete ===" << std::endl;
    
    return success ? 0 : 1;
}