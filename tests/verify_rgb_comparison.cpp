#include <IQM/hdrvdp3/pathway_comparison.h>
#include <vulkan_instance.h>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace fs=std::filesystem;
template<class T>std::vector<T> read(const fs::path& p,size_t n){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()!=std::streamoff(n*sizeof(T)))throw std::runtime_error("Invalid fixture "+p.string());std::vector<T> v(n);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),n*sizeof(T));if(!f)throw std::runtime_error("Read failure");return v;}
template<class T>std::vector<T> rows(const std::vector<T>& col,unsigned side,unsigned channels=1){std::vector<T> r(col.size());for(unsigned c=0;c<channels;++c)for(unsigned y=0;y<side;++y)for(unsigned x=0;x<side;++x)r[c*side*side+y*side+x]=col[c*side*side+x*side+y];return r;}
int main(int argc,char** argv){try{
 if(argc!=4)throw std::runtime_error("Usage: native shader fresh-output");fs::path native(argv[1]),out(argv[3]);if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");IQM::Bin::VulkanInstance instance(true);size_t doubles=0,words=0;
 std::ofstream results(out/"results.tsv");results<<"display\tscale\tbinary_words\tdouble_values\tbit_errors\n";
 for(std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"}){
  auto ref=rows(read<float>(native/(display+"_1_input.f32"),3*96*96),96,3),test=rows(read<float>(native/(display+"_2_input.f32"),3*96*96),96,3);auto mask=rows(read<uint32_t>(native/(display+"_mask.u32"),96*96),96);
  for(unsigned scale:{1,2,4,8}){unsigned side=96/scale;auto expected=rows(read<double>(native/(display+"_mask"+std::to_string(scale)+".f64"),side*side),side);auto actual=IQM::HDRVDP3::run_pathway_comparison_rgb(instance,ref,test,96,96,side,side,argv[2]);size_t errors=0;for(size_t i=0;i<mask.size();++i)errors+=actual.binary[i]!=mask[i];for(size_t i=0;i<expected.size();++i)errors+=std::bit_cast<uint64_t>(actual.resized[i])!=std::bit_cast<uint64_t>(expected[i]);results<<display<<'\t'<<scale<<'\t'<<mask.size()<<'\t'<<expected.size()<<'\t'<<errors<<'\n';results.flush();if(errors)throw std::runtime_error("RGB comparison mismatch");words+=mask.size();doubles+=expected.size();std::ofstream dump(out/(display+"_mask"+std::to_string(scale)+".f64"),std::ios::binary);dump.write(reinterpret_cast<const char*>(actual.resized.data()),expected.size()*8);}
 }
 std::ofstream(out/"PASS.txt")<<words<<" binary words and "<<doubles<<" Float64 mask resize values bitwise. RGB comparison only,not quality.\n";
 std::cout<<"RGB comparison PASS "<<words<<" words / "<<doubles<<" doubles\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
