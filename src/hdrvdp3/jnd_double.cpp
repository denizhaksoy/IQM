#include <IQM/hdrvdp3/jnd.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace IQM::HDRVDP3 {
RGBJNDResponseDouble rgb_to_jnd_response_double(const JNDLUT& lut,
    std::span<const double> lmsr,std::span<const double> adaptation,
    unsigned width,unsigned height,double area,double age) {
    const size_t n=size_t(width)*height;
    if(!width||!height||adaptation.size()!=n||lmsr.size()%4||lmsr.size()/4!=n||
       !std::isfinite(area)||area<=0||!std::isfinite(age))
        throw std::invalid_argument("Invalid Double JND dimensions/viewing parameters");
    for(size_t i=0;i<JND_LUT_SIZE;++i)if(!std::isfinite(lut.log_luminance[i])||
       !std::isfinite(lut.cone[i])||!std::isfinite(lut.rod[i])||
       (i&&lut.log_luminance[i]<=lut.log_luminance[i-1]))
        throw std::invalid_argument("Invalid Double JND table");
    for(double v:lmsr)if(!std::isfinite(v)||v<=0)throw std::invalid_argument("Invalid Double LMSR");
    for(double v:adaptation)if(!std::isfinite(v)||v<=0)throw std::invalid_argument("Invalid Double adaptation");
    double sum=0;for(unsigned x=0;x<width;++x)for(unsigned y=0;y<height;++y)sum+=std::log(adaptation[size_t(y)*width+x]);
    RGBJNDResponseDouble out;out.geometric_mean=std::exp(sum/double(n));
    out.pupil_reduction=pupil_luminance_reduction(out.geometric_mean,area,age);
    out.post_pupil.resize(4*n);out.channels.resize(4*n);out.response.resize(n);
    for(size_t i=0;i<4*n;++i)out.post_pupil[i]=lmsr[i]*out.pupil_reduction;
    const double low=std::pow(10.,lut.log_luminance.front()),high=std::pow(10.,lut.log_luminance.back());
    for(size_t c=0;c<3;++c)for(size_t i=0;i<n;++i){
        const double query=std::log10(std::clamp(out.post_pupil[(c==2?3:c)*n+i],low,high));
        auto it=std::upper_bound(lut.log_luminance.begin(),lut.log_luminance.end(),query);
        const size_t k=std::clamp<size_t>(it-lut.log_luminance.begin(),1,JND_LUT_SIZE-1)-1;
        const auto& table=c==2?lut.rod:lut.cone;
        const double slope=(table[k+1]-table[k])/(lut.log_luminance[k+1]-lut.log_luminance[k]);
        const double product=slope*(query-lut.log_luminance[k]);
        out.channels[c*n+i]=product+table[k];
    }
    for(size_t i=0;i<n;++i)out.response[i]=(out.channels[i]+out.channels[n+i])+out.channels[2*n+i];
    return out;
}
}
