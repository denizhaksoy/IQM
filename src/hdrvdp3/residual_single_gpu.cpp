#include <IQM/hdrvdp3/residual_single_gpu.h>
#include <IQM/base/vulkan_runtime.h>
#include <vkFFT.h>
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <array>
namespace IQM::HDRVDP3 {
using IQM::GPU::VulkanRuntime;
namespace {
std::vector<uint32_t> load(const std::filesystem::path& path) {
    std::ifstream f(path,std::ios::binary|std::ios::ate); auto n=f.tellg();
    if(n<20||n%4) throw std::runtime_error("Invalid residual shader: "+path.string());
    std::vector<uint32_t> v(size_t(n)/4); f.seekg(0); f.read(reinterpret_cast<char*>(v.data()),n);
    if(!f||v[0]!=0x07230203u) throw std::runtime_error("Invalid residual SPIR-V");
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
    void dispatch(vk::CommandBuffer cmd,const std::vector<uint32_t>& push,uint32_t groups) {
        cmd.bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*set},{});
        cmd.pushConstants<uint32_t>(*layout,vk::ShaderStageFlagBits::eCompute,0,push);
        cmd.dispatch(groups,1,1);
    }
};
}
struct ResidentResidualSingle::Impl {
    unsigned w,h; double base;
    VkDevice device; VkPhysicalDevice physical; VkQueue queue; VkCommandPool pool;
    vk::raii::Fence fence; VkFence rawFence;
    VkBuffer spectrum; uint64_t bytes;
    VkFFTApplication app{}; bool initialized=false;
    std::unique_ptr<Kernel> prepare,argmax,frequency;
    Impl(const vk::raii::Device& d,const vk::raii::PhysicalDevice& p,vk::Queue q,vk::CommandPool cp,
         const ResidualGpuBuffers& b,const std::filesystem::path& runtime,unsigned width,unsigned height,unsigned bands,double ppd,bool fp64)
        :w(width),h(height),base(std::ldexp(1.,-int(bands-1))*ppd/2),device(static_cast<VkDevice>(*d)),physical(static_cast<VkPhysicalDevice>(*p)),queue(static_cast<VkQueue>(q)),pool(static_cast<VkCommandPool>(cp)),fence(d,vk::FenceCreateInfo{}),rawFence(static_cast<VkFence>(*fence)),spectrum(static_cast<VkBuffer>(b.spectrum.buffer)),bytes(uint64_t(w)*h*8) {
        if(w!=240||h!=135||bands!=6||!std::isfinite(ppd)||ppd<=0||!fp64||!p.getFeatures().shaderFloat64||!q||!cp) throw std::invalid_argument("Unsupported residual configuration");
        const auto limits=p.getProperties().limits;
        const std::array<FFTBufferRegion,5> rs{b.reference,b.test,b.spectrum,b.selection,b.frequency};
        const std::array<uint64_t,5> need{bytes/2,bytes/2,bytes,12,8};
        for(size_t i=0;i<rs.size();++i){const auto& r=rs[i]; if(!r.buffer||r.offset!=0||r.bytes<need[i]||r.bytes>limits.maxStorageBufferRange)throw std::invalid_argument("Invalid residual buffer region");
            for(size_t j=0;j<i;++j)if(r.buffer==rs[j].buffer)throw std::invalid_argument("Residual buffers must be distinct");}
        if(b.spectrum.bytes!=bytes)throw std::invalid_argument("Residual FFT buffer size must be exact");
        prepare=std::make_unique<Kernel>(d,runtime/"residual_prepare.spv",std::vector<FFTBufferRegion>{b.reference,b.test,b.spectrum},8);
        argmax=std::make_unique<Kernel>(d,runtime/"residual_argmax.spv",std::vector<FFTBufferRegion>{b.spectrum,b.selection},4);
        frequency=std::make_unique<Kernel>(d,runtime/"residual_frequency.spv",std::vector<FFTBufferRegion>{b.selection,b.frequency},16);
        VkFFTConfiguration cfg{}; cfg.FFTdim=2; cfg.size[0]=h; cfg.size[1]=w; cfg.numberBatches=1; cfg.bufferNum=1;
        cfg.buffer=&spectrum; cfg.bufferSize=&bytes; cfg.device=&device; cfg.physicalDevice=&physical; cfg.queue=&queue; cfg.commandPool=&pool; cfg.fence=&rawFence;
        cfg.makeForwardPlanOnly=1; cfg.performR2C=0; cfg.doublePrecision=0; cfg.isCompilerInitialized=0;
        const auto status=initializeVkFFT(&app,cfg);
        if(status!=VKFFT_SUCCESS)throw std::runtime_error("Residual VkFFT initialization failed: "+std::to_string(int(status)));
        initialized=true;
    }
    ~Impl(){if(initialized)deleteVkFFT(&app);}
};
ResidentResidualSingle::ResidentResidualSingle(const vk::raii::Device& d,const vk::raii::PhysicalDevice& p,vk::Queue q,vk::CommandPool cp,const ResidualGpuBuffers& b,const std::filesystem::path& runtime,unsigned w,unsigned h,unsigned bands,double ppd,bool fp64):impl_(std::make_unique<Impl>(d,p,q,cp,b,runtime,w,h,bands,ppd,fp64)){}
ResidentResidualSingle::~ResidentResidualSingle()=default;
void ResidentResidualSingle::record(vk::CommandBuffer cmd){
    auto& g=*impl_; if(!cmd)throw std::invalid_argument("Null residual command buffer");
    barrier(cmd); g.prepare->dispatch(cmd,{g.w,g.h},(g.w*g.h+63)/64); barrier(cmd);
    VkCommandBuffer raw=static_cast<VkCommandBuffer>(cmd); VkFFTLaunchParams launch{}; launch.commandBuffer=&raw; launch.buffer=&g.spectrum;
    const auto status=VkFFTAppend(&g.app,-1,&launch); if(status!=VKFFT_SUCCESS)throw std::runtime_error("Residual VkFFT append failed: "+std::to_string(int(status)));
    barrier(cmd); g.argmax->dispatch(cmd,{g.w*g.h},1); barrier(cmd);
    uint64_t bits; static_assert(sizeof(bits)==sizeof(g.base)); std::memcpy(&bits,&g.base,sizeof(bits));
    g.frequency->dispatch(cmd,{uint32_t(bits),uint32_t(bits>>32),g.w,g.h},1); barrier(cmd);
}
}
