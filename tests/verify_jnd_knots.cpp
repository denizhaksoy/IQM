#include <IQM/hdrvdp3/jnd.h>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
int main(int argc,char**argv) {
    const bool production=argc==3 && std::string(argv[2])=="--production-lut";
    if(argc!=2 && !production) return 2;
    const std::filesystem::path root(argv[1]); IQM::HDRVDP3::JNDLUT lut;
    std::ifstream table(root/"lut.txt");
    for(size_t i=0;i<IQM::HDRVDP3::JND_LUT_SIZE;++i)
        if(!(table>>lut.log_luminance[i]>>lut.cone[i]>>lut.rod[i])) return 2;
    if(production) {
        const auto actual=IQM::HDRVDP3::create_jnd_lut();
        size_t grid=0,cone=0,rod=0;
        for(size_t i=0;i<IQM::HDRVDP3::JND_LUT_SIZE;++i) {
            grid+=float(actual.log_luminance[i])!=float(lut.log_luminance[i]);
            cone+=float(actual.cone[i])!=float(lut.cone[i]);
            rod+=float(actual.rod[i])!=float(lut.rod[i]);
        }
        std::cout<<"Production LUT rounded grid_mismatches="<<grid<<" cone_mismatches="<<cone<<" rod_mismatches="<<rod<<'\n';
        lut=actual;
    }
    std::ifstream in(root/"queries.txt");
    float input; size_t cases=0,queryMismatches=0,channelMismatches=0,responseMismatches=0;
    while(in>>input) {
        std::array<float,7> expected{};
        for(auto& value:expected) if(!(in>>value)) return 2;
        const auto actual=IQM::HDRVDP3::luminance_to_jnd_response_single(lut,input,1.0f);
        for(size_t ch=0;ch<3;++ch) {
            if(!std::isfinite(actual.channels[ch])) return 2;
            queryMismatches+=actual.log_query[ch]!=expected[ch];
            channelMismatches+=actual.channels[ch]!=expected[ch+3];
        }
        responseMismatches+=actual.response!=expected[6]; ++cases;
    }
    std::cout<<"cases="<<cases<<" query_mismatches="<<queryMismatches<<" channel_mismatches="<<channelMismatches<<" response_mismatches="<<responseMismatches<<'\n';
    return !cases||!in.eof()?2:queryMismatches||channelMismatches||responseMismatches?1:0;
}
