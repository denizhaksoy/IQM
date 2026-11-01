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
    const RGBSpectralSamples& spectral,const FFTBackendSingle& fft) {
    if(w<16||h<16||w>unsigned(std::numeric_limits<int>::max()/2)||h>unsigned(std::numeric_limits<int>::max()/2)||
       size_t(w)>std::vector<float>().max_size()/size_t(h)/16||linear.size()!=3*size_t(w)*h||
       !std::isfinite(ppd)||ppd<26||ppd>256||!fft.convolve)
        throw std::invalid_argument("Invalid general Single front-end shape/viewing/backend");
    QualitySideSingle result;
    result.native_rgb=prepare_linear_color_single(linear,xyz,LinearColorEncoding::BT2020);
    auto optical=filter_optical_rgb_single(result.native_rgb,w,h,ppd,fft);
    auto lmsr=project_optical_rgb_single(optical,spectral,24.);
    optical.clear();optical.shrink_to_fit();
    result.adaptation=local_adaptation_single(lmsr.adaptation_input,w,h,ppd,fft);
    auto jnd=rgb_to_jnd_response_single(create_jnd_lut(),lmsr.clamped,result.adaptation,w,h,double(size_t(w)*h)/(ppd*ppd),24.);
    lmsr={};jnd.channels.clear();jnd.channels.shrink_to_fit();
    result.pyramid=build_sp0_pyramid_single(jnd.response,w,h,ppd);
    return result;
}
GeneralQualityResultSingle evaluate_general_quality_single(
    std::span<const float> reference,std::span<const float> test,unsigned w,unsigned h,double ppd,
    const XYZDisplaySamples& xyz,const RGBSpectralSamples& spectral,const FFTBackendSingle& fft) {
    if(!fft.forward)throw std::invalid_argument("Explicit Single forward FFT required");
    auto r=prepare_quality_side_single(reference,w,h,ppd,xyz,spectral,fft);
    auto t=prepare_quality_side_single(test,w,h,ppd,xyz,spectral,fft);
    return evaluate_quality_single(r,t,w,h,ppd,fft);
}
}
