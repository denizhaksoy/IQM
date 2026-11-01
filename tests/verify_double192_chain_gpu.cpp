#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <IQM/base/vulkan_runtime.h>
#include <bit>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace fs=std::filesystem;
using IQM::GPU::VulkanRuntime;
struct Buffer{vk::raii::DeviceMemory memory=nullptr;vk::raii::Buffer handle=nullptr;};
Buffer buffer(const IQM::Bin::VulkanInstance& instance,unsigned bytes){auto pair=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),bytes,vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);Buffer b;b.handle=std::move(pair.first);b.memory=std::move(pair.second);b.handle.bindMemory(b.memory,0);return b;}
template<class T>std::vector<T> read(const fs::path&p,size_t n){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()!=std::streamoff(n*sizeof(T)))throw std::runtime_error("Input shape "+p.string());std::vector<T>v(n);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),n*sizeof(T));return v;}
#include <IQM/hdrvdp3/mtf.h>
#include <cmath>
int main(int argc,char**argv){try{
 if(argc!=9)throw std::runtime_error("Usage: native forward-first forward-second multiply inverse-first inverse-second crop fresh-output");
 fs::path native=argv[1],out=argv[8];if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 constexpr size_t N=192, count=N*N, matrices=24, words=count*matrices*2;constexpr unsigned bytes=words*8;
 std::vector<double> input,filters,expected,croppedExpected;size_t filterErrors=0;
 for(std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"})for(int side=1;side<=2;++side)for(int conv=1;conv<=4;++conv){
  auto prefix=native/(display+"_"+std::to_string(side)+"_conv"+std::to_string(conv));
  auto padded=read<double>(prefix.string()+"_padded.f64",count);for(double v:padded){input.push_back(v);input.push_back(0.);}
  auto reference=read<double>(prefix.string()+"_inverse.f64",count);expected.insert(expected.end(),reference.begin(),reference.end());
  auto cropped=read<double>(prefix.string()+"_output.f64",9216);croppedExpected.insert(croppedExpected.end(),cropped.begin(),cropped.end());
  auto oracle=read<double>(prefix.string()+"_filter.f64",count);std::vector<double> generated(count);
  if(conv<4)IQM::HDRVDP3::generate_mtf_filter(generated.data(),N,N,30);
  else{const double sigma=std::pow(10.,-.781367)*30;for(size_t x=0;x<N;++x)for(size_t y=0;y<N;++y){double kx=(std::fmod(.5+double(x)/N,1.)-.5)*(2*3.14159265358979323846),ky=(std::fmod(.5+double(y)/N,1.)-.5)*(2*3.14159265358979323846);generated[x*N+y]=std::exp((-.5*(kx*kx+ky*ky))*(sigma*sigma));}}
  for(size_t i=0;i<count;++i)filterErrors+=std::bit_cast<uint64_t>(generated[i])!=std::bit_cast<uint64_t>(oracle[i]);
  filters.insert(filters.end(),generated.begin(),generated.end());
 }
 if(filterErrors)throw std::runtime_error("Source generated filters differ: "+std::to_string(filterErrors));
 IQM::Bin::VulkanInstance instance(true,"NVIDIA",true);const auto& device=*instance.device();
 const auto properties=instance.physicalDevice()->getProperties2<vk::PhysicalDeviceProperties2,vk::PhysicalDeviceFloatControlsProperties>();const auto& controls=properties.get<vk::PhysicalDeviceFloatControlsProperties>();if(!controls.shaderRoundingModeRTEFloat64||!controls.shaderSignedZeroInfNanPreserveFloat64)throw std::runtime_error("Required controls unavailable");
 std::ofstream(out/"device.txt")<<instance.selectedDevice<<" RTE64="<<controls.shaderRoundingModeRTEFloat64<<" signedZero64="<<controls.shaderSignedZeroInfNanPreserveFloat64<<" denorm64="<<controls.shaderDenormPreserveFloat64<<'\n';
 auto layoutSet=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
 auto layout=VulkanRuntime::createPipelineLayout(device,{*layoutSet},VulkanRuntime::createPushConstantRange(16));
 std::vector<vk::raii::ShaderModule> modules;std::vector<vk::raii::Pipeline> pipelines;
 for(int i=2;i<=7;++i){std::ifstream f(argv[i],std::ios::binary|std::ios::ate);auto n=f.tellg();if(n<=0||n%4)throw std::runtime_error("Invalid shader");std::vector<uint32_t> code(size_t(n)/4);f.seekg(0);f.read(reinterpret_cast<char*>(code.data()),n);modules.push_back(VulkanRuntime::createShaderModule(device,code));pipelines.push_back(VulkanRuntime::createComputePipeline(device,modules.back(),layout));}
 std::vector<Buffer> buffers;for(int i=0;i<7;++i)buffers.push_back(buffer(instance,bytes));auto filter=buffer(instance,unsigned(filters.size()*8));
 auto upload=[](Buffer& b,const std::vector<double>& v){auto p=b.memory.mapMemory(0,v.size()*8);std::memcpy(p,v.data(),v.size()*8);b.memory.unmapMemory();};upload(buffers[0],input);upload(filter,filters);
 auto pool=VulkanRuntime::createDescPool(device,6,{{vk::DescriptorType::eStorageBuffer,18}});std::vector<vk::DescriptorSetLayout> layouts(6,*layoutSet);auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=6,.pSetLayouts=layouts.data()});
 for(unsigned i=0;i<6;++i){std::vector<vk::DescriptorBufferInfo> a{{*buffers[i].handle,0,bytes}},b{{*buffers[i+1].handle,0,bytes}},c{{*filter.handle,0,filters.size()*8}};device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[i],0,a),VulkanRuntime::createWriteSet(*sets[i],1,b),VulkanRuntime::createWriteSet(*sets[i],2,c)},{});}
 auto cmd=instance.cmdBuf();cmd->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});vk::MemoryBarrier host{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};cmd->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {host},{},{});
 for(unsigned i=0;i<6;++i){std::array<uint32_t,4> push{192,(i==0||i==3)?4608u:24u,0,0};const unsigned dispatch[]={144,146,13824,144,288,3456};cmd->bindPipeline(vk::PipelineBindPoint::eCompute,*pipelines[i]);cmd->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[i]},{});cmd->pushConstants(*layout,vk::ShaderStageFlagBits::eCompute,0,16,push.data());cmd->dispatch(dispatch[i],1,1);vk::MemoryBarrier barrier{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};cmd->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader,{}, {barrier},{},{});}
 vk::MemoryBarrier end{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};cmd->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eHost,{}, {end},{},{});cmd->end();const vk::CommandBuffer raw=**cmd;vk::SubmitInfo submit{.commandBufferCount=1,.pCommandBuffers=&raw};vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(submit,fence);instance.waitForFence(fence);
 auto actual=static_cast<const double*>(buffers[5].memory.mapMemory(0,bytes));std::vector<double> real(expected.size());size_t errors=0;for(size_t i=0;i<real.size();++i){real[i]=actual[2*i];errors+=std::bit_cast<uint64_t>(real[i])!=std::bit_cast<uint64_t>(expected[i]);}std::ofstream dump(out/"actual.f64",std::ios::binary);dump.write(reinterpret_cast<const char*>(real.data()),real.size()*8);buffers[5].memory.unmapMemory();
 auto croppedActual=static_cast<const double*>(buffers[6].memory.mapMemory(0,bytes));std::vector<double> croppedResult(croppedExpected.size());size_t cropErrors=0;for(size_t i=0;i<croppedResult.size();++i){croppedResult[i]=croppedActual[2*i];cropErrors+=std::bit_cast<uint64_t>(croppedResult[i])!=std::bit_cast<uint64_t>(croppedExpected[i]);}std::ofstream cropDump(out/"cropped.f64",std::ios::binary);cropDump.write(reinterpret_cast<const char*>(croppedResult.data()),croppedResult.size()*8);buffers[6].memory.unmapMemory();
 std::ofstream(out/"results.txt")<<errors<<" bit errors /"<<real.size()<<" real Double words;source filter errors="<<filterErrors<<";crop errors="<<cropErrors<<" /"<<croppedResult.size()<<'\n';std::cout<<errors<<" bit errors /"<<real.size()<<'\n';if(errors||cropErrors)return 2;std::ofstream(out/"PASS.txt")<<"Both forward axes -> source-generated spectral filter multiplication -> both inverse axes GPU, normalized real inverse and GPU96sq crop exact;native padded input,not full GPU metric.\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}