#include <IQM/hdrvdp3/csf.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
int main(int argc,char** argv) {
    const bool strict=argc==3 && std::string(argv[2])=="--strict-double";
    if(argc!=2 && !strict) return 2;
    try {
        std::ifstream input(std::filesystem::path(argv[1])/"base_noise.txt");
        double frequency,luminance,maximum=0; std::string noise_text,neural_text;
        std::size_t cases=0,double_mismatches=0,neural_mismatches=0;
        const double power=std::pow(10.0,.3424);
        while(input>>frequency>>luminance>>noise_text>>neural_text) {
            const double expected=std::stod(noise_text);
            const float expected_neural=std::stof(neural_text);
            const double actual=1.0/IQM::HDRVDP3::compute_ncsf_single_luminance(frequency,float(luminance));
            const float neural=float(std::pow(actual,2*power));
            if(std::isnan(actual)||std::isnan(expected)||std::isnan(neural)||std::isnan(expected_neural)) return 2;
            double_mismatches+=actual!=expected;
            if(std::isfinite(actual)&&std::isfinite(expected)) maximum=std::max(maximum,std::abs(actual-expected));
            if(neural!=expected_neural) {
                if(neural_mismatches<5) std::cout<<std::setprecision(17)<<"frequency="<<frequency<<" luminance="<<luminance<<" neural="<<neural<<" expected="<<expected_neural<<'\n';
                ++neural_mismatches;
            }
            ++cases;
        }
        if(!input.eof()||!cases) return 2;
        std::cout<<std::setprecision(17)<<"cases="<<cases<<" double_noise_mismatches="<<double_mismatches<<" max_finite_noise_delta="<<maximum<<" float_neural_mismatches="<<neural_mismatches<<'\n';
        std::cout<<(strict?"Pass precision: exact double noise and final float32 neural coefficient.\n":"Pass precision: final float32 neural coefficient; double noise equality is reported separately.\n");
        return neural_mismatches||(strict&&double_mismatches)?1:0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 2;}
}
