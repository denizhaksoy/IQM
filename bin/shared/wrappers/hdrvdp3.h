/*
 * Image Quality Metrics - HDR-VDP-3 Test Wrapper
 * Petr Volf - 2026
 */

#ifndef IQM_BIN_HDRVDP3_H
#define IQM_BIN_HDRVDP3_H

#include <IQM/hdrvdp3/corrdn.h>
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

    void hdrvdp3_test_run(const IQM::Bin::Args& args, const IQM::VulkanInstance& instance);
    HDRVDP3TestResources hdrvdp3_test_init_res(unsigned width, unsigned height, const IQM::VulkanInstance& instance);
    void hdrvdp3_test_upload(const IQM::VulkanInstance& instance, const HDRVDP3TestResources& res, const std::vector<float>& inputData);
    HDRVDP3TestResult hdrvdp3_test_copy_back(const IQM::VulkanInstance& instance, const HDRVDP3TestResources& res, unsigned outWidth, unsigned outHeight);
}

#endif // IQM_BIN_HDRVDP3_H