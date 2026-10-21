// Read-only inspection of the same FFTW DLL shipped with local Octave.
// This CPU utility is a diagnostic/reference tool, not a production fallback.
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cmath>

using Complex = float[2];
struct FFTW {
    HMODULE dll = nullptr;
    template<class T> T symbol(const char* name) {
        auto p = GetProcAddress(dll, name);
        if (!p) throw std::runtime_error(std::string("Missing FFTW symbol: ") + name);
        return reinterpret_cast<T>(p);
    }
    void* (*allocate)(size_t); void (*release)(void*);
    int (*init_threads)(); void (*threads)(int);
    void* (*real_plan)(int,int,float*,Complex*,unsigned);
    void* (*real_many_plan)(int,const int*,int,float*,const int*,int,int,Complex*,const int*,int,int,unsigned);
    void* (*complex_plan)(int,int,Complex*,Complex*,int,unsigned);
    void* (*complex_plan_1d)(int,Complex*,Complex*,int,unsigned);
    void* (*complex_many_plan)(int,const int*,int,Complex*,const int*,int,int,Complex*,const int*,int,int,int,unsigned);
    void (*execute)(const void*); void (*destroy)(void*);
    char* (*print)(const void*); void (*flops)(const void*,double*,double*,double*);
    int (*import_wisdom)(const char*); void (*forget_wisdom)();
    FFTW() {
        dll = LoadLibraryExW(L"C:\\Program Files\\GNU Octave\\Octave-11.3.0\\mingw64\\bin\\libfftw3f-3.dll",
            nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (!dll) throw std::runtime_error("Cannot load local Octave FFTW DLL: " + std::to_string(GetLastError()));
        allocate=symbol<decltype(allocate)>("fftwf_malloc"); release=symbol<decltype(release)>("fftwf_free");
        init_threads=symbol<decltype(init_threads)>("fftwf_init_threads"); threads=symbol<decltype(threads)>("fftwf_plan_with_nthreads");
        real_plan=symbol<decltype(real_plan)>("fftwf_plan_dft_r2c_2d");
        real_many_plan=symbol<decltype(real_many_plan)>("fftwf_plan_many_dft_r2c");
        complex_plan=symbol<decltype(complex_plan)>("fftwf_plan_dft_2d");
        complex_plan_1d=symbol<decltype(complex_plan_1d)>("fftwf_plan_dft_1d");
        complex_many_plan=symbol<decltype(complex_many_plan)>("fftwf_plan_many_dft");
        execute=symbol<decltype(execute)>("fftwf_execute"); destroy=symbol<decltype(destroy)>("fftwf_destroy_plan");
        print=symbol<decltype(print)>("fftwf_sprint_plan"); flops=symbol<decltype(flops)>("fftwf_flops");
        import_wisdom=symbol<decltype(import_wisdom)>("fftwf_import_wisdom_from_string");
        forget_wisdom=symbol<decltype(forget_wisdom)>("fftwf_forget_wisdom");
        if (!init_threads()) throw std::runtime_error("FFTW thread initialization failed");
    }
    ~FFTW() { if (dll) FreeLibrary(dll); }
};
struct Allocation {
    FFTW& api; void* data;
    Allocation(FFTW& a, size_t bytes) : api(a),data(a.allocate(bytes)) {
        if(!data) throw std::bad_alloc();
    }
    ~Allocation() { api.release(data); }
};
struct Plan {
    FFTW& api; void* data;
    ~Plan() { if(data) api.destroy(data); }
};
static std::vector<float> read(const std::filesystem::path& path,unsigned expected_h,unsigned expected_w) {
    std::ifstream f(path); unsigned h,w;
    if(!(f>>h>>w) || h!=expected_h || w!=expected_w) throw std::runtime_error("Invalid input shape: "+path.string());
    std::vector<float> values(h*w); for(auto& value:values) if(!(f>>value)) throw std::runtime_error("Truncated input");
    return values;
}
static void save(const std::filesystem::path& path,const std::vector<std::array<float,2>>& values,unsigned n) {
    std::ofstream f(path); if(!f) throw std::runtime_error("Cannot write diagnostic output");
    f<<n<<' '<<2*n<<'\n'<<std::setprecision(9);
    for(const auto& z:values) f<<z[0]<<'\n'<<z[1]<<'\n';
}
static int codelets(const std::filesystem::path& output,bool second=false,unsigned group=128,bool forward=false) {
    if(!std::filesystem::create_directory(output)) throw std::runtime_error("Output directory must be new");
    FFTW api; api.threads(1);
    std::ofstream plans(output/"codelet_plans.txt");
    if(second && group!=128 && group!=6 && group!=2 && group!=3 && group!=65) throw std::runtime_error("Unsupported group size");
    const unsigned batches=second?((512+group-1)/group)*group:512;
    uint32_t rng=0x7143a9u;
    for(unsigned n:{8u,16u,128u}) {
        if(second && n!=128) continue;
        const unsigned count=second?group:1;
        Allocation in_mem(api,n*count*sizeof(Complex)),out_mem(api,n*count*sizeof(Complex));
        auto* in=static_cast<Complex*>(in_mem.data); auto* out=static_cast<Complex*>(out_mem.data);
        if(second) out=in; // Native second-axis stage operates in place.
        const int dim=static_cast<int>(n);
        Plan p{api,second?api.complex_many_plan(1,&dim,count,in,nullptr,count,1,out,nullptr,count,1,forward?-1:1,64u|2u):api.complex_plan_1d(n,in,out,1,64u|2u)};
        if(!p.data) throw std::runtime_error("Cannot create scalar inverse plan");
        char* text=api.print(p.data); const std::string planText=text;
        plans<<(forward?"forward":"inverse")<<" n="<<n<<" group="<<count<<'\n'<<planText<<'\n'; std::free(text);
        const bool tail=forward && group==2;
        if(second && (tail?planText.find("n1fv_128_avx2_128")==std::string::npos:
            (planText.find(forward?"t3fv_8_avx2_128":"t3bv_16_avx2_128")==std::string::npos || planText.find(forward?"n2fv_16_avx2_128":"n1_8")==std::string::npos)))
            throw std::runtime_error("Second-axis oracle selected an unexpected plan; not a parity fixture");
        std::ofstream input(output/("input"+std::to_string(n)+".txt"));
        std::ofstream expected(output/((forward?"forward":"inverse")+std::to_string(n)+".txt"));
        input<<batches<<' '<<n*2<<'\n'<<std::setprecision(9);
        expected<<batches<<' '<<n*2<<'\n'<<std::setprecision(9);
        for(unsigned batch=0;batch<batches;++batch) {
            for(unsigned i=0;i<n;++i) for(unsigned component=0;component<2;++component) {
                rng^=rng<<13; rng^=rng>>17; rng^=rng<<5;
                const float scales[]={1e-3f,1.0f,10.0f,1e4f};
                float value=(static_cast<float>(rng>>8)/8388608.0f-1.0f)*scales[batch%4];
                if(batch==0) value=component==0?10.0f:0.0f;
                if(batch==1) value=i==3? (component==0?1.0f:-0.5f):0.0f;
                if(batch==2) value=(static_cast<float>(i)-n/2.0f)*(component?0.25f:1.0f);
                in[i*count+batch%count][component]=value; input<<value<<'\n';
            }
            if(batch%count==count-1) {
                api.execute(p.data);
                for(unsigned b=0;b<count;++b) for(unsigned i=0;i<n;++i)
                    expected<<out[i*count+b][0]<<'\n'<<out[i*count+b][1]<<'\n';
            }
        }
    }
    std::cout<<"FFTW "<<(forward?"forward":"inverse")<<" references saved to "<<output.string()<<'\n'; return 0;
}
static int transform2d(const std::filesystem::path& output,const std::string& source,bool forward=false) {
    if(!std::filesystem::create_directory(output)) throw std::runtime_error("Output directory must be new");
    FFTW api; api.threads(22);
    constexpr unsigned N=128,Count=N*N;
    Allocation in_mem(api,Count*sizeof(Complex)),out_mem(api,Count*sizeof(Complex));
    Allocation real_mem(api,Count*sizeof(float));
    auto* in=static_cast<Complex*>(in_mem.data); auto* out=static_cast<Complex*>(out_mem.data);
    auto* real=static_cast<float*>(real_mem.data);
    auto* half=out+N*((N-1)/2); const int dims[2]={N,N};
    Plan p{api,forward?api.real_many_plan(2,dims,1,real,nullptr,1,Count,half,nullptr,1,Count,64u|2u):api.complex_plan(N,N,in,out,1,64u|2u)};
    if(!p.data) throw std::runtime_error("Cannot create native 2D transform plan");
    char* text=api.print(p.data); const std::string planText=text; std::free(text);
    std::ofstream(output/(forward?"forward2d_plan.txt":"inverse2d_plan.txt"))<<planText<<'\n';
    if(forward?(planText.find("r2cf_128")==std::string::npos || planText.find("t3fv_8_avx2_128")==std::string::npos || planText.find("n2fv_16_avx2_128")==std::string::npos || planText.find("n1fv_128_avx2_128")==std::string::npos):
        (planText.find("t2_8")==std::string::npos || planText.find("t3bv_16_avx2_128")==std::string::npos))
        throw std::runtime_error("Unexpected native 2D transform decomposition");
    const unsigned matrices=source.empty()?7:4;
    std::ofstream input(output/"input128.txt"),expected(output/(forward?"forward128.txt":"inverse128.txt")),metadata(output/"source.txt");
    input<<matrices*N<<" 256\n"<<std::setprecision(9);
    expected<<matrices*N<<" 256\n"<<std::setprecision(9);
    metadata<<"column-major complex float32; threads=22; "<<(forward?"real forward, unnormalized":"inverse normalized by 16384 after both axes")<<'\n';
    uint32_t rng=0x7143a9u;
    for(unsigned m=0;m<matrices;++m) {
        if(source.empty()) {
            for(unsigned i=0;i<Count;++i) for(unsigned c=0;c<2;++c) {
                rng^=rng<<13; rng^=rng>>17; rng^=rng<<5;
                const float scales[]={1e-3f,1.0f,10.0f,1e4f};
                float v=(static_cast<float>(rng>>8)/8388608.0f-1.0f)*scales[m%4];
                if(m==0) v=c?0.0f:10.0f;
                if(m==1) v=i==3+128*19?(c?-0.5f:1.0f):0.0f;
                if(m==2) v=(static_cast<float>((i*17+i/128*23)%251)-125.0f)*(c?0.25f:1.0f);
                in[i][c]=v;
            }
            metadata<<"fixture "<<m<<'\n';
        } else {
            const auto filename=std::string(m%2?"test":"ref")+(m<2?"_mtf_stage":"_la_stage")+(forward?"0.txt":"2.txt");
            const auto data=read(std::filesystem::path(source)/filename,N,N*2);
            std::ifstream axis(std::filesystem::path(source)/"fft_axis_order.txt"); std::string order; axis>>order;
            if(order!="column" && order!="row") throw std::runtime_error("Missing axis order metadata");
            for(unsigned x=0;x<N;++x) for(unsigned y=0;y<N;++y) for(unsigned c=0;c<2;++c)
                in[x*N+y][c]=data[(order=="column"?x*N+y:y*N+x)*2+c];
            metadata<<std::filesystem::absolute(source).string()<<'/'<<filename<<" original="<<order<<'\n';
        }
        for(unsigned i=0;i<Count;++i) {
            if(forward) { in[i][1]=0; real[i]=in[i][0]; }
            input<<in[i][0]<<'\n'<<in[i][1]<<'\n';
        }
        api.execute(p.data);
        for(unsigned i=0;i<Count;++i) {
            if(forward) {
                const unsigned x=i/N,y=i%N; const bool conjugate=y>64;
                const auto index=conjugate?((N-x)%N)*65+N-y:x*65+y;
                expected<<half[index][0]<<'\n'<<(conjugate?-half[index][1]:half[index][1])<<'\n';
            } else expected<<out[i][0]/float(Count)<<'\n'<<out[i][1]/float(Count)<<'\n';
        }
    }
    std::cout<<"Native "<<(forward?"forward2d":"inverse2d")<<" fixtures: "<<matrices<<" matrices in "<<output.string()<<'\n';
    return 0;
}
static int r2c(const std::filesystem::path& output,const std::string& source) {
    if(!std::filesystem::create_directory(output)) throw std::runtime_error("Output directory must be new");
    FFTW api; api.threads(1);
    constexpr int N=128,Batches=512;
    Allocation in_mem(api,N*sizeof(float)),out_mem(api,65*sizeof(Complex));
    auto* in=static_cast<float*>(in_mem.data); auto* out=static_cast<Complex*>(out_mem.data);
    Plan p{api,api.real_many_plan(1,&N,1,in,nullptr,1,N,out,nullptr,1,65,64u|2u)};
    if(!p.data) throw std::runtime_error("Cannot create R2C plan");
    char* text=api.print(p.data); const std::string planText=text; std::free(text);
    std::ofstream(output/"r2c_plan.txt")<<planText<<'\n';
    if(planText.find("r2cf_128")==std::string::npos) throw std::runtime_error("Unexpected R2C oracle graph");
    std::ofstream input(output/"input128.txt"),expected(output/"forward128.txt"),metadata(output/"source.txt");
    input<<Batches<<" 256\n"<<std::setprecision(9); expected<<Batches<<" 256\n"<<std::setprecision(9);
    std::vector<float> imported;
    if(!source.empty()) {
        std::ifstream axis(std::filesystem::path(source)/"fft_axis_order.txt"); std::string order; axis>>order;
        if(order!="column" && order!="row") throw std::runtime_error("Missing axis order metadata");
        for(const char* kind:{"mtf","la"}) for(const char* label:{"ref","test"}) {
            const auto filename=std::string(label)+"_"+kind+"_stage0.txt";
            const auto values=read(std::filesystem::path(source)/filename,N,2*N);
            for(unsigned x=0;x<N;++x) for(unsigned y=0;y<N;++y)
                imported.push_back(values[(order=="column"?x*N+y:y*N+x)*2]);
            metadata<<std::filesystem::absolute(source).string()<<'/'<<filename<<" original="<<order<<'\n';
        }
    } else metadata<<"constant/impulse/ramp/seeded random, float32, no normalization\n";
    uint32_t rng=0x7143a9u;
    for(unsigned batch=0;batch<Batches;++batch) {
        for(unsigned i=0;i<N;++i) {
            rng^=rng<<13; rng^=rng>>17; rng^=rng<<5;
            const float scales[]={1e-3f,1.0f,10.0f,1e4f};
            float v=(static_cast<float>(rng>>8)/8388608.0f-1.0f)*scales[batch%4];
            if(batch==0) v=10.0f;
            if(batch==1) v=i==19?1.0f:0.0f;
            if(batch==2) v=float(i)-64.0f;
            if(!imported.empty()) v=imported[batch*N+i];
            in[i]=v; input<<v<<"\n0\n";
        }
        api.execute(p.data);
        for(unsigned i=0;i<N;++i) {const auto k=i<=64?i:N-i; expected<<out[k][0]<<'\n'<<(i<=64?out[k][1]:-out[k][1])<<'\n';}
    }
    std::cout<<"R2C references saved to "<<output.string()<<'\n'; return 0;
}
static int inverse256_axis(const std::filesystem::path& source,const std::filesystem::path& output,
    const std::filesystem::path& gpu) {
    if(!std::filesystem::create_directory(output))throw std::runtime_error("Fresh axis oracle directory required");
    constexpr int n=256;constexpr size_t count=size_t(n)*n;
    const auto input=read(source,n,2*n),actual=read(gpu,n,2*n);
    FFTW api;api.threads(3);api.forget_wisdom();
    Allocation in_mem(api,count*sizeof(Complex)),out_mem(api,count*sizeof(Complex));
    auto* in=static_cast<Complex*>(in_mem.data);auto* out=static_cast<Complex*>(out_mem.data);
    Plan plan{api,api.complex_many_plan(1,&n,n,in,nullptr,1,n,out,nullptr,1,n,1,64u|2u)};
    if(!plan.data)throw std::runtime_error("Cannot create inverse256 axis plan");
    char* printed=api.print(plan.data);const std::string text=printed;std::free(printed);
    std::ofstream(output/"plan.txt")<<text;
    if(text.find("t2_16")==std::string::npos||text.find("n1_16")==std::string::npos)
        throw std::runtime_error("Axis oracle selected a different native graph");
    for(size_t i=0;i<count;++i){in[i][0]=input[2*i];in[i][1]=input[2*i+1];}
    api.execute(plan.data);
    std::vector<std::array<float,2>> expected(count);size_t mismatches=0,subnormal_differences=0;double maximum=0;
    for(size_t y=0;y<n;++y)for(size_t x=0;x<n;++x)for(size_t c=0;c<2;++c){
        const float value=out[x*n+y][c];expected[y*n+x][c]=value;
        const float got=actual[2*(y*n+x)+c];
        if(got!=value){
            if(mismatches<5)std::cout<<"axis0 index="<<2*(y*n+x)+c<<" gpu="<<std::setprecision(9)<<got<<" native="<<value<<'\n';
            if((got==0||std::fpclassify(got)==FP_SUBNORMAL)&&(value==0||std::fpclassify(value)==FP_SUBNORMAL))++subnormal_differences;
            maximum=std::max(maximum,std::abs(double(got)-value));++mismatches;
        }
    }
    save(output/"axis0_expected.txt",expected,n);
    std::cout<<"axis0 values="<<2*count<<" mismatches="<<mismatches<<'\n';
    std::cout<<"axis0 subnormal_only_differences="<<subnormal_differences<<" max="<<std::setprecision(17)<<maximum<<'\n';
    return mismatches?1:0;
}
static int forward256_axes(const std::filesystem::path& source,const std::filesystem::path& output) {
    constexpr int n=256,half=129;constexpr size_t count=size_t(n)*n;
    const auto input=read(source,n,2*n);
    if(!std::filesystem::create_directory(output))throw std::runtime_error("Fresh forward oracle directory required");
    FFTW api;api.threads(3);api.forget_wisdom();
    Allocation real_mem(api,count*sizeof(float)),half_mem(api,size_t(n)*half*sizeof(Complex));
    auto* real=static_cast<float*>(real_mem.data);auto* spectrum=static_cast<Complex*>(half_mem.data);
    Plan first{api,api.real_many_plan(1,&n,n,real,nullptr,1,n,spectrum,nullptr,1,half,64u|2u)};
    if(!first.data)throw std::runtime_error("Cannot create real256 first-axis oracle");
    char* printed=api.print(first.data);const std::string firstText=printed;std::free(printed);
    std::ofstream(output/"axis0_plan.txt")<<firstText;
    for(const auto* name:{"hc2cf_32","r2cf_32","r2cfII_32","r2cf_8"})
        if(firstText.find(name)==std::string::npos)throw std::runtime_error("Unexpected real256 first-axis graph");
    Plan second{api,api.complex_many_plan(1,&n,half,spectrum,nullptr,half,1,spectrum,nullptr,half,1,-1,64u|2u)};
    if(!second.data)throw std::runtime_error("Cannot create complex256 second-axis oracle");
    printed=api.print(second.data);const std::string secondText=printed;std::free(printed);
    std::ofstream(output/"axis1_plan.txt")<<secondText;
    for(const auto* name:{"t3fv_16_avx2_128","n2fv_16_avx2_128"})
        if(secondText.find(name)==std::string::npos)throw std::runtime_error("Unexpected complex256 second-axis graph");
    for(size_t i=0;i<count;++i){
        if(!std::isfinite(input[2*i])||input[2*i+1]!=0)throw std::runtime_error("Expected finite real column-major input");
        real[i]=input[2*i];
    }
    api.execute(first.data);
    std::ofstream axis(output/"axis0_compact.txt");axis<<n<<' '<<2*half<<'\n'<<std::setprecision(9);
    for(size_t i=0;i<size_t(n)*half;++i)axis<<spectrum[i][0]<<'\n'<<spectrum[i][1]<<'\n';
    api.execute(second.data);
    std::vector<std::array<float,2>> full(count);
    for(unsigned y=0;y<n;++y)for(unsigned x=0;x<n;++x){
        const bool conjugate=y>=half;
        const size_t index=conjugate?((n-x)%n)*half+(n-y):x*half+y;
        full[y*n+x]={spectrum[index][0],conjugate?-spectrum[index][1]:spectrum[index][1]};
    }
    save(output/"forward_row_major.txt",full,n);
    std::ofstream(output/"layout.txt")<<"Input: 256x256 column-major complex text, imaginary zero.\n"
        <<"axis0_compact: 256 columns, each 129 interleaved complex float32 frequencies; no transpose.\n"
        <<"axis1: in-place complex FFT, stride129, distance1, howmany129.\n"
        <<"Final: 256x256 row-major interleaved complex, Hermitian half expanded; no normalization or DC removal.\n"
        <<"FFTW_ESTIMATE|FFTW_UNALIGNED, threads3, fresh wisdom; diagnostic only.\n";
    std::cout<<"Forward256 axis oracles saved to "<<output.string()<<'\n';return 0;
}
int main(int argc,char** argv) {
    if(argc==4&&std::string(argv[1])=="--forward256-axes") {
        try{return forward256_axes(argv[2],argv[3]);}
        catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
    }
    if(argc==5&&std::string(argv[1])=="--inverse256-axis") {
        try{return inverse256_axis(argv[2],argv[3],argv[4]);}
        catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
    }
    if((argc==3 || argc==4) && std::string(argv[1])=="--forward2d") {
        try { return transform2d(argv[2],argc==4?argv[3]:"",true); } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
    }
    if((argc==3 || argc==4) && std::string(argv[1])=="--forward-second") {
        try { return codelets(argv[2],true,argc==4?std::stoul(argv[3]):65,true); } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
    }
    if((argc==3 || argc==4) && std::string(argv[1])=="--r2c") {
        try { return r2c(argv[2],argc==4?argv[3]:""); } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
    }
    if((argc==3 || argc==4) && std::string(argv[1])=="--inverse2d") {
        try { return transform2d(argv[2],argc==4?argv[3]:""); } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
    }
    if((argc==3 || argc==4) && (std::string(argv[1])=="--codelets" || std::string(argv[1])=="--second-axis")) {
        try { return codelets(argv[2],std::string(argv[1])=="--second-axis",argc==4?std::stoul(argv[3]):128); } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
    }
    if(argc!=4 && argc!=5 && argc!=6) { std::cerr<<"Usage: IQM-fftw-plan-test <export dir> <new output dir> <native single threads> [native wisdom] [FFT side64,128 or256]\n"; return 2; }
    try {
        const unsigned N=argc==6?std::stoul(argv[5]):128;
        if(N!=64&&N!=128&&N!=256) throw std::runtime_error("Unsupported diagnostic FFT size");
        const unsigned Half=N/2+1; const size_t Count=size_t(N)*N;
        const std::filesystem::path source(argv[1]), output(argv[2]);
        const int native_threads=std::stoi(argv[3]);
        if(native_threads<=0) throw std::runtime_error("Thread count must be positive");
        if(!std::filesystem::create_directory(output)) throw std::runtime_error("Output directory must be new");
        FFTW api;
        std::string wisdom;
        if(argc>=5 && std::string(argv[4])!="-") { std::ifstream f(argv[4]); wisdom.assign(std::istreambuf_iterator<char>(f),{});
            if(wisdom.empty()) throw std::runtime_error("Cannot read native wisdom"); }
        std::ofstream summary(output/"plans.txt");
        std::ofstream(output/"single_threads.txt")<<native_threads<<'\n';
        std::ifstream axis(source/"fft_axis_order.txt"); std::string order; axis>>order;
        std::ofstream(output/"source.txt")<<std::filesystem::absolute(source).string()<<'\n';
        for(const char* label:{"ref","test"}) {
            const auto input=read(source/(std::string(label)+"_mtf_stage0.txt"),N,2*N);
            const auto filtered=read(source/(std::string(label)+"_mtf_stage2.txt"),N,2*N);
            std::vector<int> thread_counts{native_threads,3,1};
            std::sort(thread_counts.begin(),thread_counts.end());
            thread_counts.erase(std::unique(thread_counts.begin(),thread_counts.end()),thread_counts.end());
            for(int thread_count:thread_counts) for(bool unaligned:{false,true}) {
                api.threads(thread_count);
                api.forget_wisdom();
                if(!wisdom.empty() && !api.import_wisdom(wisdom.c_str()))
                {
                    auto export_wisdom=api.symbol<char* (*)()>("fftwf_export_wisdom_to_string");
                    char* diagnostic=export_wisdom();
                    std::ofstream(output/"wisdom_import_diagnostic.txt")<<diagnostic;
                    std::free(diagnostic);
                    const auto version=reinterpret_cast<const char*>(GetProcAddress(api.dll,"fftwf_version"));
                    throw std::runtime_error(std::string("Native FFTW wisdom import failed; DLL version=")+(version?version:"unknown"));
                }
                const unsigned flags=64u | (unaligned ? 2u : 0u); // FFTW_ESTIMATE, FFTW_UNALIGNED
                api.threads(thread_count);
                Allocation real_mem(api,Count*sizeof(float)), half_mem(api,Count*sizeof(Complex));
                Allocation complex_in(api,Count*sizeof(Complex)), complex_out(api,Count*sizeof(Complex));
                auto* real=static_cast<float*>(real_mem.data);
                // Octave places compact R2C output at an offset inside a full
                // complex array, then expands the missing Hermitian half.
                auto* half=static_cast<Complex*>(half_mem.data)+N*((N-1)/2);
                auto* ci=static_cast<Complex*>(complex_in.data); auto* co=static_cast<Complex*>(complex_out.data);
                for(unsigned x=0;x<N;++x) for(unsigned y=0;y<N;++y) {
                    const auto raw=order=="column" ? x*N+y : y*N+x;
                    real[x*N+y]=input[raw*2];
                    ci[x*N+y][0]=filtered[raw*2]; ci[x*N+y][1]=filtered[raw*2+1];
                }
                const std::string tag=std::string(label)+"_threads"+std::to_string(thread_count)+(unaligned?"_unaligned":"_aligned");
                const int dims[2]={static_cast<int>(N),static_cast<int>(N)};
                const unsigned planning_flags=flags | (wisdom.empty() ? 0u : (1u<<21)); // FFTW_WISDOM_ONLY
                Plan forward{api,api.real_many_plan(2,dims,1,real,nullptr,1,Count,
                    half,nullptr,1,Count,planning_flags)};
                Plan inverse{api,api.complex_plan(N,N,ci,co,1,planning_flags)};
                if(!wisdom.empty()) {
                    summary<<tag<<" native_wisdom forward="<<(forward.data!=nullptr)<<" inverse="<<(inverse.data!=nullptr)<<'\n';
                    summary.flush();
                    // Fill nonmatching cases using fresh estimate plans so the
                    // comparison format stays complete; label their provenance.
                    if(!forward.data) forward.data=api.real_many_plan(2,dims,1,real,nullptr,1,Count,half,nullptr,1,Count,flags);
                    if(!inverse.data) inverse.data=api.complex_plan(N,N,ci,co,1,flags);
                }
                if(!forward.data || !inverse.data) throw std::runtime_error("FFTW plan creation failed");
                for(const auto& entry:{std::pair{"forward",forward.data},std::pair{"inverse",inverse.data}}) {
                    char* text=api.print(entry.second); if(!text) throw std::runtime_error("Cannot print FFTW plan");
                    double adds,muls,fmas; api.flops(entry.second,&adds,&muls,&fmas);
                    summary<<tag<<' '<<entry.first<<" adds="<<adds<<" muls="<<muls<<" fmas="<<fmas<<'\n'<<text<<"\n\n";
                    // sprint_plan uses ordinary malloc, not FFTW's aligned allocator.
                    std::free(text);
                    summary.flush();
                }
                api.execute(forward.data); api.execute(inverse.data);
                std::vector<std::array<float,2>> f(Count), inv(Count);
                for(unsigned y=0;y<N;++y) for(unsigned x=0;x<N;++x) {
                    const bool conjugate=y>=Half;
                    const auto idx=conjugate ? ((N-x)%N)*Half+(N-y) : x*Half+y;
                    f[y*N+x]={half[idx][0],conjugate?-half[idx][1]:half[idx][1]};
                    inv[y*N+x]={co[x*N+y][0]/static_cast<float>(Count),co[x*N+y][1]/static_cast<float>(Count)};
                }
                save(output/(tag+"_forward.txt"),f,N); save(output/(tag+"_inverse.txt"),inv,N);
            }
        }
        std::cout<<"FFTW plan inspection saved to "<<output.string()<<'\n'; return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
