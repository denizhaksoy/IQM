/*

 * HDR-VDP-3 FFT-based Local Adaptation Test

 * Tests FFT2 -> Gaussian multiply -> IFFT2 against official Octave reference

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
#include <stdexcept>
#include <iterator>



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



inline VkBuffer toVkBuffer(const vk::raii::Buffer& raiiBuffer) { return static_cast<VkBuffer>(*raiiBuffer); }

inline VkCommandBuffer toVkCommandBuffer(const vk::raii::CommandBuffer& raiiCmdBuf) { return static_cast<VkCommandBuffer>(*raiiCmdBuf); }

inline VkQueue toVkQueue(const vk::raii::Queue& raiiQueue) { return static_cast<VkQueue>(*raiiQueue); }

inline VkCommandPool toVkCommandPool(const vk::raii::CommandPool& raiiCmdPool) { return static_cast<VkCommandPool>(*raiiCmdPool); }

inline VkFence toVkFence(const vk::raii::Fence& raiiFence) { return static_cast<VkFence>(*raiiFence); }

inline VkPhysicalDevice toVkPhysicalDevice(const vk::raii::PhysicalDevice& raiiPhysDev) { return static_cast<VkPhysicalDevice>(*raiiPhysDev); }

inline VkDevice toVkDevice(const vk::raii::Device& raiiDevice) { return static_cast<VkDevice>(*raiiDevice); }



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

    if (!file.is_open()) {
        throw std::runtime_error("Cannot open Octave reference: " + filename);
    }
    std::vector<float> data(expectedWidth * expectedHeight);

    for (size_t i = 0; i < data.size(); ++i) {
        if (!(file >> data[i])) {
            throw std::runtime_error("Octave reference has missing or invalid values: " + filename);
        }
    }

    if (!file && !file.eof()) {
        throw std::runtime_error("Failed to parse Octave reference: " + filename);
    }
    file.close();
    return data;

}



int main() {

    std::cout << "=========================================" << std::endl;

    std::cout << "HDR-VDP-3 FFT-based Local Adaptation Test" << std::endl;

    std::cout << "FFT2 -> Gaussian multiply -> IFFT2" << std::endl;

    std::cout << "=========================================" << std::endl;

    std::cout << "Test: 64x64 input -> log-clamp -> pad 128x128 (replicate) -> FFT2 x Gaussian -> IFFT2 -> crop 64x64 -> exp()" << std::endl;

    std::cout << std::endl;



    VulkanInstance instance(true);



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

    // STEP 2: Apply log-clamp to input

    // log-clamp: log(max(x, 1e-6)) to avoid log(0)

    // =========================================================================

    std::cout << "=== Applying Log-Clamp ===" << std::endl;

    std::vector<float> logClamped64(INNER_WIDTH * INNER_HEIGHT);

    for (unsigned i = 0; i < INNER_WIDTH * INNER_HEIGHT; ++i) {

        logClamped64[i] = std::log(std::max(input64[i], 1e-6f));

    }

    std::cout << "Log-clamped TL: " << logClamped64[0] << std::endl;

    std::cout << "Log-clamped BR: " << logClamped64[INNER_WIDTH * INNER_HEIGHT - 1] << std::endl;



    // =========================================================================

    // STEP 3: Pad to 128x128 with replicate padding (post-padding)

    // This matches official fast_conv_fft: padarray(X, pad_size, 'replicate', 'post')

    // =========================================================================

    std::cout << "=== Creating Replicate-Padded 128x128 Input ===" << std::endl;

    std::vector<float> paddedInput(WIDTH * HEIGHT);

    for (unsigned y = 0; y < HEIGHT; ++y) {

        for (unsigned x = 0; x < WIDTH; ++x) {

            unsigned src_x = std::min(x, INNER_WIDTH - 1);

            unsigned src_y = std::min(y, INNER_HEIGHT - 1);

            paddedInput[y * WIDTH + x] = logClamped64[src_y * INNER_WIDTH + src_x];

        }

    }

    std::cout << "Padded TL: " << paddedInput[0] << std::endl;

    std::cout << "Padded TR: " << paddedInput[INNER_WIDTH - 1] << std::endl;

    std::cout << "Padded (64,0): " << paddedInput[64 * WIDTH + 0] << " (should equal logClamped64[63,0])" << std::endl;

    std::cout << "Padded (127,127): " << paddedInput[127 * WIDTH + 127] << " (should equal logClamped64[63,63])" << std::endl;





    // =========================================================================

    // STEP 4: Generate 2D frequency-domain Gaussian kernel (matching fast_gauss.m)

    // sigma = 10^(-0.781367) * ppd (e.g., 4.9631 at ppd=30)

    // Kernel is 128x128 for 64x64 input, with DC at index 0,0

    // KX0 = fmod(0.5 + y/M, 1) - 0.5; KX1 = KX0 * 2*pi; K = exp(-0.5*(kx1^2+ky1^2)*sigma^2)

    // =========================================================================

    std::cout << "=== Generating 2D Frequency-Domain Gaussian Kernel (128x128) ===" << std::endl;

    double sigma = std::pow(10.0, -0.781367) * 30.0;  // ppd = 30

    std::cout << "Sigma: " << sigma << std::endl;



    std::vector<float> gaussianKernel(WIDTH * HEIGHT);

    for (unsigned y = 0; y < HEIGHT; ++y) {

        for (unsigned x = 0; x < WIDTH; ++x) {

            // KX0 = fmod(0.5 + y/M, 1) - 0.5 (with M=128)

            double kx0 = std::fmod(0.5 + static_cast<double>(y) / HEIGHT, 1.0) - 0.5;

            double ky0 = std::fmod(0.5 + static_cast<double>(x) / WIDTH, 1.0) - 0.5;

            double kx1 = kx0 * 2.0 * M_PI;

            double ky1 = ky0 * 2.0 * M_PI;

            gaussianKernel[y * WIDTH + x] = static_cast<float>(std::exp(-0.5 * (kx1 * kx1 + ky1 * ky1) * sigma * sigma));

        }

    }

    std::cout << "Gaussian[0,0] (DC): " << gaussianKernel[0] << std::endl;

    std::cout << "Gaussian[64,64] (Nyquist): " << gaussianKernel[64 * WIDTH + 64] << std::endl;



    // =========================================================================

    // STEP 5: Create GPU buffers

    // =========================================================================

    std::cout << "=== Creating GPU Buffers ===" << std::endl;



    // Work buffer for VkFFT (complex, interleaved float2)

    auto [workBuffer, workMemory] = VulkanResource::createBuffer(

        device, physicalDevice, COMPLEX_BUFFER_SIZE,

        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,

        vk::MemoryPropertyFlagBits::eDeviceLocal

    );

    workBuffer.bindMemory(*workMemory, 0);



    // Gaussian kernel buffer (real, single float per element)

    auto [kernelBuffer, kernelMemory] = VulkanResource::createBuffer(

        device, physicalDevice, REAL_BUFFER_SIZE,

        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,

        vk::MemoryPropertyFlagBits::eDeviceLocal

    );

    kernelBuffer.bindMemory(*kernelMemory, 0);



    // Output buffer for inverse FFT result

    auto [outputBuffer, outputMemory] = VulkanResource::createBuffer(

        device, physicalDevice, COMPLEX_BUFFER_SIZE,

        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,

        vk::MemoryPropertyFlagBits::eDeviceLocal

    );

    outputBuffer.bindMemory(*outputMemory, 0);



    // Staging buffer for host transfers

    auto [stagingBuffer, stagingMemory] = VulkanResource::createBuffer(

        device, physicalDevice, std::max(COMPLEX_BUFFER_SIZE, REAL_BUFFER_SIZE),

        vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,

        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent

    );

    stagingBuffer.bindMemory(*stagingMemory, 0);



    // =========================================================================

    // STEP 6: Upload padded input (real) to work buffer (complex, imag=0)

    // =========================================================================

    std::cout << "=== Uploading Padded Input to GPU: mapMemory ===" << std::endl;

    void* stagedData = stagingMemory.mapMemory(0, COMPLEX_BUFFER_SIZE);

    if (!stagedData) { std::cerr << "FAIL: stagingMemory.mapMemory returned nullptr" << std::endl; return 1; }

    std::cout << "  mapMemory success" << std::endl << std::flush;

    float* stagedFloat = static_cast<float*>(stagedData);

    // Write paddedInput as complex (real=value, imag=0)

    for (unsigned i = 0; i < WIDTH * HEIGHT; ++i) {

        stagedFloat[2 * i] = paddedInput[i];     // real

        stagedFloat[2 * i + 1] = 0.0f;           // imag;

    }

    stagingMemory.unmapMemory();

    std::cout << "  unmapMemory success" << std::endl << std::flush;



    vk::CommandBufferBeginInfo beginInfo{};

    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    std::cout << "=== Uploading Padded Input to GPU: cmdBuf.begin ===" << std::endl;

    cmdBuf.reset({});

    cmdBuf.begin(beginInfo);

    std::cout << "=== Uploading Padded Input to GPU: cmdBuf.begin success ===" << std::endl;

    vk::BufferCopy copyRegion{0, 0, COMPLEX_BUFFER_SIZE};

    std::cout << "=== Uploading Padded Input to GPU: about to copyBuffer ===" << std::endl;

    cmdBuf.copyBuffer(*stagingBuffer, *workBuffer, copyRegion);

    std::cout << "=== Uploading Padded Input to GPU: copyBuffer completed ===" << std::endl;



    vk::BufferMemoryBarrier barrier{

        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,

        .dstAccessMask = vk::AccessFlagBits::eShaderRead,

        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

        .buffer = *workBuffer,

        .offset = 0,

        .size = COMPLEX_BUFFER_SIZE

    };

    std::cout << "=== Uploading Padded Input to GPU: about to pipelineBarrier ===" << std::endl;

    cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, nullptr, {barrier}, nullptr);

    std::cout << "=== Uploading Padded Input to GPU: pipelineBarrier completed ===" << std::endl;

    cmdBuf.end();

    std::cout << "=== Uploading Padded Input to GPU: cmdBuf.end success ===" << std::endl;



    VkSubmitInfo submitInfo{};

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.commandBufferCount = 1;

    submitInfo.pCommandBuffers = &vkCmdBuf;

    std::cout << "=== Uploading Padded Input to GPU: vkQueueSubmit ===" << std::endl;

    VkResult resSubmit = vkQueueSubmit(vkQueue, 1, &submitInfo, vkFence);

    std::cout << "  vkQueueSubmit returned: " << resSubmit << std::endl;

    std::cout << "=== Uploading Padded Input to GPU: vkWaitForFences ===" << std::endl;

    VkResult resWait = vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);

    std::cout << "  vkWaitForFences returned: " << resWait << std::endl;

    vkResetFences(vkDev, 1, &vkFence);

    std::cout << "=== Uploading Padded Input to GPU: success ===" << std::endl;

    // === DIAG Stage 1: Read back workBuffer after input upload ===

    do {

        size_t diag1_rdbSize = COMPLEX_BUFFER_SIZE;

        auto [diag1_rb, diag1_rm] = VulkanResource::createBuffer(

            device, physicalDevice, diag1_rdbSize,

            vk::BufferUsageFlagBits::eTransferDst,

            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent

        );

        diag1_rb.bindMemory(*diag1_rm, 0);



        vk::CommandBufferBeginInfo db1_bi{};

        db1_bi.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        cmdBuf.reset({});

        cmdBuf.begin(db1_bi);

        cmdBuf.copyBuffer(*workBuffer, *diag1_rb, copyRegion);

        cmdBuf.end();



        submitInfo.pCommandBuffers = &vkCmdBuf;

        vkQueueSubmit(vkQueue, 1, &submitInfo, vkFence);

        vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);

        vkResetFences(vkDev, 1, &vkFence);



        void* diag1_rd = diag1_rm.mapMemory(0, diag1_rdbSize);

        if (diag1_rd) {

            float* diag1_f = static_cast<float*>(diag1_rd);

            std::cout << "=== DIAG1: After input upload - workBuffer first 5 complex ===" << std::endl;

            for (unsigned i = 0; i < 5; ++i) {

                std::cout << "  [" << i << "] real=" << diag1_f[2*i] << " imag=" << diag1_f[2*i+1] << std::endl;

            }

            bool allImagZero = true;

            for (unsigned i = 0; i < 10; ++i) {

                if (std::abs(diag1_f[i*2+1]) > 1e-6f) allImagZero = false;

            }

            std::cout << "  All imag zero: " << (allImagZero ? "YES" : "NO") << std::endl;

            diag1_rm.unmapMemory();

        } else {

            std::cerr << "FAIL: diag1 readback map failed" << std::endl;

        }

    } while (0);







    // =========================================================================

    // STEP 7: Upload Gaussian kernel to GPU

    // =========================================================================

    stagedData = stagingMemory.mapMemory(0, REAL_BUFFER_SIZE);

    std::memcpy(stagedData, gaussianKernel.data(), REAL_BUFFER_SIZE);

    stagingMemory.unmapMemory();



    cmdBuf.reset({});



    cmdBuf.begin(beginInfo);

    cmdBuf.copyBuffer(*stagingBuffer, *kernelBuffer, vk::BufferCopy{0, 0, REAL_BUFFER_SIZE});



    vk::BufferMemoryBarrier mtfBarrier{

        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,

        .dstAccessMask = vk::AccessFlagBits::eShaderRead,

        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

        .buffer = *kernelBuffer,

        .offset = 0,

        .size = REAL_BUFFER_SIZE

    };

    cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, nullptr, {mtfBarrier}, nullptr);

    cmdBuf.end();



    submitInfo.pCommandBuffers = &vkCmdBuf;

    vkQueueSubmit(vkQueue, 1, &submitInfo, vkFence);

    vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);

    vkResetFences(vkDev, 1, &vkFence);

    std::cout << "Gaussian kernel uploaded to GPU" << std::endl;



    // =========================================================================

    // STEP 8: Initialize VkFFT for 128x128 C2C FFT

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

    // STEP 9: Run Forward FFT2

    // =========================================================================

    std::cout << "=== Running Forward FFT2 ===" << std::endl;

    VkFFTLaunchParams launchParams = {};

    launchParams.buffer = &vkWorkBuffer;

    launchParams.commandBuffer = &vkCmdBuf;



    cmdBuf.reset({});



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



    // === DIAG Stage 2: Read back workBuffer after forward FFT ===

    do {

        size_t diag2_rdbSize = COMPLEX_BUFFER_SIZE;

        auto [diag2_rb, diag2_rm] = VulkanResource::createBuffer(

            device, physicalDevice, diag2_rdbSize,

            vk::BufferUsageFlagBits::eTransferDst,

            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent

        );

        diag2_rb.bindMemory(*diag2_rm, 0);



        vk::CommandBufferBeginInfo db2_bi{};

        db2_bi.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        cmdBuf.reset({});

        cmdBuf.begin(db2_bi);

        cmdBuf.copyBuffer(*workBuffer, *diag2_rb, copyRegion);

        cmdBuf.end();



        submitInfo.pCommandBuffers = &vkCmdBuf;

        vkQueueSubmit(vkQueue, 1, &submitInfo, vkFence);

        vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);

        vkResetFences(vkDev, 1, &vkFence);



        void* diag2_rd = diag2_rm.mapMemory(0, diag2_rdbSize);

        if (diag2_rd) {

            float* diag2_f = static_cast<float*>(diag2_rd);

            std::cout << "=== DIAG2: After forward FFT - DC coeff (real, imag) ===" << std::endl;

            std::cout << "  DC real=" << diag2_f[0] << " imag=" << diag2_f[1] << std::endl;

            std::cout << "=== DIAG2: After forward FFT - first 5 complex coeffs ===" << std::endl;

            for (unsigned i = 0; i < 5; ++i) {

                std::cout << "  [" << i << "] real=" << diag2_f[2*i] << " imag=" << diag2_f[2*i+1] << std::endl;

            }

            // Compute max magnitude and nonzero count

            float maxMag = 0.0f;

            int nonzeroCount = 0;

            for (unsigned i = 0; i < WIDTH * HEIGHT; ++i) {

                float re = diag2_f[2*i];

                float im = diag2_f[2*i+1];

                float mag = std::sqrt(re*re + im*im);

                if (mag > maxMag) maxMag = mag;

                if (mag > 1e-6f) nonzeroCount++;

            }

            std::cout << "=== DIAG2: After forward FFT - max mag=" << maxMag

                      << " nonzeroCoeffs=" << nonzeroCount << "/" << (WIDTH*HEIGHT) << std::endl;

            diag2_rm.unmapMemory();

        } else {

            std::cerr << "FAIL: diag2 readback map failed" << std::endl;

        }

    } while (0);



    // =========================================================================

    // STEP 10: Run Gaussian Multiply (compute shader)

    // =========================================================================

    std::cout << "=== Running Gaussian Frequency Multiplication ===" << std::endl;



    // Read SPIR-V binary for multiply shader

    std::ifstream spvFile("shaders/hdrvdp3/mtf_multiply.spv", std::ios::binary);

    std::vector<char> spvData((std::istreambuf_iterator<char>(spvFile)), std::istreambuf_iterator<char>());

    spvFile.close();
    if (spvData.empty() || spvData.size() % sizeof(uint32_t) != 0) {
        std::cerr << "FAIL: SPIR-V missing, empty, or malformed: shaders/hdrvdp3/mtf_multiply.spv" << std::endl;
        deleteVkFFT(&app);
        return 1;
    }



    // Use raw Vulkan API for pipeline creation

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

    VkDescriptorBufferInfo kernelFilterInfo{toVkBuffer(kernelBuffer), 0, REAL_BUFFER_SIZE};

    VkDescriptorBufferInfo spectrumOutInfo{toVkBuffer(outputBuffer), 0, COMPLEX_BUFFER_SIZE};



    VkWriteDescriptorSet writes[3] = {

        {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, descriptorSet, 0, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &spectrumInInfo, nullptr},

        {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, descriptorSet, 1, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &kernelFilterInfo, nullptr},

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



    std::cout << "Gaussian multiply pipeline created" << std::endl;



    // =========================================================================

    // STEP 10: Run Gaussian Frequency Multiplication

    // =========================================================================

std::cout << "=== Running Gaussian Frequency Multiplication ===" << std::endl << std::flush;



    // Begin command buffer for MTF multiply
    cmdBuf.reset({});
    cmdBuf.begin(beginInfo);



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

    std::cout << "Gaussian frequency multiplication completed" << std::endl;



    // === DIAG Stage 3: Read back outputBuffer after Gaussian multiply ===

    do {

        size_t diag3_rdbSize = COMPLEX_BUFFER_SIZE;

        auto [diag3_rb, diag3_rm] = VulkanResource::createBuffer(

            device, physicalDevice, diag3_rdbSize,

            vk::BufferUsageFlagBits::eTransferDst,

            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent

        );

        diag3_rb.bindMemory(*diag3_rm, 0);



        vk::CommandBufferBeginInfo db3_bi{};

        db3_bi.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        cmdBuf.reset({});

        cmdBuf.begin(db3_bi);

        cmdBuf.copyBuffer(*outputBuffer, *diag3_rb, copyRegion);

        cmdBuf.end();



        submitInfo.pCommandBuffers = &vkCmdBuf;

        vkQueueSubmit(vkQueue, 1, &submitInfo, vkFence);

        vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);

        vkResetFences(vkDev, 1, &vkFence);



        void* diag3_rd = diag3_rm.mapMemory(0, diag3_rdbSize);

        if (diag3_rd) {

            float* diag3_f = static_cast<float*>(diag3_rd);

            std::cout << "=== DIAG3: After Gaussian multiply - outputBuffer DC coeff (real, imag) ===" << std::endl;

            std::cout << "  DC real=" << diag3_f[0] << " imag=" << diag3_f[1] << std::endl;

            std::cout << "=== DIAG3: After Gaussian multiply - first 5 complex coeffs ===" << std::endl;

            for (unsigned i = 0; i < 5; ++i) {

                std::cout << "  [" << i << "] real=" << diag3_f[2*i] << " imag=" << diag3_f[2*i+1] << std::endl;

            }

            // Check if DC is preserved (Gaussian[0,0]=1 means DC should be unchanged)

            std::cout << "  DC preserved from forward FFT stage" << std::endl;

            // Check if all data is zero (would explain GPU output = 1.0 after exp())

            bool allZero = true;

            for (unsigned i = 0; i < WIDTH * HEIGHT; ++i) {

                if (std::abs(diag3_f[2*i]) > 1e-6f || std::abs(diag3_f[2*i+1]) > 1e-6f) {

                    allZero = false;

                    break;

                }

            }

            std::cout << "  All coefficients zero: " << (allZero ? "YES - this explains GPU output=1.0!" : "NO - has data") << std::endl;

            diag3_rm.unmapMemory();

        } else {

            std::cerr << "FAIL: diag3 readback map failed" << std::endl;

        }

    } while (0);



    // === DIAGNOSTIC: Verify Gaussian multiply produced non-zero data ===

    // Check if outputBuffer has all zeros (would explain GPU output = 1.0 after exp())

    cmdBuf.reset({});

    cmdBuf.begin(beginInfo);

    cmdBuf.copyBuffer(*outputBuffer, *stagingBuffer, vk::BufferCopy{0, 0, COMPLEX_BUFFER_SIZE});

    cmdBuf.end();



submitInfo.pCommandBuffers = &vkCmdBuf;

    vkQueueSubmit(vkQueue, 1, &submitInfo, vkFence);

    vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);

    vkResetFences(vkDev, 1, &vkFence);



    stagedData = stagingMemory.mapMemory(0, COMPLEX_BUFFER_SIZE);

    if (stagedData) {

        float* sf = static_cast<float*>(stagedData);

        bool allZero = true;

        for (unsigned i = 0; i < WIDTH * HEIGHT; ++i) {

            if (std::abs(sf[2*i]) > 1e-6f || std::abs(sf[2*i+1]) > 1e-6f) {

                allZero = false;

                break;

            }

        }

        std::cout << "=== DIAG: After Gaussian multiply - all coefficients zero? " << (allZero ? "YES - ALL ZERO!" : "NO - has data") << std::endl;

        stagingMemory.unmapMemory();

    }



    // =========================================================================

    // STEP 11: Copy filtered spectrum from outputBuffer to workBuffer

    // =========================================================================

    // STEP 11: Copy filtered spectrum from outputBuffer to workBuffer

    // then Run Inverse FFT2

    // =========================================================================

    // Copy outputBuffer (filtered spectrum) to workBuffer for VkFFT inverse

    cmdBuf.reset({});

    cmdBuf.begin(beginInfo);

    cmdBuf.copyBuffer(*outputBuffer, *workBuffer, vk::BufferCopy{0, 0, COMPLEX_BUFFER_SIZE});

    cmdBuf.end();



    submitInfo.pCommandBuffers = &vkCmdBuf;

    vkQueueSubmit(vkQueue, 1, &submitInfo, vkFence);

    vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);

    vkResetFences(vkDev, 1, &vkFence);



    // === DIAG Stage 4: Verify copy of filtered spectrum to workBuffer ===

    do {

        size_t diag4_rdbSize = COMPLEX_BUFFER_SIZE;

        auto [diag4_rb, diag4_rm] = VulkanResource::createBuffer(

            device, physicalDevice, diag4_rdbSize,

            vk::BufferUsageFlagBits::eTransferDst,

            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent

        );

        diag4_rb.bindMemory(*diag4_rm, 0);



        vk::CommandBufferBeginInfo db4_bi{};

        db4_bi.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        cmdBuf.reset({});

        cmdBuf.begin(db4_bi);

        cmdBuf.copyBuffer(*workBuffer, *diag4_rb, copyRegion);

        cmdBuf.end();



        submitInfo.pCommandBuffers = &vkCmdBuf;

        vkQueueSubmit(vkQueue, 1, &submitInfo, vkFence);

        vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);

        vkResetFences(vkDev, 1, &vkFence);



        void* diag4_rd = diag4_rm.mapMemory(0, diag4_rdbSize);

        if (diag4_rd) {

            float* diag4_f = static_cast<float*>(diag4_rd);

            std::cout << "=== DIAG4: After copy filtered spectrum - workBuffer DC (real, imag) ===" << std::endl;

            std::cout << "  DC real=" << diag4_f[0] << " imag=" << diag4_f[1] << std::endl;

            std::cout << "=== DIAG4: After copy filtered spectrum - first 5 complex coeffs ===" << std::endl;

            for (unsigned i = 0; i < 5; ++i) {

                std::cout << "  [" << i << "] real=" << diag4_f[2*i] << " imag=" << diag4_f[2*i+1] << std::endl;

            }

            diag4_rm.unmapMemory();

        } else {

            std::cerr << "FAIL: diag4 readback map failed" << std::endl;

        }

    } while (0);




    // === DIAGNOSTIC: Verify copy of filtered spectrum to workBuffer ===

    size_t rdbSize = COMPLEX_BUFFER_SIZE;

    auto [readBackBuffer, readBackMemory] = VulkanResource::createBuffer(

        device, physicalDevice, rdbSize,

        vk::BufferUsageFlagBits::eTransferDst,

        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent

    );

    readBackBuffer.bindMemory(*readBackMemory, 0);



    vk::CommandBufferBeginInfo diBeginInfo{};

    diBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;



    cmdBuf.reset({});



    cmdBuf.begin(diBeginInfo);

    cmdBuf.copyBuffer(*workBuffer, *readBackBuffer, copyRegion);

    cmdBuf.end();



    submitInfo.pCommandBuffers = &vkCmdBuf;

    vkQueueSubmit(vkQueue, 1, &submitInfo, vkFence);

    vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);

    vkResetFences(vkDev, 1, &vkFence);



    void* rdBackData = readBackMemory.mapMemory(0, rdbSize);

    if (rdBackData) {

        float* rdBackFloat = static_cast<float*>(rdBackData);

        std::cout << "=== DIAG: After copy filtered spectrum - workBuffer DC (real, imag) ===" << std::endl;

        std::cout << "  DC real=" << rdBackFloat[0] << " imag=" << rdBackFloat[1] << std::endl;

        std::cout << "=== DIAG: After copy filtered spectrum - first 5 complex coeffs ===" << std::endl;

        for (unsigned i = 0; i < 5; ++i) {

            std::cout << "  [" << i << "] real=" << rdBackFloat[2*i] << " imag=" << rdBackFloat[2*i+1] << std::endl;

        }

        readBackMemory.unmapMemory();

    } else {

        std::cerr << "FAIL: readBackMemory.mapMemory returned nullptr" << std::endl;

    }



    // Transfer-write -> compute-read dependency for VkFFT input.
    cmdBuf.reset({});
    cmdBuf.begin(beginInfo);
    VkBufferMemoryBarrier copyToFFTBarrier{};
    copyToFFTBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    copyToFFTBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    copyToFFTBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    copyToFFTBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copyToFFTBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copyToFFTBarrier.buffer = toVkBuffer(workBuffer);
    copyToFFTBarrier.offset = 0;
    copyToFFTBarrier.size = COMPLEX_BUFFER_SIZE;
    vkCmdPipelineBarrier(vkCmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr,
                         1, &copyToFFTBarrier, 0, nullptr);

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



    // === DIAG Stage 5: Read back workBuffer after inverse FFT ===

    do {

        size_t diag5_rdbSize = COMPLEX_BUFFER_SIZE;

        auto [diag5_rb, diag5_rm] = VulkanResource::createBuffer(

            device, physicalDevice, diag5_rdbSize,

            vk::BufferUsageFlagBits::eTransferDst,

            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent

        );

        diag5_rb.bindMemory(*diag5_rm, 0);



        vk::CommandBufferBeginInfo db5_bi{};

        db5_bi.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        cmdBuf.reset({});

        cmdBuf.begin(db5_bi);

        cmdBuf.copyBuffer(*workBuffer, *diag5_rb, copyRegion);

        cmdBuf.end();



        submitInfo.pCommandBuffers = &vkCmdBuf;

        vkQueueSubmit(vkQueue, 1, &submitInfo, vkFence);

        vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);

        vkResetFences(vkDev, 1, &vkFence);



        void* diag5_rd = diag5_rm.mapMemory(0, diag5_rdbSize);

        if (diag5_rd) {

            float* diag5_f = static_cast<float*>(diag5_rd);

            std::cout << "=== DIAG5: After inverse FFT - workBuffer first 5 complex values ===" << std::endl;

            for (unsigned i = 0; i < 5; ++i) {

                std::cout << "  [" << i << "] real=" << diag5_f[2*i] << " imag=" << diag5_f[2*i+1] << std::endl;

            }

            // Compute min/max real values and max|imag|

            float minReal = diag5_f[0], maxReal = diag5_f[0];

            float maxImagMag = 0.0f;

            int nonzeroCount = 0;

            for (unsigned i = 0; i < WIDTH * HEIGHT; ++i) {

                float re = diag5_f[2*i];

                float im = diag5_f[2*i+1];

                if (re < minReal) minReal = re;

                if (re > maxReal) maxReal = re;

                float imMag = std::abs(im);

                if (imMag > maxImagMag) maxImagMag = imMag;

                if (imMag > 1e-6f) nonzeroCount++;

            }

            std::cout << "=== DIAG5: After inverse FFT - real range=[" << minReal << ", " << maxReal

                      << "], max|imag|=" << maxImagMag << ", nonzeroCoeffs=" << nonzeroCount << "/" << (WIDTH*HEIGHT) << std::endl;

            diag5_rm.unmapMemory();

        } else {

            std::cerr << "FAIL: diag5 readback map failed" << std::endl;

        }

    } while (0);



    // =========================================================================

    // STEP 12: Copy work buffer to output buffer for download

    // =========================================================================

    cmdBuf.reset({});

    cmdBuf.begin(beginInfo);

    cmdBuf.copyBuffer(*workBuffer, *outputBuffer, copyRegion);

    cmdBuf.end();



    submitInfo.pCommandBuffers = &vkCmdBuf;

    vkQueueSubmit(vkQueue, 1, &submitInfo, vkFence);

    vkWaitForFences(vkDev, 1, &vkFence, VK_TRUE, UINT64_MAX);

    vkResetFences(vkDev, 1, &vkFence);



    // =========================================================================

    // STEP 13: Download results

    // =========================================================================

    std::cout << "=== Downloading Results ===" << std::endl;

    cmdBuf.reset({});

    cmdBuf.begin(beginInfo);

    cmdBuf.copyBuffer(*outputBuffer, *stagingBuffer, copyRegion);



    vk::BufferMemoryBarrier downloadBarrier{

        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,

        .dstAccessMask = vk::AccessFlagBits::eHostRead,

        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

        .buffer = *stagingBuffer,

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

    // STEP 14: Crop 128x128 to 64x64 (top-left), take real part, apply exp()

    // =========================================================================

    std::cout << "=== Cropping and Applying exp() ===" << std::endl;

    std::vector<float> output64(INNER_WIDTH * INNER_HEIGHT);

    std::cout << "=== Downloaded reconstructed values (real part, before exp) ===" << std::endl;

    for (unsigned i = 0; i < std::min(20u, INNER_WIDTH * INNER_HEIGHT); ++i) {

        std::cout << "  reconstructed[" << i << "]=" << reconstructed[i * 2] << " imag=" << reconstructed[i * 2 + 1] << std::endl;

    }

    std::cout << "RECON_FIRST_20 below:" << std::endl;

    for (unsigned i = 0; i < std::min(20u, INNER_WIDTH * INNER_HEIGHT); ++i) {

        std::cout << "  recon[" << i << "]=" << reconstructed[i * 2] << " imag=" << reconstructed[i * 2 + 1] << std::endl;

    }



    std::cout << "=== Cropping and Applying exp() ===" << std::endl;

    for (unsigned y = 0; y < INNER_HEIGHT; ++y) {

        for (unsigned x = 0; x < INNER_WIDTH; ++x) {

            float val = reconstructed[(y * WIDTH + x) * 2];  // real part only

            val = std::exp(val);  // apply exp() after inverse FFT

            output64[y * INNER_WIDTH + x] = val;

        }

    }



    printSampleValues(output64, INNER_WIDTH, INNER_HEIGHT, "Output 64x64 (after exp)");



    // =========================================================================

    // STEP 15: Load official Octave reference and compare

    // =========================================================================

    std::cout << "=== Loading Official Octave Reference ===" << std::endl;

    // Reference format: 64 lines with 64 space-separated values per line (row-major, no header)

    std::vector<float> octaveRef = loadReference("C:/temp/oct_la_verified.txt", INNER_WIDTH, INNER_HEIGHT);

    std::cout << "Octave TL: " << octaveRef[0] << std::endl;

    std::cout << "Octave TR: " << octaveRef[INNER_WIDTH - 1] << std::endl;

    std::cout << "Octave BL: " << octaveRef[(INNER_HEIGHT - 1) * INNER_WIDTH] << std::endl;

    std::cout << "Octave BR: " << octaveRef[(INNER_HEIGHT - 1) * INNER_WIDTH + INNER_WIDTH - 1] << std::endl;



    auto [maxAbsErr, rmse] = computeErrors(octaveRef, output64, INNER_WIDTH, INNER_HEIGHT);



    std::cout << "=== FFT Local Adaptation vs Octave Comparison (64x64) ===" << std::endl;

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

    bool success = (maxAbsErr < 1e-3f && rmse < 1e-4f);  // FFT should match reference within tolerance

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
