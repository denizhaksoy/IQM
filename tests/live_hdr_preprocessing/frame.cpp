#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "../single_fftw_backend.h"
#include <IQM/hdrvdp3/general_quality_single.h>
#include <IQM/hdrvdp3/pq_single.h>
#include <IQM/hdrvdp3/linear_color_single.h>
#include <IQM/hdrvdp3/optical_single.h>
#include <IQM/hdrvdp3/optical_response_single.h>
#include <IQM/hdrvdp3/jnd.h>
#include <IQM/hdrvdp3/resize.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <bit>
#include <iomanip>
#include <chrono>
#include <memory>
namespace fs=std::filesystem;using namespace IQM::HDRVDP3;
int main(int argc,char**argv){try{
 if(argc!=7||fs::exists(argv[6]))throw std::runtime_error("Usage: reference-rgb16.png test-rgb16.png data-root fftw-float-dll ppd fresh-root");
 fs::path data=argv[3],out=argv[6];fs::create_directories(out);double ppd=std::stod(argv[5]);
 SingleFFTW fft(argv[4]);auto backend=fft.backend();
 auto xyz=load_xyz_display_samples(data/"ciexyz31.csv",data/"emission_spectra_oled.csv");
 auto spectral=load_rgb_spectral_samples(data/"log_cone_smith_pokorny_1975.csv",data/"cie_scotopic_lum.txt",data/"emission_spectra_oled.csv");
 auto lut=create_jnd_lut();unsigned w=0,h=0;
 auto start=std::chrono::steady_clock::now();std::ofstream log(out/"progress.log");
 auto trace=[&](const std::string& name,const std::vector<float>& values){
#ifdef IQM_TRACE_SINGLE
   std::ofstream f(out/(name+".f32"),std::ios::binary);f.write(reinterpret_cast<const char*>(values.data()),values.size()*sizeof(float));if(!f)throw std::runtime_error("Trace write failed");
#endif
 };
 auto note=[&](const char* side,const char* stage){log<<side<<' '<<stage<<std::endl;std::cout<<side<<' '<<stage<<std::endl;};
 auto prepare=[&](const char* file,const char* side){
   if(!stbi_is_16_bit(file))throw std::runtime_error("Explicit RGB16 PNG required");int iw,ih,channels;
   std::unique_ptr<stbi_us,decltype(&stbi_image_free)> raw(stbi_load_16(file,&iw,&ih,&channels,3),stbi_image_free);
   if(!raw||iw<16||ih<16||channels!=3)throw std::runtime_error("RGB16 shape/decode failed");
   const unsigned sourceW=unsigned(iw),sourceH=unsigned(ih);const size_t sourceN=size_t(sourceW)*sourceH;
   if(!w){w=sourceW;h=sourceH;}size_t n=size_t(w)*h;
   std::vector<float> encoded(3*sourceN);for(size_t c=0;c<3;++c)for(size_t i=0;i<sourceN;++i)encoded[c*sourceN+i]=normalize_pq_u16_single(raw.get()[3*i+c]);raw.reset();
   if(sourceW!=w||sourceH!=h){std::vector<float> resized(3*n);for(size_t c=0;c<3;++c){std::vector<float> plane(encoded.begin()+c*sourceN,encoded.begin()+(c+1)*sourceN);auto values=resize_bicubic(plane,int(sourceH),int(sourceW),int(h),int(w));for(size_t i=0;i<n;++i)resized[c*n+i]=std::clamp(values[i],0.f,1.f);}encoded=std::move(resized);note(side,"PQ_bicubic_resize");}
   auto linear=pq_to_display_single(encoded,{200.,.005,2000.});encoded.clear();encoded.shrink_to_fit();
   trace(std::string(side)+"_linear",linear);
   QualitySideSingle result;result.native_rgb=prepare_linear_color_single(linear,xyz,LinearColorEncoding::BT2020);linear.clear();linear.shrink_to_fit();note(side,"color");trace(std::string(side)+"_native",result.native_rgb);
   auto optical=filter_optical_rgb_single(result.native_rgb,w,h,ppd,backend);note(side,"optical");
   trace(std::string(side)+"_optical",optical);auto lmsr=project_optical_rgb_single(optical,spectral,24.);optical.clear();optical.shrink_to_fit();trace(std::string(side)+"_lmsr",lmsr.clamped);
   result.adaptation=local_adaptation_single(lmsr.adaptation_input,w,h,ppd,backend);note(side,"adaptation");
   trace(std::string(side)+"_adaptation",result.adaptation);auto jnd=rgb_to_jnd_response_single(lut,lmsr.clamped,result.adaptation,w,h,double(n)/(ppd*ppd),24.);lmsr={};jnd.channels.clear();jnd.channels.shrink_to_fit();note(side,"JND");trace(std::string(side)+"_response",jnd.response);
   result.pyramid=build_sp0_pyramid_single(jnd.response,w,h,ppd);for(size_t b=0;b<result.pyramid.bands.size();++b)trace(std::string(side)+"_band"+std::to_string(b+1),result.pyramid.bands[b].values);note(side,"pyramid");return result;
 };
 auto reference=prepare(argv[1],"reference"),test=prepare(argv[2],"test");
 auto result=evaluate_quality_single(reference,test,w,h,ppd,backend);note("pair","quality");
 double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
 std::ofstream f(out/"score.json");f.precision(17);f<<"{\"scope\":\"independent C++ PNG-to-score Single CPU diagnostic, one frame\",\"width\":"<<w<<",\"height\":"<<h<<",\"ppd\":"<<ppd<<",\"age\":24,\"display\":\"oled\",\"ambient_lux\":200,\"reflectivity\":0.005,\"peak\":2000,\"fftw_threads\":3,\"fftw_planner\":\"estimate\",\"elapsed_seconds\":"<<elapsed<<",\"Q_text\":\""<<result.scores.Q<<"\",\"Q_JOD_text\":\""<<result.scores.Q_JOD<<"\",\"Q_hex\":\""<<std::hex<<std::setw(8)<<std::setfill('0')<<std::bit_cast<uint32_t>(result.scores.Q)<<"\",\"Q_JOD_hex\":\""<<std::setw(8)<<std::bit_cast<uint32_t>(result.scores.Q_JOD)<<std::dec<<"\",\"base_frequency\":"<<result.base_frequency<<",\"band_errors\":[";
 for(size_t b=0;b<result.band_errors.size();++b){if(b)f<<',';f<<result.band_errors[b];}f<<"]}\n";if(!f)throw std::runtime_error("Score write failed");std::cout<<std::setprecision(17)<<"Q="<<result.scores.Q<<" Q_JOD="<<result.scores.Q_JOD<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
