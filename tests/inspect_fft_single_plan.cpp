#include <IQM/hdrvdp3/fftw_single_provider.h>
#include <fstream>
#include <iostream>
#include <limits>
int main(int argc,char**argv){try{
 if(argc!=5)throw std::runtime_error("Usage: float-fftw.dll width height fresh-out");
 size_t used=0;auto w=std::stoul(argv[2],&used);if(used!=std::string(argv[2]).size())throw std::runtime_error("Width");auto h=std::stoul(argv[3],&used);if(used!=std::string(argv[3]).size()||!w||!h||w>8192||h>8192)throw std::runtime_error("Dimensions");
 std::filesystem::path out=argv[4];if(std::filesystem::exists(out)||!std::filesystem::create_directories(out))throw std::runtime_error("Fresh output required");
 IQM::HDRVDP3::FFTWProviderSingle api(argv[1]);auto text=api.symbol<char*(*)(void*)>("fftwf_sprint_plan");
 const size_t n=size_t(w)*h;{
 IQM::HDRVDP3::FFTWProviderSingle::Memory input(api,n*4),output(api,size_t(w)*(h/2+1)*8);api.forget();auto p=api.realPlan(int(w),int(h),static_cast<float*>(input.data),static_cast<IQM::HDRVDP3::FFTWProviderSingle::Complex*>(output.data),64|16|2);if(!p)throw std::runtime_error("Forward plan");char* result=text(p);if(!result)throw std::runtime_error("Forward text");std::ofstream(out/"forward.plan.txt")<<result<<'\n';// sprint_plan strings use the DLL CRT malloc, not fftwf_malloc.
 // Keep these tiny strings until this short-lived process exits.
 api.destroy(p);
 }{
 IQM::HDRVDP3::FFTWProviderSingle::Memory input(api,n*8),output(api,n*8);api.forget();auto p=api.complexPlan(int(w),int(h),static_cast<IQM::HDRVDP3::FFTWProviderSingle::Complex*>(input.data),static_cast<IQM::HDRVDP3::FFTWProviderSingle::Complex*>(output.data),1,64|16|2);if(!p)throw std::runtime_error("Inverse plan");char* result=text(p);if(!result)throw std::runtime_error("Inverse text");std::ofstream(out/"inverse.plan.txt")<<result<<'\n';// sprint_plan strings use the DLL CRT malloc, not fftwf_malloc.
 // Keep these tiny strings until this short-lived process exits.
 api.destroy(p);
 }
 std::ofstream(out/"profile.json")<<"{\"width\":"<<w<<",\"height\":"<<h<<",\"threads\":3,\"flags\":82,\"column_major\":true,\"scope\":\"plan inspection only; no numerical transform or GPU parity claim\"}";std::cout<<"Exported plan graphs "<<w<<"x"<<h<<std::endl;return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
