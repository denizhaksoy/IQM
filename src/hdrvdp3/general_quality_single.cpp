#include <IQM/hdrvdp3/general_quality_single.h>
#include <IQM/hdrvdp3/resize.h>
#include <IQM/hdrvdp3/csf.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <limits>
namespace IQM::HDRVDP3 {namespace {
std::vector<float> mutual(std::span<const float> a,std::span<const float> b,unsigned w,unsigned h){
 bool trans=w>h;unsigned cw=trans?h:w,ch=trans?w:h;std::vector<float> activity(size_t(cw)*ch),out(activity.size());
 for(unsigned y=0;y<ch;++y)for(unsigned x=0;x<cw;++x){size_t original=trans?size_t(x)*w+y:size_t(y)*w+x;activity[size_t(y)*cw+x]=std::min(std::abs(a[original]),std::abs(b[original]));}
 for(unsigned y=0;y<ch;++y)for(unsigned x=0;x<cw;++x){float acc=0;for(int fx=0;fx<3;++fx)for(int fy=2;fy>=0;--fy){int sx=int(x)+fx-1,sy=int(y)+fy-1;float value=sx<0||sy<0||sx>=int(cw)||sy>=int(ch)?0.f:activity[size_t(sy)*cw+sx];if(sy>=0&&sy<int((ch/32)*32))acc=std::fma(value,float(1./9.),acc);else{float product=value*float(1./9.);acc+=product;}}
 size_t original=trans?size_t(x)*w+y:size_t(y)*w+x;out[original]=acc;}return out;
}
float mean(std::span<const float> v,unsigned w,unsigned h){double sum=0;for(unsigned x=0;x<w;++x)for(unsigned y=0;y<h;++y)sum+=v[size_t(y)*w+x];return float(sum/double(v.size()));}
}
GeneralQualityResultSingle evaluate_quality_single(const QualitySideSingle& r,const QualitySideSingle& t,unsigned w,unsigned h,double ppd,const FFTBackendSingle& fft){
 if(w<16||h<16||w>unsigned(std::numeric_limits<int>::max()/2)||h>unsigned(std::numeric_limits<int>::max()/2)||size_t(w)>std::vector<double>().max_size()/size_t(h)/16)throw std::invalid_argument("Single quality geometry overflow/domain");
 const size_t n=size_t(w)*h,count=r.pyramid.bands.size();if(!std::isfinite(ppd)||ppd<26||ppd>256||!fft.forward||count<2||t.pyramid.bands.size()!=count||r.native_rgb.size()!=3*n||t.native_rgb.size()!=3*n||r.adaptation.size()!=n||t.adaptation.size()!=n)throw std::invalid_argument("Single quality input geometry/provider");
 for(auto side:{&r,&t}){for(float v:side->native_rgb)if(!std::isfinite(v)||v<=0)throw std::invalid_argument("Invalid Single native RGB");for(float v:side->adaptation)if(!std::isfinite(v)||v<=0)throw std::invalid_argument("Invalid Single adaptation");}
 std::vector<std::vector<float>> self;
 unsigned levels=0;for(unsigned a=w,b=h;a>=13&&b>=13;a/=2,b/=2)++levels;
 if(count!=std::min(unsigned(std::ceil(std::log2(ppd))-2),levels)+2)throw std::invalid_argument("Single pyramid band count");
 unsigned bwExpected=w,bhExpected=h;
 for(size_t b=0;b<count;++b){if(b>1){bwExpected=(bwExpected+1)/2;bhExpected=(bhExpected+1)/2;}const auto& a=r.pyramid.bands[b];const auto& other=t.pyramid.bands[b];if(a.width!=bwExpected||a.height!=bhExpected||a.values.size()!=size_t(a.width)*a.height||other.width!=a.width||other.height!=a.height||other.values.size()!=a.values.size())throw std::invalid_argument("Single band shape");for(auto values:{std::span<const float>(a.values),std::span<const float>(other.values)})for(float v:values)if(!std::isfinite(v))throw std::invalid_argument("Nonfinite Single pyramid band");self.push_back(mutual(a.values,other.values,a.width,a.height));}
 const auto& baseR=r.pyramid.bands.back();const auto& baseT=t.pyramid.bands.back();std::vector<float> baseCol(baseR.values.size());
 for(unsigned x=0;x<baseR.width;++x)for(unsigned y=0;y<baseR.height;++y)baseCol[size_t(x)*baseR.height+y]=baseT.values[size_t(y)*baseR.width+x]-baseR.values[size_t(y)*baseR.width+x];
 auto spectrum=fft.forward(baseCol,baseR.width,baseR.height);if(spectrum.size()!=2*baseCol.size())throw std::runtime_error("Single base FFT shape");spectrum[0]=spectrum[1]=0;size_t selected=0;float maximum=0;
 for(size_t i=0;i<baseCol.size();++i){float mag=std::hypot(spectrum[2*i],spectrum[2*i+1]);if(mag>maximum){selected=i;maximum=mag;}}
 const double baseFrequency=std::ldexp(1.,-int(count-1))*ppd/2;
 auto axis=[&](unsigned k,unsigned size){return ((std::fmod(.5+double(k)/size,1.)-.5)*(baseFrequency*2))*2;};
 double fx=axis(unsigned(selected/baseR.height),baseR.width),fy=axis(unsigned(selected%baseR.height),baseR.height);GeneralQualityResultSingle result;result.base_frequency=std::sqrt(fx*fx+fy*fy);
 std::vector<float> log(n);std::vector<double> comparison(n,0.);
 for(size_t i=0;i<n;++i)log[i]=std::log10(std::clamp((t.adaptation[i]+r.adaptation[i])/2.f,1e-5f,1e5f));
 for(size_t c=0;c<3;++c)for(size_t i=0;i<n;++i)if(std::abs(t.native_rgb[c*n+i]-r.native_rgb[c*n+i])/r.native_rgb[c*n+i]>.001f)comparison[i]=1.;
 const double p=std::pow(10.,.3424),q=std::pow(10.,.108934275615),pf=std::pow(10.,.34)/p;
 const float ks=float(std::pow(10.,1.41846291721)),kn=float(std::pow(10.,.136877512403));
 for(size_t b=0;b<count;++b){const auto& a=r.pyramid.bands[b];unsigned bw=a.width,bh=a.height;size_t bn=a.values.size();std::vector<float> neighbor(bn,0.f);
 auto add=[&](size_t k){const auto& from=r.pyramid.bands[k];auto resized=resize_bicubic(self[k],from.height,from.width,bh,bw);for(size_t i=0;i<bn;++i)neighbor[i]+=std::max(resized[i],0.f);};if(b>0)add(b-1);if(b+2<count)add(b+1);
 auto resizedLog=resize_bicubic(log,h,w,bh,bw);auto mask=resize_bicubic(comparison,h,w,bh,bw);auto table=create_csf_lut(std::ldexp(1.,-int(b))*ppd/2);
 for(float& v:resizedLog)v=std::clamp(v,-5.f,5.f);
 double baseNoise=-1;if(b+1==count){float lum=std::pow(10.f,mean(resizedLog,bw,bh));baseNoise=1./compute_ncsf_single_luminance(result.base_frequency,lum);}
 std::vector<float> differences(bn);
 for(size_t i=0;i<bn;++i){float s=ks*std::pow(std::abs(self[b][i]),float(q));float neighborTerm=kn*std::pow(std::abs(neighbor[i]),float(q));float m=s+0.f;m+=neighborTerm;
 float diff=t.pyramid.bands[b].values[i]-a.values[i];float sensitivity=sample_csf_lut_log_single(table,resizedLog[i]);float raw=0;masked_difference(diff,sensitivity,m,p,pf,baseNoise,&raw);differences[i]=raw;}
 result.band_errors.push_back(quality_band_error(differences,mask,bw,bh));
 }
 result.scores=quality_scores(result.band_errors);return result;
}
}
