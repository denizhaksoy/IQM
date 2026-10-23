#include <vulkan_instance.h>
#include <IQM/hdrvdp3/map_gpu.h>
#include <IQM/hdrvdp3/resize.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>
namespace fs=std::filesystem;
static std::vector<float> read_image(const fs::path& path,int& h,int& w) {
    std::ifstream in(path); if(!(in>>h>>w)||h<=0||w<=0) throw std::runtime_error("Invalid image: "+path.string());
    std::vector<float> result(size_t(h)*w);
    for(float& value:result) if(!(in>>value)||!std::isfinite(value)) throw std::runtime_error("Invalid image values");
    return result;
}
static std::vector<float> as_single(const std::vector<double>& input) {
    std::vector<float> result; result.reserve(input.size());
    for(double value:input) result.push_back(static_cast<float>(value));
    return result;
}
static std::vector<double> read_double_image(const fs::path& path) {
    std::ifstream in(path); size_t h,w; if(!(in>>h>>w)||!h||!w) throw std::runtime_error("Invalid double map");
    std::vector<double> result(h*w); for(double& v:result) if(!(in>>v)||!std::isfinite(v)) throw std::runtime_error("Invalid double values");
    return result;
}
struct Comparison {
    size_t cases=0,pixels=0,mismatches=0; double maximum=0;
    size_t double_cases=0,double_pixels=0,double_mismatches=0; double double_maximum=0;
    void compare_double(const std::vector<double>& actual,const std::vector<double>& expected) {
        if(actual.size()!=expected.size()) throw std::runtime_error("Double map size mismatch");
        for(size_t i=0;i<actual.size();++i) {
            if(!std::isfinite(actual[i])||!std::isfinite(expected[i])) throw std::runtime_error("Nonfinite double result");
            if(std::bit_cast<uint64_t>(actual[i])!=std::bit_cast<uint64_t>(expected[i])) {
                if(double_mismatches<5) std::cout<<"Double mismatch case="<<double_cases<<" pixel="<<i<<'\n';
                ++double_mismatches;
            }
            double_maximum=std::max(double_maximum,std::abs(actual[i]-expected[i]));
        }
        ++double_cases;double_pixels+=actual.size();
    }
    void compare(const std::vector<float>& actual,const std::vector<float>& expected) {
        if(actual.size()!=expected.size()) throw std::runtime_error("Map size mismatch");
        size_t different=0;
        for(size_t i=0;i<actual.size();++i) {
            if(!std::isfinite(actual[i])||!std::isfinite(expected[i])) throw std::runtime_error("Nonfinite map result");
            if(actual[i]!=expected[i]) { if(mismatches+different<5) std::cout<<"pixel="<<i<<" actual="<<actual[i]<<" expected="<<expected[i]<<'\n'; ++different; }
            maximum=std::max(maximum,std::abs(double(actual[i])-expected[i]));
        }
        ++cases; pixels+=actual.size(); mismatches+=different;
        if(different) std::cout<<"case="<<cases<<" mismatches="<<different<<'\n';
    }
};
int main(int argc,char**argv) {
    if(argc!=4) return 2;
    try {
        IQM::Bin::VulkanInstance instance(true);
        IQM::HDRVDP3::MapGPU gpu(instance,argv[1]);
        const fs::path root(argv[2]); const std::string mode(argv[3]); Comparison check;
        if(mode=="--pupil-age") {
            std::ifstream in(root/"pupil_age.txt"); int h,w; double area,age;
            while(in>>h>>w>>area>>age) {
                if((h!=32&&h!=64)||w!=h) return 2;
                std::vector<float> input(size_t(h)*w),expected(4);
                for(auto* map:{&input,&expected}) for(float& value:*map)
                    if(!(in>>value)||!std::isfinite(value)) return 2;
                const auto actual=gpu.pupil_trace(input,w,h,area,age);
                check.compare({actual.begin(),actual.end()},expected);
            }
            if(!in.eof()) return 2;
        } else if(mode=="--mask-resize") {
            std::ifstream in(root/"mask_resize.txt"); int h,w,nh,nw;
            while(in>>h>>w>>nh>>nw) {
                if(h!=w||(h!=32&&h!=64)||nh<=0||nw<=0) return 2;
                std::vector<double> input(size_t(h)*w),expected(size_t(nh)*nw);
                for(auto* map:{&input,&expected}) for(double& v:*map) if(!(in>>v)) return 2;
                const auto actual=gpu.resize_comparison_mask(input,h,w,nh,nw);
                check.compare_double(actual,expected);
            }
            if(!in.eof()) return 2;
        } else if(mode=="--base-csf-parameters") {
            std::ifstream in(root/"base_csf_parameters.txt"); size_t n;
            if(!(in>>n)||!n) return 2;
            std::vector<float> input(n),expected(5*n),actual;
            for(auto* map:{&input,&expected}) for(float& v:*map) if(!(in>>v)) return 2;
            const auto traces=gpu.base_csf_parameters(input);
            for(const auto& trace:traces) actual.insert(actual.end(),trace.begin(),trace.end());
            check.compare(actual,expected);
            std::ifstream noise_file(root/"base_noise.txt"); double frequency,luminance; std::string noise_text,neural_text;
            size_t noise_cases=0,noise_mismatches=0,neural_mismatches=0;
            while(noise_file>>frequency>>luminance>>noise_text>>neural_text) {
                const auto found=std::lower_bound(input.begin(),input.end(),float(luminance));
                if(found==input.end()||*found!=float(luminance)) return 2;
                const auto& trace=traces[size_t(found-input.begin())];
                const double noise=1.0/IQM::HDRVDP3::evaluate_ncsf_parameters(frequency,{trace[1],trace[2],trace[3],trace[4]});
                const float neural=float(std::pow(noise,2*std::pow(10.0,.3424)));
                if(std::isnan(noise)||std::isnan(neural)) return 2;
                noise_mismatches+=noise!=std::stod(noise_text);
                neural_mismatches+=neural!=std::stof(neural_text); ++noise_cases;
            }
            if(!noise_file.eof()||!noise_cases) return 2;
            check.mismatches+=noise_mismatches+neural_mismatches;
            std::cout<<"GPU parameters to CPU noise cases="<<noise_cases<<" double_noise_mismatches="<<noise_mismatches<<" float_neural_mismatches="<<neural_mismatches<<'\n';
        } else if(mode=="--base-selection") {
            std::ifstream in(root/"base_selection.txt"); size_t n,indices=0;
            while(in>>n) {
                if(n!=128) return 2;
                std::vector<float> spectrum(128),expected(64); size_t index;
                for(auto* map:{&spectrum,&expected}) for(float& v:*map) if(!(in>>v)) return 2;
                if(!(in>>index)) return 2;
                const auto actual=gpu.base_selection(spectrum);
                check.compare(actual.magnitudes,expected);
                if(actual.index+1!=index) {++check.mismatches;std::cout<<"Selection mismatch case="<<indices<<'\n';}
                ++indices;
            }
            if(!in.eof()||!indices) return 2;
            std::cout<<"First-max indices="<<indices<<'\n';
        } else if(mode=="--base-adaptation") {
            std::ifstream in(root/"base_adaptation.txt"); size_t n;
            while(in>>n) {
                if(n!=64) return 2;
                std::vector<float> input(n),expected(2);
                for(auto* map:{&input,&expected}) for(float& v:*map) if(!(in>>v)) return 2;
                const auto actual=gpu.base_adaptation(input);
                check.compare({actual[0],actual[1]},expected);
            }
            if(!in.eof()) return 2;
        } else if(mode=="--base-fft") {
            std::ifstream in(root/"base_fft.txt"); int h,w; size_t index; double frequency; size_t frequencies=0;
            while(in>>h>>w>>index>>frequency) {
                if(h!=8||w!=8) return 2;
                std::vector<float> input(64),expected(128);
                for(auto* map:{&input,&expected}) for(float& v:*map) if(!(in>>v)) return 2;
                const auto spectrum=gpu.base_spectrum(input,(root/"base_fft.spv").string());
                check.compare(spectrum,expected);
                const auto selection=gpu.base_selection(spectrum); const size_t selected=selection.index;
                const double actual=IQM::HDRVDP3::dominant_frequency_from_index(selected,8,8,3.75);
                if(selected+1!=index||actual!=frequency) {
                    std::cout<<"base frequency case="<<frequencies<<" index="<<selected+1<<" expected="<<index<<" frequency="<<actual<<" expected="<<frequency<<'\n';
                    ++check.mismatches;
                }
                ++frequencies;
            }
            if(!in.eof()||!frequencies) return 2;
            std::cout<<"Base FFT first-max selections="<<frequencies<<'\n';
        } else if(mode=="--preparation") {
            std::ifstream in(root/"preparation.txt"); int h,w; float normalization;
            while(in>>h>>w>>normalization) {
                if(h<=0||w<=0) return 2;
                std::vector<float> reference(size_t(h)*w),test(reference.size()),difference(reference.size()),mask(reference.size());
                for(auto* map:{&reference,&test,&difference,&mask}) for(float& v:*map) if(!(in>>v)) return 2;
                check.compare(gpu.normalized_difference(reference,test,normalization),difference);
                const auto actual=gpu.comparison_mask(reference,test);
                check.compare(as_single(actual),mask);
            }
            if(!in.eof()) return 2;
            std::ifstream dc(root/"dc.txt"); size_t n;
            while(dc>>n) {
                if(n!=64) return 2; std::vector<float> input(n),expected(n);
                for(auto* map:{&input,&expected}) for(float& v:*map) if(!(dc>>v)) return 2;
                check.compare(gpu.remove_residual_dc(input),expected);
            }
            if(!dc.eof()) return 2;
        } else if(mode=="--knots") {
            IQM::HDRVDP3::JNDLUT reference_lut;
            std::ifstream lut_file(root/"lut.txt");
            for(size_t i=0;i<IQM::HDRVDP3::JND_LUT_SIZE;++i)
                if(!(lut_file>>reference_lut.log_luminance[i]>>reference_lut.cone[i]>>reference_lut.rod[i])) return 2;
            std::ifstream queries(root/"queries.txt"); std::vector<float> luminance,expected; float input_value;
            while(queries>>input_value) {
                luminance.push_back(input_value);
                for(int i=0;i<7;++i) { float value; if(!(queries>>value)) return 2; expected.push_back(value); }
            }
            if(!queries.eof()) return 2;
            for(const auto& lut:{reference_lut,IQM::HDRVDP3::create_jnd_lut()}) {
                const auto response=gpu.jnd_response(luminance,lut,1.0f); std::vector<float> actual;
                for(const auto& pixel:response) {
                    actual.insert(actual.end(),pixel.log_query.begin(),pixel.log_query.end());
                    actual.insert(actual.end(),pixel.channels.begin(),pixel.channels.end()); actual.push_back(pixel.response);
                }
                check.compare(actual,expected);
            }
        } else if(mode=="--resize") {
            std::ifstream in(root/"resize.txt"); int h,w,nh,nw;
            while(in>>h>>w>>nh>>nw) {
                if(h<=0||w<=0||nh<=0||nw<=0) return 2;
                std::vector<float> input(size_t(h)*w),expected(size_t(nh)*nw);
                for(auto& v:input) if(!(in>>v)) return 2;
                for(auto& v:expected) if(!(in>>v)) return 2;
                check.compare(gpu.resize(input,h,w,nh,nw),expected);
            }
            if(!in.eof()) return 2;
        } else if(mode=="--bounds") {
            std::ifstream in(root/"bounds.txt"); int h,w,nh,nw; double rho;
            while(in>>h>>w>>nh>>nw>>rho) {
                if(h<=0||w<=0||nh<=0||nw<=0) return 2;
                std::vector<float> input(size_t(h)*w),resized(size_t(nh)*nw),csf(resized.size());
                for(auto* matrix:{&input,&resized,&csf}) for(float& value:*matrix) if(!(in>>value)) return 2;
                const auto actual=gpu.clamp_log(gpu.resize(input,h,w,nh,nw));
                check.compare(actual,resized);
                check.compare(gpu.sample_csf(actual,IQM::HDRVDP3::create_csf_lut(rho)),csf);
            }
            if(!in.eof()) return 2;
        } else if(mode=="--pool") {
            std::ifstream in(root/"pool.txt"); int h,w; float normalization;
            while(in>>h>>w>>normalization) {
                if(h<=0||w<=0) return 2;
                std::vector<float> reference(size_t(h)*w),test(reference.size()),raw(reference.size()),pooled(reference.size());
                for(auto* matrix:{&reference,&test,&raw,&pooled}) for(float& value:*matrix) if(!(in>>value)) return 2;
                const auto actual=gpu.mutual_masking(reference,test,h,w,normalization);
                check.compare(actual.raw,raw); check.compare(actual.pooled,pooled);
            }
            if(!in.eof()) return 2;
        } else if(mode=="--csf") {
            std::ifstream in(root/"csf_lut.txt"); double age,rho;
            while(in>>age>>rho) {
                IQM::HDRVDP3::CSFLUT reference;
                for(double& v:reference) if(!(in>>v)) return 2;
                size_t n; if(!(in>>n)||!n) return 2;
                std::vector<float> queries(n),expected(n),cpu(n);
                for(size_t i=0;i<n;++i) {
                    double query,double_expected,single_expected;
                    if(!(in>>query>>double_expected>>single_expected)) return 2;
                    queries[i]=float(query); expected[i]=float(single_expected);
                }
                const auto production=IQM::HDRVDP3::create_csf_lut(rho,{age,true});
                check.compare(gpu.sample_csf(queries,reference),expected);
                check.compare(gpu.sample_csf(queries,production),expected);
                for(size_t i=0;i<n;++i) cpu[i]=IQM::HDRVDP3::sample_csf_lut_log_single(production,queries[i]);
                check.compare(cpu,expected);
            }
            if(!in.eof()) return 2;
        } else if(mode=="--maps") {
            double ppd=30;
            if(fs::exists(root/"pixels_per_degree.txt")){
                std::ifstream viewing(root/"pixels_per_degree.txt");
                if(!(viewing>>ppd)||(ppd!=30&&ppd!=60))return 2;
            }
            int band_count=5;
            if(fs::exists(root/"band_count.txt")){
                std::ifstream bands(root/"band_count.txt");
                if(!(bands>>band_count)||(band_count!=4&&band_count!=5))return 2;
            }
            int h,w; auto input=read_image(root/"csf_log_adaptation.txt",h,w);
            std::vector<std::vector<float>> adaptation;
            const auto jnd_lut=IQM::HDRVDP3::create_jnd_lut();
            std::ifstream pupil_params(root/"pupil.txt"); double rr,tr,area,age;
            if(!(pupil_params>>rr>>tr>>area>>age)) return 2;
            auto matrix=IQM::HDRVDP3::D65_AGE24_LMR;
            if(fs::exists(root/"spectral_lmr.txt")) {std::ifstream f(root/"spectral_lmr.txt");for(double& v:matrix)if(!(f>>v))return 2;}
            for(const std::string label:{"ref","test"}) {
                int ah,aw,th,tw;
                auto retinal=read_image(root/(label+"_retinal.txt"),ah,aw);
                check.compare(gpu.adaptation_input(retinal,matrix),read_image(root/(label+"_adapt_input.txt"),th,tw));
                auto map=read_image(root/(label+"_adaptation.txt"),ah,aw);
                adaptation.push_back(map);
                std::ifstream native(root/(label+"_native_pupil_trace.csv")); std::string line;
                if(!std::getline(native,line)) return 2;
                std::replace(line.begin(),line.end(),',',' '); std::istringstream row(line);
                std::vector<float> expected(4); for(float& v:expected) if(!(row>>v)) return 2;
                const auto trace=gpu.pupil_trace(map,aw,ah,area,age);
                check.compare(std::vector<float>(trace.begin(),trace.end()),expected);
                const auto response=gpu.jnd_response(retinal,jnd_lut,trace[3],matrix);
                std::ifstream native_jnd(root/(label+"_native_jnd_trace.csv"));
                std::vector<float> actual_jnd,expected_jnd;
                for(int x=0;x<aw;++x) for(int y=0;y<ah;++y) {
                    if(!std::getline(native_jnd,line)) return 2;
                    std::replace(line.begin(),line.end(),',',' '); std::istringstream native_row(line);
                    for(int k=0;k<13;++k) {float value; if(!(native_row>>value)) return 2; expected_jnd.push_back(value);}
                    const auto& pixel=response[size_t(y)*aw+x];
                    for(const auto* stage:{&pixel.lmr_before,&pixel.lmr_after,&pixel.log_query,&pixel.channels})
                        actual_jnd.insert(actual_jnd.end(),stage->begin(),stage->end());
                    actual_jnd.push_back(pixel.response);
                }
                check.compare(actual_jnd,expected_jnd);
            }
            std::vector<float> mean(adaptation[0].size());
            for(size_t i=0;i<mean.size();++i) mean[i]=.5f*(adaptation[0][i]+adaptation[1][i]);
            check.compare(gpu.mean_adaptation(adaptation[0],adaptation[1]),mean);
            check.compare(gpu.log_adaptation(mean),input);
            int mh,mw,th,tw;
            const auto reference_image=read_image(root/"ref_luminance.txt",mh,mw);
            const auto test_image=read_image(root/"test_luminance.txt",th,tw);
            if(mh!=th||mw!=tw) return 2;
            std::vector<double> comparison_mask(reference_image.size());
            for(size_t i=0;i<comparison_mask.size();++i)
                comparison_mask[i]=(std::abs(test_image[i]-reference_image[i])/reference_image[i]>.001f)?1.0:0.0;
            const auto gpu_mask=gpu.comparison_mask(reference_image,test_image);
            check.compare(as_single(gpu_mask),as_single(comparison_mask));
            comparison_mask=gpu_mask;
            std::ifstream scores_file(root/"quality.txt"); std::vector<float> expected_scores(band_count+3),band_errors;
            for(float& value:expected_scores) if(!(scores_file>>value)) return 2;
            for(int b=0;b<band_count;++b) {
                int nh,nw,sh,sw;
                auto expected=read_image(root/("csf_log_band"+std::to_string(b)+".txt"),nh,nw);
                const auto resized=gpu.clamp_log(gpu.resize(input,h,w,nh,nw)); check.compare(resized,expected);
                auto sensitivity=read_image(root/("csf_band"+std::to_string(b)+".txt"),sh,sw);
                if(sh!=nh||sw!=nw) return 2;
                const auto lut=IQM::HDRVDP3::create_csf_lut(ppd/std::pow(2.0,b+1),{age,true});
                check.compare(gpu.sample_csf(resized,lut),sensitivity);
                auto mask=read_image(root/("mask_pool_band"+std::to_string(b)+".txt"),sh,sw);
                int rh,rw,th,tw;
                auto reference=read_image(root/("gpu_ref_band"+std::to_string(b)+".txt"),rh,rw);
                auto test=read_image(root/("gpu_test_band"+std::to_string(b)+".txt"),th,tw);
                if(rh!=th||rw!=tw) return 2;
                if(b==band_count-1) {
                    const auto gpu_reference=gpu.remove_residual_dc(reference),gpu_test=gpu.remove_residual_dc(test);
                    double rsum=0,tsum=0;
                    for(float v:reference) rsum+=v;
                    for(float v:test) tsum+=v;
                    const float rmean=float(rsum/reference.size()),tmean=float(tsum/test.size());
                    for(float& v:reference) v-=rmean;
                    for(float& v:test) v-=tmean;
                    check.compare(gpu_reference,reference); check.compare(gpu_test,test);
                    reference=gpu_reference; test=gpu_test;
                }
                const auto masking=gpu.mutual_masking(reference,test,rh,rw,std::ldexp(1.0f,b));
                check.compare(masking.raw,read_image(root/("mask_raw_band"+std::to_string(b)+".txt"),th,tw));
                check.compare(masking.pooled,mask);
                std::vector<float> previous(mask.size(),0.0f),next(mask.size(),0.0f);
                if(b>0) {
                    auto map=read_image(root/("mask_pool_band"+std::to_string(b-1)+".txt"),th,tw);
                    previous=gpu.resize(map,th,tw,rh,rw);
                }
                if(b+2<band_count) {
                    auto map=read_image(root/("mask_pool_band"+std::to_string(b+1)+".txt"),th,tw);
                    next=gpu.resize(map,th,tw,rh,rw);
                }
                const auto neighbor=gpu.neighbor_mask(previous,next);
                check.compare(neighbor,read_image(root/("mask_neighbor_band"+std::to_string(b)+".txt"),th,tw));
                const auto denominator=gpu.masking_denominator(masking.pooled,neighbor,float(std::pow(10.0,.108934275615)),float(std::pow(10.0,1.41846291721)),float(std::pow(10.0,.136877512403)));
                check.compare(denominator,read_image(root/("mask_denominator_band"+std::to_string(b)+".txt"),th,tw));
                const double p=std::pow(10.0,.3424),pf=std::pow(10.0,.34)/p;
                float neural=-1.0f;
                if(b==band_count-1) {
                    const auto logs=read_image(root/("csf_log_band"+std::to_string(b)+".txt"),th,tw);
                    const auto adaptation=gpu.base_adaptation(logs);
                    std::ifstream native_adaptation(root/"base_native_adaptation.txt");
                    std::vector<float> expected_adaptation(2);
                    for(float& v:expected_adaptation) if(!(native_adaptation>>v)) return 2;
                    check.compare({adaptation[0],adaptation[1]},expected_adaptation);
                    std::ifstream csf_metadata(root/"base_csf.txt"); double base_frequency; float base_luminance;
                    if(!(csf_metadata>>base_frequency>>base_luminance)) return 2;
                    const auto parameters=gpu.base_csf_parameters({base_luminance})[0];
                    std::vector<float> native_parameters(5),exported_parameters(5);
                    std::ifstream native_file(root/"base_native_csf_parameters.txt"),pipeline_file(root/"base_csf_parameters.txt");
                    for(float& v:native_parameters) if(!(native_file>>v)) return 2;
                    for(float& v:exported_parameters) if(!(pipeline_file>>v)) return 2;
                    check.compare({parameters[0],parameters[1],parameters[2],parameters[3],parameters[4]},native_parameters);
                    if(exported_parameters!=native_parameters) throw std::runtime_error("Pipeline base CSF parameter mismatch");
                    std::ifstream metadata(root/"base_csf.txt"); double frequency,luminance; std::string noise_text;
                    if(!(metadata>>frequency>>luminance>>noise_text)) return 2;
                    const double noise=std::stod(noise_text);
                    neural=float(std::pow(noise,2*p));
                }
                std::vector<float> difference(reference.size());
                const float normalization=std::ldexp(1.0f,b);
                for(size_t i=0;i<difference.size();++i) difference[i]=test[i]/normalization-reference[i]/normalization;
                const auto prepared=gpu.normalized_difference(reference,test,normalization);
                check.compare(prepared,difference); difference=prepared;
                if(b==band_count-1&&fs::exists(root/"base_frequency_backend.txt")) {
                    std::ifstream backend_file(root/"base_frequency_backend.txt"); std::string backend; backend_file>>backend;
                    if(backend=="vulkan_fft") {
                        std::ifstream path_file(root/"base_fft_shader.txt"); std::string shader_path; std::getline(path_file,shader_path);
                        const auto spectrum=gpu.base_spectrum(difference,shader_path);
                        check.compare(spectrum,read_image(root/"base_native_spectrum.txt",th,tw));
                        std::ifstream selection_file(root/"base_native_selection.txt"); size_t expected_index; double expected_frequency;
                        if(!(selection_file>>expected_index>>expected_frequency)) return 2;
                        const auto selection=gpu.base_selection(spectrum); const size_t index=selection.index;
                        check.compare(selection.magnitudes,read_image(root/"base_native_magnitudes.txt",th,tw));
                        std::ifstream exported_index(root/"base_gpu_selection.txt"); size_t pipeline_index;
                        if(!(exported_index>>pipeline_index)||pipeline_index!=expected_index) throw std::runtime_error("Pipeline base selection mismatch");
                        const double frequency=IQM::HDRVDP3::dominant_frequency_from_index(index,rw,rh,std::ldexp(4*ppd,-band_count));
                        if(index+1!=expected_index||frequency!=expected_frequency) throw std::runtime_error("Native base-frequency selection mismatch");
                    }
                }
                const auto raw=gpu.difference(difference,sensitivity,denominator,float(p),neural);
                check.compare(raw,read_image(root/("quality_difference_band"+std::to_string(b)+".txt"),th,tw));
                check.compare(gpu.psychometric(raw,float(pf)),read_image(root/("difference_band"+std::to_string(b)+".txt"),th,tw));
                const auto quality_mask=gpu.resize_comparison_mask(comparison_mask,mh,mw,rh,rw);
                const auto native_mask=read_double_image(root/("quality_mask_native_band"+std::to_string(b)+".txt"));
                check.compare_double(quality_mask,native_mask);
                const auto pipeline_mask=read_double_image(root/("quality_mask_band"+std::to_string(b)+".txt"));
                if(pipeline_mask!=native_mask) throw std::runtime_error("Pipeline double mask mismatch");
                const float error=gpu.quality_band_error(raw,quality_mask,rw,rh);
                check.compare({error},{expected_scores[b]}); band_errors.push_back(error);
                for(int neighbor:{b-1,b+1}) if(neighbor>=0&&neighbor<band_count) {
                    int th,tw; read_image(root/("mask_pool_band"+std::to_string(neighbor)+".txt"),th,tw);
                    check.compare(gpu.resize(mask,sh,sw,th,tw),IQM::HDRVDP3::resize_bicubic(mask,sh,sw,th,tw));
                }
            }
            const auto scores=gpu.quality_scores(band_errors);
            check.compare({scores.error,scores.Q,scores.Q_JOD},{expected_scores[band_count],expected_scores[band_count+1],expected_scores[band_count+2]});
        } else return 2;
        std::cout<<"cases="<<check.cases<<" pixels="<<check.pixels<<" float_mismatches="<<check.mismatches<<" max="<<check.maximum<<'\n';
        if(check.double_cases) std::cout<<"double_cases="<<check.double_cases<<" double_values="<<check.double_pixels<<" double_bit_mismatches="<<check.double_mismatches<<" double_max="<<check.double_maximum<<'\n';
        return !(check.cases+check.double_cases)?2:(check.mismatches+check.double_mismatches)?1:0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 2; }
}
