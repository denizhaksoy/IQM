// Exact comparison of the production bicubic resize against Octave fixtures.
#include <IQM/hdrvdp3/resize.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

template<class T> int verify(const std::filesystem::path& path) {
    std::ifstream file(path);
    size_t count=0,pixels=0,mismatches=0; double maximum=0;
    int h,w,nh,nw;
    while(file>>h>>w>>nh>>nw) {
        if(h<=0||w<=0||nh<=0||nw<=0) return 2;
        std::vector<T> input(size_t(h)*w),expected(size_t(nh)*nw);
        for(auto&v:input) if(!(file>>v)) return 2;
        for(auto&v:expected) if(!(file>>v)) return 2;
        auto actual=IQM::HDRVDP3::resize_bicubic(input,h,w,nh,nw);
        size_t different=0;
        for(size_t i=0;i<actual.size();++i) {
            if(!std::isfinite(actual[i])||!std::isfinite(expected[i])) return 2;
            different+=actual[i]!=expected[i];
            maximum=std::max(maximum,std::abs(double(actual[i])-expected[i]));
        }
        if(different) std::cout<<h<<'x'<<w<<" -> "<<nh<<'x'<<nw<<" mismatches="<<different<<'\n';
        mismatches+=different; pixels+=actual.size(); ++count;
    }
    std::cout<<"precision_bits="<<sizeof(T)*8<<" cases="<<count<<" pixels="<<pixels<<" mismatches="<<mismatches<<" max="<<maximum<<'\n';
    return !count||!file.eof()?2:mismatches?1:0;
}
int main(int argc,char**argv) {
    if(argc==2) return verify<float>(std::filesystem::path(argv[1])/"resize.txt");
    if(argc==3 && std::string(argv[2])=="--double") return verify<double>(std::filesystem::path(argv[1])/"resize_double.txt");
    return 2;
}
