// Explicit CPU numerical diagnostic provider, no Octave scorer or tensor injection.
#pragma once
#define NOMINMAX
#include <windows.h>
#include <IQM/hdrvdp3/fft_single.h>
#include <array>
#include <filesystem>
#include <cstring>
#include <stdexcept>
struct SingleFFTW {
 using Complex=std::array<float,2>;
 HMODULE dll=nullptr;void*(*allocate)(size_t);void(*release)(void*);int(*init)();void(*threads)(int);void(*forget)();void*(*realPlan)(int,int,float*,Complex*,unsigned);void*(*complexPlan)(int,int,Complex*,Complex*,int,unsigned);void(*execute)(void*);void(*destroy)(void*);
 template<class T>T symbol(const char* n){auto p=GetProcAddress(dll,n);if(!p)throw std::runtime_error(n);return reinterpret_cast<T>(p);}
 explicit SingleFFTW(const std::filesystem::path& path){auto native=std::filesystem::absolute(path);native.make_preferred();dll=LoadLibraryExW(native.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);if(!dll)throw std::runtime_error("Cannot load Single FFTW provider;Windows error "+std::to_string(GetLastError()));allocate=symbol<decltype(allocate)>("fftwf_malloc");release=symbol<decltype(release)>("fftwf_free");init=symbol<decltype(init)>("fftwf_init_threads");threads=symbol<decltype(threads)>("fftwf_plan_with_nthreads");forget=symbol<decltype(forget)>("fftwf_forget_wisdom");realPlan=symbol<decltype(realPlan)>("fftwf_plan_dft_r2c_2d");complexPlan=symbol<decltype(complexPlan)>("fftwf_plan_dft_2d");execute=symbol<decltype(execute)>("fftwf_execute");destroy=symbol<decltype(destroy)>("fftwf_destroy_plan");if(!init())throw std::runtime_error("FFTW threading init failed");threads(3);}
 ~SingleFFTW(){if(dll)FreeLibrary(dll);}
 struct Memory{SingleFFTW& api;void* data;Memory(SingleFFTW&a,size_t n):api(a),data(a.allocate(n)){if(!data)throw std::bad_alloc();}~Memory(){api.release(data);}};
 std::vector<float> forward(std::span<const float> values,unsigned w,unsigned h){size_t n=size_t(w)*h;if(values.size()!=n)throw std::runtime_error("Single FFT input shape");size_t halfH=h/2+1;Memory src(*this,n*4),half(*this,size_t(w)*halfH*8);std::memcpy(src.data,values.data(),n*4);forget();auto plan=realPlan(int(w),int(h),static_cast<float*>(src.data),static_cast<Complex*>(half.data),64|16|2);if(!plan)throw std::runtime_error("FFTW Single forward plan");execute(plan);destroy(plan);auto reduced=static_cast<Complex*>(half.data);std::vector<float> full(2*n);for(unsigned x=0;x<w;++x)for(unsigned y=0;y<h;++y){Complex z;if(y<halfH)z=reduced[size_t(x)*halfH+y];else{z=reduced[size_t((w-x)%w)*halfH+h-y];z[1]=-z[1];}full[2*(size_t(x)*h+y)]=z[0];full[2*(size_t(x)*h+y)+1]=z[1];}return full;}
 std::vector<float> convolve(std::span<const float> values,std::span<const double> filter,unsigned w,unsigned h,unsigned cropW,unsigned cropH){size_t n=size_t(w)*h;if(filter.size()!=n||cropW>w||cropH>h)throw std::runtime_error("Single convolution shape");auto spectrum=forward(values,w,h);Memory input(*this,n*8),output(*this,n*8);auto src=static_cast<Complex*>(input.data);for(size_t i=0;i<n;++i){src[i][0]=spectrum[2*i]*float(filter[i]);src[i][1]=spectrum[2*i+1]*float(filter[i]);}forget();auto plan=complexPlan(int(w),int(h),src,static_cast<Complex*>(output.data),1,64|16|2);if(!plan)throw std::runtime_error("FFTW Single inverse plan");execute(plan);destroy(plan);auto inverse=static_cast<Complex*>(output.data);std::vector<float> cropped(size_t(cropW)*cropH);for(unsigned x=0;x<cropW;++x)for(unsigned y=0;y<cropH;++y)cropped[size_t(x)*cropH+y]=inverse[size_t(x)*h+y][0]/float(n);return cropped;}
 IQM::HDRVDP3::FFTBackendSingle backend(){return {[this](auto v,auto f,unsigned w,unsigned h,unsigned cw,unsigned ch){return convolve(v,f,w,h,cw,ch);},[this](auto v,unsigned w,unsigned h){return forward(v,w,h);}};}
};
