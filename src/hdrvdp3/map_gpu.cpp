#include <IQM/hdrvdp3/map_gpu.h>
#include <IQM/hdrvdp3/resize.h>
#include <IQM/base/vulkan_runtime.h>
#include <vulkan_res.h>
#include <vulkan.h>
#include <array>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
namespace IQM::HDRVDP3 {
using IQM::GPU::VulkanRuntime;
namespace {
struct Buffer { vk::raii::DeviceMemory memory=nullptr; vk::raii::Buffer handle=nullptr; };
Buffer allocate(const IQM::VulkanInstance& instance,uint32_t bytes) {
    auto allocation=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),bytes,
        vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
    Buffer result; result.handle=std::move(allocation.first); result.memory=std::move(allocation.second);
    result.handle.bindMemory(result.memory,0); return result;
}
uint32_t pixels(int h,int w) {
    if(h<=0||w<=0||uint64_t(h)*w>std::numeric_limits<uint32_t>::max()/4)
        throw std::invalid_argument("Invalid GPU map dimensions");
    return uint32_t(h)*uint32_t(w);
}
struct Push { uint32_t count,mode,width,height,out_width,out_height,taps,plan_offset; };
}
struct MapGPU::State {
    const IQM::VulkanInstance& instance;
    vk::raii::ShaderModule module=nullptr;
    vk::raii::DescriptorPool pool=nullptr;
    vk::raii::DescriptorSetLayout set_layout=nullptr;
    vk::raii::PipelineLayout layout=nullptr;
    vk::raii::Pipeline pipeline=nullptr;
    State(const IQM::VulkanInstance& owner,const std::string& shader_path,uint32_t expected_abi=13):instance(owner) {
        const auto& device=*instance.device();
        std::ifstream file(shader_path,std::ios::binary|std::ios::ate);
        auto bytes=file.tellg(); if(bytes<=0||bytes%4) throw std::runtime_error("Invalid map shader");
        std::vector<uint32_t> code(static_cast<size_t>(bytes)/4); file.seekg(0);
        if(!file.read(reinterpret_cast<char*>(code.data()),bytes)) throw std::runtime_error("Truncated map shader");
        if(code.size()<5||code[0]!=0x07230203u) throw std::runtime_error("Invalid map SPIR-V header");
        uint32_t abi_id=0; bool matching_abi=false;
        for(size_t i=5;i<code.size();) {
            const auto count=code[i]>>16,op=code[i]&65535u;
            if(!count||i+count>code.size()) throw std::runtime_error("Invalid map SPIR-V instruction");
            if(op==71&&count==4&&code[i+2]==1&&code[i+3]==31) abi_id=code[i+1];
            i+=count;
        }
        for(size_t i=5;i<code.size();i+=code[i]>>16)
            if((code[i]&65535u)==50&&(code[i]>>16)==4&&code[i+2]==abi_id&&code[i+3]==expected_abi) matching_abi=true;
        if(!abi_id||!matching_abi) throw std::runtime_error("MapGPU requires shader ABI"+std::to_string(expected_abi)+"; regenerate the selected shader");
        module=VulkanRuntime::createShaderModule(device,code);
        pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,2}});
        set_layout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
        layout=VulkanRuntime::createPipelineLayout(device,{*set_layout},VulkanRuntime::createPushConstantRange(sizeof(Push)));
        pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
    }
    std::vector<uint32_t> execute_words(const std::vector<uint32_t>& words,const Push& push) {
        if(words.empty()||words.size()>std::numeric_limits<uint32_t>::max()/4||!push.count||push.count>std::numeric_limits<uint32_t>::max()/4)
            throw std::invalid_argument("Invalid GPU map buffer size");
        const uint32_t input_bytes=uint32_t(words.size()*4),output_bytes=push.count*4;
        auto input=allocate(instance,input_bytes),output=allocate(instance,output_bytes);
        void* mapped=input.memory.mapMemory(0,input_bytes); std::memcpy(mapped,words.data(),input_bytes); input.memory.unmapMemory();
        const auto& device=*instance.device(); vk::DescriptorSetLayout raw=*set_layout;
        vk::raii::DescriptorSets sets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});
        const std::vector<vk::DescriptorBufferInfo> in_info{{*input.handle,0,input_bytes}},out_info{{*output.handle,0,output_bytes}};
        device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,in_info),VulkanRuntime::createWriteSet(*sets[0],1,out_info)},{});
        auto cmd=instance.cmdBuf(); cmd->reset();
        cmd->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        const vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
        cmd->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
        cmd->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);
        cmd->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});
        cmd->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,push);
        cmd->dispatch((push.count+63)/64,1,1);
        const vk::MemoryBarrier after{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};
        cmd->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eHost,{}, {after},{},{});
        cmd->end(); vk::CommandBuffer raw_cmd=**cmd;
        vk::SubmitInfo submit{.commandBufferCount=1,.pCommandBuffers=&raw_cmd};
        vk::raii::Fence fence(device,vk::FenceCreateInfo{}); instance.queue()->submit(submit,fence); instance.waitForFence(fence);
        std::vector<uint32_t> result(push.count); mapped=output.memory.mapMemory(0,output_bytes);
        std::memcpy(result.data(),mapped,output_bytes); output.memory.unmapMemory(); return result;
    }
    std::vector<float> execute(const std::vector<uint32_t>& words,const Push& push) {
        const auto raw=execute_words(words,push);
        std::vector<float> result(raw.size()); std::memcpy(result.data(),raw.data(),raw.size()*4); return result;
    }
    std::vector<float> axis(const std::vector<float>& input,int h,int w,int nh,int nw,bool horizontal) {
        const int n=horizontal?w:h,m=horizontal?nw:nh;
        if(n==m) return input;
        const auto plan=make_resize_axis_plan(n,m);
        std::vector<uint32_t> words; words.reserve(input.size()+size_t(m)*(1+2*plan.taps));
        for(float value:input) words.push_back(std::bit_cast<uint32_t>(value));
        const uint32_t offset=uint32_t(words.size());
        for(int d=0;d<m;++d) {
            words.push_back(std::bit_cast<uint32_t>(static_cast<float>(plan.normalization[d])));
            for(int tap=0;tap<plan.taps;++tap) {
                const size_t i=size_t(d)*plan.taps+tap;
                words.push_back(uint32_t(plan.indices[i]));
                words.push_back(std::bit_cast<uint32_t>(static_cast<float>(plan.weights[i])));
            }
        }
        return execute(words,{pixels(nh,nw),horizontal?0u:1u,uint32_t(w),uint32_t(h),uint32_t(nw),uint32_t(nh),uint32_t(plan.taps),offset});
    }
};
MapGPU::MapGPU(const IQM::VulkanInstance& instance,const std::string& shader_path):state(std::make_unique<State>(instance,shader_path)){}
MapGPU::~MapGPU()=default;
std::vector<float> MapGPU::resize(const std::vector<float>& input,int h,int w,int nh,int nw) {
    if(input.size()!=pixels(h,w)) throw std::invalid_argument("GPU resize input shape mismatch"); pixels(nh,nw);
    auto horizontal=state->axis(input,h,w,h,nw,true);
    return state->axis(horizontal,h,nw,nh,nw,false);
}
std::vector<float> MapGPU::sample_csf(const std::vector<float>& input,const CSFLUT& lut) {
    const auto table=create_csf_single_lut(lut); std::vector<uint32_t> words;
    for(float value:input) words.push_back(std::bit_cast<uint32_t>(value));
    const uint32_t offset=uint32_t(words.size());
    for(const auto& values:{table.log_luminance,table.sensitivity}) for(float value:values) words.push_back(std::bit_cast<uint32_t>(value));
    for(float value:table.slope) words.push_back(std::bit_cast<uint32_t>(value));
    return state->execute(words,{uint32_t(input.size()),2,0,0,0,0,0,offset});
}
std::vector<float> MapGPU::clamp_log(const std::vector<float>& input) {
    std::vector<uint32_t> words; words.reserve(input.size());
    for(float value:input) words.push_back(std::bit_cast<uint32_t>(value));
    return state->execute(words,{uint32_t(input.size()),5,0,0,0,0,0,0});
}
std::vector<float> MapGPU::neighbor_mask(const std::vector<float>& previous,const std::vector<float>& next) {
    if(previous.size()!=next.size()) throw std::invalid_argument("GPU neighbor shape mismatch");
    std::vector<uint32_t> words;
    for(const auto* map:{&previous,&next}) for(float value:*map) words.push_back(std::bit_cast<uint32_t>(value));
    return state->execute(words,{uint32_t(previous.size()),7,0,0,0,0,0,uint32_t(previous.size())});
}
std::vector<float> MapGPU::masking_denominator(const std::vector<float>& self,const std::vector<float>& neighbor,float q,float self_scale,float neighbor_scale) {
    if(self.size()!=neighbor.size()) throw std::invalid_argument("GPU masking denominator shape mismatch");
    std::vector<uint32_t> words;
    for(const auto* map:{&self,&neighbor}) for(float value:*map) words.push_back(std::bit_cast<uint32_t>(value));
    words.push_back(std::bit_cast<uint32_t>(q)); words.push_back(std::bit_cast<uint32_t>(self_scale)); words.push_back(std::bit_cast<uint32_t>(neighbor_scale));
    return state->execute(words,{uint32_t(self.size()),6,0,0,0,0,0,uint32_t(self.size())});
}
std::vector<float> MapGPU::difference(const std::vector<float>& difference,const std::vector<float>& sensitivity,const std::vector<float>& mask,float p,float base_neural) {
    if(difference.size()!=sensitivity.size()||difference.size()!=mask.size()) throw std::invalid_argument("GPU difference shape mismatch");
    std::vector<uint32_t> words;
    for(const auto* map:{&difference,&sensitivity,&mask}) for(float value:*map) words.push_back(std::bit_cast<uint32_t>(value));
    words.push_back(std::bit_cast<uint32_t>(p)); words.push_back(std::bit_cast<uint32_t>(base_neural));
    return state->execute(words,{uint32_t(difference.size()),8,0,0,0,0,0,uint32_t(difference.size())});
}
std::vector<float> MapGPU::psychometric(const std::vector<float>& input,float exponent) {
    std::vector<uint32_t> words;
    for(float value:input) words.push_back(std::bit_cast<uint32_t>(value));
    return state->execute(words,{uint32_t(input.size()),9,0,0,0,0,std::bit_cast<uint32_t>(exponent),0});
}
float MapGPU::quality_band_error(const std::vector<float>& difference,const std::vector<double>& comparison_mask,unsigned width,unsigned height) {
    const uint32_t count=pixels(height,width);
    if(difference.size()!=count||comparison_mask.size()!=count) throw std::invalid_argument("GPU quality band shape mismatch");
    std::vector<uint32_t> words;
    for(float value:difference) words.push_back(std::bit_cast<uint32_t>(value));
    for(double value:comparison_mask) words.push_back(std::bit_cast<uint32_t>(static_cast<float>(value)));
    return state->execute(words,{1,10,width,height,0,0,0,count})[0];
}
QualityScores MapGPU::quality_scores(const std::vector<float>& band_errors) {
    if(band_errors.empty()) throw std::invalid_argument("Empty GPU quality bands");
    std::vector<uint32_t> words;
    for(float value:band_errors) words.push_back(std::bit_cast<uint32_t>(value));
    words.push_back(std::bit_cast<uint32_t>(static_cast<float>(std::exp(-4.0))));
    const auto values=state->execute(words,{3,11,uint32_t(band_errors.size()),0,0,0,0,0});
    return {values[0],values[1],values[2]};
}
std::array<float,4> MapGPU::pupil_trace(const std::vector<float>& adaptation,unsigned width,unsigned height,double area,double age) {
    const uint32_t count=pixels(height,width);
    if(adaptation.size()!=count||!std::isfinite(area)||area<=0||!std::isfinite(age)) throw std::invalid_argument("Invalid GPU pupil input");
    std::vector<uint32_t> words;
    for(float value:adaptation) {
        if(!std::isfinite(value)||value<=0) throw std::invalid_argument("Nonpositive GPU adaptation luminance");
        words.push_back(std::bit_cast<uint32_t>(value));
    }
    words.push_back(std::bit_cast<uint32_t>(static_cast<float>(area)));
    words.push_back(std::bit_cast<uint32_t>(static_cast<float>(std::clamp(age,20.0,83.0)-28.58)));
    words.push_back(std::bit_cast<uint32_t>(static_cast<float>(28.0-28.58)));
    const auto values=state->execute(words,{4,12,width,height,0,0,0,count});
    return {values[0],values[1],values[2],values[3]};
}
std::vector<float> MapGPU::mean_adaptation(const std::vector<float>& reference,const std::vector<float>& test) {
    if(reference.size()!=test.size()) throw std::invalid_argument("GPU adaptation shape mismatch");
    std::vector<uint32_t> words;
    for(const auto* map:{&reference,&test}) for(float value:*map) words.push_back(std::bit_cast<uint32_t>(value));
    return state->execute(words,{uint32_t(reference.size()),13,0,0,0,0,0,uint32_t(reference.size())});
}
std::vector<float> MapGPU::adaptation_input(const std::vector<float>& retinal,const std::array<double,3>& matrix) {
    std::vector<uint32_t> words;
    for(float value:retinal) words.push_back(std::bit_cast<uint32_t>(value));
    for(size_t ch=0;ch<2;++ch)words.push_back(std::bit_cast<uint32_t>(static_cast<float>(matrix[ch])));
    return state->execute(words,{uint32_t(retinal.size()),14,0,0,0,0,0,uint32_t(retinal.size())});
}
std::vector<float> MapGPU::log_adaptation(const std::vector<float>& input) {
    std::vector<uint32_t> words;
    for(float value:input) words.push_back(std::bit_cast<uint32_t>(value));
    return state->execute(words,{uint32_t(input.size()),16,0,0,0,0,0,0});
}
std::vector<float> MapGPU::normalized_difference(const std::vector<float>& reference,const std::vector<float>& test,float normalization) {
    if(reference.size()!=test.size()||!std::isfinite(normalization)||normalization<=0)
        throw std::invalid_argument("Invalid GPU difference preparation input");
    std::vector<uint32_t> words;
    for(const auto* map:{&reference,&test}) for(float value:*map) {
        if(!std::isfinite(value)) throw std::invalid_argument("Nonfinite GPU band input");
        words.push_back(std::bit_cast<uint32_t>(value));
    }
    return state->execute(words,{uint32_t(reference.size()),17,0,0,0,0,std::bit_cast<uint32_t>(normalization),uint32_t(reference.size())});
}
std::vector<double> MapGPU::comparison_mask(const std::vector<float>& reference,const std::vector<float>& test) {
    if(reference.size()!=test.size()) throw std::invalid_argument("GPU comparison mask shape mismatch");
    std::vector<uint32_t> words;
    for(float value:reference) {
        if(!std::isfinite(value)||value<=0) throw std::invalid_argument("GPU comparison mask requires positive finite reference luminance");
        words.push_back(std::bit_cast<uint32_t>(value));
    }
    for(float value:test) {
        if(!std::isfinite(value)) throw std::invalid_argument("Nonfinite GPU comparison luminance");
        words.push_back(std::bit_cast<uint32_t>(value));
    }
    const auto values=state->execute(words,{uint32_t(reference.size()),18,0,0,0,0,0,uint32_t(reference.size())});
    // Widen the GPU-computed binary mask for the still-double resize pathway.
    return std::vector<double>(values.begin(),values.end());
}
std::vector<float> MapGPU::remove_residual_dc(const std::vector<float>& input) {
    if(input.size()!=64) throw std::invalid_argument("GPU residual DC currently requires64 samples");
    int minimum=1000,maximum=-1000; std::vector<uint32_t> words;
    for(float value:input) {
        if(!std::isfinite(value)) throw std::invalid_argument("Nonfinite GPU residual");
        if(value!=0.0f) { const int e=std::ilogb(std::abs(value)); minimum=std::min(minimum,e); maximum=std::max(maximum,e); }
        words.push_back(std::bit_cast<uint32_t>(value));
    }
    // At most24+16+6=46 significant bits in any partial sum. Native double
    // summation is exact in this domain; the compensated float pair retains it.
    // Restrict exponents so additions/subtractions cannot flush subnormals.
    if(maximum!=-1000&&(maximum-minimum>16||minimum < -90||maximum>100))
        throw std::invalid_argument("GPU DC input exceeds validated exponent domain");
    return state->execute(words,{64,19,0,0,0,0,0,0});
}
std::array<float,2> MapGPU::base_adaptation(const std::vector<float>& input) {
    if(input.size()!=64) throw std::invalid_argument("GPU base adaptation currently requires64 samples");
    int minimum=1000,maximum=-1000; std::vector<uint32_t> words;
    for(float value:input) {
        if(!std::isfinite(value)||value < -5.0f||value > 5.0f)
            throw std::invalid_argument("GPU base adaptation log outside[-5,5]");
        if(value!=0.0f) { const int e=std::ilogb(std::abs(value)); minimum=std::min(minimum,e); maximum=std::max(maximum,e); }
        words.push_back(std::bit_cast<uint32_t>(value));
    }
    // The exact sum needs at most46 bits. Power-of-two division and a single
    // rounding reproduce Octave's double accumulation of single inputs.
    if(maximum!=-1000&&(maximum-minimum>16||minimum < -90))
        throw std::invalid_argument("GPU base adaptation exceeds validated exponent domain");
    const auto values=state->execute(words,{2,20,0,0,0,0,0,0});
    return {values[0],values[1]};
}
std::vector<double> MapGPU::resize_comparison_mask(const std::vector<double>& input,int h,int w,int nh,int nw) {
    if(h!=w||(h!=32&&h!=64)||nh!=nw||nh>h||(nh!=8&&nh!=16&&nh!=32&&nh!=64)||input.size()!=size_t(h)*w)
        throw std::invalid_argument("Unvalidated GPU binary mask resize dimensions");
    std::vector<uint32_t> words;
    for(double v:input) {
        if(v!=0.0&&v!=1.0) throw std::invalid_argument("GPU comparison resize requires binary mask");
        words.push_back(v==1.0?1u:0u);
    }
    auto append_plan=[](std::vector<uint32_t>& packed,const ResizeAxisPlan& plan,int n) {
        for(int d=0;d<n;++d) {
            if(plan.normalization[d]!=1.0) throw std::invalid_argument("GPU mask resize requires unit normalization");
            double absolute_sum=0;
            for(int t=0;t<plan.taps;++t) {
                const auto i=size_t(d)*plan.taps+t; const double weight=plan.weights[i];
                const double fixed=std::ldexp(weight,16); absolute_sum+=std::abs(weight);
                if(!std::isfinite(fixed)||fixed!=std::trunc(fixed)||std::abs(fixed)>131072.0)
                    throw std::invalid_argument("GPU mask resize requires exact Q16 weights");
                packed.push_back(uint32_t(plan.indices[i]));
                packed.push_back(std::bit_cast<uint32_t>(static_cast<int32_t>(fixed)));
            }
            if(absolute_sum>2.0) throw std::invalid_argument("GPU mask resize exceeds exact accumulation bound");
        }
    };
    const auto horizontal=make_resize_axis_plan(w,nw),vertical=make_resize_axis_plan(h,nh);
    append_plan(words,horizontal,nw);
    const auto first=state->execute_words(words,{uint32_t(h*nw),23,uint32_t(w),uint32_t(h),uint32_t(nw),uint32_t(h),uint32_t(horizontal.taps),uint32_t(h*w)});
    words=first; const uint32_t offset=uint32_t(words.size()); append_plan(words,vertical,nh);
    const auto raw=state->execute_words(words,{uint32_t(2*nh*nw),24,uint32_t(nw),uint32_t(h),uint32_t(nw),uint32_t(nh),uint32_t(vertical.taps),offset});
    std::vector<double> result(size_t(nh)*nw);
    for(size_t i=0;i<result.size();++i) result[i]=std::bit_cast<double>(uint64_t(raw[2*i])|(uint64_t(raw[2*i+1])<<32));
    return result;
}
std::vector<std::array<float,5>> MapGPU::base_csf_parameters(const std::vector<float>& input) {
    if(input.empty()||input.size()>std::numeric_limits<uint32_t>::max()/20)
        throw std::invalid_argument("Invalid GPU base CSF query count");
    std::vector<uint32_t> words;
    for(float v:input) {
        if(!std::isfinite(v)||v<1e-5f||v>1e5f) throw std::invalid_argument("GPU base CSF luminance outside[1e-5,1e5]");
        words.push_back(std::bit_cast<uint32_t>(v));
    }
    const auto lut=create_csf_parameter_single_lut();
    for(float v:lut.log_luminance) words.push_back(std::bit_cast<uint32_t>(v));
    for(float v:lut.values) words.push_back(std::bit_cast<uint32_t>(v));
    for(float v:lut.slopes) words.push_back(std::bit_cast<uint32_t>(v));
    const auto values=state->execute(words,{uint32_t(5*input.size()),22,uint32_t(input.size()),0,0,0,0,uint32_t(input.size())});
    std::vector<std::array<float,5>> result(input.size());
    for(size_t i=0;i<input.size();++i) for(size_t k=0;k<5;++k) result[i][k]=values[5*i+k];
    return result;
}
BaseSelectionGPU MapGPU::base_selection(const std::vector<float>& input) {
    if(input.size()!=128) throw std::invalid_argument("GPU base selection requires8x8 complex spectrum");
    std::vector<uint32_t> words;
    for(float value:input) {
        if(!std::isfinite(value)) throw std::invalid_argument("Nonfinite GPU base spectrum");
        if(value!=0.0f&&(std::ilogb(std::abs(value)) < -90||std::ilogb(std::abs(value))>100))
            throw std::invalid_argument("GPU base spectrum exceeds validated exponent domain");
        words.push_back(std::bit_cast<uint32_t>(value));
    }
    auto values=state->execute(words,{65,21,0,0,0,0,0,0});
    const auto index=std::bit_cast<uint32_t>(values[64]); values.resize(64);
    return {std::move(values),index};
}
std::vector<float> MapGPU::base_spectrum(const std::vector<float>& input,const std::string& shader_path) {
    if(input.size()!=64) throw std::invalid_argument("GPU base FFT currently requires8x8 input");
    std::vector<uint32_t> words(256,0);
    for(size_t i=0;i<input.size();++i) {
        if(!std::isfinite(input[i])) throw std::invalid_argument("Nonfinite GPU base FFT input");
        words[2*i]=std::bit_cast<uint32_t>(input[i]);
    }
    State fft(state->instance,shader_path,81);
    // FFT probe ABI uses the first16 push bytes: n(unused), batches=1,
    // graph(unused), fused(unused). Output is128 interleaved pairs with padding.
    auto result=fft.execute(words,{256,1,0,0,0,0,0,0});
    result.resize(128); return result;
}
std::vector<JNDResponseSingle> MapGPU::jnd_response(const std::vector<float>& luminance,const JNDLUT& lut,float pupil_reduction,const std::array<double,3>& matrix) {
    if(luminance.empty()||luminance.size()>std::numeric_limits<uint32_t>::max()/52)
        throw std::invalid_argument("GPU JND dimensions exceed output size");
    if(!std::isfinite(pupil_reduction)||pupil_reduction<=0) throw std::invalid_argument("Invalid GPU pupil reduction");
    std::vector<uint32_t> words;
    for(float value:luminance) {
        if(!std::isfinite(value)) throw std::invalid_argument("Nonfinite GPU JND input");
        words.push_back(std::bit_cast<uint32_t>(value));
    }
    for(const auto* table:{&lut.log_luminance,&lut.cone,&lut.rod})
        for(double value:*table) words.push_back(std::bit_cast<uint32_t>(static_cast<float>(value)));
    for(const auto* table:{&lut.cone,&lut.rod}) for(size_t i=0;i+1<JND_LUT_SIZE;++i) {
        const float slope=static_cast<float>(((*table)[i+1]-(*table)[i])/(lut.log_luminance[i+1]-lut.log_luminance[i]));
        words.push_back(std::bit_cast<uint32_t>(slope));
    }
    words.push_back(std::bit_cast<uint32_t>(pupil_reduction));
    for(double value:matrix)
        words.push_back(std::bit_cast<uint32_t>(static_cast<float>(value)));
    const uint32_t n=uint32_t(luminance.size());
    const auto packed=state->execute(words,{13*n,15,n,0,0,0,0,n});
    std::vector<JNDResponseSingle> result(n);
    for(size_t i=0;i<n;++i) {
        for(size_t ch=0;ch<3;++ch) {
            result[i].lmr_before[ch]=packed[13*i+ch]; result[i].lmr_after[ch]=packed[13*i+3+ch];
            result[i].log_query[ch]=packed[13*i+6+ch]; result[i].channels[ch]=packed[13*i+9+ch];
        }
        result[i].response=packed[13*i+12];
    }
    return result;
}
MaskingMapsGPU MapGPU::mutual_masking(const std::vector<float>& reference,const std::vector<float>& test,int h,int w,float normalization) {
    const uint32_t count=pixels(h,w);
    if(reference.size()!=count||test.size()!=count) throw std::invalid_argument("GPU masking shape mismatch");
    // Native Octave conv2's accumulation dispatch is pinned only for these
    // square pyramid bands; other shapes need their own validation.
    if(h!=w || (h!=8&&h!=16&&h!=32&&h!=64)) throw std::invalid_argument("Unvalidated GPU masking dimensions");
    int exponent=0;
    if(!std::isfinite(normalization)||normalization<1||normalization>16||std::frexp(normalization,&exponent)!=0.5f)
        throw std::invalid_argument("GPU masking requires power-of-two normalization");
    std::vector<uint32_t> words; words.reserve(size_t(count)*2);
    for(float value:reference) { if(!std::isfinite(value)) throw std::invalid_argument("Nonfinite GPU mask input"); words.push_back(std::bit_cast<uint32_t>(value)); }
    for(float value:test) { if(!std::isfinite(value)) throw std::invalid_argument("Nonfinite GPU mask input"); words.push_back(std::bit_cast<uint32_t>(value)); }
    MaskingMapsGPU result;
    result.raw=state->execute(words,{count,3,uint32_t(w),uint32_t(h),uint32_t(w),uint32_t(h),std::bit_cast<uint32_t>(1.0f/normalization),count});
    words.clear(); for(float value:result.raw) words.push_back(std::bit_cast<uint32_t>(value));
    result.pooled=state->execute(words,{count,4,uint32_t(w),uint32_t(h),uint32_t(w),uint32_t(h),0,0});
    return result;
}
}
