#include <IQM/hdrvdp3/general_quality_single.h>
#include <IQM/hdrvdp3/linear_color_single.h>
#include <IQM/hdrvdp3/optical_single.h>
#include <IQM/hdrvdp3/optical_response_single.h>
#include <IQM/hdrvdp3/jnd.h>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace IQM::HDRVDP3 {
QualitySideSingle prepare_quality_side_single(std::span<const float> linear,
    unsigned w,unsigned h,double ppd,const XYZDisplaySamples& xyz,
    const RGBSpectralSamples& spectral,const FFTBackendSingle& fft,const SingleMatrixBackend* matrices,const SingleAdaptationInputBackend* adaptationInput,const SinglePaddingBackend* padding) {
    if(w<16||h<16||w>unsigned(std::numeric_limits<int>::max()/2)||h>unsigned(std::numeric_limits<int>::max()/2)||
       size_t(w)>std::vector<float>().max_size()/size_t(h)/16||linear.size()!=3*size_t(w)*h||
       !std::isfinite(ppd)||ppd<26||ppd>256||!fft.convolve)
        throw std::invalid_argument("Invalid general Single front-end shape/viewing/backend");
    if(adaptationInput&&(!matrices||!adaptationInput->sum))throw std::invalid_argument("Adaptation input provider requires explicit matrices and sum");
    QualitySideSingle result;
    if(matrices){
        if(!matrices->project)throw std::invalid_argument("Explicit matrix provider is empty");
        for(float v:linear)if(!std::isfinite(v)||v<0)throw std::invalid_argument("Invalid Single linear RGB");
        const auto color=linear_color_transform(xyz,LinearColorEncoding::BT2020);
        std::vector<float> coefficients;for(unsigned c=0;c<3;++c)for(unsigned p=0;p<3;++p)coefficients.push_back(float(color[c][p]));
        result.native_rgb=matrices->project(linear,coefficients,SingleProjectionKind::NativeColor);
        if(result.native_rgb.size()!=linear.size())throw std::runtime_error("Native matrix provider shape mismatch");
        for(float v:result.native_rgb)if(!std::isfinite(v)||v<1e-6f)throw std::runtime_error("Native matrix provider output invalid");
    }else result.native_rgb=prepare_linear_color_single(linear,xyz,LinearColorEncoding::BT2020);
    auto optical=filter_optical_rgb_single(result.native_rgb,w,h,ppd,fft,padding);
    RGBOpticalResponseSingle lmsr;
    if(matrices){
        const auto model=calibrate_rgb_spectral_age(spectral,24.);std::vector<float> coefficients;
        for(unsigned c=0;c<4;++c)for(unsigned p=0;p<3;++p)coefficients.push_back(float(model.matrix[p][c]));
        lmsr.clamped=matrices->project(optical,coefficients,SingleProjectionKind::LMSR);
        const size_t n=size_t(w)*h;if(lmsr.clamped.size()!=4*n)throw std::runtime_error("LMSR matrix provider shape mismatch");
        for(float v:lmsr.clamped)if(!std::isfinite(v)||v<1e-8f||v>1e10f)throw std::runtime_error("LMSR matrix provider output invalid");
        if(adaptationInput){
            lmsr.adaptation_input=adaptationInput->sum(std::span<const float>(lmsr.clamped).first(n),std::span<const float>(lmsr.clamped).subspan(n,n));
            if(lmsr.adaptation_input.size()!=n)throw std::runtime_error("Adaptation input provider shape mismatch");
            for(float v:lmsr.adaptation_input)if(!std::isfinite(v)||v<2e-8f||v>2e10f)throw std::runtime_error("Adaptation input provider domain");
        }else {lmsr.adaptation_input.resize(n);for(size_t i=0;i<n;++i)lmsr.adaptation_input[i]=lmsr.clamped[i]+lmsr.clamped[n+i];}
    }else lmsr=project_optical_rgb_single(optical,spectral,24.);
    optical.clear();optical.shrink_to_fit();
    result.adaptation=local_adaptation_single(lmsr.adaptation_input,w,h,ppd,fft,padding);
    auto jnd=rgb_to_jnd_response_single(create_jnd_lut(),lmsr.clamped,result.adaptation,w,h,double(size_t(w)*h)/(ppd*ppd),24.);
    lmsr={};jnd.channels.clear();jnd.channels.shrink_to_fit();
    result.pyramid=build_sp0_pyramid_single(jnd.response,w,h,ppd);
    return result;
}
GeneralQualityResultSingle evaluate_general_quality_single(
    std::span<const float> reference,std::span<const float> test,unsigned w,unsigned h,double ppd,
    const XYZDisplaySamples& xyz,const RGBSpectralSamples& spectral,const FFTBackendSingle& fft,const SingleMatrixBackend* matrices,const SingleComparisonBackend* comparison,const SingleMutualBackend* mutual,const SingleComparisonResizeBackend* resize,const SingleAdaptationInputBackend* adaptationInput,const SinglePaddingBackend* padding) {
    if(!fft.forward)throw std::invalid_argument("Explicit Single forward FFT required");
    auto r=prepare_quality_side_single(reference,w,h,ppd,xyz,spectral,fft,matrices,adaptationInput,padding);
    auto t=prepare_quality_side_single(test,w,h,ppd,xyz,spectral,fft,matrices,adaptationInput,padding);
    return evaluate_quality_single(r,t,w,h,ppd,fft,comparison,mutual,resize);
}
}
