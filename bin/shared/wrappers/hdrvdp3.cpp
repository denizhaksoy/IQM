/*
 * Image Quality Metrics - HDR-VDP-3 Test Wrapper
 * Petr Volf - 2026
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <string>
#include <algorithm>
#include "hdrvdp3.h"
#include "../../shared/debug_utils.h"
#include "../../shared/vulkan_res.h"

namespace IQM::Bin {

using VulkanInstance = IQM::VulkanInstance;

// Reference filter coefficients from sp0Filters.m
// MATLAB stores matrices in column-major, but the .m file writes them row-by-row
// When MATLAB does reshape(filt, N, N), it fills column by column
// Our shader expects row-major: filter_coeffs[fy * filter_size + fx]
// So we need to TRANSPOSE the filter from MATLAB's column-major to row-major

// Helper to transpose filter from column-major (MATLAB) to row-major (shader)
static std::vector<float> transpose_filter(const float* matlab_colmajor, int N) {
    std::vector<float> rowmajor(N * N);
    for (int row = 0; row < N; ++row) {
        for (int col = 0; col < N; ++col) {
            // MATLAB column-major: index = col * N + row
            // Our row-major: index = row * N + col
            rowmajor[row * N + col] = matlab_colmajor[col * N + row];
        }
    }
    return rowmajor;
}

// lo0filt: 7x7 - as written in sp0Filters.m (row-by-row in source = MATLAB column-major when reshaped)
static const float lo0filt_matlab[49] = {
    -4.514000e-04f, -1.137100e-04f, -3.725800e-04f, -3.743860e-03f, -3.725800e-04f, -1.137100e-04f, -4.514000e-04f,
    -1.137100e-04f, -6.119520e-03f, -1.344160e-02f, -7.563200e-03f, -1.344160e-02f, -6.119520e-03f, -1.137100e-04f,
    -3.725800e-04f, -1.344160e-02f,  6.441488e-02f,  1.524935e-01f,  6.441488e-02f, -1.344160e-02f, -3.725800e-04f,
    -3.743860e-03f, -7.563200e-03f,  1.524935e-01f,  3.153017e-01f,  1.524935e-01f, -7.563200e-03f, -3.743860e-03f,
    -3.725800e-04f, -1.344160e-02f,  6.441488e-02f,  1.524935e-01f,  6.441488e-02f, -1.344160e-02f, -3.725800e-04f,
    -1.137100e-04f, -6.119520e-03f, -1.344160e-02f, -7.563200e-03f, -1.344160e-02f, -6.119520e-03f, -1.137100e-04f,
    -4.514000e-04f, -1.137100e-04f, -3.725800e-04f, -3.743860e-03f, -3.725800e-04f, -1.137100e-04f, -4.514000e-04f
};

// hi0filt: 9x9
static const float hi0filt_matlab[81] = {
    5.997200e-04f, -6.068000e-05f, -3.324900e-04f, -3.325600e-04f, -2.406600e-04f, -3.325600e-04f, -3.324900e-04f, -6.068000e-05f, 5.997200e-04f,
    -6.068000e-05f, 1.263100e-04f, 4.927100e-04f, 1.459700e-04f, -3.732100e-04f, 1.459700e-04f, 4.927100e-04f, 1.263100e-04f, -6.068000e-05f,
    -3.324900e-04f, 4.927100e-04f, -1.616650e-03f, -1.437358e-02f, -2.420138e-02f, -1.437358e-02f, -1.616650e-03f, 4.927100e-04f, -3.324900e-04f,
    -3.325600e-04f, 1.459700e-04f, -1.437358e-02f, -6.300923e-02f, -9.623594e-02f, -6.300923e-02f, -1.437358e-02f, 1.459700e-04f, -3.325600e-04f,
    -2.406600e-04f, -3.732100e-04f, -2.420138e-02f, -9.623594e-02f, 8.554893e-01f, -9.623594e-02f, -2.420138e-02f, -3.732100e-04f, -2.406600e-04f,
    -3.325600e-04f, 1.459700e-04f, -1.437358e-02f, -6.300923e-02f, -9.623594e-02f, -6.300923e-02f, -1.437358e-02f, 1.459700e-04f, -3.325600e-04f,
    -3.324900e-04f, 4.927100e-04f, -1.616650e-03f, -1.437358e-02f, -2.420138e-02f, -1.437358e-02f, -1.616650e-03f, 4.927100e-04f, -3.324900e-04f,
    -6.068000e-05f, 1.263100e-04f, 4.927100e-04f, 1.459700e-04f, -3.732100e-04f, 1.459700e-04f, 4.927100e-04f, 1.263100e-04f, -6.068000e-05f,
    5.997200e-04f, -6.068000e-05f, -3.324900e-04f, -3.325600e-04f, -2.406600e-04f, -3.325600e-04f, -3.324900e-04f, -6.068000e-05f, 5.997200e-04f
};

// lofilt: 13x13
static const float lofilt_matlab[169] = {
    -2.257000e-04f, -8.064400e-04f, -5.686000e-05f, 8.741400e-04f, -1.862800e-04f, -1.031640e-03f, -1.871920e-03f, -1.031640e-03f, -1.862800e-04f, 8.741400e-04f, -5.686000e-05f, -8.064400e-04f, -2.257000e-04f,
    -8.064400e-04f, 1.417620e-03f, -1.903800e-04f, -2.449060e-03f, -4.596420e-03f, -7.006740e-03f, -6.948900e-03f, -7.006740e-03f, -4.596420e-03f, -2.449060e-03f, -1.903800e-04f, 1.417620e-03f, -8.064400e-04f,
    -5.686000e-05f, -1.903800e-04f, -3.059760e-03f, -6.401000e-03f, -6.720800e-03f, -5.236180e-03f, -3.781600e-03f, -5.236180e-03f, -6.720800e-03f, -6.401000e-03f, -3.059760e-03f, -1.903800e-04f, -5.686000e-05f,
    8.741400e-04f, -2.449060e-03f, -6.401000e-03f, -5.260020e-03f, 3.938620e-03f, 1.722078e-02f, 2.449600e-02f, 1.722078e-02f, 3.938620e-03f, -5.260020e-03f, -6.401000e-03f, -2.449060e-03f, 8.741400e-04f,
    -1.862800e-04f, -4.596420e-03f, -6.720800e-03f, 3.938620e-03f, 3.220744e-02f, 6.306262e-02f, 7.624674e-02f, 6.306262e-02f, 3.220744e-02f, 3.938620e-03f, -6.720800e-03f, -4.596420e-03f, -1.862800e-04f,
    -1.031640e-03f, -7.006740e-03f, -5.236180e-03f, 1.722078e-02f, 6.306262e-02f, 1.116388e-01f, 1.348999e-01f, 1.116388e-01f, 6.306262e-02f, 1.722078e-02f, -5.236180e-03f, -7.006740e-03f, -1.031640e-03f,
    -1.871920e-03f, -6.948900e-03f, -3.781600e-03f, 2.449600e-02f, 7.624674e-02f, 1.348999e-01f, 1.576508e-01f, 1.348999e-01f, 7.624674e-02f, 2.449600e-02f, -3.781600e-03f, -6.948900e-03f, -1.871920e-03f,
    -1.031640e-03f, -7.006740e-03f, -5.236180e-03f, 1.722078e-02f, 6.306262e-02f, 1.116388e-01f, 1.348999e-01f, 1.116388e-01f, 6.306262e-02f, 1.722078e-02f, -5.236180e-03f, -7.006740e-03f, -1.031640e-03f,
    -1.862800e-04f, -4.596420e-03f, -6.720800e-03f, 3.938620e-03f, 3.220744e-02f, 6.306262e-02f, 7.624674e-02f, 6.306262e-02f, 3.220744e-02f, 3.938620e-03f, -6.720800e-03f, -4.596420e-03f, -1.862800e-04f,
    8.741400e-04f, -2.449060e-03f, -6.401000e-03f, -5.260020e-03f, 3.938620e-03f, 1.722078e-02f, 2.449600e-02f, 1.722078e-02f, 3.938620e-03f, -5.260020e-03f, -6.401000e-03f, -2.449060e-03f, 8.741400e-04f,
    -5.686000e-05f, -1.903800e-04f, -3.059760e-03f, -6.401000e-03f, -6.720800e-03f, -5.236180e-03f, -3.781600e-03f, -5.236180e-03f, -6.720800e-03f, -6.401000e-03f, -3.059760e-03f, -1.903800e-04f, -5.686000e-05f,
    -8.064400e-04f, 1.417620e-03f, -1.903800e-04f, -2.449060e-03f, -4.596420e-03f, -7.006740e-03f, -6.948900e-03f, -7.006740e-03f, -4.596420e-03f, -2.449060e-03f, -1.903800e-04f, 1.417620e-03f, -8.064400e-04f,
    -2.257000e-04f, -8.064400e-04f, -5.686000e-05f, 8.741400e-04f, -1.862800e-04f, -1.031640e-03f, -1.871920e-03f, -1.031640e-03f, -1.862800e-04f, 8.741400e-04f, -5.686000e-05f, -8.064400e-04f, -2.257000e-04f
};

// bfilts: 9x9, first orientation (81 values = first column of 81x8 matrix after transpose)
// The source writes 8 rows of 81 values, then transposes. First column = first row of source.
static const float bfilt1_matlab[81] = {
    -9.066000e-05f, -1.738640e-03f, -4.942500e-03f, -7.889390e-03f, -1.009473e-02f, -7.889390e-03f, -4.942500e-03f, -1.738640e-03f, -9.066000e-05f,
    -1.738640e-03f, -4.625150e-03f, -7.272540e-03f, -7.623410e-03f, -9.091950e-03f, -7.623410e-03f, -7.272540e-03f, -4.625150e-03f, -1.738640e-03f,
    -4.942500e-03f, -7.272540e-03f, -2.129540e-02f, -2.435662e-02f, -3.487008e-02f, -2.435662e-02f, -2.129540e-02f, -7.272540e-03f, -4.942500e-03f,
    -7.889390e-03f, -7.623410e-03f, -2.435662e-02f, -1.730466e-02f, -3.158605e-02f, -1.730466e-02f, -2.435662e-02f, -7.623410e-03f, -7.889390e-03f,
    -1.009473e-02f, -9.091950e-03f, -3.487008e-02f, -3.158605e-02f, 9.464195e-01f, -3.158605e-02f, -3.487008e-02f, -9.091950e-03f, -1.009473e-02f,
    -7.889390e-03f, -7.623410e-03f, -2.435662e-02f, -1.730466e-02f, -3.158605e-02f, -1.730466e-02f, -2.435662e-02f, -7.623410e-03f, -7.889390e-03f,
    -4.942500e-03f, -7.272540e-03f, -2.129540e-02f, -2.435662e-02f, -3.487008e-02f, -2.435662e-02f, -2.129540e-02f, -7.272540e-03f, -4.942500e-03f,
    -1.738640e-03f, -4.625150e-03f, -7.272540e-03f, -7.623410e-03f, -9.091950e-03f, -7.623410e-03f, -7.272540e-03f, -4.625150e-03f, -1.738640e-03f,
    -9.066000e-05f, -1.738640e-03f, -4.942500e-03f, -7.889390e-03f, -1.009473e-02f, -7.889390e-03f, -4.942500e-03f, -1.738640e-03f, -9.066000e-05f
};

// Pre-transposed filters for shader (row-major)
static const std::vector<float> lo0filt_shader = transpose_filter(lo0filt_matlab, 7);
static const std::vector<float> hi0filt_shader = transpose_filter(hi0filt_matlab, 9);
static const std::vector<float> lofilt_shader = transpose_filter(lofilt_matlab, 13);
static const std::vector<float> bfilt1_shader = transpose_filter(bfilt1_matlab, 9);

// Test input generator - matches MATLAB: reshape(single(1:N), W, H) / N
static std::vector<float> generate_test_input(unsigned width, unsigned height) {
    std::vector<float> data(width * height);
    float N = float(width * height);
    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            // MATLAB column-major: index = x * height + y
            data[y * width + x] = float(x * height + y + 1) / N;
        }
    }
    return data;
}

// Load reference output from Octave-generated file
static std::vector<float> load_reference(const std::string& filename, int& out_rows, int& out_cols) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open reference file: " + filename);
    }
    file >> out_rows >> out_cols;
    std::vector<float> data(out_rows * out_cols);
    for (int i = 0; i < out_rows * out_cols; ++i) {
        file >> data[i];
    }
    return data;
}

static std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> create_filter_buffer(
    const VulkanInstance& instance,
    const float* filter_coeffs,
    int filter_size)
{
    constexpr int MAX_FILTER_SIZE = 169;
    constexpr unsigned bufferSize = MAX_FILTER_SIZE * sizeof(float);
    
    auto [buffer, memory] = VulkanResource::createBuffer(
        *instance.device(),
        *instance.physicalDevice(),
        bufferSize,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    
    buffer.bindMemory(memory, 0);
    
    void* data = memory.mapMemory(0, bufferSize, {});
    std::memset(data, 0, bufferSize);
    std::memcpy(data, filter_coeffs, filter_size * filter_size * sizeof(float));
    memory.unmapMemory();
    
    return {std::move(buffer), std::move(memory)};
}

HDRVDP3TestResources hdrvdp3_test_init_res(unsigned width, unsigned height, const VulkanInstance& instance) {
    HDRVDP3TestResources res;

    vk::ImageCreateInfo imageInfo = {
        .imageType = vk::ImageType::e2D,
        .format = vk::Format::eR32Sfloat,
        .extent = {width, height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst,
        .initialLayout = vk::ImageLayout::eUndefined
    };

    res.imageInput = std::make_shared<VulkanImage>(VulkanResource::createImage(*instance.device(), *instance.physicalDevice(), imageInfo));
    res.imageOutput = std::make_shared<VulkanImage>(VulkanResource::createImage(*instance.device(), *instance.physicalDevice(), imageInfo));
    res.imageInput->width = width;
    res.imageInput->height = height;
    res.imageOutput->width = width;
    res.imageOutput->height = height;

    constexpr vk::DeviceSize bufferSize = 32 * 32 * sizeof(float);
    auto [stgInBuf, stgInMem] = VulkanResource::createBuffer(
        *instance.device(), *instance.physicalDevice(),
        static_cast<unsigned>(bufferSize),
        vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    stgInBuf.bindMemory(stgInMem, 0);
    res.stgInput = std::move(stgInBuf);
    res.stgInputMemory = std::move(stgInMem);

    auto [stgOutBuf, stgOutMem] = VulkanResource::createBuffer(
        *instance.device(), *instance.physicalDevice(),
        static_cast<unsigned>(bufferSize),
        vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    stgOutBuf.bindMemory(stgOutMem, 0);
    res.stgOutput = std::move(stgOutBuf);
    res.stgOutputMemory = std::move(stgOutMem);

    res.uploadDone = instance.device()->createSemaphore(vk::SemaphoreCreateInfo{});
    res.computeDone = instance.device()->createSemaphore(vk::SemaphoreCreateInfo{});
    res.transferFence = vk::raii::Fence(*instance.device(), vk::FenceCreateInfo{});

    return res;
}

void hdrvdp3_test_upload(const VulkanInstance& instance, const HDRVDP3TestResources& res, const std::vector<float>& inputData) {
    vk::DeviceSize dataSize = inputData.size() * sizeof(float);

    void* mapped = res.stgInputMemory.mapMemory(0, dataSize, {});
    std::memcpy(mapped, inputData.data(), static_cast<size_t>(dataSize));
    res.stgInputMemory.unmapMemory();

    const vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };
    instance.cmdBufTransfer()->reset();
    instance.cmdBufTransfer()->begin(beginInfo);

    // Initialize images: transition from UNDEFINED to GENERAL
    VulkanResource::initImages(*instance.cmdBufTransfer(), {res.imageInput, res.imageOutput});

    vk::BufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .imageOffset = {0, 0, 0},
        .imageExtent = {res.imageInput->width, res.imageInput->height, 1}
    };
    // Copy directly to GENERAL layout (compute shader will read from GENERAL)
    instance.cmdBufTransfer()->copyBufferToImage(res.stgInput, *res.imageInput->image, vk::ImageLayout::eGeneral, region);

    instance.cmdBufTransfer()->end();

    vk::CommandBuffer cmdBufTransfer = *instance.cmdBufTransfer();
    vk::SubmitInfo submitInfo = {
        .sType = vk::StructureType::eSubmitInfo,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdBufTransfer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*res.uploadDone
    };
    vk::raii::Fence uploadFence(*instance.device(), vk::FenceCreateInfo{});
    instance.queueTransfer()->submit(submitInfo, uploadFence);
    instance.waitForFence(uploadFence);
}

HDRVDP3TestResult hdrvdp3_test_copy_back(const VulkanInstance& instance, const HDRVDP3TestResources& res, unsigned outWidth, unsigned outHeight) {
    HDRVDP3TestResult result;
    result.outputData.resize(outWidth * outHeight);

    const vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };
    instance.cmdBufTransfer()->reset();
    instance.cmdBufTransfer()->begin(beginInfo);

    vk::BufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .imageOffset = {0, 0, 0},
        .imageExtent = {outWidth, outHeight, 1}
    };
    // Copy directly from GENERAL layout (compute shader wrote to GENERAL)
    instance.cmdBufTransfer()->copyImageToBuffer(*res.imageOutput->image, vk::ImageLayout::eGeneral, res.stgOutput, region);

    instance.cmdBufTransfer()->end();

    vk::CommandBuffer cmdBufTransfer = *instance.cmdBufTransfer();
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eTransfer;
    vk::SubmitInfo submitInfo = {
        .sType = vk::StructureType::eSubmitInfo,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*res.computeDone,
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdBufTransfer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr
    };
    vk::raii::Fence copyBackFence(*instance.device(), vk::FenceCreateInfo{});
    instance.queueTransfer()->submit(submitInfo, copyBackFence);
    instance.waitForFence(copyBackFence);

    void* mapped = res.stgOutputMemory.mapMemory(0, outWidth * outHeight * sizeof(float), {});
    std::memcpy(result.outputData.data(), mapped, outWidth * outHeight * sizeof(float));
    res.stgOutputMemory.unmapMemory();

    return result;
}

// Run single corrDn test and compare with reference
static void run_corrdn_test(
    const VulkanInstance& instance,
    IQM::HDRVDP3::CorrDn& corrdn,
    const std::string& filter_name,
    const std::vector<float>& filter_shader,
    int filter_size,
    const std::vector<float>& input_data,
    unsigned width, unsigned height,
    int step,
    const std::string& ref_filename)
{
    const unsigned outWidth = (width + step - 1) / step;
    const unsigned outHeight = (height + step - 1) / step;
    
    // Create filter buffer
    auto [filterBuffer, filterMemory] = create_filter_buffer(instance, filter_shader.data(), filter_size);
    
    // Initialize resources
    auto res = hdrvdp3_test_init_res(width, height, instance);
    hdrvdp3_test_upload(instance, res, input_data);
    
    auto corrdnArgs = IQM::HDRVDP3::CorrDnInput {
        .device = instance.device(),
        .cmdBuf = &*instance.cmdBuf(),
        .ivInput = &res.imageInput->imageView,
        .ivOutput = &res.imageOutput->imageView,
        .filterBuffer = &filterBuffer,
        .width = outWidth,
        .height = outHeight,
        .filter_size = filter_size,
        .step_x = step,
        .step_y = step,
        .start_x = 0,
        .start_y = 0,
    };
    
    const vk::CommandBufferBeginInfo beginInfo = {
        .flags = vk::CommandBufferUsageFlags{vk::CommandBufferUsageFlagBits::eOneTimeSubmit},
    };
    
    // Create separate fences for each submission to avoid validation warnings
    vk::raii::Fence computeFence(*instance.device(), vk::FenceCreateInfo{});
    vk::raii::Fence transferFence(*instance.device(), vk::FenceCreateInfo{});
    
    instance.cmdBuf()->reset();
    instance.cmdBuf()->begin(beginInfo);
    corrdn.compute(corrdnArgs);
    instance.cmdBuf()->end();
    
    vk::CommandBuffer cmdBuf = *instance.cmdBuf();
    auto mask = vk::PipelineStageFlags{vk::PipelineStageFlagBits::eComputeShader};
    const vk::SubmitInfo submitInfo{
        .sType = vk::StructureType::eSubmitInfo,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*res.uploadDone,
        .pWaitDstStageMask = &mask,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdBuf,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*res.computeDone
    };
    
    instance.queue()->submit(submitInfo, computeFence);
    instance.waitForFence(computeFence);
    
    auto result = hdrvdp3_test_copy_back(instance, res, outWidth, outHeight);
    
    // Load reference and compare
    int ref_rows, ref_cols;
    auto ref_data = load_reference(ref_filename, ref_rows, ref_cols);
    
    if (ref_rows != static_cast<int>(outHeight) || ref_cols != static_cast<int>(outWidth)) {
        std::cout << "  WARNING: Size mismatch! GPU: " << outWidth << "x" << outHeight 
                  << " Ref: " << ref_cols << "x" << ref_rows << std::endl;
    }
    
    float max_abs_error = 0.0f;
    float rmse_sum = 0.0f;
    int compare_count = std::min(static_cast<int>(result.outputData.size()), static_cast<int>(ref_data.size()));
    
    for (int i = 0; i < compare_count; ++i) {
        float error = std::abs(result.outputData[i] - ref_data[i]);
        max_abs_error = std::max(max_abs_error, error);
        float diff = result.outputData[i] - ref_data[i];
        rmse_sum += diff * diff;
    }
    float rmse = std::sqrt(rmse_sum / compare_count);
    
    std::cout << "  " << filter_name << " step=" << step << ": " << outWidth << "x" << outHeight
              << "  MaxAbsErr=" << max_abs_error << "  RMSE=" << rmse << std::endl;
    
    // Print first few values for debugging
    if (filter_name.find("lo0filt") != std::string::npos && step == 1) {
        std::cout << "  First 4x4:" << std::endl;
        for (int y = 0; y < 4 && y < static_cast<int>(outHeight); ++y) {
            std::cout << "    ";
            for (int x = 0; x < 4 && x < static_cast<int>(outWidth); ++x) {
                int idx = y * static_cast<int>(outWidth) + x;
                std::cout << " GPU=" << result.outputData[idx] << " Ref=" << ref_data[idx] 
                          << " Err=" << std::abs(result.outputData[idx] - ref_data[idx]) << "  ";
            }
            std::cout << std::endl;
        }
    }
    
    // Check corners
    if (outWidth > 0 && outHeight > 0) {
        std::cout << "  Corners: TL=" << result.outputData[0] 
                  << " TR=" << result.outputData[outWidth-1]
                  << " BL=" << result.outputData[(outHeight-1)*outWidth]
                  << " BR=" << result.outputData[(outHeight-1)*outWidth + outWidth-1] << std::endl;
    }
}

void hdrvdp3_test_run(const Args& args, const VulkanInstance& instance) {
    IQM::HDRVDP3::CorrDn corrdn(*instance.device());
    
    const unsigned width = 32;
    const unsigned height = 32;
    auto input_data = generate_test_input(width, height);
    
    std::cout << "==================================================" << std::endl;
    std::cout << "HDR-VDP-3 corrDn Numerical Validation" << std::endl;
    std::cout << "Input: " << width << "x" << height << " (MATLAB-compatible)" << std::endl;
    std::cout << "Reference: C:/temp/ref_*.txt (from Octave)" << std::endl;
    std::cout << "==================================================" << std::endl;
    
    struct FilterTest {
        const char* name;
        const std::vector<float>* filter_shader;
        int filter_size;
        const char* ref_prefix;
    };
    
    FilterTest filters[] = {
        {"lo0filt (7x7)", &lo0filt_shader, 7, "C:/temp/ref_lo0filt_7x7"},
        {"hi0filt (9x9)", &hi0filt_shader, 9, "C:/temp/ref_hi0filt_9x9"},
        {"lofilt (13x13)", &lofilt_shader, 13, "C:/temp/ref_lofilt_13x13"},
        {"bfilt orient1 (9x9)", &bfilt1_shader, 9, "C:/temp/ref_bfilt_9x9_orient1"},
    };
    
    int steps[] = {1, 2};
    
    for (const auto& f : filters) {
        std::cout << "\n--- Testing " << f.name << " ---" << std::endl;
        for (int step : steps) {
            std::string ref_file = std::string(f.ref_prefix) + "_step" + std::to_string(step) + ".txt";
            try {
                run_corrdn_test(instance, corrdn, f.name, *f.filter_shader, f.filter_size,
                               input_data, width, height, step, ref_file);
            } catch (const std::exception& e) {
                std::cout << "  ERROR: " << e.what() << std::endl;
            }
        }
    }
    
    std::cout << "\n==================================================" << std::endl;
    std::cout << "Validation complete." << std::endl;
    std::cout << "==================================================" << std::endl;
}

} // namespace IQM::Bin