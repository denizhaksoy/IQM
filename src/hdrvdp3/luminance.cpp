#include <IQM/hdrvdp3/luminance.h>
#include <bit>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>

namespace IQM::HDRVDP3 {
namespace {
std::string line(std::istream& in) {
    std::string value;if(!std::getline(in,value))throw std::invalid_argument("Truncated PFM header");
    if(!value.empty()&&value.back()=='\r')value.pop_back();return value;
}
uint32_t swap_bytes(uint32_t v){return (v>>24)|((v>>8)&0xff00u)|((v<<8)&0xff0000u)|(v<<24);}
}
LuminanceImage load_luminance_pfm(const std::filesystem::path& filename) {
    std::ifstream in(filename,std::ios::binary);
    if(!in)throw std::invalid_argument("Cannot open luminance PFM");
    if(line(in)!="Pf")throw std::invalid_argument("Expected grayscale Pf; RGB conversion must be explicit");
    std::istringstream dimensions(line(in));dimensions.imbue(std::locale::classic());
    long long width,height;std::string extra;
    if(!(dimensions>>width>>height)||width<=0||height<=0||
       width>std::numeric_limits<unsigned>::max()||height>std::numeric_limits<unsigned>::max()||(dimensions>>extra))
        throw std::invalid_argument("Invalid PFM dimensions");
    std::istringstream scaling(line(in));scaling.imbue(std::locale::classic());float scale;
    if(!(scaling>>scale)||!std::isfinite(scale)||scale==0||(scaling>>extra))throw std::invalid_argument("Invalid PFM scale");
    const size_t w=static_cast<size_t>(width),h=static_cast<size_t>(height);
    if(w>std::numeric_limits<size_t>::max()/h||w*h>std::vector<float>().max_size())throw std::invalid_argument("PFM dimensions overflow");
    const auto start=in.tellg();in.seekg(0,std::ios::end);const auto remaining=in.tellg()-start;in.seekg(start);
    const size_t count=w*h;
    if(count>std::numeric_limits<size_t>::max()/4||remaining<0||static_cast<uint64_t>(remaining)!=static_cast<uint64_t>(count)*4)
        throw std::invalid_argument("PFM raster size mismatch");
    LuminanceImage result{static_cast<unsigned>(w),static_cast<unsigned>(h),std::vector<float>(count)};
    const bool swap=(scale<0)!=(std::endian::native==std::endian::little);const float magnitude=std::abs(scale);
    for(size_t row=0;row<h;++row)for(size_t x=0;x<w;++x) {
        uint32_t bits;if(!in.read(reinterpret_cast<char*>(&bits),4))throw std::invalid_argument("Truncated PFM raster");
        if(swap)bits=swap_bytes(bits);
        const float value=std::bit_cast<float>(bits)*magnitude;
        if(!(value>0)||!std::isfinite(value))throw std::invalid_argument("Expected positive finite absolute luminance");
        result.pixels[(h-1-row)*w+x]=value;
    }
    return result;
}
}
