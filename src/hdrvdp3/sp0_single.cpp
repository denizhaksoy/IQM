#include <IQM/hdrvdp3/sp0_single.h>
#include <IQM/hdrvdp3/sp0_double.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <limits>
namespace IQM::HDRVDP3 {namespace {
int reflect(int p,int n){int period=2*n-2;p%=period;if(p<0)p+=period;return p<n?p:period-p;}
SP0BandSingle correlate(const SP0BandSingle& input,const std::vector<double>& filter,unsigned k,unsigned step){
 SP0BandSingle out{(input.width+step-1)/step,(input.height+step-1)/step,{}};out.values.resize(size_t(out.width)*out.height);
 const bool transposed=input.width>input.height;
 for(unsigned y=0;y<out.height;++y)for(unsigned x=0;x<out.width;++x){float sum=0;
 for(unsigned ix=0;ix<k;++ix)for(unsigned iy=0;iy<k;++iy){int fx=int(k-1-(transposed?iy:ix)),fy=int(k-1-(transposed?ix:iy));
 int sx=reflect(int(x*step)+fx-int(k/2),int(input.width)),sy=reflect(int(y*step)+fy-int(k/2),int(input.height));
 float a=input.values[size_t(sy)*input.width+sx],b=float(filter[fy*k+fx]);
 if((transposed?x:y)*step<((transposed?input.width:input.height)/32)*32)sum=std::fma(a,b,sum);else{float product=a*b;sum+=product;}}
 out.values[size_t(y)*out.width+x]=sum;}return out;
}
}
SP0PyramidSingle build_sp0_pyramid_single(std::span<const float> response,unsigned w,unsigned h,double ppd){
 if(w<16||h<16||w>unsigned(std::numeric_limits<int>::max()/2)||h>unsigned(std::numeric_limits<int>::max()/2)||size_t(w)>std::vector<float>().max_size()/size_t(h)/4||response.size()!=size_t(w)*h||!std::isfinite(ppd)||ppd<26||ppd>256)throw std::invalid_argument("Single SP0 geometry/viewing domain");
 for(float v:response)if(!std::isfinite(v))throw std::invalid_argument("Nonfinite Single SP0 response");
 unsigned max=0;for(unsigned a=w,b=h;a>=13&&b>=13;a/=2,b/=2)++max;unsigned levels=std::min(unsigned(std::ceil(std::log2(ppd))-2),max);
 auto f=create_sp0_model_double();SP0BandSingle input{w,h,{response.begin(),response.end()}};SP0PyramidSingle out;
 out.bands.push_back(correlate(input,f.high,9,1));auto low=correlate(input,f.lo0,7,1);
 for(unsigned i=0;i<levels;++i){out.bands.push_back(correlate(low,f.orientation,9,1));low=correlate(low,f.lo,13,2);}
 out.bands.push_back(std::move(low));for(size_t b=0;b<out.bands.size();++b)for(float& v:out.bands[b].values)v/=std::ldexp(1.f,int(b));
 // Octave mean(Single) uses sum(...,"extra"): Double accumulation, then
 // division and final Single cast. It is not the ordinary Single sum used
 // by geomean in the pupil stage.
 auto& base=out.bands.back();double sum=0;for(unsigned x=0;x<base.width;++x)for(unsigned y=0;y<base.height;++y)sum+=double(base.values[size_t(y)*base.width+x]);out.base_mean=float(sum/double(base.values.size()));for(float& v:base.values)v-=out.base_mean;return out;
}
}
