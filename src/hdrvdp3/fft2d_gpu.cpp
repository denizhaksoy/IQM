#include <IQM/hdrvdp3/fft2d_gpu.h>
#include <algorithm>
#include <stdexcept>
#include <vector>
namespace IQM::HDRVDP3 {
struct ResidentFFT2DForward::Impl {
 std::vector<std::unique_ptr<ResidentFFT4320>> real;
 std::vector<std::unique_ptr<ResidentFFT>> complex;
 Impl(const vk::raii::Device&d,const vk::raii::PhysicalDevice&p,const FFT2DBuffers&b,const FFT2DShaders&s,FFT2DChunks c){
  if(!c.real4320||c.real4320>1024||!c.complex7680||c.complex7680>4096)throw std::invalid_argument("FFT2D chunk capacity");
  auto limits=p.getProperties().limits;
  std::vector<FFTBufferRegion> regions{b.input.region,b.output.region,b.intermediate};for(auto w:b.workspace)regions.push_back(w);
  for(auto r:regions)if(!r.buffer||!r.bytes||r.bytes%4||r.offset%limits.minStorageBufferOffsetAlignment||r.offset>UINT64_MAX-r.bytes||r.bytes>limits.maxStorageBufferRange)throw std::invalid_argument("FFT2D region");
  for(size_t i=0;i<regions.size();++i)for(size_t j=i+1;j<regions.size();++j)if(regions[i].buffer==regions[j].buffer&&regions[i].offset<regions[j].offset+regions[j].bytes&&regions[j].offset<regions[i].offset+regions[i].bytes)throw std::invalid_argument("FFT2D overlapping regions");
  auto view=[](uint32_t first,uint32_t xs,uint32_t ys,uint32_t height,uint64_t available){if(!xs||!ys||(uint64_t(xs)<uint64_t(height-1)*ys+1&&uint64_t(ys)<7679ull*xs+1))throw std::invalid_argument("FFT2D view strides");uint64_t end=uint64_t(first)+7679ull*xs+uint64_t(height-1)*ys+1;if(end>available||end>UINT32_MAX/2)throw std::invalid_argument("FFT2D view bounds");};
  view(b.input.first,b.input.xStride,b.input.yStride,4320,b.input.region.bytes/4);
  view(b.output.first,b.output.xStride,b.output.yStride,2161,b.output.region.bytes/8);
  if(b.output.region.bytes%8||b.intermediate.offset%8||b.intermediate.bytes%8||b.intermediate.bytes<7680ull*2161*8)throw std::invalid_argument("FFT2D half plane");
  const uint64_t required=std::max(uint64_t(c.real4320)*4320,uint64_t(c.complex7680)*7680)*8;
  for(auto w:b.workspace)if(w.bytes<required||w.bytes%8)throw std::invalid_argument("FFT2D workspace");
  for(uint32_t x=0;x<7680;x+=c.real4320){auto n=std::min(c.real4320,7680-x);FFT4320Buffers a{{b.input.region,b.input.first+x*b.input.xStride,b.input.yStride,b.input.xStride},{b.intermediate,x*2161*2,2,2161*2},b.workspace};real.push_back(std::make_unique<ResidentFFT4320>(d,p,FFT4320Mode::RealForward,n,a,s.axis4320));}
  for(uint32_t y=0;y<2161;y+=c.complex7680){auto n=std::min(c.complex7680,2161-y);ResidentFFTBuffers a{{b.intermediate,y,2161,1},{b.output.region,b.output.first+y*b.output.yStride,b.output.xStride,b.output.yStride},{b.workspace[0],b.workspace[1],b.workspace[2]}};complex.push_back(std::make_unique<ResidentFFT>(d,p,ResidentFFTSpec{FFTDomain::Complex1D,7680,n,FFTDirection::Forward,FFTNumericalGraph::Official4KAxis7680},a,s.axis7680));}
 }
};
ResidentFFT2DForward::ResidentFFT2DForward(const vk::raii::Device&d,const vk::raii::PhysicalDevice&p,const FFT2DBuffers&b,const FFT2DShaders&s,FFT2DChunks c):impl_(std::make_unique<Impl>(d,p,b,s,c)){}
ResidentFFT2DForward::~ResidentFFT2DForward()=default;
ResidentFFT2DForward::ResidentFFT2DForward(ResidentFFT2DForward&&)noexcept=default;
ResidentFFT2DForward& ResidentFFT2DForward::operator=(ResidentFFT2DForward&&)noexcept=default;
void ResidentFFT2DForward::recordRealAxis(vk::CommandBuffer c)const{if(!impl_||!c)throw std::invalid_argument("FFT2D null job/cmd");for(const auto&j:impl_->real)j->record(c);}
void ResidentFFT2DForward::recordComplexAxis(vk::CommandBuffer c)const{if(!impl_||!c)throw std::invalid_argument("FFT2D null job/cmd");for(const auto&j:impl_->complex)j->record(c);}
void ResidentFFT2DForward::record(vk::CommandBuffer c)const{recordRealAxis(c);recordComplexAxis(c);}
uint32_t ResidentFFT2DForward::dispatchCount()const noexcept{return impl_?uint32_t(6*(impl_->real.size()+impl_->complex.size())):0;}
}
