#include <IQM/hdrvdp3/resize.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace IQM::HDRVDP3 {
static int symmetric(int i,int n) {
    int p=i%(2*n); if(p<0) p+=2*n;
    return p<n?p:2*n-1-p;
}
static double cubic(double x) {
    x=std::abs(x); double s=x*x,c=s*x;
    if(x<=1) return 1.5*c-2.5*s+1;
    if(x<=2) return -0.5*c+2.5*s-4*x+2;
    return 0;
}
ResizeAxisPlan make_resize_axis_plan(int n,int m) {
    if(n<=0||m<=0) throw std::invalid_argument("Invalid resize axis");
    const double scale=double(m)/n, factor=std::min(1.0,scale);
    int padding=int(std::ceil(2/factor))+2;
    ResizeAxisPlan plan;
    plan.taps=2*padding;
    plan.indices.resize(size_t(m)*plan.taps);
    plan.weights.resize(size_t(m)*plan.taps);
    plan.normalization.resize(m);
    for(int d=0;d<m;++d) {
        const double coordinate=0.5+1/scale/2+double(d)/scale;
        const int base=int(std::floor(coordinate)); const double fraction=coordinate-base;
        double sum=0;
        for(int shift=1-padding;shift<=padding;++shift) {
            const size_t i=size_t(d)*plan.taps+(shift+padding-1);
            plan.weights[i]=factor*cubic(factor*(shift-fraction));
            plan.indices[i]=symmetric(base+shift-1,n);
            sum+=plan.weights[i];
        }
        plan.normalization[d]=sum==0?1:sum;
    }
    return plan;
}
template<class T> static std::vector<T> axis(const std::vector<T>& in,int h,int w,int nh,int nw,bool horizontal) {
    int n=horizontal?w:h, m=horizontal?nw:nh;
    if(n==m) return in;
    const auto plan=make_resize_axis_plan(n,m);
    std::vector<T> out(size_t(nh)*nw);
    for(int y=0;y<nh;++y) for(int x=0;x<nw;++x) {
        int d=horizontal?x:y;
        T sum=0;
        for(int tap=0;tap<plan.taps;++tap) {
            const size_t i=size_t(d)*plan.taps+tap;
            const double weight=plan.weights[i];
            int source=plan.indices[i];
            T value=horizontal?in[size_t(y)*w+source]:in[size_t(source)*w+x];
            // Octave mixed double/single multiplication and addition return single.
            T product=value*static_cast<T>(weight);
            sum=sum+product;
        }
        out[size_t(y)*nw+x]=sum/static_cast<T>(plan.normalization[d]);
    }
    return out;
}
std::vector<float> resize_bicubic(const std::vector<float>& input,int h,int w,int nh,int nw) {
    if(h<=0||w<=0||nh<=0||nw<=0||input.size()!=size_t(h)*w) throw std::invalid_argument("Invalid resize dimensions");
    auto horizontal=axis(input,h,w,h,nw,true);
    return axis(horizontal,h,nw,nh,nw,false);
}
std::vector<double> resize_bicubic(const std::vector<double>& input,int h,int w,int nh,int nw) {
    if(h<=0||w<=0||nh<=0||nw<=0||input.size()!=size_t(h)*w) throw std::invalid_argument("Invalid resize dimensions");
    auto horizontal=axis(input,h,w,h,nw,true);
    return axis(horizontal,h,nw,nh,nw,false);
}
}
