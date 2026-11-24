#pragma once
#include <span>
#include <string>
#include <vector>
namespace IQM::Bin { class VulkanInstance; }
namespace IQM::HDRVDP3 {
// GPU-resident L+M addition of two clamped L/M channels.
// Input planes are contiguous Float32 [1e-8, 1e10] values.
// Output plane is contiguous Float32 L+M sum, same precision.
// No host image transfers, submit, wait, or CPU image arithmetic inside the record chain.
struct LmsrLmBuffers{
    vk::Buffer lHandle=nullptr;vk::Buffer mHandle=nullptr;vk::Buffer outHandle=nullptr;
    vk::DeviceSize lBytes=0,mBytes=0,outBytes=0;
    uint32_t pixels=0;
};
// Record L+M addition into outHandle using L and M buffers.
// All buffers must be device-local, non-host-visible, and of matching size.
// Caller owns lifetime; record() performs only GPU compute, no map/readback/submit/wait.
void record_lmsr_lm_gpu(vk::CommandBuffer cmd,
    const LmsrLmBuffers& buffers);
}
// Alias for backward compatibility / test use
namespace IQM::HDRVDP3 {
void record_lmsr_lm_gpu(vk::CommandBuffer cmd, const std::vector<float>& l, const std::vector<float>& m, std::vector<float>& out, const IQM::Bin::VulkanInstance& instance);
}