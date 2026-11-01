#include <vulkan_instance.h>
#include "diagnostic_float_kernel.h"
#include <IQM/hdrvdp3/quality.h>
#include <iomanip>
#include <iostream>
namespace fs=std::filesystem;
static std::vector<uint32_t> words(const fs::path& path){
    std::ifstream f(path);unsigned h,w;if(!(f>>h>>w)||h!=1||w>4096)throw std::runtime_error("Bad words");
    std::vector<uint32_t> v(w);for(auto& x:v)if(!(f>>x))throw std::runtime_error("Missing word");return v;
}
int main(int argc,char** argv){try{
    #if defined(IQM_BASE256_DIAGNOSTIC)
    if(argc!=6&&argc!=7)throw std::runtime_error("Usage:base.spv map.spv limits-manifest pipeline-results fresh-output [5|6]");
    const unsigned expectedCases=argc==7?unsigned(std::stoul(argv[6])):5u;
    if(expectedCases!=5&&expectedCases!=6)throw std::runtime_error("Unvalidated case count");
#else
    if(argc!=6)throw std::runtime_error("Usage:base.spv map.spv limits-manifest pipeline-results fresh-output");
#endif
    const fs::path out(argv[5]);if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
    IQM::Bin::VulkanInstance instance(true);IQM::HDRVDP3::MapGPU gpu(instance,argv[2]);
    std::ifstream manifest(argv[3]);if(!manifest)throw std::runtime_error("Missing manifest");
    std::ofstream results(out/"results.tsv");results<<"case\tindex\tfrequency\tadaptation\tnoise\tbit_errors\n";
    std::string tag,logpath,adaptpath,limitpath;unsigned h,w,cases=0,zeroLimits=0;
    while(manifest>>tag>>h>>w>>std::quoted(logpath)>>std::quoted(adaptpath)>>std::quoted(limitpath)){
        #if defined(IQM_BASE256_DIAGNOSTIC)
        if(h!=16||w!=16)
#else
        if(!((h==10||h==12)&&(w==10||w==12)))
#endif
        throw std::runtime_error("Unvalidated limit shape");
        const unsigned n=h*w;auto logwords=words(logpath),expected=words(adaptpath);
        if(logwords.size()!=n||expected.size()!=2)throw std::runtime_error("Limit coverage");
        std::vector<float> log;int mn=1000,mx=-1000;
        for(auto word:logwords){float v=std::bit_cast<float>(word);if(!std::isfinite(v)||v < -5||v>5)throw std::runtime_error("Log domain");log.push_back(v);if(v!=0){const int e=std::ilogb(std::abs(v));mn=std::min(mn,e);mx=std::max(mx,e);}}
        if(mx!=-1000&&(mn < -90||mx>100||mx-mn>16))throw std::runtime_error("Mean domain");
        const auto adaptation=runFloatKernel(instance,argv[1],log,2,1,n);
        for(unsigned i=0;i<2;++i)if(std::bit_cast<uint32_t>(adaptation[i])!=expected[i])throw std::runtime_error("Adaptation bits:"+tag);
        std::ifstream selectedFile(fs::path(argv[4])/(tag+"_stage3.u32"),std::ios::binary|std::ios::ate);
        if(selectedFile.tellg()!=std::streamoff((3*n+1)*4))throw std::runtime_error("Missing actual GPU residual");
        selectedFile.seekg(3*n*4);float selected;if(!selectedFile.read(reinterpret_cast<char*>(&selected),4)||!std::isfinite(selected)||selected<0||selected>=n||selected!=std::trunc(selected))throw std::runtime_error("Bad GPU index");
        const double frequency=IQM::HDRVDP3::dominant_frequency_from_index(unsigned(selected),w,h,3.75);
        const auto parameters=gpu.base_csf_parameters({adaptation[1]})[0];
        const std::array<float,4> csf{parameters[1],parameters[2],parameters[3],parameters[4]};
        const double noise=1.0/IQM::HDRVDP3::evaluate_ncsf_parameters(frequency,csf,{24,true});
        std::array<double,2> limits;std::ifstream lf(limitpath,std::ios::binary);
        if(!lf.read(reinterpret_cast<char*>(limits.data()),16))throw std::runtime_error("Missing native limits");
        if(std::bit_cast<uint64_t>(frequency)!=std::bit_cast<uint64_t>(limits[0])||std::bit_cast<uint64_t>(noise)!=std::bit_cast<uint64_t>(limits[1]))throw std::runtime_error("Frequency/noise bits:"+tag);
        if(!std::isfinite(noise)){if(!(frequency==0&&std::isinf(noise)&&noise>0))throw std::runtime_error("Invalid noise limit");++zeroLimits;}
        results<<std::setprecision(17)<<tag<<'\t'<<selected<<'\t'<<frequency<<'\t'<<adaptation[1]<<'\t'<<noise<<"\t0\n";results.flush();
        std::ofstream dump(out/(tag+"_adaptation.f32"),std::ios::binary);dump.write(reinterpret_cast<const char*>(adaptation.data()),8);++cases;
    }
    if(!manifest.eof()||cases!=
#if defined(IQM_BASE256_DIAGNOSTIC)
        expectedCases||zeroLimits!=1
#else
        20||zeroLimits!=4
#endif
        )throw std::runtime_error("Incomplete limit cohort");
    std::ofstream(out/"PASS.txt")<<cases<<" actual GPU-selected base bins; general GPU logmean/power10 and Float64 frequency/NCSF noise match native bits; "<<zeroLimits<<" valid +Inf identity limits. CPU frequency/NCSF remain.\n";
    std::cout<<"General base limits:"<<cases<<" cases bitwise PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
