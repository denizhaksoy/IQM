#include <IQM/hdrvdp3/jnd.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

static std::vector<float> image(const std::filesystem::path& path, unsigned& h, unsigned& w) {
    std::ifstream f(path); if (!(f >> h >> w)) throw std::runtime_error("Cannot read image");
    std::vector<float> values(h*w); for (auto& v : values) if (!(f >> v)) throw std::runtime_error("Truncated image");
    return values;
}
static float diameter(float luminance, double area, double age) {
    float x = std::pow((luminance*static_cast<float>(area))/846.0f,0.41f);
    float sd = 7.75f - 5.75f*(x/(x+2.0f));
    float correction = static_cast<float>(std::clamp(age,20.0,83.0)-28.58)*(0.02132f-0.009562f*sd);
    return sd+correction;
}
int main(int argc, char** argv) {
    if (argc!=2) return 2;
    try {
        std::filesystem::path root(argv[1]); const auto lut = IQM::HDRVDP3::create_jnd_lut();
        double unused1,unused2,area,age;
        std::ifstream params(root/"pupil.txt");
        if (!(params>>unused1>>unused2>>area>>age)) throw std::runtime_error("Invalid pupil parameters");
        bool equal=true;
        auto matrix=IQM::HDRVDP3::D65_AGE24_LMR;
        if(std::filesystem::exists(root/"spectral_lmr.txt")) {
            std::ifstream coefficients(root/"spectral_lmr.txt");
            for(double& value:matrix)if(!(coefficients>>value))throw std::runtime_error("Invalid spectral coefficients");
        }
        for (const char* name : {"ref","test"}) {
            unsigned h,w; auto lum=image(root/(std::string(name)+"_luminance.txt"),h,w);
            if (std::filesystem::exists(root/(std::string(name)+"_retinal.txt")))
                lum=image(root/(std::string(name)+"_retinal.txt"),h,w);
            auto adapt=image(root/(std::string(name)+"_adaptation.txt"),h,w);
            float sum=0; for(unsigned x=0;x<w;++x) for(unsigned y=0;y<h;++y) sum+=std::log(adapt[y*w+x]);
            float geo=std::exp(sum/static_cast<float>(adapt.size()));
            float dr=diameter(geo,area,28), da=diameter(geo,area,age);
            float reduction=IQM::HDRVDP3::pupil_luminance_reduction_single(adapt,w,h,area,age);
            std::ifstream pupil(root/(std::string(name)+"_native_pupil_trace.csv"));
            std::string line; std::getline(pupil,line); std::replace(line.begin(),line.end(),',',' ');
            std::istringstream ps(line); double expected;
            for(float value : {geo,dr,da,reduction}) {
                if (!(ps>>expected) || !std::isfinite(expected)) throw std::runtime_error("Invalid pupil trace");
                equal &= (value==expected);
                std::cout<<name<<" pupil C++="<<std::setprecision(17)<<value<<" Octave="<<expected<<'\n'; }
            std::ifstream trace(root/(std::string(name)+"_native_jnd_trace.csv"));
            std::array<unsigned,13> mismatches{}; std::array<double,13> maxerr{};
            for(unsigned x=0;x<w;++x) for(unsigned y=0;y<h;++y) {
                std::array<float,13> values{};
                const auto actual=IQM::HDRVDP3::luminance_to_jnd_response_single(lut,lum[y*w+x],reduction,matrix);
                for(unsigned ch=0;ch<3;++ch) {
                    values[ch]=actual.lmr_before[ch]; values[3+ch]=actual.lmr_after[ch];
                    values[6+ch]=actual.log_query[ch]; values[9+ch]=actual.channels[ch];
                }
                values[12]=actual.response;
                if(!std::getline(trace,line)) throw std::runtime_error("Truncated Octave trace");
                std::replace(line.begin(),line.end(),',',' '); std::istringstream row(line);
                for(size_t i=0;i<13;++i) { if(!(row>>expected) || !std::isfinite(expected) || !std::isfinite(values[i])) throw std::runtime_error("Invalid trace row");
                    double err=std::abs(values[i]-expected); maxerr[i]=std::max(maxerr[i],err); mismatches[i]+=(err!=0); }
            }
            for(size_t i=0;i<13;++i) { equal &= mismatches[i]==0;
                std::cout<<name<<" stage="<<i<<" max="<<maxerr[i]<<" mismatches="<<mismatches[i]<<'\n'; }
        }
        return equal ? 0 : 1;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 2; }
}
