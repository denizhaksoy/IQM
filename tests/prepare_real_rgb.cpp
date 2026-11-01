#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_HDR
#include <stb_image.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
int main(int argc,char** argv){try{
 if(argc!=3||!stbi_is_hdr(argv[1]))throw std::runtime_error("Usage: source.hdr fresh-output");
 namespace fs=std::filesystem;fs::path out(argv[2]);if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 int w,h,c;float* source=stbi_loadf(argv[1],&w,&h,&c,3);if(!source)throw std::runtime_error(stbi_failure_reason());
 if(w<96||h<96)throw std::runtime_error("Source too small");double peak=0;
 for(size_t i=0;i<size_t(w)*h*3;++i){if(!std::isfinite(source[i])||source[i]<0)throw std::runtime_error("Invalid source RGB");peak=std::max(peak,double(source[i]));}
 if(!(peak>0))throw std::runtime_error("Invalid source peak");const double scale=1000/peak;unsigned x0=(w-96)/2,y0=(h-96)/2;
 std::vector<float> ref(3*96*96);float minimum=std::numeric_limits<float>::infinity(),maximum=0;
 for(unsigned channel=0;channel<3;++channel)for(unsigned x=0;x<96;++x)for(unsigned y=0;y<96;++y){
  float v=float(double(source[3*(size_t(y+y0)*w+x+x0)+channel])*scale);ref[channel*96*96+x*96+y]=v;minimum=std::min(minimum,v);maximum=std::max(maximum,v);
 }
 stbi_image_free(source);auto test=ref;
 for(unsigned x=45;x<76;++x)for(unsigned y=39;y<66;++y)test[x*96+y]*=.85f;
 for(unsigned x=62;x<85;++x)for(unsigned y=11;y<30;++y)test[2*96*96+x*96+y]+=.1f;
 for(const auto& pair:{std::pair{"reference_rgb.f32",&ref},std::pair{"test_rgb.f32",&test}}){std::ofstream f(out/pair.first,std::ios::binary);f.write(reinterpret_cast<const char*>(pair.second->data()),pair.second->size()*4);if(!f)throw std::runtime_error("Cannot write RGB crop");}
 std::ofstream metadata(out/"mapping.tsv");metadata<<std::setprecision(17)<<"source_width\tsource_height\tx\ty\twidth\theight\tscale\tminimum\tmaximum\n"<<w<<'\t'<<h<<'\t'<<x0<<'\t'<<y0<<"\t96\t96\t"<<scale<<'\t'<<minimum<<'\t'<<maximum<<'\n';
 std::ofstream(out/"scope.txt")<<"Relative linear RGBE decoded with stbi_loadf; original three channels retained, global maximum channel assigned1000 virtual display units, one Float32 cast,center crop,no averaging/floor/gamma/resize. Interpreted as native display primaries for a numerical test,not a recovered absolute/colorimetric camera calibration. Test has local red scaling and blue addition. Binary column-major planar Float32.\n";
 std::cout<<"Prepared true-three-channel HDR-derived RGB96 crop\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
