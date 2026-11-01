#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <Imath/half.h>
#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
namespace fs=std::filesystem;namespace Imf=OPENEXR_IMF_NAMESPACE;
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("Usage: fresh-output");fs::path out=argv[1];if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 const std::array<float,12> samples{0.f,-0.f,.1f,1.f,1000.f,65504.f,1e-5f,1e5f,-.125f,std::numeric_limits<float>::min(),std::numeric_limits<float>::denorm_min(),123.456f};
 for(bool half:{false,true})for(bool xyz:{false,true}){std::string name=(xyz?"xyz_":"rgb_")+(half?std::string("half"):std::string("float"));const std::string names=xyz?"XYZ":"RGB";const int w=7,h=5;Imf::Header header(w,h);header.dataWindow()=IMATH_NAMESPACE::Box2i(IMATH_NAMESPACE::V2i(-4,7),IMATH_NAMESPACE::V2i(2,11));header.displayWindow()=header.dataWindow();std::vector<float> floats(3*w*h);std::vector<IMATH_NAMESPACE::half> halves(3*w*h);std::vector<float> expected(3*w*h);Imf::FrameBuffer frame;
  for(size_t c=0;c<3;++c){header.channels().insert(std::string(1,names[c]),Imf::Channel(half?Imf::HALF:Imf::FLOAT));for(size_t i=0;i<size_t(w*h);++i){const float v=samples[(i+c*3)%samples.size()];if(half){halves[c*w*h+i]=IMATH_NAMESPACE::half(v>65504.f?65504.f:v);expected[c*w*h+i]=float(halves[c*w*h+i]);}else{floats[c*w*h+i]=v;expected[c*w*h+i]=v;}}
   frame.insert(std::string(1,names[c]),Imf::Slice::Make(half?Imf::HALF:Imf::FLOAT,half?static_cast<const void*>(halves.data()+c*w*h):static_cast<const void*>(floats.data()+c*w*h),header.dataWindow()));}
  {Imf::OutputFile image((out/(name+".exr")).string().c_str(),header);image.setFrameBuffer(frame);image.writePixels(h);}
  std::ofstream f(out/(name+"_expected.f32"),std::ios::binary);f.write(reinterpret_cast<const char*>(expected.data()),expected.size()*4);
 }
 {const int w=97,h=65;Imf::Header header(w,h);header.dataWindow()=IMATH_NAMESPACE::Box2i(IMATH_NAMESPACE::V2i(-4,7),IMATH_NAMESPACE::V2i(92,71));header.displayWindow()=header.dataWindow();std::vector<float> values(3*w*h);Imf::FrameBuffer frame;for(unsigned c=0;c<3;++c){std::string channel(1,"RGB"[c]);header.channels().insert(channel,Imf::Channel(Imf::FLOAT));for(unsigned i=0;i<unsigned(w*h);++i)values[c*w*h+i]=.5f+float(i%257)*.03125f+float(c);frame.insert(channel,Imf::Slice::Make(Imf::FLOAT,values.data()+c*w*h,header.dataWindow()));}{Imf::OutputFile file((out/"rgb_metric.exr").string().c_str(),header);file.setFrameBuffer(frame);file.writePixels(h);}std::ofstream f(out/"rgb_metric_expected.f32",std::ios::binary);f.write(reinterpret_cast<const char*>(values.data()),values.size()*4);}
 std::cout<<"4 RGB/XYZ FLOAT/HALF bit controls plus1 positive97x65 reference-run control;not UPIQ\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
