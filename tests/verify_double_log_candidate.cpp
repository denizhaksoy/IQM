#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <IQM/base/vulkan_runtime.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace fs=std::filesystem;
using IQM::GPU::VulkanRuntime;
struct Buffer{vk::raii::DeviceMemory memory=nullptr;vk::raii::Buffer handle=nullptr;};
Buffer buffer(const IQM::Bin::VulkanInstance&i,unsigned bytes){auto pair=IQM::Bin::VulkanResource::createBuffer(*i.device(),*i.physicalDevice(),bytes,vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);Buffer b;b.handle=std::move(pair.first);b.memory=std::move(pair.second);b.handle.bindMemory(b.memory,0);return b;}
std::vector<double> read(const fs::path&p,size_t n){if(fs::file_size(p)!=n*8)throw std::runtime_error("Shape: "+p.string());std::vector<double>v(n);std::ifstream f(p,std::ios::binary);if(!f.read(reinterpret_cast<char*>(v.data()),std::streamsize(n*8)))throw std::runtime_error("Read failed");return v;}
void save(const fs::path&p,const std::vector<double>&v){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(v.data()),std::streamsize(v.size()*8));if(!f)throw std::runtime_error("Write failed");}
uint64_t ordered(double v){auto b=std::bit_cast<uint64_t>(v);return b>>63?~b:b|(uint64_t(1)<<63);}
int main(int argc,char**argv){try{
 if(argc!=4&&!(argc==5&&std::string(argv[4])=="--boundaries"))throw std::runtime_error("Usage: native-root shader fresh-output [--boundaries]");const fs::path native=argv[1],out=argv[3];if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 std::vector<double> input,expected;std::ofstream groups(out/"groups.tsv");groups<<"group\toffset\tcount\n";
 for(std::string capture:{"native","native_ramps","native_identity_v2"})for(std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"})for(std::string side:{"reference","test"}){
  // The capture uses numeric side identifiers, resolved explicitly below.
  const auto prefix=display+"_"+(side=="reference"?"1":"2")+"_";
  auto adaptation=read(native/capture/(prefix+"adaptation.f64"),9216);
  groups<<capture<<'/'<<prefix<<"adapt\t"<<input.size()<<"\t9216\n";input.insert(input.end(),adaptation.begin(),adaptation.end());
  auto post=read(native/capture/(prefix+"post_pupil.f64"),36864);
  groups<<capture<<'/'<<prefix<<"post_pupil_LMR_clamped\t"<<input.size()<<"\t27648\n";
  for(unsigned c:{0u,1u,3u})for(size_t j=0;j<9216;++j)input.push_back(std::clamp(post[c*9216+j],std::pow(10.,-5.),std::pow(10.,5.)));
 }
 const size_t controlsStart=input.size();
 for(double v:{std::numeric_limits<double>::min(),1e-300,1e-5,.5,1.,2.,10.,1e5,1e300,std::numeric_limits<double>::max()}){input.push_back(v);input.push_back(std::nextafter(v,0.));input.push_back(std::nextafter(v,std::numeric_limits<double>::infinity()));}
 // Exclude subnormals and infinity: this first candidate's declared domain.
 input.erase(std::remove_if(input.begin()+controlsStart,input.end(),[](double v){return !std::isnormal(v);}),input.end());groups<<"normal_controls\t"<<controlsStart<<'\t'<<input.size()-controlsStart<<'\n';groups.close();
 if(argc==5){size_t start=input.size();auto around=[&](double x){for(double v:{std::nextafter(x,0.),x,std::nextafter(x,std::numeric_limits<double>::infinity())})if(std::isnormal(v)&&v>0)input.push_back(v);};for(int e=-1022;e<=1023;++e)around(std::ldexp(1.,e));for(int e:{-20,0,20})for(unsigned k=0;k<=256;++k)around(std::ldexp(1.+double(k)/256.,e));for(double x:{.9375,1.0625})around(x);std::ofstream extra(out/"groups.tsv",std::ios::app);extra<<"table_exponent_nearone_boundaries\t"<<start<<'\t'<<input.size()-start<<'\n';}
 for(double v:input){if(!std::isnormal(v)||v<=0)throw std::runtime_error("Candidate domain violation");expected.push_back(std::log(v));expected.push_back(std::log10(v));}save(out/"input.f64",input);save(out/"expected.f64",expected);
 IQM::Bin::VulkanInstance instance(true,"NVIDIA",true);const auto&device=*instance.device();const auto properties=instance.physicalDevice()->getProperties2<vk::PhysicalDeviceProperties2,vk::PhysicalDeviceFloatControlsProperties>();const auto&controls=properties.get<vk::PhysicalDeviceFloatControlsProperties>();if(!instance.float64Enabled||!controls.shaderRoundingModeRTEFloat64||!controls.shaderSignedZeroInfNanPreserveFloat64)throw std::runtime_error("Double controls unavailable");std::ofstream(out/"device.txt")<<instance.selectedDevice<<"\nRTE64/signedZero64 enabled;denormPreserve64="<<controls.shaderDenormPreserveFloat64<<'\n';
 std::ifstream f(argv[2],std::ios::binary|std::ios::ate);auto length=f.tellg();if(length<=0||length%4)throw std::runtime_error("Shader shape");std::vector<uint32_t>code(size_t(length)/4);f.seekg(0);f.read(reinterpret_cast<char*>(code.data()),length);
 auto module=VulkanRuntime::createShaderModule(device,code);auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(16));auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);auto pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,2}});const vk::DescriptorSetLayout layouts[]{*setLayout};auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=layouts});
 unsigned inBytes=unsigned(input.size()*8),outBytes=unsigned(expected.size()*8);auto source=buffer(instance,inBytes),output=buffer(instance,outBytes);auto ptr=source.memory.mapMemory(0,inBytes);std::memcpy(ptr,input.data(),inBytes);source.memory.unmapMemory();std::vector<vk::DescriptorBufferInfo>a{{*source.handle,0,inBytes}},b{{*output.handle,0,outBytes}};device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,a),VulkanRuntime::createWriteSet(*sets[0],1,b)},{});
 auto cmd=instance.cmdBuf();cmd->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});vk::MemoryBarrier host{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};cmd->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {host},{},{});cmd->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);cmd->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});std::array<uint32_t,4>push{unsigned(input.size()),0,0,0};cmd->pushConstants(*layout,vk::ShaderStageFlagBits::eCompute,0,16,push.data());cmd->dispatch(unsigned((input.size()+63)/64),1,1);vk::MemoryBarrier end{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};cmd->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eHost,{}, {end},{},{});cmd->end();const vk::CommandBuffer raw=**cmd;vk::SubmitInfo submit{.commandBufferCount=1,.pCommandBuffers=&raw};vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(submit,fence);instance.waitForFence(fence);
 ptr=output.memory.mapMemory(0,outBytes);std::vector<double>actual(expected.size());std::memcpy(actual.data(),ptr,outBytes);output.memory.unmapMemory();save(out/"actual.f64",actual);
 size_t errors[2]{},nonfinite[2]{};uint64_t maxUlp[2]{};for(size_t i=0;i<expected.size();++i){unsigned op=unsigned(i%2);errors[op]+=std::bit_cast<uint64_t>(actual[i])!=std::bit_cast<uint64_t>(expected[i]);if(!std::isfinite(actual[i])){++nonfinite[op];continue;}auto a=ordered(actual[i]),b=ordered(expected[i]);maxUlp[op]=std::max(maxUlp[op],a>b?a-b:b-a);}
 std::ofstream results(out/"results.tsv");results<<"operation\tqueries\tbit_errors\tmax_ulp\tnonfinite\n";for(unsigned op=0;op<2;++op)results<<(op?"log10":"log")<<'\t'<<input.size()<<'\t'<<errors[op]<<'\t'<<maxUlp[op]<<'\t'<<nonfinite[op]<<'\n';results.close();std::ofstream(out/"COMPLETE.txt")<<"Diagnostic completed;"<<errors[0]+errors[1]<<" bit mismatches;not accepted for production\n";std::cout<<errors[0]<<" log / "<<errors[1]<<" log10 bit mismatches of "<<input.size()<<" queries each\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
