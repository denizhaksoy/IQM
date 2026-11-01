#include <IQM/hdrvdp3/pathway_spyr.h>
#include <IQM/hdrvdp3/sp0_model.h>
#include <vulkan_instance.h>
#include <bit>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
namespace fs=std::filesystem;
namespace IQM::HDRVDP3 {
PathwaySPYRResult run_square_spyr_diagnostic(const IQM::VulkanInstance&,const std::vector<float>&,const std::vector<float>&,unsigned,unsigned,const PathwaySPYRFilters&,const PathwaySPYRAssets&);
}
template<class T>static std::vector<T> matrix(const fs::path& p){
    std::ifstream f(p);unsigned h,w;if(!(f>>h>>w)||!h||!w||size_t(h)*w>1000000)throw std::runtime_error("Invalid fixture");
    std::vector<T> v(size_t(h)*w);for(auto& x:v)if(!(f>>x))throw std::runtime_error("Missing value");return v;
}
int main(int argc,char** argv){try{
    #if defined(IQM_BASE256_DIAGNOSTIC)
    if(argc!=4&&argc!=5)throw std::runtime_error("Usage:manifest assets fresh-output [5|6]");
#else
    if(argc!=4)throw std::runtime_error("Usage:manifest assets fresh-output");
#endif
    const fs::path assets(argv[2]),out(argv[3]);if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
    IQM::Bin::VulkanInstance instance(true);
    const IQM::HDRVDP3::PathwaySPYRAssets shaders{assets/"spyr_fma.spv",assets/"spyr_separate.spv",assets/"mean.spv",assets/"difference.spv",assets/"residual.spv"};
    const auto filters=IQM::HDRVDP3::create_sp0_model();
    std::ifstream manifest(argv[1]);if(!manifest)throw std::runtime_error("Missing manifest");
    std::ofstream results(out/"results.tsv");results<<"case\tstage\tvalues\tbit_errors\n";
    unsigned h,w,cases=0;size_t total=0;std::string tag,native,golden;
    while(manifest>>tag>>h>>w>>std::quoted(native)>>std::quoted(golden)){
        const auto ref=matrix<float>(fs::path(native)/"P_reference.txt"),test=matrix<float>(fs::path(native)/"P_test.txt");
        const auto actual=IQM::HDRVDP3::run_square_spyr_diagnostic(instance,ref,test,w,h,filters,shaders);
        for(unsigned j=0;j<11;++j){
            const auto expected=matrix<uint32_t>(fs::path(golden)/("stage"+std::to_string(j)+".txt"));
            if(actual.stages[j]!=expected)throw std::runtime_error("SPYR bits:"+tag+"/"+std::to_string(j));
            results<<tag<<'\t'<<j<<'\t'<<expected.size()<<"\t0\n";results.flush();total+=expected.size();
            std::ofstream f(out/(tag+"_stage"+std::to_string(j)+".u32"),std::ios::binary);f.write(reinterpret_cast<const char*>(actual.stages[j].data()),actual.stages[j].size()*4);
        }++cases;
    }
#if defined(IQM_BASE256_DIAGNOSTIC)
    const unsigned expectedCases=argc==5?unsigned(std::stoul(argv[4])):5u;
    if(expectedCases!=5&&expectedCases!=6)throw std::runtime_error("Unvalidated case count");
    const size_t expectedValues=121091u*expectedCases;
#else
    constexpr unsigned expectedCases=20;constexpr size_t expectedValues=1144720;
#endif
    if(!manifest.eof()||cases!=expectedCases||total!=expectedValues)throw std::runtime_error("Incomplete general SPYR coverage");
    std::ofstream(out/"PASS.txt")<<cases<<" native P-map pairs,"<<cases*11<<" SPYR/base checks,"<<total<<" Float32 words bitwise including signed zero. Native P input,not full visual pipeline.\n";
    std::cout<<"General SPYR:"<<cases*11<<" checks/"<<total<<" words bitwise PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
