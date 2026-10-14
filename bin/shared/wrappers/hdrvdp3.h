/*
 * Image Quality Metrics - HDR-VDP-3 Test Wrapper
 * Petr Volf - 2026
 */

#ifndef IQM_BIN_HDRVDP3_H
#define IQM_BIN_HDRVDP3_H

#include <IQM/hdrvdp3/corrdn.h>
#include <IQM/hdrvdp3/local_adapt.h>
#include "../../shared/vulkan.h"
#include "../../shared/vulkan_res.h"
#include "../../shared/io.h"
#include "../../IQM/args.h"

namespace IQM::Bin {
    struct HDRVDP3TestResources {
        vk::raii::Buffer stgInput = VK_NULL_HANDLE;
        vk::raii::DeviceMemory stgInputMemory = VK_NULL_HANDLE;
        vk::raii::Buffer stgOutput = VK_NULL_HANDLE;
        vk::raii::DeviceMemory stgOutputMemory = VK_NULL_HANDLE;

        std::shared_ptr<VulkanImage> imageInput;
        std::shared_ptr<VulkanImage> imageOutput;

        vk::raii::Semaphore uploadDone = VK_NULL_HANDLE;
        vk::raii::Semaphore computeDone = VK_NULL_HANDLE;
        vk::raii::Fence transferFence = VK_NULL_HANDLE;
    };

    struct HDRVDP3TestResult {
        std::vector<float> outputData;
    };

    // Local Adaptation test resources (need additional temp images)
    struct LocalAdaptTestResources {
        vk::raii::Buffer stgInput = VK_NULL_HANDLE;
        vk::raii::DeviceMemory stgInputMemory = VK_NULL_HANDLE;
        vk::raii::Buffer stgOutput = VK_NULL_HANDLE;
        vk::raii::DeviceMemory stgOutputMemory = VK_NULL_HANDLE;

        std::shared_ptr<VulkanImage> imageInput;       // 64x64
        std::shared_ptr<VulkanImage> imageLogClamp;    // 64x64
        std::shared_ptr<VulkanImage> imagePadded;      // 128x128
        std::shared_ptr<VulkanImage> imageGaussH;      // 128x128
        std::shared_ptr<VulkanImage> imageGaussV;      // 128x128
        std::shared_ptr<VulkanImage> imageOutput;      // 64x64

        vk::raii::Semaphore uploadDone = VK_NULL_HANDLE;
        vk::raii::Semaphore computeDone = VK_NULL_HANDLE;
        vk::raii::Fence transferFence = VK_NULL_HANDLE;
    };

    struct LocalAdaptTestResult {
        std::vector<float> outputData;
    };

    void hdrvdp3_test_run(const IQM::Bin::Args& args, const IQM::VulkanInstance& instance);
    HDRVDP3TestResources hdrvdp3_test_init_res(unsigned width, unsigned height, const IQM::VulkanInstance& instance);
    void hdrvdp3_test_upload(const IQM::VulkanInstance& instance, const HDRVDP3TestResources& res, const std::vector<float>& inputData);
    HDRVDP3TestResult hdrvdp3_test_copy_back(const IQM::VulkanInstance& instance, const HDRVDP3TestResources& res, unsigned outWidth, unsigned outHeight);

    // Local Adaptation functions
    void local_adapt_test_run(const IQM::Bin::Args& args, const IQM::VulkanInstance& instance);
    std::vector<float> compute_local_adaptation_gpu(
    const IQM::VulkanInstance& instance,
    const std::vector<float>& luminance,
    float ppd
);
    LocalAdaptTestResources local_adapt_test_init_res(const IQM::VulkanInstance& instance);
    void local_adapt_test_upload(const IQM::VulkanInstance& instance, const LocalAdaptTestResources& res, const std::vector<float>& inputData);
    LocalAdaptTestResult local_adapt_test_copy_back(const IQM::VulkanInstance& instance, const LocalAdaptTestResources& res);
}

#endif // IQM_BIN_HDRVDP3_H