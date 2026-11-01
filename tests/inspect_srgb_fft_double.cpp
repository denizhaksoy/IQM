// Local native-library diagnostic only; not linked into production IQM.
#define NOMINMAX
#include <windows.h>
#include <array>
#include <algorithm>
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <IQM/hdrvdp3/srgb_display.h>
#include <IQM/hdrvdp3/mtf.h>
#include <IQM/hdrvdp3/jnd.h>
#include <IQM/hdrvdp3/sp0_double.h>
#include <IQM/hdrvdp3/quality_double.h>
#include <map>
#include <iomanip>
namespace fs=std::filesystem;
using Complex=std::array<double,2>;
struct FFTW {
 HMODULE dll=nullptr;
 void*(*allocate)(size_t);void(*release)(void*);int(*init)();void(*threads)(int);void(*forget)();
 void*(*real_plan)(int,int,double*,Complex*,unsigned);void*(*complex_plan)(int,int,Complex*,Complex*,int,unsigned);
 void(*execute)(void*);void(*destroy)(void*);char*(*print)(void*);
 template<class T>T symbol(const char* n){auto p=GetProcAddress(dll,n);if(!p)throw std::runtime_error(n);return reinterpret_cast<T>(p);}
 FFTW(){dll=LoadLibraryExW(L"C:\\Program Files\\GNU Octave\\Octave-11.3.0\\mingw64\\bin\\libfftw3-3.dll",nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);if(!dll)throw std::runtime_error("Cannot load native diagnostic FFTW");
  allocate=symbol<decltype(allocate)>("fftw_malloc");release=symbol<decltype(release)>("fftw_free");init=symbol<decltype(init)>("fftw_init_threads");threads=symbol<decltype(threads)>("fftw_plan_with_nthreads");forget=symbol<decltype(forget)>("fftw_forget_wisdom");real_plan=symbol<decltype(real_plan)>("fftw_plan_dft_r2c_2d");complex_plan=symbol<decltype(complex_plan)>("fftw_plan_dft_2d");execute=symbol<decltype(execute)>("fftw_execute");destroy=symbol<decltype(destroy)>("fftw_destroy_plan");print=symbol<decltype(print)>("fftw_sprint_plan");if(!init())throw std::runtime_error("FFTW threads init failed");threads(3);
 }
 ~FFTW(){if(dll)FreeLibrary(dll);}
};
struct Buffer{FFTW& api;void* p;Buffer(FFTW& a,size_t bytes):api(a),p(a.allocate(bytes)){if(!p)throw std::bad_alloc();}~Buffer(){api.release(p);}};
template<class T>std::vector<T> read(const fs::path& p,size_t count){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()!=std::streamoff(count*sizeof(T)))throw std::runtime_error("Capture shape "+p.string());std::vector<T> v(count);f.seekg(0);f.read(reinterpret_cast<char*>(v.data()),count*sizeof(T));return v;}
std::vector<double> rows(const std::vector<double>& v){auto out=v;for(size_t c=0;c<v.size()/9216;++c)for(size_t y=0;y<96;++y)for(size_t x=0;x<96;++x)out[c*9216+y*96+x]=v[c*9216+x*96+y];return out;}
int main(int argc,char**argv){try{
 if(argc!=3&&argc!=4&&argc!=6)throw std::runtime_error("Usage: native-captures fresh-output [unaligned [raw-data original-inputs]]");const bool independent=argc==6;const unsigned flags=64|16|(argc>=4?2:0);fs::path native=argv[1],out=argv[2];if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 FFTW api;constexpr size_t N=192,Count=N*N,Half=N/2+1;Buffer realMem(api,Count*8),halfMem(api,N*Half*16),complexMem(api,Count*16),inverseMem(api,Count*16);
 struct OwnedSide{std::vector<double> rgb,adaptation;IQM::HDRVDP3::SP0PyramidDouble pyramid;};std::map<std::string,std::array<OwnedSide,2>> owned;
 auto real=static_cast<double*>(realMem.p);auto half=static_cast<Complex*>(halfMem.p);auto complex=static_cast<Complex*>(complexMem.p);auto inverse=static_cast<Complex*>(inverseMem.p);
 std::ofstream log(out/"results.tsv");log<<"display\tside\tconv\tforward_bits\tforward_numeric\tinverse_bits\tinverse_numeric\n";size_t forwardBits=0,inverseBits=0,forwardNumeric=0,inverseNumeric=0,paddingBits=0,filterBits=0,adaptationBits=0;std::vector<double> optical(3*9216),sourceRGB,adaptationLog;IQM::HDRVDP3::RGBSpectralSamples spectral;
 for(std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"})for(int side=1;side<=2;++side)for(int conv=1;conv<=4;++conv){const auto prefix=(native/(display+"_"+std::to_string(side)+"_conv"+std::to_string(conv))).string();auto padded=read<double>(prefix+"_padded.f64",Count);auto expected=read<Complex>(prefix+"_fft.f64",Count);auto filter=read<double>(prefix+"_filter.f64",Count);auto spatial=read<double>(prefix+"_inverse.f64",Count);
  if(independent){
   const fs::path data=argv[4],inputs=argv[5];
   if(conv==1){auto encoded=read<float>(inputs/(display+"_"+std::to_string(side)+"_input.f32"),27648);auto xyz=IQM::HDRVDP3::load_xyz_display_samples(data/"ciexyz31.csv",data/("emission_spectra_"+display+".csv"));sourceRGB=IQM::HDRVDP3::prepare_srgb_display(encoded,xyz).native_rgb;spectral=IQM::HDRVDP3::load_rgb_spectral_samples(data/"log_cone_smith_pokorny_1975.csv",data/"cie_scotopic_lum.txt",data/("emission_spectra_"+display+".csv"));}
   if(conv==4)adaptationLog=IQM::HDRVDP3::project_optical_rgb_double(optical,spectral,24).adaptation_log;
   std::vector<double> generatedFilter(Count);if(conv<4)IQM::HDRVDP3::generate_mtf_filter(generatedFilter.data(),N,N,30);else{const double sigma=std::pow(10.,-.781367)*30;for(size_t x=0;x<N;++x)for(size_t y=0;y<N;++y){double kx=(std::fmod(.5+double(x)/N,1.)-.5)*(2*3.14159265358979323846),ky=(std::fmod(.5+double(y)/N,1.)-.5)*(2*3.14159265358979323846);generatedFilter[x*N+y]=std::exp((-.5*(kx*kx+ky*ky))*(sigma*sigma));}}
   for(size_t x=0;x<N;++x)for(size_t y=0;y<N;++y){size_t sx=x<96?x:(conv<4?191-x:95),sy=y<96?y:(conv<4?191-y:95);const double value=conv<4?sourceRGB[(conv-1)*9216+sx*96+sy]:adaptationLog[sx*96+sy];paddingBits+=std::bit_cast<uint64_t>(value)!=std::bit_cast<uint64_t>(padded[x*N+y]);padded[x*N+y]=value;}
   for(size_t i=0;i<Count;++i)filterBits+=std::bit_cast<uint64_t>(generatedFilter[i])!=std::bit_cast<uint64_t>(filter[i]);filter=std::move(generatedFilter);
  }
  api.forget();std::copy(padded.begin(),padded.end(),real);auto p=api.real_plan(N,N,real,half,flags);if(!p)throw std::runtime_error("Forward plan");if(side==1&&conv==1){char* text=api.print(p);std::ofstream(out/"forward_plan.txt")<<text;/* Foreign CRT string retained until diagnostic process exit. */}api.execute(p);api.destroy(p);
  for(size_t x=0;x<N;++x)for(size_t y=0;y<N;++y){Complex z;if(y<Half)z=half[x*Half+y];else{z=half[((N-x)%N)*Half+N-y];z[1]=-z[1];}complex[x*N+y]=z;}
  size_t fb=0,fn=0,ib=0,in=0;
  for(size_t i=0;i<Count;++i)for(size_t c=0;c<2;++c){fb+=std::bit_cast<uint64_t>(complex[i][c])!=std::bit_cast<uint64_t>(expected[i][c]);fn+=complex[i][c]!=expected[i][c];}
  // Isolate inverse arithmetic from forward errors with native FFT inputs.
  for(size_t i=0;i<Count;++i)for(size_t c=0;c<2;++c)complex[i][c]=(independent?complex[i][c]:expected[i][c])*filter[i];
  p=api.complex_plan(N,N,complex,inverse,1,flags);if(!p)throw std::runtime_error("Inverse plan");if(side==1&&conv==1){char* text=api.print(p);std::ofstream(out/"inverse_plan.txt")<<text;/* Foreign CRT string retained until diagnostic process exit. */}api.execute(p);api.destroy(p);
  for(size_t i=0;i<Count;++i){double value=inverse[i][0]/double(Count);ib+=std::bit_cast<uint64_t>(value)!=std::bit_cast<uint64_t>(spatial[i]);in+=value!=spatial[i];}
  if(independent){std::vector<double> cropped(9216);for(size_t x=0;x<96;++x)for(size_t y=0;y<96;++y)cropped[x*96+y]=inverse[x*N+y][0]/double(Count);if(conv<4){for(size_t i=0;i<9216;++i)optical[(conv-1)*9216+i]=std::clamp(cropped[i],1e-5,1e10);}else{auto actual=IQM::HDRVDP3::finish_local_adaptation_double(cropped);auto gold=read<double>(native/(display+"_"+std::to_string(side)+"_adaptation.f64"),9216);for(size_t i=0;i<9216;++i)adaptationBits+=std::bit_cast<uint64_t>(actual[i])!=std::bit_cast<uint64_t>(gold[i]);
   const auto clamped=IQM::HDRVDP3::project_optical_rgb_double(optical,spectral,24).clamped;
   const auto jnd=IQM::HDRVDP3::rgb_to_jnd_response_double(IQM::HDRVDP3::create_jnd_lut(),rows(clamped),rows(actual),96,96,96.*96./30./30.,24);
   size_t errors=0,words=0;const auto sidePrefix=(native/(display+"_"+std::to_string(side))).string();
   for(auto item:{std::pair{"post_pupil",&jnd.post_pupil},std::pair{"jnd",&jnd.channels},std::pair{"P",&jnd.response}}){auto expected=rows(read<double>(sidePrefix+"_"+item.first+".f64",item.second->size()));for(size_t i=0;i<expected.size();++i){errors+=std::bit_cast<uint64_t>(expected[i])!=std::bit_cast<uint64_t>((*item.second)[i]);++words;}}
   std::ofstream result(out/(display+"_"+std::to_string(side)+"_JND.txt"));result<<errors<<" bit errors /"<<words<<" words;independent original-input to P\n";if(errors)throw std::runtime_error("Independent Double JND mismatch");
   const auto pyramid=IQM::HDRVDP3::build_sp0_pyramid_double(jnd.response);size_t pyramidErrors=0,pyramidWords=0;
   auto comparePyramid=[&](const std::vector<double>&a,const std::vector<double>&e){for(size_t i=0;i<a.size();++i){pyramidErrors+=std::bit_cast<uint64_t>(a[i])!=std::bit_cast<uint64_t>(e[i]);++pyramidWords;}};
   for(size_t b=0;b<pyramid.bands.size();++b){const auto& band=pyramid.bands[b];auto expected=read<double>(sidePrefix+"_band"+std::to_string(b+1)+".f64",band.values.size());auto row=expected;for(unsigned y=0;y<band.height;++y)for(unsigned x=0;x<band.width;++x)row[y*band.width+x]=expected[x*band.height+y];comparePyramid(band.values,row);}
   auto pre=read<double>(sidePrefix+"_base_pre.f64",144);auto row=pre;for(unsigned y=0;y<12;++y)for(unsigned x=0;x<12;++x)row[y*12+x]=pre[x*12+y];comparePyramid(pyramid.base_pre,row);comparePyramid({pyramid.base_mean},read<double>(sidePrefix+"_base_mean.f64",1));
   std::ofstream(out/(display+"_"+std::to_string(side)+"_pyramid.txt"))<<pyramidErrors<<" bit errors /"<<pyramidWords<<" words;independent original-input through normalized/DC-corrected pyramid\n";if(pyramidErrors)throw std::runtime_error("Independent Double pyramid mismatch");
   owned[display][side-1]={rows(sourceRGB),rows(actual),pyramid};
  }}
  forwardBits+=fb;forwardNumeric+=fn;inverseBits+=ib;inverseNumeric+=in;log<<display<<'\t'<<side<<'\t'<<conv<<'\t'<<fb<<'\t'<<fn<<'\t'<<ib<<'\t'<<in<<'\n';
 }
 if(independent)for(const auto& [display,pair]:owned){
  size_t qualityWords=0,qualityErrors=0;std::ofstream qualityLog(out/(display+"_full_quality.tsv"));
  auto compare=[&](const std::string& field,const std::vector<double>& a,const std::vector<double>& e){size_t errors=0;for(size_t i=0;i<a.size();++i){errors+=std::bit_cast<uint64_t>(a[i])!=std::bit_cast<uint64_t>(e[i]);++qualityWords;}qualityErrors+=errors;qualityLog<<field<<'\t'<<errors<<'/'<<a.size()<<'\n';};
  auto rowSquare=[](std::vector<double> v,unsigned w){auto row=v;for(unsigned y=0;y<w;++y)for(unsigned x=0;x<w;++x)row[y*w+x]=v[x*w+y];return row;};
  const auto& reference=pair[0];const auto& test=pair[1];std::vector<std::vector<double>> self;for(unsigned b=0;b<5;++b){const auto& r=reference.pyramid.bands[b];self.push_back(IQM::HDRVDP3::mutual_mask_double(r.values,test.pyramid.bands[b].values,r.width,r.height));}
  // Actual base-band FFT, no captured frequency/index supplied to calculation.
  for(unsigned x=0;x<12;++x)for(unsigned y=0;y<12;++y)real[x*12+y]=test.pyramid.bands[4].values[y*12+x]-reference.pyramid.bands[4].values[y*12+x];
  api.forget();auto basePlan=api.real_plan(12,12,real,half,flags);if(!basePlan)throw std::runtime_error("Base FFT plan failed");api.execute(basePlan);api.destroy(basePlan);
  std::vector<double> spectrum(288);size_t selected=0;double maximum=0;
  for(unsigned x=0;x<12;++x)for(unsigned y=0;y<12;++y){auto z=y<7?half[x*7+y]:half[((12-x)%12)*7+12-y];if(y>=7)z[1]=-z[1];if(x==0&&y==0)z={0.,0.};size_t i=x*12+y;spectrum[2*i]=z[0];spectrum[2*i+1]=z[1];const double magnitude=std::hypot(z[0],z[1]);if(magnitude>maximum){maximum=magnitude;selected=i;}}
  // Octave simplifies an entirely real FFT result to real storage. Its
  // exported imaginary component is then +0, including conjugate partners.
  bool realSpectrum=true;for(size_t i=0;i<144;++i)if(spectrum[2*i+1]!=0.)realSpectrum=false;
  if(realSpectrum)for(size_t i=0;i<144;++i)spectrum[2*i+1]=0.;
  const auto axis=[](unsigned k){const double normalized=std::fmod(.5+double(k)/12.,1.)-.5;return (normalized*1.875)*2.;};const double fx=axis(unsigned(selected/12)),fy=axis(unsigned(selected%12));const double frequency=std::sqrt(fx*fx+fy*fy);
  const auto basePrefix=(native/(display+"_quality_band5")).string();compare("base_fft",spectrum,read<double>(basePrefix+"_base_fft.f64",288));
  const auto meanLog=IQM::HDRVDP3::quality_log_adaptation_double(reference.adaptation,test.adaptation);std::vector<double> bandErrors;double accumulated=0;
  for(unsigned b=0;b<5;++b){const unsigned w=reference.pyramid.bands[b].width;const auto prefix=(native/(display+"_quality_band"+std::to_string(b+1))).string();auto expected=[&](std::string field){return rowSquare(read<double>(prefix+"_"+field+".f64",size_t(w)*w),w);};
   const auto neighbor=IQM::HDRVDP3::quality_neighbor_mask_double(self,b);const auto csf=IQM::HDRVDP3::quality_csf_double(meanLog,b,frequency);const auto comparison=IQM::HDRVDP3::quality_comparison_double(reference.rgb,test.rgb,b);std::vector<double> difference(size_t(w)*w);for(size_t i=0;i<difference.size();++i)difference[i]=test.pyramid.bands[b].values[i]-reference.pyramid.bands[b].values[i];const auto result=IQM::HDRVDP3::evaluate_quality_band_double(difference,self[b],neighbor,csf.noise,comparison,w,w);
   for(auto item:{std::pair{"self",static_cast<const std::vector<double>*>(&self[b])},std::pair{"neighbor",&neighbor},std::pair{"log_adaptation",&csf.log_adaptation},std::pair{"csf",&csf.sensitivity},std::pair{"noise",&csf.noise},std::pair{"comparison",&comparison},std::pair{"mask_noise",&result.mask_noise},std::pair{"excitation",&result.excitation},std::pair{"difference",&result.difference},std::pair{"psychometric",&result.psychometric},std::pair{"masked",&result.masked}})compare(item.first,*item.second,expected(item.first));
   compare("csf_table",{csf.table.begin(),csf.table.end()},read<double>(prefix+"_csf_table.f64",256));const auto parameters=IQM::HDRVDP3::quality_mask_parameters_double();compare("parameters",{parameters.begin(),parameters.end()},read<double>(prefix+"_parameters.f64",6));compare("band_error",{result.error},read<double>(prefix+"_error.f64",1));bandErrors.push_back(result.error);accumulated+=result.error/5.;compare("accumulated",{accumulated},read<double>(prefix+"_accumulated.f64",1));if(b==4)compare("base_metadata",{double(selected+1),frequency,csf.base_luminance},read<double>(prefix+"_base_metadata.f64",3));
  }
  const auto scores=IQM::HDRVDP3::quality_scores_double(bandErrors);compare("scores",{scores.error,scores.Q,scores.Q_JOD},read<double>(native/(display+"_quality.f64"),3));std::ofstream(out/(display+"_full_quality_summary.txt"))<<qualityErrors<<" bit errors /"<<qualityWords<<" Double words;Q="<<std::setprecision(17)<<scores.Q<<" Q_JOD="<<scores.Q_JOD<<'\n';if(qualityErrors)throw std::runtime_error("Independent full Double quality mismatch");
 }
 std::cout<<"Forward bit/numeric errors "<<forwardBits<<'/'<<forwardNumeric<<" of "<<24*Count*2<<"; inverse "<<inverseBits<<'/'<<inverseNumeric<<" of "<<24*Count<<'\n';
 if(independent)std::cout<<"Independent original-input padding errors "<<paddingBits<<" filter errors "<<filterBits<<" final adaptation errors "<<adaptationBits<<" /55296\n";
 std::ofstream(out/"scope.txt")<<(independent?"Independent original Single image to Double Q/Q_JOD CPU diagnostic using explicitly local Octave FFTW DLL;captures used only for comparisons. Exact within tested96x96/30ppd/age24/defaultsp0 configurations. Not production FFT backend,GPU or whole-metric coverage.\n":"Same native FFTW DLL diagnostic;forward uses captured padded input,inverse uses captured FFT/filter. Not independent optical production acceptance.\n");
 std::ofstream(out/"summary.txt")<<"forward_bits="<<forwardBits<<" inverse_bits="<<inverseBits<<" padding_bits="<<paddingBits<<" filter_bits="<<filterBits<<" adaptation_bits="<<adaptationBits<<" independent="<<independent<<'\n';
 if(independent){if(forwardBits||inverseBits||paddingBits||filterBits||adaptationBits)return 2;std::ofstream(out/"PASS.txt")<<"Independent original Single input through Double Q/Q_JOD bitwise exact;all three spectra full quality summaries passed. Local native FFTW diagnostic,not production FFT/GPU or whole-metric coverage. Frontend5106246Doublewords plus fullquality summaries.\n";}
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
