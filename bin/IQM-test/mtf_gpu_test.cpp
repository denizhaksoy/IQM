/*
 * Image Quality Metrics - HDR-VDP-3 Optical MTF GPU Test
 * Tests GPU FFT2 -> MTF multiply -> IFFT2 against official Octave reference
 */

#include <IQM/base/vulkan_runtime.h>
#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <IQM/hdrvdp3/mtf.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vkFFT.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <fstream>

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
static constexpr size_t COMPLEX_BUFFER_SIZE = WIDTH * HEIGHT * NUM_CHANNELS * sizeof(float);
static constexpr size_t REAL_BUFFER_SIZE = WIDTH * HEIGHT * sizeof(float);

// Helper to convert vk::raii types to raw Vulkan handles
inline VkBuffer toVkBuffer(const vk::raii::Buffer& raiiBuffer) { return static_cast<VkBuffer>(*raiiBuffer); }
inline VkCommandBuffer toVkCommandBuffer(const vk::raii::CommandBuffer& raiiCmdBuf) { return static_cast<VkCommandBuffer>(*raiiCmdBuf); }
inline VkQueue toVkQueue(const vk::raii::Queue& raiiQueue) { return static_cast<VkQueue>(*raiiQueue); }
inline VkCommandPool toVkCommandPool(const vk::raii::CommandPool& raiiCmdPool) { return static_cast<VkCommandPool>(*raiiCmdPool); }
inline VkFence toVkFence(const vk::raii::Fence& raiiFence) { return static_cast<VkFence>(*raiiFence); }
inline VkPhysicalDevice toVkPhysicalDevice(const vk::raii::PhysicalDevice& raiiPhysDev) { return static_cast<VkPhysicalDevice>(*raiiPhysDev); }
inline VkDevice toVkDevice(const vk::raii::Device& raiiDevice) { return static_cast<VkDevice>(*raiiDevice); }
inline VkDebugUtilsMessengerEXT toVkDebugMessenger(const vk::raii::DebugUtilsMessengerEXT& raiiMessenger) { return static_cast<VkDebugUtilsMessengerEXT>(*raiiMessenger); }

void printValidationStatus(const VulkanInstance& instance) {
    std::cout << "=== Vulkan Validation Layer Status ===" << std::endl;
    bool hasDebugMessenger = toVkDebugMessenger(instance.debugMessenger) != VK_NULL_HANDLE;
    if (hasDebugMessenger) {
        std::cout << "Validation layer: ENABLED (VK_LAYER_KHRONOS_validation)" << std::endl;
        std::cout << "Debug messenger: ACTIVE" << std::endl;
    } else {
        std::cout << "Validation layer: DISABLED" << std::endl;
    }
    std::cout << "Selected device: " << instance.selectedDevice << std::endl << std::endl;
}

std::pair<float, float> computeErrors(const std::vector<float>& reference, const std::vector<float>& reconstructed,
                                       unsigned width, unsigned height) {
    float maxAbsErr = 0.0f;
    float sumSqErr = 0.0f;
    size_t count = 0;
    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            size_t idx = y * width + x;
            float ref = reference[idx];
            float rec = reconstructed[idx];
            float err = std::abs(ref - rec);
            maxAbsErr = std::max(maxAbsErr, err);
            sumSqErr += err * err;
            count++;
        }
    }
    float rmse = (count > 0) ? std::sqrt(sumSqErr / count) : 0.0f;
    return {maxAbsErr, rmse};
}

void printSampleValues(const std::vector<float>& data, unsigned width, unsigned height, const char* label, unsigned numSamples = 10) {
    std::cout << label << " (first " << numSamples << " samples):" << std::endl;
    for (unsigned i = 0; i < std::min(numSamples, width * height); ++i) {
        unsigned y = i / width;
        unsigned x = i % width;
        std::cout << "  [" << y << "," << x << "] = " << data[y * width + x] << std::endl;
    }
    std::cout << std::endl;
}

std::vector<float> loadReference(const std::string& filename, unsigned expectedWidth, unsigned expectedHeight) {
    std::ifstream file(filename);
    std::vector<float> data(expectedWidth * expectedHeight);
    for (size_t i = 0; i < data.size() && file.good(); ++i) {
        file >> data[i];
    }
    file.close();
    return data;
}

int main() {
    std::cout << "=========================================" << std::endl;
    std::cout << "HDR-VDP-3 Optical MTF GPU Test" << std::endl;
    std::cout << "GPU FFT2 -> MTF multiply -> IFFT2" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "VkFFT version: " << VkFFTGetVersion() << std::endl;
    std::cout << "Test: 64x64 input -> pad 128x128 (replicate) -> MTF -> crop 64x64" << std::endl;
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

    // =========================================================================
    // STEP 1: Generate deterministic 64x64 linear luminance input
    // Exact match to GPU test: data[y * width + x] = (x * height + y + 1) / N * 100
    // =========================================================================
    std::cout << "=== Generating Deterministic 64x64 Input ===" << std::endl;
    std::vector<float> input64(INNER_WIDTH * INNER_HEIGHT);
    const float N = static_cast<float>(INNER_WIDTH * INNER_HEIGHT);
    for (unsigned y = 0; y < INNER_HEIGHT; ++y) {
        for (unsigned x = 0; x < INNER_WIDTH; ++x) {
            input64[y * INNER_WIDTH + x] = (static_cast<float>(x * INNER_HEIGHT + y + 1) / N) * 100.0f;
        }
    }
    std::cout << "Input TL: " << input64[0] << std::endl;
    std::cout << "Input TR: " << input64[INNER_WIDTH - 1] << std::endl;
    std::cout << "Input BL: " << input64[(INNER_HEIGHT - 1) * INNER_WIDTH] << std::endl;
    std::cout << "Input BR: " << input64[(INNER_HEIGHT - 1) * INNER_WIDTH + INNER_WIDTH - 1] << std::endl;

    // =========================================================================
    // STEP 2: Pad to 128x128 with replicate padding (post-padding, edge replication)
    // This matches official fast_conv_fft: padarray(X, pad_size, 'replicate', 'post')
    // =========================================================================
    std::cout << "=== Creating Replicate-Padded 128x128 Input ===" << std::endl;
    std::vector<float> paddedInput(WIDTH * HEIGHT);
    for (unsigned y = 0; y < HEIGHT; ++y) {
        for (unsigned x = 0; x < WIDTH; ++x) {
            unsigned src_x = std::min(x, INNER_WIDTH - 1);
            unsigned src_y = std::min(y, INNER_HEIGHT - 1);
            paddedInput[y * WIDTH + x] = input64[src_y * INNER_WIDTH + src_x];
        }
    }
    std::cout << "Padded TL: " << paddedInput[0] << std::endl;
    std::cout << "Padded TR: " << paddedInput[INNER_WIDTH - 1] << std::endl;
    std::cout << "Padded (64,0): " << paddedInput[64 * WIDTH + 0] << " (should equal input64[63,0])" << std::endl;
    std::cout << "Padded (0,64): " << paddedInput[64] << " (should equal input64[0,63])" << std::endl;
    std::cout << "Padded (127,127): " << paddedInput[127 * WIDTH + 127] << " (should equal input64[63,63])" << std::endl;

    // =========================================================================
    // STEP 3: Generate MTF filter using validated C++ MTF generator
    // Parameters match official hdrvdp_parse_options.m defaults
    // =========================================================================
    std::cout << "=== Generating MTF Filter (128x128) ===" << std::endl;
    double a[4], b[4];
    IQM::HDRVDP3::get_default_mtf_coefficients(a, b);
    std::vector<float> mtfFilter(WIDTH * HEIGHT);
    IQM::HDRVDP3::generate_mtf_filter(mtfFilter.data(), WIDTH, HEIGHT, 30.0, true);  // PPD = 30
    std::cout << "MTF[0,0] (DC): " << mtfFilter[0] << std::endl;
    std::cout << "MTF[64,64] (Nyquist): " << mtfFilter[64 * WIDTH + 64] << std::endl;

    // =========================================================================
    // STEP 4: Create GPU buffers
    // =========================================================================
    std::cout << "=== Creating GPU Buffers ===" << std::endl;
    
    // Work buffer for VkFFT (complex, interleaved float2)
    auto [workBuffer, workMemory] = VulkanResource::createBuffer(
        device, physicalDevice, COMPLEX_BUFFER_SIZE,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    workBuffer.bindMemory(workMemory, 0);

    // MTF filter buffer (real, single float per element)
    auto [mtfBuffer, mtfMemory] = VulkanResource::createBuffer(
        device, physicalDevice, REAL_BUFFER_SIZE,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    mtfBuffer.bindMemory(mtfMemory, 0);

    // Output buffer for inverse FFT result
    auto [outputBuffer, outputMemory] = VulkanResource::createBuffer(
        device, physicalDevice, COMPLEX_BUFFER_SIZE,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    outputBuffer.bindMemory(outputMemory, 0);

    // Staging buffer for host transfers
    auto [stagingBuffer, stagingMemory] = VulkanResource::createBuffer(
        device, physicalDevice, std::max(COMPLEX_BUFFER_SIZE, REAL_BUFFER_SIZE),
        vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    stagingBuffer.bindMemory(stagingMemory, 0);

    // =========================================================================
    // STEP 5: Upload padded input (real) to work buffer (complex, imag=0)
    // =========================================================================
    std::cout << "=== Uploading Padded Input to GPU ===" << std::endl;
    void* stagedData = stagingMemory.mapMemory(0, COMPLEX_BUFFER_SIZE);
    float* stagedFloat = static_cast<float*>(stagedData);
    for (unsigned i = 0; i < WIDTH * HEIGHT; ++i) {
        stagedFloat[i * 2] = paddedInput[i];     // real
        stagedFloat[i * 2 + 1] = 0.0f;           // imag
    }
    stagingMemory.unmapMemory();

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    cmdBuf.begin(beginInfo);
    vk::BufferCopy copyRegion{0, 0, COMPLEX_BUFFER_SIZE};
    cmdBuf.copyBuffer(stagingBuffer, workBuffer, copyRegion);
    
    vk::BufferMemoryBarrier barrier{
        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = workBuffer,
        .offset = 0,
        .size = COMPLEX_BUFFER_SIZE
    };
    cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, nullptr, {barrier}, nullptr);
    cmdBuf.end();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkCmdBuf;
    vkQueueSubmit(vkQueue, 1, &submitInfo, vkFence);
    vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkDev, 1, &vkFence);
    std::cout << "Padded input uploaded to GPU" << std::endl;

    // =========================================================================
    // STEP 6: Upload MTF filter to GPU
    // =========================================================================
    stagedData = stagingMemory.mapMemory(0, REAL_BUFFER_SIZE);
    std::memcpy(stagedData, mtfFilter.data(), REAL_BUFFER_SIZE);
    stagingMemory.unmapMemory();

    cmdBuf.begin(beginInfo);
    cmdBuf.copyBuffer(stagingBuffer, mtfBuffer, vk::BufferCopy{0, 0, REAL_BUFFER_SIZE});
    
    vk::BufferMemoryBarrier mtfBarrier{
        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = mtfBuffer,
        .offset = 0,
        .size = REAL_BUFFER_SIZE
    };
    cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, nullptr, {mtfBarrier}, nullptr);
    cmdBuf.end();

    submitInfo.pCommandBuffers = &vkCmdBuf;
    vkQueueSubmit(vkQueue, 1, &submitInfo, vkFence);
    vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkDev, 1, &vkFence);
    std::cout << "MTF filter uploaded to GPU" << std::endl;

    // =========================================================================
    // STEP 7: Initialize VkFFT for 128x128 C2C FFT
    // =========================================================================
    std::cout << "=== Initializing VkFFT (128x128 C2C) ===" << std::endl;
    VkFFTConfiguration config = {};
    config.FFTdim = 2;
    config.size[0] = WIDTH;
    config.size[1] = HEIGHT;
    config.numberBatches = 1;
    config.bufferNum = 1;
    
    VkBuffer vkWorkBuffer = toVkBuffer(workBuffer);
    uint64_t bufferSize[1] = {COMPLEX_BUFFER_SIZE};
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

    // =========================================================================
    // STEP 8: Create MTF multiply compute pipeline from SPIR-V
    // =========================================================================
    std::cout << "=== Creating MTF Multiply Compute Pipeline ===" << std::endl;
    
    // Read SPIR-V binary
    std::ifstream spvFile("shaders/hdrvdp3/mtf_multiply.spv", std::ios::binary);
    std::vector<char> spvData((std::istreambuf_iterator<char>(spvFile)), std::istreambuf_iterator<char>());
    spvFile.close();
    
    // Use raw Vulkan API for pipeline creation (RAII wrappers have different signatures)
    VkShaderModuleCreateInfo shaderCreateInfo{};
    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderCreateInfo.codeSize = spvData.size();
    shaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(spvData.data());
    VkShaderModule shaderModule;
    vkCreateShaderModule(vkDev, &shaderCreateInfo, nullptr, &shaderModule);
    
    // Descriptor set layout
    VkDescriptorSetLayoutBinding bindings[3] = {
        {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}
    };
    VkDescriptorSetLayoutCreateInfo dslCreateInfo{};
    dslCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dslCreateInfo.bindingCount = 3;
    dslCreateInfo.pBindings = bindings;
    VkDescriptorSetLayout descriptorSetLayout;
    vkCreateDescriptorSetLayout(vkDev, &dslCreateInfo, nullptr, &descriptorSetLayout);
    
    // Pipeline layout
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = 2 * sizeof(uint32_t);
    VkPipelineLayoutCreateInfo plCreateInfo{};
    plCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plCreateInfo.setLayoutCount = 1;
    plCreateInfo.pSetLayouts = &descriptorSetLayout;
    plCreateInfo.pushConstantRangeCount = 1;
    plCreateInfo.pPushConstantRanges = &pushConstantRange;
    VkPipelineLayout pipelineLayout;
    vkCreatePipelineLayout(vkDev, &plCreateInfo, nullptr, &pipelineLayout);
    
    // Descriptor pool
    VkDescriptorPoolSize poolSizes[3] = {
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}
    };
    VkDescriptorPoolCreateInfo dpCreateInfo{};
    dpCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dpCreateInfo.maxSets = 1;
    dpCreateInfo.poolSizeCount = 3;
    dpCreateInfo.pPoolSizes = poolSizes;
    VkDescriptorPool descriptorPool;
    vkCreateDescriptorPool(vkDev, &dpCreateInfo, nullptr, &descriptorPool);
    
    // Allocate descriptor set
    VkDescriptorSetAllocateInfo dsAllocInfo{};
    dsAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsAllocInfo.descriptorPool = descriptorPool;
    dsAllocInfo.descriptorSetCount = 1;
    dsAllocInfo.pSetLayouts = &descriptorSetLayout;
    VkDescriptorSet descriptorSet;
    vkAllocateDescriptorSets(vkDev, &dsAllocInfo, &descriptorSet);
    
    // Update descriptor set
    VkDescriptorBufferInfo spectrumInInfo{toVkBuffer(workBuffer), 0, COMPLEX_BUFFER_SIZE};
    VkDescriptorBufferInfo mtfFilterInfo{toVkBuffer(mtfBuffer), 0, REAL_BUFFER_SIZE};
    VkDescriptorBufferInfo spectrumOutInfo{toVkBuffer(outputBuffer), 0, COMPLEX_BUFFER_SIZE};
    
    VkWriteDescriptorSet writes[3] = {
        {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, descriptorSet, 0, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &spectrumInInfo, nullptr},
        {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, descriptorSet, 1, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &mtfFilterInfo, nullptr},
        {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, descriptorSet, 2, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &spectrumOutInfo, nullptr}
    };
    vkUpdateDescriptorSets(vkDev, 3, writes, 0, nullptr);
    
    // Compute pipeline
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName = "main";
    VkComputePipelineCreateInfo cpCreateInfo{};
    cpCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cpCreateInfo.stage = shaderStageInfo;
    cpCreateInfo.layout = pipelineLayout;
    VkPipeline computePipeline;
    vkCreateComputePipelines(vkDev, VK_NULL_HANDLE, 1, &cpCreateInfo, nullptr, &computePipeline);
    
    std::cout << "MTF multiply pipeline created" << std::endl;

    // =========================================================================
    // STEP 9: Run Forward FFT2
    // =========================================================================
    std::cout << "=== Running Forward FFT2 ===" << std::endl;
    VkFFTLaunchParams launchParams = {};
    launchParams.buffer = &vkWorkBuffer;
    launchParams.commandBuffer = &vkCmdBuf;

    cmdBuf.begin(beginInfo);
    resFFT = VkFFTAppend(&app, -1, &launchParams);
    if (resFFT != VKFFT_SUCCESS) {
        std::cerr << "Forward FFT failed: " << getVkFFTErrorString(resFFT) << std::endl;
        deleteVkFFT(&app);
        return 1;
    }
    cmdBuf.end();
    
    submitInfo.pCommandBuffers = &vkCmdBuf;
    vkQueueSubmit(vkQueue, 1, &submitInfo, vkFence);
    vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkDev, 1, &vkFence);
    std::cout << "Forward FFT2 completed" << std::endl;

    // =========================================================================
    // STEP 10: Run MTF Multiply (compute shader)
    // =========================================================================
    std::cout << "=== Running MTF Frequency Multiplication ===" << std::endl;
    
    // Begin command buffer for MTF multiply
    VkCommandBufferBeginInfo beginInfoMtf{};
    beginInfoMtf.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfoMtf.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(vkCmdBuf, &beginInfoMtf);
    
    // Barrier: FFT write -> MTF multiply read
    VkBufferMemoryBarrier fftToMtfBarrier{};
    fftToMtfBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    fftToMtfBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    fftToMtfBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    fftToMtfBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    fftToMtfBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    fftToMtfBarrier.buffer = toVkBuffer(workBuffer);
    fftToMtfBarrier.offset = 0;
    fftToMtfBarrier.size = COMPLEX_BUFFER_SIZE;
    vkCmdPipelineBarrier(vkCmdBuf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &fftToMtfBarrier, 0, nullptr);
    
    // Bind pipeline and descriptor set
    vkCmdBindPipeline(vkCmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
    vkCmdBindDescriptorSets(vkCmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    
    // Push constants
    uint32_t pushConstants[2] = {WIDTH, HEIGHT};
    vkCmdPushConstants(vkCmdBuf, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 8, pushConstants);
    
    // Dispatch
    vkCmdDispatch(vkCmdBuf, (WIDTH + 15) / 16, (HEIGHT + 15) / 16, 1);
    
    // Barrier: MTF multiply write -> IFFT read
    VkBufferMemoryBarrier mtfToIfftBarrier{};
    mtfToIfftBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    mtfToIfftBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    mtfToIfftBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    mtfToIfftBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mtfToIfftBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mtfToIfftBarrier.buffer = toVkBuffer(outputBuffer);
    mtfToIfftBarrier.offset = 0;
    mtfToIfftBarrier.size = COMPLEX_BUFFER_SIZE;
    vkCmdPipelineBarrier(vkCmdBuf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &mtfToIfftBarrier, 0, nullptr);
    vkEndCommandBuffer(vkCmdBuf);
    
    submitInfo.pCommandBuffers = &vkCmdBuf;
    vkQueueSubmit(vkQueue, 1, &submitInfo, vkFence);
    vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkDev, 1, &vkFence);
    std::cout << "MTF multiplication completed" << std::endl;

    // =========================================================================
    // STEP 11: Copy MTF output back to work buffer for inverse FFT
    // =========================================================================
    cmdBuf.begin(beginInfo);
    cmdBuf.copyBuffer(outputBuffer, workBuffer, copyRegion);
    cmdBuf.end();
    
    submitInfo.pCommandBuffers = &vkCmdBuf;
    vkQueueSubmit(vkQueue, 1, &submitInfo, vkFence);
    vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkDev, 1, &vkFence);

    // =========================================================================
    // STEP 12: Run Inverse FFT2
    // =========================================================================
    std::cout << "=== Running Inverse FFT2 ===" << std::endl;
    cmdBuf.begin(beginInfo);
    resFFT = VkFFTAppend(&app, 1, &launchParams);
    if (resFFT != VKFFT_SUCCESS) {
        std::cerr << "Inverse FFT failed: " << getVkFFTErrorString(resFFT) << std::endl;
        deleteVkFFT(&app);
        return 1;
    }
    cmdBuf.end();
    
    submitInfo.pCommandBuffers = &vkCmdBuf;
    vkQueueSubmit(vkQueue, 1, &submitInfo, vkFence);
    vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkDev, 1, &vkFence);
    std::cout << "Inverse FFT2 completed" << std::endl;

    // =========================================================================
    // STEP 13: Copy work buffer to output buffer for download
    // =========================================================================
    cmdBuf.begin(beginInfo);
    cmdBuf.copyBuffer(workBuffer, outputBuffer, copyRegion);
    cmdBuf.end();
    
    submitInfo.pCommandBuffers = &vkCmdBuf;
    vkQueueSubmit(vkQueue, 1, &submitInfo, vkFence);
    vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkDev, 1, &vkFence);

    // =========================================================================
    // STEP 14: Download results
    // =========================================================================
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
        .size = COMPLEX_BUFFER_SIZE
    };
    cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eHost, {}, nullptr, {downloadBarrier}, nullptr);
    cmdBuf.end();
    
    submitInfo.pCommandBuffers = &vkCmdBuf;
    vkQueueSubmit(vkQueue, 1, &submitInfo, vkFence);
    vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkDev, 1, &vkFence);
    
    std::vector<float> reconstructed(COMPLEX_BUFFER_SIZE / sizeof(float));
    stagedData = stagingMemory.mapMemory(0, COMPLEX_BUFFER_SIZE);
    std::memcpy(reconstructed.data(), stagedData, COMPLEX_BUFFER_SIZE);
    stagingMemory.unmapMemory();
    std::cout << "Results downloaded from GPU" << std::endl;

    // =========================================================================
    // STEP 15: Extract real part, crop 64x64, clamp to [1e-5, 1e10]
    // =========================================================================
    std::vector<float> output64(INNER_WIDTH * INNER_HEIGHT);
    for (unsigned y = 0; y < INNER_HEIGHT; ++y) {
        for (unsigned x = 0; x < INNER_WIDTH; ++x) {
            float val = reconstructed[(y * WIDTH + x) * 2];  // real part only
            val = std::max(1e-5f, std::min(1e10f, val));      // clamp
            output64[y * INNER_WIDTH + x] = val;
        }
    }
    
    printSampleValues(output64, INNER_WIDTH, INNER_HEIGHT, "Output 64x64 (clamped)");

    // =========================================================================
    // STEP 16: Load official Octave reference and compare
    // =========================================================================
    std::cout << "=== Loading Official Octave Reference ===" << std::endl;
    std::vector<float> octaveRef = loadReference("C:/temp/oct_mtf_output_rowmajor.txt", INNER_WIDTH, INNER_HEIGHT);
    std::cout << "Octave TL: " << octaveRef[0] << std::endl;
    std::cout << "Octave TR: " << octaveRef[INNER_WIDTH - 1] << std::endl;
    std::cout << "Octave BL: " << octaveRef[(INNER_HEIGHT - 1) * INNER_WIDTH] << std::endl;
    std::cout << "Octave BR: " << octaveRef[(INNER_HEIGHT - 1) * INNER_WIDTH + INNER_WIDTH - 1] << std::endl;

    auto [maxAbsErr, rmse] = computeErrors(octaveRef, output64, INNER_WIDTH, INNER_HEIGHT);
    
    std::cout << "=== GPU vs Octave Comparison (64x64) ===" << std::endl;
    std::cout << std::fixed << std::setprecision(8);
    std::cout << "MaxAbsErr: " << maxAbsErr << std::endl;
    std::cout << "RMSE:      " << rmse << std::endl;
    
    // Print corner comparison
    std::cout << "Corners:" << std::endl;
    std::cout << "  TL GPU=" << output64[0] << " Oct=" << octaveRef[0] << " Diff=" << std::abs(output64[0] - octaveRef[0]) << std::endl;
    std::cout << "  TR GPU=" << output64[INNER_WIDTH - 1] << " Oct=" << octaveRef[INNER_WIDTH - 1] << " Diff=" << std::abs(output64[INNER_WIDTH - 1] - octaveRef[INNER_WIDTH - 1]) << std::endl;
    std::cout << "  BL GPU=" << output64[(INNER_HEIGHT - 1) * INNER_WIDTH] << " Oct=" << octaveRef[(INNER_HEIGHT - 1) * INNER_WIDTH] << " Diff=" << std::abs(output64[(INNER_HEIGHT - 1) * INNER_WIDTH] - octaveRef[(INNER_HEIGHT - 1) * INNER_WIDTH]) << std::endl;
    std::cout << "  BR GPU=" << output64[(INNER_HEIGHT - 1) * INNER_WIDTH + INNER_WIDTH - 1] << " Oct=" << octaveRef[(INNER_HEIGHT - 1) * INNER_WIDTH + INNER_WIDTH - 1] << " Diff=" << std::abs(output64[(INNER_HEIGHT - 1) * INNER_WIDTH + INNER_WIDTH - 1] - octaveRef[(INNER_HEIGHT - 1) * INNER_WIDTH + INNER_WIDTH - 1]) << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // Cleanup
    // =========================================================================
    vkDestroyPipeline(vkDev, computePipeline, nullptr);
    vkDestroyPipelineLayout(vkDev, pipelineLayout, nullptr);
    vkDestroyDescriptorPool(vkDev, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(vkDev, descriptorSetLayout, nullptr);
    vkDestroyShaderModule(vkDev, shaderModule, nullptr);
    deleteVkFFT(&app);
    std::cout << "=== Cleanup Complete ===" << std::endl;
    
    // Report result
    bool success = (maxAbsErr < 1e-3f) && (rmse < 1e-4f);  // Relaxed tolerance for MTF
    std::cout << "=== Test Result ===" << std::endl;
    if (success) {
        std::cout << "PASSED" << std::endl;
    } else {
        std::cout << "FAILED" << std::endl;
        std::cout << "  MaxAbsErr: " << maxAbsErr << " (tolerance: 1e-3)" << std::endl;
        std::cout << "  RMSE: " << rmse << " (tolerance: 1e-4)" << std::endl;
    }
    
    return success ? 0 : 1;
}