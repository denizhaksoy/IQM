#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <IQM/hdrvdp3/rgb_image.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
namespace fs=std::filesystem;
int main(int argc,char**argv){try{if(argc!=3)throw std::runtime_error("Usage: source.png|source.pfm fresh-output");const fs::path file=argv[1],out=argv[2];if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");std::string extension=file.extension().string();std::transform(extension.begin(),extension.end(),extension.begin(),[](char c){return char(std::tolower(static_cast<unsigned char>(c)));});unsigned w=0,h=0;int bitDepth=0;std::vector<double> pixels;
 if(extension==".pfm"){auto image=IQM::HDRVDP3::load_rgb_pfm(file);w=image.width;h=image.height;pixels.assign(image.pixels.begin(),image.pixels.end());bitDepth=32;}
 else if(extension==".png"){int width=0,height=0,channels=0;bitDepth=stbi_is_16_bit(file.string().c_str())?16:8;void* raw=bitDepth==16?static_cast<void*>(stbi_load_16(file.string().c_str(),&width,&height,&channels,3)):static_cast<void*>(stbi_load(file.string().c_str(),&width,&height,&channels,3));if(!raw||width<=0||height<=0){if(raw)stbi_image_free(raw);throw std::runtime_error("PNG decode failed");}w=unsigned(width);h=unsigned(height);size_t n=size_t(w)*h;pixels.resize(3*n);for(size_t i=0;i<n;++i)for(size_t c=0;c<3;++c)pixels[c*n+i]=bitDepth==16?double(static_cast<stbi_us*>(raw)[3*i+c])/65535.:double(static_cast<stbi_uc*>(raw)[3*i+c])/255.;stbi_image_free(raw);}
 else throw std::runtime_error("Only PNG and PFM are accepted by this explicit decoder");
 std::ofstream binary(out/"image.f64",std::ios::binary);binary.write(reinterpret_cast<const char*>(pixels.data()),std::streamsize(pixels.size()*8));binary.close();if(!binary)throw std::runtime_error("Image write failed");std::ofstream metadata(out/"image.json");metadata<<"{\"width\":"<<w<<",\"height\":"<<h<<",\"source_bit_depth\":"<<bitDepth<<",\"layout\":\"planar-row-major-f64\",\"resized\":false,\"policy\":\""<<(extension==".png"?"original-integer-samples-divided-by-integer-max":"PFM-float32-scale-then-promote")<<"\"}\n";std::cout<<w<<'x'<<h<<" original samples decoded to Double\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
