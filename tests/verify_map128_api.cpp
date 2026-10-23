// Calls the actual MapGPU interface, preserving the default ABI13 contract.
#include <vulkan_instance.h>
#include <IQM/hdrvdp3/map_gpu.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using Path=std::filesystem::path;
using namespace IQM::HDRVDP3;
template<class T=float>static std::vector<T> read(const Path& path,unsigned h,unsigned w){
    std::ifstream f(path);unsigned rh,rw;if(!(f>>rh>>rw)||rh!=h||rw!=w)throw std::runtime_error("Invalid shape:"+path.string());
    std::vector<T> result(h*w);for(auto& value:result)if(!(f>>value)||!std::isfinite(value))throw std::runtime_error("Invalid values");
    std::string extra;if(f>>extra)throw std::runtime_error("Trailing values");return result;
}
static int contracts(const Path& shader13,const Path& shader14,const Path& base81,const Path& output){
    if(!std::filesystem::create_directory(output))throw std::runtime_error("Fresh contract export required");
    IQM::Bin::VulkanInstance instance(true);unsigned rejected=0;
    auto reject=[&](auto action){try{action();}catch(const std::exception&){++rejected;return;}throw std::runtime_error("Expected profile rejection");};
    reject([&]{MapGPU bad(instance,shader14.string());});
    reject([&]{MapGPU bad(instance,shader13.string(),MapGPUProfile::Extended128Components);});
    reject([&]{MapGPU bad(instance,shader14.string(),static_cast<MapGPUProfile>(17));});
    MapGPU bounded(instance,shader13.string()),extended(instance,shader14.string(),MapGPUProfile::Extended128Components);
    reject([&]{bounded.remove_residual_dc(std::vector<float>(256,1));});
    reject([&]{bounded.base_adaptation(std::vector<float>(256,1));});
    reject([&]{bounded.base_spectrum(std::vector<float>(256,1),base81.string());});
    reject([&]{bounded.base_selection(std::vector<float>(512,0));});
    reject([&]{bounded.mutual_masking(std::vector<float>(16384,1),std::vector<float>(16384,1),128,128,1);});
    reject([&]{bounded.resize_comparison_mask(std::vector<double>(16384,1),128,128,8,8);});
    reject([&]{extended.base_spectrum(std::vector<float>(256,1),base81.string());});
    reject([&]{extended.remove_residual_dc(std::vector<float>(65,1));});
    reject([&]{extended.base_adaptation(std::vector<float>(256,6));});
    if(rejected!=12)throw std::runtime_error("Contract coverage");
    std::ofstream(output/"contracts.txt")<<"rejected="<<rejected<<'\n';std::cout<<"contracts rejected="<<rejected<<'\n';return 0;
}
static int rectangular_contracts(const Path& shader13,const Path& shader14,const Path& shader15,const Path& base83,const Path& output){
    if(!std::filesystem::create_directory(output))throw std::runtime_error("Fresh contract export required");
    IQM::Bin::VulkanInstance instance(true);unsigned rejected=0;
    auto reject=[&](auto action){try{action();}catch(const std::exception&){++rejected;return;}throw std::runtime_error("Expected rectangular profile rejection");};
    reject([&]{MapGPU bad(instance,shader15.string());});
    reject([&]{MapGPU bad(instance,shader15.string(),MapGPUProfile::Extended128Components);});
    reject([&]{MapGPU bad(instance,shader14.string(),MapGPUProfile::RectangularResiduals);});
    reject([&]{MapGPU bad(instance,shader15.string(),static_cast<MapGPUProfile>(17));});
    MapGPU bounded(instance,shader13.string()),extended(instance,shader14.string(),MapGPUProfile::Extended128Components),rect(instance,shader15.string(),MapGPUProfile::RectangularResiduals);
    reject([&]{bounded.remove_residual_dc(std::vector<float>(128,1));});
    reject([&]{extended.remove_residual_dc(std::vector<float>(128,1));});
    reject([&]{bounded.base_adaptation(std::vector<float>(128,1));});
    reject([&]{extended.base_adaptation(std::vector<float>(128,1));});
    reject([&]{bounded.base_spectrum_rectangular(std::vector<float>(128,1),base83.string(),16,8);});
    reject([&]{extended.base_selection_rectangular(std::vector<float>(256,0),8,16);});
    reject([&]{rect.base_spectrum_rectangular(std::vector<float>(128,1),base83.string(),16,8);});
    reject([&]{rect.base_spectrum_rectangular(std::vector<float>(128,1),base83.string(),8,8);});
    reject([&]{rect.base_selection_rectangular(std::vector<float>(256,0),16,16);});
    reject([&]{rect.remove_residual_dc(std::vector<float>(129,1));});
    reject([&]{rect.base_adaptation(std::vector<float>(128,6));});
    reject([&]{auto values=std::vector<float>(256,0);values[3]=std::numeric_limits<float>::quiet_NaN();rect.base_selection_rectangular(values,16,8);});
    if(rejected!=16)throw std::runtime_error("Incomplete rectangular contracts");
    std::ofstream(output/"contracts.txt")<<"rejected="<<rejected<<'\n';std::cout<<"rectangular contracts rejected="<<rejected<<'\n';return 0;
}
static int map_contracts(const Path& shader13,const Path& shader14,const Path& shader15,const Path& shader16,const Path& output){
    if(!std::filesystem::create_directory(output))throw std::runtime_error("Fresh contract output required");
    IQM::Bin::VulkanInstance instance(true);unsigned rejected=0;
    auto reject=[&](auto action){try{action();}catch(const std::exception&){++rejected;return;}throw std::runtime_error("Expected rectangular map rejection");};
    reject([&]{MapGPU bad(instance,shader16.string());});
    reject([&]{MapGPU bad(instance,shader16.string(),MapGPUProfile::Extended128Components);});
    reject([&]{MapGPU bad(instance,shader16.string(),MapGPUProfile::RectangularResiduals);});
    reject([&]{MapGPU bad(instance,shader15.string(),MapGPUProfile::RectangularMaps);});
    reject([&]{MapGPU bad(instance,shader16.string(),static_cast<MapGPUProfile>(17));});
    MapGPU bounded(instance,shader13.string()),extended(instance,shader14.string(),MapGPUProfile::Extended128Components),residual(instance,shader15.string(),MapGPUProfile::RectangularResiduals),rect(instance,shader16.string(),MapGPUProfile::RectangularMaps);
    for(MapGPU* gpu:{&bounded,&extended,&residual}){
        reject([&]{gpu->mutual_masking(std::vector<float>(8192,1),std::vector<float>(8192,1),64,128,1);});
        reject([&]{gpu->resize_comparison_mask(std::vector<double>(8192,1),64,128,8,16);});
    }
    reject([&]{rect.mutual_masking(std::vector<float>(1024,1),std::vector<float>(1024,1),16,64,1);});
    reject([&]{rect.mutual_masking(std::vector<float>(1152,1),std::vector<float>(1152,1),24,48,1);});
    reject([&]{rect.resize_comparison_mask(std::vector<double>(8192,1),64,128,16,16);});
    reject([&]{rect.resize_comparison_mask(std::vector<double>(2048,1),32,64,8,16);});
    reject([&]{rect.resize_comparison_mask(std::vector<double>(8192,1),64,128,128,256);});
    reject([&]{rect.mutual_masking(std::vector<float>(2048,1),std::vector<float>(2048,1),32,64,3);});
    reject([&]{rect.resize_comparison_mask(std::vector<double>(8192,.5),64,128,8,16);});
    reject([&]{rect.mutual_masking(std::vector<float>(8191,1),std::vector<float>(8192,1),64,128,1);});
    reject([&]{auto a=std::vector<float>(8192,1);a[0]=std::numeric_limits<float>::quiet_NaN();rect.mutual_masking(a,std::vector<float>(8192,1),64,128,1);});
    if(rejected!=20)throw std::runtime_error("Incomplete rectangular map contracts");
    std::ofstream(output/"contracts.txt")<<"rejected="<<rejected<<'\n';std::cout<<"map contracts rejected="<<rejected<<'\n';return 0;
}
int main(int argc,char** argv){try{
    if(argc==7&&std::string(argv[1])=="--contracts-maps")return map_contracts(argv[2],argv[3],argv[4],argv[5],argv[6]);
    if(argc==7&&std::string(argv[1])=="--contracts-rectangular")return rectangular_contracts(argv[2],argv[3],argv[4],argv[5],argv[6]);
    if(argc==6&&std::string(argv[1])=="--contracts")return contracts(argv[2],argv[3],argv[4],argv[5]);
    if(argc!=7)throw std::runtime_error("Usage:map.spv base.spv fixture mode profile13|14 fresh-output");
    const Path source(argv[3]),output(argv[6]);const std::string mode(argv[4]);const unsigned profile=unsigned(std::stoul(argv[5]));
    if(!std::filesystem::create_directory(output))throw std::runtime_error("Fresh export required");
    IQM::Bin::VulkanInstance instance(true);MapGPU gpu(instance,argv[1],static_cast<MapGPUProfile>(profile));
    std::vector<float> actual,expected;size_t doubleValues=0,doubleBad=0;
    if(mode=="rect-masking"||mode=="rect-float"||mode=="rect-double"){
        std::ifstream manifest(source/"manifest.txt");if(!manifest)throw std::runtime_error("Missing native manifest");
        const std::string wanted=mode=="rect-masking"?"mask":mode=="rect-float"?"float":"double";
        std::string kind,tag;unsigned h,w,nh,nw,cases,checked=0;std::ofstream doubleDump(output/"double.txt");doubleDump<<std::setprecision(17);
        while(manifest>>kind>>tag>>h>>w>>nh>>nw>>cases){
            if(kind!=wanted)continue;++checked;const auto dir=source/tag;const size_t n=h*w,m=nh*nw;
            if(kind=="double"){
                const auto input=read<double>(dir/"input.txt",cases,unsigned(n)),reference=read<double>(dir/"expected.txt",cases,unsigned(m));
                for(unsigned c=0;c<cases;++c){
                    const auto result=gpu.resize_comparison_mask(std::vector<double>(input.begin()+c*n,input.begin()+(c+1)*n),h,w,nh,nw);
                    for(size_t k=0;k<m;++k){doubleDump<<result[k]<<'\n';if(std::bit_cast<uint64_t>(result[k])!=std::bit_cast<uint64_t>(reference[c*m+k]))++doubleBad;}
                    doubleValues+=m;
                }
            }else{
                const auto input=read(dir/"input.txt",cases,unsigned(kind=="mask"?2*n:n)),reference=read(dir/"expected.txt",cases,unsigned(kind=="mask"?2*n:m));
                expected.insert(expected.end(),reference.begin(),reference.end());
                for(unsigned c=0;c<cases;++c){
                    if(kind=="mask"){
                        const auto result=gpu.mutual_masking(std::vector<float>(input.begin()+c*2*n,input.begin()+c*2*n+n),std::vector<float>(input.begin()+c*2*n+n,input.begin()+(c+1)*2*n),h,w,float(1u<<(c%5)));
                        actual.insert(actual.end(),result.raw.begin(),result.raw.end());actual.insert(actual.end(),result.pooled.begin(),result.pooled.end());
                    }else{
                        const auto result=gpu.resize(std::vector<float>(input.begin()+c*n,input.begin()+(c+1)*n),h,w,nh,nw);actual.insert(actual.end(),result.begin(),result.end());
                    }
                }
            }
            std::cout<<"completed "<<tag<<'\n';
        }
        if(checked!=(wanted=="mask"?8u:wanted=="float"?20u:8u))throw std::runtime_error("Incomplete native manifest");
    }else if(mode=="reductions64"||mode=="reductions128"||mode=="reductions256"){
        const unsigned n=mode=="reductions64"?64:mode=="reductions128"?128:256;const auto input=read(source/"input.txt",512,2*n),reference=read(source/"expected.txt",512,n+3);
        for(unsigned c=0;c<512;++c){
            std::vector<float> residual(input.begin()+c*2*n,input.begin()+c*2*n+n),logs(input.begin()+c*2*n+n,input.begin()+(c+1)*2*n);
            auto centered=gpu.remove_residual_dc(residual);const auto adaptation=gpu.base_adaptation(logs);actual.insert(actual.end(),centered.begin(),centered.end());actual.insert(actual.end(),adaptation.begin(),adaptation.end());
            expected.insert(expected.end(),reference.begin()+c*(n+3),reference.begin()+c*(n+3)+n);
            expected.push_back(reference[c*(n+3)+n+1]);expected.push_back(reference[c*(n+3)+n+2]);
        }
    }else if(mode=="residual8x16"||mode=="residual16x8"){
        const unsigned h=mode=="residual8x16"?8:16,w=128/h;
        const auto input=read(source/"input.txt",512,256),centered=read(source/"expected.txt",512,131),reference=read(source/("expected_fft"+std::to_string(h)+"x"+std::to_string(w)+".txt"),512,385);
        // Eight controls then56 distributed cases;full512-domain chains are
        // validated separately without intermediate CPU readback.
        for(unsigned j=0;j<64;++j){const unsigned c=j<8?j:8+9*(j-8);
            const auto dc=gpu.remove_residual_dc(std::vector<float>(input.begin()+c*256,input.begin()+c*256+128));
            const auto spectrum=gpu.base_spectrum_rectangular(dc,argv[2],w,h);
            const auto selection=gpu.base_selection_rectangular(spectrum,w,h);
            actual.insert(actual.end(),dc.begin(),dc.end());actual.insert(actual.end(),spectrum.begin(),spectrum.end());
            actual.insert(actual.end(),selection.magnitudes.begin(),selection.magnitudes.end());actual.push_back(float(selection.index));
            expected.insert(expected.end(),centered.begin()+c*131,centered.begin()+c*131+128);
            expected.insert(expected.end(),reference.begin()+c*385,reference.begin()+(c+1)*385);
        }
    }else if(mode=="base16"){
        const auto input=read(source/"input.txt",64,512),reference=read(source/"expected.txt",64,769);
        for(unsigned c=0;c<64;++c){std::vector<float> image(256);for(unsigned k=0;k<256;++k)image[k]=input[c*512+2*k];
            const auto spectrum=gpu.base_spectrum(image,argv[2]);const auto selection=gpu.base_selection(spectrum);
            actual.insert(actual.end(),spectrum.begin(),spectrum.end());actual.insert(actual.end(),selection.magnitudes.begin(),selection.magnitudes.end());actual.push_back(float(selection.index));
        }expected=reference;
    }else if(mode=="mask128"){
        constexpr unsigned n=16384;const auto input=read(source/"input.txt",16,2*n);expected=read(source/"expected.txt",16,2*n);
        for(unsigned c=0;c<16;++c){const auto result=gpu.mutual_masking(std::vector<float>(input.begin()+c*2*n,input.begin()+c*2*n+n),std::vector<float>(input.begin()+c*2*n+n,input.begin()+(c+1)*2*n),128,128,float(1u<<(c%5)));
            actual.insert(actual.end(),result.raw.begin(),result.raw.end());actual.insert(actual.end(),result.pooled.begin(),result.pooled.end());}
    }else if(mode=="pupil128"||mode=="quality128"){
        constexpr unsigned n=16384;const bool pupil=mode=="pupil128";const unsigned count=pupil?30:16,width=pupil?n+3:2*n;
        const auto prefix=pupil?"pupil":"quality";const auto input=read(source/(std::string(prefix)+"_input.txt"),count,width);expected=read(source/(std::string(prefix)+"_expected.txt"),count,pupil?4:1);
        for(unsigned c=0;c<count;++c){std::vector<float> image(input.begin()+c*width,input.begin()+c*width+n);
            if(pupil){const double ppd=c<15?30:60;const double age=std::array<double,3>{24,60,60.001}[c%3];const auto result=gpu.pupil_trace(image,128,128,n/(ppd*ppd),age);actual.insert(actual.end(),result.begin(),result.end());}
            else{std::vector<double> mask(input.begin()+c*width+n,input.begin()+(c+1)*width);actual.push_back(gpu.quality_band_error(image,mask,128,128));}
        }
    }else if(mode=="resize-float128"){
        const std::array<std::array<unsigned,2>,10> pairs{{{128,128},{128,64},{128,32},{128,16},{128,8},{64,128},{32,64},{16,32},{8,16},{16,128}}};
        for(auto pair:pairs){const auto side=pair[0],n=pair[1];const auto dir=source/("from"+std::to_string(side)+"_to"+std::to_string(n));
            const auto input=read(dir/"input.txt",8,side*side),reference=read(dir/"expected.txt",8,n*n);expected.insert(expected.end(),reference.begin(),reference.end());
            for(unsigned c=0;c<8;++c){const auto result=gpu.resize(std::vector<float>(input.begin()+c*side*side,input.begin()+(c+1)*side*side),side,side,n,n);actual.insert(actual.end(),result.begin(),result.end());}
        }
    }else if(mode=="resize-mask128"){
        const auto input=read<double>(source/"input.txt",8,16384);std::ofstream dump(output/"double.txt");dump<<std::setprecision(17);
        for(unsigned c=0;c<8;++c)for(unsigned n:{128u,64u,32u,16u,8u}){const auto reference=read<double>(source/("mask"+std::to_string(c)+"_to"+std::to_string(n)+".txt"),n,n);
            const auto result=gpu.resize_comparison_mask(std::vector<double>(input.begin()+c*16384,input.begin()+(c+1)*16384),128,128,n,n);
            for(size_t k=0;k<result.size();++k){dump<<result[k]<<'\n';if(std::bit_cast<uint64_t>(result[k])!=std::bit_cast<uint64_t>(reference[k]))++doubleBad;}doubleValues+=result.size();
        }
    }else throw std::runtime_error("Unknown API mode");
    if(actual.size()!=expected.size())throw std::runtime_error("Output size mismatch");size_t bad=0;std::ofstream dump(output/"float.txt");dump<<std::setprecision(9);
    for(size_t i=0;i<actual.size();++i){dump<<actual[i]<<'\n';if(!std::isfinite(actual[i])||actual[i]!=expected[i]){if(bad<5)std::cout<<"index="<<i<<" gpu="<<actual[i]<<" native="<<expected[i]<<'\n';++bad;}}
    std::cout<<mode<<" profile="<<profile<<" float_values="<<actual.size()<<" mismatches="<<bad<<" double_values="<<doubleValues<<" double_bit_mismatches="<<doubleBad<<'\n';return bad||doubleBad?1:0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
