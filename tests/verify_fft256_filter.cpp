// Isolated128-pixel MTF/LA chain. Does not change the bounded public API.
#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <IQM/base/vulkan_runtime.h>
#include <IQM/hdrvdp3/mtf.h>
#include <IQM/hdrvdp3/resize.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
using IQM::GPU::VulkanRuntime;
using IQM::Bin::VulkanInstance;
using IQM::Bin::VulkanResource;
using Path=std::filesystem::path;
struct Buffer{vk::raii::DeviceMemory memory=nullptr;vk::raii::Buffer handle=nullptr;unsigned bytes;};
static Buffer buffer(const VulkanInstance& instance,unsigned bytes){
    auto pair=VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),bytes,
        vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
    Buffer b;b.handle=std::move(pair.first);b.memory=std::move(pair.second);b.bytes=bytes;b.handle.bindMemory(b.memory,0);return b;
}
static std::vector<float> read(const Path& path,unsigned h,unsigned w){
    std::ifstream f(path);unsigned rh,rw;if(!(f>>rh>>rw)||rh!=h||rw!=w)throw std::runtime_error("Invalid shape:"+path.string());
    std::vector<float> result(h*w);for(auto& v:result){double d;if(!(f>>d)||!std::isfinite(d))throw std::runtime_error("Invalid input");v=float(d);}
    std::string extra;if(f>>extra)throw std::runtime_error("Trailing values");return result;
}
static void upload(Buffer& b,const std::vector<float>& data){
    if(data.size()*4!=b.bytes)throw std::runtime_error("Upload size");auto* p=b.memory.mapMemory(0,b.bytes);std::memcpy(p,data.data(),b.bytes);b.memory.unmapMemory();
}
struct Kernel{
    vk::raii::DescriptorSetLayout setLayout=nullptr;vk::raii::DescriptorPool pool=nullptr;
    vk::raii::PipelineLayout layout=nullptr;vk::raii::Pipeline pipeline=nullptr;vk::raii::DescriptorSet set=nullptr;
    Kernel(const VulkanInstance& instance,const Path& path,const std::vector<vk::DescriptorType>& types,unsigned pushBytes=16){
        const auto& device=*instance.device();std::ifstream f(path,std::ios::binary|std::ios::ate);auto size=f.tellg();
        if(size<=0||size%4)throw std::runtime_error("Invalid shader:"+path.string());std::vector<uint32_t> code(size_t(size)/4);
        f.seekg(0);if(!f.read(reinterpret_cast<char*>(code.data()),size))throw std::runtime_error("Truncated shader");
        auto module=VulkanRuntime::createShaderModule(device,code);
        std::vector<std::pair<vk::DescriptorType,unsigned>> bindings;
        for(auto t:types)bindings.emplace_back(t,1);
        setLayout=VulkanRuntime::createDescLayout(device,bindings);
        pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,3},{vk::DescriptorType::eStorageImage,2}});
        layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(pushBytes));
        pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
        vk::DescriptorSetLayout raw=*setLayout;
        auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});set=std::move(sets[0]);
    }
    void bind(const VulkanInstance& instance,unsigned binding,Buffer& b){
        const std::vector<vk::DescriptorBufferInfo> info{{*b.handle,0,b.bytes}};
        instance.device()->updateDescriptorSets({VulkanRuntime::createWriteSet(*set,binding,info)},{});
    }
    void bind(const VulkanInstance& instance,unsigned binding,const vk::raii::ImageView& image){
        const auto info=VulkanRuntime::createImageInfos({&image});instance.device()->updateDescriptorSets({VulkanRuntime::createWriteSet(*set,binding,info)},{});
    }
    void run(vk::CommandBuffer cmd,std::array<uint32_t,4> push,unsigned x,unsigned y=1){
        cmd.bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*set},{});
        cmd.pushConstants(*layout,vk::ShaderStageFlagBits::eCompute,0,16,push.data());cmd.dispatch(x,y,1);
        vk::MemoryBarrier barrier{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eHostRead|vk::AccessFlagBits::eTransferRead};
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eHost|vk::PipelineStageFlagBits::eTransfer,{}, {barrier},{},{});
    }
    void runMap(vk::CommandBuffer cmd,std::array<uint32_t,8> push,unsigned x){
        cmd.bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*set},{});
        cmd.pushConstants(*layout,vk::ShaderStageFlagBits::eCompute,0,32,push.data());cmd.dispatch(x,1,1);
        vk::MemoryBarrier barrier{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eHostRead};
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eHost,{}, {barrier},{},{});
    }
};
static size_t compare(Buffer& b,const std::vector<float>& expected,const Path& dump,unsigned h,unsigned w,bool transpose){
    const auto* actual=static_cast<const float*>(b.memory.mapMemory(0,b.bytes));size_t bad=0;double maximum=0;
    std::ofstream f(dump);f<<h<<' '<<w<<'\n'<<std::setprecision(9);
    for(size_t i=0;i<expected.size();++i){
        const auto ci=i/2;const size_t index=transpose?2*((ci%256)*256+ci/256)+i%2:i;
        const float v=actual[index];f<<v<<'\n';
        if(!std::isfinite(v)||v!=expected[i]){if(bad<3)std::cout<<dump.filename()<<" index="<<i<<" gpu="<<v<<" native="<<expected[i]<<'\n';++bad;maximum=std::max(maximum,std::abs(double(v)-expected[i]));}
    }b.memory.unmapMemory();std::cout<<dump.filename()<<" values="<<expected.size()<<" mismatches="<<bad<<" max="<<maximum<<'\n';return bad;
}
static int base16(const Path& source,const Path& shader,const Path& output){
    if(!std::filesystem::create_directory(output))throw std::runtime_error("Fresh base16 export required");
    const auto input=read(source/"input.txt",64,512),expected=read(source/"expected.txt",64,769);
    VulkanInstance instance(true);const auto& device=*instance.device();
    const auto props=instance.physicalDevice()->getProperties2<vk::PhysicalDeviceProperties2,vk::PhysicalDeviceFloatControlsProperties>();
    const auto& controls=props.get<vk::PhysicalDeviceFloatControlsProperties>();
    if(!controls.shaderDenormPreserveFloat32||!controls.shaderRoundingModeRTEFloat32)throw std::runtime_error("Required float controls unsupported");
    auto in=buffer(instance,unsigned(input.size()*4)),out=buffer(instance,unsigned(expected.size()*4));upload(in,input);
    const auto sb=vk::DescriptorType::eStorageBuffer;Kernel kernel(instance,shader,{sb,sb});kernel.bind(instance,0,in);kernel.bind(instance,1,out);
    vk::CommandBuffer cmd=**instance.cmdBuf();cmd.begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    vk::MemoryBarrier host{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {host},{},{});
    kernel.run(cmd,{16,64,0,0},2);cmd.end();vk::SubmitInfo submit{.commandBufferCount=1,.pCommandBuffers=&cmd};
    vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(submit,fence);instance.waitForFence(fence);
    const auto bad=compare(out,expected,output/"base16_output.txt",64,769,false);
    std::cout<<"base16 values=49216 mismatches="<<bad<<'\n';return bad?1:0;
}
static int base_reductions(const Path& source,const Path& shader,const Path& output,unsigned n,const Path& fftShader={}){
    if(n!=64&&n!=256)throw std::runtime_error("Reduction sample count must be64 or256");
    const auto input=read(source/"input.txt",512,2*n),expected=read(source/"expected.txt",512,n+3);
    for(unsigned batch=0;batch<512;++batch)for(unsigned kind=0;kind<2;++kind){
        int minimum=1000,maximum=-1000;
        for(unsigned k=0;k<n;++k){const float v=input[batch*2*n+kind*n+k];
            if(!std::isfinite(v)||(kind&&std::abs(v)>5))throw std::runtime_error("Reduction value outside declared domain");
            if(v!=0){const int e=std::ilogb(std::abs(v));minimum=std::min(minimum,e);maximum=std::max(maximum,e);}
        }
        if(maximum!=-1000&&(maximum-minimum>16||minimum < -90||maximum>100))throw std::runtime_error("Reduction exponent domain");
    }
    if(!std::filesystem::create_directory(output))throw std::runtime_error("Fresh reduction export required");
    VulkanInstance instance(true);const auto& device=*instance.device();
    const auto props=instance.physicalDevice()->getProperties2<vk::PhysicalDeviceProperties2,vk::PhysicalDeviceFloatControlsProperties>();
    const auto& controls=props.get<vk::PhysicalDeviceFloatControlsProperties>();
    if(!controls.shaderDenormPreserveFloat32||!controls.shaderRoundingModeRTEFloat32)throw std::runtime_error("Required float controls unsupported");
    const bool chain=!fftShader.empty();if(chain&&n!=256)throw std::runtime_error("Only residual16 chain supported");
    auto in=buffer(instance,unsigned(input.size()*4)),out=buffer(instance,unsigned((chain?512*2*n:expected.size())*4));upload(in,input);
    const auto sb=vk::DescriptorType::eStorageBuffer;Kernel kernel(instance,shader,{sb,sb},32);kernel.bind(instance,0,in);kernel.bind(instance,1,out);
    std::unique_ptr<Kernel> fft;Buffer spectrum;std::vector<float> expectedSpectrum;
    if(chain){expectedSpectrum=read(source/"expected_fft16.txt",512,769);spectrum=buffer(instance,unsigned(expectedSpectrum.size()*4));
        fft=std::make_unique<Kernel>(instance,fftShader,std::vector<vk::DescriptorType>{sb,sb});fft->bind(instance,0,out);fft->bind(instance,1,spectrum);
    }
    vk::CommandBuffer cmd=**instance.cmdBuf();cmd.begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    vk::MemoryBarrier host{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {host},{},{});
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute,*kernel.pipeline);cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute,*kernel.layout,0,{*kernel.set},{});
    const std::array<uint32_t,8> push{512,chain?1u:0u,n,0,0,0,0,0};cmd.pushConstants(*kernel.layout,vk::ShaderStageFlagBits::eCompute,0,32,push.data());cmd.dispatch(8,1,1);
    vk::MemoryBarrier after{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead|(chain?vk::AccessFlagBits::eShaderRead:vk::AccessFlags{})};cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eHost|(chain?vk::PipelineStageFlagBits::eComputeShader:vk::PipelineStageFlags{}),{}, {after},{},{});
    if(chain)fft->run(cmd,{16,512,0,0},16);
    cmd.end();vk::SubmitInfo submit{.commandBufferCount=1,.pCommandBuffers=&cmd};vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(submit,fence);instance.waitForFence(fence);
    if(chain){
        std::vector<float> centered(512*2*n,0);for(size_t batch=0;batch<512;++batch)for(size_t k=0;k<n;++k)centered[batch*2*n+2*k]=expected[batch*(n+3)+k];
        auto bad=compare(out,centered,output/"centered_interleaved.txt",512,2*n,false);
        bad+=compare(spectrum,expectedSpectrum,output/"fft_selection.txt",512,769,false);
        std::cout<<"DC_FFT_selection values="<<centered.size()+expectedSpectrum.size()<<" mismatches="<<bad<<'\n';return bad?1:0;
    }
    const auto bad=compare(out,expected,output/"reductions.txt",512,n+3,false);
    std::cout<<"reductions samples="<<n<<" values="<<expected.size()<<" mismatches="<<bad<<'\n';return bad?1:0;
}
static int masking128(const Path& source,const Path& shader,const Path& output){
    constexpr unsigned n=16384;const auto input=read(source/"input.txt",16,2*n),expected=read(source/"expected.txt",16,2*n);
    if(!std::filesystem::create_directory(output))throw std::runtime_error("Fresh masking export required");
    VulkanInstance instance(true);const auto& device=*instance.device();
    auto in=buffer(instance,2*n*4),raw=buffer(instance,n*4),pooled=buffer(instance,n*4);
    const auto sb=vk::DescriptorType::eStorageBuffer;Kernel rawKernel(instance,shader,{sb,sb},32),poolKernel(instance,shader,{sb,sb},32);
    rawKernel.bind(instance,0,in);rawKernel.bind(instance,1,raw);poolKernel.bind(instance,0,raw);poolKernel.bind(instance,1,pooled);
    size_t bad=0;
    for(unsigned c=0;c<16;++c){
        upload(in,std::vector<float>(input.begin()+c*2*n,input.begin()+(c+1)*2*n));
        instance.cmdBuf()->reset();vk::CommandBuffer cmd=**instance.cmdBuf();cmd.begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        vk::MemoryBarrier host{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};cmd.pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {host},{},{});
        const float reciprocal=1.0f/float(1u<<(c%5));
        rawKernel.runMap(cmd,{n,3,128,128,128,128,std::bit_cast<uint32_t>(reciprocal),n},n/64);
        poolKernel.runMap(cmd,{n,4,128,128,128,128,0,0},n/64);
        cmd.end();vk::SubmitInfo submit{.commandBufferCount=1,.pCommandBuffers=&cmd};vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(submit,fence);instance.waitForFence(fence);
        bad+=compare(raw,std::vector<float>(expected.begin()+c*2*n,expected.begin()+c*2*n+n),output/("raw"+std::to_string(c)+".txt"),128,128,false);
        bad+=compare(pooled,std::vector<float>(expected.begin()+c*2*n+n,expected.begin()+(c+1)*2*n),output/("pool"+std::to_string(c)+".txt"),128,128,false);
    }
    std::cout<<"mask128 values=524288 mismatches="<<bad<<'\n';return bad?1:0;
}
static int scalar128(const Path& source,const Path& shader,const Path& output,bool pupil){
    constexpr unsigned n=16384;const unsigned cases=pupil?30:16,width=pupil?n+3:2*n,outputs=pupil?4:1;
    const std::string prefix=pupil?"pupil":"quality";
    const auto input=read(source/(prefix+"_input.txt"),cases,width),expected=read(source/(prefix+"_expected.txt"),cases,outputs);
    if(!std::filesystem::create_directory(output))throw std::runtime_error("Fresh scalar export required");
    VulkanInstance instance(true);const auto& device=*instance.device();auto in=buffer(instance,width*4),out=buffer(instance,outputs*4);
    const auto sb=vk::DescriptorType::eStorageBuffer;Kernel kernel(instance,shader,{sb,sb},32);kernel.bind(instance,0,in);kernel.bind(instance,1,out);size_t bad=0;
    for(unsigned c=0;c<cases;++c){
        upload(in,std::vector<float>(input.begin()+c*width,input.begin()+(c+1)*width));instance.cmdBuf()->reset();
        vk::CommandBuffer cmd=**instance.cmdBuf();cmd.begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        vk::MemoryBarrier host{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};cmd.pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {host},{},{});
        kernel.runMap(cmd,{outputs,pupil?12u:10u,128,128,0,0,0,n},1);
        cmd.end();vk::SubmitInfo submit{.commandBufferCount=1,.pCommandBuffers=&cmd};vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(submit,fence);instance.waitForFence(fence);
        bad+=compare(out,std::vector<float>(expected.begin()+c*outputs,expected.begin()+(c+1)*outputs),output/(prefix+std::to_string(c)+".txt"),1,outputs,false);
    }
    std::cout<<prefix<<"128 values="<<expected.size()<<" mismatches="<<bad<<'\n';return bad?1:0;
}
static std::vector<uint32_t> execute_map_words(const VulkanInstance& instance,Kernel& kernel,const std::vector<uint32_t>& words,std::array<uint32_t,8> push){
    auto in=buffer(instance,unsigned(words.size()*4)),out=buffer(instance,push[0]*4);
    auto* mapped=in.memory.mapMemory(0,in.bytes);std::memcpy(mapped,words.data(),in.bytes);in.memory.unmapMemory();kernel.bind(instance,0,in);kernel.bind(instance,1,out);
    instance.cmdBuf()->reset();vk::CommandBuffer cmd=**instance.cmdBuf();cmd.begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    vk::MemoryBarrier host{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};cmd.pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {host},{},{});
    kernel.runMap(cmd,push,(push[0]+63)/64);cmd.end();vk::SubmitInfo submit{.commandBufferCount=1,.pCommandBuffers=&cmd};
    vk::raii::Fence fence(*instance.device(),vk::FenceCreateInfo{});instance.queue()->submit(submit,fence);instance.waitForFence(fence);
    std::vector<uint32_t> result(push[0]);mapped=out.memory.mapMemory(0,out.bytes);std::memcpy(result.data(),mapped,out.bytes);out.memory.unmapMemory();return result;
}
static int resize_mask128(const Path& source,const Path& shader,const Path& output,unsigned fractionBits=16){
    if(fractionBits!=16&&fractionBits!=20)throw std::runtime_error("Only Q16/Q20 resize supported");
    const auto input=read(source/"input.txt",8,16384);if(!std::filesystem::create_directory(output))throw std::runtime_error("Fresh resize export required");
    VulkanInstance instance(true);const auto sb=vk::DescriptorType::eStorageBuffer;Kernel kernel(instance,shader,{sb,sb},32);size_t bad=0,total=0;
    auto append=[fractionBits](std::vector<uint32_t>& packed,const IQM::HDRVDP3::ResizeAxisPlan& plan,unsigned n){
        for(unsigned d=0;d<n;++d){if(plan.normalization[d]!=1)throw std::runtime_error("Nonunit resize normalization");double absolute=0;
            for(int t=0;t<plan.taps;++t){const size_t index=size_t(d)*plan.taps+t;const double fixed=std::ldexp(plan.weights[index],int(fractionBits));absolute+=std::abs(plan.weights[index]);
                if(!std::isfinite(fixed)||fixed!=std::trunc(fixed)||std::abs(fixed)>std::ldexp(2.0,int(fractionBits)))throw std::runtime_error("Nonexact Q"+std::to_string(fractionBits)+" resize weight");
                packed.push_back(uint32_t(plan.indices[index]));packed.push_back(std::bit_cast<uint32_t>(int32_t(fixed)));
            }if(absolute>2)throw std::runtime_error("Resize accumulation bound");
        }
    };
    for(unsigned c=0;c<8;++c)for(unsigned n:{128u,64u,32u,16u,8u}){
        std::vector<uint32_t> words;for(unsigned k=0;k<16384;++k){const float v=input[c*16384+k];if(v!=0&&v!=1)throw std::runtime_error("Nonbinary mask");words.push_back(v==1?1u:0u);}
        const auto horizontal=IQM::HDRVDP3::make_resize_axis_plan(128,int(n)),vertical=IQM::HDRVDP3::make_resize_axis_plan(128,int(n));append(words,horizontal,n);
        auto first=execute_map_words(instance,kernel,words,{128*n,23,128,128,n,128,uint32_t(horizontal.taps),16384});
        const auto offset=uint32_t(first.size());append(first,vertical,n);
        const auto result=execute_map_words(instance,kernel,first,{2*n*n,24,n,128,n,n,uint32_t(vertical.taps),offset});
        const auto name="mask"+std::to_string(c)+"_to"+std::to_string(n)+".txt";std::ifstream f(source/name);unsigned h,w;if(!(f>>h>>w)||h!=n||w!=n)throw std::runtime_error("Invalid double resize shape");
        std::ofstream dump(output/name);dump<<n<<' '<<n<<'\n'<<std::setprecision(17);size_t local=0;
        for(unsigned k=0;k<n*n;++k){double expected;if(!(f>>expected)||!std::isfinite(expected))throw std::runtime_error("Invalid double reference");
            const uint64_t bits=uint64_t(result[2*k])|(uint64_t(result[2*k+1])<<32);const auto value=std::bit_cast<double>(bits);dump<<value<<'\n';
            if(bits!=std::bit_cast<uint64_t>(expected))++local;
        }bad+=local;total+=n*n;std::cout<<name<<" double_values="<<n*n<<" bit_mismatches="<<local<<'\n';
    }
    std::cout<<"resize_mask128 double_values="<<total<<" bit_mismatches="<<bad<<'\n';return bad?1:0;
}
static int resize_float128(const Path& source,const Path& shader,const Path& output){
    if(!std::filesystem::create_directory(output))throw std::runtime_error("Fresh float resize export required");
    VulkanInstance instance(true);const auto sb=vk::DescriptorType::eStorageBuffer;Kernel kernel(instance,shader,{sb,sb},32);size_t bad=0,total=0;
    const std::array<std::array<unsigned,2>,10> pairs{{{128,128},{128,64},{128,32},{128,16},{128,8},{64,128},{32,64},{16,32},{8,16},{16,128}}};
    auto append=[](std::vector<uint32_t>& words,const IQM::HDRVDP3::ResizeAxisPlan& plan,unsigned n){
        for(unsigned d=0;d<n;++d){words.push_back(std::bit_cast<uint32_t>(float(plan.normalization[d])));
            for(int t=0;t<plan.taps;++t){const size_t index=size_t(d)*plan.taps+t;words.push_back(uint32_t(plan.indices[index]));words.push_back(std::bit_cast<uint32_t>(float(plan.weights[index])));}
        }
    };
    for(auto pair:pairs){
        const unsigned side=pair[0],n=pair[1];const auto tag="from"+std::to_string(side)+"_to"+std::to_string(n);
        const auto input=read(source/tag/"input.txt",8,side*side),expected=read(source/tag/"expected.txt",8,n*n);std::filesystem::create_directory(output/tag);
        const auto horizontal=IQM::HDRVDP3::make_resize_axis_plan(int(side),int(n)),vertical=IQM::HDRVDP3::make_resize_axis_plan(int(side),int(n));
        for(unsigned c=0;c<8;++c){
            std::vector<uint32_t> words;for(unsigned k=0;k<side*side;++k)words.push_back(std::bit_cast<uint32_t>(input[c*side*side+k]));append(words,horizontal,n);
            auto first=execute_map_words(instance,kernel,words,{side*n,0,side,side,n,side,uint32_t(horizontal.taps),side*side});const auto offset=uint32_t(first.size());append(first,vertical,n);
            const auto result=execute_map_words(instance,kernel,first,{n*n,1,n,side,n,n,uint32_t(vertical.taps),offset});
            std::ofstream dump(output/tag/(std::to_string(c)+".txt"));dump<<n<<' '<<n<<'\n'<<std::setprecision(9);size_t local=0;
            for(unsigned k=0;k<n*n;++k){const float v=std::bit_cast<float>(result[k]);dump<<v<<'\n';if(!std::isfinite(v)||v!=expected[c*n*n+k])++local;}
            bad+=local;total+=n*n;if(local)std::cout<<tag<<" case="<<c<<" mismatches="<<local<<'\n';
        }
    }
    std::cout<<"resize_float128 values="<<total<<" mismatches="<<bad<<'\n';return bad?1:0;
}
int main(int argc,char** argv){try{
    if(argc==6&&std::string(argv[1])=="--resize-mask128")return resize_mask128(argv[2],argv[3],argv[4],unsigned(std::stoul(argv[5])));
    if(argc==5&&std::string(argv[1])=="--resize-float128")return resize_float128(argv[2],argv[3],argv[4]);
    if(argc==5&&std::string(argv[1])=="--resize-mask128")return resize_mask128(argv[2],argv[3],argv[4]);
    if(argc==5&&(std::string(argv[1])=="--pupil128"||std::string(argv[1])=="--quality128"))return scalar128(argv[2],argv[3],argv[4],std::string(argv[1])=="--pupil128");
    if(argc==5&&std::string(argv[1])=="--mask128")return masking128(argv[2],argv[3],argv[4]);
    if(argc==6&&std::string(argv[1])=="--base-reductions-chain")return base_reductions(argv[2],argv[3],argv[5],256,argv[4]);
    if(argc==6&&std::string(argv[1])=="--base-reductions")return base_reductions(argv[2],argv[3],argv[4],unsigned(std::stoul(argv[5])));
    if(argc==5&&std::string(argv[1])=="--base16")return base16(argv[2],argv[3],argv[4]);
    if(argc!=8)throw std::runtime_error("Usage: fixture forward.spv inverse.spv math_dir ref|test mtf|la fresh_export");
    const Path source(argv[1]),math(argv[4]),output(argv[7]);const std::string label(argv[5]),mode(argv[6]);
    if((label!="ref"&&label!="test")||(mode!="mtf"&&mode!="la"))throw std::runtime_error("Invalid mode/label");
    if(!std::filesystem::create_directory(output))throw std::runtime_error("Fresh export directory required");
    const bool mtf=mode=="mtf";const std::string tag=label+"_"+mode;
    const auto input=read(source/(label+(mtf?"_luminance.txt":"_adapt_input.txt")),128,128);
    auto filter=read(source/(mode+"_filter_double.txt"),256,256);std::vector<float> generated(65536);
    if(mtf)IQM::HDRVDP3::generate_mtf_filter(generated.data(),256,256,30);
    else{
        const double sigma=std::pow(10.0,-0.781367)*30;
        for(unsigned y=0;y<256;++y)for(unsigned x=0;x<256;++x){
            const double kx=(std::fmod(0.5+double(x)/256,1.0)-0.5)*(2*3.14159265358979323846);
            const double ky=(std::fmod(0.5+double(y)/256,1.0)-0.5)*(2*3.14159265358979323846);
            generated[y*256+x]=float(std::exp(-0.5*(kx*kx+ky*ky)*sigma*sigma));
        }
    }
    size_t filterBad=0;for(size_t i=0;i<filter.size();++i)if(filter[i]!=generated[i])++filterBad;
    std::cout<<"filter float32 values=65536 mismatches="<<filterBad<<'\n';if(filterBad)throw std::runtime_error("Generated filter mismatch");
    VulkanInstance instance(true);const auto& device=*instance.device();
    const auto props=instance.physicalDevice()->getProperties2<vk::PhysicalDeviceProperties2,vk::PhysicalDeviceFloatControlsProperties>();
    const auto& controls=props.get<vk::PhysicalDeviceFloatControlsProperties>();
    if(!controls.shaderDenormPreserveFloat32||!controls.shaderRoundingModeRTEFloat32)throw std::runtime_error("Required float controls unsupported");
    auto imageInfo=vk::ImageCreateInfo{.imageType=vk::ImageType::e2D,.format=vk::Format::eR32Sfloat,.extent={128,128,1},.mipLevels=1,.arrayLayers=1,
        .samples=vk::SampleCountFlagBits::e1,.tiling=vk::ImageTiling::eOptimal,
        .usage=vk::ImageUsageFlagBits::eStorage|vk::ImageUsageFlagBits::eTransferSrc|vk::ImageUsageFlagBits::eTransferDst,.initialLayout=vk::ImageLayout::eUndefined};
    auto imageIn=VulkanResource::createImage(device,*instance.physicalDevice(),imageInfo),imageOut=VulkanResource::createImage(device,*instance.physicalDevice(),imageInfo);
    auto uploadBuffer=buffer(instance,65536),download=buffer(instance,65536),kernel=buffer(instance,262144);
    upload(uploadBuffer,input);upload(kernel,generated);
    auto prepared=buffer(instance,524288),scratch=buffer(instance,524288),forward=buffer(instance,524288),filtered=buffer(instance,524288),inverseScratch=buffer(instance,524288),inverse=buffer(instance,524288);
    const auto sb=vk::DescriptorType::eStorageBuffer,si=vk::DescriptorType::eStorageImage;
    Kernel prep(instance,math/"prepare.spv",{si,sb}),f0(instance,argv[2],{sb,sb}),f1(instance,argv[2],{sb,sb}),mul(instance,math/"multiply.spv",{sb,sb,sb}),i0(instance,argv[3],{sb,sb}),i1(instance,argv[3],{sb,sb}),finish(instance,math/"finish.spv",{sb,si});
    prep.bind(instance,0,imageIn.imageView);prep.bind(instance,1,prepared);
    f0.bind(instance,0,prepared);f0.bind(instance,1,scratch);f1.bind(instance,0,scratch);f1.bind(instance,1,forward);
    mul.bind(instance,0,forward);mul.bind(instance,1,kernel);mul.bind(instance,2,filtered);
    i0.bind(instance,0,filtered);i0.bind(instance,1,inverseScratch);i1.bind(instance,0,inverseScratch);i1.bind(instance,1,inverse);
    finish.bind(instance,0,inverse);finish.bind(instance,1,imageOut.imageView);
    vk::CommandBuffer cmd=**instance.cmdBuf();cmd.begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    const vk::ImageSubresourceRange range{vk::ImageAspectFlagBits::eColor,0,1,0,1};
    std::array<vk::ImageMemoryBarrier,2> init{};
    for(unsigned j=0;j<2;++j)init[j]=vk::ImageMemoryBarrier{.dstAccessMask=vk::AccessFlagBits::eTransferWrite|vk::AccessFlagBits::eShaderWrite,.oldLayout=vk::ImageLayout::eUndefined,.newLayout=vk::ImageLayout::eGeneral,
        .srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.image=j?*imageOut.image:*imageIn.image,.subresourceRange=range};
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,vk::PipelineStageFlagBits::eTransfer|vk::PipelineStageFlagBits::eComputeShader,{}, {},{},init);
    vk::BufferImageCopy copy{.imageSubresource={vk::ImageAspectFlagBits::eColor,0,0,1},.imageExtent={128,128,1}};
    cmd.copyBufferToImage(*uploadBuffer.handle,*imageIn.image,vk::ImageLayout::eGeneral,{copy});
    vk::MemoryBarrier uploaded{.srcAccessMask=vk::AccessFlagBits::eTransferWrite|vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer|vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {uploaded},{},{});
    const std::array<uint32_t,4> params{256,256,mtf?1u:0u,1};
    prep.run(cmd,params,16,16);f0.run(cmd,{256,256,3,0},8);f1.run(cmd,{256,129,4,0},5);
    mul.run(cmd,{256,256,0,0},16,16);i0.run(cmd,{256,256,3,0},8);i1.run(cmd,{256,256,4,0},8);finish.run(cmd,params,8,8);
    cmd.copyImageToBuffer(*imageOut.image,vk::ImageLayout::eGeneral,*download.handle,{copy});
    vk::MemoryBarrier downloaded{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{}, {downloaded},{},{});
    cmd.end();vk::SubmitInfo submit{.commandBufferCount=1,.pCommandBuffers=&cmd};vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(submit,fence);instance.waitForFence(fence);
    size_t bad=0;
    bad+=compare(prepared,read(source/(tag+"_stage0.txt"),256,512),output/"stage0.txt",256,512,false);
    bad+=compare(forward,read(source/(label+(mtf?"_native_forward.txt":"_la_native_forward.txt")),256,512),output/"stage1_row_major.txt",256,512,true);
    bad+=compare(filtered,read(source/(tag+"_stage2.txt"),256,512),output/"stage2.txt",256,512,false);
    bad+=compare(inverse,read(source/(label+(mtf?"_native_inverse.txt":"_la_native_inverse.txt")),256,512),output/"stage3_row_major.txt",256,512,true);
    bad+=compare(download,read(source/(label+(mtf?"_retinal.txt":"_adaptation.txt")),128,128),output/"output.txt",128,128,false);
    std::cout<<"filter_chain GPU values=540672 mismatches="<<bad<<'\n';return bad?1:0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
