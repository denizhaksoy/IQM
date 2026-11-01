#include <IQM/hdrvdp3/general_quality_double.h>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace IQM::HDRVDP3;
int main(){try{unsigned rejected=0;GeneralQualityConfigDouble c{80,64,30,24,GeneralEncodingDouble::NativeRGB};std::vector<double> reference(80*64*3,1.),test=reference;XYZDisplaySamples xyz;RGBSpectralSamples spectral;GeneralFFTBackendDouble backend;
 auto reject=[&](auto call,const char* expected){bool caught=false;try{call();}catch(const std::exception& e){if(std::string(e.what()).find(expected)==std::string::npos)throw;caught=true;}if(!caught)throw std::runtime_error("Invalid configuration accepted");++rejected;};
 auto evaluate=[&]{evaluate_general_quality_double(reference,test,c,xyz,spectral,backend);};
 reject(evaluate,"Explicit FFT backend");c.width=15;reject(evaluate,"geometry/viewing");c.width=80;c.height=0;reject(evaluate,"geometry/viewing");c.height=64;c.ppd=25;reject(evaluate,"geometry/viewing");c.ppd=257;reject(evaluate,"geometry/viewing");c.ppd=std::numeric_limits<double>::quiet_NaN();reject(evaluate,"geometry/viewing");c.ppd=30;c.age=-1;reject(evaluate,"geometry/viewing");c.age=91;reject(evaluate,"geometry/viewing");c.age=24;c.encoding=static_cast<GeneralEncodingDouble>(999);reject(evaluate,"Unknown general Double encoding");c.encoding=GeneralEncodingDouble::NativeRGB;reference.pop_back();reject(evaluate,"shape mismatch");reference=test;reference[0]=std::numeric_limits<double>::infinity();reject(evaluate,"nonfinite");reference[0]=-1;reject(evaluate,"input encoding domain");reference[0]=1.01;c.encoding=GeneralEncodingDouble::SRGBDisplay;reject(evaluate,"input encoding domain");
 std::cout<<rejected<<" invalid API/viewing/input inputs rejected;not numerical parity proof\n";return rejected==13?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
