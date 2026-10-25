#include <IQM/hdrvdp3/quality.h>
#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

int main(int argc,char** argv) {
    try {
        if(argc!=3) throw std::runtime_error("Usage: native-grid fresh-output");
        const std::filesystem::path out(argv[2]);
        if(std::filesystem::exists(out)||!std::filesystem::create_directories(out))
            throw std::runtime_error("Fresh output required");
        std::ifstream input(argv[1]);
        if(!input) throw std::runtime_error("Missing native frequency grid");
        unsigned h,w,count=0,grids=0; double ppd;
        while(input>>h>>w>>ppd) {
            if(!((h==10&&w==12)||(h==12&&w==10))||ppd!=3.75)
                throw std::runtime_error("Unvalidated fixture dimensions/ppd");
            for(unsigned i=0;i<h*w;++i) {
                double expected;
                if(!(input>>expected)) throw std::runtime_error("Truncated grid");
                const auto actual=IQM::HDRVDP3::dominant_frequency_from_index(i,w,h,ppd);
                if(std::bit_cast<uint64_t>(actual)!=std::bit_cast<uint64_t>(expected))
                    throw std::runtime_error("Double bit mismatch at node "+std::to_string(i));
                ++count;
            }
            ++grids;
        }
        if(!input.eof()||count!=240||grids!=2) throw std::runtime_error("Incomplete grid fixture");
        std::ofstream(out/"PASS.txt")<<"Production frequency grid:240 native double values bitwise exact.\n";
        std::cout<<"Production frequency grid PASS:240 double words\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
