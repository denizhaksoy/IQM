#include <IQM/hdrvdp3/spectral.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <stdexcept>
int main(int argc,char**argv) {
    if(argc!=2&&argc!=3)return 2;
    namespace fs=std::filesystem;
    std::ifstream samples(fs::path(argv[1])/"samples.txt"),expected(fs::path(argv[1])/"expected.txt");
    size_t n;if(!(samples>>n)||n<2)return 2;
    IQM::HDRVDP3::SpectralSamples s;s.wavelength.resize(n);s.emission.resize(n);s.sensitivity.resize(n);
    for(size_t i=0;i<n;++i) {
        if(!(samples>>s.wavelength[i]>>s.emission[i]))return 2;
        for(double& value:s.sensitivity[i])if(!(samples>>value))return 2;
    }
    if(argc==3) {
        const fs::path raw(argv[2]);
        const auto loaded=IQM::HDRVDP3::load_d65_spectral_samples(raw/"log_cone_smith_pokorny_1975.csv",raw/"cie_scotopic_lum.txt",raw/"d65.csv");
        size_t differences=0;
        if(loaded.wavelength.size()!=n)return 2;
        for(size_t i=0;i<n;++i){differences+=loaded.wavelength[i]!=s.wavelength[i];differences+=loaded.emission[i]!=s.emission[i];
            for(size_t ch=0;ch<4;++ch)differences+=loaded.sensitivity[i][ch]!=s.sensitivity[i][ch];}
        std::cout<<"raw_sample_values="<<6*n<<" double_mismatches="<<differences<<'\n';
        if(differences)return 1;
        s=loaded;
    }
    size_t cases=0,total=0,mismatches=0;double age;
    while(expected>>age) {
        const auto actual=IQM::HDRVDP3::calibrate_spectral_age(s,age);
        if(age==24 && (actual.matrix[0]!=0.65293140835027819 ||
           actual.matrix[1]!=0.34706859164972181 || actual.matrix[3]!=1.0171300679539299))
            throw std::runtime_error("Age24 legacy D65 coefficients changed");
        const auto compare=[&](const auto& values,const char* label) {
            size_t bad=0;double max=0;
            for(double value:values) {
                double ref;if(!(expected>>ref)||!std::isfinite(value)||!std::isfinite(ref))throw std::runtime_error("Invalid trace");
                bad+=value!=ref;max=std::max(max,std::abs(value-ref));++total;
            }
            mismatches+=bad;
            std::cout<<std::setprecision(17)<<"age="<<age<<" stage="<<label<<" mismatches="<<bad<<" max="<<max<<'\n';
        };
        compare(actual.optical_density_delta,"density_nodes");compare(actual.interpolated_density,"pchip");
        compare(actual.transmission,"transmission");compare(actual.filtered_emission,"emission");
        compare(actual.integrals,"integrals");compare(actual.matrix,"matrix");++cases;
    }
    if(!expected.eof()||!cases)return 2;
    std::cout<<"cases="<<cases<<" double_values="<<total<<" double_mismatches="<<mismatches<<'\n';
    return mismatches?1:0;
}
