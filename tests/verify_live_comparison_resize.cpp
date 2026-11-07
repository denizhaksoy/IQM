#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <vulkan_instance.h>
#include <IQM/hdrvdp3/comparison_resize_gpu.h>
#include <IQM/hdrvdp3/resize.h>
#include <IQM/hdrvdp3/pq_single.h>
#include <IQM/hdrvdp3/linear_color_single.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
namespace fs=std::filesystem;using namespace IQM::HDRVDP3;
std::vector<float> native(const fs::path& file,unsigned& w,unsigned& h,const XYZDisplaySamples& model){
 if(!stbi_is_16_bit(file.string().c_str()))throw std::runtime_error("RGB16 required");int iw,ih,channels;
 std::unique_ptr<stbi_us,decltype(&stbi_image_free)> raw(stbi_load_16(file.string().c_str(),&iw,&ih,&channels,3),stbi_image_free);
 if(!raw||iw<16||ih<16||channels!=3)throw std::runtime_error("RGB16 decode/shape");
 const unsigned sw=unsigned(iw),sh=unsigned(ih);const size_t sn=size_t(sw)*sh;if(!w){w=sw;h=sh;}
 std::vector<float> encoded(3*sn);for(size_t c=0;c<3;++c)for(size_t i=0;i<sn;++i)encoded[c*sn+i]=normalize_pq_u16_single(raw.get()[3*i+c]);raw.reset();
 if(sw!=w||sh!=h){const size_t n=size_t(w)*h;std::vector<float> resized(3*n);for(size_t c=0;c<3;++c){std::vector<float> plane(encoded.begin()+c*sn,encoded.begin()+(c+1)*sn);auto values=resize_bicubic(plane,int(sh),int(sw),int(h),int(w));for(size_t i=0;i<n;++i)resized[c*n+i]=std::clamp(values[i],0.f,1.f);}encoded=std::move(resized);}
 return prepare_linear_color_single(pq_to_display_single(encoded,{200.,.005,2000.}),model,LinearColorEncoding::BT2020);
}
int main(int argc,char**argv){fs::path out;try{
 if(argc!=5)throw std::runtime_error("Usage: verified-cases.tsv model-directory shader fresh-output");out=argv[4];if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 std::ifstream list(argv[1]);if(!list)throw std::runtime_error("Case list missing");const fs::path data=argv[2];const auto model=load_xyz_display_samples(data/"ciexyz31.csv",data/"emission_spectra_oled.csv");
 IQM::Bin::VulkanInstance instance(true,"",true);std::ofstream results(out/"results.jsonl");size_t frames=0;uint64_t total=0;std::string line;
 auto save=[&](bool complete){std::ofstream f(out/"checkpoint.json");f<<"{\"complete\":"<<(complete?"true":"false")<<",\"passed\":"<<(complete?"true":"false")<<",\"completed_frames\":"<<frames<<",\"double_values\":"<<total<<",\"scope\":\"comparison resize stage only, not full quality scores\"}";};save(false);
 while(std::getline(list,line)){if(line.empty())continue;std::istringstream row(line);std::string id,reference,test,extra;std::getline(row,id,'\t');std::getline(row,reference,'\t');std::getline(row,test,'\t');if(id.empty()||reference.empty()||test.empty()||std::getline(row,extra,'\t'))throw std::runtime_error("Case list shape");
  unsigned w=0,h=0;auto r=native(reference,w,h,model),t=native(test,w,h,model);if(w!=3840||h!=2160)throw std::runtime_error("This stage sweep requires verified 4K reference geometry");const size_t n=size_t(w)*h;std::vector<double> mask(n,0.);
  for(size_t c=0;c<3;++c)for(size_t i=0;i<n;++i)if(std::abs(t[c*n+i]-r[c*n+i])/r[c*n+i]>.001f)mask[i]=1.;r.clear();r.shrink_to_fit();t.clear();t.shrink_to_fit();
  uint64_t checked=0;for(auto [nw,nh]:{std::pair{1920u,1080u},{960u,540u},{480u,270u},{240u,135u}}){auto expected=resize_bicubic(mask,int(h),int(w),int(nh),int(nw));auto actual=resize_comparison_gpu(instance,argv[3],mask,h,w,nh,nw);if(actual.dispatches!=2||actual.values.size()!=expected.size())throw std::runtime_error("Resize dispatch/shape");
   for(size_t i=0;i<expected.size();++i)if(std::bit_cast<uint64_t>(expected[i])!=std::bit_cast<uint64_t>(actual.values[i])){results<<"{\"case\":\""<<id<<"\",\"exact\":false,\"target_width\":"<<nw<<",\"first_mismatch\":"<<i<<"}\n";results.flush();throw std::runtime_error("Real mask resize mismatch: "+id);}checked+=expected.size();}
  ++frames;total+=checked;results<<"{\"case\":\""<<id<<"\",\"exact\":true,\"double_values\":"<<checked<<",\"dispatches\":8}\n";results.flush();save(false);std::cout<<"EXACT "<<frames<<" "<<id<<std::endl;
 }
 if(!frames)throw std::runtime_error("Empty case list");save(true);return 0;
 }catch(const std::exception& e){if(!out.empty()&&fs::exists(out))std::ofstream(out/"failure.txt")<<e.what()<<'\n';std::cerr<<e.what()<<'\n';return 1;}}
