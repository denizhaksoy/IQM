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
namespace fs=std::filesystem;
int main(int argc,char** argv){try{
 if(argc!=3&&!(argc==4&&std::string(argv[3])=="--square96"))throw std::runtime_error("Usage:source.hdr fresh-output [--square96]");if(!stbi_is_hdr(argv[1]))throw std::runtime_error("HDR/RGBE input required");const fs::path out(argv[2]);if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");int width,height,channels;float* image=stbi_loadf(argv[1],&width,&height,&channels,3);if(!image)throw std::runtime_error(stbi_failure_reason());if(width<96||height<96)throw std::runtime_error("HDR source too small");double peak=0;for(size_t i=0;i<size_t(width)*height;++i){double sum=0;for(unsigned c=0;c<3;++c){const float v=image[3*i+c];if(!std::isfinite(v)||v<0)throw std::runtime_error("Invalid relative RGB");sum+=double(v);}peak=std::max(peak,sum/3.0);}if(!(peak>0))throw std::runtime_error("Nonpositive HDR peak");const double scale=1000.0/peak;std::ofstream meta(out/"mapping.tsv");meta<<std::setprecision(17)<<"source_width\tsource_height\tcrop_width\tcrop_height\tx\ty\tscale\tfloor\tmin\tmax\tfloored\n";
 const std::vector<std::pair<unsigned,unsigned>> shapes=argc==4?std::vector<std::pair<unsigned,unsigned>>{{96,96}}:std::vector<std::pair<unsigned,unsigned>>{{80,96},{96,80}};
 for(const auto hw:shapes){const auto h=hw.first,w=hw.second;const unsigned x0=(width-w)/2,y0=(height-h)/2;const fs::path folder=out/("h"+std::to_string(h)+"_w"+std::to_string(w));fs::create_directories(folder);std::ofstream f(folder/"reference.txt",std::ios::binary);f<<h<<' '<<w<<'\n'<<std::setprecision(17);float mn=std::numeric_limits<float>::infinity(),mx=0;unsigned clipped=0;for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){const size_t i=size_t(y+y0)*width+x+x0;const double relative=(double(image[3*i])+double(image[3*i+1])+double(image[3*i+2]))/3.0;const double assigned=relative*scale;if(assigned<1e-4)++clipped;const float luminance=float(std::max(1e-4,assigned));mn=std::min(mn,luminance);mx=std::max(mx,luminance);f<<luminance<<'\n';}if(!f)throw std::runtime_error("Cannot write crop");meta<<width<<'\t'<<height<<'\t'<<w<<'\t'<<h<<'\t'<<x0<<'\t'<<y0<<'\t'<<scale<<"\t0.0001\t"<<mn<<'\t'<<mx<<'\t'<<clipped<<'\n';}
 stbi_image_free(image);std::ofstream(out/"mapping.txt")<<"stbi_loadf RGBE linear Float32 RGB;arithmetic RGB mean in double,global source peak assigned1000cd/m2,floor1e-4,oneFloat32 cast;center crops,no resize/gamma/tone mapping. This is an explicitly assigned grayscale display stimulus,not recovered capture luminance or RGB/spectral validation.\n";std::cout<<"Prepared "<<shapes.size()<<" HDR-derived grayscale crops\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
