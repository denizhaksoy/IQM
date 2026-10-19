#include <IQM/hdrvdp3/spectral.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace IQM::HDRVDP3 {
namespace {
int sign_product(double a,double b) {
    return (a==0||b==0)?0:((std::signbit(a)==std::signbit(b))?1:-1);
}
// Shape-preserving derivatives follow the SLATEC DPCHIM operation order.
// Algorithm source: https://www.netlib.org/slatec/pchip/dpchim.f
std::array<double,26> derivatives(const std::array<double,26>& y) {
    std::array<double,26> d{};
    const double h=10,hsum=h+h;
    double a=(y[1]-y[0])/h,b=(y[2]-y[1])/h;
    d[0]=((h+hsum)/hsum)*a+(-h/hsum)*b;
    if(sign_product(d[0],a)<=0)d[0]=0;
    else if(sign_product(a,b)<0&&std::abs(d[0])>std::abs(3*a))d[0]=3*a;
    for(size_t i=1;i<25;++i) {
        a=(y[i]-y[i-1])/h;b=(y[i+1]-y[i])/h;
        if(sign_product(a,b)>0) {
            const double w=(hsum+h)/(hsum+hsum+hsum);
            const double largest=std::max(std::abs(a),std::abs(b));
            const double smallest=std::min(std::abs(a),std::abs(b));
            d[i]=smallest/(w*(a/largest)+w*(b/largest));
        }
    }
    d[25]=(-h/hsum)*a+((h+hsum)/hsum)*b;
    if(sign_product(d[25],b)<=0)d[25]=0;
    else if(sign_product(a,b)<0&&std::abs(d[25])>std::abs(3*b))d[25]=3*b;
    return d;
}
}
SpectralCalibration calibrate_spectral_age(const SpectralSamples& s,double age) {
    const size_t n=s.wavelength.size();
    if(n<2||s.emission.size()!=n||s.sensitivity.size()!=n||!std::isfinite(age))
        throw std::invalid_argument("Invalid spectral calibration input");
    for(size_t i=0;i<n;++i) {
        if(!std::isfinite(s.wavelength[i])||!std::isfinite(s.emission[i])||s.emission[i]<0||
           (i&&s.wavelength[i]<=s.wavelength[i-1]))throw std::invalid_argument("Invalid spectral samples");
        for(double value:s.sensitivity[i])if(!std::isfinite(value)||value<0)
            throw std::invalid_argument("Invalid spectral sensitivity");
    }
    constexpr std::array<double,26> tl1{.600,.510,.433,.377,.327,.295,.267,.233,.207,.187,.167,.147,.133,.120,.107,.093,.080,.067,.053,.040,.033,.027,.020,.013,.007,0};
    constexpr std::array<double,26> tl2{1,.583,.300,.116,.033,.005};
    SpectralCalibration out;
    const double factor=age<=60?1+.02*(age-32):1.56+.0667*(age-60);
    for(size_t i=0;i<26;++i)out.optical_density_delta[i]=(tl1[i]*factor+tl2[i])-(tl1[i]+tl2[i]);
    const auto d=derivatives(out.optical_density_delta);
    for(double lambda:s.wavelength) {
        const double query=std::clamp(lambda,400.0,650.0);
        const size_t i=std::min(size_t((query-400)/10),size_t(24));
        const double delta=(out.optical_density_delta[i+1]-out.optical_density_delta[i])/10;
        const double del1=(d[i]-delta)/10,del2=(d[i+1]-delta)/10;
        const double c3=del1+del2,c2=-c3-del1;
        const double offset=query-(400+10*i);
        const double density=(((c3/10)*offset+c2)*offset+d[i])*offset+out.optical_density_delta[i];
        out.interpolated_density.push_back(density);
        out.transmission.push_back(std::pow(10.0,-density));
    }
    for(size_t i=0;i<n;++i)out.filtered_emission.push_back(s.emission[i]*out.transmission[i]);
    for(size_t ch=0;ch<4;++ch) {
        double sum=0;
        for(size_t i=1;i<n;++i) {
            const double left=(s.sensitivity[i-1][ch]*out.filtered_emission[i-1])*683.002;
            const double right=(s.sensitivity[i][ch]*out.filtered_emission[i])*683.002;
            sum+=(s.wavelength[i]-s.wavelength[i-1])*(right+left);
        }
        out.integrals[ch]=.5*sum;
    }
    const double norm=out.integrals[0]+out.integrals[1];
    if(!(norm>0)||!std::isfinite(norm))throw std::invalid_argument("Invalid spectral normalization");
    for(size_t ch=0;ch<4;++ch)out.matrix[ch]=out.integrals[ch]/norm;
    return out;
}
}
