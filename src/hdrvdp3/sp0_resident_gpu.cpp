#include <IQM/hdrvdp3/sp0_resident_gpu.h>
#include <IQM/base/vulkan_runtime.h>
#include <vulkan_res.h>
#include <algorithm>
#include <cmath>
#include <cstring>
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

struct ResidentSP0Pyramid::Impl {
    vk::raii::Device const* dev=nullptr;
    SP0ResidentBuffers bufs;
    unsigned w=0,h=0; double ppd=0; unsigned levels=0;
    vk::raii::DescriptorSetLayout dsl=nullptr;
    vk::raii::PipelineLayout pl=nullptr;
    vk::raii::ShaderModule corrMod=nullptr;
    vk::raii::ShaderModule meanMod=nullptr;
    vk::raii::Pipeline corrPipe=nullptr;
    vk::raii::Pipeline meanPipe=nullptr;
    vk::raii::DescriptorPool pool=nullptr;
    std::vector<vk::raii::DescriptorSet> sets;
    SP0PyramidGpuResult res;
    uint32_t dispatches=0;

    struct CorrJob { FFTBufferRegion in; FFTBufferRegion out; FFTBufferRegion filter; uint32_t inW,inH,outW,outH,kernel,step,normExp,transposed; vk::DescriptorSet set; };
    std::vector<CorrJob> corrJobs;
    struct MeanJob { FFTBufferRegion base; FFTBufferRegion mean; uint32_t width,height; vk::DescriptorSet set; } meanJob{};

    Impl(const vk::raii::Device& d,const vk::raii::PhysicalDevice&,const SP0ResidentShaders& shaders,const SP0ResidentBuffers& buffers,unsigned width,unsigned height,double ppd_)
    :dev(&d),bufs(buffers),w(width),h(height),ppd(ppd_){
        if(w<16||h<16||ppd<26||ppd>256) throw std::invalid_argument("SP0 resident geometry");
        unsigned maxLevels=0; for(unsigned a=w,b=h;a>=13&&b>=13;a/=2,b/=2) ++maxLevels;
        levels=std::min(unsigned(std::ceil(std::log2(ppd))-2),maxLevels);
        size_t expectedBands=size_t(levels)+2;
        if(bufs.bandRegions.size()!=expectedBands) throw std::invalid_argument("SP0 band region count");
        using R=IQM::GPU::VulkanRuntime;
        dsl=R::createDescLayout(d,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
        pl=R::createPipelineLayout(d,{*dsl},R::createPushConstantRange(32));
        corrMod=R::createShaderModule(d,loadSpv(shaders.corrdn));
        meanMod=R::createShaderModule(d,loadSpv(shaders.baseMean));
        corrPipe=R::createComputePipeline(d,corrMod,pl);
        meanPipe=R::createComputePipeline(d,meanMod,pl);

        // Plan bands
        res.levels=levels;
        res.baseMean=bufs.baseMean;
        res.bands.resize(expectedBands);
        unsigned bw=w,bh=h;
        for(size_t b=0;b<expectedBands;++b){ res.bands[b]={bw,bh,bufs.bandRegions[b]}; if(b>=1){bw=(bw+1)/2; bh=(bh+1)/2;} }

        // Build corr jobs in order: high, lo0->workA, then per level orient, lo(decimated)->workB/A, final base already in work
        auto addCorr=[&](FFTBufferRegion in,FFTBufferRegion out,FFTBufferRegion filter,uint32_t inW,uint32_t inH,uint32_t outW,uint32_t outH,uint32_t k,uint32_t step,uint32_t normExp){
            CorrJob j; j.in=in; j.out=out; j.filter=filter; j.inW=inW; j.inH=inH; j.outW=outW; j.outH=outH; j.kernel=k; j.step=step; j.normExp=normExp; j.transposed=(inW>inH)?1u:0u; corrJobs.push_back(j);
        };
        // high pass: input -> band0
        addCorr(bufs.input,res.bands[0].region,bufs.hi0Filter,w,h,w,h,9,1,0);
        // lo0: input -> workA (full res low)
        FFTBufferRegion lowCur=bufs.workA; // holds current low
        addCorr(bufs.input,lowCur,bufs.lo0Filter,w,h,w,h,7,1,0);
        unsigned curW=w,curH=h;
        FFTBufferRegion lowScratch=bufs.workB;
        for(unsigned i=0;i<levels;++i){
            // orientation band from lowCur -> band[i+1]
            addCorr(lowCur,res.bands[i+1].region,bufs.orientFilter,curW,curH,curW,curH,9,1,i+1);
            // decimated low: lowCur -> lowScratch
            unsigned nw=(curW+1)/2, nh=(curH+1)/2;
            addCorr(lowCur,lowScratch,bufs.loFilter,curW,curH,nw,nh,13,2,0);
            std::swap(lowCur,lowScratch);
            curW=nw; curH=nh;
        }
        // final lowCur is base band pre-norm; copy to base band region with norm levels+1? base band index = levels+1
        // sp0_single: bands.push_back(std::move(low)); then normalize all by 2^b. so base gets 2^(levels+1)
        // our correlate applies norm; so do a "copy with norm" by using kernel 1 identity? easier: correlate with 1x1 identity filter is overkill. Instead add a copy/norm step. Simpler: make a small shader or just do correlate as identity. 
        // Better: include norm in last low correlate: yes, last correlate should have normExp=levels+1.
        // Fix: adjust last lo correlate normExp
        // find last corr job with lo filter to cur size - easier to build carefully.
        // Rebuild properly below.
        corrJobs.clear();
        addCorr(bufs.input,res.bands[0].region,bufs.hi0Filter,w,h,w,h,9,1,0);
        lowCur=bufs.workA; lowScratch=bufs.workB; curW=w; curH=h;
        addCorr(bufs.input,lowCur,bufs.lo0Filter,w,h,w,h,7,1,0);
        for(unsigned i=0;i<levels;++i){
            addCorr(lowCur,res.bands[i+1].region,bufs.orientFilter,curW,curH,curW,curH,9,1,i+1);
            unsigned nw=(curW+1)/2, nh=(curH+1)/2;
            bool last=(i+1==levels);
            FFTBufferRegion dstLow = last ? res.bands.back().region : lowScratch;
            addCorr(lowCur,dstLow,bufs.loFilter,curW,curH,nw,nh,13,2,last ? (levels+1) : 0);
            if(!last) std::swap(lowCur,lowScratch);
            curW=nw; curH=nh;
        }
        // Base mean job
        meanJob.base=res.bands.back().region; meanJob.mean=bufs.baseMean; meanJob.width=curW; meanJob.height=curH;

        uint32_t totalSets=uint32_t(corrJobs.size()+1);
        pool=R::createDescPool(d,totalSets,{{vk::DescriptorType::eStorageBuffer,totalSets*3}});
        std::vector<vk::DescriptorSetLayout> layouts(totalSets,*dsl);
        auto alloc=vk::raii::DescriptorSets(d,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=totalSets,.pSetLayouts=layouts.data()});
        for(auto& s:alloc) sets.push_back(std::move(s));
        auto bind3=[&](vk::DescriptorSet set,FFTBufferRegion a,FFTBufferRegion b,FFTBufferRegion c){
            vk::DescriptorBufferInfo ia{a.buffer,a.offset,a.bytes}, ib{b.buffer,b.offset,b.bytes}, ic{c.buffer,c.offset,c.bytes};
            d.updateDescriptorSets({vk::WriteDescriptorSet{.dstSet=set,.dstBinding=0,.descriptorCount=1,.descriptorType=vk::DescriptorType::eStorageBuffer,.pBufferInfo=&ia},vk::WriteDescriptorSet{.dstSet=set,.dstBinding=1,.descriptorCount=1,.descriptorType=vk::DescriptorType::eStorageBuffer,.pBufferInfo=&ib},vk::WriteDescriptorSet{.dstSet=set,.dstBinding=2,.descriptorCount=1,.descriptorType=vk::DescriptorType::eStorageBuffer,.pBufferInfo=&ic}},{});
        };
        for(size_t i=0;i<corrJobs.size();++i){ corrJobs[i].set=*sets[i]; bind3(corrJobs[i].set,corrJobs[i].in,corrJobs[i].out,corrJobs[i].filter); dispatches += 1; }
        meanJob.set=*sets[corrJobs.size()]; bind3(meanJob.set,meanJob.base,meanJob.mean,meanJob.mean); dispatches += 1;
    }

    void record(vk::CommandBuffer cmd)const{
        for(const auto& j:corrJobs){
            cmd.bindPipeline(vk::PipelineBindPoint::eCompute,*corrPipe);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute,*pl,0,{j.set},{});
            struct Push { uint32_t inW,inH,outW,outH,kernel,step,normExp,transposed; } p{j.inW,j.inH,j.outW,j.outH,j.kernel,j.step,j.normExp,j.transposed};
            cmd.pushConstants<Push>(*pl,vk::ShaderStageFlagBits::eCompute,0,p);
            uint32_t gx=(j.outW+7)/8, gy=(j.outH+7)/8;
            cmd.dispatch(gx,gy,1);
            barrier(cmd);
        }
        cmd.bindPipeline(vk::PipelineBindPoint::eCompute,*meanPipe);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute,*pl,0,{meanJob.set},{});
        struct MPush { uint32_t width,height; } mp{meanJob.width,meanJob.height};
        cmd.pushConstants<MPush>(*pl,vk::ShaderStageFlagBits::eCompute,0,mp);
        cmd.dispatch(1,1,1);
        barrier(cmd);
    }
};

ResidentSP0Pyramid::ResidentSP0Pyramid(const vk::raii::Device& d,const vk::raii::PhysicalDevice& p,const SP0ResidentShaders& s,const SP0ResidentBuffers& b,unsigned w,unsigned h,double ppd)
:impl_(std::make_unique<Impl>(d,p,s,b,w,h,ppd)){}
ResidentSP0Pyramid::~ResidentSP0Pyramid()=default;
ResidentSP0Pyramid::ResidentSP0Pyramid(ResidentSP0Pyramid&&)noexcept=default;
ResidentSP0Pyramid& ResidentSP0Pyramid::operator=(ResidentSP0Pyramid&&)noexcept=default;
void ResidentSP0Pyramid::record(vk::CommandBuffer cmd)const{ if(!impl_) throw std::runtime_error("null"); impl_->record(cmd); }
uint32_t ResidentSP0Pyramid::dispatchCount()const noexcept{ return impl_?impl_->dispatches:0; }
const SP0PyramidGpuResult& ResidentSP0Pyramid::result()const{ if(!impl_) throw std::runtime_error("null"); return impl_->res; }
}
