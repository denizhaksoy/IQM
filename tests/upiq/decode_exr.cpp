#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfTestFile.h>
#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
namespace fs=std::filesystem;namespace Imf=OPENEXR_IMF_NAMESPACE;
int main(int argc,char**argv){try{
 if(argc!=4)throw std::runtime_error("Usage: input.exr RGB|XYZ fresh-output");
 static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559);if(std::endian::native!=std::endian::little)throw std::runtime_error("Little-endian host required");
 const std::string mapping=argv[2];if(mapping!="RGB"&&mapping!="XYZ")throw std::runtime_error("Explicit source channel names RGB or XYZ required");
 bool tiled=false,deep=false,multipart=false;if(!Imf::isOpenExrFile(argv[1],tiled,deep,multipart)||deep||multipart)throw std::runtime_error("Only single-part non-deep EXR supported");
 Imf::InputFile image(argv[1]);const auto&header=image.header();const auto dw=header.dataWindow(),display=header.displayWindow();
 const int64_t w=int64_t(dw.max.x)-dw.min.x+1,h=int64_t(dw.max.y)-dw.min.y+1;
 if(w<=0||h<=0||uint64_t(w)>std::numeric_limits<size_t>::max()/uint64_t(h)/3/sizeof(float))throw std::runtime_error("Invalid/overflowing dimensions");
 const size_t n=size_t(w)*size_t(h);std::array<const Imf::Channel*,3> channels{};
 for(size_t c=0;c<3;++c){const std::string name(1,mapping[c]);channels[c]=header.channels().findChannel(name);if(!channels[c]||channels[c]->xSampling!=1||channels[c]->ySampling!=1||(channels[c]->type!=Imf::FLOAT&&channels[c]->type!=Imf::HALF))throw std::runtime_error("Missing, subsampled or unsupported channel: "+name);}
 const fs::path out=argv[3];if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 std::vector<float> values(3*n);Imf::FrameBuffer frame;
 for(size_t c=0;c<3;++c)frame.insert(std::string(1,mapping[c]),Imf::Slice::Make(Imf::FLOAT,values.data()+c*n,dw));
 image.setFrameBuffer(frame);image.readPixels(dw.min.y,dw.max.y);
 std::ofstream binary(out/"image.f32",std::ios::binary);binary.write(reinterpret_cast<const char*>(values.data()),std::streamsize(values.size()*sizeof(float)));binary.close();if(!binary)throw std::runtime_error("Pixel write failed");
 std::ofstream metadata(out/"image.json");metadata<<"{\"width\":"<<w<<",\"height\":"<<h<<",\"channels\":\""<<mapping<<"\",\"source_types\":[";for(size_t c=0;c<3;++c)metadata<<(c?",":"")<<'"'<<(channels[c]->type==Imf::FLOAT?"FLOAT":"HALF")<<'"';metadata<<"],\"layout\":\"planar-row-major-little-endian-f32\",\"data_window\":["<<dw.min.x<<','<<dw.min.y<<','<<dw.max.x<<','<<dw.max.y<<"],\"display_window\":["<<display.min.x<<','<<display.min.y<<','<<display.max.x<<','<<display.max.y<<"],\"tiled\":"<<(tiled?"true":"false")<<",\"normalization\":false,\"color_transform\":false,\"resized\":false,\"channel_names_are_not_color_calibration\":true}\n";metadata.close();if(!metadata)throw std::runtime_error("Metadata write failed");
 std::cout<<w<<'x'<<h<<" decoded;original data-window samples, no scale/color transform\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
