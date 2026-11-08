#pragma once
#include "fft128_single_gpu.h"
#include "fft_filter_single_gpu.h"
#include <IQM/hdrvdp3/fft_single.h>
#include <stdexcept>
namespace IQM::HDRVDP3::Experimental {
// Explicit fixed-size GPU provider: unsupported shapes fail, never fall back.
// Reference3thread/estimate128graph. CPU provides model/filter coefficients.
// Stage buffers still transfer synchronously; not a resident/fullmetric backend.
struct FFT128ProviderGPU {
 const IQM::Bin::VulkanInstance& gpu;
 std::string fftShader,filterShader,cropShader;
 FFTBackendSingle backend() const {
  return {
   [this](auto input,auto filter,unsigned w,unsigned h,unsigned cw,unsigned ch){
    if(w!=128||h!=128||!cw||!ch||cw>w||ch>h)throw std::invalid_argument("Experimental GPU FFT provider supports128x128 only");
    auto spectrum=fft128_gpu(gpu,fftShader,input,true);
    auto product=fft_filter_single_gpu(gpu,filterShader,spectrum,filter);
    auto inverse=fft128_gpu(gpu,fftShader,product,false);
    return fft128_crop_single_gpu(gpu,cropShader,inverse,cw,ch);
   },
   [this](auto input,unsigned w,unsigned h){
    if(w!=128||h!=128)throw std::invalid_argument("Experimental GPU FFT forward supports128x128 only");
    return fft128_gpu(gpu,fftShader,input,true);
   }
  };
 }
};
}
