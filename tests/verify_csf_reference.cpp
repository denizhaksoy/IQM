#include <IQM/hdrvdp3/csf.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
int main(int argc,char** argv) {
    const bool strictSingle=argc==3 && std::string(argv[2])=="--single-luminance-strict";
    const bool singleLum=strictSingle || (argc==3 && std::string(argv[2])=="--single-luminance");
    const bool floatOutput=!strictSingle && (singleLum || (argc==3 && std::string(argv[2])=="--float-output"));
    if(argc!=2 && !singleLum && !floatOutput) return 2;
    std::ifstream input(std::filesystem::path(argv[1])/(singleLum?"csf_single.txt":"csf.txt"));
    if(!input) return 2;
    double rho,lum,age,expected,maxError=0,maxRelative=0; int aesl;
    size_t count=0,different=0,floatDifferent=0,agedFloatDifferent=0,baseFloatDifferent=0;
    while(input>>rho>>lum>>age>>aesl>>expected) {
        const auto actual=singleLum?IQM::HDRVDP3::compute_ncsf_single_luminance(rho,float(lum),{age,aesl!=0}):IQM::HDRVDP3::compute_ncsf(rho,lum,{age,aesl!=0});
        if(!std::isfinite(actual) || !std::isfinite(expected)) return 2;
        const double delta=std::abs(actual-expected);
        maxError=std::max(maxError,delta);
        if(expected!=0) maxRelative=std::max(maxRelative,delta/std::abs(expected));
        different+=actual!=expected;
        if(static_cast<float>(actual)!=static_cast<float>(expected)) {
            if(floatDifferent<6) std::cout<<std::setprecision(17)<<"rho="<<rho<<" lum="<<lum<<" age="<<age<<" aesl="<<aesl<<" actual="<<actual<<" expected="<<expected<<'\n';
            ++floatDifferent;
            if(aesl && age>24) ++agedFloatDifferent; else ++baseFloatDifferent;
        }
        ++count;
    }
    if(!input.eof() || !count) return 2;
    std::cout<<std::setprecision(17)<<"cases="<<count<<" max="<<maxError<<" relative="<<maxRelative
        <<" double_mismatches="<<different<<" float_mismatches="<<floatDifferent
        <<" aged_float_mismatches="<<agedFloatDifferent<<" base_float_mismatches="<<baseFloatDifferent<<'\n';
    std::cout<<"Comparison precision: "<<(floatOutput?"float32":"float64")<<'\n';
    return (floatOutput?floatDifferent:different)?1:0;
}
