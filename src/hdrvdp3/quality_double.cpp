#include <IQM/hdrvdp3/quality_double.h>
#include <IQM/hdrvdp3/resize.h>
#include <IQM/hdrvdp3/csf.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace IQM::HDRVDP3 {
std::vector<double> quality_log_adaptation_double(std::span<const double> reference,std::span<const double> test){
 if(reference.size()!=9216||test.size()!=9216)throw std::invalid_argument("Double quality adaptation requires96x96");std::vector<double> result(9216);
 for(size_t i=0;i<9216;++i){if(!std::isfinite(reference[i])||!std::isfinite(test[i])||reference[i]<=0||test[i]<=0)throw std::invalid_argument("Invalid quality adaptation");result[i]=std::log10(std::clamp((test[i]+reference[i])/2.,1e-5,1e5));}return result;
}
std::vector<double> quality_comparison_double(std::span<const double> reference,std::span<const double> test,unsigned band){
 if(reference.size()!=27648||test.size()!=27648||band>=5)throw std::invalid_argument("Invalid Double comparison geometry");std::vector<double> mask(9216,0.);
 for(size_t c=0;c<3;++c)for(size_t i=0;i<9216;++i){const size_t k=c*9216+i;if(!std::isfinite(reference[k])||!std::isfinite(test[k])||reference[k]<0||test[k]<0)throw std::invalid_argument("Invalid comparison RGB");if(std::abs(test[k]-reference[k])/reference[k]>.001)mask[i]=1.;}
 const unsigned w=band<2?96:96/(1u<<(band-1));return resize_bicubic(mask,96,96,w,w);
}
QualityCSFDouble quality_csf_double(std::span<const double> mean_log,unsigned band,double base_frequency){
 if(mean_log.size()!=9216||band>=5||!std::isfinite(base_frequency)||base_frequency<0)throw std::invalid_argument("Invalid Double CSF geometry/frequency");
 std::vector<double> input(mean_log.begin(),mean_log.end());for(double v:input)if(!std::isfinite(v))throw std::invalid_argument("Invalid Double CSF adaptation");
 const unsigned w=band<2?96:96/(1u<<(band-1));QualityCSFDouble out;out.log_adaptation=resize_bicubic(input,96,96,w,w);out.table=create_csf_lut(15./double(1u<<band));out.sensitivity.resize(size_t(w)*w);out.noise.resize(size_t(w)*w);
 for(size_t i=0;i<out.noise.size();++i){const double v=std::clamp(out.log_adaptation[i],-5.,5.);out.log_adaptation[i]=v;out.sensitivity[i]=sample_csf_lut_log(out.table,v);out.noise[i]=1./out.sensitivity[i];}
 if(band==4){double sum=0,correction=0;for(unsigned x=0;x<w;++x)for(unsigned y=0;y<w;++y){const double value=out.log_adaptation[size_t(y)*w+x],next=sum+value;correction+=std::abs(sum)>=std::abs(value)?(sum-next)+value:(value-next)+sum;sum=next;}out.base_luminance=std::pow(10.,(sum+correction)/double(w*w));const double noise=1./compute_ncsf(base_frequency,out.base_luminance);std::fill(out.noise.begin(),out.noise.end(),noise);}return out;
}
std::vector<double> mutual_mask_double(std::span<const double> reference,std::span<const double> test,unsigned w,unsigned h){
 if(!w||!h||reference.size()!=size_t(w)*h||test.size()!=reference.size())throw std::invalid_argument("Double mutual mask shape mismatch");
 // Native default do_si_gauss=false,masking_norm=0,pool_size=3.
 std::array<double,9> kernel;kernel.fill(1./9.);
 std::vector<double> activity(reference.size()),out(reference.size());for(size_t i=0;i<activity.size();++i){if(!std::isfinite(reference[i])||!std::isfinite(test[i]))throw std::invalid_argument("Nonfinite mutual mask input");activity[i]=std::min(std::abs(reference[i]),std::abs(test[i]));}
 for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){double acc=0;for(int fx=0;fx<3;++fx)for(int fy=2;fy>=0;--fy){int sx=int(x)+fx-1,sy=int(y)+fy-1;double value=sx<0||sy<0||sx>=int(w)||sy>=int(h)?0.:activity[size_t(sy)*w+sx];if(sy>=0&&sy<int((h/16)*16))acc=std::fma(value,kernel[fy*3+fx],acc);else{const double product=value*kernel[fy*3+fx];acc+=product;}}out[size_t(y)*w+x]=acc;}return out;
}
std::array<double,6> quality_mask_parameters_double(){
 const double p=std::pow(10.,.3424),q=std::pow(10.,.108934275615);
 return {p,q,std::pow(10.,.34)/p,std::pow(10.,1.41846291721),std::pow(10.,.136877512403),std::pow(10.,-50.)};
}
std::vector<double> quality_neighbor_mask_double(std::span<const std::vector<double>> masks,unsigned band){
 if(masks.size()!=5||band>=5)throw std::invalid_argument("Double neighbor mask requires five sp0 bands");
 const unsigned widths[]{96,96,48,24,12};for(size_t b=0;b<5;++b){if(masks[b].size()!=size_t(widths[b])*widths[b])throw std::invalid_argument("Double neighbor mask geometry mismatch");for(double v:masks[b])if(!std::isfinite(v)||v<0)throw std::invalid_argument("Invalid Double self mask");}
 const unsigned w=widths[band];std::vector<double> result(size_t(w)*w,0.);
 auto add=[&](unsigned b){auto v=resize_bicubic(masks[b],widths[b],widths[b],w,w);for(size_t i=0;i<v.size();++i)result[i]+=std::max(v[i],0.);};
 if(band>0)add(band-1);if(band<3)add(band+1);return result;
}
QualityBandDouble evaluate_quality_band_double(std::span<const double> diff,
 std::span<const double> self,std::span<const double> neighbor,std::span<const double> noise,
 std::span<const double> comparison,unsigned width,unsigned height){
 const size_t n=size_t(width)*height;
 if(!width||!height||diff.size()!=n||self.size()!=n||neighbor.size()!=n||noise.size()!=n||comparison.size()!=n)throw std::invalid_argument("Double quality band shape mismatch");
 const auto parameters=quality_mask_parameters_double();const auto p=parameters[0],q=parameters[1],pf=parameters[2];
 QualityBandDouble out;out.mask_noise.resize(n);out.excitation.resize(n);out.difference.resize(n);out.psychometric.resize(n);out.masked.resize(n);
 for(size_t i=0;i<n;++i){
  if(!std::isfinite(diff[i])||!std::isfinite(self[i])||self[i]<0||!std::isfinite(neighbor[i])||neighbor[i]<0||!(noise[i]>0)||!std::isfinite(comparison[i]))throw std::invalid_argument("Invalid Double quality stage input");
  const double own=parameters[3]*std::pow(std::abs(self[i]),q),cross=parameters[5]*std::pow(0.,q),adjacent=parameters[4]*std::pow(std::abs(neighbor[i]),q);
  const double mask=(own+cross)+adjacent;out.mask_noise[i]=mask;
  const double excitation=(diff[i]>0?1.:diff[i]<0?-1.:0.)*std::pow(std::abs(diff[i]),p);out.excitation[i]=excitation;
  const double d=excitation/std::sqrt(std::pow(noise[i],2*p)+mask*mask);out.difference[i]=d;
  out.psychometric[i]=(d>0?1.:d<0?-1.:0.)*std::pow(std::abs(d),pf);
  out.masked[i]=d*comparison[i];
 }
 double sum=0;for(unsigned x=0;x<width;++x)for(unsigned y=0;y<height;++y)sum+=std::pow(std::abs(out.masked[size_t(y)*width+x]),.8)/double(n);
 out.error=std::pow(sum,1./.8);return out;
}
QualityScoresDouble quality_scores_double(std::span<const double> errors){
 if(errors.empty())throw std::invalid_argument("Empty Double quality errors");double error=0;for(double e:errors){if(!std::isfinite(e)||e<0)throw std::invalid_argument("Invalid Double band error");error+=e/double(errors.size());}
 const double Q=6.-std::log(std::exp(-4.)+error);const double jod=10.-.5200*std::pow(std::max(10.-Q,0.),1.2812);return {error,Q,jod};
}
}
