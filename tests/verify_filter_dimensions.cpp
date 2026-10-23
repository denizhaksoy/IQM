// Runs production LocalAdaptFFT via the isolated evaluation diagnostic hook.
#include <vulkan_instance.h>
#include <IQM/hdrvdp3/local_adapt.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <stdexcept>
#include <vector>
namespace IQM::HDRVDP3 {
std::vector<float> diagnostic_fft_dimensions(const IQM::VulkanInstance&,
    const std::vector<float>&,unsigned,unsigned,FFTFilterMode,
    const std::string&,const std::string&,const std::string&);
}
namespace fs=std::filesystem;
static std::vector<float> read(const fs::path& path,unsigned& h,unsigned& w){
    std::ifstream f(path);if(!(f>>h>>w)||!h||!w)throw std::runtime_error("Invalid shape:"+path.string());
    std::vector<float> result(size_t(h)*w);for(auto& value:result){double parsed;
        if(!(f>>parsed)||!std::isfinite(parsed))throw std::runtime_error("Invalid values:"+path.string());
        value=static_cast<float>(parsed);
    }
    std::string extra;if(f>>extra)throw std::runtime_error("Trailing values");return result;
}
static size_t compare(const std::vector<float>& actual,const std::vector<float>& expected,unsigned h,unsigned packedW,bool column){
    if(actual.size()!=expected.size()||actual.size()!=size_t(h)*packedW)throw std::runtime_error("Size mismatch");
    size_t bad=0;double maximum=0;
    for(size_t k=0;k<expected.size();++k){const auto ci=k/2;
        const auto index=column?2*((ci%(packedW/2))*h+ci/(packedW/2))+k%2:k;
        if(!std::isfinite(actual[index])||actual[index]!=expected[k])++bad;
        maximum=std::max(maximum,std::abs(double(actual[index])-expected[k]));
    }
    std::cout<<"values="<<expected.size()<<" mismatches="<<bad<<" max="<<maximum<<'\n';return bad;
}
int main(int argc,char** argv){try{
    if(argc!=7)throw std::runtime_error("Usage:native-case combined.spv math-dir ref|test mtf|la fresh-export");
    const fs::path source(argv[1]),out(argv[6]);const std::string label(argv[4]),mode(argv[5]);
    if((label!="ref"&&label!="test")||(mode!="mtf"&&mode!="la"))throw std::runtime_error("Invalid mode/label");
    if(!fs::create_directory(out))throw std::runtime_error("Fresh export required");
    const bool mtf=mode=="mtf";unsigned h,w;
    const auto input=read(source/(label+(mtf?"_luminance.txt":"_adapt_input.txt")),h,w);
    const unsigned fh=2*h,fw=2*w;IQM::Bin::VulkanInstance instance(true);
    const auto actual=IQM::HDRVDP3::diagnostic_fft_dimensions(instance,input,w,h,
        mtf?IQM::HDRVDP3::FFTFilterMode::OpticalMTF:IQM::HDRVDP3::FFTFilterMode::LocalAdaptation,
        argv[2],argv[3],(out/"trace_").string());
    const std::string tag=label+"_"+mode;size_t bad=0;
    for(unsigned stage=0;stage<4;++stage){
        unsigned gh,gw,nh,nw;const auto gpu=read(out/("trace_stage"+std::to_string(stage)+".txt"),gh,gw);
        std::string filename=(stage==0||stage==2)?tag+"_stage"+std::to_string(stage)+".txt":label+(mtf?"_native_":"_la_native_")+(stage==1?"forward.txt":"inverse.txt");
        const auto native=read(source/filename,nh,nw);
        if(gh!=fh||gw!=2*fw||nh!=fh||nw!=2*fw)throw std::runtime_error("Stage geometry mismatch");
        std::cout<<"stage="<<stage<<' ';bad+=compare(gpu,native,fh,2*fw,stage==1||stage==3);
    }
    unsigned nh,nw;const auto expected=read(source/(label+(mtf?"_retinal.txt":"_adaptation.txt")),nh,nw);
    std::cout<<"output ";bad+=compare(actual,expected,h,w,false);
    std::ofstream dump(out/"output.txt");dump<<h<<' '<<w<<'\n'<<std::setprecision(9);for(float v:actual)dump<<v<<'\n';
    unsigned gh,gw;const auto filter=read(out/"trace_filter.txt",gh,gw);const auto nativeFilter=read(source/(mode+"_filter_double.txt"),nh,nw);
    if(gh!=fh||gw!=fw||nh!=fh||nw!=fw)throw std::runtime_error("Filter geometry mismatch");
    std::cout<<"host_filter ";bad+=compare(filter,nativeFilter,fh,fw,false);
    std::cout<<"LocalAdaptFFT GPU values="<<8*fh*fw+h*w<<" mismatches="<<bad<<'\n';return bad?1:0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
