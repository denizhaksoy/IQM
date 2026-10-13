/* HDR-VDP-3 LocalAdaptFFT image-to-image integration test.
 * Separate from the validated buffer-based local_adapt_fft_test.cpp.
 * Assumes execution from the IQM repository root (shader paths are relative).
 */
#include <IQM/hdrvdp3/local_adapt.h>
#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace IQM::Bin;
using namespace IQM::GPU;
using namespace IQM::HDRVDP3;

namespace {
constexpr uint32_t W = 64, H = 64;
constexpr vk::DeviceSize Bytes = W * H * sizeof(float);

uint32_t memoryType(const vk::raii::PhysicalDevice& gpu, uint32_t bits,
                    vk::MemoryPropertyFlags wanted) {
    const auto props = gpu.getMemoryProperties();
    for (uint32_t i = 0; i < props.memoryTypeCount; ++i) {
        if ((bits & (1u << i)) &&
            (props.memoryTypes[i].propertyFlags & wanted) == wanted) return i;
    }
    throw std::runtime_error("No suitable Vulkan memory type");
}

struct ImageResource {
    // Memory must outlive the bound image; members are destroyed in reverse order.
    vk::raii::DeviceMemory memory = nullptr;
    vk::raii::Image image = nullptr;
    vk::raii::ImageView view = nullptr;
};

ImageResource makeImage(const vk::raii::Device& dev,
                        const vk::raii::PhysicalDevice& gpu) {
    ImageResource r;
    vk::ImageCreateInfo ci{};
    ci.imageType = vk::ImageType::e2D;
    ci.format = vk::Format::eR32Sfloat;
    ci.extent = vk::Extent3D{W, H, 1};
    ci.mipLevels = 1;
    ci.arrayLayers = 1;
    ci.samples = vk::SampleCountFlagBits::e1;
    ci.tiling = vk::ImageTiling::eOptimal;
    ci.usage = vk::ImageUsageFlagBits::eStorage |
               vk::ImageUsageFlagBits::eTransferDst |
               vk::ImageUsageFlagBits::eTransferSrc;
    ci.sharingMode = vk::SharingMode::eExclusive;
    ci.initialLayout = vk::ImageLayout::eUndefined;
    r.image = vk::raii::Image(dev, ci);
    const auto req = r.image.getMemoryRequirements();
    vk::MemoryAllocateInfo alloc{};
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = memoryType(gpu, req.memoryTypeBits,
                                       vk::MemoryPropertyFlagBits::eDeviceLocal);
    r.memory = vk::raii::DeviceMemory(dev, alloc);
    r.image.bindMemory(*r.memory, 0);
    vk::ImageViewCreateInfo vi{};
    vi.image = *r.image;
    vi.viewType = vk::ImageViewType::e2D;
    vi.format = vk::Format::eR32Sfloat;
    vi.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    vi.subresourceRange.levelCount = 1;
    vi.subresourceRange.layerCount = 1;
    r.view = vk::raii::ImageView(dev, vi);
    return r;
}

void imageBarrier(vk::CommandBuffer cmd, vk::Image image,
                  vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                  vk::AccessFlags srcAccess, vk::AccessFlags dstAccess,
                  vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage) {
    vk::ImageMemoryBarrier b{};
    b.srcAccessMask = srcAccess;
    b.dstAccessMask = dstAccess;
    b.oldLayout = oldLayout;
    b.newLayout = newLayout;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = image;
    b.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    b.subresourceRange.levelCount = 1;
    b.subresourceRange.layerCount = 1;
    cmd.pipelineBarrier(srcStage, dstStage, {}, {}, {}, {b});
}

std::vector<float> referenceData(const char* path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error(std::string("Cannot open Octave reference: ") + path);
    std::vector<float> data(W * H);
    for (float& x : data) {
        if (!(in >> x)) throw std::runtime_error("Incomplete Octave reference");
    }
    return data;
}
} // namespace

int main() {
    try {
        VulkanInstance instance(true);
        const auto& device = *instance.device();
        const auto& gpu = *instance.physicalDevice();
        const auto& queue = *instance.queue();
        const auto& pool = *instance.cmdPool();
        const auto& cmdBuf = *instance.cmdBuf();
        vk::CommandBuffer cmd = *cmdBuf;

        auto input = makeImage(device, gpu);
        auto output = makeImage(device, gpu);
        auto [upload, uploadMem] = VulkanResource::createBuffer(
            device, gpu, Bytes, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent);
        upload.bindMemory(*uploadMem, 0);
        auto [readback, readbackMem] = VulkanResource::createBuffer(
            device, gpu, Bytes, vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent);
        readback.bindMemory(*readbackMem, 0);

        std::vector<float> source(W * H);
        const float n = static_cast<float>(W * H);
        for (uint32_t y = 0; y < H; ++y)
            for (uint32_t x = 0; x < W; ++x)
                source[y * W + x] = (static_cast<float>(x * H + y + 1) / n) * 100.0f;
        void* mapped = uploadMem.mapMemory(0, Bytes);
        std::memcpy(mapped, source.data(), static_cast<size_t>(Bytes));
        uploadMem.unmapMemory();

        LocalAdaptFFT fft(device, gpu);
        vk::CommandBufferBeginInfo begin{};
        begin.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        cmdBuf.reset({});
        cmdBuf.begin(begin);
        imageBarrier(cmd, *input.image, vk::ImageLayout::eUndefined,
                     vk::ImageLayout::eTransferDstOptimal, {},
                     vk::AccessFlagBits::eTransferWrite,
                     vk::PipelineStageFlagBits::eTopOfPipe,
                     vk::PipelineStageFlagBits::eTransfer);
        imageBarrier(cmd, *output.image, vk::ImageLayout::eUndefined,
                     vk::ImageLayout::eGeneral, {},
                     vk::AccessFlagBits::eShaderWrite,
                     vk::PipelineStageFlagBits::eTopOfPipe,
                     vk::PipelineStageFlagBits::eComputeShader);
        vk::BufferImageCopy region{};
        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = vk::Extent3D{W, H, 1};
        cmd.copyBufferToImage(*upload, *input.image,
                              vk::ImageLayout::eTransferDstOptimal, {region});
        imageBarrier(cmd, *input.image, vk::ImageLayout::eTransferDstOptimal,
                     vk::ImageLayout::eGeneral,
                     vk::AccessFlagBits::eTransferWrite,
                     vk::AccessFlagBits::eShaderRead,
                     vk::PipelineStageFlagBits::eTransfer,
                     vk::PipelineStageFlagBits::eComputeShader);

        LocalAdaptFFTInput args{};
        args.device = &device;
        args.physicalDevice = &gpu;
        args.cmdBuf = &cmdBuf;
        args.cmdPool = &pool;
        args.queue = &queue;
        args.ivInput = &input.view;
        args.ivOutput = &output.view;
        args.ppd = 30.0f;
        fft.compute(args);

        imageBarrier(cmd, *output.image, vk::ImageLayout::eGeneral,
                     vk::ImageLayout::eTransferSrcOptimal,
                     vk::AccessFlagBits::eShaderWrite,
                     vk::AccessFlagBits::eTransferRead,
                     vk::PipelineStageFlagBits::eComputeShader,
                     vk::PipelineStageFlagBits::eTransfer);
        cmd.copyImageToBuffer(*output.image, vk::ImageLayout::eTransferSrcOptimal,
                              *readback, {region});
        vk::BufferMemoryBarrier hostBarrier{};
        hostBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        hostBarrier.dstAccessMask = vk::AccessFlagBits::eHostRead;
        hostBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        hostBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        hostBarrier.buffer = *readback;
        hostBarrier.size = Bytes;
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                            vk::PipelineStageFlagBits::eHost, {}, {}, {hostBarrier}, {});
        cmdBuf.end();

        vk::raii::Fence fence(device, vk::FenceCreateInfo{});
        vk::CommandBuffer raw = *cmdBuf;
        vk::SubmitInfo submit{};
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &raw;
        queue.submit({submit}, *fence);
        const auto waitResult = device.waitForFences({*fence}, VK_TRUE,
                                                     std::numeric_limits<uint64_t>::max());
        if (waitResult != vk::Result::eSuccess)
            throw std::runtime_error("GPU fence wait failed");

        std::vector<float> result(W * H);
        mapped = readbackMem.mapMemory(0, Bytes);
        std::memcpy(result.data(), mapped, static_cast<size_t>(Bytes));
        readbackMem.unmapMemory();
        const auto reference = referenceData("C:/temp/oct_la_verified.txt");
        double sumSquared = 0.0;
        float maxAbs = 0.0f;
        bool finite = true;
        for (size_t i = 0; i < result.size(); ++i) {
            if (!std::isfinite(result[i])) { finite = false; break; }
            const float err = std::abs(result[i] - reference[i]);
            maxAbs = std::max(maxAbs, err);
            sumSquared += static_cast<double>(err) * err;
        }
        const double rmse = std::sqrt(sumSquared / result.size());
        std::cout << std::fixed << std::setprecision(8)
                  << "LocalAdaptFFT image integration vs Octave\n"
                  << "MaxAbsErr: " << maxAbs << "\n"
                  << "RMSE:      " << rmse << "\n"
                  << "Corners: TL=" << result[0]
                  << " TR=" << result[W - 1]
                  << " BL=" << result[(H - 1) * W]
                  << " BR=" << result.back() << "\n";
        const bool passed = finite && maxAbs < 1e-3f && rmse < 1e-4;
        std::cout << (passed ? "PASSED" : "FAILED") << '\n';
        return passed ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "Integration test error: " << e.what() << '\n';
        return 1;
    }
}
