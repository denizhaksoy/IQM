#include <IQM/hdrvdp3/fft_inverse_gpu.h>
#include <IQM/base/vulkan_runtime.h>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <vector>
#include <algorithm>

namespace IQM::HDRVDP3 {
using IQM::GPU::VulkanRuntime;
namespace {
std::vector<uint32_t> load(const std::filesystem::path& file){
 std::ifstream f(file,std::ios::binary|std::ios::ate);auto n=f.tellg();
 if(n<20||n%4)throw std::invalid_argument("FFT shader missing/invalid size: "+file.string());
 std::vector<uint32_t> code(size_t(n)/4);f.seekg(0);f.read(reinterpret_cast<char*>(code.data()),n);
 if(!f||code[0]!=0x07230203u)throw std::invalid_argument("FFT shader invalid SPIR-V");return code;
}
uint64_t footprint(const FFTBufferView& v,const ResidentFFTSpec& s){
 if(!v.sampleStride||!v.batchStride)throw std::invalid_argument("FFT zero stride");
 // Admit nonoverlapping row-major batches OR interleaved columns. Other exotic
 // layouts are rejected instead of guessing injectivity of the address map.
 if(uint64_t(v.batchStride)<uint64_t(s.length-1)*v.sampleStride+1&&
    uint64_t(v.sampleStride)<uint64_t(s.batches-1)*v.batchStride+1)
    throw std::invalid_argument("FFT overlapping/unsupported strides");
 uint64_t end=uint64_t(v.firstComplex)+uint64_t(s.batches-1)*v.batchStride+uint64_t(s.length-1)*v.sampleStride+1;
 if(end>std::numeric_limits<uint32_t>::max()/2||end>v.region.bytes/8)throw std::invalid_argument("FFT view exceeds region/address domain");
 return end*8;
}
bool overlap(const FFTBufferRegion& a,const FFTBufferRegion& b){
 return a.buffer==b.buffer&&a.offset<b.offset+b.bytes&&b.offset<a.offset+a.bytes;
}
void dependency(vk::CommandBuffer cmd,vk::PipelineStageFlags from,vk::AccessFlags src,
                vk::PipelineStageFlags to,vk::AccessFlags dst){
 vk::MemoryBarrier barrier{.srcAccessMask=src,.dstAccessMask=dst};
 cmd.pipelineBarrier(from,to,{}, {barrier},{},{});
}
}
struct ResidentFFTInverse7680::Impl {
 ResidentFFTSpec spec;ResidentFFTBuffers buffers;vk::PhysicalDeviceLimits limits;
 vk::raii::DescriptorSetLayout setLayout=nullptr;
 vk::raii::PipelineLayout layout=nullptr;
 std::vector<vk::raii::ShaderModule> modules;
 std::vector<vk::raii::Pipeline> pipelines;
 vk::raii::DescriptorPool pool=nullptr;
 std::vector<vk::raii::DescriptorSet> sets;
 std::vector<uint32_t> pipelineIndex,counts;
 std::vector<std::array<uint32_t,8>> pushes;
 Impl(const vk::raii::Device& device,const vk::raii::PhysicalDevice& physical,
      ResidentFFTSpec s,const ResidentFFTBuffers& b,const Inverse7680Shaders& shaders):spec(s),buffers(b),limits(physical.getProperties().limits){
  if(s.length!=7680||!s.batches||s.batches>4096||s.direction!=FFTDirection::Inverse)throw std::invalid_argument("Unsupported resident FFT transform; no CPU fallback");
  const uint64_t bytes=uint64_t(s.length)*s.batches*8;
  std::vector<FFTBufferRegion> regions{b.input.region,b.output.region};
  for(unsigned i=0;i<(s.length==720?2u:3u);++i){if(b.workspace[i].bytes<bytes)throw std::invalid_argument("FFT workspace too small");regions.push_back(b.workspace[i]);}
  for(const auto& r:regions){if(!r.buffer||!r.bytes||r.offset%limits.minStorageBufferOffsetAlignment||r.offset%8||r.bytes%8||r.bytes>limits.maxStorageBufferRange||r.offset>std::numeric_limits<uint64_t>::max()-r.bytes)throw std::invalid_argument("FFT invalid region/alignment/device limit");}
  for(size_t i=0;i<regions.size();++i)for(size_t j=i+1;j<regions.size();++j)if(overlap(regions[i],regions[j]))throw std::invalid_argument("FFT overlapping regions");
  footprint(b.input,s);footprint(b.output,s);
  setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
  layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(32));
  auto pipeline=[&](const auto& file){auto code=load(file);modules.push_back(VulkanRuntime::createShaderModule(device,code));pipelines.push_back(VulkanRuntime::createComputePipeline(device,modules.back(),layout));};
  pipeline(shaders.copy);for(const auto& file:shaders.kernels)pipeline(file);
  std::vector<std::pair<FFTBufferRegion,FFTBufferRegion>> bindings;
  auto add=[&](uint32_t pi,uint32_t n,FFTBufferRegion src,FFTBufferRegion dst,std::array<uint32_t,8> push){pipelineIndex.push_back(pi);counts.push_back(n);pushes.push_back(push);bindings.push_back({src,dst});};
  add(0,s.length*s.batches,b.input.region,b.workspace[0],{s.length*s.batches,s.length,b.input.firstComplex,b.input.sampleStride,b.input.batchStride,0,1,s.length});
  FFTBufferRegion result;
  if(s.length==720){add(1,s.batches,b.workspace[0],b.workspace[1],{s.batches,s.direction==FFTDirection::Inverse?1u:0u,0,0,0,0,0,0});result=b.workspace[1];}
  else {const std::array<uint32_t,4> n{s.batches*1920,s.batches*320,s.batches*20,s.batches};
   for(unsigned i=0;i<4;++i){auto src=i==0?b.workspace[0]:(i%2?b.workspace[1]:b.workspace[2]);auto dst=i%2?b.workspace[2]:b.workspace[1];add(i+1,n[i],src,dst,{n[i],0,0,0,0,0,0,0});}result=b.workspace[2];}
  add(0,s.length*s.batches,result,b.output.region,{s.length*s.batches,s.length,0,1,s.length,b.output.firstComplex,b.output.sampleStride,b.output.batchStride});
  const uint32_t number=uint32_t(bindings.size());pool=VulkanRuntime::createDescPool(device,number,{{vk::DescriptorType::eStorageBuffer,3*number}});
  std::vector<vk::DescriptorSetLayout> layouts(number,*setLayout);auto allocated=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=number,.pSetLayouts=layouts.data()});
  for(auto& set:allocated)sets.push_back(std::move(set));
  for(unsigned i=0;i<number;++i){const auto& [src,dst]=bindings[i];const std::vector<vk::DescriptorBufferInfo> a{{src.buffer,src.offset,src.bytes}},d{{dst.buffer,dst.offset,dst.bytes}};
   device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[i],0,a),VulkanRuntime::createWriteSet(*sets[i],1,a),VulkanRuntime::createWriteSet(*sets[i],2,d)},{});
   const uint32_t groups=(counts[i]+63)/64,x=std::min(groups,limits.maxComputeWorkGroupCount[0]);if(!x||(groups+x-1)/x>limits.maxComputeWorkGroupCount[1])throw std::invalid_argument("FFT dispatch exceeds device limits");}
 }
};
ResidentFFTInverse7680::ResidentFFTInverse7680(const vk::raii::Device& d,const vk::raii::PhysicalDevice& p,uint32_t batches,const ResidentFFTBuffers& b,const Inverse7680Shaders& sh):impl_(std::make_unique<Impl>(d,p,ResidentFFTSpec{FFTDomain::Complex1D,7680,batches,FFTDirection::Inverse,FFTNumericalGraph::Official4KAxis7680},b,sh)){}
ResidentFFTInverse7680::~ResidentFFTInverse7680()=default;
ResidentFFTInverse7680::ResidentFFTInverse7680(ResidentFFTInverse7680&&) noexcept=default;
ResidentFFTInverse7680& ResidentFFTInverse7680::operator=(ResidentFFTInverse7680&&) noexcept=default;
uint32_t ResidentFFTInverse7680::dispatchCount() const noexcept{return impl_?uint32_t(impl_->counts.size()):0;}
void ResidentFFTInverse7680::recordKernel(vk::CommandBuffer cmd,uint32_t kernel)const{
 if(!impl_||!cmd||kernel>3)throw std::invalid_argument("Inverse kernel record");const auto&p=*impl_;size_t i=kernel+1;
 dependency(cmd,vk::PipelineStageFlagBits::eAllCommands,vk::AccessFlagBits::eMemoryRead|vk::AccessFlagBits::eMemoryWrite,vk::PipelineStageFlagBits::eComputeShader,vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eShaderWrite);
 cmd.bindPipeline(vk::PipelineBindPoint::eCompute,*p.pipelines[p.pipelineIndex[i]]);cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute,*p.layout,0,{*p.sets[i]},{});cmd.pushConstants<uint32_t>(*p.layout,vk::ShaderStageFlagBits::eCompute,0,p.pushes[i]);uint32_t groups=(p.counts[i]+63)/64,x=std::min(groups,p.limits.maxComputeWorkGroupCount[0]);cmd.dispatch(x,(groups+x-1)/x,1);
 dependency(cmd,vk::PipelineStageFlagBits::eComputeShader,vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eShaderWrite,vk::PipelineStageFlagBits::eAllCommands,vk::AccessFlagBits::eMemoryRead|vk::AccessFlagBits::eMemoryWrite);
}
void ResidentFFTInverse7680::record(vk::CommandBuffer cmd) const {
 if(!impl_||!cmd)throw std::invalid_argument("FFT moved-from job/null command buffer");
 const auto& p=*impl_;
 dependency(cmd,vk::PipelineStageFlagBits::eAllCommands,vk::AccessFlagBits::eMemoryWrite,vk::PipelineStageFlagBits::eComputeShader,vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eShaderWrite);
 for(size_t i=0;i<p.counts.size();++i){cmd.bindPipeline(vk::PipelineBindPoint::eCompute,*p.pipelines[p.pipelineIndex[i]]);cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute,*p.layout,0,{*p.sets[i]},{});cmd.pushConstants<uint32_t>(*p.layout,vk::ShaderStageFlagBits::eCompute,0,p.pushes[i]);uint32_t groups=(p.counts[i]+63)/64,x=std::min(groups,p.limits.maxComputeWorkGroupCount[0]);cmd.dispatch(x,(groups+x-1)/x,1);
  dependency(cmd,vk::PipelineStageFlagBits::eComputeShader,vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eShaderWrite,vk::PipelineStageFlagBits::eComputeShader,vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eShaderWrite);}
 dependency(cmd,vk::PipelineStageFlagBits::eComputeShader,vk::AccessFlagBits::eShaderWrite,vk::PipelineStageFlagBits::eAllCommands,vk::AccessFlagBits::eMemoryRead|vk::AccessFlagBits::eMemoryWrite);
}
}
