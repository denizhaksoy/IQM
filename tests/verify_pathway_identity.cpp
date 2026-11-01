#include <IQM/hdrvdp3/pathway_runtime.h>
#include <vulkan_instance.h>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace fs=std::filesystem;
static std::vector<float> read(const fs::path& p,unsigned& w,unsigned& h){std::ifstream f(p);if(!(f>>h>>w)||!h||!w||size_t(h)*w>1000000)throw std::runtime_error("Bad identity fixture");std::vector<float> v(size_t(h)*w);for(auto& x:v)if(!(f>>x)||!std::isfinite(x))throw std::runtime_error("Invalid native finite data");std::string extra;if(f>>extra)throw std::runtime_error("Trailing native data");return v;}
static void equal(const std::vector<float>& a,const std::vector<float>& b){if(a.size()!=b.size())throw std::runtime_error("Shape mismatch");for(size_t i=0;i<a.size();++i)if(std::bit_cast<uint32_t>(a[i])!=std::bit_cast<uint32_t>(b[i]))throw std::runtime_error("Native bit mismatch");}
int main(int argc,char** argv){try{
 if(argc!=4)throw std::runtime_error("Usage:native-case runtime fresh-output");const fs::path native(argv[1]),out(argv[3]);if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");unsigned w,h,tw,th;const auto reference=read(native/"reference.txt",w,h),test=read(native/"test.txt",tw,th);if(w!=tw||h!=th)throw std::runtime_error("Pair dimensions differ");equal(reference,test);
 const auto assets=IQM::HDRVDP3::load_pathway_runtime(argv[2],w,h);IQM::Bin::VulkanInstance instance(true);IQM::HDRVDP3::PathwayQualityTrace trace;const auto score=IQM::HDRVDP3::evaluate_pathway_quality(instance,reference,test,w,h,assets,&trace);
 equal({score.Q,score.Q_JOD},read(native/"scores.txt",tw,th));if(score.Q!=10||score.Q_JOD!=10||score.error!=0)throw std::runtime_error("Identity score/error mismatch");
 equal(trace.P[0],read(native/"P_reference.txt",tw,th));equal(trace.P[1],read(native/"P_test.txt",tw,th));equal(trace.logAdaptation,read(native/"log_La.txt",tw,th));
 std::ifstream base(native/"base_selection.txt");unsigned bh,bw;double index,frequency,luminance;std::string noise;if(!(base>>bh>>bw>>index>>frequency>>luminance>>noise)||bh!=1||bw!=4||index!=1||frequency!=0||noise!="Inf")throw std::runtime_error("Native zero-frequency limit missing");
 if(trace.spyr.stages[10].size()!=361||std::bit_cast<float>(trace.spyr.stages[10][360])!=0||trace.masking.baseNoise[0]!=0||trace.masking.baseNoise[1]!=0||!std::isinf(trace.masking.baseNoise[2])||trace.masking.baseNoise[2]<0)throw std::runtime_error("Actual GPU selected index/base-noise limit mismatch");
 if(trace.masking.baseAdaptation.size()!=2||std::bit_cast<uint32_t>(trace.masking.baseAdaptation[1])!=std::bit_cast<uint32_t>(float(luminance)))throw std::runtime_error("Base adaptation bits differ");
 for(const auto& b:trace.masking.bands){if(b.error!=0)throw std::runtime_error("Nonzero band error");for(const auto* v:{&b.D,&b.psychometric,&b.masked})for(float x:*v)if(x!=0)throw std::runtime_error("Nonzero identity difference");}
 std::ofstream dump(out/"base_noise.f64",std::ios::binary);dump.write(reinterpret_cast<const char*>(trace.masking.baseNoise.data()),24);if(!dump)throw std::runtime_error("Diagnostic write failed");std::ofstream(out/"PASS.txt")<<"Native P/log/score bits exact;actual GPU selection0,base frequency0,positive infinite noise;all D/psychometric/masked maps and errors zero;Q=Q_JOD=10.\n";std::cout<<"Identity API trace PASS\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
