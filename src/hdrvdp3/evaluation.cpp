#include <IQM/hdrvdp3/evaluation.h>
#include <IQM/hdrvdp3/corrdn.h>
#include <IQM/hdrvdp3/local_adapt.h>
#include <IQM/hdrvdp3/spyr.h>
#include <IQM/hdrvdp3/mtf.h>
#include <IQM/hdrvdp3/jnd.h>
#include <IQM/hdrvdp3/csf.h>
#include <IQM/hdrvdp3/resize.h>
#include <IQM/hdrvdp3/map_gpu.h>
#include <vulkan_res.h>
#include <vulkan.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace IQM::HDRVDP3 {
namespace {
using IQM::Bin::VulkanResource;
using IQM::Bin::VulkanImage;
struct FilterResources {
    vk::raii::DeviceMemory stgInputMemory=nullptr,stgOutputMemory=nullptr;
    vk::raii::Buffer stgInput=nullptr,stgOutput=nullptr;
    std::shared_ptr<VulkanImage> imageInput,imageOutput;
    vk::raii::Semaphore uploadDone=nullptr,computeDone=nullptr;
    vk::raii::Fence transferFence=nullptr;
};
struct FilterResult { std::vector<float> outputData; };
static std::vector<float> transpose_filter(const float* input,int n) {
    std::vector<float> output(n*n);
    for(int row=0;row<n;++row)for(int col=0;col<n;++col)
        output[row*n+col]=input[col*n+row];
    return output;
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

FilterResources init_filter_resources(unsigned width, unsigned height, const VulkanInstance& instance) {
    FilterResources res;

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

void upload_filter_input(const VulkanInstance& instance, const FilterResources& res, const std::vector<float>& inputData) {
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

FilterResult read_filter_output(const VulkanInstance& instance, const FilterResources& res, unsigned outWidth, unsigned outHeight) {
    FilterResult result;
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
    auto res = init_filter_resources(width, height, instance);
    upload_filter_input(instance, res, input_data);
    
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
    
    auto result = read_filter_output(instance, res, outWidth, outHeight);
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

std::vector<float> compute_fft_filter_gpu(const IQM::VulkanInstance& instance,
    const std::vector<float>& input, float ppd, IQM::HDRVDP3::FFTFilterMode mode,
    const FFTExecutionConfig& config, const std::string& trace_prefix) {
    const unsigned side=input.size()==32*32?32:64,fft_side=2*side;
    if (input.size() != side*side)
        throw std::invalid_argument("FFT filter currently requires32x32 or64x64 input");
    IQM::HDRVDP3::LocalAdaptFFT filter(*instance.device(), *instance.physicalDevice());
    auto res = init_filter_resources(side, side, instance);
    upload_filter_input(instance, res, input);
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
    return read_filter_output(instance,res,side,side).outputData;
}

} // namespace
EvaluationResult evaluate_quality(const IQM::VulkanInstance& instance,
    const LuminanceImage& reference,const LuminanceImage& test,
    const EvaluationOptions& options,const EvaluationTrace& trace) {
    const unsigned width64=reference.width,height64=reference.height;
    if(width64!=height64||(width64!=32&&width64!=64)||test.width!=width64||test.height!=height64)
        throw std::invalid_argument("Quality API requires equal square32/64 images");
    const auto count=static_cast<size_t>(width64)*height64;
    if(reference.pixels.size()!=count||test.pixels.size()!=count)
        throw std::invalid_argument("Luminance pixel count mismatch");
    for(const auto* image:{&reference,&test})for(float value:image->pixels)
        if(!(value>0)||!std::isfinite(value))throw std::invalid_argument("Expected positive finite absolute luminance");
    if(options.pixelsPerDegree!=30&&options.pixelsPerDegree!=60)throw std::invalid_argument("Unvalidated pixels per degree");
    if(options.observerAge!=24&&options.observerAge!=60&&options.observerAge!=60.001)throw std::invalid_argument("Unvalidated observer age");
    if(!options.fft.columnFirst||options.fft.useTwiddleLUT||options.fft.referenceShaderPath.empty()||options.fft.referenceMathDirectory.empty()||options.mapShader.empty()||options.baseFFTShader.empty())
        throw std::invalid_argument("Quality API requires pinned native column-first reference FFT, math, map and base FFT assets");
    if(trace.directory.empty()||std::filesystem::exists(trace.directory))throw std::invalid_argument("A fresh diagnostic directory is required");
    if(!std::filesystem::create_directories(trace.directory))throw std::invalid_argument("Cannot create diagnostic directory");
    const auto& reference_image=reference.pixels;
    const auto& test_image=test.pixels;
    const auto& input_data_64=reference_image;
    const auto& pattern=trace.pattern;
    const auto& transform=trace.transform;
        const double viewing_ppd=options.pixelsPerDegree;
        const double observer_age=options.observerAge;
        const double visual_area=static_cast<double>(width64)*height64/(viewing_ppd*viewing_ppd);
        auto spectral_matrix=IQM::HDRVDP3::D65_AGE24_LMR;
        const char* samples_path=options.spectralSamples?"explicit_samples":nullptr;
        const auto raw_data_text=options.spectralDataDirectory.string(); const char* raw_data_path=raw_data_text.c_str();
        if(raw_data_path&&*raw_data_path) {
            if(samples_path&&*samples_path)throw std::invalid_argument("Choose raw or resampled spectral inputs");
            const std::filesystem::path data(raw_data_path);
            const auto samples=IQM::HDRVDP3::load_d65_spectral_samples(data/"log_cone_smith_pokorny_1975.csv",data/"cie_scotopic_lum.txt",data/"d65.csv");
            const auto calibration=IQM::HDRVDP3::calibrate_spectral_age(samples,observer_age);
            spectral_matrix={calibration.matrix[0],calibration.matrix[1],calibration.matrix[3]};
        } else if(samples_path&&*samples_path) {
            const auto& samples=*options.spectralSamples;
            const auto calibration=IQM::HDRVDP3::calibrate_spectral_age(samples,observer_age);
            spectral_matrix={calibration.matrix[0],calibration.matrix[1],calibration.matrix[3]};
        } else if(observer_age!=24)throw std::invalid_argument("Age extension requires pinned spectral inputs");
        const auto& export_dir=trace.directory;
        std::ofstream(export_dir/"input_source.txt")<<trace.inputSource<<'\n';
        {std::ofstream matrix_file(export_dir/"spectral_lmr.txt");matrix_file<<std::setprecision(17);
         for(double value:spectral_matrix)matrix_file<<value<<'\n';}
        std::ofstream(export_dir/"spectral_source.txt")<<((raw_data_path&&*raw_data_path)?"raw_d65":((samples_path&&*samples_path)?"resampled_d65":"legacy_age24"))<<'\n';
        std::ofstream(export_dir/"input_size.txt")<<height64<<' '<<width64<<'\n';
        std::ofstream(export_dir/"pixels_per_degree.txt")<<viewing_ppd<<'\n';
        std::ofstream(export_dir/"band_count.txt")<<(Spyr::max_pyr_height(width64,height64,13)+2)<<'\n';
        std::ofstream(export_dir / "test_transform.txt") << transform << '\n';
        const char* map_backend_env="vulkan";
        const std::string map_backend=map_backend_env?map_backend_env:"cpu";
        if(map_backend!="cpu" && map_backend!="vulkan") throw std::invalid_argument("Unknown IQM_MAP_BACKEND");
        const auto map_shader_text=options.mapShader.string();const char* map_shader_env=map_shader_text.c_str();
        std::unique_ptr<IQM::HDRVDP3::MapGPU> gpu_maps;
        if(map_backend=="vulkan") {
            if(!map_shader_env || !*map_shader_env) throw std::invalid_argument("IQM_MAP_SHADER is required for Vulkan maps");
            gpu_maps=std::make_unique<IQM::HDRVDP3::MapGPU>(instance,map_shader_env);
        }
        std::ofstream(export_dir / "map_backend.txt") << map_backend << '\n';
        std::ofstream(export_dir / "map_shader.txt") << (map_shader_env?map_shader_env:"") << '\n';
        const auto base_fft_text=options.baseFFTShader.string();const char* base_fft_env=base_fft_text.c_str();
        const bool gpu_base_fft=base_fft_env&&*base_fft_env;
        if(gpu_base_fft&&!gpu_maps) throw std::invalid_argument("GPU base FFT requires Vulkan maps");
        std::ofstream(export_dir / "base_frequency_backend.txt") << (gpu_base_fft?"vulkan_fft":"cpu_dft") << '\n';
        std::ofstream(export_dir / "base_fft_shader.txt") << (base_fft_env?base_fft_env:"") << '\n';

        std::cout << "Reference first pixel: "
                << reference_image.front() << std::endl;

        std::cout << "Test first pixel: "
                << test_image.front() << std::endl;

        const char* preprocessing_env="fft";
        const std::string preprocessing = preprocessing_env ? preprocessing_env : "none";
        if (preprocessing != "none" && preprocessing != "fft")
            throw std::invalid_argument("Unknown IQM_PREPROCESSING: " + preprocessing);
        if((viewing_ppd!=30.0||observer_age!=24.0)&&(preprocessing!="fft"||!gpu_maps))
            throw std::invalid_argument("Extended viewing parameters require reference FFT and Vulkan maps");
        const auto& fft_config=options.fft;
        auto reference_retinal = reference_image;
        auto test_retinal = test_image;
        if (preprocessing == "fft") {
            reference_retinal = compute_fft_filter_gpu(instance, reference_image, static_cast<float>(viewing_ppd),
                IQM::HDRVDP3::FFTFilterMode::OpticalMTF, fft_config, (export_dir / "ref_mtf_").string());
            test_retinal = compute_fft_filter_gpu(instance, test_image, static_cast<float>(viewing_ppd),
                IQM::HDRVDP3::FFTFilterMode::OpticalMTF, fft_config, (export_dir / "test_mtf_").string());
        }
        auto reference_adapt_input = reference_retinal;
        auto test_adapt_input = test_retinal;
        if (preprocessing == "fft") {
            if(gpu_maps) {
                reference_adapt_input=gpu_maps->adaptation_input(reference_retinal,spectral_matrix);
                test_adapt_input=gpu_maps->adaptation_input(test_retinal,spectral_matrix);
            } else for (std::size_t i=0; i<reference_adapt_input.size(); ++i) {
                reference_adapt_input[i] = reference_retinal[i] * static_cast<float>(spectral_matrix[0])
                    + reference_retinal[i] * static_cast<float>(spectral_matrix[1]);
                test_adapt_input[i] = test_retinal[i] * static_cast<float>(spectral_matrix[0])
                    + test_retinal[i] * static_cast<float>(spectral_matrix[1]);
            }
        }
        // Compute local adaptation from the same luminance image
        // used by the JND -> SPYR pipeline.
        auto reference_adaptation = preprocessing == "fft"
            ? compute_fft_filter_gpu(instance, reference_adapt_input, static_cast<float>(viewing_ppd), IQM::HDRVDP3::FFTFilterMode::LocalAdaptation, fft_config, (export_dir / "ref_la_").string())
            : throw std::logic_error("Quality API requires FFT preprocessing");
        auto test_adaptation = preprocessing == "fft"
            ? compute_fft_filter_gpu(instance, test_adapt_input, static_cast<float>(viewing_ppd), IQM::HDRVDP3::FFTFilterMode::LocalAdaptation, fft_config, (export_dir / "test_la_").string())
            : throw std::logic_error("Quality API requires FFT preprocessing");

        const float ref_reduction = gpu_maps?gpu_maps->pupil_trace(reference_adaptation,width64,height64,visual_area,observer_age)[3]:
            IQM::HDRVDP3::pupil_luminance_reduction_single(reference_adaptation, width64, height64, visual_area, observer_age);
        const float test_reduction = gpu_maps?gpu_maps->pupil_trace(test_adaptation,width64,height64,visual_area,observer_age)[3]:
            IQM::HDRVDP3::pupil_luminance_reduction_single(test_adaptation, width64, height64, visual_area, observer_age);
        std::cout << std::fixed << std::setprecision(15);
        std::cout << "Pupil Reference = " << ref_reduction << '\n';
        std::cout << "Pupil Test = " << test_reduction << '\n';

        std::vector<float> adaptation_map(reference_adaptation.size());

        if (reference_adaptation.size() != test_adaptation.size()) {
            throw std::runtime_error("Adaptation map dimensions mismatch");
        }

        if(gpu_maps) adaptation_map=gpu_maps->mean_adaptation(reference_adaptation,test_adaptation);
        else for (std::size_t i = 0; i < adaptation_map.size(); ++i) {
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
        const double ppd = viewing_ppd;
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

if(gpu_maps) {
    const auto responses=gpu_maps->jnd_response(reference_retinal,jnd_lut,ref_reduction,spectral_matrix);
    for(std::size_t i=0;i<jnd_image.size();++i) jnd_image[i]=responses[i].response;
} else for (std::size_t i = 0; i < input_data_64.size(); ++i) {
    jnd_image[i] = IQM::HDRVDP3::luminance_to_jnd_response_single(
        jnd_lut, reference_retinal[i], ref_reduction,spectral_matrix).response;
}

std::cout << std::fixed << std::setprecision(15);
std::cout << "JND Reference = " << jnd_image[0] << '\n';

std::cout << "JND transformed first pixel: "
          << jnd_image.front() << std::endl;
        std::cout << "SPYR validation directory: " << export_dir.string() << '\n';
        std::ofstream(export_dir / "pattern.txt") << pattern << '\n';
        std::ofstream(export_dir / "pupil_precision.txt") << "native_single\n";
        std::ofstream(export_dir / "preprocessing.txt") << preprocessing << '\n';
        const char* axis_env=fft_config.columnFirst?"column":"row";
        std::ofstream(export_dir / "fft_axis_order.txt") << (axis_env ? axis_env : "row") << '\n';
        const char* backend_env="reference";
        const std::string fft_backend=backend_env?backend_env:"vkfft";
        std::ofstream(export_dir / "fft_backend.txt") << fft_backend << '\n';
        const char* math_env=fft_config.referenceMathDirectory.c_str();
        std::ofstream(export_dir / "fft_math_shader_dir.txt") << (math_env?math_env:"") << '\n';
        std::ofstream(export_dir / "fft_inverse_storage.txt") << (fft_backend=="reference"?"column":"row") << '\n';
        const char* twiddle_env=fft_config.useTwiddleLUT?"lut":"native";
        std::ofstream(export_dir / "fft_twiddles.txt") << (twiddle_env ? twiddle_env : "native") << '\n';
        if (preprocessing == "fft") {
            save_reference((export_dir / "ref_retinal.txt").string(), reference_retinal, height64, width64);
            save_reference((export_dir / "test_retinal.txt").string(), test_retinal, height64, width64);
            save_reference((export_dir / "ref_adapt_input.txt").string(), reference_adapt_input, height64, width64);
            save_reference((export_dir / "test_adapt_input.txt").string(), test_adapt_input, height64, width64);
        }
        save_reference((export_dir / "ref_luminance.txt").string(), reference_image, height64, width64);
        save_reference((export_dir / "test_luminance.txt").string(), test_image, height64, width64);
        save_reference((export_dir / "ref_adaptation.txt").string(), reference_adaptation, height64, width64);
        save_reference((export_dir / "test_adaptation.txt").string(), test_adaptation, height64, width64);
        std::ofstream(export_dir / "pupil.txt") << std::setprecision(17)
            << ref_reduction << ' ' << test_reduction << ' '
            << visual_area << ' ' << observer_age << '\n';
        save_reference((export_dir / "ref_jnd.txt").string(), jnd_image, height64, width64);

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
            [&](const IQM::VulkanInstance& inst,
                IQM::HDRVDP3::CorrDn& cdn,
                const float* filter_coeffs,
                int filter_size,
                const std::vector<float>& input_data,
                unsigned w, unsigned h, int step,
                bool use_non_fma) {

                return run_corrdn_gpu(
                    inst, cdn, filter_coeffs, filter_size,
                    input_data, w, h, step, use_non_fma
                );
            }, (export_dir / "gpu_ref_band").string());

                // Convert test image to JND responses.
        std::vector<float> test_jnd_image(test_image.size());

        if(gpu_maps) {
            const auto responses=gpu_maps->jnd_response(test_retinal,jnd_lut,test_reduction,spectral_matrix);
            for(std::size_t i=0;i<test_jnd_image.size();++i) test_jnd_image[i]=responses[i].response;
        } else for (std::size_t i = 0; i < test_image.size(); ++i) {
            test_jnd_image[i] = IQM::HDRVDP3::luminance_to_jnd_response_single(
                jnd_lut, test_retinal[i], test_reduction,spectral_matrix).response;
        }

        std::cout << "JND Test = " << test_jnd_image[0] << '\n';
        save_reference((export_dir / "test_jnd.txt").string(), test_jnd_image, height64, width64);

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
                int step,
                bool use_non_fma
            ) {
                return run_corrdn_gpu(
                    inst, cdn, filter_coeffs, filter_size,
                    input_data, w, h, step, use_non_fma
                );
            }, (export_dir / "gpu_test_band").string()
        );

        // Remove the DC component from the residual lowpass bands.
        // Matches hdrvdp_visual_pathway.m in HDR-VDP-3.
        auto remove_residual_dc = [&](auto& pyramid) {
            auto& residual = pyramid.bands.back().data;

            if (residual.empty()) {
                return;
            }
            if(gpu_maps) { residual=gpu_maps->remove_residual_dc(residual); return; }

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

        // Debug: Compare SPYR bands against Octave
        std::cout << "\n=== C++ PYRAMID BAND VALUES ===\n";

        for (std::size_t b = 0; b < result.bands.size(); ++b) {
            const auto& ref_band = result.bands[b];
            const auto& tst_band = test_result.bands[b];

            auto print_band = [](const char* name, const auto& band) {
                double sum_abs = 0.0;
                double max_abs = 0.0;

                for (float value : band.data) {
                    const double abs_value =
                        std::abs(static_cast<double>(value));

                    sum_abs += abs_value;
                    max_abs = std::max(max_abs, abs_value);
                }

                const double mean_abs =
                    sum_abs / static_cast<double>(band.data.size());

                std::cout << name
                        << ": size=" << band.width << "x" << band.height
                        << ", first=" << band.data.front()
                        << ", meanAbs=" << mean_abs
                        << ", maxAbs=" << max_abs
                        << '\n';
            };

            std::cout << "Band " << (b + 1) << '\n';
            print_band("  Reference", ref_band);
            print_band("  Test", tst_band);
}

        auto resize_single_map=[&](const std::vector<float>& input,int h,int w,int nh,int nw) {
            return gpu_maps?gpu_maps->resize(input,h,w,nh,nw):IQM::HDRVDP3::resize_bicubic(input,h,w,nh,nw);
        };
        auto resize_csf_log=[&](const std::vector<float>& input,int h,int w,int nh,int nw) {
            auto result=resize_single_map(input,h,w,nh,nw);
            if(gpu_maps) return gpu_maps->clamp_log(result);
            for(float& value:result) value=std::clamp(value,-5.0f,5.0f);
            return result;
        };
        // Both-image mean adaptation is already computed above in single precision.
        std::vector<float> log_adaptation(adaptation_map.size());
        if(gpu_maps) log_adaptation=gpu_maps->log_adaptation(adaptation_map);
        else for (std::size_t i=0;i<adaptation_map.size();++i)
            log_adaptation[i]=std::log10(std::clamp(adaptation_map[i],1e-5f,1e5f));
        save_reference((export_dir / "csf_log_adaptation.txt").string(),log_adaptation,height64,width64);
        std::vector<std::vector<float>> csf_maps;
        csf_maps.reserve(result.bands.size());
        for (std::size_t b=0;b<result.bands.size();++b) {
            const auto& band=result.bands[b];
            const double frequency=IQM::HDRVDP3::compute_band_frequency(static_cast<int>(b)+1,viewing_ppd);
            const auto lut=IQM::HDRVDP3::create_csf_lut(frequency,{observer_age,true});
            auto resized=resize_csf_log(log_adaptation,height64,width64,band.height,band.width);
            std::vector<float> csf_map(resized.size());
            if(gpu_maps) csf_map=gpu_maps->sample_csf(resized,lut);
            else for (std::size_t i=0;i<resized.size();++i)
                csf_map[i]=IQM::HDRVDP3::sample_csf_lut_log_single(lut,resized[i]);
            save_reference((export_dir / ("csf_log_band"+std::to_string(b)+".txt")).string(),resized,band.height,band.width);
            save_reference((export_dir / ("csf_band"+std::to_string(b)+".txt")).string(),csf_map,band.height,band.width);
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
    std::vector<float> pooled_mask(mask.size(),0.0f);
    if(gpu_maps) {
        auto maps=gpu_maps->mutual_masking(ref_band.data,tst_band.data,ref_band.height,ref_band.width,static_cast<float>(normalization));
        mask=std::move(maps.raw); pooled_mask=std::move(maps.pooled);
    }

    double sum_mask = 0.0;
    double max_mask = 0.0;

    for (std::size_t i = 0; i < mask.size(); ++i) {
        double activity;
        if(gpu_maps) activity=mask[i];
        else {
        const double ref_value =
            std::abs(static_cast<double>(ref_band.data[i])) /
            normalization;

        const double test_value =
            std::abs(static_cast<double>(tst_band.data[i])) /
            normalization;

        activity = std::min(ref_value, test_value);

        mask[i] = static_cast<float>(activity);
        }

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

if(!gpu_maps) for(unsigned y=0;y<ref_band.height;++y) for(unsigned x=0;x<ref_band.width;++x) {
    float sum=0.0f;
    // Pinned Octave single conv2 on current square pyramid sizes: 32/64-row
    // bands use FMA, 8/16-row bands use separate multiply/add.
    for(int dx=-1;dx<=1;++dx) for(int dy=1;dy>=-1;--dy) {
        int nx=static_cast<int>(x)+dx, ny=static_cast<int>(y)+dy;
        if(nx>=0&&ny>=0&&nx<static_cast<int>(ref_band.width)&&ny<static_cast<int>(ref_band.height)) {
            const float value=mask[static_cast<std::size_t>(ny)*ref_band.width+nx];
            if(ref_band.height>=32) sum=std::fma(value,1.0f/9.0f,sum);
            else { const float product=value*(1.0f/9.0f); sum=sum+product; }
        }
    }
    pooled_mask[y*ref_band.width+x]=sum;
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

        save_reference((export_dir / ("mask_raw_band"+std::to_string(b)+".txt")).string(),mask,ref_band.height,ref_band.width);
        save_reference((export_dir / ("mask_pool_band"+std::to_string(b)+".txt")).string(),pooled_mask,ref_band.height,ref_band.width);
        mutual_masking_maps.push_back(std::move(pooled_mask));
}

        std::cout << "\n--- HDR-VDP-3 Masking Denominator ---" << std::endl;

const double masking_q = std::pow(10.0, 0.108934275615);
const double k_mask_self = std::pow(10.0, 1.41846291721);
const double k_mask_xn = std::pow(10.0, 0.136877512403);

std::vector<std::vector<float>> n_mask_maps;
n_mask_maps.reserve(mutual_masking_maps.size());

// Match the same default imresize used by the reference CSF maps.
auto resize_mask = [&](std::size_t source_band,unsigned target_width,unsigned target_height) {
    const auto& band=result.bands[source_band];
    auto resized=resize_single_map(mutual_masking_maps[source_band],band.height,band.width,target_height,target_width);
    if(!gpu_maps) for(float& value:resized) value=std::max(value,0.0f);
    return resized;
};
const std::size_t band_count = mutual_masking_maps.size();

for (std::size_t b = 0; b < band_count; ++b) {

    const unsigned width = result.bands[b].width;
    const unsigned height = result.bands[b].height;

    const auto& self_mask = mutual_masking_maps[b];

    std::vector<float> neighbor_mask(width * height, 0.0f);
    std::vector<float> previous_gpu(width*height,0.0f),next_gpu(width*height,0.0f);

    // Previous frequency band.
    if (b > 0) {
        const auto previous = resize_mask(b - 1, width, height);

        if(gpu_maps) previous_gpu=previous;
        else for (std::size_t i = 0; i < neighbor_mask.size(); ++i) {
            neighbor_mask[i] += previous[i];
        }
    }

    // Next frequency band, excluding the final lowpass band.
    if (b + 2 < band_count) {
        const auto next = resize_mask(b + 1, width, height);

        if(gpu_maps) next_gpu=next;
        else for (std::size_t i = 0; i < neighbor_mask.size(); ++i) {
            neighbor_mask[i] += next[i];
        }
    }

    std::vector<float> n_mask(width * height);
    if(gpu_maps) {
        neighbor_mask=gpu_maps->neighbor_mask(previous_gpu,next_gpu);
        n_mask=gpu_maps->masking_denominator(self_mask,neighbor_mask,static_cast<float>(masking_q),static_cast<float>(k_mask_self),static_cast<float>(k_mask_xn));
    }

    double sum_n_mask = 0.0;
    double max_n_mask = 0.0;

    for (std::size_t i = 0; i < n_mask.size(); ++i) {

        if(!gpu_maps) {
            const float self_term=static_cast<float>(k_mask_self)*std::pow(std::abs(self_mask[i]),static_cast<float>(masking_q));
            const float neighbor_term=static_cast<float>(k_mask_xn)*std::pow(std::abs(neighbor_mask[i]),static_cast<float>(masking_q));
            n_mask[i]=self_term+neighbor_term;
        }
        const float value=n_mask[i];
        sum_n_mask += value;
        max_n_mask = std::max(max_n_mask, static_cast<double>(value));
    }

    std::cout << "Band " << b
              << ": MeanNMask=" << sum_n_mask / n_mask.size()
              << ", MaxNMask=" << max_n_mask
              << std::endl;

    save_reference((export_dir / ("mask_neighbor_band"+std::to_string(b)+".txt")).string(),neighbor_mask,height,width);
    save_reference((export_dir / ("mask_denominator_band"+std::to_string(b)+".txt")).string(),n_mask,height,width);
    n_mask_maps.push_back(std::move(n_mask));
}

        std::cout << "\n--- HDR-VDP-3 Masked Difference Bands ---" << std::endl;

const double masking_p = std::pow(10.0, 0.3424);
const double psych_slope = std::pow(10.0, 0.34);
const double pf = psych_slope / masking_p;

std::vector<std::vector<float>> d_band_maps;
std::vector<std::vector<float>> quality_d_maps;
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
    std::vector<float> quality_d(ref.size());
    std::vector<float> prepared_difference;
    if(gpu_maps) prepared_difference=gpu_maps->normalized_difference(ref,tst,static_cast<float>(normalization));

    double base_noise=-1.0;
    if(b==result.bands.size()-1) {
        std::vector<float> difference=prepared_difference;
        if(!gpu_maps) {
            difference.resize(ref.size());
            for(std::size_t i=0;i<ref.size();++i) difference[i]=tst[i]/static_cast<float>(normalization)-ref[i]/static_cast<float>(normalization);
        }
        const auto& band=result.bands[b];
        const double base_ppd=IQM::HDRVDP3::compute_band_frequency(static_cast<int>(b)+1,viewing_ppd)*4;
        double freq;
        if(gpu_base_fft) {
            const auto selection=gpu_maps->base_selection(gpu_maps->base_spectrum(difference,base_fft_env));
            freq=IQM::HDRVDP3::dominant_frequency_from_index(selection.index,band.width,band.height,base_ppd);
            std::ofstream(export_dir / "base_gpu_selection.txt") << selection.index+1 << '\n';
        } else freq=IQM::HDRVDP3::dominant_frequency(difference,band.width,band.height,base_ppd);
        std::ofstream(export_dir / "base_selection_backend.txt") << (gpu_base_fft?"vulkan":"cpu") << '\n';
        const auto logs=resize_csf_log(log_adaptation,height64,width64,band.height,band.width);
        float mean_luminance;
        if(gpu_maps) mean_luminance=gpu_maps->base_adaptation(logs)[1];
        else {
            double sum=0;
            for(unsigned x=0;x<band.width;++x) for(unsigned y=0;y<band.height;++y) sum+=logs[y*band.width+x];
            const float mean_log=static_cast<float>(sum/static_cast<double>(logs.size()));
            mean_luminance=std::pow(10.0f,mean_log);
        }
        if(gpu_maps) {
            const auto trace=gpu_maps->base_csf_parameters({mean_luminance})[0];
            const std::array<float,4> parameters={trace[1],trace[2],trace[3],trace[4]};
            base_noise=1.0/IQM::HDRVDP3::evaluate_ncsf_parameters(freq,parameters,{observer_age,true});
            std::ofstream trace_file(export_dir / "base_csf_parameters.txt");
            trace_file << std::setprecision(17); for(float value:trace) trace_file << value << ' '; trace_file << '\n';
        } else base_noise=1.0/IQM::HDRVDP3::compute_ncsf_single_luminance(freq,mean_luminance,{observer_age,true});
        std::ofstream(export_dir / "base_csf_parameters_backend.txt") << (gpu_maps?"vulkan":"cpu") << '\n';
        std::ofstream(export_dir / "base_csf.txt") << std::setprecision(17) << freq << ' ' << mean_luminance << ' ' << base_noise << '\n';
    }
    double sum_abs_d=0,max_abs_d=0;
    if(gpu_maps) {
        const float neural=base_noise>=0?static_cast<float>(std::pow(base_noise,2*masking_p)):-1.0f;
        quality_d=gpu_maps->difference(prepared_difference,csf,nmask,static_cast<float>(masking_p),neural);
        d_map=gpu_maps->psychometric(quality_d,static_cast<float>(pf));
    }
    for(std::size_t i=0;i<ref.size();++i) {
        if(!gpu_maps) {
            const float difference=tst[i]/static_cast<float>(normalization)-ref[i]/static_cast<float>(normalization);
            d_map[i]=IQM::HDRVDP3::masked_difference(difference,csf[i],nmask[i],masking_p,pf,base_noise,&quality_d[i]);
        }
        const float value=d_map[i];
        sum_abs_d+=std::abs(static_cast<double>(value));
        max_abs_d=std::max(max_abs_d,std::abs(static_cast<double>(value)));
    }
    save_reference((export_dir / ("difference_band"+std::to_string(b)+".txt")).string(),d_map,result.bands[b].height,result.bands[b].width);
    save_reference((export_dir / ("quality_difference_band"+std::to_string(b)+".txt")).string(),quality_d,result.bands[b].height,result.bands[b].width);
    std::cout << "Band " << b
              << ": MeanAbsD=" << sum_abs_d / d_map.size()
              << ", MaxAbsD=" << max_abs_d
              << std::endl;

    d_band_maps.push_back(std::move(d_map));
    quality_d_maps.push_back(std::move(quality_d));
}

        std::cout << "\n--- HDR-VDP-3 Preliminary Quality Score ---"
          << std::endl;

std::vector<double> comparison_mask(reference_image.size());
if(gpu_maps) comparison_mask=gpu_maps->comparison_mask(reference_image,test_image);
else for(std::size_t i=0;i<comparison_mask.size();++i)
    comparison_mask[i]=(std::abs(test_image[i]-reference_image[i])/reference_image[i]>0.001f)?1.0:0.0;
std::vector<float> band_errors;
std::ofstream(export_dir / "comparison_mask_resize_backend.txt") << (gpu_maps?"vulkan":"cpu") << '\n';
for(std::size_t b=0;b<d_band_maps.size();++b) {
    const auto& band=result.bands[b];
    const auto mask=gpu_maps?gpu_maps->resize_comparison_mask(comparison_mask,height64,width64,band.height,band.width):
        IQM::HDRVDP3::resize_bicubic(comparison_mask,height64,width64,band.height,band.width);
    save_reference((export_dir / ("quality_mask_band"+std::to_string(b)+".txt")).string(),mask,band.height,band.width);
    band_errors.push_back(gpu_maps?gpu_maps->quality_band_error(quality_d_maps[b],mask,band.width,band.height):
        IQM::HDRVDP3::quality_band_error(quality_d_maps[b],mask,band.width,band.height));
}
const auto scores=gpu_maps?gpu_maps->quality_scores(band_errors):IQM::HDRVDP3::quality_scores(band_errors);
std::ofstream quality_file(export_dir / "quality.txt");
quality_file << std::setprecision(17);
for(float error:band_errors) quality_file << error << ' ';
quality_file << '\n' << scores.error << ' ' << scores.Q << ' ' << scores.Q_JOD << '\n';
std::cout << "Q_err=" << scores.error << "\nQ=" << scores.Q << "\nQ_JOD=" << scores.Q_JOD << '\n';

    return {scores,std::move(band_errors)};
}
} // namespace IQM::HDRVDP3
