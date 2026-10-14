/*
 * Image Quality Metrics - HDR-VDP-3 Test Wrapper
 * Deniz Aksoy - 2026
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <string>
#include <algorithm>
#include "hdrvdp3.h"
#include <IQM/hdrvdp3/csf.h>
#include "../../shared/debug_utils.h"
#include "../../shared/vulkan_res.h"
#include <IQM/hdrvdp3/spyr.h>
#include <IQM/hdrvdp3/jnd.h>

namespace IQM::Bin {

using VulkanInstance = IQM::VulkanInstance;
using Spyr = IQM::HDRVDP3::Spyr;

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

// CPU reference implementation of corrDn matching Octave's rconv2 + corrDn with reflect1 boundary
// Filter is in MATLAB .m file format (row-by-row, row-major linear), NOT transposed.
// Octave's corrDn flips the filter, then conv2 flips again -> net: correlation with original filter.
static std::vector<float> cpu_corrdn_ref(
    const std::vector<float>& input,
    unsigned width, unsigned height,
    const float* matlab_mfile_filter,  // .m file format (row-by-row, row-major)
    int filter_size,
    int step_x, int step_y,
    int start_x, int start_y)
{
    // Octave's corrDn: filt = filt(end:-1:1, end:-1:1); then conv2 (which flips again)
    // Net effect: correlation with ORIGINAL filter (no flip needed here)
    // Use the .m file filter directly (row-major linear: row * N + col)

    // Step 1: Reflect1 padding (matching Octave's rconv2.m exactly)
    // pad = floor((filter_size + ctr - 1)/2), ctr=0 default
    int pad_y = (filter_size - 1) / 2;
    int pad_x = (filter_size - 1) / 2;
    
    unsigned padded_h = height + 2 * pad_y;
    unsigned padded_w = width + 2 * pad_x;
    std::vector<float> padded(padded_h * padded_w);

    // Fill padded image with reflect1 boundary handling (matching rconv2.m indexing)
    for (unsigned py = 0; py < padded_h; ++py) {
        for (unsigned px = 0; px < padded_w; ++px) {
            int iy = static_cast<int>(py) - pad_y;
            int ix = static_cast<int>(px) - pad_x;
            
            // Reflect1: reflect about edge pixels (index 0 and height-1)
            if (iy < 0) iy = -iy;
            if (ix < 0) ix = -ix;
            if (iy >= static_cast<int>(height)) iy = 2 * (height - 1) - iy;
            if (ix >= static_cast<int>(width)) ix = 2 * (width - 1) - ix;
            
            // Handle multiple reflections for large filters
            while (iy < 0 || iy >= static_cast<int>(height) || ix < 0 || ix >= static_cast<int>(width)) {
                if (iy < 0) iy = -iy;
                if (iy >= static_cast<int>(height)) iy = 2 * (height - 1) - iy;
                if (ix < 0) ix = -ix;
                if (ix >= static_cast<int>(width)) ix = 2 * (width - 1) - ix;
            }
            
            padded[py * padded_w + px] = input[iy * width + ix];
        }
    }

    // Step 2: Correlation with ORIGINAL filter (no flip - Octave flips then conv2 flips)
    // Filter is in .m file format: row-major linear [row * N + col]
    std::vector<float> corr_result(height * width);
    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            float acc = 0.0f;
            for (int fy = 0; fy < filter_size; ++fy) {
                for (int fx = 0; fx < filter_size; ++fx) {
                    unsigned py = y + fy;
                    unsigned px = x + fx;
                    // Filter index: row-major .m file format
                    float coeff = matlab_mfile_filter[fy * filter_size + fx];
                    acc += padded[py * padded_w + px] * coeff;
                }
            }
            corr_result[y * width + x] = acc;
        }
    }

    // Step 3: Downsample with step (matching corrDn.m: start(1):step(1):stop(1))
    unsigned out_h = (height - start_y + step_y - 1) / step_y;
    unsigned out_w = (width - start_x + step_x - 1) / step_x;
    std::vector<float> output(out_h * out_w);
    
    for (unsigned oy = 0; oy < out_h; ++oy) {
        for (unsigned ox = 0; ox < out_w; ++ox) {
            unsigned iy = start_y + oy * step_y;
            unsigned ix = start_x + ox * step_x;
            if (iy < height && ix < width) {
                output[oy * out_w + ox] = corr_result[iy * width + ix];
            }
        }
    }
    
    return output;
}

// Save reference output to file (for generating Octave references)
static void save_reference(const std::string& filename, const std::vector<float>& data, unsigned rows, unsigned cols) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open reference file for writing: " + filename);
    }
    file << rows << " " << cols << "\n";
    for (unsigned i = 0; i < data.size(); ++i) {
        file << data[i] << "\n";
    }
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

    constexpr vk::DeviceSize bufferSize = 64 * 64 * sizeof(float);  // Support up to 64x64
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

// Helper to run corrDn on GPU and return output data
static std::vector<float> run_corrdn_gpu(
    const VulkanInstance& instance,
    IQM::HDRVDP3::CorrDn& corrdn,
    const float* filter_coeffs,
    int filter_size,
    const std::vector<float>& input_data,
    unsigned width, unsigned height,
    int step)
{
    const unsigned outWidth = (width + step - 1) / step;
    const unsigned outHeight = (height + step - 1) / step;
    
    // Create filter buffer
    auto [filterBuffer, filterMemory] = create_filter_buffer(instance, filter_coeffs, filter_size);
    
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
    
    vk::raii::Fence computeFence(*instance.device(), vk::FenceCreateInfo{});
    
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
    return result.outputData;
}

// Helper to compare GPU and CPU results
static void compare_results(
    const std::string& name,
    const std::vector<float>& gpu_data,
    const std::vector<float>& cpu_data,
    unsigned out_w, unsigned out_h)
{
    if (gpu_data.size() != cpu_data.size()) {
        std::cout << "  WARNING: Size mismatch! GPU: " << gpu_data.size() 
                  << " CPU: " << cpu_data.size() << std::endl;
    }
    
    int compare_count = std::min(static_cast<int>(gpu_data.size()), static_cast<int>(cpu_data.size()));
    float max_abs_error = 0.0f;
    float rmse_sum = 0.0f;
    
    for (int i = 0; i < compare_count; ++i) {
        float error = std::abs(gpu_data[i] - cpu_data[i]);
        max_abs_error = std::max(max_abs_error, error);
        float diff = gpu_data[i] - cpu_data[i];
        rmse_sum += diff * diff;
    }
    float rmse = std::sqrt(rmse_sum / compare_count);
    
    std::cout << "  " << name << ": " << out_w << "x" << out_h
              << "  MaxAbsErr=" << max_abs_error << "  RMSE=" << rmse << std::endl;
    
    // Print corners
    if (out_w > 0 && out_h > 0 && compare_count > 0) {
        std::cout << "  Corners: TL GPU=" << gpu_data[0] << " CPU=" << cpu_data[0]
                  << " TR GPU=" << gpu_data[out_w-1] << " CPU=" << cpu_data[out_w-1]
                  << " BL GPU=" << gpu_data[(out_h-1)*out_w] << " CPU=" << cpu_data[(out_h-1)*out_w]
                  << " BR GPU=" << gpu_data[(out_h-1)*out_w + out_w-1] 
                  << " CPU=" << cpu_data[(out_h-1)*out_w + out_w-1] << std::endl;
    }
}

// Run steerable pyramid first level test - compare GPU results directly against Octave references
static void run_spyr_level1_test(
    const VulkanInstance& instance,
    IQM::HDRVDP3::CorrDn& corrdn,
    const std::vector<float>& input_data,
    unsigned width, unsigned height)
{
    std::cout << "\n==================================================" << std::endl;
    std::cout << "HDR-VDP-3 Steerable Pyramid Level 1 Validation" << std::endl;
    std::cout << "Input: " << width << "x" << height << " (MATLAB-compatible)" << std::endl;
    std::cout << "Reference: Official Octave implementation (C:/HDR-VDP-3_Ref)" << std::endl;
    std::cout << "==================================================" << std::endl;

    // Helper to load Octave reference and transpose (Octave is column-major, GPU is row-major)
    auto load_octave_ref_transposed = [&](const std::string& filename, unsigned& out_w, unsigned& out_h) {
        int ref_rows, ref_cols;
        auto data = load_reference(filename, ref_rows, ref_cols);
        // Octave stores column-major, GPU expects row-major -> transpose
        // Octave ref_rows x ref_cols -> GPU ref_cols x ref_rows
        std::vector<float> transposed(ref_cols * ref_rows);
        for (int r = 0; r < ref_rows; ++r) {
            for (int c = 0; c < ref_cols; ++c) {
                transposed[c * ref_rows + r] = data[r * ref_cols + c];
            }
        }
        out_w = ref_rows;
        out_h = ref_cols;
        return transposed;
    };

    std::cout << "\n==================================================" << std::endl;
    std::cout << "HDR-VDP-3 Steerable Pyramid Level 1 Validation" << std::endl;
    std::cout << "Input: " << width << "x" << height << " (MATLAB-compatible)" << std::endl;
    std::cout << "Reference: Official Octave implementation (C:/HDR-VDP-3_Ref)" << std::endl;
    std::cout << "==================================================" << std::endl;

    // Step 1: Compute initial lowpass (lo0filt) and highpass (hi0filt)
    std::cout << "\n--- Computing initial lowpass (lo0filt) ---" << std::endl;
    auto lo0_gpu = run_corrdn_gpu(instance, corrdn, lo0filt_shader.data(), 7, input_data, width, height, 1);
    unsigned ref_w, ref_h;
    auto oct_lowpass = load_octave_ref_transposed("C:/temp/oct_lowpass.txt", ref_w, ref_h);
    compare_results("lo0filt (initial lowpass) vs Octave", lo0_gpu, oct_lowpass, ref_w, ref_h);

    std::cout << "\n--- Computing initial highpass (hi0filt) ---" << std::endl;
    auto hi0_gpu = run_corrdn_gpu(instance, corrdn, hi0filt_shader.data(), 9, input_data, width, height, 1);
    auto oct_highpass = load_octave_ref_transposed("C:/temp/oct_highpass.txt", ref_w, ref_h);
    compare_results("hi0filt (initial highpass) vs Octave", hi0_gpu, oct_highpass, ref_w, ref_h);

    // Step 2: Compute first orientation band (bfilt orient1 on lowpass)
    std::cout << "\n--- Computing first orientation band (bfilt orient1 on lowpass) ---" << std::endl;
    auto bfilt1_gpu = run_corrdn_gpu(instance, corrdn, bfilt1_shader.data(), 9, lo0_gpu, 32, 32, 1);
    auto oct_orient1 = load_octave_ref_transposed("C:/temp/oct_orient1.txt", ref_w, ref_h);
    compare_results("bfilt orient1 (on lowpass) vs Octave", bfilt1_gpu, oct_orient1, ref_w, ref_h);

    // Step 3: Compute downsampled lowpass (lofilt with step=2 on lowpass)
    std::cout << "\n--- Computing downsampled lowpass (lofilt step=2 on lowpass) ---" << std::endl;
    auto lo1_gpu = run_corrdn_gpu(instance, corrdn, lofilt_shader.data(), 13, lo0_gpu, 32, 32, 2);
    auto oct_ds_lowpass = load_octave_ref_transposed("C:/temp/oct_ds_lowpass.txt", ref_w, ref_h);
    compare_results("lofilt step=2 (downsampled lowpass) vs Octave", lo1_gpu, oct_ds_lowpass, ref_w, ref_h);

    std::cout << "\n==================================================" << std::endl;
    std::cout << "Steerable Pyramid Level 1 Validation Complete" << std::endl;
    std::cout << "==================================================" << std::endl;
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
    
    // First, verify CPU reference matches Octave reference files
    std::cout << "\n--- Verifying CPU reference against Octave references ---" << std::endl;
    {
        struct OctaveRefTest {
            const char* name;
            const float* matlab_filter;
            int filter_size;
            const char* ref_file;
            int step;
        };
        
        OctaveRefTest octave_tests[] = {
            {"lo0filt (7x7) step=1", lo0filt_matlab, 7, "C:/temp/ref_lo0filt_7x7_step1.txt", 1},
            {"hi0filt (9x9) step=1", hi0filt_matlab, 9, "C:/temp/ref_hi0filt_9x9_step1.txt", 1},
            {"lofilt (13x13) step=1", lofilt_matlab, 13, "C:/temp/ref_lofilt_13x13_step1.txt", 1},
            {"bfilt orient1 (9x9) step=1", bfilt1_matlab, 9, "C:/temp/ref_bfilt_9x9_orient1_step1.txt", 1},
        };
        
        bool all_cpu_verified = true;
        for (const auto& t : octave_tests) {
            int ref_rows, ref_cols;
            auto octave_ref = load_reference(t.ref_file, ref_rows, ref_cols);
            auto cpu_ref = cpu_corrdn_ref(input_data, width, height, t.matlab_filter, t.filter_size, t.step, t.step, 0, 0);
            
            int compare_count = std::min(static_cast<int>(cpu_ref.size()), static_cast<int>(octave_ref.size()));
            float max_abs_error = 0.0f;
            float rmse_sum = 0.0f;
            for (int i = 0; i < compare_count; ++i) {
                float error = std::abs(cpu_ref[i] - octave_ref[i]);
                max_abs_error = std::max(max_abs_error, error);
                float diff = cpu_ref[i] - octave_ref[i];
                rmse_sum += diff * diff;
            }
            float rmse = std::sqrt(rmse_sum / compare_count);
            
            std::cout << "  CPU vs Octave " << t.name << ": " << ref_cols << "x" << ref_rows
                      << "  MaxAbsErr=" << max_abs_error << "  RMSE=" << rmse;
            if (max_abs_error > 1e-6f) {
                std::cout << "  *** MISMATCH ***";
                all_cpu_verified = false;
            }
            std::cout << std::endl;
        }
        
        if (all_cpu_verified) {
            std::cout << "  CPU reference VERIFIED against Octave references." << std::endl;
        } else {
            std::cout << "  WARNING: CPU reference does not match Octave!" << std::endl;
        }
    }
    
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
    
    // Run steerable pyramid first level validation
    try {
        run_spyr_level1_test(instance, corrdn, input_data, width, height);
    } catch (const std::exception& e) {
        std::cout << "  ERROR in steerable pyramid test: " << e.what() << std::endl;
    }
// Run full steerable pyramid test using Spyr class (64x64)
    std::cout << "\n==================================================" << std::endl;
    std::cout << "HDR-VDP-3 Full Steerable Pyramid Validation (64x64)" << std::endl;
    std::cout << "==================================================" << std::endl;
    
    try {
        // Generate 64x64 test input
        const unsigned width64 = 64;
        const unsigned height64 = 64;
        auto input_data_64 = generate_test_input(width64, height64);

        // Synthetic reference/test pair for HDR-VDP-3 integration.
        const auto reference_image = input_data_64;

        auto test_image = reference_image;

        // Apply a controlled 5% luminance increase.
        for (auto& pixel : test_image) {
            pixel *= 1.05f;
        }

        std::cout << "Reference first pixel: "
                << reference_image.front() << std::endl;

        std::cout << "Test first pixel: "
                << test_image.front() << std::endl;

        // Compute local adaptation from the same luminance image
        // used by the JND -> SPYR pipeline.
        auto reference_adaptation = compute_local_adaptation_gpu(
            instance,
            reference_image,
            30.0f
        );

        auto test_adaptation = compute_local_adaptation_gpu(
            instance,
            test_image,
            30.0f
        );

        std::vector<float> adaptation_map(reference_adaptation.size());

        if (reference_adaptation.size() != test_adaptation.size()) {
            throw std::runtime_error("Adaptation map dimensions mismatch");
        }

        for (std::size_t i = 0; i < adaptation_map.size(); ++i) {
            adaptation_map[i] = 0.5f * (
                reference_adaptation[i] + test_adaptation[i]
            );
        }

        std::cout << "\n--- HDR-VDP-3 Integration ---" << std::endl;
        std::cout << "Local adaptation pixels: "
                << adaptation_map.size() << std::endl;

        std::cout << "First input luminance: "
                << input_data_64.front() << " cd/m^2" << std::endl;

        std::cout << "First adapted luminance: "
                << adaptation_map.front() << " cd/m^2" << std::endl;
        
        // CSF integration: first steerable pyramid band
        const double ppd = 30.0;
        const double band_frequency =
            IQM::HDRVDP3::compute_band_frequency(1, ppd);

        const auto csf_lut =
            IQM::HDRVDP3::create_csf_lut(band_frequency);

        const double first_csf =
            IQM::HDRVDP3::sample_csf_lut(
                csf_lut,
                static_cast<double>(adaptation_map.front())
            );

        std::cout << "Band 1 frequency: "
                << band_frequency << " cycles/degree" << std::endl;

        std::cout << "First pixel CSF sensitivity: "
                << first_csf << std::endl;
        
        // Convert the luminance image into photoreceptor JND responses.
// The input must represent absolute luminance in cd/m^2.
const auto jnd_lut = IQM::HDRVDP3::create_jnd_lut();

std::vector<float> jnd_image(input_data_64.size());

for (std::size_t i = 0; i < input_data_64.size(); ++i) {
    jnd_image[i] = static_cast<float>(
        IQM::HDRVDP3::luminance_to_jnd_response(
            jnd_lut,
            static_cast<double>(input_data_64[i])
        )
    );
}

std::cout << "JND transformed first pixel: "
          << jnd_image.front() << std::endl;
          save_reference("C:/temp/jnd_image_64.txt", jnd_image, height64, width64);

        // Create Spyr instance with sp0Filters
        IQM::HDRVDP3::Spyr::Filters filters = {
            .lo0filt = lo0filt_matlab,
            .hi0filt = hi0filt_matlab,
            .lofilt = lofilt_matlab,
            .bfilts = bfilt1_matlab,  // Only first orientation (81 values)
            .num_orientations = 1
        };
        
        Spyr spyr(*instance.device(), filters);
        
        // Compute pyramid using Spyr class with run_corrdn_gpu as the callback
        std::cout << "\n--- Computing full steerable pyramid (64x64) ---" << std::endl;
        auto result = spyr.compute(jnd_image, width64, height64, instance,
                                   [&](const IQM::VulkanInstance& inst, IQM::HDRVDP3::CorrDn& cdn,
                                       const float* filter_coeffs, int filter_size,
                                       const std::vector<float>& input_data,
                                       unsigned w, unsigned h, int step) {
            return run_corrdn_gpu(inst, cdn, filter_coeffs, filter_size, input_data, w, h, step);
        });

                // Convert test image to JND responses.
        std::vector<float> test_jnd_image(test_image.size());

        for (std::size_t i = 0; i < test_image.size(); ++i) {
            test_jnd_image[i] = static_cast<float>(
                IQM::HDRVDP3::luminance_to_jnd_response(
                    jnd_lut,
                    static_cast<double>(test_image[i])
                )
            );
        }

        // Compute SPYR for the test image.
        auto test_result = spyr.compute(
            test_jnd_image,
            width64,
            height64,
            instance,
            [&](const IQM::VulkanInstance& inst,
                IQM::HDRVDP3::CorrDn& cdn,
                const float* filter_coeffs,
                int filter_size,
                const std::vector<float>& input_data,
                unsigned w,
                unsigned h,
                int step) {
                return run_corrdn_gpu(
                    inst, cdn, filter_coeffs, filter_size,
                    input_data, w, h, step
                );
            }
        );

        // Remove the DC component from the residual lowpass bands.
        // Matches hdrvdp_visual_pathway.m in HDR-VDP-3.
        auto remove_residual_dc = [](auto& pyramid) {
            auto& residual = pyramid.bands.back().data;

            if (residual.empty()) {
                return;
            }

            double sum = 0.0;
            for (float value : residual) {
                sum += static_cast<double>(value);
            }

            const float mean = static_cast<float>(
                sum / static_cast<double>(residual.size())
            );

            for (float& value : residual) {
                value -= mean;
            }
        };


        // Compare reference and test SPYR bands.
        std::cout << "\n--- Reference vs Test SPYR ---" << std::endl;

        for (std::size_t b = 0; b < result.bands.size(); ++b) {
            const auto& ref_band = result.bands[b];
            const auto& tst_band = test_result.bands[b];

            if (ref_band.width != tst_band.width ||
                ref_band.height != tst_band.height ||
                ref_band.data.size() != tst_band.data.size()) {
                throw std::runtime_error("SPYR band dimensions mismatch");
            }

            double sum_abs_diff = 0.0;
            double max_abs_diff = 0.0;

            for (std::size_t i = 0; i < ref_band.data.size(); ++i) {
            const double normalization = std::ldexp(1.0, static_cast<int>(b));

            const double diff = std::abs(
                static_cast<double>(tst_band.data[i]) -
                static_cast<double>(ref_band.data[i])
            ) / normalization;

                sum_abs_diff += diff;
                max_abs_diff = std::max(max_abs_diff, diff);
            }

            const double mean_abs_diff =
                sum_abs_diff / ref_band.data.size();

            std::cout << "Band " << b
                    << ": MeanAbsDiff=" << mean_abs_diff
                    << ", MaxAbsDiff=" << max_abs_diff
                    << std::endl;
        }

        // Preserve the raw residual lowpass for Octave pyramid validation.
        const std::vector<float> raw_residual_band = result.bands.back().data;

        remove_residual_dc(result);
        remove_residual_dc(test_result);

        // Build CSF sensitivity maps for each SPYR band.
        // Integration test: one luminance image, not yet a reference/test pair.
        std::vector<std::vector<float>> csf_maps;
        csf_maps.reserve(result.bands.size());

        for (std::size_t b = 0; b < result.bands.size(); ++b) {
            const auto& band = result.bands[b];

            // Use the raw residual band for Octave validation.
            const std::vector<float>& band_data =
                (b == result.bands.size() - 1)
                    ? raw_residual_band
                    : band.data;

            const double frequency =
                IQM::HDRVDP3::compute_band_frequency(
                    static_cast<int>(b) + 1, 30.0
                );

            const auto lut = IQM::HDRVDP3::create_csf_lut(frequency);

            std::vector<float> csf_map(band.width * band.height);

            for (unsigned y = 0; y < band.height; ++y) {
                for (unsigned x = 0; x < band.width; ++x) {

                   // Resize log10(local adaptation luminance) using bilinear interpolation.
                    // TODO: Match Octave imresize antialiasing for downsampling.

                    const double source_x =
                        (static_cast<double>(x) + 0.5) * width64 / band.width - 0.5;

                    const double source_y =
                        (static_cast<double>(y) + 0.5) * height64 / band.height - 0.5;

                    const double clamped_x =
                        std::clamp(source_x, 0.0, static_cast<double>(width64 - 1));

                    const double clamped_y =
                        std::clamp(source_y, 0.0, static_cast<double>(height64 - 1));

                    const unsigned x0 = static_cast<unsigned>(std::floor(clamped_x));
                    const unsigned y0 = static_cast<unsigned>(std::floor(clamped_y));

                    const unsigned x1 = std::min(x0 + 1, width64 - 1);
                    const unsigned y1 = std::min(y0 + 1, height64 - 1);

                    const double wx = clamped_x - x0;
                    const double wy = clamped_y - y0;

                    auto log_luminance = [&](unsigned px, unsigned py) {
                        const double L = adaptation_map[py * width64 + px];

                        return std::log10(std::clamp(L, 1e-5, 1e5));
                    };

                    const double l00 = log_luminance(x0, y0);
                    const double l10 = log_luminance(x1, y0);
                    const double l01 = log_luminance(x0, y1);
                    const double l11 = log_luminance(x1, y1);

                    const double log_L =
                        (1.0 - wy) * ((1.0 - wx) * l00 + wx * l10) +
                        wy * ((1.0 - wx) * l01 + wx * l11);

                    const double luminance = std::pow(10.0, log_L);

                    csf_map[y * band.width + x] =
                        static_cast<float>(
                            IQM::HDRVDP3::sample_csf_lut(lut, luminance)
                        );
                }
            }

            std::cout << "CSF Band " << b
                    << ": " << band.width << "x" << band.height
                    << ", frequency=" << frequency
                    << " cpd, first sensitivity=" << csf_map.front()
                    << std::endl;

            csf_maps.push_back(std::move(csf_map));
        }

        std::cout << "\n--- CSF-Weighted Band Differences ---" << std::endl;

for (std::size_t b = 0; b < result.bands.size(); ++b) {
    const auto& ref_band = result.bands[b];
    const auto& tst_band = test_result.bands[b];
    const auto& csf = csf_maps[b];

    if (ref_band.data.size() != tst_band.data.size() ||
        ref_band.data.size() != csf.size()) {
        throw std::runtime_error("CSF-weighted band size mismatch");
    }

    const double normalization =
        std::ldexp(1.0, static_cast<int>(b));

    double sum_weighted_diff = 0.0;
    double max_weighted_diff = 0.0;

    for (std::size_t i = 0; i < ref_band.data.size(); ++i) {
        const double normalized_diff =
            std::abs(
                static_cast<double>(tst_band.data[i]) -
                static_cast<double>(ref_band.data[i])
            ) / normalization;

        const double weighted_diff =
            normalized_diff * static_cast<double>(csf[i]);

        sum_weighted_diff += weighted_diff;
        max_weighted_diff = std::max(max_weighted_diff, weighted_diff);
    }

    const double mean_weighted_diff =
        sum_weighted_diff / ref_band.data.size();

    std::cout << "Band " << b
              << ": MeanWeightedDiff=" << mean_weighted_diff
              << ", MaxWeightedDiff=" << max_weighted_diff
              << std::endl;
}
        
        std::cout << "\n--- Raw Mutual Masking Activity ---" << std::endl;

std::vector<std::vector<float>> mutual_masking_maps;
mutual_masking_maps.reserve(result.bands.size());

for (std::size_t b = 0; b < result.bands.size(); ++b) {
    const auto& ref_band = result.bands[b];
    const auto& tst_band = test_result.bands[b];

    if (ref_band.data.size() != tst_band.data.size()) {
        throw std::runtime_error("Mutual masking band size mismatch");
    }

    const double normalization =
        std::ldexp(1.0, static_cast<int>(b));
    
    double ref_sum_abs = 0.0;
    double ref_max_abs = 0.0;

    for (float value : ref_band.data) {
        const double normalized =
            std::abs(static_cast<double>(value)) / normalization;

        ref_sum_abs += normalized;
        ref_max_abs = std::max(ref_max_abs, normalized);
    }

    std::cout << "DEBUG Pyramid Band " << b
            << ": RefMeanAbs=" << ref_sum_abs / ref_band.data.size()
            << " RefMaxAbs=" << ref_max_abs
            << std::endl;

    std::vector<float> mask(ref_band.data.size());

    double sum_mask = 0.0;
    double max_mask = 0.0;

    for (std::size_t i = 0; i < mask.size(); ++i) {
        const double ref_value =
            std::abs(static_cast<double>(ref_band.data[i])) /
            normalization;

        const double test_value =
            std::abs(static_cast<double>(tst_band.data[i])) /
            normalization;

        const double activity = std::min(ref_value, test_value);

        mask[i] = static_cast<float>(activity);

        sum_mask += activity;
        max_mask = std::max(max_mask, activity);
    }

    std::cout << "Band " << b
              << ": MeanMask=" << sum_mask / mask.size()
              << ", MaxMask=" << max_mask
              << std::endl;

    // Default HDR-VDP-3 mutual masking pooling:
// do_si_gauss = false
// do_pu_dilate = false
// masking_norm = 0 -> p = 1
// masking_pool_size = 3
//
// Octave conv2(..., 'same') uses zero padding.

std::vector<float> pooled_mask(mask.size(), 0.0f);

for (unsigned y = 0; y < ref_band.height; ++y) {
    for (unsigned x = 0; x < ref_band.width; ++x) {

        double sum = 0.0;

        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {

                const int nx = static_cast<int>(x) + dx;
                const int ny = static_cast<int>(y) + dy;

                if (nx >= 0 &&
                    ny >= 0 &&
                    nx < static_cast<int>(ref_band.width) &&
                    ny < static_cast<int>(ref_band.height)) {

                    sum += mask[
                        static_cast<std::size_t>(ny) * ref_band.width +
                        static_cast<std::size_t>(nx)
                    ];
                }
            }
        }

        pooled_mask[y * ref_band.width + x] =
            static_cast<float>(sum / 9.0);
    }
}

        double pooled_sum = 0.0;
        double pooled_max = 0.0;

        for (float value : pooled_mask) {
            pooled_sum += value;
            pooled_max = std::max(pooled_max, static_cast<double>(value));
        }

        std::cout << "Band " << b
                << ": MeanPooledMask=" << pooled_sum / pooled_mask.size()
                << ", MaxPooledMask=" << pooled_max
                << std::endl;

        mutual_masking_maps.push_back(std::move(pooled_mask));
}

        std::cout << "\n--- HDR-VDP-3 Masking Denominator ---" << std::endl;

const double masking_q = std::pow(10.0, 0.108934275615);
const double k_mask_self = std::pow(10.0, 1.41846291721);
const double k_mask_xn = std::pow(10.0, 0.136877512403);

std::vector<std::vector<float>> n_mask_maps;
n_mask_maps.reserve(mutual_masking_maps.size());

// Resize one masking map to the dimensions of another band.
// Bilinear approximation; Octave imresize antialiasing is not yet matched.
auto resize_mask = [&](std::size_t source_band,
                       unsigned target_width,
                       unsigned target_height) {

    const auto& src = mutual_masking_maps[source_band];

    const unsigned src_width = result.bands[source_band].width;
    const unsigned src_height = result.bands[source_band].height;

    std::vector<float> resized(target_width * target_height);

    for (unsigned y = 0; y < target_height; ++y) {
        for (unsigned x = 0; x < target_width; ++x) {

            const double sx = std::clamp(
                (static_cast<double>(x) + 0.5) * src_width /
                    target_width - 0.5,
                0.0,
                static_cast<double>(src_width - 1)
            );

            const double sy = std::clamp(
                (static_cast<double>(y) + 0.5) * src_height /
                    target_height - 0.5,
                0.0,
                static_cast<double>(src_height - 1)
            );

            const unsigned x0 = static_cast<unsigned>(std::floor(sx));
            const unsigned y0 = static_cast<unsigned>(std::floor(sy));
            const unsigned x1 = std::min(x0 + 1, src_width - 1);
            const unsigned y1 = std::min(y0 + 1, src_height - 1);

            const double wx = sx - x0;
            const double wy = sy - y0;

            const double v00 = src[y0 * src_width + x0];
            const double v10 = src[y0 * src_width + x1];
            const double v01 = src[y1 * src_width + x0];
            const double v11 = src[y1 * src_width + x1];

            const double value =
                (1.0 - wy) * ((1.0 - wx) * v00 + wx * v10) +
                wy * ((1.0 - wx) * v01 + wx * v11);

            resized[y * target_width + x] =
                static_cast<float>(std::max(value, 0.0));
        }
    }

    return resized;
};

const std::size_t band_count = mutual_masking_maps.size();

for (std::size_t b = 0; b < band_count; ++b) {

    const unsigned width = result.bands[b].width;
    const unsigned height = result.bands[b].height;

    const auto& self_mask = mutual_masking_maps[b];

    std::vector<float> neighbor_mask(width * height, 0.0f);

    // Previous frequency band.
    if (b > 0) {
        const auto previous = resize_mask(b - 1, width, height);

        for (std::size_t i = 0; i < neighbor_mask.size(); ++i) {
            neighbor_mask[i] += previous[i];
        }
    }

    // Next frequency band, excluding the final lowpass band.
    if (b + 2 < band_count) {
        const auto next = resize_mask(b + 1, width, height);

        for (std::size_t i = 0; i < neighbor_mask.size(); ++i) {
            neighbor_mask[i] += next[i];
        }
    }

    std::vector<float> n_mask(width * height);

    double sum_n_mask = 0.0;
    double max_n_mask = 0.0;

    for (std::size_t i = 0; i < n_mask.size(); ++i) {

        const double self_term =
            k_mask_self *
            std::pow(std::abs(static_cast<double>(self_mask[i])),
                     masking_q);

        const double neighbor_term =
            k_mask_xn *
            std::pow(std::abs(static_cast<double>(neighbor_mask[i])),
                     masking_q);

        const double value = self_term + neighbor_term;

        n_mask[i] = static_cast<float>(value);

        sum_n_mask += value;
        max_n_mask = std::max(max_n_mask, value);
    }

    std::cout << "Band " << b
              << ": MeanNMask=" << sum_n_mask / n_mask.size()
              << ", MaxNMask=" << max_n_mask
              << std::endl;

    n_mask_maps.push_back(std::move(n_mask));
}

        std::cout << "\n--- HDR-VDP-3 Masked Difference Bands ---" << std::endl;

const double masking_p = std::pow(10.0, 0.3424);
const double psych_slope = std::pow(10.0, 0.34);
const double pf = psych_slope / masking_p;

std::vector<std::vector<float>> d_band_maps;
d_band_maps.reserve(result.bands.size());

for (std::size_t b = 0; b < result.bands.size(); ++b) {
    const auto& ref = result.bands[b].data;
    const auto& tst = test_result.bands[b].data;
    const auto& csf = csf_maps[b];
    const auto& nmask = n_mask_maps[b];

    if (ref.size() != tst.size() ||
        ref.size() != csf.size() ||
        ref.size() != nmask.size()) {
        throw std::runtime_error("D-band size mismatch");
    }

    const double normalization = std::ldexp(1.0, static_cast<int>(b));

    std::vector<float> d_map(ref.size());

    double sum_abs_d = 0.0;
    double max_abs_d = 0.0;

    for (std::size_t i = 0; i < ref.size(); ++i) {
        const double band_diff =
            (static_cast<double>(tst[i]) -
             static_cast<double>(ref[i])) / normalization;

        const double ex_diff =
            std::copysign(std::pow(std::abs(band_diff), masking_p),
                          band_diff);

        const double sensitivity = std::max(
            static_cast<double>(csf[i]), 1e-12
        );

        // Temporary validation against HDR-VDP-3 v3.0.7 reference.
        // Base band uses a scalar CSF computed at its dominant FFT frequency.
        const bool is_base_band = (b == result.bands.size() - 1);

        const double n_ncsf = is_base_band
            ? 1.522517668677
            : 1.0 / sensitivity;

        if (is_base_band && i == 0) {
            std::cout << "DEBUG BaseBand:"
                    << " OldN_nCSF=" << 1.0 / sensitivity
                    << " NewN_nCSF=" << n_ncsf
                    << " NMask=" << nmask[i]
                    << " CSFTerm=" << std::pow(n_ncsf, 2.0 * masking_p)
                    << " MaskTerm=" << std::pow(static_cast<double>(nmask[i]), 2.0)
                    << std::endl;
        }

        const double denominator = std::sqrt(
            std::pow(n_ncsf, 2.0 * masking_p) +
            std::pow(static_cast<double>(nmask[i]), 2.0)
        );

        const double D = ex_diff / denominator;

        const double final_d =
            std::copysign(std::pow(std::abs(D), pf), D);

        d_map[i] = static_cast<float>(final_d);

        sum_abs_d += std::abs(final_d);
        max_abs_d = std::max(max_abs_d, std::abs(final_d));
    }

    std::cout << "Band " << b
              << ": MeanAbsD=" << sum_abs_d / d_map.size()
              << ", MaxAbsD=" << max_abs_d
              << std::endl;

    d_band_maps.push_back(std::move(d_map));
}

        std::cout << "\n--- HDR-VDP-3 Preliminary Quality Score ---"
          << std::endl;

const double minkowski_p = 0.8;

double q_err = 0.0;

for (std::size_t b = 0; b < d_band_maps.size(); ++b) {

    const auto& d_map = d_band_maps[b];

    if (d_map.empty()) {
        throw std::runtime_error("Empty D-band");
    }

    double sum = 0.0;

    for (float value : d_map) {
        sum += std::pow(
            std::abs(static_cast<double>(value)),
            minkowski_p
        );
    }

    const double band_error = std::pow(
        sum / static_cast<double>(d_map.size()),
        1.0 / minkowski_p
    );

    q_err += band_error /
             static_cast<double>(d_band_maps.size());

    std::cout << "Band " << b
              << ": QualityError=" << band_error
              << std::endl;
}

        const double quality_floor = -4.0;
        const double max_q = 10.0;

        const double Q =
            max_q + quality_floor -
            std::log(std::exp(quality_floor) + q_err);

        const double Q_JOD =
            10.0 -
            0.5200 * std::pow(std::max(10.0 - Q, 0.0), 1.2812);

        std::cout << "Q_err=" << q_err << std::endl;
        std::cout << "Q=" << Q << std::endl;
        std::cout << "Q_JOD=" << Q_JOD << std::endl;

        // Load Octave references for each band
        std::cout << "\n--- Comparing against Octave references ---" << std::endl;
        
        // Helper to load Octave reference (transposed)
        auto load_octave_ref = [&](const std::string& filename, unsigned& out_w, unsigned& out_h) {
            int ref_rows, ref_cols;
            auto data = load_reference(filename, ref_rows, ref_cols);
            std::vector<float> transposed(ref_cols * ref_rows);
            for (int r = 0; r < ref_rows; ++r) {
                for (int c = 0; c < ref_cols; ++c) {
                    transposed[c * ref_rows + r] = data[r * ref_cols + c];
                }
            }
            out_w = ref_rows;
            out_h = ref_cols;
            return transposed;
        };
        
        // Compare each band
        for (size_t b = 0; b < result.bands.size() && b < result.pind.size(); ++b) {
            unsigned ref_w, ref_h;
            std::string ref_file = "C:/temp/oct_jnd_spyr64_band" + std::to_string(b) + ".txt";
            auto octave_ref = load_octave_ref(ref_file, ref_w, ref_h);
            
            auto& band = result.bands[b];

            const std::vector<float>& band_data =
            (b == result.bands.size() - 1)
                ? raw_residual_band
                : band.data;

            if (band.width != (unsigned)ref_w || band.height != (unsigned)ref_h) {
                std::cout << "  Band " << b << ": Size mismatch! GPU: " << band.width << "x" << band.height
                          << " Octave: " << ref_w << "x" << ref_h << std::endl;
            }
            
            int compare_count = std::min(
                static_cast<int>(band_data.size()),
                static_cast<int>(octave_ref.size())
            );

            float max_abs_error = 0.0f;
            float rmse_sum = 0.0f;

            for (int i = 0; i < compare_count; ++i) {
                float diff = band_data[i] - octave_ref[i];

                max_abs_error = std::max(max_abs_error, std::abs(diff));
                rmse_sum += diff * diff;
            }

            float rmse = std::sqrt(rmse_sum / compare_count);
            
            std::cout << "  Band " << b << " (" << band.width << "x" << band.height << ")"
                      << "  MaxAbsErr=" << max_abs_error << "  RMSE=" << rmse << std::endl;
            
            // Print corners
            if (band.width > 0 && band.height > 0 && compare_count > 0) {
                std::cout << "    Corners: TL GPU=" << band.data[0] << " Oct=" << octave_ref[0]
                          << " TR GPU=" << band.data[band.width-1] << " Oct=" << octave_ref[band.width-1]
                          << " BL GPU=" << band.data[(band.height-1)*band.width] << " Oct=" << octave_ref[(band.height-1)*band.width]
                          << " BR GPU=" << band.data[(band.height-1)*band.width + band.width-1] 
                          << " Oct=" << octave_ref[(band.height-1)*band.width + band.width-1] << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cout << "  ERROR in full pyramid test: " << e.what() << std::endl;
    }
    
    std::cout << "\n==================================================" << std::endl;
    std::cout << "Validation complete." << std::endl;
    std::cout << "==================================================" << std::endl;
}

// Local Adaptation test implementation
static std::vector<float> generate_la_test_input(unsigned width, unsigned height) {
    std::vector<float> data(width * height);
    float N = float(width * height);
    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            data[y * width + x] = float(x * height + y + 1) / N * 100.0f;
        }
    }
    return data;
}

static void compare_la_results(
    const std::string& name,
    const std::vector<float>& gpu_data,
    const std::vector<float>& ref_data,
    unsigned out_w, unsigned out_h)
{
    if (gpu_data.size() != ref_data.size()) {
        std::cout << "  WARNING: Size mismatch! GPU: " << gpu_data.size() 
                  << " Ref: " << ref_data.size() << std::endl;
    }
    
    int compare_count = std::min(static_cast<int>(gpu_data.size()), static_cast<int>(ref_data.size()));
    float max_abs_error = 0.0f;
    float rmse_sum = 0.0f;
    
    for (int i = 0; i < compare_count; ++i) {
        float error = std::abs(gpu_data[i] - ref_data[i]);
        max_abs_error = std::max(max_abs_error, error);
        float diff = gpu_data[i] - ref_data[i];
        rmse_sum += diff * diff;
    }
    float rmse = std::sqrt(rmse_sum / compare_count);
    
    std::cout << "  " << name << ": " << out_w << "x" << out_h
              << "  MaxAbsErr=" << max_abs_error << "  RMSE=" << rmse << std::endl;
    
    if (out_w > 0 && out_h > 0 && compare_count > 0) {
        std::cout << "    Corners: TL GPU=" << gpu_data[0] << " Ref=" << ref_data[0]
                  << " TR GPU=" << gpu_data[out_w-1] << " Ref=" << ref_data[out_w-1]
                  << " BL GPU=" << gpu_data[(out_h-1)*out_w] << " Ref=" << ref_data[(out_h-1)*out_w]
                  << " BR GPU=" << gpu_data[(out_h-1)*out_w + out_w-1] 
                  << " Ref=" << ref_data[(out_h-1)*out_w + out_w-1] << std::endl;
    }
}

LocalAdaptTestResources local_adapt_test_init_res(const VulkanInstance& instance) {
    LocalAdaptTestResources res;

    // 64x64 images
    vk::ImageCreateInfo imgInfo64 = {
        .imageType = vk::ImageType::e2D,
        .format = vk::Format::eR32Sfloat,
        .extent = {64, 64, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst,
        .initialLayout = vk::ImageLayout::eUndefined
    };

    // 128x128 images
    vk::ImageCreateInfo imgInfo128 = {
        .imageType = vk::ImageType::e2D,
        .format = vk::Format::eR32Sfloat,
        .extent = {128, 128, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst,
        .initialLayout = vk::ImageLayout::eUndefined
    };

    res.imageInput = std::make_shared<VulkanImage>(VulkanResource::createImage(*instance.device(), *instance.physicalDevice(), imgInfo64));
    res.imageLogClamp = std::make_shared<VulkanImage>(VulkanResource::createImage(*instance.device(), *instance.physicalDevice(), imgInfo64));
    res.imagePadded = std::make_shared<VulkanImage>(VulkanResource::createImage(*instance.device(), *instance.physicalDevice(), imgInfo128));
    res.imageGaussH = std::make_shared<VulkanImage>(VulkanResource::createImage(*instance.device(), *instance.physicalDevice(), imgInfo128));
    res.imageGaussV = std::make_shared<VulkanImage>(VulkanResource::createImage(*instance.device(), *instance.physicalDevice(), imgInfo128));
    res.imageOutput = std::make_shared<VulkanImage>(VulkanResource::createImage(*instance.device(), *instance.physicalDevice(), imgInfo64));

    constexpr vk::DeviceSize bufferSize = 128 * 128 * sizeof(float);
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
        static_cast<unsigned>(64 * 64 * sizeof(float)),
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

void local_adapt_test_upload(const VulkanInstance& instance, const LocalAdaptTestResources& res, const std::vector<float>& inputData) {
    vk::DeviceSize dataSize = inputData.size() * sizeof(float);

    void* mapped = res.stgInputMemory.mapMemory(0, dataSize, {});
    std::memcpy(mapped, inputData.data(), static_cast<size_t>(dataSize));
    res.stgInputMemory.unmapMemory();

    const vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };
    instance.cmdBufTransfer()->reset();
    instance.cmdBufTransfer()->begin(beginInfo);

    // Initialize all images
    VulkanResource::initImages(*instance.cmdBufTransfer(), {
        res.imageInput, res.imageLogClamp, res.imagePadded, 
        res.imageGaussH, res.imageGaussV, res.imageOutput
    });

    // Copy input to imageInput (64x64)
    vk::BufferImageCopy region64 = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .imageOffset = {0, 0, 0},
        .imageExtent = {64, 64, 1}
    };
    instance.cmdBufTransfer()->copyBufferToImage(res.stgInput, *res.imageInput->image, vk::ImageLayout::eGeneral, region64);

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

LocalAdaptTestResult local_adapt_test_copy_back(const VulkanInstance& instance, const LocalAdaptTestResources& res) {
    LocalAdaptTestResult result;
    result.outputData.resize(64 * 64);

    const vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };
    instance.cmdBufTransfer()->reset();
    instance.cmdBufTransfer()->begin(beginInfo);

    // Copy output image (64x64) to staging buffer
    vk::BufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .imageOffset = {0, 0, 0},
        .imageExtent = {64, 64, 1}
    };
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

    void* mapped = res.stgOutputMemory.mapMemory(0, 64 * 64 * sizeof(float), {});
    std::memcpy(result.outputData.data(), mapped, 64 * 64 * sizeof(float));
    res.stgOutputMemory.unmapMemory();

    return result;
}

void local_adapt_test_run(const Args& args, const VulkanInstance& instance) {
    IQM::HDRVDP3::LocalAdapt localAdapt(*instance.device(), *instance.physicalDevice());
    
    const unsigned width = 64;
    const unsigned height = 64;
    const float ppd = 30.0f;
    auto input_data = generate_la_test_input(width, height);
    
    std::cout << "==================================================" << std::endl;
    std::cout << "HDR-VDP-3 Local Adaptation Validation" << std::endl;
    std::cout << "Input: " << width << "x" << height << " (MATLAB-compatible)" << std::endl;
    std::cout << "PPD: " << ppd << " (Model #7 from Vangorp et al. 2015)" << std::endl;
    std::cout << "Reference: Official Octave hdrvdp_local_adapt (C:/temp/oct_la_output_rowmajor.txt)" << std::endl;
    std::cout << "Note: GPU uses separable 1D circular conv; Ref uses full 2D FFT conv (inherent ~6 MaxAbsErr)" << std::endl;
    std::cout << "==================================================" << std::endl;
    
    // Initialize resources
    auto res = local_adapt_test_init_res(instance);
    local_adapt_test_upload(instance, res, input_data);
    
    // Run Local Adaptation
    auto laInput = IQM::HDRVDP3::LocalAdaptInput {
        .device = instance.device(),
        .cmdBuf = &*instance.cmdBuf(),
        .ivInput = &res.imageInput->imageView,
        .ivLogClamp = &res.imageLogClamp->imageView,
        .ivPadded = &res.imagePadded->imageView,
        .ivGaussH = &res.imageGaussH->imageView,
        .ivGaussV = &res.imageGaussV->imageView,
        .ivOutput = &res.imageOutput->imageView,
        .ppd = ppd,
    };
    
    vk::raii::Fence computeFence(*instance.device(), vk::FenceCreateInfo{});
    
    const vk::CommandBufferBeginInfo beginInfo = {
        .flags = vk::CommandBufferUsageFlags{vk::CommandBufferUsageFlagBits::eOneTimeSubmit},
    };
    
    instance.cmdBuf()->reset();
    instance.cmdBuf()->begin(beginInfo);
    localAdapt.compute(laInput);
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
    
    std::cout << "[DEBUG] submitting compute" << std::endl;
    instance.queue()->submit(submitInfo, computeFence);
    instance.waitForFence(computeFence);
    
    // Copy back and compare
    auto result = local_adapt_test_copy_back(instance, res);
    
    // Load reference
    int ref_rows, ref_cols;
    auto ref_data = load_reference("C:/temp/oct_la_output_rowmajor.txt", ref_rows, ref_cols);
    
    std::cout << "\n--- Local Adaptation vs Octave Reference ---" << std::endl;
    compare_la_results("Local Adaptation", result.outputData, ref_data, 64, 64);
    
    std::cout << "\n==================================================" << std::endl;
    std::cout << "Local Adaptation validation complete." << std::endl;
    std::cout << "==================================================" << std::endl;
}

std::vector<float> compute_local_adaptation_gpu(
    const IQM::VulkanInstance& instance,
    const std::vector<float>& luminance,
    float ppd
) {
    if (luminance.size() != 64 * 64) {
        throw std::invalid_argument(
            "Local Adaptation integration currently requires a 64x64 image"
        );
    }

    IQM::HDRVDP3::LocalAdapt localAdapt(
        *instance.device(),
        *instance.physicalDevice()
    );

    auto res = local_adapt_test_init_res(instance);
    local_adapt_test_upload(instance, res, luminance);

    auto laInput = IQM::HDRVDP3::LocalAdaptInput{
        .device = instance.device(),
        .cmdBuf = &*instance.cmdBuf(),
        .ivInput = &res.imageInput->imageView,
        .ivLogClamp = &res.imageLogClamp->imageView,
        .ivPadded = &res.imagePadded->imageView,
        .ivGaussH = &res.imageGaussH->imageView,
        .ivGaussV = &res.imageGaussV->imageView,
        .ivOutput = &res.imageOutput->imageView,
        .ppd = ppd,
    };

    vk::raii::Fence computeFence(
        *instance.device(),
        vk::FenceCreateInfo{}
    );

    const vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
    };

    instance.cmdBuf()->reset();
    instance.cmdBuf()->begin(beginInfo);

    localAdapt.compute(laInput);

    instance.cmdBuf()->end();

    vk::CommandBuffer cmdBuf = *instance.cmdBuf();

    auto waitStage = vk::PipelineStageFlags{
        vk::PipelineStageFlagBits::eComputeShader
    };

    const vk::SubmitInfo submitInfo{
        .sType = vk::StructureType::eSubmitInfo,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*res.uploadDone,
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdBuf,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*res.computeDone,
    };

    instance.queue()->submit(submitInfo, computeFence);
    instance.waitForFence(computeFence);

    auto result = local_adapt_test_copy_back(instance, res);

    return result.outputData;
}

} // namespace IQM::Bin