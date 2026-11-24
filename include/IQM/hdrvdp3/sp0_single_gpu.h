#pragma once
#include <IQM/hdrvdp3/sp0_single.h>
#include <IQM/hdrvdp3/pq_display_single_gpu.h>
#include <vulkan/vulkan_raii.hpp>
#include <cstdint>
#include <string>
#include <vector>
namespace IQM { class VulkanInstance; }
namespace IQM::HDRVDP3 {
struct SP0PyramidGpuSingle {
  std::vector<GpuFloatBuffer> bands;
  GpuFloatBuffer baseMean;
  unsigned levels=0;
  uint32_t dispatches=0;
  uint32_t hostReadbacks=0; // band readbacks + base mean readback
  SP0PyramidSingle host;
};
SP0PyramidGpuSingle build_sp0_pyramid_single_gpu(const IQM::VulkanInstance& inst,
    const std::string& corrdnShader,const std::string& baseMeanShader,
    vk::Buffer jndResponse,unsigned w,unsigned h,double ppd,bool readHost=true);
}