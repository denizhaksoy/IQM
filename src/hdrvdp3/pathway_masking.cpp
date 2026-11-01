#include <IQM/hdrvdp3/pathway_masking.h>
#include <IQM/hdrvdp3/map_gpu.h>
#include <IQM/base/vulkan_runtime.h>
#include <vulkan_res.h>
#include <vulkan.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>
namespace {
static std::vector<float> runFloatKernel(const IQM::VulkanInstance& instance,const std::string& shader,const std::vector<float>& input,size_t outputCount,uint32_t groups,uint32_t count=0){
    using IQM::GPU::VulkanRuntime;const auto& device=*instance.device();
    std::ifstream file(shader,std::ios::binary|std::ios::ate);const auto length=file.tellg();if(length<=0||length%4)throw std::runtime_error("Bad product shader");std::vector<uint32_t> code(size_t(length)/4);file.seekg(0);file.read(reinterpret_cast<char*>(code.data()),length);if(!file||code[0]!=0x07230203u)throw std::runtime_error("Bad SPIR-V");
    auto module=VulkanRuntime::createShaderModule(device,code);auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(16));auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
    const unsigned bytes=unsigned(outputCount*4),inputBytes=unsigned(input.size()*4);auto src=IQM::Bin::VulkanResource::createBuffer(device,*instance.physicalDevice(),inputBytes,vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);auto dst=IQM::Bin::VulkanResource::createBuffer(device,*instance.physicalDevice(),bytes,vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);src.first.bindMemory(src.second,0);dst.first.bindMemory(dst.second,0);
    void* mapped=src.second.mapMemory(0,inputBytes);std::memcpy(mapped,input.data(),inputBytes);src.second.unmapMemory();
    auto pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,2}});const vk::DescriptorSetLayout raw=*setLayout;auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});const std::vector<vk::DescriptorBufferInfo> a{{*src.first,0,inputBytes}},b{{*dst.first,0,bytes}};device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,a),VulkanRuntime::createWriteSet(*sets[0],1,b)},{});
    instance.cmdBuf()->reset();instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
    instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});struct Push{uint32_t count,a,b,c;};instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,Push{groups,count,0,0});instance.cmdBuf()->dispatch((groups+63)/64,1,1);vk::MemoryBarrier after{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eHost,{}, {after},{},{});instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);
    std::vector<float> output(outputCount);mapped=dst.second.mapMemory(0,bytes);std::memcpy(output.data(),mapped,bytes);dst.second.unmapMemory();instance.cmdBuf()->reset();return output;
}
}
namespace IQM::HDRVDP3 {
#if defined(IQM_HDRVDP3_SQUARE_QUALITY_DIAGNOSTIC)
#define run_pathway_masking run_square_masking_diagnostic
#endif
PathwayMaskingModel pathway_masking_model(){const double p=std::pow(10.0,0.3424);return {p,std::pow(10.0,0.34)/p,std::pow(10.0,0.108934275615),std::pow(10.0,1.41846291721),std::pow(10.0,0.136877512403)};}
PathwayMaskingResult run_pathway_masking(const IQM::VulkanInstance& instance,const std::array<PathwayBandInput,5>& bands,const std::vector<float>& logAdaptation,unsigned w,unsigned h,unsigned selectedIndex,const PathwayMaskingAssets& assets){
#if defined(IQM_HDRVDP3_SQUARE128_QUALITY_DIAGNOSTIC)
 const bool shapeOK=h==128&&w==128;
#elif defined(IQM_HDRVDP3_SQUARE80_QUALITY_DIAGNOSTIC)
 const bool shapeOK=h==80&&w==80;
#elif defined(IQM_HDRVDP3_SQUARE_QUALITY_DIAGNOSTIC)
 const bool shapeOK=h==96&&w==96;
#else
 const bool shapeOK=((h==128&&w==128)||(h==80&&w==80)||(h==96&&w==96)||(h==80&&w==96)||(h==96&&w==80));
#endif
 if(!shapeOK||logAdaptation.size()!=size_t(h)*w||selectedIndex>=size_t(w/8)*(h/8))throw std::invalid_argument("Unvalidated masking geometry/selection");
 const std::array<unsigned,5> widths{w,w,w/2,w/4,w/8},heights{h,h,h/2,h/4,h/8};
 for(float v:logAdaptation)if(!std::isfinite(v))throw std::invalid_argument("Nonfinite adaptation");
 for(unsigned b=0;b<5;++b){const size_t n=size_t(widths[b])*heights[b];const auto& a=bands[b];if(a.reference.size()!=n||a.test.size()!=n||a.self.size()!=n||a.comparison.size()!=n)throw std::invalid_argument("Band input size");for(const auto* v:{&a.reference,&a.test,&a.self})for(float x:*v)if(!std::isfinite(x))throw std::invalid_argument("Nonfinite band");for(double x:a.comparison)if(!std::isfinite(x))throw std::invalid_argument("Nonfinite comparison");}
#if defined(IQM_HDRVDP3_SQUARE128_QUALITY_DIAGNOSTIC)
 MapGPU gpu(instance,assets.map.string(),MapGPUProfile::Extended128Components);
#else
 MapGPU gpu(instance,assets.map.string(),static_cast<MapGPUProfile>(assets.mapABI));
#endif
 const auto model=pathway_masking_model();PathwayMaskingResult out;std::vector<float> errors;
 for(unsigned b=0;b<5;++b){const auto& a=bands[b];auto& r=out.bands[b];const unsigned bw=widths[b],bh=heights[b];std::vector<float> previous(a.self.size(),0),next(previous);
 if(b>0)previous=gpu.resize(bands[b-1].self,heights[b-1],widths[b-1],bh,bw);
 if(b<3)next=gpu.resize(bands[b+1].self,heights[b+1],widths[b+1],bh,bw);
 r.neighbor=gpu.neighbor_mask(previous,next);r.denominator=gpu.masking_denominator(a.self,r.neighbor,float(model.q),float(model.selfScale),float(model.neighborScale));
 r.difference=gpu.normalized_difference(a.reference,a.test,1);
 const auto resized=gpu.clamp_log(gpu.resize(logAdaptation,h,w,bh,bw));r.sensitivity=gpu.sample_csf(resized,create_csf_lut(30.0/std::pow(2.0,b+1),{24,true}));
 float neural=-1;
 if(b==4){int mn=1000,mx=-1000;for(float v:resized){if(!std::isfinite(v)||v < -5||v>5)throw std::runtime_error("Base log domain");if(v!=0){int e=std::ilogb(std::abs(v));mn=std::min(mn,e);mx=std::max(mx,e);}}if(mx!=-1000&&(mn < -90||mx>100||mx-mn>16))throw std::runtime_error("Base mean exponent domain");
 uint32_t countPush=assets.variableBaseCount?bw*bh:0;
#if defined(IQM_HDRVDP3_SQUARE80_QUALITY_DIAGNOSTIC)||defined(IQM_HDRVDP3_SQUARE128_QUALITY_DIAGNOSTIC)
 countPush=bw*bh;
#endif
 out.baseLog=resized;out.baseAdaptation=runFloatKernel(instance,assets.base.string(),resized,2,1,countPush);const double frequency=dominant_frequency_from_index(selectedIndex,bw,bh,3.75);const auto parameters=gpu.base_csf_parameters({out.baseAdaptation[1]})[0];const std::array<float,4> csf{parameters[1],parameters[2],parameters[3],parameters[4]};const double noise=1.0/evaluate_ncsf_parameters(frequency,csf,{24,true});out.baseNoise={double(selectedIndex),frequency,noise};neural=float(std::pow(noise,2*model.p));}
 r.D=gpu.difference(r.difference,r.sensitivity,r.denominator,float(model.p),neural);r.psychometric=gpu.psychometric(r.D,float(model.pf));
 auto input=r.D;for(double v:a.comparison)input.push_back(float(v));r.masked=runFloatKernel(instance,assets.product.string(),input,r.D.size(),uint32_t(r.D.size()));r.error=gpu.quality_band_error(r.D,a.comparison,bw,bh);errors.push_back(r.error);
 }
 out.scores=gpu.quality_scores(errors);return out;
}
}
