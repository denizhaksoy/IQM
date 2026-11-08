#include <IQM/hdrvdp3/fftw_single_provider.h>
#include <fstream>
#include <iostream>
struct Trig {void(*cexp)(Trig*,ptrdiff_t,float*);void* cexpl;void* rotate;ptrdiff_t shift,radix,mask;void* W0;void* W1;ptrdiff_t n;};
int main(int argc,char**argv){try{if(argc!=3)throw std::runtime_error("Usage DLL fresh-output");IQM::HDRVDP3::FFTWProviderSingle api(argv[1]);auto make=api.symbol<Trig*(*)(int,ptrdiff_t)>("fftwf_mktriggen");auto destroy=api.symbol<void(*)(Trig*)>("fftwf_triggen_destroy");std::filesystem::path out=argv[2];if(std::filesystem::exists(out))throw std::runtime_error("Fresh output required");std::filesystem::create_directories(out);for(int mode:{2,3}){auto p=make(mode,720);std::ofstream f(out/(std::to_string(mode)+".f32"),std::ios::binary);for(int k=0;k<48;k++)for(int j=1;j<15;j++){float w[2];p->cexp(p,k*j,w);f.write((char*)w,8);}destroy(p);}return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
