#include <IQM/hdrvdp3/csf.h>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
int main(int argc,char**argv) {
    const bool strict=argc==3 && std::string(argv[2])=="--strict-double";
    if(argc!=2 && !strict) return 2;
    std::ifstream in(std::filesystem::path(argv[1])/"csf_lut.txt");
    size_t nodes=0,nodeFloat=0,queries=0,queryFloat=0,nodeDouble=0,queryDouble=0;
    double age,rho,maxError=0; size_t singleMismatches=0;
    while(in>>age>>rho) {
        auto lut=IQM::HDRVDP3::create_csf_lut(rho,{age,true});
        for(auto actual:lut) {
            double expected; if(!(in>>expected)) return 2;
            ++nodes; nodeDouble+=actual!=expected;
            nodeFloat+=float(actual)!=float(expected);
        }
        size_t n; if(!(in>>n)) return 2;
        for(size_t i=0;i<n;++i) {
            double q,expected,singleExpected; if(!(in>>q>>expected>>singleExpected)) return 2;
            double actual=IQM::HDRVDP3::sample_csf_lut_log(lut,q);
            if(!std::isfinite(actual)) return 2;
            ++queries; queryDouble+=actual!=expected;
            queryFloat+=float(actual)!=float(expected);
            singleMismatches+=IQM::HDRVDP3::sample_csf_lut_log_single(lut,float(q))!=float(singleExpected);
            maxError=std::max(maxError,std::abs(actual-expected));
        }
    }
    std::cout<<"nodes="<<nodes<<" node_double_mismatches="<<nodeDouble<<" node_float_mismatches="<<nodeFloat
        <<" queries="<<queries<<" query_double_mismatches="<<queryDouble<<" query_float_mismatches="<<queryFloat<<" single_query_mismatches="<<singleMismatches<<" max="<<maxError<<'\n';
    return !in.eof()||!nodes?2:(nodeFloat||queryFloat||singleMismatches||(strict&&(nodeDouble||queryDouble)))?1:0;
}
