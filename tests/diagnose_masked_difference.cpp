#include <IQM/hdrvdp3/quality.h>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
int main(int argc,char**argv) {
    if(argc!=2) return 2;
    std::ifstream in(argv[1]); float diff,sensitivity,mask,ex,neural,square,den,d;
    double p,pf;
    while(in>>diff>>sensitivity>>mask>>p>>pf>>ex>>neural>>square>>den>>d) {
        float raw; IQM::HDRVDP3::masked_difference(diff,sensitivity,mask,p,pf,-1,&raw);
        std::cout<<std::setprecision(17)<<"diff="<<diff<<" S="<<sensitivity<<" mask="<<mask<<'\n';
        const float excitation=std::copysign(std::pow(std::abs(diff),float(p)),diff);
        const float noise=std::pow(1.0f/sensitivity,float(2*p));
        const float sq=mask*mask;
        const float denominator=std::sqrt(noise+sq);
        std::cout<<"excitation "<<excitation<<" expected "<<ex<<"\nneural "<<noise<<" expected "<<neural
          <<"\nsquare "<<sq<<" expected "<<square<<"\ndenominator "<<denominator<<" expected "<<den
          <<"\nraw "<<raw<<" expected "<<d<<'\n';
    }
    return in.eof()?0:2;
}
