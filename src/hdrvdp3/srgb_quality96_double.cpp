#include <IQM/hdrvdp3/srgb_quality96_double.h>
#include <IQM/hdrvdp3/srgb_display.h>
#include <IQM/hdrvdp3/mtf.h>
#include <IQM/hdrvdp3/jnd.h>
#include <IQM/hdrvdp3/sp0_double.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace IQM::HDRVDP3 {
namespace {
constexpr size_t pixels=9216,paddedPixels=36864;
std::vector<double> rows(std::vector<double> v){auto r=v;for(size_t c=0;c<v.size()/pixels;++c)for(size_t y=0;y<96;++y)for(size_t x=0;x<96;++x)r[c*pixels+y*96+x]=v[c*pixels+x*96+y];return r;}
void validate(std::span<const float> values){if(values.size()!=3*pixels)throw std::invalid_argument("sRGB quality96 requires planar96x96 RGB");for(float v:values)if(!std::isfinite(v)||v<0||v>1)throw std::invalid_argument("sRGB quality96 requires finite encoded RGB in[0,1]");}
void validate_result(std::span<const double> values,size_t size){if(values.size()!=size)throw std::runtime_error("sRGB quality96 FFT backend returned wrong shape");for(double v:values)if(!std::isfinite(v))throw std::runtime_error("sRGB quality96 FFT backend returned nonfinite output");}
std::vector<double> pad(std::span<const double> v,bool symmetric){std::vector<double> out(paddedPixels);for(size_t x=0;x<192;++x)for(size_t y=0;y<192;++y){const size_t sx=x<96?x:(symmetric?191-x:95),sy=y<96?y:(symmetric?191-y:95);out[x*192+y]=v[sx*96+sy];}return out;}
struct Side{std::vector<double> rgb,adaptation;SP0PyramidDouble pyramid;};
}
SRGBQuality96ResultDouble evaluate_srgb_quality96_double(std::span<const float> reference,std::span<const float> test,const XYZDisplaySamples& xyz,const RGBSpectralSamples& spectral,const SRGBQuality96BackendDouble& backend){
 validate(reference);validate(test);if(!backend.convolve||!backend.base_fft)throw std::invalid_argument("sRGB quality96 requires explicit convolution and baseFFT backends");
 std::vector<double> mtf(paddedPixels),gaussian(paddedPixels);generate_mtf_filter(mtf.data(),192,192,30);const double sigma=std::pow(10.,-.781367)*30;
 for(size_t x=0;x<192;++x)for(size_t y=0;y<192;++y){double kx=(std::fmod(.5+double(x)/192,1.)-.5)*(2*3.14159265358979323846),ky=(std::fmod(.5+double(y)/192,1.)-.5)*(2*3.14159265358979323846);gaussian[x*192+y]=std::exp((-.5*(kx*kx+ky*ky))*(sigma*sigma));}
 auto prepare=[&](std::span<const float> encoded){Side side;auto rgb=prepare_srgb_display(encoded,xyz).native_rgb;std::vector<double> optical(3*pixels);
  for(size_t c=0;c<3;++c){auto padded=pad(std::span<const double>(rgb).subspan(c*pixels,pixels),true);auto result=backend.convolve(padded,mtf);validate_result(result,pixels);for(size_t i=0;i<pixels;++i)optical[c*pixels+i]=backend.project?result[i]:std::clamp(result[i],1e-5,1e10);}
  auto projected=backend.project?backend.project(optical,spectral):project_optical_rgb_double(optical,spectral,24);validate_result(projected.clamped,4*pixels);validate_result(projected.adaptation_log,pixels);auto log=pad(projected.adaptation_log,false);auto filtered=backend.convolve(log,gaussian);validate_result(filtered,pixels);auto adaptation=finish_local_adaptation_double(filtered);
  auto lut=create_jnd_lut();auto lmsrRows=rows(projected.clamped),adaptationRows=rows(adaptation);auto jnd=backend.jnd?backend.jnd(lut,lmsrRows,adaptationRows):rgb_to_jnd_response_double(lut,lmsrRows,adaptationRows,96,96,96.*96./30./30.,24);validate_result(jnd.post_pupil,4*pixels);validate_result(jnd.channels,4*pixels);validate_result(jnd.response,pixels);if(!std::isfinite(jnd.geometric_mean)||jnd.geometric_mean<=0||!std::isfinite(jnd.pupil_reduction)||jnd.pupil_reduction<=0)throw std::runtime_error("Invalid JND backend scalars");
  side.rgb=rows(std::move(rgb));side.adaptation=rows(std::move(adaptation));side.pyramid=backend.pyramid?backend.pyramid(jnd.response):build_sp0_pyramid_double(jnd.response);const unsigned widths[]{96,96,48,24,12};if(side.pyramid.bands.size()!=5)throw std::runtime_error("Pyramid backend band count");for(size_t b=0;b<5;++b){const auto& band=side.pyramid.bands[b];if(band.width!=widths[b]||band.height!=widths[b])throw std::runtime_error("Pyramid backend geometry");validate_result(band.values,size_t(widths[b])*widths[b]);}validate_result(side.pyramid.base_pre,144);if(!std::isfinite(side.pyramid.base_mean))throw std::runtime_error("Pyramid backend mean");return side;
 };
 const auto r=prepare(reference),t=prepare(test);std::vector<std::vector<double>> self;
 for(unsigned b=0;b<5;++b){const auto& band=r.pyramid.bands[b];auto mask=backend.mutual?backend.mutual(band.values,t.pyramid.bands[b].values,band.width,band.height):mutual_mask_double(band.values,t.pyramid.bands[b].values,band.width,band.height);validate_result(mask,band.values.size());for(double v:mask)if(v<0)throw std::runtime_error("Negative mutual mask backend output");self.push_back(std::move(mask));}
 std::vector<double> base(288);for(unsigned x=0;x<12;++x)for(unsigned y=0;y<12;++y)base[2*(x*12+y)]=t.pyramid.bands[4].values[y*12+x]-r.pyramid.bands[4].values[y*12+x];auto spectrum=backend.base_fft(base);validate_result(spectrum,288);
 if(spectrum[0]!=0.||spectrum[1]!=0.)throw std::runtime_error("sRGB quality96 baseFFT backend must zero DC");
 size_t selected=0;double maximum=0;for(size_t i=0;i<144;++i){double magnitude=std::hypot(spectrum[2*i],spectrum[2*i+1]);if(magnitude>maximum){maximum=magnitude;selected=i;}}
 const auto axis=[](unsigned k){const double n=std::fmod(.5+double(k)/12.,1.)-.5;return(n*1.875)*2.;};const double fx=axis(unsigned(selected/12)),fy=axis(unsigned(selected%12)),frequency=std::sqrt(fx*fx+fy*fy);
 const auto meanLog=quality_log_adaptation_double(r.adaptation,t.adaptation);SRGBQuality96ResultDouble output;
 for(unsigned b=0;b<5;++b){const unsigned w=r.pyramid.bands[b].width;const auto neighbor=backend.neighbor?backend.neighbor(self,b):quality_neighbor_mask_double(self,b),comparison=backend.comparison?backend.comparison(r.rgb,t.rgb,b):quality_comparison_double(r.rgb,t.rgb,b);validate_result(comparison,size_t(w)*w);validate_result(neighbor,size_t(w)*w);for(double v:neighbor)if(v<0)throw std::runtime_error("Negative neighbor mask backend output");const auto csf=backend.csf?backend.csf(meanLog,b,frequency):quality_csf_double(meanLog,b,frequency);validate_result(csf.log_adaptation,size_t(w)*w);validate_result(csf.sensitivity,size_t(w)*w);validate_result(csf.table,256);if(csf.noise.size()!=size_t(w)*w)throw std::runtime_error("CSF backend noise shape");for(double v:csf.noise)if(!(v>0)||std::isnan(v))throw std::runtime_error("Invalid CSF backend noise");if(b==4&&(!std::isfinite(csf.base_luminance)||csf.base_luminance<=0))throw std::runtime_error("Invalid CSF backend luminance");std::vector<double> difference(size_t(w)*w);for(size_t i=0;i<difference.size();++i)difference[i]=t.pyramid.bands[b].values[i]-r.pyramid.bands[b].values[i];auto result=backend.quality?backend.quality(difference,self[b],neighbor,csf.noise,comparison,w,w):evaluate_quality_band_double(difference,self[b],neighbor,csf.noise,comparison,w,w);for(const auto* field:{&result.mask_noise,&result.excitation,&result.difference,&result.psychometric,&result.masked})validate_result(*field,size_t(w)*w);if(!std::isfinite(result.error)||result.error<0)throw std::runtime_error("Invalid quality backend error");output.band_errors[b]=result.error;}
 output.scores=quality_scores_double(output.band_errors);return output;
}
}
