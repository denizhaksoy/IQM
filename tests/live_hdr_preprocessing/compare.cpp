// Independent equal-resolution Single diagnostic. No resize approximation.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <IQM/hdrvdp3/pq_single.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace fs=std::filesystem;
uint32_t bits(float f){uint32_t u;std::memcpy(&u,&f,4);return u;}
int main(int argc,char**argv){try{
 if(argc!=4)throw std::runtime_error("Usage: RGB16.png official.f32 fresh-output.json");
 if(fs::exists(argv[3]))throw std::runtime_error("Fresh output required");
 if(!stbi_is_16_bit(argv[1]))throw std::runtime_error("Requires RGB16 PNG");
 int w,h,c;auto *raw=stbi_load_16(argv[1],&w,&h,&c,3);
 if(!raw||w<=0||h<=0||c!=3)throw std::runtime_error("RGB16 decode failed");
 size_t n=size_t(w)*h;std::vector<float> ref(3*n);
 std::ifstream in(argv[2],std::ios::binary);in.read(reinterpret_cast<char*>(ref.data()),std::streamsize(12*n));
 if(!in||in.peek()!=std::char_traits<char>::eof())throw std::runtime_error("Reference byte count mismatch");
 std::vector<float> unit(3*n);
 for(size_t ch=0;ch<3;++ch)for(size_t i=0;i<n;++i)
   unit[ch*n+i]=IQM::HDRVDP3::normalize_pq_u16_single(raw[3*i+ch]);
 const auto decoded=IQM::HDRVDP3::pq_to_display_single(unit,{200.,.005,2000.});
 uint64_t mismatch=0;uint32_t maxUlp=0;double maxAbs=0;size_t first=0;
 for(size_t ch=0;ch<3;++ch)for(size_t i=0;i<n;++i){
   size_t j=ch*n+i;float value=decoded[j];
   if(!std::isfinite(value)||!std::isfinite(ref[j]))throw std::runtime_error("Nonfinite value");
   auto a=bits(value),b=bits(ref[j]);if(a!=b){if(!mismatch)first=j;++mismatch;}
   maxUlp=std::max(maxUlp,a>b?a-b:b-a);maxAbs=std::max(maxAbs,std::abs(double(value)-ref[j]));
 }
 stbi_image_free(raw);
 std::ofstream out(argv[3]);out.precision(17);
 out<<"{\"scope\":\"equal-resolution single preprocessing diagnostic; native C++ pow\",\"width\":"<<w<<",\"height\":"<<h
 <<",\"values\":"<<3*n<<",\"bit_mismatches\":"<<mismatch<<",\"max_ulp\":"<<maxUlp<<",\"max_abs\":"<<maxAbs
 <<",\"first_mismatch_planar_index\":"<<first<<",\"exact\":"<<(mismatch?"false":"true")<<"}\n";
 if(!out)throw std::runtime_error("Output write failed");
 std::cout<<"values="<<3*n<<" bit_mismatches="<<mismatch<<" max_ulp="<<maxUlp<<'\n';
 return mismatch?2:0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
