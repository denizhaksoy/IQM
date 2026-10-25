#include <IQM/hdrvdp3/pathway_pool.h>
#include <vulkan_res.h>
#include <IQM/base/vulkan_runtime.h>
#include <vulkan.h>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <memory>
#include <stdexcept>
using IQM::GPU::VulkanRuntime;
namespace {
struct Buffer{vk::raii::DeviceMemory memory=nullptr;vk::raii::Buffer handle=nullptr;};
static Buffer buffer(const IQM::VulkanInstance& instance,unsigned bytes){
    auto pair=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),bytes,
        vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
    Buffer b;b.handle=std::move(pair.first);b.memory=std::move(pair.second);b.handle.bindMemory(b.memory,0);return b;
}
static std::vector<uint32_t> code(const std::filesystem::path& path){
    std::ifstream f(path,std::ios::binary|std::ios::ate);auto length=f.tellg();if(length<=0||length%4)throw std::runtime_error("Invalid SPIR-V");
    std::vector<uint32_t> v(size_t(length)/4);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),length);
    if(!f||v[0]!=0x07230203u)throw std::runtime_error("Invalid shader words");return v;
}
}
namespace IQM::HDRVDP3 {
PathwayPoolResult run_pathway_pool(const IQM::VulkanInstance& instance,const std::vector<float>& reference,const std::vector<float>& test,unsigned w,unsigned h,const PathwayPoolAssets& assets){
 if(!((h==80&&w==96)||(h==96&&w==80)||(h==40&&w==48)||(h==48&&w==40)||(h==20&&w==24)||(h==24&&w==20)||(h==10&&w==12)||(h==12&&w==10))||reference.size()!=size_t(h)*w||test.size()!=reference.size())throw std::invalid_argument("Unvalidated pool shape");
 std::vector<uint32_t> input;for(const auto* band:{&reference,&test})for(float v:*band){if(!std::isfinite(v))throw std::invalid_argument("Nonfinite pool band");input.push_back(std::bit_cast<uint32_t>(v));}
 const unsigned n=h*w,bytes=2*n*4;const auto& device=*instance.device();
    const auto props=instance.physicalDevice()->getProperties2<vk::PhysicalDeviceProperties2,vk::PhysicalDeviceFloatControlsProperties>();const auto& controls=props.get<vk::PhysicalDeviceFloatControlsProperties>();
    if(!controls.shaderDenormPreserveFloat32||!controls.shaderRoundingModeRTEFloat32)throw std::runtime_error("Float32 modes unsupported");
    auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
    auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(16));
    std::vector<vk::raii::ShaderModule> modules;std::vector<vk::raii::Pipeline> pipelines;
    for(int j=0;j<2;++j){modules.push_back(VulkanRuntime::createShaderModule(device,code(j==0?assets.fma:assets.separate)));pipelines.push_back(VulkanRuntime::createComputePipeline(device,modules.back(),layout));}
        auto src=buffer(instance,bytes),dst=buffer(instance,bytes);void* ptr=src.memory.mapMemory(0,bytes);std::memcpy(ptr,input.data(),bytes);src.memory.unmapMemory();
        auto pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,2}});const vk::DescriptorSetLayout raw=*setLayout;
        auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});
        const std::vector<vk::DescriptorBufferInfo> a{{*src.handle,0,bytes}},b{{*dst.handle,0,bytes}};
        device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,a),VulkanRuntime::createWriteSet(*sets[0],1,b)},{});
        instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
        instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
        instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});
        struct Push{uint32_t count,w,h,reserved;};instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,Push{n,w,h,0});
        for(unsigned j=0;j<2;++j){instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipelines[j]);instance.cmdBuf()->dispatch((n+63)/64,1,1);}
        vk::MemoryBarrier after{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};
        instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eHost,{}, {after},{},{});
        instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);

 const auto* actual=static_cast<const float*>(dst.memory.mapMemory(0,bytes));
 PathwayPoolResult result;result.raw.assign(actual,actual+n);result.pooled.assign(actual+n,actual+2*n);dst.memory.unmapMemory();instance.cmdBuf()->reset();return result;
}
}
