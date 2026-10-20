#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

static void write(const std::filesystem::path& root,const std::string& name,unsigned side,const std::vector<float>& values) {
    std::ofstream pfm(root/(name+".pfm"),std::ios::binary),text(root/(name+".txt"));
    pfm<<"Pf\n"<<side<<' '<<side<<"\n-1\n";text<<side<<' '<<side<<'\n'<<std::setprecision(9);
    for(float value:values)text<<value<<'\n';
    for(unsigned y=side;y-->0;)for(unsigned x=0;x<side;++x){
        const auto bits=std::bit_cast<uint32_t>(values[y*side+x]);
        for(unsigned b=0;b<4;++b)pfm.put(static_cast<char>((bits>>(8*b))&255));
    }
    if(!pfm||!text)throw std::runtime_error("Cannot write fresh luminance fixture");
}
int main(int argc,char**argv) {
    if(argc!=3)return 2;
    try {
        const std::filesystem::path out(argv[2]);if(std::filesystem::exists(out))throw std::runtime_error("Output must be fresh");
        if(!stbi_is_hdr(argv[1]))throw std::runtime_error("Expected Radiance HDR source");
        int width,height,channels;
        std::unique_ptr<float,decltype(&stbi_image_free)> rgb(stbi_loadf(argv[1],&width,&height,&channels,3),stbi_image_free);
        if(!rgb||width<64||height<64)throw std::runtime_error("HDR decode failed or source too small");
        std::filesystem::create_directories(out);std::ofstream manifest(out/"cases.txt"),meta(out/"metadata.json");
        meta<<std::setprecision(17)<<"{\"source\":"<<std::quoted(argv[1])<<",\"width\":"<<width<<",\"height\":"<<height
            <<",\"conversion\":\"explicit linear Rec709 numeric Y, binary32 non-FMA\",\"unit_assumption\":\"1 cd/m2 per derived Y unit; not independently photometrically calibrated\",\"crops\":[";
        bool first=true;
        for(unsigned side:{32u,64u}) {
            const unsigned x0=(width-side)/2,y0=(height-side)/2;std::vector<float> luminance(side*side);
            for(unsigned y=0;y<side;++y)for(unsigned x=0;x<side;++x){
                const size_t i=(size_t(y0+y)*width+x0+x)*3;
                const float r=rgb.get()[i]*0.2126f,g=rgb.get()[i+1]*0.7152f,b=rgb.get()[i+2]*0.0722f;
                const float rg=r+g;const float value=rg+b;
                if(!(value>0)||!std::isfinite(value))throw std::runtime_error("Crop outside positive finite luminance scope");
                luminance[y*side+x]=value;
            }
            const auto range=std::minmax_element(luminance.begin(),luminance.end());
            if(!first)meta<<',';first=false;
            meta<<"{\"side\":"<<side<<",\"x\":"<<x0<<",\"y\":"<<y0<<",\"minimum\":"<<*range.first<<",\"maximum\":"<<*range.second<<'}';
            const auto ref="reference"+std::to_string(side);write(out,ref,side,luminance);manifest<<ref<<".pfm "<<ref<<".txt\n";
            for(const std::string mode:{"localized","brighter"}){
                auto test=luminance;
                for(unsigned y=0;y<side;++y)for(unsigned x=0;x<side;++x)
                    if(mode=="brighter"||(x>=side/2&&x<side/2+8&&y>=13&&y<21))test[y*side+x]*=mode=="brighter"?1.05f:1.5f;
                const auto name="test"+std::to_string(side)+"_"+mode;write(out,name,side,test);manifest<<name<<".pfm "<<name<<".txt\n";
            }
            std::cout<<"crop="<<side<<" x="<<x0<<" y="<<y0<<" min="<<*range.first<<" max="<<*range.second<<'\n';
        }
        meta<<"]}\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
