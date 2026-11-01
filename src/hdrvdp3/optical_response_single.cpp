#include <IQM/hdrvdp3/optical_response_single.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace IQM::HDRVDP3 {
RGBOpticalResponseSingle project_optical_rgb_single(std::span<const float> optical,
    const RGBSpectralSamples& display,double age) {
    if(optical.empty()||optical.size()%3||!std::isfinite(age)||age<0||age>90)
        throw std::invalid_argument("Invalid Single optical shape or age");
    for(float x:optical)if(!std::isfinite(x)||x<0)
        throw std::invalid_argument("Optical Single input must be finite nonnegative");
    const auto calibration=calibrate_rgb_spectral_age(display,age);
    const size_t n=optical.size()/3;RGBOpticalResponseSingle result;
    result.clamped.resize(4*n);result.adaptation_input.resize(n);result.adaptation_log.resize(n);
    for(size_t c=0;c<4;++c)for(size_t i=0;i<n;++i){
        const float first=optical[i]*float(calibration.matrix[0][c]);
        const float second=std::fma(optical[n+i],float(calibration.matrix[1][c]),first);
        result.clamped[c*n+i]=std::clamp(std::fma(optical[2*n+i],float(calibration.matrix[2][c]),second),1e-8f,1e10f);
    }
    for(size_t i=0;i<n;++i){
        result.adaptation_input[i]=result.clamped[i]+result.clamped[n+i];
        result.adaptation_log[i]=std::log(std::max(result.adaptation_input[i],1e-6f));
    }
    return result;
}
}
