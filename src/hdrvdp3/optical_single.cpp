#include <IQM/hdrvdp3/optical_single.h>
#include <IQM/hdrvdp3/mtf.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace IQM::HDRVDP3 {namespace {
size_t check(unsigned w,unsigned h,double ppd,const FFTBackendSingle& fft){
 if(w<1||h<1||w>unsigned(std::numeric_limits<int>::max()/2)||h>unsigned(std::numeric_limits<int>::max()/2)||!std::isfinite(ppd)||ppd<26||ppd>256||!fft.convolve)
   throw std::invalid_argument("Single FFT geometry/viewing/provider domain");
 if(size_t(w)>std::vector<double>().max_size()/size_t(h)/16)throw std::invalid_argument("Single FFT size overflow");return size_t(w)*h;
}
std::vector<float> pad(std::span<const float> row,unsigned w,unsigned h,bool symmetric){
 std::vector<float> out(size_t(2*w)*(2*h));for(unsigned x=0;x<2*w;++x)for(unsigned y=0;y<2*h;++y){
 unsigned sx=x<w?x:(symmetric?2*w-1-x:w-1),sy=y<h?y:(symmetric?2*h-1-y:h-1);out[size_t(x)*(2*h)+y]=row[size_t(sy)*w+sx];}return out;
}
std::vector<float> apply_pad(std::span<const float> input,unsigned w,unsigned h,bool symmetric,const SinglePaddingBackend* backend){
 if(!backend)return pad(input,w,h,symmetric);
 if(!backend->pad)throw std::invalid_argument("Padding provider is empty");
 auto output=backend->pad(input,w,h,symmetric);
 if(output.size()!=4*size_t(w)*h)throw std::runtime_error("Padding provider shape mismatch");
 for(float v:output)if(!std::isfinite(v))throw std::runtime_error("Padding provider nonfinite output");
 return output;
}
void valid(std::span<const float> input,size_t n){if(input.size()!=n)throw std::invalid_argument("Single FFT input shape");for(float x:input)if(!std::isfinite(x)||x<0)throw std::invalid_argument("Single FFT input domain");}
}
std::vector<float> filter_optical_rgb_single(std::span<const float> native,unsigned w,unsigned h,double ppd,const FFTBackendSingle& fft,const SinglePaddingBackend* padding){
 size_t n=check(w,h,ppd,fft);valid(native,3*n);unsigned pw=2*w,ph=2*h;size_t pn=size_t(pw)*ph;
 std::vector<double> row(pn),filter(pn);generate_mtf_filter(row.data(),pw,ph,ppd);
 for(unsigned x=0;x<pw;++x)for(unsigned y=0;y<ph;++y)filter[size_t(x)*ph+y]=row[size_t(y)*pw+x];
 std::vector<float> output(3*n);for(size_t c=0;c<3;++c){auto padded=apply_pad(native.subspan(c*n,n),w,h,true,padding);auto col=fft.convolve(padded,filter,pw,ph,w,h);if(col.size()!=n)throw std::runtime_error("Single FFT output shape");
 for(unsigned x=0;x<w;++x)for(unsigned y=0;y<h;++y){float v=col[size_t(x)*h+y];if(!std::isfinite(v))throw std::runtime_error("Nonfinite optical FFT result");output[c*n+size_t(y)*w+x]=std::clamp(v,1e-5f,1e10f);}}
 return output;
}
std::vector<float> local_adaptation_single(std::span<const float> luminance,unsigned w,unsigned h,double ppd,const FFTBackendSingle& fft,const SinglePaddingBackend* padding){
 size_t n=check(w,h,ppd,fft);valid(luminance,n);std::vector<float> log(n);for(size_t i=0;i<n;++i)log[i]=std::log(std::max(luminance[i],1e-6f));
 unsigned pw=2*w,ph=2*h;std::vector<double> filter(size_t(pw)*ph);double sigma=std::pow(10.,-.781367)*ppd;
 for(unsigned x=0;x<pw;++x)for(unsigned y=0;y<ph;++y){double kx=(std::fmod(.5+double(x)/pw,1.)-.5)*(2*3.14159265358979323846),ky=(std::fmod(.5+double(y)/ph,1.)-.5)*(2*3.14159265358979323846);filter[size_t(x)*ph+y]=std::exp((-.5*(kx*kx+ky*ky))*(sigma*sigma));}
 auto padded=apply_pad(log,w,h,false,padding);auto col=fft.convolve(padded,filter,pw,ph,w,h);if(col.size()!=n)throw std::runtime_error("Single adaptation output shape");std::vector<float> output(n);
 for(unsigned x=0;x<w;++x)for(unsigned y=0;y<h;++y){float v=std::exp(col[size_t(x)*h+y]);if(!std::isfinite(v)||v<=0)throw std::runtime_error("Single adaptation result domain");output[size_t(y)*w+x]=v;}return output;
}
}
