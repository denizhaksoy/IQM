#pragma once
#include <IQM/hdrvdp3/fft_inverse_gpu.h>
namespace IQM::HDRVDP3 {
enum class OpticalStage {Pad,Forward,Expand,Multiply,Inverse,Normalize,Crop};
struct OpticalResidentBuffers {
 FFTBufferRegion input,output,filter,padded,half,spectrum,inverse,fftIntermediate,status;
 std::array<FFTBufferRegion,4>workspace;
};
struct OpticalResidentShaders {std::filesystem::path padding,io;FFT2DShaders forward;FFTInverse2DShaders inverse;};
// One native-display channel3840x2160f32(row-major), symmetric post padding,
// actual7680x4320 FFT graph, caller-supplied static MTF f32 coefficients(x-major).
// All intermediates caller-owned device-local buffers; no CPU image computation,
// staging/readback, buffer allocation, submission or waiting inside this API.
// filter/padded:7680*4320*4; half:7680*2161*8; spectrum/inverse/fftIntermediate:
// 7680*4320*8; input/output:3840*2160*4; status:4bytes; workspaces as FFT2D APIs.
// Padded is reused for normalized real output; spectrum is multiplied in place.
// status bit1=nonreal spectrum,bit2=nonreal product. Only status==3 is supported:
// real-narrowed Octave alternative transform graphs are NOT implemented here.
// Unsupported final output is NaN; caller must propagate/check scalar status.
class ResidentOpticalFilter {
public:
 ResidentOpticalFilter(const vk::raii::Device&,const vk::raii::PhysicalDevice&,const OpticalResidentBuffers&,const OpticalResidentShaders&);
 ~ResidentOpticalFilter();ResidentOpticalFilter(ResidentOpticalFilter&&)noexcept;ResidentOpticalFilter&operator=(ResidentOpticalFilter&&)noexcept;
 ResidentOpticalFilter(const ResidentOpticalFilter&)=delete;ResidentOpticalFilter&operator=(const ResidentOpticalFilter&)=delete;
 void record(vk::CommandBuffer)const;
 // Ordered GPU-only debug/composition stages; no CPU tensor observations.
 // Pad initializes status on GPU. Crop includes official [1e-5,1e10] clamp.
 void recordStage(vk::CommandBuffer,OpticalStage)const;
 uint32_t dispatchCount()const noexcept;
private:struct Impl;std::unique_ptr<Impl>impl_;
};
}
