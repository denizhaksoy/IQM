#include <IQM/hdrvdp3/sp0_double.h>
#include <cmath>
#include <stdexcept>
namespace IQM::HDRVDP3 {
namespace {
int reflect(int p,int n){if(p<0)return -p;if(p>=n)return 2*n-2-p;return p;}
SP0BandDouble correlate(const SP0BandDouble& input,const std::vector<double>& filter,unsigned k,unsigned step){
 SP0BandDouble out{(input.width+step-1)/step,(input.height+step-1)/step,{}};out.values.resize(size_t(out.width)*out.height);
 for(unsigned y=0;y<out.height;++y)for(unsigned x=0;x<out.width;++x){double sum=0;
  // Native Double conv2 uses FMA in 16-row blocks and separate tail steps.
  for(unsigned ix=0;ix<k;++ix)for(unsigned iy=0;iy<k;++iy){int fx=int(k-1-ix),fy=int(k-1-iy);int sx=reflect(int(x*step)+fx-int(k/2),input.width),sy=reflect(int(y*step)+fy-int(k/2),input.height);const double value=input.values[size_t(sy)*input.width+sx],coefficient=filter[fy*k+fx];const bool fused=y*step<(input.height/16)*16;if(fused)sum=std::fma(value,coefficient,sum);else{const double product=value*coefficient;sum+=product;}}
  out.values[size_t(y)*out.width+x]=sum;
 }return out;
}
}
SP0PyramidDouble build_sp0_pyramid_double(std::span<const double> response){
 if(response.size()!=9216)throw std::invalid_argument("Double sp0 currently requires96x96/30ppd");for(double v:response)if(!std::isfinite(v))throw std::invalid_argument("Nonfinite Double pyramid input");
 const auto filters=create_sp0_model_double();SP0BandDouble input{96,96,{response.begin(),response.end()}};SP0PyramidDouble result;
 result.bands.push_back(correlate(input,filters.high,9,1));auto low=correlate(input,filters.lo0,7,1);
 for(unsigned level=0;level<3;++level){result.bands.push_back(correlate(low,filters.orientation,9,1));low=correlate(low,filters.lo,13,2);}
 result.bands.push_back(std::move(low));for(size_t b=0;b<result.bands.size();++b)for(double& v:result.bands[b].values)v/=double(1u<<b);
 auto& base=result.bands.back();result.base_pre=base.values;double sum=0,correction=0;for(unsigned x=0;x<base.width;++x)for(unsigned y=0;y<base.height;++y){const double value=base.values[size_t(y)*base.width+x],next=sum+value;correction+=std::abs(sum)>=std::abs(value)?(sum-next)+value:(value-next)+sum;sum=next;}result.base_mean=(sum+correction)/double(base.values.size());for(double& v:base.values)v-=result.base_mean;return result;
}
}
