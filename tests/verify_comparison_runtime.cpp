#include <IQM/hdrvdp3/pathway_comparison.h>
#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <IQM/base/vulkan_runtime.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <IQM/hdrvdp3/resize.h>
using IQM::GPU::VulkanRuntime;
static std::vector<uint32_t> words(const std::filesystem::path& path){
    std::ifstream f(path);unsigned h,w;if(!(f>>h>>w)||!h||!w||size_t(h)*w>1000000)throw std::runtime_error("Invalid word fixture");
    std::vector<uint32_t> v(size_t(h)*w);for(auto& x:v)if(!(f>>x))throw std::runtime_error("Truncated word fixture");
    std::string extra;if(f>>extra)throw std::runtime_error("Trailing fixture words");return v;
}
int main(int argc,char** argv){try{
 if(argc!=4)throw std::runtime_error("Usage:shader.spv native-directory fresh-output");const std::filesystem::path native(argv[2]),out(argv[3]);if(std::filesystem::exists(out)||!std::filesystem::create_directories(out))throw std::runtime_error("Fresh output required");
 IQM::Bin::VulkanInstance instance(true); std::ifstream manifest(native/"manifest.txt");if(!manifest)throw std::runtime_error("Missing manifest");std::ofstream results(out/"results.tsv");results<<"case\tbinary_words\tdouble_values\tbinary_errors\tdouble_bit_errors\n";
 std::string tag,folder,expectedPath;unsigned h,w,nh,nw,cases=0;size_t total=0,binaryTotal=0;
 while(manifest>>tag>>h>>w>>nh>>nw>>std::quoted(folder)>>std::quoted(expectedPath)){
  if(!((h==80&&w==96)||(h==96&&w==80))||!nh||!nw||h%nh||w%nw||h/nh!=w/nw||!(h/nh==1||h/nh==2||h/nh==4||h/nh==8))throw std::runtime_error("Unvalidated shape");
  const unsigned n=h*w,first=h*nw,last=nh*nw;auto reference=words(std::filesystem::path(folder)/"reference.txt"),test=words(std::filesystem::path(folder)/"test.txt"),binary=words(std::filesystem::path(folder)/"binary.txt");if(reference.size()!=n||test.size()!=n||binary.size()!=n)throw std::runtime_error("Bad input shape");
  for(unsigned i=0;i<n;++i)if(!std::isfinite(std::bit_cast<float>(reference[i]))||std::bit_cast<float>(reference[i])<=0||!std::isfinite(std::bit_cast<float>(test[i]))||binary[i]>1)throw std::runtime_error("Input outside domain");
  std::ifstream f(expectedPath);unsigned eh,ew;if(!(f>>eh>>ew)||eh!=nh||ew!=nw)throw std::runtime_error("Bad expected shape");std::vector<uint64_t> expected(last);for(auto& u:expected){double v;if(!(f>>v)||!std::isfinite(v))throw std::runtime_error("Bad native double");u=std::bit_cast<uint64_t>(v);}std::string extra;if(f>>extra)throw std::runtime_error("Trailing expected values");
  std::vector<float> ref,tst;for(auto u:reference)ref.push_back(std::bit_cast<float>(u));for(auto u:test)tst.push_back(std::bit_cast<float>(u));
  const auto result=IQM::HDRVDP3::run_pathway_comparison(instance,ref,tst,w,h,nw,nh,argv[1]);
  size_t mbad=0,bad=0;if(result.binary.size()!=n||result.resized.size()!=last)throw std::runtime_error("Runtime result size");for(unsigned i=0;i<n;++i)mbad+=result.binary[i]!=binary[i];for(unsigned i=0;i<last;++i)bad+=std::bit_cast<uint64_t>(result.resized[i])!=expected[i];
  std::ofstream dump(out/(tag+".u64"),std::ios::binary);dump.write(reinterpret_cast<const char*>(result.resized.data()),last*8);
  results<<tag<<'\t'<<n<<'\t'<<last<<'\t'<<mbad<<'\t'<<bad<<'\n';results.flush();if(mbad||bad)throw std::runtime_error("Comparison mask mismatch:"+tag);total+=last;binaryTotal+=n;++cases;
 }
 if(!manifest.eof()||cases!=90||total!=321840||binaryTotal!=691200)throw std::runtime_error("Incomplete comparison coverage");
 std::cout<<"comparison cases="<<cases<<" binary_words="<<binaryTotal<<" double_values="<<total<<" bit_mismatches=0\n";std::ofstream(out/"PASS.txt")<<"GPU binary mask ->Q20 horizontal ->Q40 double bits exact;90 cases; no host intermediate computation.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
