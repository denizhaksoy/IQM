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
int main(int argc,char** argv){try{
    const bool allChain=argc==10&&std::string(argv[5])=="--all-chain";
    const bool chain=argc==10&&(std::string(argv[5])=="--chain"||allChain);
    const bool bands=(argc==6&&std::string(argv[5])=="--bands")||allChain;
    if(argc!=5&&!chain&&!bands)throw std::runtime_error("Usage:fma.spv separate.spv native-directory fresh-output [--bands|--chain mean.spv difference.spv fft.spv combined-native]");
    const std::filesystem::path native(argv[3]),output(argv[4]);
    if(std::filesystem::exists(output)||!std::filesystem::create_directories(output))throw std::runtime_error("Fresh output required");
    IQM::Bin::VulkanInstance instance(true);const auto& device=*instance.device();
    const auto props=instance.physicalDevice()->getProperties2<vk::PhysicalDeviceProperties2,vk::PhysicalDeviceFloatControlsProperties>();
    const auto& controls=props.get<vk::PhysicalDeviceFloatControlsProperties>();
    if(!controls.shaderDenormPreserveFloat32||!controls.shaderRoundingModeRTEFloat32)throw std::runtime_error("Float32 modes unsupported");
    auto setLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
    auto layout=VulkanRuntime::createPipelineLayout(device,{*setLayout},VulkanRuntime::createPushConstantRange(32));
    std::vector<vk::raii::ShaderModule> modules;std::vector<vk::raii::Pipeline> pipelines;
    for(int i=1;i<=2;++i){modules.push_back(VulkanRuntime::createShaderModule(device,code(argv[i])));pipelines.push_back(VulkanRuntime::createComputePipeline(device,modules.back(),layout));}
    auto residualSetLayout=VulkanRuntime::createDescLayout(device,{{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
    auto residualLayout=VulkanRuntime::createPipelineLayout(device,{*residualSetLayout},VulkanRuntime::createPushConstantRange(16));
    if(chain)for(int i=6;i<=8;++i){modules.push_back(VulkanRuntime::createShaderModule(device,code(argv[i])));pipelines.push_back(VulkanRuntime::createComputePipeline(device,modules.back(),residualLayout));}
    const auto lo0=words(native/"lo0.txt"),lo=words(native/"lo.txt");
    if(lo0.size()!=49||lo.size()!=169)throw std::runtime_error("Unexpected sp0Filters geometry");
    auto filter0=buffer(instance,196),filter=buffer(instance,676);
    auto upload=[](Buffer& b,const std::vector<uint32_t>& v){void* p=b.memory.mapMemory(0,v.size()*4);std::memcpy(p,v.data(),v.size()*4);b.memory.unmapMemory();};
    upload(filter0,lo0);upload(filter,lo);
    auto highFilter=buffer(instance,324),orientFilter=buffer(instance,324);
    if(bands){auto high=words(native/"hi0.txt"),orient=words(native/"orient.txt");if(high.size()!=81||orient.size()!=81)throw std::runtime_error("Bad band filter shape");upload(highFilter,high);upload(orientFilter,orient);}
    std::ifstream manifest(native/"manifest.txt");if(!manifest)throw std::runtime_error("Missing manifest");
    std::ofstream results(output/"results.tsv");results<<"case\tstage\tvalues\tnumeric_errors\tbit_errors\n";
    std::string tag,folder;unsigned h,w,cases=0;size_t total=0;
    while(manifest>>tag>>h>>w>>std::quoted(folder)){
        if(chain&&tag.ends_with("_test"))continue;
        if(!((h==80&&w==96)||(h==96&&w==80)))throw std::runtime_error("Unvalidated shape");
        auto input=words(std::filesystem::path(folder)/"input.txt");if(input.size()!=size_t(h)*w)throw std::runtime_error("Bad input geometry");
        const unsigned batches=chain?2:1;
        const std::string pairTag=chain?tag.substr(0,tag.size()-10):tag;
        const auto testFolder=native/(pairTag+"_test");
        if(chain){auto v=words(testFolder/"input.txt");if(v.size()!=input.size())throw std::runtime_error("Pair shape mismatch");input.insert(input.end(),v.begin(),v.end());}
        std::array<std::vector<uint32_t>,11> expected;
        std::array<unsigned,9> widths{w,w,w/2,w/4,w/8,w,w,w/2,w/4},heights{h,h,h/2,h/4,h/8,h,h,h/2,h/4};
        const unsigned lowStages=bands?8:4;
        std::vector<Buffer> buffers;for(unsigned j=0;j<lowStages+1;++j)buffers.push_back(buffer(instance,batches*widths[j]*heights[j]*4));
        upload(buffers[0],input);
        for(unsigned j=0;j<4;++j){expected[j]=words(std::filesystem::path(folder)/("stage"+std::to_string(j)+".txt"));if(expected[j].size()!=widths[j+1]*heights[j+1])throw std::runtime_error("Bad stage geometry");if(chain){auto v=words(testFolder/("stage"+std::to_string(j)+".txt"));if(v.size()!=expected[j].size())throw std::runtime_error("Pair stage mismatch");expected[j].insert(expected[j].end(),v.begin(),v.end());}}
        if(bands)for(unsigned j=4;j<8;++j){const std::string name=j==4?"high.txt":"orient"+std::to_string(j-5)+".txt";expected[j]=words(std::filesystem::path(folder)/name);if(expected[j].size()!=widths[j+1]*heights[j+1])throw std::runtime_error("Bad band geometry");if(chain){auto v=words(testFolder/name);if(v.size()!=expected[j].size())throw std::runtime_error("Pair band mismatch");expected[j].insert(expected[j].end(),v.begin(),v.end());}}
        if(chain){
            const auto golden=std::filesystem::path(argv[9])/pairTag;
            expected[lowStages]=words(golden/"mean.txt");expected[lowStages+1]=words(golden/"difference_pack_words.txt");expected[lowStages+2]=words(golden/"difference_expected_words.txt");
            if(expected[lowStages].size()!=242||expected[lowStages+1].size()!=240||expected[lowStages+2].size()!=361)throw std::runtime_error("Chain golden coverage mismatch");
            for(unsigned batch=0;batch<2;++batch){int minimum=1000,maximum=-1000;for(unsigned k=0;k<120;++k){float v=std::bit_cast<float>(expected[3][batch*120+k]);if(!std::isfinite(v))throw std::runtime_error("Nonfinite base");if(v!=0){int e=std::ilogb(std::abs(v));minimum=std::min(minimum,e);maximum=std::max(maximum,e);}}if(maximum!=-1000&&(minimum < -90||maximum>100||maximum-minimum>16))throw std::runtime_error("Mean domain exceeded");}
            for(unsigned k=0;k<240;++k){float v=std::bit_cast<float>(expected[lowStages+2][k]);if(!std::isfinite(v)||(v!=0&&(std::ilogb(std::abs(v)) < -90||std::ilogb(std::abs(v))>100)))throw std::runtime_error("Norm domain exceeded");}
            for(unsigned j=lowStages;j<lowStages+3;++j)buffers.push_back(buffer(instance,unsigned(expected[j].size()*4)));
        }
        const unsigned stages=lowStages+(chain?3:0);
        auto pool=VulkanRuntime::createDescPool(device,stages,{{vk::DescriptorType::eStorageBuffer,lowStages*3+(chain?6u:0u)}});
        std::vector<vk::DescriptorSetLayout> raw(lowStages,*setLayout);if(chain)for(unsigned j=0;j<3;++j)raw.push_back(*residualSetLayout);
        auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{.descriptorPool=*pool,.descriptorSetCount=stages,.pSetLayouts=raw.data()});
        const unsigned inputs[]={0,1,2,3,0,1,2,3};
        for(unsigned j=0;j<lowStages;++j){const unsigned in=inputs[j];const std::vector<vk::DescriptorBufferInfo> a{{*buffers[in].handle,0,batches*widths[in]*heights[in]*4}},b{{*buffers[j+1].handle,0,batches*widths[j+1]*heights[j+1]*4}},c{{j==0?*filter0.handle:j<4?*filter.handle:j==4?*highFilter.handle:*orientFilter.handle,0,j==0?196u:j<4?676u:324u}};
            device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[j],0,a),VulkanRuntime::createWriteSet(*sets[j],1,b),VulkanRuntime::createWriteSet(*sets[j],2,c)},{});}
        if(chain)for(unsigned j=lowStages;j<stages;++j){const unsigned in=j==lowStages?4:j;const auto bytes=j==lowStages?expected[3].size()*4:expected[j-1].size()*4;const std::vector<vk::DescriptorBufferInfo> a{{*buffers[in].handle,0,bytes}},b{{*buffers[j+1].handle,0,expected[j].size()*4}};device.updateDescriptorSets({VulkanRuntime::createWriteSet(*sets[j],0,a),VulkanRuntime::createWriteSet(*sets[j],1,b)},{});}
        instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        vk::MemoryBarrier before{.srcAccessMask=vk::AccessFlagBits::eHostWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead};
        instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eHost,vk::PipelineStageFlagBits::eComputeShader,{}, {before},{},{});
        struct Push{uint32_t w,h,ow,oh,n,step,normalize,reserved;};
        for(unsigned j=0;j<lowStages;++j){
            const unsigned in=inputs[j];
            instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*layout,0,{*sets[j]},{});
            instance.cmdBuf()->pushConstants<Push>(*layout,vk::ShaderStageFlagBits::eCompute,0,Push{widths[in],heights[in],widths[j+1],heights[j+1],j==0?7u:j<4?13u:9u,j==0||j>=4?1u:2u,j==3?1u:j>=5?j-3:0u,batches});
            for(unsigned variant=0;variant<2;++variant){instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipelines[variant]);instance.cmdBuf()->dispatch((batches*widths[j+1]*heights[j+1]+63)/64,1,1);}
            vk::MemoryBarrier barrier{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eHostRead};
            instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eHost,{}, {barrier},{},{});
        }
        if(chain)for(unsigned j=lowStages;j<stages;++j){
            struct ResidualPush{uint32_t groups,h,w,reserved;};
            instance.cmdBuf()->bindPipeline(vk::PipelineBindPoint::eCompute,*pipelines[j-lowStages+2]);
            instance.cmdBuf()->bindDescriptorSets(vk::PipelineBindPoint::eCompute,*residualLayout,0,{*sets[j]},{});
            instance.cmdBuf()->pushConstants<ResidualPush>(*residualLayout,vk::ShaderStageFlagBits::eCompute,0,ResidualPush{j==lowStages?2u:1u,h/8,w/8,0});
            instance.cmdBuf()->dispatch(j==lowStages+1?2:1,1,1);
            vk::MemoryBarrier barrier{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eHostRead};
            instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eHost,{}, {barrier},{},{});
        }
        instance.cmdBuf()->end();vk::CommandBuffer cmd=*instance.cmdBuf();vk::raii::Fence fence(device,vk::FenceCreateInfo{});
        instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);instance.waitForFence(fence);
        size_t bad=0;
        for(unsigned j=0;j<stages;++j){const auto* actual=static_cast<const uint32_t*>(buffers[j+1].memory.mapMemory(0,expected[j].size()*4));size_t numeric=0,bits=0,magBits=0;
            for(size_t k=0;k<expected[j].size();++k){numeric+=std::bit_cast<float>(actual[k])!=std::bit_cast<float>(expected[j][k]);bits+=actual[k]!=expected[j][k];if(chain&&j==lowStages+2&&k>=240)magBits+=actual[k]!=expected[j][k];}
            std::ofstream dump(output/(tag+"_stage"+std::to_string(j)+".u32"),std::ios::binary);dump.write(reinterpret_cast<const char*>(actual),expected[j].size()*4);buffers[j+1].memory.unmapMemory();
            results<<tag<<'\t'<<j<<'\t'<<expected[j].size()<<'\t'<<numeric<<'\t'<<bits<<'\n';results.flush();bad+=numeric+magBits;if(chain&&(j==lowStages||j==lowStages+1))bad+=bits;total+=expected[j].size();
        }
        instance.cmdBuf()->reset();if(bad)throw std::runtime_error("SPYR lowpass numeric mismatch:"+tag);++cases;
    }
    if(!manifest.eof()||cases!=(chain?2u:4u)||total!=(allChain?113526u:chain?42486u:bands?111840u:40800u))throw std::runtime_error("Incomplete coverage");
    std::ofstream(output/"PASS.txt")<<"Actual pathway GPU chains;values="<<total<<";residual_chain="<<chain<<";all_bands="<<bands<<". No host intermediate computation.\n";
    std::cout<<"SPYR lowpass cases="<<cases<<" values="<<total<<" mismatches=0\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
