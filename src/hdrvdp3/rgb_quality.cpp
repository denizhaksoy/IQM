#include <IQM/hdrvdp3/rgb_quality.h>
#include <IQM/base/vulkan_runtime.h>
#include <vulkan_res.h>
#include <vulkan.h>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <utility>
#include <cmath>
namespace {
static std::vector<float> dispatchRGB(const IQM::VulkanInstance& instance,const std::string& shader,const std::vector<float>& input,size_t outputCount,uint32_t groups,uint32_t count){
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
QualityScores evaluate_pathway_linear_color_quality(const IQM::VulkanInstance& instance,
 const std::vector<float>& reference,const std::vector<float>& test,
 unsigned w,unsigned h,const XYZDisplaySamples& xyz,const RGBSpectralSamples& spectra,
 const RGBQualityAssets& assets,const std::filesystem::path& conversion,LinearColorEncoding encoding,LinearColorQualityTrace* trace) {
 if(w!=96||h!=96||reference.size()!=3*w*h||test.size()!=reference.size()||conversion.empty())
  throw std::invalid_argument("Invalid709 profile/input dimensions");
 for(const auto* image:{&reference,&test})for(float v:*image)if(!std::isfinite(v)||v<0)throw std::invalid_argument("Finite nonnegative709 input required");
 if(xyz.emission.size()!=xyz.wavelength.size())throw std::invalid_argument("Invalid XYZ emission grid");
 for(unsigned c=0;c<3;++c){if(xyz.wavelength!=spectra[c].wavelength||spectra[c].emission.size()!=xyz.emission.size())throw std::invalid_argument("709 models require shared grid");
  for(size_t i=0;i<xyz.emission.size();++i)if(xyz.emission[i][c]!=spectra[c].emission[i])throw std::invalid_argument("709 models describe different display primaries");}
 (void)calibrate_rgb_spectral_age(spectra,24); // Validate sensors before dispatch.
 LinearColorQualityTrace result;result.transform=linear_color_transform(xyz,encoding);const unsigned n=w*h;
 const std::array<const std::vector<float>*,2> images{&reference,&test};
 for(unsigned image=0;image<2;++image){auto packed=*images[image];
  for(unsigned col=0;col<3;++col)for(unsigned row=0;row<3;++row)packed.push_back(float(result.transform[row][col]));
  auto values=dispatchRGB(instance,conversion.string(),packed,6*n,3*n,n);
  result.raw[image].assign(values.begin(),values.begin()+3*n);result.converted[image].assign(values.begin()+3*n,values.end());
 }
 auto scores=evaluate_pathway_rgb_quality(instance,result.converted[0],result.converted[1],w,h,spectra,assets,trace?&result.native:nullptr);
 if(trace)*trace=std::move(result);return scores;
}
QualityScores evaluate_pathway_rgb709_quality(const IQM::VulkanInstance& instance,
 const std::vector<float>& reference,const std::vector<float>& test,
 unsigned w,unsigned h,const XYZDisplaySamples& xyz,const RGBSpectralSamples& spectra,
 const RGBQualityAssets& assets,const std::filesystem::path& conversion,RGB709QualityTrace* trace) {
 return evaluate_pathway_linear_color_quality(instance,reference,test,w,h,xyz,spectra,assets,conversion,LinearColorEncoding::BT709,trace);
}
QualityScores evaluate_pathway_rgb_quality(const IQM::VulkanInstance& instance,
 const std::vector<float>& reference,const std::vector<float>& test,
 unsigned w,unsigned h,const RGBSpectralSamples& spectra,
 const RGBQualityAssets& assets,RGBQualityTrace* trace) {
 if(w!=96||h!=96||reference.size()!=3*w*h||test.size()!=reference.size())
  throw std::invalid_argument("RGB quality requires96x96 three planar channels");
 if(assets.quality.masking.mapABI!=13||assets.quality.filter.reference256Graph||
    assets.projection.empty()||assets.adaptation.empty()||assets.jnd.empty()||assets.quality.comparison.empty())
  throw std::invalid_argument("Invalid RGB quality assets");
 for(const auto* image:{&reference,&test})for(float v:*image)
  if(!std::isfinite(v)||v<0)throw std::invalid_argument("Finite nonnegative RGB required");
 const auto calibration=calibrate_rgb_spectral_age(spectra,24);
 MapGPU gpu(instance,assets.jnd.string(),MapGPUProfile::RGBResponse);
 const auto lut=create_jnd_lut();const unsigned n=w*h;
 RGBQualityTrace result;std::array<std::vector<float>,2> P,adaptation;
 const std::array<const std::vector<float>*,2> originals{&reference,&test};
 for(unsigned image=0;image<2;++image){
  auto& optical=result.optical[image];optical.reserve(3*n+12);
  for(unsigned c=0;c<3;++c){const auto& input=*originals[image];std::vector<float> plane(input.begin()+c*n,input.begin()+(c+1)*n);
   auto filtered=run_mixed_filter(instance,plane,w,h,MixedFilterMode::OpticalRGB,assets.quality.filter);
   optical.insert(optical.end(),filtered.output.begin(),filtered.output.end());
  }
  auto packed=optical;for(unsigned sensor=0;sensor<4;++sensor)for(unsigned c=0;c<3;++c)packed.push_back(float(calibration.matrix[c][sensor]));
  result.lmsr[image]=dispatchRGB(instance,assets.projection.string(),packed,4*n,4*n,n);
  result.adaptationInput[image]=dispatchRGB(instance,assets.adaptation.string(),result.lmsr[image],n,n,n);
  adaptation[image]=run_mixed_filter(instance,result.adaptationInput[image],w,h,MixedFilterMode::LocalAdaptation,assets.quality.filter).output;
  result.pupil[image]=gpu.pupil_trace(adaptation[image],w,h,double(n)/900,24);
  result.jnd[image]=gpu.jnd_response_lmsr(result.lmsr[image],lut,result.pupil[image][3]);
  P[image].reserve(n);for(const auto& v:result.jnd[image])P[image].push_back(v.response);
 }
 auto scores=evaluate_pathway_rgb_response_quality(instance,reference,test,P,adaptation,w,h,assets.quality,trace?&result.quality:nullptr);
 if(trace)*trace=std::move(result);
 return scores;
}
}
