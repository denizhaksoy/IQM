#include <IQM/hdrvdp3/narrowing_resident_gpu.h>
#include <IQM/base/vulkan_runtime.h>
#include <fstream>
#include <algorithm>
#include <stdexcept>
namespace IQM::HDRVDP3 {namespace {
void barrier(vk::CommandBuffer c){vk::MemoryBarrier b{.srcAccessMask=vk::AccessFlagBits::eMemoryRead|vk::AccessFlagBits::eMemoryWrite,.dstAccessMask=vk::AccessFlagBits::eMemoryRead|vk::AccessFlagBits::eMemoryWrite};c.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,vk::PipelineStageFlagBits::eAllCommands,{}, {b},{},{});}
std::vector<uint32_t>load(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary|std::ios::ate);auto n=f.tellg();if(n<20||n%4)throw std::invalid_argument("Narrowing SPIR-V");std::vector<uint32_t>v(size_t(n)/4);f.seekg(0);f.read((char*)v.data(),n);if(!f||v[0]!=0x07230203u)throw std::invalid_argument("Narrowing shader read");return v;}
}
struct ResidentComplexNarrowing::Impl {
 FFTBufferRegion tensor,status;uint32_t count,gx,gy;
 vk::raii::DescriptorSetLayout desc=nullptr;vk::raii::PipelineLayout layout=nullptr;vk::raii::ShaderModule module=nullptr;vk::raii::Pipeline pipeline=nullptr;vk::raii::DescriptorPool pool=nullptr;std::vector<vk::raii::DescriptorSet>sets;
 Impl(const vk::raii::Device&d,const vk::raii::PhysicalDevice&p,FFTBufferRegion t,FFTBufferRegion s,uint32_t n,const std::filesystem::path&shader):tensor(t),status(s),count(n){
  auto lim=p.getProperties().limits;
  for(auto r:{t,s})if(!r.buffer||r.bytes%4||!r.bytes||r.offset%lim.minStorageBufferOffsetAlignment||r.offset>UINT64_MAX-r.bytes||r.bytes>lim.maxStorageBufferRange)throw std::invalid_argument("Narrowing region");
  if(!n||n>UINT32_MAX/2||uint64_t(n)*8>t.bytes||s.bytes<4)throw std::invalid_argument("Narrowing count/size");
  if(t.buffer==s.buffer&&t.offset<s.offset+s.bytes&&s.offset<t.offset+t.bytes)throw std::invalid_argument("Narrowing status overlap");
  uint32_t groups=(n+63)/64;gx=std::min(groups,lim.maxComputeWorkGroupCount[0]);gy=(groups+gx-1)/gx;if(gy>lim.maxComputeWorkGroupCount[1])throw std::invalid_argument("Narrowing dispatch range");
  using R=IQM::GPU::VulkanRuntime;desc=R::createDescLayout(d,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});layout=R::createPipelineLayout(d,{*desc},R::createPushConstantRange(8));module=R::createShaderModule(d,load(shader));pipeline=R::createComputePipeline(d,module,layout);pool=R::createDescPool(d,1,{{vk::DescriptorType::eStorageBuffer,2}});vk::DescriptorSetLayout dl=*desc;auto allocated=vk::raii::DescriptorSets(d,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&dl});for(auto&x:allocated)sets.push_back(std::move(x));std::vector<vk::DescriptorBufferInfo>a{{t.buffer,t.offset,t.bytes}},b{{s.buffer,s.offset,4}};d.updateDescriptorSets({R::createWriteSet(*sets[0],0,a),R::createWriteSet(*sets[0],1,b)},{});
 }
};
ResidentComplexNarrowing::ResidentComplexNarrowing(const vk::raii::Device&d,const vk::raii::PhysicalDevice&p,FFTBufferRegion t,FFTBufferRegion s,uint32_t n,const std::filesystem::path&shader):impl_(std::make_unique<Impl>(d,p,t,s,n,shader)){}
ResidentComplexNarrowing::~ResidentComplexNarrowing()=default;
ResidentComplexNarrowing::ResidentComplexNarrowing(ResidentComplexNarrowing&&)noexcept=default;
ResidentComplexNarrowing&ResidentComplexNarrowing::operator=(ResidentComplexNarrowing&&)noexcept=default;
void ResidentComplexNarrowing::record(vk::CommandBuffer c)const{
 if(!impl_||!c)throw std::invalid_argument("Narrowing null job/command buffer");auto&p=*impl_;barrier(c);c.fillBuffer(p.status.buffer,p.status.offset,4,0);barrier(c);c.bindPipeline(vk::PipelineBindPoint::eCompute,*p.pipeline);c.bindDescriptorSets(vk::PipelineBindPoint::eCompute,*p.layout,0,{*p.sets[0]},{});
 for(uint32_t mode=0;mode<2;++mode){std::array<uint32_t,2>push{p.count,mode};c.pushConstants<uint32_t>(*p.layout,vk::ShaderStageFlagBits::eCompute,0,push);c.dispatch(p.gx,p.gy,1);barrier(c);}
}
}
