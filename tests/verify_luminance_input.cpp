#include <IQM/hdrvdp3/luminance.h>
#include <bit>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <stdexcept>
int main(int argc,char**argv) {
    if(argc!=2)return 2;const std::filesystem::path root(argv[1]);
    std::ifstream cases(root/"cases.txt");std::string file,expected;size_t count=0,values=0,rejected=0;
    while(cases>>file>>expected) {
        if(expected=="reject") {
            try{IQM::HDRVDP3::load_luminance_pfm(root/file);return 1;}
            catch(const std::invalid_argument&){++rejected;}
        } else {
            const auto image=IQM::HDRVDP3::load_luminance_pfm(root/file);std::ifstream in(root/expected);unsigned h,w;
            if(!(in>>h>>w)||h!=image.height||w!=image.width)return 2;
            for(float actual:image.pixels){float reference;if(!(in>>reference))return 2;
                if(std::bit_cast<uint32_t>(actual)!=std::bit_cast<uint32_t>(reference))return 1;++values;}
        }
        ++count;
    }
    if(!cases.eof()||!count)return 2;
    std::cout<<"cases="<<count<<" float_bit_values="<<values<<" bit_mismatches=0 rejected="<<rejected<<'\n';
}
