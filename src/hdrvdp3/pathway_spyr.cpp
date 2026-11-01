#include <IQM/hdrvdp3/pathway_spyr.h>
#include <vulkan_res.h>
#include <IQM/base/vulkan_runtime.h>
#include <vulkan.h>
#include <algorithm>
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
PathwaySPYRResult
#if defined(IQM_HDRVDP3_SQUARE_SPYR_DIAGNOSTIC)
run_square_spyr_diagnostic(
#else
run_pathway_spyr(
#endif
const IQM::VulkanInstance& instance,const std::vector<float>& reference,const std::vector<float>& test,unsigned w,unsigned h,const PathwaySPYRFilters& filters,const PathwaySPYRAssets& assets){
#if defined(IQM_HDRVDP3_SQUARE128_SPYR_DIAGNOSTIC)
 constexpr unsigned baseCount=256;const bool shapeOK=h==128&&w==128;
#elif defined(IQM_HDRVDP3_GENERAL_SPYR_DIAGNOSTIC)
 const unsigned baseCount=(h/8)*(w/8);const bool shapeOK=(h==80||h==96)&&(w==80||w==96);
#elif defined(IQM_HDRVDP3_SQUARE_SPYR_DIAGNOSTIC)
 constexpr unsigned baseCount=144;const bool shapeOK=h==96&&w==96;
#else
 const unsigned baseCount=(h/8)*(w/8);const bool shapeOK=(h==128&&w==128)||(h==80&&w==96)||(h==80&&w==80)||(h==96&&w==80)||(h==96&&w==96);
#endif
 if(!shapeOK||reference.size()!=size_t(h)*w||test.size()!=reference.size())throw std::invalid_argument("Unvalidated SPYR shape");
 std::vector<uint32_t> input;for(const auto* v:{&reference,&test})for(float x:*v){if(!std::isfinite(x))throw std::invalid_argument("Nonfinite SPYR input");input.push_back(std::bit_cast<uint32_t>(x));}
 const auto& device=*instance.device();const bool chain=true,bands=true;
    const auto props=instance.physicalDevice()->getProperties2<vk::PhysicalDeviceProperties2,vk::PhysicalDeviceFloatControlsProperties>();
    const auto& controls=props.get<vk::PhysicalDeviceFloatControlsProperties>();
    if(!controls.shaderDenormPreserveFloat32||!controls.shaderRoundingModeRTEFloat32)throw std::runtime_error("Float32 modes unsupported");
    auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
    auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(32));
    std::vector<vk::raii::ShaderModule> modules;std::vector<vk::raii::Pipeline> pipelines;
    for(int i=0;i<2;++i){modules.push_back(VulkanRuntime::createShaderModule(device,code(i==0?assets.fma:assets.separate)));pipelines.push_back(VulkanRuntime::createComputePipeline(device,modules.back(),layout));}
    auto residualSetLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
    auto residualLayout=VulkanRuntime::createPipelineLayout(device,{*residualSetLayout},VulkanRuntime::createPushConstantRange(16));
    for(int i=0;i<3;++i){modules.push_back(VulkanRuntime::createShaderModule(device,code(i==0?assets.mean:i==1?assets.difference:assets.residual)));pipelines.push_back(VulkanRuntime::createComputePipeline(device,modules.back(),residualLayout));}
    const auto lo0=filters.lo0,lo=filters.lo;
    if(lo0.size()!=49||lo.size()!=169)throw std::runtime_error("Unexpected sp0Filters geometry");
    auto filter0=buffer(instance,196),filter=buffer(instance,676);
    auto upload=[](Buffer& b,const std::vector<uint32_t>& v){void* p=b.memory.mapMemory(0,v.size()*4);std::memcpy(p,v.data(),v.size()*4);b.memory.unmapMemory();};
    upload(filter0,lo0);upload(filter,lo);
    auto highFilter=buffer(instance,324),orientFilter=buffer(instance,324);
    {auto high=filters.high,orient=filters.orientation;if(high.size()!=81||orient.size()!=81)throw std::runtime_error("Bad band filter shape");upload(highFilter,high);upload(orientFilter,orient);}

 const unsigned batches=2,lowStages=8;
 std::array<unsigned,9> widths{w,w,w/2,w/4,w/8,w,w,w/2,w/4},heights{h,h,h/2,h/4,h/8,h,h,h/2,h/4};
 std::array<size_t,11> counts{};std::vector<Buffer> buffers;
 for(unsigned j=0;j<9;++j){buffers.push_back(buffer(instance,2*widths[j]*heights[j]*4));if(j)counts[j-1]=2*widths[j]*heights[j];}
 counts[8]=2*(baseCount+1);counts[9]=2*baseCount;counts[10]=3*baseCount+1;
 for(unsigned j=8;j<11;++j)buffers.push_back(buffer(instance,unsigned(counts[j]*4)));
 upload(buffers[0],input);
        const unsigned stages=lowStages+(chain?3:0);
        auto pool=VulkanRuntime::createDescPool(device,stages,{{vk::DescriptorType::eStorageBuffer,lowStages*3+(chain?6u:0u)}});
        std::vector<vk::DescriptorSetLayout> raw(lowStages,*setLayout);if(chain)for(unsigned j=0;j<3;++j)raw.push_back(*residualSetLayout);
        auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=stages,.pSetLayouts=raw.data()});
        const unsigned inputs[]={0,1,2,3,0,1,2,3};
        for(unsigned j=0;j<lowStages;++j){const unsigned in=inputs[j];const std::vector<vk::DescriptorBufferInfo> a{{*buffers[in].handle,0,batches*widths[in]*heights[in]*4}},b{{*buffers[j+1].handle,0,batches*widths[j+1]*heights[j+1]*4}},c{{j==0?*filter0.handle:j<4?*filter.handle:j==4?*highFilter.handle:*orientFilter.handle,0,j==0?196u:j<4?676u:324u}};
            device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[j],0,a),VulkanRuntime::createWriteSet(*sets[j],1,b),VulkanRuntime::createWriteSet(*sets[j],2,c)},{});}
        if(chain)for(unsigned j=lowStages;j<stages;++j){const unsigned in=j==lowStages?4:j;const auto bytes=j==lowStages?counts[3]*4:counts[j-1]*4;const std::vector<vk::DescriptorBufferInfo> a{{*buffers[in].handle,0,bytes}},b{{*buffers[j+1].handle,0,counts[j]*4}};device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[j],0,a),VulkanRuntime::createWriteSet(*sets[j],1,b)},{});}
        instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
        instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
        struct Push{uint32_t w,h,ow,oh,n,step,normalize,reserved;};
        for(unsigned j=0;j<lowStages;++j){
            const unsigned in=inputs[j];
            instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[j]},{});
            instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,Push{widths[in],heights[in],widths[j+1],heights[j+1],j==0?7u:j<4?13u:9u,j==0||j>=4?1u:2u,j==3?1u:j>=5?j-3:0u,batches});
            for(unsigned variant=0;variant<2;++variant){instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipelines[variant]);instance.cmdBuf()->dispatch((batches*widths[j+1]*heights[j+1]+63)/64,1,1);}
            vk::MemoryBarrier barrier{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eHostRead};
            instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eHost,{}, {barrier},{},{});
        }
{instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);instance.cmdBuf()->reset();}
 const auto* base=static_cast<const float*>(buffers[4].memory.mapMemory(0,2*baseCount*4));
 bool valid=true;for(unsigned b=0;b<2;++b){int mn=1000,mx=-1000;for(unsigned k=0;k<baseCount;++k){const float v=base[b*baseCount+k];if(!std::isfinite(v)){valid=false;continue;}if(v!=0){const int e=std::ilogb(std::abs(v));mn=std::min(mn,e);mx=std::max(mx,e);}}if(mx!=-1000&&(mn < -90||mx>100||mx-mn>16))valid=false;}
 buffers[4].memory.unmapMemory();if(!valid)throw std::runtime_error("Actual SPYR base exceeds mean domain");
 instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        if(chain)for(unsigned j=lowStages;j<stages;++j){
            struct ResidualPush{uint32_t groups,h,w,reserved;};
            instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipelines[j-lowStages+2]);
            instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*residualLayout,0,{*sets[j]},{});
            unsigned second=j==lowStages&&assets.variableMeanCount?baseCount:h/8;
#if defined(IQM_HDRVDP3_GENERAL_MEAN_DIAGNOSTIC)
            // Isolated alternate mean ABI; production v1/v2 payloads stay pinned.
            if(j==lowStages)second=baseCount;
#endif
            instance.cmdBuf()->pushConstants<ResidualPush>(*residualLayout,vk::ShaderStageFlagBits::eCompute,0,ResidualPush{j==lowStages?2u:1u,second,w/8,0});
            instance.cmdBuf()->dispatch(j==lowStages+1?(baseCount+63)/64:1,1,1);
            vk::MemoryBarrier barrier{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eHostRead};
            instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eHost,{}, {barrier},{},{});
        }
        instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});
        instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);

 PathwaySPYRResult out;
 for(unsigned j=0;j<11;++j){out.stages[j].resize(counts[j]);const auto* data=static_cast<const uint32_t*>(buffers[j+1].memory.mapMemory(0,counts[j]*4));std::memcpy(out.stages[j].data(),data,counts[j]*4);buffers[j+1].memory.unmapMemory();}
 instance.cmdBuf()->reset();
 for(unsigned k=0;k<2*baseCount;++k){const float v=std::bit_cast<float>(out.stages[10][k]);if(!std::isfinite(v)||(v!=0&&(std::ilogb(std::abs(v)) < -90||std::ilogb(std::abs(v))>100)))throw std::runtime_error("Actual residual exceeds norm domain");}
 return out;
}
}
