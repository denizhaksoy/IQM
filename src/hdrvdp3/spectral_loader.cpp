#include <IQM/hdrvdp3/spectral.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace IQM::HDRVDP3 {
namespace {
struct Table {std::vector<double> x;std::vector<std::vector<double>> channels;};
Table read_table(const std::filesystem::path& path,size_t count) {
    std::ifstream in(path);if(!in)throw std::invalid_argument("Cannot open spectral data");
    Table table;table.channels.resize(count);std::string line;
    while(std::getline(in,line)) {
        if(line.find_first_not_of(" \t\r")==std::string::npos)continue;
        std::replace(line.begin(),line.end(),',',' ');std::istringstream row(line);
        double x;if(!(row>>x)||!std::isfinite(x)||(!table.x.empty()&&x<=table.x.back()))
            throw std::invalid_argument("Invalid spectral wavelengths");
        table.x.push_back(x);
        for(auto& channel:table.channels){double y;if(!(row>>y)||!std::isfinite(y))throw std::invalid_argument("Invalid spectral data");channel.push_back(y);}
        std::string extra;if(row>>extra)throw std::invalid_argument("Unexpected spectral columns");
    }
    if(table.x.size()<3)throw std::invalid_argument("Insufficient spectral samples");
    return table;
}
int sign_product(double a,double b){return (a==0||b==0)?0:(std::signbit(a)==std::signbit(b)?1:-1);}
// Nonuniform SLATEC DPCHIM derivative ordering, matching the native pchip path.
std::vector<double> resample(const Table& t,size_t channel,const std::vector<double>& queries) {
    const auto& x=t.x;const auto& y=t.channels[channel];const size_t n=x.size();
    std::vector<double> d(n),h(n-1),delta(n-1),result;
    for(size_t i=0;i+1<n;++i){h[i]=x[i+1]-x[i];delta[i]=(y[i+1]-y[i])/h[i];}
    double hs=h[0]+h[1];
    d[0]=((h[0]+hs)/hs)*delta[0]+(-h[0]/hs)*delta[1];
    if(sign_product(d[0],delta[0])<=0)d[0]=0;
    else if(sign_product(delta[0],delta[1])<0&&std::abs(d[0])>std::abs(3*delta[0]))d[0]=3*delta[0];
    for(size_t i=1;i+1<n;++i) {
        const double a=delta[i-1],b=delta[i];hs=h[i-1]+h[i];
        if(sign_product(a,b)>0) {
            const double denom=hs+hs+hs,w1=(hs+h[i-1])/denom,w2=(hs+h[i])/denom;
            const double largest=std::max(std::abs(a),std::abs(b)),smallest=std::min(std::abs(a),std::abs(b));
            d[i]=smallest/(w1*(a/largest)+w2*(b/largest));
        }
    }
    d[n-1]=(-h[n-2]/hs)*delta[n-3]+((h[n-2]+hs)/hs)*delta[n-2];
    if(sign_product(d[n-1],delta[n-2])<=0)d[n-1]=0;
    else if(sign_product(delta[n-3],delta[n-2])<0&&std::abs(d[n-1])>std::abs(3*delta[n-2]))d[n-1]=3*delta[n-2];
    for(double q:queries) {
        if(q<x.front()||q>x.back()){result.push_back(0);continue;}
        const size_t i=std::min(size_t(std::upper_bound(x.begin(),x.end(),q)-x.begin()-1),n-2);
        const double dl=(d[i]-delta[i])/h[i],dr=(d[i+1]-delta[i])/h[i];
        const double c3=dl+dr,c2=-c3-dl,offset=q-x[i];
        result.push_back((((c3/h[i])*offset+c2)*offset+d[i])*offset+y[i]);
    }
    return result;
}
}
SpectralSamples load_d65_spectral_samples(const std::filesystem::path& cone_path,
    const std::filesystem::path& rod_path,const std::filesystem::path& emission_path) {
    const auto cone=read_table(cone_path,3),rod=read_table(rod_path,1),emission=read_table(emission_path,1);
    SpectralSamples out;out.wavelength.resize(420);out.sensitivity.resize(420);
    const double step=(780.0-360.0)/419;
    for(size_t i=0;i<210;++i){out.wavelength[i]=360+i*step;out.wavelength[419-i]=780-i*step;}
    std::array<std::vector<double>,3> cones;
    double smallest=0;
    for(size_t ch=0;ch<3;++ch){cones[ch]=resample(cone,ch,out.wavelength);for(double value:cones[ch])smallest=std::min(smallest,value);}
    const auto rods=resample(rod,0,out.wavelength);
    out.emission=resample(emission,0,out.wavelength);
    for(size_t i=0;i<420;++i){
        for(size_t ch=0;ch<3;++ch)out.sensitivity[i][ch]=std::pow(10.0,cones[ch][i]==0?smallest:cones[ch][i]);
        out.sensitivity[i][3]=rods[i];
    }
    return out;
}
}
