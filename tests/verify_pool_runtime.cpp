#include <IQM/hdrvdp3/pathway_pool.h>
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
static std::vector<uint32_t> words(const std::filesystem::path& path){
    std::ifstream f(path);unsigned h,w;if(!(f>>h>>w)||!h||!w||size_t(h)*w>1000000)throw std::runtime_error("Invalid word fixture");
    std::vector<uint32_t> v(size_t(h)*w);for(auto& x:v)if(!(f>>x))throw std::runtime_error("Truncated word fixture");
    std::string extra;if(f>>extra)throw std::runtime_error("Trailing fixture words");return v;
}
int main(int argc,char** argv){try{
    if(argc!=5)throw std::runtime_error("Usage:fma.spv separate.spv pool.txt fresh-output");
    const std::filesystem::path output(argv[4]);if(std::filesystem::exists(output)||!std::filesystem::create_directories(output))throw std::runtime_error("Fresh output required");
    IQM::Bin::VulkanInstance instance(true);    std::ifstream fixture(argv[3]);if(!fixture)throw std::runtime_error("Missing fixture");
    std::ofstream results(output/"results.tsv");results<<"case\th\tw\tvalues\tnumeric_errors\tbit_errors\n";
    unsigned h,w,cases=0;float normalization;size_t total=0;
    while(fixture>>h>>w>>normalization){
        if(normalization!=1||!((h==80&&w==96)||(h==96&&w==80)||(h==40&&w==48)||(h==48&&w==40)||(h==20&&w==24)||(h==24&&w==20)||(h==10&&w==12)||(h==12&&w==10)))throw std::runtime_error("Unvalidated pool shape/normalization");
        const unsigned n=h*w,bytes=2*n*4;std::vector<uint32_t> input(2*n),expected(2*n);
        for(auto* v:{&input,&expected})for(auto& u:*v){float f;if(!(fixture>>f)||!std::isfinite(f))throw std::runtime_error("Invalid fixture float");u=std::bit_cast<uint32_t>(f);}
        std::vector<float> reference,testBand;for(unsigned k=0;k<n;++k){reference.push_back(std::bit_cast<float>(input[k]));testBand.push_back(std::bit_cast<float>(input[n+k]));}
        const auto value=IQM::HDRVDP3::run_pathway_pool(instance,reference,testBand,w,h,{argv[1],argv[2]});
        std::vector<uint32_t> actual;for(const auto* band:{&value.raw,&value.pooled})for(float v:*band)actual.push_back(std::bit_cast<uint32_t>(v));
        size_t numeric=0,bits=0;for(size_t k=0;k<expected.size();++k){numeric+=std::bit_cast<float>(actual[k])!=std::bit_cast<float>(expected[k]);bits+=actual[k]!=expected[k];}
        std::ofstream dump(output/("case"+std::to_string(cases)+".u32"),std::ios::binary);dump.write(reinterpret_cast<const char*>(actual.data()),bytes);
        results<<cases<<'\t'<<h<<'\t'<<w<<'\t'<<expected.size()<<'\t'<<numeric<<'\t'<<bits<<'\n';results.flush();
        if(numeric||bits)throw std::runtime_error("Pool mismatch case="+std::to_string(cases));total+=expected.size();++cases;
    }
    if(!fixture.eof()||cases!=10||total!=71520)throw std::runtime_error("Incomplete pool coverage");
    std::ofstream(output/"PASS.txt")<<"Actual pathway mutual masking/pool:10 bands,71520 runtime values bitwise exact.\n";std::cout<<"pool cases="<<cases<<" values="<<total<<" mismatches=0\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
