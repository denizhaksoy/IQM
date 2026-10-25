#include <IQM/hdrvdp3/pathway_spyr.h>
#include <vulkan_instance.h>
#include <bit>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
static std::vector<uint32_t> words(const std::filesystem::path& path){
    std::ifstream f(path);unsigned h,w;if(!(f>>h>>w)||!h||!w||size_t(h)*w>1000000)throw std::runtime_error("Invalid word fixture");
    std::vector<uint32_t> v(size_t(h)*w);for(auto& x:v)if(!(f>>x))throw std::runtime_error("Truncated word fixture");
    std::string extra;if(f>>extra)throw std::runtime_error("Trailing fixture words");return v;
}

int main(int argc,char** argv){try{
 if(argc!=10||std::string(argv[5])!="--all-chain")throw std::runtime_error("Runtime checker requires --all-chain profile");
 const std::filesystem::path native(argv[3]),output(argv[4]);if(std::filesystem::exists(output)||!std::filesystem::create_directories(output))throw std::runtime_error("Fresh output required");
 IQM::Bin::VulkanInstance instance(true);
 const IQM::HDRVDP3::PathwaySPYRFilters filters{words(native/"lo0.txt"),words(native/"lo.txt"),words(native/"hi0.txt"),words(native/"orient.txt")};
 const IQM::HDRVDP3::PathwaySPYRAssets assets{argv[1],argv[2],argv[6],argv[7],argv[8]};
 std::ifstream manifest(native/"manifest.txt");if(!manifest)throw std::runtime_error("Missing manifest");
 std::ofstream results(output/"results.tsv");results<<"case\tstage\tvalues\tnumeric_errors\tbit_errors\n";
 unsigned h,w,cases=0;size_t total=0;std::string tag,folder;
 while(manifest>>tag>>h>>w>>std::quoted(folder)){
 if(tag.ends_with("_test"))continue;
 const auto pairTag=tag.substr(0,tag.size()-10);const auto testFolder=native/(pairTag+"_test");
 auto a=words(std::filesystem::path(folder)/"input.txt"),b=words(testFolder/"input.txt");std::vector<float> ref,test;
 for(auto x:a)ref.push_back(std::bit_cast<float>(x));for(auto x:b)test.push_back(std::bit_cast<float>(x));
 const auto actual=IQM::HDRVDP3::run_pathway_spyr(instance,ref,test,w,h,filters,assets);
 for(unsigned j=0;j<11;++j){std::vector<uint32_t> expected;
 if(j<8){const std::string name=j<4?"stage"+std::to_string(j)+".txt":j==4?"high.txt":"orient"+std::to_string(j-5)+".txt";expected=words(std::filesystem::path(folder)/name);auto v=words(testFolder/name);expected.insert(expected.end(),v.begin(),v.end());}
 else {const auto golden=std::filesystem::path(argv[9])/pairTag;expected=words(golden/(j==8?"mean.txt":j==9?"difference_pack_words.txt":"difference_expected_words.txt"));}
 const auto& values=actual.stages[j];if(values.size()!=expected.size())throw std::runtime_error("Runtime stage size");size_t numeric=0,bits=0,magBits=0;
 for(size_t k=0;k<values.size();++k){numeric+=std::bit_cast<float>(values[k])!=std::bit_cast<float>(expected[k]);bits+=values[k]!=expected[k];if(j==10&&k>=240)magBits+=values[k]!=expected[k];}
 std::ofstream dump(output/(tag+"_stage"+std::to_string(j)+".u32"),std::ios::binary);dump.write(reinterpret_cast<const char*>(values.data()),values.size()*4);
 results<<tag<<'\t'<<j<<'\t'<<values.size()<<'\t'<<numeric<<'\t'<<bits<<'\n';results.flush();
 if(numeric||magBits||((j==8||j==9)&&bits))throw std::runtime_error("Runtime SPYR parity failure");total+=values.size();
 }++cases;
 }
 if(!manifest.eof()||cases!=2||total!=113526)throw std::runtime_error("Incomplete runtime coverage");
 std::ofstream(output/"PASS.txt")<<"Runtime SPYR all-chain:113526 values; native historical numeric/bit acceptance preserved; actual-data range guards.\n";
 std::cout<<"Runtime SPYR PASS values="<<total<<'\n';return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
