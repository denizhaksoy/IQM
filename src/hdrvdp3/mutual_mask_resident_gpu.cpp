#include <IQM/hdrvdp3/mutual_mask_resident_gpu.h>
#include <IQM/base/vulkan_runtime.h>
#include <algorithm>
#include <fstream>
#include <stdexcept>
namespace IQM::HDRVDP3 {
namespace {
static std::vector<uint32_t> loadSpv(const std::filesystem::path& p){
    std::ifstream f(p,std::ios::binary|std::ios::ate);
    auto n=f.tellg(); if(n<20||n%4) throw std::runtime_error("bad spv: "+p.string());
    std::vector<uint32_t> v(size_t(n)/4); f.seekg(0); f.read(reinterpret_cast<char*>(v.data()),n);
    if(!f||v[0]!=0x07230203u) throw std::runtime_error("bad spv magic: "+p.string()); return v;
}
static void barrier(vk::CommandBuffer cmd){
    vk::MemoryBarrier b{.srcAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eShaderWrite};
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader,{},{b},{},{});
}
}
struct ResidentMutualMaskSingle::Impl {
    vk::raii::Device const* dev=nullptr;
    FFTBufferRegion ref{}, tst{}, out{};
    unsigned w=0,h=0;
    vk::raii::DescriptorSetLayout dsl=nullptr;
    vk::raii::PipelineLayout pl=nullptr;
    vk::raii::ShaderModule mod=nullptr;
    vk::raii::Pipeline pipe=nullptr;
    vk::raii::DescriptorPool pool=nullptr;
    vk::raii::DescriptorSet set=nullptr;
    uint32_t dispatches=0;
    Impl(const vk::raii::Device& d,const vk::raii::PhysicalDevice& phys,const std::filesystem::path& shaderSpv,FFTBufferRegion reference,FFTBufferRegion test,FFTBufferRegion output,unsigned width,unsigned height)
    :dev(&d),ref(reference),tst(test),out(output),w(width),h(height){
        if(!w||!h) throw std::invalid_argument("Mutual resident geometry");
        size_t n=size_t(w)*h; size_t bytes=n*4;
        if(ref.bytes<bytes||tst.bytes<bytes||out.bytes<bytes) throw std::invalid_argument("Mutual resident buffer range");
        const auto limits=phys.getProperties().limits;
        if(bytes>limits.maxStorageBufferRange) throw std::runtime_error("Mutual resident buffer limit");
        using R=IQM::GPU::VulkanRuntime;
        dsl=R::createDescLayout(d,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
        pl=R::createPipelineLayout(d,{*dsl},R::createPushConstantRange(16));
        mod=R::createShaderModule(d,loadSpv(shaderSpv));
        pipe=R::createComputePipeline(d,mod,pl);
        pool=R::createDescPool(d,1,{{vk::DescriptorType::eStorageBuffer,3}});
        const vk::DescriptorSetLayout raw=*dsl;
        auto sets=vk::raii::DescriptorSets(d,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});
        set=std::move(sets[0]);
        vk::DescriptorBufferInfo i0{ref.buffer,ref.offset,bytes}, i1{tst.buffer,tst.offset,bytes}, i2{out.buffer,out.offset,bytes};
        d.updateDescriptorSets({vk::WriteDescriptorSet{.dstSet=*set,.dstBinding=0,.descriptorCount=1,.descriptorType=vk::DescriptorType::eStorageBuffer,.pBufferInfo=&i0},
            vk::WriteDescriptorSet{.dstSet=*set,.dstBinding=1,.descriptorCount=1,.descriptorType=vk::DescriptorType::eStorageBuffer,.pBufferInfo=&i1},
            vk::WriteDescriptorSet{.dstSet=*set,.dstBinding=2,.descriptorCount=1,.descriptorType=vk::DescriptorType::eStorageBuffer,.pBufferInfo=&i2}}, {});
        dispatches=1;
    }
    void record(vk::CommandBuffer cmd) const {
        cmd.bindPipeline(vk::PipelineBindPoint::eCompute,*pipe);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute,*pl,0,{*set},{});
        struct Push { uint32_t count,width,height,unused; } p{uint32_t(size_t(w)*h),w,h,0};
        cmd.pushConstants<Push>(*pl,vk::ShaderStageFlagBits::eCompute,0,p);
        uint32_t total=p.count; uint32_t gx=(total+63)/64; // shader uses 64-wide local + 2D group logic? see dispatch in existing code
        // Existing mutual_single_gpu dispatches (gx,gy,1) with gx=min(total,limits) and gy=ceil(total/gx), local_size_x=64
        // Shader computes i=(gl_WorkGroupID.y*gl_NumWorkGroups.x+gl_WorkGroupID.x)*64+gl_LocalInvocationID.x
        // so dispatch must be (gx,gy,1) with same convention.
        // We need device limits; store gx/gy.
        cmd.dispatch(gxDisp,gyDisp,1);
        barrier(cmd);
    }
    uint32_t gxDisp=1,gyDisp=1;
};
ResidentMutualMaskSingle::ResidentMutualMaskSingle(const vk::raii::Device& d,const vk::raii::PhysicalDevice& p,const std::filesystem::path& s,FFTBufferRegion ref,FFTBufferRegion tst,FFTBufferRegion out,unsigned w,unsigned h)
:impl_(std::make_unique<Impl>(d,p,s,ref,tst,out,w,h)){
    const auto limits=p.getProperties().limits;
    uint32_t total=uint32_t((size_t(w)*h+63)/64);
    impl_->gxDisp=(std::min)(total,limits.maxComputeWorkGroupCount[0]);
    impl_->gyDisp=(total+impl_->gxDisp-1)/impl_->gxDisp;
    if(impl_->gyDisp>limits.maxComputeWorkGroupCount[1]) throw std::runtime_error("Mutual resident dispatch limit");
}
ResidentMutualMaskSingle::~ResidentMutualMaskSingle()=default;
ResidentMutualMaskSingle::ResidentMutualMaskSingle(ResidentMutualMaskSingle&&)noexcept=default;
ResidentMutualMaskSingle& ResidentMutualMaskSingle::operator=(ResidentMutualMaskSingle&&)noexcept=default;
void ResidentMutualMaskSingle::record(vk::CommandBuffer cmd) const { if(!impl_) throw std::runtime_error("null"); impl_->record(cmd); }
uint32_t ResidentMutualMaskSingle::dispatchCount() const noexcept { return impl_?impl_->dispatches:0; }
}