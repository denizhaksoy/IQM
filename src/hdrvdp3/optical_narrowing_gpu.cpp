#include <IQM/hdrvdp3/optical_narrowing_gpu.h>
#include <IQM/base/vulkan_runtime.h>
#include <fstream>
#include <algorithm>
#include <stdexcept>
namespace IQM::HDRVDP3 {namespace {
void barrier(vk::CommandBuffer c){vk::MemoryBarrier b{.srcAccessMask=vk::AccessFlagBits::eMemoryRead|vk::AccessFlagBits::eMemoryWrite,.dstAccessMask=vk::AccessFlagBits::eMemoryRead|vk::AccessFlagBits::eMemoryWrite};c.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,vk::PipelineStageFlagBits::eAllCommands,{}, {b},{},{});}
std::vector<uint32_t>load(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary|std::ios::ate);auto n=f.tellg();if(n<20||n%4)throw std::invalid_argument("Optical select SPIR-V");std::vector<uint32_t>v(size_t(n)/4);f.seekg(0);f.read((char*)v.data(),n);if(!f||v[0]!=0x07230203u)throw std::invalid_argument("Optical select shader read");return v;}
}
struct ResidentOpticalNarrowing::Impl{
 OpticalNarrowingBuffers b;vk::PhysicalDeviceLimits limits;
 std::unique_ptr<ResidentOpticalFilter>base;std::unique_ptr<ResidentComplexNarrowing>nf,np;std::unique_ptr<ResidentFFT2DInverseInplace>inverse;
 vk::raii::DescriptorSetLayout desc=nullptr;vk::raii::PipelineLayout layout=nullptr;vk::raii::ShaderModule module=nullptr;vk::raii::Pipeline pipeline=nullptr;vk::raii::DescriptorPool pool=nullptr;std::vector<vk::raii::DescriptorSet>sets;
 Impl(const vk::raii::Device&d,const vk::raii::PhysicalDevice&p,const OpticalNarrowingBuffers&buf,const OpticalNarrowingShaders&s):b(buf),limits(p.getProperties().limits){
  std::vector<FFTBufferRegion>r{b.base.input,b.base.output,b.base.filter,b.base.padded,b.base.half,b.base.spectrum,b.base.inverse,b.base.fftIntermediate,b.base.status,b.alternateInverse,b.fftStatus,b.productStatus};for(auto w:b.base.workspace)r.push_back(w);
  for(auto a:r)if(!a.buffer||!a.bytes||a.bytes%4||a.offset%limits.minStorageBufferOffsetAlignment||a.offset>UINT64_MAX-a.bytes||a.bytes>limits.maxStorageBufferRange)throw std::invalid_argument("Optical narrowing region");
  for(size_t i=0;i<r.size();++i)for(size_t j=i+1;j<r.size();++j)if(r[i].buffer==r[j].buffer&&r[i].offset<r[j].offset+r[j].bytes&&r[j].offset<r[i].offset+r[i].bytes)throw std::invalid_argument("Optical narrowing alias");
  constexpr uint64_t n=7680ull*4320;if(b.alternateInverse.bytes<n*8||b.fftStatus.bytes<4||b.productStatus.bytes<4)throw std::invalid_argument("Optical narrowing size");
  base=std::make_unique<ResidentOpticalFilter>(d,p,b.base,s.base);nf=std::make_unique<ResidentComplexNarrowing>(d,p,b.base.spectrum,b.fftStatus,uint32_t(n),s.narrowing);np=std::make_unique<ResidentComplexNarrowing>(d,p,b.base.spectrum,b.productStatus,uint32_t(n),s.narrowing);
  inverse=std::make_unique<ResidentFFT2DInverseInplace>(d,p,FFTInverse2DBuffers{{b.base.spectrum,0,4320,1},{b.alternateInverse,0,4320,1},b.base.fftIntermediate,b.base.workspace},FFTInverse2DInplaceShaders{s.inverse4320,s.base.inverse.axis7680});
  using R=IQM::GPU::VulkanRuntime;desc=R::createDescLayout(d,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});layout=R::createPipelineLayout(d,{*desc},R::createPushConstantRange(8));module=R::createShaderModule(d,load(s.select));pipeline=R::createComputePipeline(d,module,layout);pool=R::createDescPool(d,2,{{vk::DescriptorType::eStorageBuffer,8}});std::array<vk::DescriptorSetLayout,2>dl{*desc,*desc};auto allocated=vk::raii::DescriptorSets(d,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=2,.pSetLayouts=dl.data()});for(auto&x:allocated)sets.push_back(std::move(x));
  for(unsigned i=0;i<2;++i){auto src=i?b.base.padded:b.alternateInverse;std::vector<vk::DescriptorBufferInfo>a{{src.buffer,src.offset,src.bytes}},raw{{b.base.inverse.buffer,b.base.inverse.offset,b.base.inverse.bytes}},stat{{b.productStatus.buffer,b.productStatus.offset,4}},out{{b.base.output.buffer,b.base.output.offset,b.base.output.bytes}};d.updateDescriptorSets({R::createWriteSet(*sets[i],0,a),R::createWriteSet(*sets[i],1,raw),R::createWriteSet(*sets[i],2,stat),R::createWriteSet(*sets[i],3,out)},{});}
 }
 void io(vk::CommandBuffer c,uint32_t mode)const{barrier(c);c.bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);c.bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[mode]},{});std::array<uint32_t,2>push{mode?3840u*2160:7680u*4320*2,mode};c.pushConstants<uint32_t>(*layout,vk::ShaderStageFlagBits::eCompute,0,push);uint32_t groups=(push[0]+63)/64,x=std::min(groups,limits.maxComputeWorkGroupCount[0]);c.dispatch(x,(groups+x-1)/x,1);barrier(c);}
};
ResidentOpticalNarrowing::ResidentOpticalNarrowing(const vk::raii::Device&d,const vk::raii::PhysicalDevice&p,const OpticalNarrowingBuffers&b,const OpticalNarrowingShaders&s):impl_(std::make_unique<Impl>(d,p,b,s)){}
ResidentOpticalNarrowing::~ResidentOpticalNarrowing()=default;ResidentOpticalNarrowing::ResidentOpticalNarrowing(ResidentOpticalNarrowing&&)noexcept=default;ResidentOpticalNarrowing&ResidentOpticalNarrowing::operator=(ResidentOpticalNarrowing&&)noexcept=default;
uint32_t ResidentOpticalNarrowing::dispatchCount()const noexcept{return impl_?impl_->base->dispatchCount()+impl_->inverse->dispatchCount()+5:0;}
void ResidentOpticalNarrowing::recordStage(vk::CommandBuffer c,OpticalStage stage)const{
 if(!impl_||!c)throw std::invalid_argument("Optical narrowing null job/cmd");auto&p=*impl_;switch(stage){case OpticalStage::Expand:p.base->recordStage(c,stage);p.nf->record(c);break;case OpticalStage::Multiply:p.base->recordStage(c,stage);p.np->record(c);break;case OpticalStage::Inverse:p.inverse->record(c);p.base->recordStage(c,stage);p.io(c,0);break;case OpticalStage::Crop:p.io(c,1);break;default:p.base->recordStage(c,stage);}
}
void ResidentOpticalNarrowing::record(vk::CommandBuffer c)const{for(auto s:{OpticalStage::Pad,OpticalStage::Forward,OpticalStage::Expand,OpticalStage::Multiply,OpticalStage::Inverse,OpticalStage::Normalize,OpticalStage::Crop})recordStage(c,s);}
}
