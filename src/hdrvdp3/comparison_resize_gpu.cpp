#include <IQM/hdrvdp3/comparison_resize_gpu.h>
#include <IQM/hdrvdp3/resize.h>
#include <IQM/base/vulkan_runtime.h>
#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>
namespace IQM::HDRVDP3 {namespace {
std::vector<double> axis(const IQM::Bin::VulkanInstance& instance,const std::string& shader,
 const std::vector<double>& input,unsigned h,unsigned w,unsigned nh,unsigned nw,bool horizontal){
 using IQM::GPU::VulkanRuntime;
 const unsigned from=horizontal?w:h,to=horizontal?nw:nh;
 const auto plan=make_resize_axis_plan(int(from),int(to));
 std::vector<double> packed(size_t(to)*(2*plan.taps+1));
 for(unsigned d=0;d<to;++d){const size_t at=size_t(d)*(2*plan.taps+1);packed[at]=plan.normalization[d];for(int t=0;t<plan.taps;++t){const size_t k=size_t(d)*plan.taps+t;packed[at+1+2*t]=double(plan.indices[k]);packed[at+2+2*t]=plan.weights[k];}}
 const auto limits=instance.physicalDevice()->getProperties().limits;
 const size_t inputBytes=input.size()*8,planBytes=packed.size()*8,bytes=size_t(nh)*nw*8;
 if(inputBytes>limits.maxStorageBufferRange||planBytes>limits.maxStorageBufferRange||bytes>limits.maxStorageBufferRange)throw std::runtime_error("Double resize storage limit");
 const uint32_t count=uint32_t(size_t(nh)*nw),groups=(count+63)/64,gx=std::min(groups,limits.maxComputeWorkGroupCount[0]),gy=(groups+gx-1)/gx;
 if(gy>limits.maxComputeWorkGroupCount[1])throw std::runtime_error("Double resize dispatch limit");
 std::ifstream file(shader,std::ios::binary|std::ios::ate);const auto length=file.tellg();if(length<=0||length%4)throw std::runtime_error("Double resize shader");
 std::vector<uint32_t> code(size_t(length)/4);file.seekg(0);file.read(reinterpret_cast<char*>(code.data()),length);if(!file||code[0]!=0x07230203u)throw std::runtime_error("Double resize SPIR-V");
 const auto& device=*instance.device();auto module=VulkanRuntime::createShaderModule(device,code);
 auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
 auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(32));auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
 auto allocate=[&](size_t size){auto buffer=IQM::Bin::VulkanResource::createBuffer(device,*instance.physicalDevice(),unsigned(size),vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);buffer.first.bindMemory(buffer.second,0);return buffer;};
 auto src=allocate(inputBytes),weights=allocate(planBytes),dst=allocate(bytes);
 auto upload=[&](auto& buffer,const std::vector<double>& values){const auto n=values.size()*8;void* p=buffer.second.mapMemory(0,n);std::memcpy(p,values.data(),n);buffer.second.unmapMemory();};upload(src,input);upload(weights,packed);
 auto pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,3}});const vk::DescriptorSetLayout raw=*setLayout;
 auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});
 const std::vector<vk::DescriptorBufferInfo> a{{*src.first,0,inputBytes}},b{{*weights.first,0,planBytes}},c{{*dst.first,0,bytes}};
 device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,a),VulkanRuntime::createWriteSet(*sets[0],1,b),VulkanRuntime::createWriteSet(*sets[0],2,c)},{});
 instance.cmdBuf()->reset();instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
 vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
 instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});
 struct Push{uint32_t count,w,h,nw,nh,taps,horizontal,unused;};instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,Push{count,w,h,nw,nh,uint32_t(plan.taps),horizontal?1u:0u,0});
 instance.cmdBuf()->dispatch(gx,gy,1);vk::MemoryBarrier after{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eHost,{}, {after},{},{});instance.cmdBuf()->end();
 vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);
 std::vector<double> output(size_t(nh)*nw);void* p=dst.second.mapMemory(0,bytes);std::memcpy(output.data(),p,bytes);dst.second.unmapMemory();instance.cmdBuf()->reset();return output;
}
}
ComparisonResizeGPUResult resize_comparison_gpu(const IQM::Bin::VulkanInstance& instance,const std::string& shader,std::span<const double> mask,unsigned h,unsigned w,unsigned nh,unsigned nw){
 if(!instance.float64Enabled)throw std::runtime_error("Double resize requires enabled shaderFloat64");
 if(!h||!w||!nh||!nw||h>16384||w>16384||nh>16384||nw>16384||mask.size()!=size_t(h)*w||mask.size()>3840u*2160u||size_t(nh)*nw>3840u*2160u||size_t(h)*nw>3840u*2160u)throw std::invalid_argument("Double comparison resize geometry");
 for(double v:mask)if(v!=0.&&v!=1.)throw std::invalid_argument("Double comparison resize requires binary mask");
 ComparisonResizeGPUResult result;result.values.assign(mask.begin(),mask.end());
 if(w!=nw){result.values=axis(instance,shader,result.values,h,w,h,nw,true);++result.dispatches;}
 if(h!=nh){result.values=axis(instance,shader,result.values,h,nw,nh,nw,false);++result.dispatches;}
 return result;
}
}
