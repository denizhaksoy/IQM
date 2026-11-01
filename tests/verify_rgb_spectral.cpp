#include <IQM/hdrvdp3/spectral.h>
#include <bit>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
namespace fs=std::filesystem;
int main(int argc,char** argv){try{
    if(argc!=4)throw std::runtime_error("Usage:native-fixtures raw-spectral-data fresh-output");
    const fs::path native(argv[1]),raw(argv[2]),out(argv[3]);if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
    size_t total=0,cases=0;std::ofstream results(out/"results.tsv");results<<"display\tage\tvalues\tbit_errors\n";
    auto compare=[&](double actual,double expected){if(!std::isfinite(actual)||!std::isfinite(expected)||std::bit_cast<uint64_t>(actual)!=std::bit_cast<uint64_t>(expected))throw std::runtime_error("RGB spectral bits differ");++total;};
    for(const std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"}){
        const auto loaded=IQM::HDRVDP3::load_rgb_spectral_samples(raw/"log_cone_smith_pokorny_1975.csv",raw/"cie_scotopic_lum.txt",raw/("emission_spectra_"+display+".csv"));
        std::ifstream samples(native/("samples_"+display+".txt"));size_t n;if(!(samples>>n)||n!=420)throw std::runtime_error("Native samples missing");
        for(size_t i=0;i<n;++i){double v;if(!(samples>>v))throw std::runtime_error("Sample missing");compare(loaded[0].wavelength[i],v);for(size_t color=0;color<3;++color){if(!(samples>>v))throw std::runtime_error("Emission missing");compare(loaded[color].emission[i],v);}for(size_t channel=0;channel<4;++channel){if(!(samples>>v))throw std::runtime_error("Sensitivity missing");compare(loaded[0].sensitivity[i][channel],v);}}
        std::ifstream expected(native/("expected_"+display+".txt"));if(!expected)throw std::runtime_error("Missing calibration fixtures");double age;unsigned ageCases=0;
        while(expected>>age){
            const auto actual=IQM::HDRVDP3::calibrate_rgb_spectral_age(loaded,age);const auto before=total;
            std::ofstream dump(out/(display+"_case"+std::to_string(ageCases)+".f64"),std::ios::binary);
            for(const auto* matrix:{&actual.integrals,&actual.matrix})for(const auto& row:*matrix)for(double value:row){double v;if(!(expected>>v))throw std::runtime_error("Native matrix missing");compare(value,v);dump.write(reinterpret_cast<const char*>(&value),8);}
            results<<std::setprecision(17)<<display<<'\t'<<age<<'\t'<<total-before<<"\t0\n";results.flush();++cases;++ageCases;
        }
        if(!expected.eof()||ageCases!=3)throw std::runtime_error("Native age coverage");
    }
    if(cases!=9||total!=10296)throw std::runtime_error("RGB calibration coverage");
    std::ofstream(out/"PASS.txt")<<"9RGB spectral configurations;10296Float64 raw-sample/integral/matrix values bitwise exact. CPU calibration only,not RGB GPU quality.\n";
    std::cout<<"RGB spectral:9cases/10296Double values bitwise PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
