#include "hdrvdp3_live_quality.h"
#include "../shared/hdrvdp3_single_cli.h"
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <vector>
#include <stdexcept>
#include <iostream>
namespace IQM::Bin {
int run_hdrvdp3_live_quality_cli(const Args& args){
    const auto& q=*args.hdrvdp3Quality;const std::filesystem::path runtime=q.runtimeDirectory;
    for(const auto* name:{"projection.spv","mask.spv","mutual.spv","resize.spv","adaptation.spv","padding.spv"})
        if(!std::filesystem::is_regular_file(runtime/name))throw std::invalid_argument(std::string("Missing live-hybrid runtime shader: ")+name);
    std::ostringstream ppd;ppd<<std::setprecision(17)<<q.pixelsPerDegree;
    std::vector<std::string> values{"IQM live-hybrid","--input",args.inputPath,"--ref",args.refPath,
        "--encoding","pq-bt2020-rgb16","--ppd",ppd.str(),"--model-data",(runtime/"model").string(),
        "--fftw",*q.fftwLibrary,"--output",*q.exportDirectory};
    const char* noCmpEnv=getenv("IQM_HDRVDP3_NO_COMPARISON");
    const bool noCmp = (noCmpEnv && std::string(noCmpEnv)=="1");
    for(const auto& pair:{std::pair{"--projection-shader","projection.spv"},std::pair{"--comparison-shader","mask.spv"},
        std::pair{"--mutual-shader","mutual.spv"},std::pair{"--comparison-resize-shader","resize.spv"},
        std::pair{"--adaptation-input-shader","adaptation.spv"},std::pair{"--padding-shader","padding.spv"}}){
        if(noCmp && std::string(pair.first)=="--comparison-shader") continue;
        values.emplace_back(pair.first);values.push_back((runtime/pair.second).string());}
    if(const char* dbg=getenv("IQM_HDRVDP3_DEBUG_ARGS"); dbg && std::string(dbg)=="1"){
        std::cerr<<"[DBG] IQM_HDRVDP3_FULL_GPU_STAGE1="<<(getenv("IQM_HDRVDP3_FULL_GPU_STAGE1")?getenv("IQM_HDRVDP3_FULL_GPU_STAGE1"):"<unset>")<<"\n";
        std::cerr<<"[DBG] IQM_HDRVDP3_NO_COMPARISON="<<(noCmpEnv?noCmpEnv:"<unset>")<<"\n";
        std::cerr<<"[DBG] noCmp="<<(noCmp?"1":"0")<<"\n";
        for(size_t i=0;i<values.size();++i) std::cerr<<"[DBG] argv["<<i<<"]="<<values[i]<<"\n";
        bool hasCmp=false; for(const auto&v:values) if(v=="--comparison-shader") hasCmp=true; std::cerr<<"[DBG] has_comparison_shader="<<(hasCmp?"1":"0")<<"\n";
    }
    if(const char* stage=getenv("IQM_HDRVDP3_FULL_GPU_STAGE1"); stage && std::string(stage)=="1"){
        const char* pqFp64Env = getenv("IQM_HDRVDP3_PQ_FP64");
        const bool pqFp64 = pqFp64Env && std::string(pqFp64Env) == "1";

        const auto pqShader = runtime /
            (pqFp64 ? "pq_display_fp64.spv" : "pq_display.spv");

        if (!std::filesystem::is_regular_file(pqShader))
            throw std::invalid_argument(
                "Missing PQ display shader: " + pqShader.string());

        values.emplace_back("--pq-display-shader");
        values.push_back(pqShader.string());
        if(std::filesystem::is_regular_file(runtime/"jnd.spv")){ values.emplace_back("--jnd-shader"); values.push_back((runtime/"jnd.spv").string()); }
        if(std::filesystem::is_regular_file(runtime/"sp0_corrdn.spv")){ values.emplace_back("--sp0-corrdn-shader"); values.push_back((runtime/"sp0_corrdn.spv").string()); }
        if(std::filesystem::is_regular_file(runtime/"sp0_base_mean.spv")){ values.emplace_back("--sp0-base-mean-shader"); values.push_back((runtime/"sp0_base_mean.spv").string()); }
        for(size_t i=0;i+1<values.size();++i){ if(values[i]=="--adaptation-input-shader" && std::filesystem::is_regular_file(runtime/"adaptation_lmsr.spv")) values[i+1]=(runtime/"adaptation_lmsr.spv").string(); }
        values.emplace_back("--full-gpu");
    }
    if(q.cpuSp0) values.emplace_back("--cpu-sp0");
    std::vector<const char*> argv;for(const auto& value:values)argv.push_back(value.c_str());
    return run_hdrvdp3_single_cli(static_cast<int>(argv.size()),argv.data());
}
}
