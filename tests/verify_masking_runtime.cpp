#include <IQM/hdrvdp3/pathway_masking.h>
#include <vulkan_instance.h>
#include <bit>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace fs=std::filesystem;
struct Image{unsigned h,w;std::vector<double> data;};
static Image read(const fs::path& p){std::ifstream f(p);Image r;if(!(f>>r.h>>r.w)||!r.h||!r.w||size_t(r.h)*r.w>1000000)throw std::runtime_error("Bad fixture:"+p.string());r.data.resize(r.h*r.w);for(auto& v:r.data)if(!(f>>v)||!std::isfinite(v))throw std::runtime_error("Bad value");std::string extra;if(f>>extra)throw std::runtime_error("Trailing fixture");return r;}
static std::vector<float> single(const Image& a){return {a.data.begin(),a.data.end()};}

static std::vector<float> loadFloat(const fs::path& p,size_t count,size_t offset=0,size_t total=0){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!total)total=count;if(f.tellg()!=std::streamoff(total*4))throw std::runtime_error("Bad GPU input:"+p.string());f.seekg(offset*4);std::vector<float> v(count);if(!f.read(reinterpret_cast<char*>(v.data()),count*4))throw std::runtime_error("Truncated GPU input");return v;}
static std::vector<double> loadDouble(const fs::path& p,size_t count){std::ifstream f(p,std::ios::binary|std::ios::ate);if(f.tellg()!=std::streamoff(count*8))throw std::runtime_error("Bad comparison input");f.seekg(0);std::vector<double> v(count);if(!f.read(reinterpret_cast<char*>(v.data()),count*8))throw std::runtime_error("Truncated mask");return v;}
int main(int argc,char** argv){try{
 if(argc!=11)throw std::runtime_error("Runtime masking requires full GPU-input profile");const fs::path native(argv[2]),out(argv[3]);if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 IQM::Bin::VulkanInstance instance(true);std::ofstream results(out/"results.tsv");results<<"case\tstage\tvalues\tnumeric_errors\tbit_errors\n";size_t failures=0,checks=0,values=0;
 auto compare=[&](const std::string& tag,const std::string& stage,const std::vector<float>& actual,const std::vector<float>& expected){if(actual.size()!=expected.size())throw std::runtime_error("Shape mismatch");size_t bad=0,bits=0;for(size_t i=0;i<actual.size();++i){bad+=!std::isfinite(actual[i])||actual[i]!=expected[i];bits+=std::bit_cast<uint32_t>(actual[i])!=std::bit_cast<uint32_t>(expected[i]);}std::ofstream dump(out/(tag+"_"+stage+".f32"),std::ios::binary);dump.write(reinterpret_cast<const char*>(actual.data()),actual.size()*4);results<<tag<<'\t'<<stage<<'\t'<<actual.size()<<'\t'<<bad<<'\t'<<bits<<'\n';results.flush();failures+=bad;if(stage=="quality_scores")failures+=bits;values+=actual.size();++checks;};
 const auto model=IQM::HDRVDP3::pathway_masking_model();
 for(const std::string orientation:{"h80_w96","h96_w80"}){
 const unsigned h=orientation=="h80_w96"?80:96,w=orientation=="h80_w96"?96:80;const auto folder=native/orientation;std::array<IQM::HDRVDP3::PathwayBandInput,5> inputs;
 for(unsigned b=1;b<=5;++b){const std::string prefix="band"+std::to_string(b),tag=orientation+"_"+prefix;auto image=[&](const std::string& field){return read(folder/(prefix+"_"+field+".txt"));};const auto self=image("self"),ref=image("reference"),test=image("test");size_t n=self.data.size();auto& a=inputs[b-1];unsigned index=(h==80?0:5)+b-1;
 a.self=loadFloat(fs::path(argv[7])/("case"+std::to_string(index)+".u32"),n,n,2*n);a.reference=loadFloat(fs::path(argv[6])/orientation/(prefix+"_reference.f32"),n);a.test=loadFloat(fs::path(argv[6])/orientation/(prefix+"_test.f32"),n);a.comparison=loadDouble(fs::path(argv[5])/(orientation+"_p0_b"+std::to_string(b)+".u64"),n);
 compare(tag,"GPU_self_input",a.self,single(self));compare(tag,"GPU_reference_input",a.reference,single(ref));compare(tag,"GPU_test_input",a.test,single(test));
 const std::array<std::pair<std::string,double>,5> params{{{"p",model.p},{"pf",model.pf},{"q",model.q},{"self_scale",model.selfScale},{"neighbor_scale",model.neighborScale}}};for(const auto& [field,value]:params){const auto v=image(field);if(v.data.size()!=1||std::bit_cast<uint64_t>(value)!=std::bit_cast<uint64_t>(v.data[0]))throw std::runtime_error("Default model double mismatch:"+field);}
 }
 const auto log=read(fs::path(argv[8])/orientation/"log_La.txt");if(log.w!=w||log.h!=h)throw std::runtime_error("Log shape");auto selected=loadFloat(fs::path(argv[10])/(orientation+"_reference_stage10.u32"),1,360,361)[0];if(!std::isfinite(selected)||selected<0||selected>=120||selected!=std::trunc(selected))throw std::runtime_error("Invalid GPU selection");
 const auto actual=IQM::HDRVDP3::run_pathway_masking(instance,inputs,single(log),w,h,unsigned(selected),{argv[1],argv[4],argv[9]});
 for(unsigned b=1;b<=5;++b){const std::string prefix="band"+std::to_string(b),tag=orientation+"_"+prefix;auto image=[&](const std::string& field){return read(folder/(prefix+"_"+field+".txt"));};const auto& r=actual.bands[b-1];
 compare(tag,"neighbor",r.neighbor,single(image("neighbor")));compare(tag,"denominator",r.denominator,single(image("denominator")));const auto nr=single(image("reference")),nt=single(image("test"));auto diff=nr;for(size_t i=0;i<diff.size();++i)diff[i]=nt[i]-nr[i];compare(tag,"band_difference",r.difference,diff);compare(tag,"GPU_CSF_input",r.sensitivity,single(image("csf")));
 if(b==5){const auto gold=read(fs::path(argv[8])/orientation/"base_selection.txt");if(gold.data.size()!=4)throw std::runtime_error("Base golden shape");double sum=0;for(float v:actual.baseLog)sum+=double(v);compare(tag,"base_adaptation",actual.baseAdaptation,{float(sum/120.0),float(gold.data[2])});const std::array<double,3> expected{gold.data[0]-1,gold.data[1],gold.data[3]};size_t bad=0;for(unsigned i=0;i<3;++i)bad+=std::bit_cast<uint64_t>(actual.baseNoise[i])!=std::bit_cast<uint64_t>(expected[i]);std::ofstream dump(out/(tag+"_base_noise.f64"),std::ios::binary);dump.write(reinterpret_cast<const char*>(actual.baseNoise.data()),24);results<<tag<<"\tbase_frequency_noise\t3\t"<<bad<<'\t'<<bad<<'\n';failures+=bad;values+=3;++checks;}
 compare(tag,"D",r.D,single(image("difference")));compare(tag,"psychometric",r.psychometric,single(image("psychometric")));const auto golden=image("comparison");size_t bad=0;if(golden.data.size()!=inputs[b-1].comparison.size())throw std::runtime_error("Mask golden size");for(size_t i=0;i<golden.data.size();++i)bad+=std::bit_cast<uint64_t>(golden.data[i])!=std::bit_cast<uint64_t>(inputs[b-1].comparison[i]);results<<tag<<"\tcomparison_double\t"<<golden.data.size()<<'\t'<<bad<<'\t'<<bad<<'\n';failures+=bad;values+=golden.data.size();++checks;
 compare(tag,"D_p",r.masked,single(image("quality_difference")));compare(tag,"quality_error",{r.error},{single(read(folder/"errors.txt")).at(b-1)});
 }
 compare(orientation,"quality_scores",{actual.scores.Q,actual.scores.Q_JOD},single(read(folder/"scores.txt")));
 }
 if(checks!=126||values!=393384)throw std::runtime_error("Incomplete runtime coverage");if(failures)throw std::runtime_error("Runtime masking parity failure");std::ofstream(out/"PASS.txt")<<"Runtime masking:126checks/393384values,native numerical parity and final score bit identity.\n";std::cout<<"Runtime masking PASS\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
