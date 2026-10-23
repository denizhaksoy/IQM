// Uses the actual evaluation.cpp SPYR dispatch; isolated CMake option only.
#include <vulkan_instance.h>
#include <IQM/hdrvdp3/spyr.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <stdexcept>
namespace IQM::HDRVDP3 {
SpyrResult diagnostic_spyr_dimensions(const IQM::VulkanInstance&,
    const std::vector<float>&,unsigned,unsigned,double);
}
namespace fs=std::filesystem;
static std::vector<float> read(const fs::path& file,unsigned& h,unsigned& w){
    std::ifstream f(file);if(!(f>>h>>w)||!h||!w)throw std::runtime_error("Invalid shape:"+file.string());
    std::vector<float> values(size_t(h)*w);
    for(auto& v:values)if(!(f>>v)||!std::isfinite(v))throw std::runtime_error("Invalid values:"+file.string());
    std::string extra;if(f>>extra)throw std::runtime_error("Trailing input");return values;
}
int main(int argc,char** argv){try{
    if(argc!=5)throw std::runtime_error("Usage:native-case ppd band-count fresh-output");
    const fs::path native(argv[1]),output(argv[4]);
    if(!fs::create_directory(output))throw std::runtime_error("Fresh output required");
    unsigned h,w;const auto input=read(native/"input.txt",h,w);
    IQM::Bin::VulkanInstance instance(true);
    const auto result=IQM::HDRVDP3::diagnostic_spyr_dimensions(instance,input,w,h,std::stod(argv[2]));
    if(result.bands.size()!=std::stoul(argv[3]))throw std::runtime_error("Band count mismatch");
    size_t count=0,bad=0;
    for(size_t b=0;b<result.bands.size();++b){
        unsigned nh,nw;const auto expected=read(native/("band"+std::to_string(b)+".txt"),nh,nw);
        const auto& band=result.bands[b];
        if(nh!=band.height||nw!=band.width||expected.size()!=band.data.size())throw std::runtime_error("Band geometry mismatch");
        std::ofstream dump(output/("band"+std::to_string(b)+".txt"));dump<<nh<<' '<<nw<<'\n'<<std::setprecision(9);
        size_t differences=0;double maximum=0;
        for(size_t k=0;k<expected.size();++k){
            // hdrvdp_spyr.get_band divides raw SPYR band b by2^b.
            const float value=band.data[k]/std::ldexp(1.0f,static_cast<int>(b));dump<<value<<'\n';
            if(!std::isfinite(value)||value!=expected[k])++differences;
            maximum=std::max(maximum,std::abs(double(value)-expected[k]));
        }
        std::cout<<"band="<<b<<" shape="<<nh<<'x'<<nw<<" values="<<expected.size()<<" mismatches="<<differences<<" max="<<maximum<<'\n';
        count+=expected.size();bad+=differences;
    }
    std::ofstream(output/"result.txt")<<"values="<<count<<" mismatches="<<bad<<'\n';
    std::cout<<"TOTAL values="<<count<<" mismatches="<<bad<<'\n';return bad?1:0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
