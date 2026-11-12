#include "hdrvdp3_live_quality.h"
#include "../shared/hdrvdp3_single_cli.h"
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <vector>
#include <stdexcept>
namespace IQM::Bin {
int run_hdrvdp3_live_quality_cli(const Args& args){
    const auto& q=*args.hdrvdp3Quality;const std::filesystem::path runtime=q.runtimeDirectory;
    for(const auto* name:{"projection.spv","mask.spv","mutual.spv","resize.spv","adaptation.spv","padding.spv"})
        if(!std::filesystem::is_regular_file(runtime/name))throw std::invalid_argument(std::string("Missing live-hybrid runtime shader: ")+name);
    std::ostringstream ppd;ppd<<std::setprecision(17)<<q.pixelsPerDegree;
    std::vector<std::string> values{"IQM live-hybrid","--input",args.inputPath,"--ref",args.refPath,
        "--encoding","pq-bt2020-rgb16","--ppd",ppd.str(),"--model-data",(runtime/"model").string(),
        "--fftw",*q.fftwLibrary,"--output",*q.exportDirectory};
    for(const auto& pair:{std::pair{"--projection-shader","projection.spv"},std::pair{"--comparison-shader","mask.spv"},
        std::pair{"--mutual-shader","mutual.spv"},std::pair{"--comparison-resize-shader","resize.spv"},
        std::pair{"--adaptation-input-shader","adaptation.spv"},std::pair{"--padding-shader","padding.spv"}}){
        values.emplace_back(pair.first);values.push_back((runtime/pair.second).string());}
    std::vector<const char*> argv;for(const auto& value:values)argv.push_back(value.c_str());
    return run_hdrvdp3_single_cli(static_cast<int>(argv.size()),argv.data());
}
}
