#include <vulkan_instance.h>
#include <IQM/hdrvdp3/map_gpu.h>
#include <IQM/base/vulkan_runtime.h>
#include <vulkan_res.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <bit>
#include <stdexcept>
namespace fs=std::filesystem;
struct Image{unsigned h,w;std::vector<double> data;};
static Image read(const fs::path& p){std::ifstream f(p);Image r;if(!(f>>r.h>>r.w)||!r.h||!r.w||size_t(r.h)*r.w>1000000)throw std::runtime_error("Bad fixture:"+p.string());r.data.resize(r.h*r.w);for(auto& v:r.data)if(!(f>>v)||!std::isfinite(v))throw std::runtime_error("Bad value");std::string extra;if(f>>extra)throw std::runtime_error("Trailing fixture");return r;}
static std::vector<float> single(const Image& a){return {a.data.begin(),a.data.end()};}
int main(int argc,char** argv){try{
 const bool cpuModel=(argc==8&&std::string(argv[7])=="--cpu-lut")||(argc==6&&std::string(argv[4])=="--prepare-only-cpu");
 const bool gpuPupil=argc>=5&&argc<=8&&std::string(argv[4])=="--gpu-pupil"&&(argc!=8||cpuModel);
 const bool prepareOnly=argc==6&&(std::string(argv[4])=="--prepare-only"||std::string(argv[4])=="--prepare-only-cpu");
 if(argc!=4&&!gpuPupil&&!prepareOnly)throw std::runtime_error("Usage:map13.spv visual-trace-root fresh-output [--gpu-pupil [GPU-filter-root GPU-LA-root]|--prepare-only GPU-filter-root]");const fs::path native(argv[2]),out(argv[3]);if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 IQM::Bin::VulkanInstance instance(true);IQM::HDRVDP3::MapGPU gpu(instance,argv[1]);std::ofstream results(out/"results.tsv");results<<"case\tstage\tvalues\tnumeric_errors\tbit_errors\n";size_t total=0,bad=0,checks=0;
 auto compare=[&](const std::string& tag,const std::string& stage,const std::vector<float>& actual,const Image& expected){auto e=single(expected);if(actual.size()!=e.size())throw std::runtime_error("Shape mismatch");size_t numeric=0,bits=0;for(size_t i=0;i<e.size();++i){numeric+=!std::isfinite(actual[i])||actual[i]!=e[i];bits+=std::bit_cast<uint32_t>(actual[i])!=std::bit_cast<uint32_t>(e[i]);}std::ofstream dump(out/(tag+"_"+stage+".f32"),std::ios::binary);dump.write(reinterpret_cast<const char*>(actual.data()),actual.size()*4);std::ofstream text(out/(tag+"_"+stage+".txt"));text<<expected.h<<' '<<expected.w<<'\n'<<std::setprecision(17);for(float v:actual)text<<v<<'\n';results<<tag<<'\t'<<stage<<'\t'<<e.size()<<'\t'<<numeric<<'\t'<<bits<<'\n';results.flush();bad+=numeric;total+=e.size();++checks;};
 for(const std::string orientation:{"h80_w96","h96_w80"}){
  std::vector<std::vector<float>> adaptation;
  for(const std::string label:{"reference","test"}){
   const auto folder=native/"stages"/orientation;const std::string tag=orientation+"_"+label;auto golden=[&](const std::string& field){return read(folder/(label+"_"+field+".txt"));};auto image=[&](const std::string& field){const std::string shortLabel=label=="reference"?"ref":"test";if(argc>=6&&field=="retinal")return read(fs::path(argv[5])/("gpu_"+orientation+"_mtf_"+shortLabel)/"output.txt");if(argc>=7&&field=="adaptation")return read(fs::path(argv[6])/("gpu_"+orientation+"_la_"+shortLabel+"_from_gpu")/"output.txt");return golden(field);};
   const auto retinal=image("retinal"),matrix=image("matrix");if(retinal.data.size()!=7680||matrix.data.size()!=4)throw std::runtime_error("Visual geometry mismatch");const std::array<double,3> lmr=cpuModel?IQM::HDRVDP3::D65_AGE24_LMR:std::array<double,3>{matrix.data[0],matrix.data[1],matrix.data[3]};
   if(argc>=6)compare(tag,"GPU_retinal_input",single(retinal),golden("retinal"));
   compare(tag,"adaptation_input",gpu.adaptation_input(single(retinal),lmr),image("adaptation_input"));
   if(prepareOnly)continue;
   const auto pupil=image("pupil");if(pupil.data.size()!=4)throw std::runtime_error("Pupil geometry mismatch");
   IQM::HDRVDP3::JNDLUT lut;const auto y1=image("pn_Y1"),y2=image("pn_Y2"),cone=image("pn_JND1"),rod=image("pn_JND2");if(y1.data.size()!=2048||y2.data!=y1.data||cone.data.size()!=2048||rod.data.size()!=2048)throw std::runtime_error("JND LUT geometry mismatch");for(size_t i=0;i<2048;++i){lut.log_luminance[i]=y1.data[i];lut.cone[i]=cone.data[i];lut.rod[i]=rod.data[i];}
   if(cpuModel){const auto generated=IQM::HDRVDP3::create_jnd_lut();std::vector<float> actual;Image expected{1,10241,{}};for(unsigned table=0;table<3;++table){const auto& a=table==0?generated.log_luminance:table==1?generated.cone:generated.rod;const auto& e=table==0?lut.log_luminance:table==1?lut.cone:lut.rod;for(size_t i=0;i<2048;++i){actual.push_back(float(a[i]));expected.data.push_back(e[i]);}}
    for(unsigned table=0;table<2;++table){const auto& a=table==0?generated.cone:generated.rod;const auto& e=table==0?lut.cone:lut.rod;for(size_t i=0;i<2047;++i){actual.push_back(float((a[i+1]-a[i])/(generated.log_luminance[i+1]-generated.log_luminance[i])));expected.data.push_back(float((e[i+1]-e[i])/(lut.log_luminance[i+1]-lut.log_luminance[i])));}}
    for(unsigned ch=0;ch<3;++ch){actual.push_back(float(lmr[ch]));expected.data.push_back(matrix.data[ch==2?3:ch]);}compare(tag,"CPU_model_words",actual,expected);lut=generated;}
   float reduction=float(pupil.data[3]);
   if(gpuPupil){const auto a=image("adaptation"),area=image("area");if(argc>=7)compare(tag,"GPU_adaptation_input",single(a),golden("adaptation"));if(area.data.size()!=1)throw std::runtime_error("Bad pupil area");const double viewingArea=cpuModel?double(a.w)*a.h/(30.0*30.0):area.data[0];const auto trace=gpu.pupil_trace(single(a),a.w,a.h,viewingArea,24);compare(tag,"pupil",{trace.begin(),trace.end()},pupil);reduction=trace[3];}
   const auto response=gpu.jnd_response(single(retinal),lut,reduction,lmr);std::vector<float> channel(response.size());const unsigned sourceChannel[]={1,2,4};
   for(unsigned ch=0;ch<3;++ch){for(size_t i=0;i<response.size();++i)channel[i]=response[i].lmr_before[ch];compare(tag,"LMSR_before"+std::to_string(sourceChannel[ch]),channel,image("LMSR_before"+std::to_string(sourceChannel[ch])));for(size_t i=0;i<response.size();++i)channel[i]=response[i].lmr_after[ch];compare(tag,"LMSR_after"+std::to_string(sourceChannel[ch]),channel,image("LMSR_after"+std::to_string(sourceChannel[ch])));for(size_t i=0;i<response.size();++i)channel[i]=response[i].channels[ch];compare(tag,"JND_channel"+std::to_string(ch+1),channel,image("JND_channel"+std::to_string(ch+1)));}
   for(size_t i=0;i<response.size();++i)channel[i]=response[i].response;compare(tag,"P",channel,image("P"));adaptation.push_back(single(image("adaptation")));
  }
  if(!prepareOnly){const auto mean=gpu.mean_adaptation(adaptation[0],adaptation[1]);compare(orientation,"L_mean",mean,read(native/orientation/"L_mean.txt"));compare(orientation,"log_La",gpu.log_adaptation(mean),read(native/orientation/"log_La.txt"));}
 }
 const unsigned expectedChecks=prepareOnly?8:cpuModel?64:argc==7?60:argc==6?56:gpuPupil?52:48;const size_t expectedValues=prepareOnly?61440:cpuModel?471060:argc==7?430096:argc==6?399376:gpuPupil?368656:368640;
 if(checks!=expectedChecks||total!=expectedValues)throw std::runtime_error("Incomplete visual checks");std::cout<<"visual checks="<<checks<<" values="<<total<<" mismatches="<<bad<<'\n';if(bad)return 1;std::ofstream(out/"PASS.txt")<<"GPU adaptation preparation,JND/P,mean/log; GPU pupil="<<gpuPupil<<";prepare_only="<<prepareOnly<<";GPU_retinal="<<(argc>=6)<<";GPU_adaptation="<<(argc>=7)<<";CPU_model="<<cpuModel<<";checks="<<checks<<";values="<<total<<".\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
