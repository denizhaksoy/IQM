/*
 * Image Quality Metrics - HDR-VDP-3 Test Wrapper
 * Deniz Aksoy - 2026
 */

#include <iostream>
#include <iomanip>
#include <limits>
#include <vector>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <cstdlib>
#include <string>
#include <algorithm>
#include "hdrvdp3.h"
#include <IQM/hdrvdp3/csf.h>
#include <IQM/hdrvdp3/spectral.h>
#include <IQM/hdrvdp3/luminance.h>
#include <IQM/hdrvdp3/resize.h>
#include <IQM/hdrvdp3/quality.h>
#include <IQM/hdrvdp3/map_gpu.h>
#include "../../shared/debug_utils.h"
#include "../../shared/vulkan_res.h"
#include <IQM/hdrvdp3/spyr.h>
#include <IQM/hdrvdp3/jnd.h>
#include <IQM/hdrvdp3/mtf.h>

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
template<class T> static void save_reference(const std::string& filename, const std::vector<T>& data, unsigned rows, unsigned cols) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open reference file for writing: " + filename);
    }

    file << std::setprecision(std::numeric_limits<T>::max_digits10);

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

    const vk::DeviceSize bufferSize = vk::DeviceSize(width) * height * sizeof(float);
    if (!width || !height || bufferSize > std::numeric_limits<unsigned>::max())
        throw std::invalid_argument("Invalid corrDn staging dimensions");
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
    int step,bool use_non_fma = false)
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
        .use_non_fma = use_non_fma,
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
    // Native single conv2 uses32-element vector blocks on the longer axis.
    // Keep arithmetic in distinct shaders; merge only already-rounded values.
    const unsigned axis_length=std::max(width,height);
    if(!use_non_fma && axis_length%32!=0) {
        const auto tail=run_corrdn_gpu(instance,corrdn,filter_coeffs,filter_size,
            input_data,width,height,step,true);
        const unsigned vector_end=(axis_length/32)*32;
        for(unsigned y=0;y<outHeight;++y) for(unsigned x=0;x<outWidth;++x)
            if((width>height?x:y)*step>=vector_end)
                result.outputData[y*outWidth+x]=tail[y*outWidth+x];
    }
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

static HDRVDP3FFTConfig fft_config_from_environment(unsigned side);

static void run_quality_inputs(const VulkanInstance& instance);
void hdrvdp3_test_run(const Args& args, const VulkanInstance& instance) {
    // Explicit luminance files are a quality request; unrelated component
    // diagnostics remain in the historical synthetic entry point.
    if(const char* path=std::getenv("IQM_LUMINANCE_REFERENCE");path&&*path){
        run_quality_inputs(instance);return;
    }
    if(const char* path=std::getenv("IQM_LUMINANCE_TEST");path&&*path){
        run_quality_inputs(instance);return;
    }
    if(const char* fixture=std::getenv("IQM_FFT_SIZE_FIXTURE");fixture&&*fixture){
        const auto directory=std::filesystem::path("validation")/("fft_size_pipeline_"+std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(directory);
        for(const std::string label:{"ref","test"}){
            std::ifstream input(std::filesystem::path(fixture)/(label+"_luminance.txt"));
            unsigned h,w;if(!(input>>h>>w)||h!=w||(h!=32&&h!=64))throw std::runtime_error("FFT size probe requires32x32 or64x64 fixture");
            std::vector<float> values(w*h);for(float& v:values)if(!(input>>v)||!std::isfinite(v))throw std::runtime_error("Invalid FFT size probe input");
            const auto retinal=compute_fft_filter_gpu(instance,values,30.0f,IQM::HDRVDP3::FFTFilterMode::OpticalMTF,(directory/(label+"_mtf_")).string());
            save_reference((directory/(label+"_retinal.txt")).string(),retinal,h,w);
            if(const char* shader=std::getenv("IQM_MAP_SHADER");shader&&*shader){
                IQM::HDRVDP3::MapGPU maps(instance,shader);
                const auto adapt_input=maps.adaptation_input(retinal);
                const auto adaptation=compute_fft_filter_gpu(instance,adapt_input,30.0f,IQM::HDRVDP3::FFTFilterMode::LocalAdaptation,(directory/(label+"_la_")).string());
                save_reference((directory/(label+"_adapt_input.txt")).string(),adapt_input,h,w);
                save_reference((directory/(label+"_adaptation.txt")).string(),adaptation,h,w);
            }
        }
        std::ofstream(directory/"source.txt")<<fixture<<'\n';
        std::cout<<"FFT size pipeline directory: "<<directory.string()<<'\n';
        return;
    }
    // Isolated dimension probe: raw single SPYR input, no MTF/JND/quality.
    // Keep unsupported full-metric backend guards intact.
    if (const char* probe = std::getenv("IQM_SPYR_DIMENSION_PROBE"); probe && std::string(probe)=="1") {
        const auto directory = std::filesystem::path("validation") /
            ("spyr_dimensions_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(directory);
        const char* probe_pattern_env=std::getenv("IQM_SPYR_PROBE_PATTERN");
        const std::string probe_pattern=probe_pattern_env?probe_pattern_env:"asymmetric";
        if(probe_pattern!="asymmetric"&&probe_pattern!="impulse"&&probe_pattern!="random")
            throw std::invalid_argument("Unknown IQM_SPYR_PROBE_PATTERN");
        std::ofstream(directory/"pattern.txt") << probe_pattern << '\n';
        const unsigned sizes[][2]={{32,32},{48,48},{64,64},{96,96},{128,128},{49,65},{64,96},{65,49},{96,64}};
        IQM::HDRVDP3::Spyr::Filters filters{lo0filt_matlab,hi0filt_matlab,lofilt_matlab,bfilt1_matlab,1};
        IQM::HDRVDP3::Spyr pyramid(*instance.device(),filters);
        std::ofstream manifest(directory/"cases.txt");
        for (const auto& size:sizes) {
            const unsigned h=size[0],w=size[1];
            const auto name=std::to_string(h)+"x"+std::to_string(w);
            const auto output=directory/name;
            std::filesystem::create_directories(output);
            std::vector<float> input(w*h);
            uint32_t random_state=0x12345678u;
            for(unsigned y=0;y<h;++y) for(unsigned x=0;x<w;++x) {
                input[y*w+x]=100.0f+0.125f*float(int((3*x+7*y+x*y)%101)-50);
                if(probe_pattern=="impulse") {
                    input[y*w+x]=100.0f;
                    if((x==0&&y==7)||(x==w-1&&y==h-2)) input[y*w+x]=125.0f;
                    if(x==w/3&&y==2*h/3) input[y*w+x]=75.0f;
                } else if(probe_pattern=="random") {
                    random_state=1664525u*random_state+1013904223u;
                    input[y*w+x]=float(1u+(random_state>>12))*(1.0f/1024.0f);
                }
            }
            save_reference((output/"input.txt").string(),input,h,w);
            // Official min(ceil(log2(ppd))-2,maxPyrHt(...)), ppd=30.
            const auto result=pyramid.compute(input,w,h,instance,
                [&](const IQM::VulkanInstance& inst,IQM::HDRVDP3::CorrDn& cdn,
                    const float* filter,int filter_size,const std::vector<float>& data,
                    unsigned width,unsigned height,int step,bool non_fma) {
                    return run_corrdn_gpu(inst,cdn,filter,filter_size,data,width,height,step,non_fma);
                },(output/"gpu_band").string(),3);
            std::ofstream info(output/"parameters.txt");
            info << "30 " << result.pyr_height << ' ' << result.bands.size() << '\n';
            manifest << name << '\n';
        }
        std::cout << "SPYR dimension directory: " << directory.string() << '\n';
        return;
    }
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
    run_quality_inputs(instance);
}
static void run_quality_inputs(const VulkanInstance& instance) {
    std::cout << "\n==================================================" << std::endl;
    std::cout << "HDR-VDP-3 Full Steerable Pyramid Validation (64x64)" << std::endl;
    std::cout << "==================================================" << std::endl;
    
    try {
            // Generate 64x64 spatially varying luminance images
            const char* size_env=std::getenv("IQM_SPYR_SIZE");
            const std::string size_text=size_env?size_env:"64";
            unsigned width64 = size_text=="32"?32:64;
            unsigned height64 = width64;
            const char* reference_path=std::getenv("IQM_LUMINANCE_REFERENCE");
            const char* test_path=std::getenv("IQM_LUMINANCE_TEST");
            const bool file_input=reference_path&&*reference_path;
            if(file_input!=bool(test_path&&*test_path))throw std::invalid_argument("Both luminance PFM paths are required");
            IQM::HDRVDP3::LuminanceImage reference_file,test_file;
            if(file_input) {
                reference_file=IQM::HDRVDP3::load_luminance_pfm(reference_path);
                test_file=IQM::HDRVDP3::load_luminance_pfm(test_path);
                width64=reference_file.width;height64=reference_file.height;
                const bool square=width64==height64&&(width64==32||width64==64||width64==128);
                const bool rectangle=(width64==128&&height64==64)||(width64==64&&height64==128);
                if((!square&&!rectangle)||test_file.width!=width64||test_file.height!=height64)
                    throw std::invalid_argument("PFM pair requires matching32/64/128square or64x128/128x64 dimensions;no automatic resizing");
            }
            else if(size_text!="32"&&size_text!="64")throw std::invalid_argument("Unvalidated IQM_SPYR_SIZE");

            // Reference: spatially varying luminance (9.5 to 10.5 cd/m^2)
            std::vector<float> input_data_64=file_input?reference_file.pixels:std::vector<float>(width64*height64);
            const char* pattern_env = std::getenv("IQM_SPYR_PATTERN");
            const std::string pattern = file_input?"file":(pattern_env ? pattern_env : "diagonal");
            if (!file_input && pattern != "diagonal" && pattern != "asymmetric" && pattern != "impulse" && pattern != "hdr_ramp" && pattern != "hdr_edges" && pattern != "sinusoidal_x" && pattern != "sinusoidal_y") {
                throw std::invalid_argument("Unknown IQM_SPYR_PATTERN: " + pattern);
            }
            std::cout << "SPYR input pattern: " << pattern << '\n';

            if(!file_input) for (unsigned y = 0; y < height64; ++y) {
                for (unsigned x = 0; x < width64; ++x) {
                    input_data_64[y * width64 + x] =
                        10.0f + 0.1f * static_cast<float>(
                            static_cast<int>((x + y) % 11) - 5
                        );
                    if (pattern == "asymmetric") {
                        input_data_64[y * width64 + x] = 10.0f + 0.01f *
                            static_cast<float>(static_cast<int>(
                                (3 * x + 7 * y + x * y) % 101) - 50);
                    } else if (pattern == "sinusoidal_x" || pattern == "sinusoidal_y") {
                        const double primary=pattern=="sinusoidal_x"?x:y;
                        const double secondary=pattern=="sinusoidal_x"?y:x;
                        input_data_64[y*width64+x]=static_cast<float>(10.0+std::sin(2*std::acos(-1.0)*primary/32.0)+0.25*std::cos(2*std::acos(-1.0)*secondary/16.0));
                    } else if (pattern == "hdr_ramp") {
                        const float t=static_cast<float>(3*x+2*y)/static_cast<float>(3*(width64-1)+2*(height64-1));
                        input_data_64[y*width64+x]=std::pow(10.0f,-3.0f+7.0f*t);
                    } else if (pattern == "hdr_edges") {
                        const float levels[]={0.001f,0.01f,0.1f,1.0f,10.0f,100.0f,1000.0f,10000.0f};
                        input_data_64[y*width64+x]=levels[((x/(width64/8))+(y/(height64/4)))%8];
                    } else if (pattern == "impulse") {
                        // Edge impulses exercise reflect1; the off-center impulse
                        // also exposes an accidental transpose or sampling shift.
                        input_data_64[y * width64 + x] = 10.0f;
                        if (x == 0 && y == 7) input_data_64[y * width64 + x] = 10.5f;
                        if (x == (19*width64)/64 && y == (43*height64)/64) input_data_64[y * width64 + x] = 9.5f;
                        if (x == width64-1 && y == height64-3) input_data_64[y * width64 + x] = 10.25f;
                    }
                }
            }

            const auto reference_image = input_data_64;

            // Validation cases keep the original 5% brighter default.
            auto test_image = file_input?test_file.pixels:reference_image;
            const char* transform_env=std::getenv("IQM_SPYR_TEST_TRANSFORM");
            const std::string transform=file_input?"file":(transform_env?transform_env:"brighter");
            if(!file_input&&transform!="brighter"&&transform!="identical"&&transform!="localized"&&transform!="stronger"&&transform!="localized_stronger"&&transform!="phase_shift"&&transform!="threshold_below"&&transform!="threshold_nominal"&&transform!="threshold_above")
                throw std::invalid_argument("Unknown IQM_SPYR_TEST_TRANSFORM");
            float threshold_gain=1.001f;
            if(transform=="threshold_below") threshold_gain=std::nextafter(1.001f,0.0f);
            if(transform=="threshold_above") threshold_gain=std::nextafter(1.001f,2.0f);
            if(!file_input) for(unsigned y=0;y<height64;++y) for(unsigned x=0;x<width64;++x) {
                if(transform=="brighter") test_image[y*width64+x]*=1.05f;
                else if(transform=="stronger") test_image[y*width64+x]*=1.5f;
                else if((transform=="localized"||transform=="localized_stronger") && x>=width64/2&&x<width64/2+8&&y>=13&&y<21) test_image[y*width64+x]*=(transform=="localized"?1.05f:2.0f);
                else if(transform=="phase_shift") test_image[y*width64+x]=reference_image[((y+8)%height64)*width64+(x+8)%width64];
                else if(transform.rfind("threshold_",0)==0) test_image[y*width64+x]*=threshold_gain;
            }

        // All environment parsing and synthetic/file preparation stay in the
        // harness. The library call receives explicit inputs and configuration.
        auto setting=[](const char* name,const char* fallback="") {
            const char* value=std::getenv(name);return std::string(value?value:fallback);
        };
        const auto runtime=setting("IQM_QUALITY_RUNTIME");
        if(runtime.empty()&&(setting("IQM_PREPROCESSING","none")!="fft" || setting("IQM_MAP_BACKEND","cpu")!="vulkan" || setting("IQM_FFT_BACKEND","vkfft")!="reference"))
            throw std::invalid_argument("Quality API harness requires reference FFT preprocessing and Vulkan maps");
        IQM::HDRVDP3::EvaluationOptions options;
        const auto ppd=setting("IQM_PIX_PER_DEGREE","30"),age=setting("IQM_OBSERVER_AGE","24");
        if(ppd!="30"&&ppd!="60")throw std::invalid_argument("Unvalidated IQM_PIX_PER_DEGREE");
        if(age!="24"&&age!="60"&&age!="60.001")throw std::invalid_argument("Unvalidated observer age");
        options.pixelsPerDegree=std::stod(ppd);options.observerAge=std::stod(age);
        if(runtime.empty()) {
            options.fft=fft_config_from_environment(width64);
            options.mapShader=setting("IQM_MAP_SHADER");
            options.baseFFTShader=setting("IQM_BASE_FFT_SHADER");
        } else {
            const std::filesystem::path assets(runtime);
            options.fft.columnFirst=true;
            options.fft.referenceShaderPath=(assets/(width64==height64&&width64!=128?"fft"+std::to_string(width64)+".spv":"fft.spv")).string();
            options.fft.referenceMathDirectory=(assets/"math").string();
            options.mapShader=assets/"map.spv";options.baseFFTShader=assets/"base.spv";
        }
        options.spectralDataDirectory=setting("IQM_SPECTRAL_DATA");
        const auto sampled_path=setting("IQM_SPECTRAL_SAMPLES");
        if(!sampled_path.empty()) {
            if(!options.spectralDataDirectory.empty())throw std::invalid_argument("Choose raw or resampled spectral inputs");
            std::ifstream input(sampled_path);size_t count;
            if(!(input>>count)||count!=420)throw std::invalid_argument("Expected pinned420-point native spectral fixture");
            auto& samples=options.spectralSamples.emplace();
            samples.wavelength.resize(count);samples.emission.resize(count);samples.sensitivity.resize(count);
            for(size_t i=0;i<count;++i){
                if(!(input>>samples.wavelength[i]>>samples.emission[i]))throw std::invalid_argument("Truncated spectra");
                for(double& value:samples.sensitivity[i])if(!(input>>value))throw std::invalid_argument("Truncated sensitivity");
            }
        }
        const auto export_dir=std::filesystem::path("validation")/(
            "spyr_spatial_"+std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        const auto evaluated=IQM::HDRVDP3::evaluate_quality(instance,
            IQM::HDRVDP3::LuminanceImage{width64,height64,reference_image},
            IQM::HDRVDP3::LuminanceImage{width64,height64,test_image},options,
            IQM::HDRVDP3::EvaluationTrace{export_dir,file_input?"pfm_absolute_luminance":"synthetic",pattern,transform});
        std::cout<<"Quality API returned Q="<<evaluated.scores.Q<<" Q_JOD="<<evaluated.scores.Q_JOD<<'\n';
        std::cout<<"Quality export directory: "<<std::filesystem::absolute(export_dir).string()<<'\n';
    } catch (const std::exception& e) {
        std::cout << "  ERROR in full pyramid test: " << e.what() << std::endl;
        throw;
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

static HDRVDP3FFTConfig fft_config_from_environment(unsigned side) {
    HDRVDP3FFTConfig config;
    const char* axis_env = std::getenv("IQM_FFT_AXIS_ORDER");
    const std::string axis_order = axis_env ? axis_env : "row";
    if(axis_order!="row" && axis_order!="column")throw std::invalid_argument("Unknown IQM_FFT_AXIS_ORDER: "+axis_order);
    config.columnFirst=axis_order=="column";
    const char* twiddle_env=std::getenv("IQM_FFT_TWIDDLES");
    const std::string twiddles=twiddle_env?twiddle_env:"native";
    if(twiddles!="native" && twiddles!="lut")throw std::invalid_argument("Unknown IQM_FFT_TWIDDLES: "+twiddles);
    config.useTwiddleLUT=twiddles=="lut";
    const char* backend_env=std::getenv("IQM_FFT_BACKEND");
    const std::string backend=backend_env?backend_env:"vkfft";
    if(backend!="vkfft" && backend!="reference")throw std::invalid_argument("Unknown FFT backend: "+backend);
    if(backend=="reference") {
        const char* shader=std::getenv(side==32?"IQM_FFT_REFERENCE_SHADER64":"IQM_FFT_REFERENCE_SHADER");
        if(!shader || !*shader)throw std::invalid_argument("Reference FFT requires IQM_FFT_REFERENCE_SHADER");
        config.referenceShaderPath=shader;
    }
    const char* math=std::getenv("IQM_FFT_MATH_SHADER_DIR");
    if(math && *math)config.referenceMathDirectory=math;
    return config;
}

std::vector<float> compute_fft_filter_gpu(const IQM::VulkanInstance& instance,
    const std::vector<float>& input, float ppd, IQM::HDRVDP3::FFTFilterMode mode,
    const std::string& trace_prefix) {
    return compute_fft_filter_gpu(instance,input,ppd,mode,
        fft_config_from_environment(input.size()==32*32?32:64),trace_prefix);
}

std::vector<float> compute_fft_filter_gpu(const IQM::VulkanInstance& instance,
    const std::vector<float>& input, float ppd, IQM::HDRVDP3::FFTFilterMode mode,
    const HDRVDP3FFTConfig& config, const std::string& trace_prefix) {
    const unsigned side=input.size()==32*32?32:64,fft_side=2*side;
    if (input.size() != side*side)
        throw std::invalid_argument("FFT filter currently requires32x32 or64x64 input");
    IQM::HDRVDP3::LocalAdaptFFT filter(*instance.device(), *instance.physicalDevice());
    auto res = hdrvdp3_test_init_res(side, side, instance);
    hdrvdp3_test_upload(instance, res, input);
    IQM::HDRVDP3::LocalAdaptFFTInput args{};
    args.device = instance.device(); args.physicalDevice = instance.physicalDevice();
    args.cmdBuf = &*instance.cmdBuf(); args.cmdPool = &*instance.cmdPool();
    args.queue = &*instance.queue();
    args.ivInput = &res.imageInput->imageView; args.ivOutput = &res.imageOutput->imageView;
    args.ppd = ppd; args.mode = mode;args.width=side;args.height=side;
    args.columnFirst=config.columnFirst;
    args.useTwiddleLUT=config.useTwiddleLUT;
    args.referenceShaderPath=config.referenceShaderPath;
    args.referenceMathDirectory=config.referenceMathDirectory;
    const vk::DeviceSize snapshotBytes = fft_side * fft_side * 2 * sizeof(float);
    // Declare memory before buffer so the bound buffer is destroyed first.
    vk::raii::DeviceMemory traceMemory = nullptr;
    vk::raii::Buffer traceBuffer = nullptr;
    if (!trace_prefix.empty()) {
        auto allocation = VulkanResource::createBuffer(*instance.device(), *instance.physicalDevice(),
            static_cast<unsigned>(snapshotBytes * 4), vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        traceBuffer = std::move(allocation.first); traceMemory = std::move(allocation.second);
        traceBuffer.bindMemory(traceMemory, 0); args.traceBuffer = &traceBuffer;
    }
    instance.cmdBuf()->reset();
    instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    filter.compute(args);
    instance.cmdBuf()->end();
    vk::CommandBuffer cmd = *instance.cmdBuf();
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eComputeShader;
    vk::SubmitInfo submit{};
    submit.waitSemaphoreCount=1; submit.pWaitSemaphores=&*res.uploadDone;
    submit.pWaitDstStageMask=&waitStage; submit.commandBufferCount=1;
    submit.pCommandBuffers=&cmd; submit.signalSemaphoreCount=1;
    submit.pSignalSemaphores=&*res.computeDone;
    vk::raii::Fence fence(*instance.device(), vk::FenceCreateInfo{});
    instance.queue()->submit(submit,fence); instance.waitForFence(fence);
    if (!trace_prefix.empty()) {
        const float* mapped = static_cast<const float*>(traceMemory.mapMemory(0, snapshotBytes * 4));
        for (unsigned stage = 0; stage < 4; ++stage) {
            const float* begin = mapped + stage * fft_side * fft_side * 2;
            save_reference(trace_prefix + "stage" + std::to_string(stage) + ".txt",
                std::vector<float>(begin, begin + fft_side * fft_side * 2), fft_side, 2*fft_side);
        }
        traceMemory.unmapMemory();
        std::vector<float> kernel(fft_side * fft_side);
        if (mode == IQM::HDRVDP3::FFTFilterMode::OpticalMTF) {
            IQM::HDRVDP3::generate_mtf_filter(kernel.data(), fft_side, fft_side, ppd);
        } else {
            const double sigma = std::pow(10.0, -0.781367) * ppd;
            for (unsigned y=0; y<fft_side; ++y) for (unsigned x=0; x<fft_side; ++x) {
                const double kx=(std::fmod(0.5+double(x)/fft_side,1.0)-0.5)*(2*3.14159265358979323846);
                const double ky=(std::fmod(0.5+double(y)/fft_side,1.0)-0.5)*(2*3.14159265358979323846);
                kernel[y*fft_side+x]=static_cast<float>(std::exp(-0.5*(kx*kx+ky*ky)*sigma*sigma));
            }
        }
        save_reference(trace_prefix + "filter.txt", kernel, fft_side, fft_side);
    }
    return hdrvdp3_test_copy_back(instance,res,side,side).outputData;
}

} // namespace IQM::Bin
