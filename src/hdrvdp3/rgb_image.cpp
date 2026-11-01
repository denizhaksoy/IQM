#include <IQM/hdrvdp3/rgb_image.h>
#include <bit>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
namespace IQM::HDRVDP3 {
RGBImage load_rgb_pfm(const std::filesystem::path& filename){
 std::ifstream f(filename,std::ios::binary);if(!f)throw std::invalid_argument("Cannot open RGB PFM");
 auto line=[&](){std::string s;if(!std::getline(f,s))throw std::invalid_argument("Truncated RGB PFM header");if(!s.empty()&&s.back()=='\r')s.pop_back();return s;};
 if(line()!="PF")throw std::invalid_argument("Expected color PF for rgb-native");
 std::istringstream dims(line());dims.imbue(std::locale::classic());long long w,h;std::string extra;
 if(!(dims>>w>>h)||w<=0||h<=0||w>std::numeric_limits<unsigned>::max()||h>std::numeric_limits<unsigned>::max()||(dims>>extra))throw std::invalid_argument("Invalid RGB PFM dimensions");
 std::istringstream scaling(line());scaling.imbue(std::locale::classic());float scale;
 if(!(scaling>>scale)||!std::isfinite(scale)||scale==0||(scaling>>extra))throw std::invalid_argument("Invalid RGB PFM scale");
 const size_t width=size_t(w),height=size_t(h);
 if(width>std::numeric_limits<size_t>::max()/height||width*height>std::vector<float>().max_size()/3)throw std::invalid_argument("RGB PFM dimensions overflow");
 const size_t n=width*height,count=3*n;auto start=f.tellg();f.seekg(0,std::ios::end);auto remaining=f.tellg()-start;f.seekg(start);
 if(count>std::numeric_limits<size_t>::max()/4||remaining<0||uint64_t(remaining)!=uint64_t(count)*4)throw std::invalid_argument("RGB PFM raster size mismatch");
 RGBImage result{unsigned(w),unsigned(h),std::vector<float>(count)};
 bool swap=(scale<0)!=(std::endian::native==std::endian::little);float magnitude=std::abs(scale);
 for(size_t row=0;row<height;++row)for(size_t x=0;x<width;++x)for(size_t c=0;c<3;++c){
  uint32_t bits;if(!f.read(reinterpret_cast<char*>(&bits),4))throw std::invalid_argument("Truncated RGB PFM raster");
  if(swap)bits=(bits>>24)|((bits>>8)&0xff00u)|((bits<<8)&0xff0000u)|(bits<<24);
  float value=std::bit_cast<float>(bits)*magnitude;if(!std::isfinite(value)||value<0)throw std::invalid_argument("Finite nonnegative RGB required");
  result.pixels[c*n+(height-1-row)*width+x]=value;
 }
 return result;
}
}
