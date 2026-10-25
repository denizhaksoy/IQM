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
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <IQM/hdrvdp3/resize.h>
using IQM::GPU::VulkanRuntime;
struct Buffer{vk::raii::DeviceMemory memory=nullptr;vk::raii::Buffer handle=nullptr;};
static Buffer buffer(const IQM::Bin::VulkanInstance& instance,unsigned bytes){
    auto pair=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),bytes,
        vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
    Buffer b;b.handle=std::move(pair.first);b.memory=std::move(pair.second);b.handle.bindMemory(b.memory,0);return b;
}
static std::vector<uint32_t> words(const std::filesystem::path& path){
    std::ifstream f(path);unsigned h,w;if(!(f>>h>>w)||!h||!w||size_t(h)*w>1000000)throw std::runtime_error("Invalid word fixture");
    std::vector<uint32_t> v(size_t(h)*w);for(auto& x:v)if(!(f>>x))throw std::runtime_error("Truncated word fixture");
    std::string extra;if(f>>extra)throw std::runtime_error("Trailing fixture words");return v;
}
static std::vector<uint32_t> code(const std::filesystem::path& path){
    std::ifstream f(path,std::ios::binary|std::ios::ate);auto length=f.tellg();if(length<=0||length%4)throw std::runtime_error("Invalid SPIR-V");
    std::vector<uint32_t> v(size_t(length)/4);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),length);
    if(!f||v[0]!=0x07230203u)throw std::runtime_error("Invalid shader words");return v;
}
static void plan(std::vector<uint32_t>& data,const IQM::HDRVDP3::ResizeAxisPlan& p,int n){
    for(int d=0;d<n;++d){if(p.normalization[d]!=1)throw std::runtime_error("Nonunit resize normalization");double absolute=0;for(int k=0;k<p.taps;++k){const auto i=size_t(d)*p.taps+k;double f=std::ldexp(p.weights[i],20);absolute+=std::abs(p.weights[i]);if(!std::isfinite(f)||f!=std::trunc(f)||std::abs(f)>1048576)throw std::runtime_error("Inexact Q20 weight");data.push_back(uint32_t(p.indices[i]));data.push_back(std::bit_cast<uint32_t>(int32_t(f)));}if(absolute>2)throw std::runtime_error("Accumulation bound exceeded");}
}
int main(int argc,char** argv){try{
 if(argc!=4)throw std::runtime_error("Usage:shader.spv native-directory fresh-output");const std::filesystem::path native(argv[2]),out(argv[3]);if(std::filesystem::exists(out)||!std::filesystem::create_directories(out))throw std::runtime_error("Fresh output required");
 IQM::Bin::VulkanInstance instance(true);const auto& device=*instance.device();const auto props=instance.physicalDevice()->getProperties2<vk::PhysicalDeviceProperties2,vk::PhysicalDeviceFloatControlsProperties>();const auto& controls=props.get<vk::PhysicalDeviceFloatControlsProperties>();if(!controls.shaderDenormPreserveFloat32||!controls.shaderRoundingModeRTEFloat32)throw std::runtime_error("Float32 modes unsupported");
 auto module=VulkanRuntime::createShaderModule(device,code(argv[1]));auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(32));auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
 std::ifstream manifest(native/"manifest.txt");if(!manifest)throw std::runtime_error("Missing manifest");std::ofstream results(out/"results.tsv");results<<"case\tbinary_words\tdouble_values\tbinary_errors\tdouble_bit_errors\n";
 std::string tag,folder,expectedPath;unsigned h,w,nh,nw,cases=0;size_t total=0,binaryTotal=0;
 while(manifest>>tag>>h>>w>>nh>>nw>>std::quoted(folder)>>std::quoted(expectedPath)){
  if(!((h==80&&w==96)||(h==96&&w==80))||!nh||!nw||h%nh||w%nw||h/nh!=w/nw||!(h/nh==1||h/nh==2||h/nh==4||h/nh==8))throw std::runtime_error("Unvalidated shape");
  const unsigned n=h*w,first=h*nw,last=nh*nw;auto reference=words(std::filesystem::path(folder)/"reference.txt"),test=words(std::filesystem::path(folder)/"test.txt"),binary=words(std::filesystem::path(folder)/"binary.txt");if(reference.size()!=n||test.size()!=n||binary.size()!=n)throw std::runtime_error("Bad input shape");
  for(unsigned i=0;i<n;++i)if(!std::isfinite(std::bit_cast<float>(reference[i]))||std::bit_cast<float>(reference[i])<=0||!std::isfinite(std::bit_cast<float>(test[i]))||binary[i]>1)throw std::runtime_error("Input outside domain");
  std::ifstream f(expectedPath);unsigned eh,ew;if(!(f>>eh>>ew)||eh!=nh||ew!=nw)throw std::runtime_error("Bad expected shape");std::vector<uint64_t> expected(last);for(auto& u:expected){double v;if(!(f>>v)||!std::isfinite(v))throw std::runtime_error("Bad native double");u=std::bit_cast<uint64_t>(v);}std::string extra;if(f>>extra)throw std::runtime_error("Trailing expected values");
  auto hp=IQM::HDRVDP3::make_resize_axis_plan(w,nw),vp=IQM::HDRVDP3::make_resize_axis_plan(h,nh);std::array<std::vector<uint32_t>,4> data;data[0]=reference;data[0].insert(data[0].end(),test.begin(),test.end());data[1].resize(n);plan(data[1],hp,nw);data[2].resize(first);plan(data[2],vp,nh);data[3].resize(2*last);
  std::vector<Buffer> buffers;for(auto& v:data){buffers.push_back(buffer(instance,unsigned(v.size()*4)));void* p=buffers.back().memory.mapMemory(0,v.size()*4);std::memcpy(p,v.data(),v.size()*4);buffers.back().memory.unmapMemory();}
  auto pool=VulkanRuntime::createDescPool(device,3,{{vk::DescriptorType::eStorageBuffer,6}});const vk::DescriptorSetLayout raw[]={*setLayout,*setLayout,*setLayout};auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=3,.pSetLayouts=raw});for(unsigned j=0;j<3;++j){const std::vector<vk::DescriptorBufferInfo>a{{*buffers[j].handle,0,data[j].size()*4}},b{{*buffers[j+1].handle,0,data[j+1].size()*4}};device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[j],0,a),VulkanRuntime::createWriteSet(*sets[j],1,b)},{});}
  instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
  struct Push{uint32_t count,mode,w,h,nw,nh,taps,offset;};const Push pushes[]={{n,0,w,h,w,h,0,0},{first,1,w,h,nw,h,uint32_t(hp.taps),n},{last,2,nw,h,nw,nh,uint32_t(vp.taps),first}};
  for(unsigned j=0;j<3;++j){instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[j]},{});instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,pushes[j]);instance.cmdBuf()->dispatch((pushes[j].count+63)/64,1,1);vk::MemoryBarrier barrier{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eHostRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eHost,{}, {barrier},{},{});}
  instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);
  const auto* mask=static_cast<const uint32_t*>(buffers[1].memory.mapMemory(0,n*4));size_t mbad=0;for(unsigned i=0;i<n;++i)mbad+=mask[i]!=binary[i];buffers[1].memory.unmapMemory();const auto* actual=static_cast<const uint32_t*>(buffers[3].memory.mapMemory(0,last*8));size_t bad=0;for(unsigned i=0;i<last;++i)bad+=(uint64_t(actual[2*i])|(uint64_t(actual[2*i+1])<<32))!=expected[i];std::ofstream dump(out/(tag+".u64"),std::ios::binary);dump.write(reinterpret_cast<const char*>(actual),last*8);buffers[3].memory.unmapMemory();instance.cmdBuf()->reset();results<<tag<<'\t'<<n<<'\t'<<last<<'\t'<<mbad<<'\t'<<bad<<'\n';results.flush();if(mbad||bad)throw std::runtime_error("Comparison mask mismatch:"+tag);total+=last;binaryTotal+=n;++cases;
 }
 if(!manifest.eof()||cases!=90||total!=321840||binaryTotal!=691200)throw std::runtime_error("Incomplete comparison coverage");
 std::cout<<"comparison cases="<<cases<<" binary_words="<<binaryTotal<<" double_values="<<total<<" bit_mismatches=0\n";std::ofstream(out/"PASS.txt")<<"GPU binary mask ->Q20 horizontal ->Q40 double bits exact;90 cases; no host intermediate computation.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
