#include <IQM/hdrvdp3/srgb_display.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace IQM::HDRVDP3 {
SRGBDisplayTrace prepare_srgb_display(std::span<const float> encoded,
    const XYZDisplaySamples& display) {
    if(encoded.empty()||encoded.size()%3)
        throw std::invalid_argument("sRGB display requires three nonempty planar channels");
    for(float value:encoded)if(!std::isfinite(value)||value<0||value>1)
        throw std::invalid_argument("sRGB display requires finite encoded values in [0,1]");
    const auto matrix=rgb709_transform(display);
    SRGBDisplayTrace result;
    result.display_rgb.resize(encoded.size());
    result.raw_native_rgb.resize(encoded.size());
    result.native_rgb.resize(encoded.size());
    for(size_t i=0;i<encoded.size();++i) {
        const float value=encoded[i];
        // Native Single expressions are assigned into a Double zeros array.
        // Keep every operation before that assignment in binary32.
        float linear;
        if(value<=0.04045)linear=value/float(12.92);
        else {
            const float base=(value+float(.055))/float(1.055);
            linear=std::pow(base,float(2.4));
        }
        result.display_rgb[i]=99.0*double(linear)+1.0;
    }
    const size_t pixels=encoded.size()/3;
    for(size_t c=0;c<3;++c)for(size_t i=0;i<pixels;++i) {
        const double first=result.display_rgb[i]*matrix[c][0];
        const double value=std::fma(result.display_rgb[2*pixels+i],matrix[c][2],
            std::fma(result.display_rgb[pixels+i],matrix[c][1],first));
        result.raw_native_rgb[c*pixels+i]=value;
        result.native_rgb[c*pixels+i]=std::max(value,1e-6);
    }
    return result;
}
RGBOpticalResponseDouble project_optical_rgb_double(std::span<const double> optical,
    const RGBSpectralSamples& display, double age) {
    if(optical.empty()||optical.size()%3)
        throw std::invalid_argument("Optical RGB requires three nonempty planar channels");
    for(double value:optical)if(!std::isfinite(value)||value<0)
        throw std::invalid_argument("Optical RGB requires finite nonnegative values");
    const auto calibration=calibrate_rgb_spectral_age(display,age);
    const size_t pixels=optical.size()/3;
    RGBOpticalResponseDouble result;
    result.response.resize(4*pixels);result.clamped.resize(4*pixels);
    result.adaptation_input.resize(pixels);result.adaptation_log.resize(pixels);
    for(size_t c=0;c<4;++c)for(size_t i=0;i<pixels;++i) {
        const double first=optical[i]*calibration.matrix[0][c];
        const double value=std::fma(optical[2*pixels+i],calibration.matrix[2][c],
            std::fma(optical[pixels+i],calibration.matrix[1][c],first));
        result.response[c*pixels+i]=value;
        result.clamped[c*pixels+i]=std::clamp(value,1e-8,1e10);
    }
    for(size_t i=0;i<pixels;++i) {
        const double value=result.clamped[i]+result.clamped[pixels+i];
        result.adaptation_input[i]=value;
        result.adaptation_log[i]=std::log(std::max(value,1e-6));
    }
    return result;
}
std::vector<double> finish_local_adaptation_double(std::span<const double> filtered_log) {
    if(filtered_log.empty())throw std::invalid_argument("Filtered adaptation cannot be empty");
    std::vector<double> result;result.reserve(filtered_log.size());
    for(double value:filtered_log) {
        if(!std::isfinite(value))throw std::invalid_argument("Filtered adaptation must be finite");
        const double adapted=std::exp(value);
        if(!std::isfinite(adapted)||adapted<=0)throw std::invalid_argument("Adaptation out of finite positive range");
        result.push_back(adapted);
    }
    return result;
}
}
