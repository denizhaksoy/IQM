#include <IQM/hdrvdp3/optical_resident_gpu.h>
#include <IQM/base/vulkan_runtime.h>
#include <fstream>
#include <stdexcept>
#include <algorithm>
namespace IQM::HDRVDP3 {namespace {
void barrier(vk::CommandBuffer c){vk::MemoryBarrier b{.srcAccessMask=vk::AccessFlagBits::eMemoryRead|vk::AccessFlagBits::eMemoryWrite,.dstAccessMask=vk::AccessFlagBits::eMemoryRead|vk::AccessFlagBits::eMemoryWrite};c.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,vk::PipelineStageFlagBits::eAllCommands,{}, {b},{},{});}
std::vector<uint32_t>load(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary|std::ios::ate);auto n=f.tellg();if(n<20||n%4)throw std::invalid_argument("Optical SPIR-V");std::vector<uint32_t>a(size_t(n)/4);f.seekg(0);f.read((char*)a.data(),n);if(!f||a[0]!=0x07230203u)throw std::invalid_argument("Optical SPIR-V read");return a;}
}
struct ResidentOpticalFilter::Impl {
 OpticalResidentBuffers b;vk::PhysicalDeviceLimits limits;std::unique_ptr<ResidentFFT2DForward>forward;std::unique_ptr<ResidentFFT2DInverse>inverse;
 vk::raii::DescriptorSetLayout desc=nullptr;vk::raii::PipelineLayout layout=nullptr;vk::raii::DescriptorPool pool=nullptr;std::vector<vk::raii::DescriptorSet>sets;std::vector<vk::raii::ShaderModule>modules;std::vector<vk::raii::Pipeline>pipelines;
 Impl(const vk::raii::Device&d,const vk::raii::PhysicalDevice&p,const OpticalResidentBuffers&buf,const OpticalResidentShaders&s):b(buf),limits(p.getProperties().limits){
  constexpr uint64_t n=7680ull*4320,pixels=3840ull*2160;std::vector<FFTBufferRegion>regions{b.input,b.output,b.filter,b.padded,b.half,b.spectrum,b.inverse,b.fftIntermediate,b.status};for(auto r:b.workspace)regions.push_back(r);
  for(auto r:regions)if(!r.buffer||!r.bytes||r.bytes%4||r.offset%limits.minStorageBufferOffsetAlignment||r.offset>UINT64_MAX-r.bytes||r.bytes>limits.maxStorageBufferRange)throw std::invalid_argument("Optical region");
  for(size_t i=0;i<regions.size();++i)for(size_t j=i+1;j<regions.size();++j)if(regions[i].buffer==regions[j].buffer&&regions[i].offset<regions[j].offset+regions[j].bytes&&regions[j].offset<regions[i].offset+regions[i].bytes)throw std::invalid_argument("Optical overlap");
  auto size=[](FFTBufferRegion r,uint64_t n){if(r.bytes<n)throw std::invalid_argument("Optical region too small");};size(b.input,pixels*4);size(b.output,pixels*4);size(b.filter,n*4);size(b.padded,n*4);size(b.half,7680ull*2161*8);for(auto r:{b.spectrum,b.inverse,b.fftIntermediate})size(r,n*8);size(b.status,4);
  forward=std::make_unique<ResidentFFT2DForward>(d,p,FFT2DBuffers{{b.padded,0,4320,1},{b.half,0,2161,1},b.fftIntermediate,b.workspace},s.forward);
  inverse=std::make_unique<ResidentFFT2DInverse>(d,p,FFTInverse2DBuffers{{b.spectrum,0,4320,1},{b.inverse,0,4320,1},b.fftIntermediate,b.workspace},s.inverse);
  using R=IQM::GPU::VulkanRuntime;desc=R::createDescLayout(d,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});layout=R::createPipelineLayout(d,{*desc},R::createPushConstantRange(16));
  for(auto file:{s.padding,s.io}){modules.push_back(R::createShaderModule(d,load(file)));pipelines.push_back(R::createComputePipeline(d,modules.back(),layout));}
  pool=R::createDescPool(d,5,{{vk::DescriptorType::eStorageBuffer,20}});std::array<vk::DescriptorSetLayout,5>layouts{*desc,*desc,*desc,*desc,*desc};auto a=vk::raii::DescriptorSets(d,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=5,.pSetLayouts=layouts.data()});for(auto&x:a)sets.push_back(std::move(x));
  std::array<std::pair<FFTBufferRegion,FFTBufferRegion>,5>bindings{{{b.input,b.padded},{b.half,b.spectrum},{b.spectrum,b.spectrum},{b.inverse,b.padded},{b.padded,b.output}}};
  for(unsigned i=0;i<5;++i){auto [src,dst]=bindings[i];std::vector<vk::DescriptorBufferInfo>v0{{src.buffer,src.offset,src.bytes}},v1{{b.filter.buffer,b.filter.offset,b.filter.bytes}},v2{{dst.buffer,dst.offset,dst.bytes}},v3{{b.status.buffer,b.status.offset,4}};d.updateDescriptorSets({R::createWriteSet(*sets[i],0,v0),R::createWriteSet(*sets[i],1,v1),R::createWriteSet(*sets[i],2,v2),R::createWriteSet(*sets[i],3,v3)},{});}
 }
 void io(vk::CommandBuffer c,unsigned idx,uint32_t count,uint32_t mode)const{barrier(c);c.bindPipeline(vk::PipelineBindPoint::eCompute,*pipelines[idx?1:0]);c.bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[idx]},{});std::array<uint32_t,4>push=idx?std::array<uint32_t,4>{count,mode,0,0}:std::array<uint32_t,4>{count,3840,2160,1};c.pushConstants<uint32_t>(*layout,vk::ShaderStageFlagBits::eCompute,0,push);uint32_t groups=(count+63)/64,x=std::min(groups,limits.maxComputeWorkGroupCount[0]);c.dispatch(x,(groups+x-1)/x,1);barrier(c);}
};
ResidentOpticalFilter::ResidentOpticalFilter(const vk::raii::Device&d,const vk::raii::PhysicalDevice&p,const OpticalResidentBuffers&b,const OpticalResidentShaders&s):impl_(std::make_unique<Impl>(d,p,b,s)){}
ResidentOpticalFilter::~ResidentOpticalFilter()=default;ResidentOpticalFilter::ResidentOpticalFilter(ResidentOpticalFilter&&)noexcept=default;ResidentOpticalFilter&ResidentOpticalFilter::operator=(ResidentOpticalFilter&&)noexcept=default;
uint32_t ResidentOpticalFilter::dispatchCount()const noexcept{return impl_?impl_->forward->dispatchCount()+impl_->inverse->dispatchCount()+5:0;}
void ResidentOpticalFilter::recordStage(vk::CommandBuffer c,OpticalStage stage)const{if(!impl_||!c)throw std::invalid_argument("Optical null job/cmd");auto&p=*impl_;constexpr uint32_t n=7680u*4320;switch(stage){case OpticalStage::Pad:barrier(c);c.fillBuffer(p.b.status.buffer,p.b.status.offset,4,0);p.io(c,0,n,0);break;case OpticalStage::Forward:p.forward->record(c);break;case OpticalStage::Expand:p.io(c,1,n,0);break;case OpticalStage::Multiply:p.io(c,2,n,1);break;case OpticalStage::Inverse:p.inverse->record(c);break;case OpticalStage::Normalize:p.io(c,3,n,2);break;case OpticalStage::Crop:p.io(c,4,3840u*2160,3);break;default:throw std::invalid_argument("Optical stage");}}
void ResidentOpticalFilter::record(vk::CommandBuffer c)const{for(auto s:{OpticalStage::Pad,OpticalStage::Forward,OpticalStage::Expand,OpticalStage::Multiply,OpticalStage::Inverse,OpticalStage::Normalize,OpticalStage::Crop})recordStage(c,s);}
}
