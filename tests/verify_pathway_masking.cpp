#include <vulkan_instance.h>
#include <IQM/hdrvdp3/map_gpu.h>
#include <IQM/base/vulkan_runtime.h>
#include <vulkan_res.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <bit>
#include <stdexcept>
namespace fs=std::filesystem;
struct Image{unsigned h,w;std::vector<double> data;};
static Image read(const fs::path& p){std::ifstream f(p);Image r;if(!(f>>r.h>>r.w)||!r.h||!r.w||size_t(r.h)*r.w>1000000)throw std::runtime_error("Bad fixture:"+p.string());r.data.resize(r.h*r.w);for(auto& v:r.data)if(!(f>>v)||!std::isfinite(v))throw std::runtime_error("Bad value");std::string extra;if(f>>extra)throw std::runtime_error("Trailing fixture");return r;}
static std::vector<float> single(const Image& a){return {a.data.begin(),a.data.end()};}
static std::vector<float> runFloatKernel(const IQM::Bin::VulkanInstance& instance,const std::string& shader,const std::vector<float>& input,size_t outputCount,uint32_t groups){
    using IQM::GPU::VulkanRuntime;const auto& device=*instance.device();
    std::ifstream file(shader,std::ios::binary|std::ios::ate);const auto length=file.tellg();if(length<=0||length%4)throw std::runtime_error("Bad product shader");std::vector<uint32_t> code(size_t(length)/4);file.seekg(0);file.read(reinterpret_cast<char*>(code.data()),length);if(!file||code[0]!=0x07230203u)throw std::runtime_error("Bad SPIR-V");
    auto module=VulkanRuntime::createShaderModule(device,code);auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(16));auto pipeline=VulkanRuntime::createComputePipeline(device,module,layout);
    const unsigned bytes=unsigned(outputCount*4),inputBytes=unsigned(input.size()*4);auto src=IQM::Bin::VulkanResource::createBuffer(device,*instance.physicalDevice(),inputBytes,vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);auto dst=IQM::Bin::VulkanResource::createBuffer(device,*instance.physicalDevice(),bytes,vk::BufferUsageFlagBits::eStorageBuffer,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);src.first.bindMemory(src.second,0);dst.first.bindMemory(dst.second,0);
    void* mapped=src.second.mapMemory(0,inputBytes);std::memcpy(mapped,input.data(),inputBytes);src.second.unmapMemory();
    auto pool=VulkanRuntime::createDescPool(device,1,{{vk::DescriptorType::eStorageBuffer,2}});const vk::DescriptorSetLayout raw=*setLayout;auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=1,.pSetLayouts=&raw});const std::vector<vk::DescriptorBufferInfo> a{{*src.first,0,inputBytes}},b{{*dst.first,0,bytes}};device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[0],0,a),VulkanRuntime::createWriteSet(*sets[0],1,b)},{});
    instance.cmdBuf()->reset();instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
    instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline);instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[0]},{});struct Push{uint32_t count,a,b,c;};instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,Push{groups,0,0,0});instance.cmdBuf()->dispatch((groups+63)/64,1,1);vk::MemoryBarrier after{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eHost,{}, {after},{},{});instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);
    std::vector<float> output(outputCount);mapped=dst.second.mapMemory(0,bytes);std::memcpy(output.data(),mapped,bytes);dst.second.unmapMemory();instance.cmdBuf()->reset();return output;
}
static std::vector<float> maskedProduct(const IQM::Bin::VulkanInstance& instance,const std::string& shader,const std::vector<float>& D,const Image& mask){
    auto input=D;const auto m=single(mask);if(m.size()!=D.size())throw std::runtime_error("Mask shape mismatch");input.insert(input.end(),m.begin(),m.end());return runFloatKernel(instance,shader,input,D.size(),uint32_t(D.size()));
}
int main(int argc,char** argv){try{
    if(argc==4&&std::string(argv[1])=="--frequency-grid"){
        const fs::path output(argv[3]);if(fs::exists(output)||!fs::create_directories(output))throw std::runtime_error("Fresh frequency output required");std::ifstream f(argv[2]);if(!f)throw std::runtime_error("Missing grid fixture");unsigned h,w,count=0;double ppd;size_t bad=0;
        while(f>>h>>w>>ppd){if(!((h==10&&w==12)||(h==12&&w==10))||ppd!=3.75)throw std::runtime_error("Unvalidated frequency grid");for(unsigned i=0;i<h*w;++i){double expected;if(!(f>>expected))throw std::runtime_error("Truncated grid");bad+=std::bit_cast<uint64_t>(IQM::HDRVDP3::dominant_frequency_from_index(i,w,h,ppd))!=std::bit_cast<uint64_t>(expected);++count;}}
        if(!f.eof()||count!=240||bad)throw std::runtime_error("Frequency grid mismatch/count: "+std::to_string(bad));std::ofstream(output/"PASS.txt")<<"240 native double frequency grid values bitwise exact.\n";std::cout<<"frequency_grid values=240 bit_mismatches=0\n";return 0;
    }
    if(argc<4||argc>11||argc==7||argc==10)throw std::runtime_error("Usage:ABI13-map.spv native-directory fresh-output [product.spv [GPU-comparison-directory [GPU-bands GPU-pool]]]");
    const fs::path native(argv[2]),out(argv[3]);if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
    IQM::Bin::VulkanInstance instance(true);IQM::HDRVDP3::MapGPU gpu(instance,argv[1]);
    std::ofstream results(out/"results.tsv");results<<"case\tstage\tvalues\tnumeric_errors\tbit_errors\n";size_t failures=0,values=0,checks=0;
    auto compare=[&](const std::string& tag,const std::string& stage,const std::vector<float>& actual,const std::vector<float>& expected){if(actual.size()!=expected.size())throw std::runtime_error("Shape mismatch");size_t bad=0,bits=0;for(size_t i=0;i<actual.size();++i){bad+=!std::isfinite(actual[i])||actual[i]!=expected[i];bits+=std::bit_cast<uint32_t>(actual[i])!=std::bit_cast<uint32_t>(expected[i]);}std::ofstream dump(out/(tag+"_"+stage+".f32"),std::ios::binary);dump.write(reinterpret_cast<const char*>(actual.data()),actual.size()*4);results<<tag<<'\t'<<stage<<'\t'<<actual.size()<<'\t'<<bad<<'\t'<<bits<<'\n';results.flush();failures+=bad;values+=actual.size();++checks;};
    for(const std::string orientation:{"h80_w96","h96_w80"}){
        const auto folder=native/orientation;std::vector<float> errors;
        Image logAdaptation;if(argc>=9)logAdaptation=read(fs::path(argv[8])/orientation/"log_La.txt");
        std::array<Image,5> gpuSelf,gpuReference,gpuTest;
        if(argc>=8)for(unsigned b=1;b<=5;++b){const std::string prefix="band"+std::to_string(b),tag=orientation+"_"+prefix;
            auto load=[&](const fs::path& path,const Image& golden,size_t offset,size_t words){std::ifstream f(path,std::ios::binary|std::ios::ate);if(f.tellg()!=std::streamoff(words*4))throw std::runtime_error("Bad GPU input size:"+path.string());f.seekg(offset*4);std::vector<float> actual(golden.data.size());f.read(reinterpret_cast<char*>(actual.data()),actual.size()*4);if(!f)throw std::runtime_error("Truncated GPU input");return actual;};
            const auto nativeSelf=read(folder/(prefix+"_self.txt")),r=read(folder/(prefix+"_reference.txt")),t=read(folder/(prefix+"_test.txt"));const size_t n=nativeSelf.data.size();
            const unsigned caseIndex=(orientation=="h80_w96"?0:5)+b-1;
            auto pooled=load(fs::path(argv[7])/("case"+std::to_string(caseIndex)+".u32"),nativeSelf,n,2*n);
            auto reference=load(fs::path(argv[6])/orientation/(prefix+"_reference.f32"),r,0,n),test=load(fs::path(argv[6])/orientation/(prefix+"_test.f32"),t,0,n);
            compare(tag,"GPU_self_input",pooled,single(nativeSelf));compare(tag,"GPU_reference_input",reference,single(r));compare(tag,"GPU_test_input",test,single(t));
            gpuSelf[b-1]={nativeSelf.h,nativeSelf.w,{pooled.begin(),pooled.end()}};gpuReference[b-1]={r.h,r.w,{reference.begin(),reference.end()}};gpuTest[b-1]={t.h,t.w,{test.begin(),test.end()}};
        }
        for(unsigned b=1;b<=5;++b){const std::string prefix="band"+std::to_string(b),tag=orientation+"_"+prefix;
            auto image=[&](const std::string& field){return read(folder/(prefix+"_"+field+".txt"));};
            auto scalar=[&](const std::string& field){const auto a=image(field);if(a.data.size()!=1)throw std::runtime_error("Expected scalar");return a.data[0];};
            const auto self=argc>=8?gpuSelf[b-1]:image("self");std::vector<float> previous(self.data.size(),0),next(previous);
            if(b>1){const auto a=argc>=8?gpuSelf[b-2]:read(folder/("band"+std::to_string(b-1)+"_self.txt"));previous=gpu.resize(single(a),a.h,a.w,self.h,self.w);}
            if(b<4){const auto a=argc>=8?gpuSelf[b]:read(folder/("band"+std::to_string(b+1)+"_self.txt"));next=gpu.resize(single(a),a.h,a.w,self.h,self.w);}
            const auto neighbor=gpu.neighbor_mask(previous,next);compare(tag,"neighbor",neighbor,single(image("neighbor")));
            const auto denominator=gpu.masking_denominator(single(self),neighbor,float(scalar("q")),float(scalar("self_scale")),float(scalar("neighbor_scale")));compare(tag,"denominator",denominator,single(image("denominator")));
            const auto reference=argc>=8?gpuReference[b-1]:image("reference"),test=argc>=8?gpuTest[b-1]:image("test");const auto difference=gpu.normalized_difference(single(reference),single(test),1);
            const auto nr=image("reference"),nt=image("test");std::vector<float> nativeDifference(nr.data.size());for(size_t i=0;i<nativeDifference.size();++i)nativeDifference[i]=float(nt.data[i])-float(nr.data[i]);compare(tag,"band_difference",difference,nativeDifference);
            const double p=scalar("p");float neural=-1;if(b==5&&argc!=11)neural=float(std::pow(scalar("neural"),2*p));
            auto sensitivity=single(image("csf"));
            std::vector<float> resized;
            if(argc>=9){resized=gpu.clamp_log(gpu.resize(single(logAdaptation),logAdaptation.h,logAdaptation.w,self.h,self.w));sensitivity=gpu.sample_csf(resized,IQM::HDRVDP3::create_csf_lut(30.0/std::pow(2.0,b),{24,true}));compare(tag,"GPU_CSF_input",sensitivity,single(image("csf")));}
            if(argc==11&&b==5){
                if(resized.size()!=120)throw std::runtime_error("Base120 shape mismatch");int minimum=1000,maximum=-1000;double sum=0;
                for(float v:resized){if(!std::isfinite(v)||v < -5||v>5)throw std::runtime_error("Base log domain exceeded");sum+=double(v);if(v!=0){const int e=std::ilogb(std::abs(v));minimum=std::min(minimum,e);maximum=std::max(maximum,e);}}
                if(maximum!=-1000&&(minimum < -90||maximum>100||maximum-minimum>16))throw std::runtime_error("Mean exponent domain exceeded");
                const auto baseGolden=read(fs::path(argv[8])/orientation/"base_selection.txt");if(baseGolden.data.size()!=4)throw std::runtime_error("Missing base golden");
                const auto adaptation=runFloatKernel(instance,argv[9],resized,2,1);compare(tag,"base_adaptation",adaptation,{float(sum/120.0),float(baseGolden.data[2])});
                const auto path=fs::path(argv[10])/(orientation+"_reference_stage10.u32");std::ifstream spectrum(path,std::ios::binary|std::ios::ate);if(spectrum.tellg()!=1444)throw std::runtime_error("Bad GPU selection export");spectrum.seekg(1440);uint32_t bits;if(!spectrum.read(reinterpret_cast<char*>(&bits),4))throw std::runtime_error("Missing GPU selection");const float index=std::bit_cast<float>(bits);if(!std::isfinite(index)||index<0||index>=120||index!=std::trunc(index))throw std::runtime_error("Invalid GPU selected index");
                const double frequency=IQM::HDRVDP3::dominant_frequency_from_index(unsigned(index),self.w,self.h,3.75);
                const auto parameters=gpu.base_csf_parameters({adaptation[1]})[0];const std::array<float,4> csfParameters{parameters[1],parameters[2],parameters[3],parameters[4]};
                const double noise=1.0/IQM::HDRVDP3::evaluate_ncsf_parameters(frequency,csfParameters,{24,true});
                const std::array<double,3> actual{double(index),frequency,noise},expected{baseGolden.data[0]-1,baseGolden.data[1],baseGolden.data[3]};size_t numeric=0,bitErrors=0;for(unsigned i=0;i<3;++i){numeric+=actual[i]!=expected[i];bitErrors+=std::bit_cast<uint64_t>(actual[i])!=std::bit_cast<uint64_t>(expected[i]);}
                std::ofstream dump(out/(tag+"_base_noise.f64"),std::ios::binary);dump.write(reinterpret_cast<const char*>(actual.data()),24);results<<tag<<"\tbase_frequency_noise\t3\t"<<numeric<<'\t'<<bitErrors<<'\n';results.flush();failures+=numeric;values+=3;++checks;
                neural=float(std::pow(noise,2*p));
            }
            const auto D=gpu.difference(difference,sensitivity,denominator,float(p),neural);compare(tag,"D",D,single(image("difference")));
            compare(tag,"psychometric",gpu.psychometric(D,float(scalar("pf"))),single(image("psychometric")));
            auto mask=image("comparison");
            if(argc>=6){const auto path=fs::path(argv[5])/(orientation+"_p0_b"+std::to_string(b)+".u64");std::ifstream f(path,std::ios::binary|std::ios::ate);if(f.tellg()!=std::streamoff(mask.data.size()*8))throw std::runtime_error("Bad GPU mask size");f.seekg(0);std::vector<uint64_t> actual(mask.data.size());f.read(reinterpret_cast<char*>(actual.data()),actual.size()*8);if(!f)throw std::runtime_error("Truncated GPU mask");size_t bad=0;for(size_t i=0;i<actual.size();++i){bad+=actual[i]!=std::bit_cast<uint64_t>(mask.data[i]);mask.data[i]=std::bit_cast<double>(actual[i]);}results<<tag<<"\tcomparison_double\t"<<actual.size()<<'\t'<<bad<<'\t'<<bad<<'\n';results.flush();failures+=bad;values+=actual.size();++checks;}
            if(argc>=5)compare(tag,"D_p",maskedProduct(instance,argv[4],D,mask),single(image("quality_difference")));
            const float error=gpu.quality_band_error(D,mask.data,self.w,self.h);errors.push_back(error);const auto expectedErrors=single(read(folder/"errors.txt"));compare(tag,"quality_error",{error},{expectedErrors.at(b-1)});
        }
        const auto scores=gpu.quality_scores(errors);const auto expected=single(read(folder/"scores.txt"));compare(orientation,"quality_scores",{scores.Q,scores.Q_JOD},expected);
    }
    if(checks!=(argc==11?126u:argc==9?122u:argc==8?112u:argc==6?82u:argc==5?72u:62u))throw std::runtime_error("Incomplete stage coverage");
    std::cout<<"masking checks="<<checks<<" values="<<values<<" numeric_mismatches="<<failures<<'\n';if(failures)return 1;
    std::ofstream(out/"PASS.txt")<<"Native-input masking/neighbor/D/psychometric/quality component stages exact; no complete GPU metric claim.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
