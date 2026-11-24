#include <IQM/hdrvdp3/fft4320_gpu.h>
#include <IQM/base/vulkan_runtime.h>
#include <fstream>
#include <stdexcept>
#include <limits>
#include <algorithm>
namespace IQM::HDRVDP3 {
namespace {
void barrier(vk::CommandBuffer c){vk::MemoryBarrier b{.srcAccessMask=vk::AccessFlagBits::eMemoryRead|vk::AccessFlagBits::eMemoryWrite,.dstAccessMask=vk::AccessFlagBits::eMemoryRead|vk::AccessFlagBits::eMemoryWrite};c.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,vk::PipelineStageFlagBits::eAllCommands,{}, {b},{},{});}
std::vector<uint32_t> load(const std::filesystem::path& f){std::ifstream in(f,std::ios::binary|std::ios::ate);auto n=in.tellg();if(n<20||n%4)throw std::invalid_argument("FFT4320 SPIR-V size");std::vector<uint32_t> r(size_t(n)/4);in.seekg(0);in.read((char*)r.data(),n);if(!in||r[0]!=0x07230203u)throw std::invalid_argument("FFT4320 SPIR-V");return r;}
}
struct ResidentFFT4320::Impl {
 FFT4320Mode mode;uint32_t batches;FFT4320Buffers buffers;vk::PhysicalDeviceLimits limits;
 vk::raii::DescriptorSetLayout setLayout=nullptr;vk::raii::PipelineLayout layout=nullptr;
 std::vector<vk::raii::ShaderModule> modules;std::vector<vk::raii::Pipeline> pipelines;
 vk::raii::DescriptorPool pool=nullptr;std::vector<vk::raii::DescriptorSet> sets;
 std::unique_ptr<ResidentFFT> child;
 Impl(const vk::raii::Device& device,const vk::raii::PhysicalDevice& physical,FFT4320Mode m,uint32_t n,const FFT4320Buffers& b,const FFT4320Shaders& s):mode(m),batches(n),buffers(b),limits(physical.getProperties().limits){
  if(n==0||n>1024||(m!=FFT4320Mode::RealForward&&m!=FFT4320Mode::ComplexForward&&m!=FFT4320Mode::ComplexInverse))throw std::invalid_argument("Unsupported4320 mode/batch");
  std::array<FFTBufferRegion,6> regions{b.input.region,b.output.region,b.workspace[0],b.workspace[1],b.workspace[2],b.workspace[3]};
  for(const auto&r:regions)if(!r.buffer||r.bytes==0||r.bytes%4||r.offset%limits.minStorageBufferOffsetAlignment||r.offset>UINT64_MAX-r.bytes||r.bytes>limits.maxStorageBufferRange)throw std::invalid_argument("FFT4320 region/device limit");
  for(size_t i=0;i<regions.size();++i)for(size_t j=i+1;j<regions.size();++j)if(regions[i].buffer==regions[j].buffer&&regions[i].offset<regions[j].offset+regions[j].bytes&&regions[j].offset<regions[i].offset+regions[i].bytes)throw std::invalid_argument("FFT4320 overlap");
  const uint64_t bytes=uint64_t(n)*4320*8;for(auto r:b.workspace)if(r.bytes<bytes||r.bytes%8)throw std::invalid_argument("FFT4320 workspace too small");
  const bool real=m==FFT4320Mode::RealForward;
  auto check=[&](const FFT4320View& v,uint32_t samples,uint32_t words){if(v.sampleStride<words||v.batchStride<words)throw std::invalid_argument("FFT4320 stride");if(uint64_t(v.batchStride)<uint64_t(samples-1)*v.sampleStride+words&&uint64_t(v.sampleStride)<uint64_t(n-1)*v.batchStride+words)throw std::invalid_argument("FFT4320 overlapping view");uint64_t last=uint64_t(v.firstScalar)+uint64_t(n-1)*v.batchStride+uint64_t(samples-1)*v.sampleStride+words;if(last>UINT32_MAX||last>v.region.bytes/4)throw std::invalid_argument("FFT4320 view bounds");};
  check(b.input,4320,real?1:2);check(b.output,real?2161:4320,2);
  ResidentFFTBuffers cb{{b.workspace[0],0,1,720},{b.workspace[1],0,1,720},{b.workspace[2],b.workspace[3],{}}};
  child=std::make_unique<ResidentFFT>(device,physical,ResidentFFTSpec{FFTDomain::Complex1D,720,n*(real?3u:6u),FFTDirection::Forward,FFTNumericalGraph::Standalone720},cb,s.children);
  using R=IQM::GPU::VulkanRuntime;setLayout=R::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});layout=R::createPipelineLayout(device,{*setLayout},R::createPushConstantRange(32));
  for(const auto& file:{s.io,real?s.realOuter:s.complexOuter}){modules.push_back(R::createShaderModule(device,load(file)));pipelines.push_back(R::createComputePipeline(device,modules.back(),layout));}
  pool=R::createDescPool(device,3,{{vk::DescriptorType::eStorageBuffer,9}});std::array<vk::DescriptorSetLayout,3> layouts{*setLayout,*setLayout,*setLayout};auto allocated=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=3,.pSetLayouts=layouts.data()});for(auto&set:allocated)sets.push_back(std::move(set));
  const std::array<std::pair<FFTBufferRegion,FFTBufferRegion>,3> bindings{{{b.input.region,b.workspace[0]},{b.workspace[1],b.workspace[0]},{b.workspace[0],b.output.region}}};
  for(unsigned i=0;i<3;++i){auto [src,dst]=bindings[i];std::vector<vk::DescriptorBufferInfo>a{{src.buffer,src.offset,src.bytes}},o{{dst.buffer,dst.offset,dst.bytes}};device.updateDescriptorSets({R::createWriteSet(*sets[i],0,a),R::createWriteSet(*sets[i],1,a),R::createWriteSet(*sets[i],2,o)},{});}
 }
 void dispatch(vk::CommandBuffer cmd,unsigned index,uint32_t count,const std::array<uint32_t,8>& push)const{uint32_t groups=(count+63)/64,x=std::min(groups,limits.maxComputeWorkGroupCount[0]),y=(groups+x-1)/x;if(y>limits.maxComputeWorkGroupCount[1])throw std::invalid_argument("FFT4320 dispatch limit");cmd.bindPipeline(vk::PipelineBindPoint::eCompute,*pipelines[index==1?1:0]);cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[index]},{});cmd.pushConstants<uint32_t>(*layout,vk::ShaderStageFlagBits::eCompute,0,push);cmd.dispatch(x,y,1);}
};
ResidentFFT4320::ResidentFFT4320(const vk::raii::Device&d,const vk::raii::PhysicalDevice&p,FFT4320Mode m,uint32_t n,const FFT4320Buffers&b,const FFT4320Shaders&s):impl_(std::make_unique<Impl>(d,p,m,n,b,s)){}
ResidentFFT4320::~ResidentFFT4320()=default;
ResidentFFT4320::ResidentFFT4320(ResidentFFT4320&&)noexcept=default;
ResidentFFT4320& ResidentFFT4320::operator=(ResidentFFT4320&&)noexcept=default;
uint32_t ResidentFFT4320::dispatchCount()const noexcept{return impl_?6:0;}
void ResidentFFT4320::recordChildren(vk::CommandBuffer cmd)const{
 if(!impl_||!cmd)throw std::invalid_argument("FFT4320 null job/cmd");const auto&p=*impl_;bool real=p.mode==FFT4320Mode::RealForward;uint32_t count=p.batches*(real?2160u:4320u);barrier(cmd);p.dispatch(cmd,0,count,{count,p.batches,0,real?1u:0u,p.buffers.input.firstScalar,p.buffers.input.sampleStride,p.buffers.input.batchStride,p.mode==FFT4320Mode::ComplexInverse?1u:0u});barrier(cmd);p.child->record(cmd);
}
void ResidentFFT4320::recordOuter(vk::CommandBuffer cmd)const{
 if(!impl_||!cmd)throw std::invalid_argument("FFT4320 null job/cmd");const auto&p=*impl_;bool real=p.mode==FFT4320Mode::RealForward;uint32_t count=p.batches*(real?360u:720u),outputs=p.batches*(real?2161u:4320u);barrier(cmd);p.dispatch(cmd,1,count,{count,p.batches,p.mode==FFT4320Mode::ComplexInverse?1u:0u,0,0,0,0,0});barrier(cmd);p.dispatch(cmd,2,outputs,{outputs,p.batches,1,real?1u:0u,p.buffers.output.firstScalar,p.buffers.output.sampleStride,p.buffers.output.batchStride,0});barrier(cmd);
}
void ResidentFFT4320::record(vk::CommandBuffer cmd)const{recordChildren(cmd);recordOuter(cmd);}
}
