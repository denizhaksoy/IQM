#include "args.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using IQM::Bin::Args;
static Args parse(std::vector<std::string> values){
    values.insert(values.begin(),"IQM");std::vector<const char*> pointers;
    for(const auto& value:values)pointers.push_back(value.c_str());
    return Args(static_cast<unsigned>(pointers.size()),pointers.data());
}
int main(){try{
    const std::vector<std::string> base={"--method","HDRVDP3","--input","test.pfm","--ref","ref.pfm","--hdrvdp3-runtime","runtime"};
    auto args=parse(base);
    if(!args.hdrvdp3Quality||args.hdrvdp3Quality->pixelsPerDegree!=30||args.hdrvdp3Quality->observerAge!=24||args.inputPath!="test.pfm"||args.refPath!="ref.pfm")throw std::runtime_error("CLI defaults or input order changed");
    auto configured=base;configured.insert(configured.end(),{"--hdrvdp3-ppd","60","--hdrvdp3-age","60.001","--hdrvdp3-spectral-data","spectra","--hdrvdp3-export","fresh"});
    args=parse(configured);
    if(args.hdrvdp3Quality->pixelsPerDegree!=60||args.hdrvdp3Quality->observerAge!=60.001||args.hdrvdp3Quality->spectralDirectory!="spectra"||args.hdrvdp3Quality->exportDirectory!="fresh")throw std::runtime_error("Explicit CLI fields changed");
    if(parse({"--method","HDRVDP3"}).hdrvdp3Quality)throw std::runtime_error("Legacy synthetic mode changed");
    auto other=parse({"--method","PSNR","--input","test.png","--ref","ref.png","--psnr-variant","luma"});
    if(other.options.at("--psnr-variant")!="luma"||other.hdrvdp3Quality)throw std::runtime_error("Other metric arguments changed");
    if(!parse({"--help"}).printHelp)throw std::runtime_error("Help changed");
    unsigned rejected=0;
    auto reject=[&](std::vector<std::string> values){try{parse(values);}catch(const std::exception&){++rejected;return;}throw std::runtime_error("Invalid CLI arguments accepted");};
    for(const auto& value:{"nan","inf","1e309","30tail","31","-1"}){auto bad=base;bad.insert(bad.end(),{"--hdrvdp3-ppd",value});reject(bad);}
    for(const auto& value:{"nan","inf","24tail","25","60"}){auto bad=base;bad.insert(bad.end(),{"--hdrvdp3-age",value});reject(bad);}
    for(const auto& suffix:std::vector<std::vector<std::string>>{
        {"--hdrvdp3-runtime","another"},{"--hdrvdp3-runtime"},{"--hdrvdp3-export",""},
        {"--hdrvdp3-export","--enable-validation"},{"--hdrvdp3-unknown","value"},{"--output","image.png"}}){auto bad=base;bad.insert(bad.end(),suffix.begin(),suffix.end());reject(bad);}
    reject({"--method","HDRVDP3","--input","test.pfm","--hdrvdp3-runtime","runtime"});
    reject({"--method","HDRVDP3","--ref","ref.pfm","--hdrvdp3-runtime","runtime"});
    reject({"--method","HDRVDP3","--input","test.pfm","--ref","ref.pfm"});
    reject({"--method","HDRVDP3","--hdrvdp3-runtime","runtime"});
    reject({"--method","PSNR","--input","test.png","--ref","ref.png","--hdrvdp3-runtime","runtime"});
    if(rejected!=22)throw std::runtime_error("Incomplete argument rejection coverage");
    std::cout<<"CLI parser:5 valid scenarios,22 invalid scenarios passed;no Vulkan needed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
