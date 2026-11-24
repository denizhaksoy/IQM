#include <IQM/hdrvdp3/csf_single_gpu.h>
#include <IQM/base/vulkan_runtime.h>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <cstdint>

namespace IQM::HDRVDP3 {

using IQM::GPU::VulkanRuntime;

namespace {
std::vector<uint32_t> load(const std::filesystem::path& path) {
    std::ifstream f(path,std::ios::binary|std::ios::ate); auto n=f.tellg();
    if(n<20||n%4) throw std::runtime_error("Invalid CSF shader: "+path.string());
    std::vector<uint32_t> v(size_t(n)/4); f.seekg(0); f.read(reinterpret_cast<char*>(v.data()),n);
    if(!f||v[0]!=0x07230203u) throw std::runtime_error("Invalid CSF SPIR-V");
    return v;
}
void barrier(vk::CommandBuffer cmd) {
    vk::MemoryBarrier b{.srcAccessMask=vk::AccessFlagBits::eMemoryWrite,
        .dstAccessMask=vk::AccessFlagBits::eMemoryRead|vk::AccessFlagBits::eMemoryWrite};
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,vk::PipelineStageFlagBits::eAllCommands,{}, {b},{},{});
}
struct Kernel {
    vk::raii::DescriptorSetLayout setLayout=nullptr;
    vk::raii::PipelineLayout layout=nullptr;
    vk::raii::ShaderModule module=nullptr;
    vk::raii::Pipeline pipeline=nullptr;
    vk::raii::DescriptorPool pool=nullptr;
    vk::raii::DescriptorSet set=nullptr;
    Kernel(const vk::raii::Device& d,const std::filesystem::path& path,
           const std::vector<FFTBufferRegion>& regions,uint32_t pushBytes) {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        for(uint32_t i=0;i<regions.size();++i) bindings.push_back({.binding=i,.descriptorType=vk::DescriptorType::eStorageBuffer,.descriptorCount=1,.stageFlags=vk::ShaderStageFlagBits::eCompute});
        setLayout=vk::raii::DescriptorSetLayout(d,vk::DescriptorSetLayoutCreateInfo{.bindingCount=uint32_t(bindings.size()),.pBindings=bindings.data()});
        layout=VulkanRuntime::createPipelineLayout(d,{*setLayout},VulkanRuntime::createPushConstantRange(pushBytes));
        module=VulkanRuntime::createShaderModule(d,load(path));
        pipeline=VulkanRuntime::createComputePipeline(d,module,layout);
        pool=VulkanRuntime::createDescPool(d,1,{{vk::DescriptorType::eStorageBuffer,uint32_t(regions.size())}});
        const vk::DescriptorSetLayout sl=*setLayout;
        auto sets=vk::raii::DescriptorSets(d,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&sl});
        set=std::move(sets[0]);
        for(uint32_t i=0;i<regions.size();++i){const auto& r=regions[i]; const std::vector<vk::DescriptorBufferInfo> info{{r.buffer,r.offset,r.bytes}};
            d.updateDescriptorSets({VulkanRuntime::createWriteSet(*set,i,info)},{});}
    }
    void dispatch(vk::CommandBuffer cmd,const std::vector<uint32_t>& push,uint32_t groups,uint32_t groupsY=1) {
        cmd.bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*set},{});
        cmd.pushConstants<uint32_t>(*layout,vk::ShaderStageFlagBits::eCompute,0,push);
        cmd.dispatch(groups,groupsY,1);
    }
};
}

struct CSFDispatchSingle::Impl {
    std::unique_ptr<Kernel> kernel;
    std::vector<uint32_t> push;
    unsigned gx,gy;

    Impl(
        const vk::raii::Device& d,
        const vk::raii::PhysicalDevice& p,
        const std::filesystem::path& runtime,
        CSFKernelSingle kind,
        const std::vector<FFTBufferRegion>& regions,
        const std::vector<uint32_t>& constants,
        unsigned x,unsigned y,bool fp64
    ):push(constants),gx(x),gy(y) {

        const char* filename=nullptr;
        unsigned bindings=0;
        unsigned words=0;

        switch(kind) {
        case CSFKernelSingle::Log:
            if(!fp64 || !p.getFeatures().shaderFloat64)
                throw std::invalid_argument(
                    "CSF log requires Float64"
                );
            filename="csf_log.spv";
            bindings=3;
            words=1;
            break;

        case CSFKernelSingle::Resize:
            filename="csf_resize.spv";
            bindings=5;
            words=6;
            break;

        case CSFKernelSingle::Generate:
            filename="csf_generate.spv";
            bindings=6;
            words=5;

            if(!fp64 || !p.getFeatures().shaderFloat64)
                throw std::invalid_argument(
                    "CSF generation requires Float64"
                );

            break;

        case CSFKernelSingle::Sample:
            filename="csf_sample.spv";
            bindings=5;
            words=1;
            break;

        case CSFKernelSingle::Neighbor:
            filename="neighbor_single.spv";
            bindings=3;
            words=3;
            break;

        case CSFKernelSingle::MaskedDifference:
            if(!fp64 || !p.getFeatures().shaderFloat64)
                throw std::invalid_argument(
                    "Masked difference requires Float64"
                );

            filename="masked_difference_single.spv";
            bindings=7;
            words=7;
            break;

        case CSFKernelSingle::Comparison:
            if(!fp64 || !p.getFeatures().shaderFloat64)
                throw std::invalid_argument(
                    "Resident comparison requires Float64"
                );

            filename="comparison_resident.spv";
            bindings=3;
            words=1;
            break;

        case CSFKernelSingle::ComparisonResize:
            if(!fp64 || !p.getFeatures().shaderFloat64)
                throw std::invalid_argument(
                    "Resident comparison resize requires Float64"
                );

            filename="comparison_resize_resident.spv";
            bindings=3;
            words=8;
            break;

        case CSFKernelSingle::PoolingTerms:
            if(!fp64 || !p.getFeatures().shaderFloat64)
                throw std::invalid_argument(
                    "Pooling terms require Float64"
                );

            filename="pooling_terms.spv";
            bindings=3;
            words=1;
            break;

        case CSFKernelSingle::PoolingReduce:
            if(!fp64 || !p.getFeatures().shaderFloat64)
                throw std::invalid_argument(
                    "Pooling reduction requires Float64"
                );

            if(gx!=1 || gy!=1)
                throw std::invalid_argument(
                    "Pooling reduction requires one workgroup"
                );

            filename="pooling_reduce.spv";
            bindings=2;
            words=2;
            break;

        case CSFKernelSingle::RGBResize:
            if(!fp64 || !p.getFeatures().shaderFloat64)
                throw std::invalid_argument(
                    "Resident RGB resize requires Float64"
                );

            filename="rgb_resize_resident.spv";
            bindings=5;
            words=8;
            break;

        case CSFKernelSingle::FinalScore:
            if(!fp64 || !p.getFeatures().shaderFloat64)
                throw std::invalid_argument(
                    "Final score requires Float64"
                );

            if(gx!=1 || gy!=1)
                throw std::invalid_argument(
                    "Final score requires one workgroup"
                );

            if(constants.size()!=1 ||
               constants[0]!=6u)
                throw std::invalid_argument(
                    "Final score requires six bands"
                );

            filename="final_score_single.spv";
            bindings=7;
            words=1;
            break;

        default:
            throw std::invalid_argument(
                "Unknown CSF kernel"
            );
        }

        if(regions.size()!=bindings ||
           push.size()!=words)
            throw std::invalid_argument(
                "Invalid CSF descriptor/push shape"
            );

        const auto limits=p.getProperties().limits;

        if(!gx || !gy ||
           gx>limits.maxComputeWorkGroupCount[0] ||
           gy>limits.maxComputeWorkGroupCount[1])
            throw std::invalid_argument(
                "Invalid CSF dispatch dimensions"
            );

        for(size_t i=0;i<regions.size();++i) {
            const auto& r=regions[i];

            if(!r.buffer ||
               !r.bytes ||
               r.bytes>limits.maxStorageBufferRange ||
               r.offset%limits.minStorageBufferOffsetAlignment)
                throw std::invalid_argument(
                    "Invalid CSF buffer region"
                );

            for(size_t j=0;j<i;++j)
                if(r.buffer==regions[j].buffer)
                    throw std::invalid_argument(
                        "CSF buffers must be distinct"
                    );
        }

        kernel=std::make_unique<Kernel>(
            d,runtime/filename,regions,words*4
        );
    }
};

CSFDispatchSingle::CSFDispatchSingle(
    const vk::raii::Device& d,
    const vk::raii::PhysicalDevice& p,
    const std::filesystem::path& runtime,
    CSFKernelSingle kind,
    const std::vector<FFTBufferRegion>& regions,
    const std::vector<uint32_t>& push,
    unsigned gx,unsigned gy,bool fp64
):impl_(std::make_unique<Impl>(
    d,p,runtime,kind,regions,push,gx,gy,fp64
)){}

CSFDispatchSingle::~CSFDispatchSingle()=default;

void CSFDispatchSingle::record(vk::CommandBuffer cmd) {
    if(!cmd)
        throw std::invalid_argument(
            "Null CSF command buffer"
        );

    barrier(cmd);

    impl_->kernel->dispatch(
        cmd,impl_->push,impl_->gx,impl_->gy
    );

    barrier(cmd);
}

}