#include <IQM/hdrvdp3/sp0_model.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv){try{if(argc!=2)throw std::runtime_error("Usage:fresh-output");const std::filesystem::path out(argv[1]);if(std::filesystem::exists(out)||!std::filesystem::create_directories(out))throw std::runtime_error("Fresh model output required");const auto f=IQM::HDRVDP3::create_sp0_model();auto save=[&](const char* name,const std::vector<uint32_t>& v,unsigned n){std::ofstream file(out/name,std::ios::binary);file<<n<<' '<<n<<'\n';for(auto word:v)file<<word<<'\n';if(!file)throw std::runtime_error("Model write failed");};save("lo0.txt",f.lo0,7);save("lo.txt",f.lo,13);save("hi0.txt",f.high,9);save("orient.txt",f.orientation,9);std::cout<<"Created380 fixed sp0 model words from C++ source\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
