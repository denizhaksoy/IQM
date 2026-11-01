#include <IQM/hdrvdp3/rgb_image.h>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv){
 if(argc==3&&std::string(argv[1])=="--reject"){
  try{IQM::HDRVDP3::load_rgb_pfm(argv[2]);std::cerr<<"Invalid PF accepted\n";return 1;}catch(const std::exception&){return 0;}
 }
 try{if(argc!=5)throw std::runtime_error("Usage: input expected-planar-f32 width height");auto image=IQM::HDRVDP3::load_rgb_pfm(argv[1]);
  if(image.width!=std::stoul(argv[3])||image.height!=std::stoul(argv[4]))throw std::runtime_error("Image shape mismatch");
  std::ifstream f(argv[2],std::ios::binary|std::ios::ate);if(!f||f.tellg()!=std::streamoff(image.pixels.size()*4))throw std::runtime_error("Expected shape");f.seekg(0);
  for(float v:image.pixels){uint32_t expected;if(!f.read(reinterpret_cast<char*>(&expected),4)||std::bit_cast<uint32_t>(v)!=expected)throw std::runtime_error("PF decoded bits mismatch");}
  std::cout<<"RGB PF bits PASS\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
