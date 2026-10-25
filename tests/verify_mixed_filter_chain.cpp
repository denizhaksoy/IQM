#include <IQM/hdrvdp3/mixed_filter.h>
// Isolated128-pixel MTF/LA chain. Does not change the bounded public API.
#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <IQM/base/vulkan_runtime.h>
#include <IQM/hdrvdp3/mtf.h>
#include <IQM/hdrvdp3/resize.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
using IQM::GPU::VulkanRuntime;
using IQM::Bin::VulkanInstance;
using IQM::Bin::VulkanResource;
using Path=std::filesystem::path;
static std::vector<float> read(const Path& path,unsigned h,unsigned w){
    std::ifstream f(path);unsigned rh,rw;if(!(f>>rh>>rw)||rh!=h||rw!=w)throw std::runtime_error("Invalid shape:"+path.string());
    std::vector<float> result(h*w);for(auto& v:result){double d;if(!(f>>d)||!std::isfinite(d))throw std::runtime_error("Invalid input");v=float(d);}
    std::string extra;if(f>>extra)throw std::runtime_error("Trailing values");return result;
}
int main(int argc,char** argv){try{
 if(argc!=10&&argc!=11)throw std::runtime_error("Usage:native h w mtf|la ref|test forward.spv inverse.spv io.spv fresh-output [input.f32]");
 const Path source(argv[1]),out(argv[9]);if(std::filesystem::exists(out)||!std::filesystem::create_directories(out))throw std::runtime_error("Fresh output required");const unsigned h=std::stoul(argv[2]),w=std::stoul(argv[3]),fh=2*h,fw=2*w,n=fh*fw,pixels=h*w;const std::string mode(argv[4]),label(argv[5]);const bool mtf=mode=="mtf";if((!mtf&&mode!="la")||(label!="ref"&&label!="test")||!((h==80&&w==96)||(h==96&&w==80)))throw std::runtime_error("Unvalidated filter case");
 auto input=read(source/(label+(mtf?"_luminance.txt":"_adapt_input.txt")),h,w);if(argc==11){std::ifstream f(argv[10],std::ios::binary|std::ios::ate);if(f.tellg()!=std::streamoff(pixels*4))throw std::runtime_error("Bad override input");f.seekg(0);f.read(reinterpret_cast<char*>(input.data()),pixels*4);if(!f)throw std::runtime_error("Truncated override");}
 VulkanInstance instance(true);
 const IQM::HDRVDP3::MixedFilterAssets assets{argv[6],argv[7],argv[8]};
 const auto filterMode=mtf?IQM::HDRVDP3::MixedFilterMode::OpticalMTF:IQM::HDRVDP3::MixedFilterMode::LocalAdaptation;
 const auto data=IQM::HDRVDP3::run_mixed_filter(instance,input,w,h,filterMode,assets,true);
 const auto nativeFilter=read(source/(mtf?"mtf_filter_double.txt":"la_filter_double.txt"),fh,fw);
 size_t modelBad=0;if(data.modelFilter.size()!=n)throw std::runtime_error("Missing runtime model");
 for(size_t i=0;i<n;++i)modelBad+=std::bit_cast<uint32_t>(data.modelFilter[i])!=std::bit_cast<uint32_t>(nativeFilter[i]);
 if(modelBad)throw std::runtime_error("Runtime model filter bit mismatch");
 std::ofstream modelDump(out/"model_filter.f32",std::ios::binary);modelDump.write(reinterpret_cast<const char*>(data.modelFilter.data()),n*4);
 if(h==80&&mtf&&label=="ref"){
 const auto plain=IQM::HDRVDP3::run_mixed_filter(instance,input,w,h,filterMode,assets);
 if(!plain.prepared.empty()||!plain.forward.empty()||!plain.filtered.empty()||!plain.inverse.empty()||!plain.modelFilter.empty()||plain.output.size()!=data.output.size())throw std::runtime_error("Nontrace runtime contract");
 for(size_t i=0;i<plain.output.size();++i)if(std::bit_cast<uint32_t>(plain.output[i])!=std::bit_cast<uint32_t>(data.output[i]))throw std::runtime_error("Nontrace/repeated-call mismatch");
 }
 std::ofstream results(out/"results.tsv");results<<"stage\tvalues\tnumeric_errors\tbit_errors\n";size_t bad=0,total=0;
 auto check=[&](const std::vector<float>& row,const std::vector<float>& expected,unsigned rows,unsigned cols,bool,const std::string& name){size_t numeric=0,bits=0;if(row.size()!=expected.size())throw std::runtime_error("Runtime trace size");for(size_t i=0;i<row.size();++i){numeric+=!std::isfinite(row[i])||row[i]!=expected[i];bits+=std::bit_cast<uint32_t>(row[i])!=std::bit_cast<uint32_t>(expected[i]);}std::ofstream dump(out/(name+".f32"),std::ios::binary);dump.write(reinterpret_cast<const char*>(row.data()),row.size()*4);std::ofstream text(out/(name+".txt"));text<<rows<<' '<<cols<<'\n'<<std::setprecision(17);for(float v:row)text<<v<<'\n';results<<name<<'\t'<<expected.size()<<'\t'<<numeric<<'\t'<<bits<<'\n';results.flush();bad+=numeric+bits;total+=expected.size();};
 const std::string tag=label+"_"+(mtf?"mtf":"la");check(data.prepared,read(source/(tag+"_stage0.txt"),fh,2*fw),fh,2*fw,false,"stage0");check(data.forward,read(source/(label+(mtf?"_native_forward.txt":"_la_native_forward.txt")),fh,2*fw),fh,2*fw,true,"stage1");check(data.filtered,read(source/(tag+"_stage2.txt"),fh,2*fw),fh,2*fw,false,"stage2");check(data.inverse,read(source/(label+(mtf?"_native_inverse.txt":"_la_native_inverse.txt")),fh,2*fw),fh,2*fw,true,"stage3");check(data.output,read(source/(label+(mtf?"_retinal.txt":"_adaptation.txt")),h,w),h,w,false,"output");
 std::cout<<"mixed_filter "<<h<<'x'<<w<<' '<<mode<<' '<<label<<" values="<<total<<" mismatches="<<bad<<'\n';if(bad)return 1;std::ofstream(out/"PASS.txt")<<"Seven-dispatch GPU mixed-size filter chain exact from original input; no host intermediate computation.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
