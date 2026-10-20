#include <IQM/hdrvdp3/quality.h>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
namespace IQM::HDRVDP3 {
double dominant_frequency_from_index(std::size_t selected,unsigned width,unsigned height,double ppd) {
    if(!width||!height||selected>=size_t(width)*height) throw std::invalid_argument("Invalid base frequency index");
    const unsigned kx=unsigned(selected/height),ky=unsigned(selected%height);
    const double fx=(kx<(width+1)/2?double(kx):double(kx)-width)/width*ppd;
    const double fy=(ky<(height+1)/2?double(ky):double(ky)-height)/height*ppd;
    return std::sqrt(fx*fx+fy*fy);
}
double dominant_frequency_from_spectrum(const std::vector<float>& interleaved,unsigned width,unsigned height,double ppd,std::size_t* selected_index) {
    if(!width||!height||interleaved.size()!=2*size_t(width)*height)
        throw std::invalid_argument("Invalid base spectrum");
    float maximum=0.0f; size_t selected=0;
    // Native abs(complex single) rounds hypot to single before finding the
    // first maximum in column-major order. DC is ignored, including allzero.
    for(unsigned kx=0;kx<width;++kx) for(unsigned ky=0;ky<height;++ky) {
        const size_t row=size_t(ky)*width+kx;
        if(!std::isfinite(interleaved[2*row])||!std::isfinite(interleaved[2*row+1]))
            throw std::invalid_argument("Nonfinite base spectrum");
        if(kx==0&&ky==0) continue;
        const float magnitude=std::hypot(interleaved[2*row],interleaved[2*row+1]);
        if(magnitude>maximum) {maximum=magnitude; selected=size_t(kx)*height+ky;}
    }
    if(selected_index) *selected_index=selected;
    return dominant_frequency_from_index(selected,width,height,ppd);
}
double dominant_frequency(const std::vector<float>& difference,unsigned width,unsigned height,double ppd) {
    if(!width||!height||difference.size()!=size_t(width)*height) throw std::invalid_argument("Invalid base band");
    // Direct double DFT selects a frequency only. Native Octave single FFT
    // spectra and near-tie selection must be validated separately.
    double maximum=0.0,frequency=0.0;
    for(unsigned kx=0;kx<width;++kx) for(unsigned ky=0;ky<height;++ky) {
        if(kx==0&&ky==0) continue;
        double real=0,imag=0;
        for(unsigned x=0;x<width;++x) for(unsigned y=0;y<height;++y) {
            const double angle=-2*std::numbers::pi*(double(kx)*x/width+double(ky)*y/height);
            const double value=difference[size_t(y)*width+x];
            real+=value*std::cos(angle); imag+=value*std::sin(angle);
        }
        const double magnitude=std::hypot(real,imag);
        if(magnitude>maximum) {
            maximum=magnitude;
            const double fx=(kx<(width+1)/2?double(kx):double(kx)-width)/width*ppd;
            const double fy=(ky<(height+1)/2?double(ky):double(ky)-height)/height*ppd;
            frequency=std::sqrt(fx*fx+fy*fy);
        }
    }
    return frequency;
}
float masked_difference(float difference,float sensitivity,float mask,double p,double pf,double base_noise,float* raw_difference) {
    const float excitation=std::copysign(std::pow(std::abs(difference),static_cast<float>(p)),difference);
    float neural;
    if(base_noise>=0) neural=static_cast<float>(std::pow(base_noise,2*p));
    else neural=std::pow(1.0f/sensitivity,static_cast<float>(2*p));
    // Octave optimizes element-wise power2 as multiplication; native powf(x,2)
    // can round differently even with strict compiler floating-point settings.
    const float mask_term=mask*mask;
    const float denominator=std::sqrt(neural+mask_term);
    const float d=excitation/denominator;
    if(raw_difference) *raw_difference=d;
    return std::copysign(std::pow(std::abs(d),static_cast<float>(pf)),d);
}
float quality_band_error(const std::vector<float>& difference,const std::vector<double>& comparison_mask,unsigned width,unsigned height) {
    if(!width||!height||difference.size()!=size_t(width)*height||comparison_mask.size()!=difference.size())
        throw std::invalid_argument("Invalid quality band");
    float sum=0;
    for(unsigned x=0;x<width;++x) for(unsigned y=0;y<height;++y) {
        const size_t i=size_t(y)*width+x;
        const float masked=difference[i]*static_cast<float>(comparison_mask[i]);
        const float power=std::pow(std::abs(masked),0.8f);
        sum+=power/static_cast<float>(difference.size());
    }
    return std::pow(sum,static_cast<float>(1.0/0.8));
}
QualityScores quality_scores(const std::vector<float>& band_errors) {
    if(band_errors.empty()) throw std::invalid_argument("Empty quality bands");
    float error=0;
    for(float band:band_errors) error+=band/static_cast<float>(band_errors.size());
    const float Q=6.0f-std::log(static_cast<float>(std::exp(-4.0))+error);
    const float loss=std::pow(std::max(10.0f-Q,0.0f),static_cast<float>(1.2812));
    const float product=static_cast<float>(0.5200)*loss;
    return {error,Q,10.0f-product};
}
}
