// SPDX-License-Identifier: GPL-2.0-or-later
// Diagnostic derived from FFTW 3.3.10 t1fv_15; copyright 2003, 2007-14
// Matteo Frigo and Massachusetts Institute of Technology.
// Original source and license retained in tests/fftw_reference/COPYING and
// validation/fftw_4k_sources_20261107_run01/fftw-3.3.10/.
#include <IQM/hdrvdp3/fftw_single_provider.h>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
struct vec2{float x,y;vec2():x(0),y(0){}explicit vec2(float v):x(v),y(v){}vec2(float a,float b):x(a),y(b){}};
vec2 VFMS(vec2 a,vec2 b,vec2 c){return {std::fma(a.x,b.x,-c.x),std::fma(a.y,b.y,-c.y)};}
vec2 VADD(vec2 a,vec2 b){return {a.x+b.x,a.y+b.y};}vec2 VSUB(vec2 a,vec2 b){return {a.x-b.x,a.y-b.y};}vec2 VMUL(vec2 a,vec2 b){return {a.x*b.x,a.y*b.y};}vec2 VBYI(vec2 a){return {-a.y,a.x};}vec2 VFMA(vec2 a,vec2 b,vec2 c){return {std::fma(a.x,b.x,c.x),std::fma(a.y,b.y,c.y)};}vec2 VFNMS(vec2 a,vec2 b,vec2 c){return {std::fma(-a.x,b.x,c.x),std::fma(-a.y,b.y,c.y)};}vec2 VZMULJ(vec2 a,vec2 b){float r=a.y*b.y,i=a.y*b.x;return {std::fma(a.x,b.x,r),std::fma(a.x,b.y,-i)};}
void twiddle15(vec2 x[15],const vec2 W[14]){const float KP216506350=+0.216506350946109661690930792688234045867850657;
const float KP484122918=+0.484122918275927110647408174972799951354115213;
const float KP866025403=+0.866025403784438646763723170752936183471402627;
const float KP509036960=+0.509036960455127183450980863393907648510733164;
const float KP823639103=+0.823639103546331925877420039278190003029660514;
const float KP587785252=+0.587785252292473129168705954639072768597652438;
const float KP951056516=+0.951056516295153572116439333379382143405698634;
const float KP250000000=+0.250000000000000000000000000000000000000000000;
const float KP559016994=+0.559016994374947424102293417182819058860154590;
const float KP500000000=+0.500000000000000000000000000000000000000000000;

	       vec2 T1e, T7, TP, T12, T15, Tf, Tn, To, T1b, T1c, T1f, TQ, TR, TS, Tw;
	       vec2 TE, TF, TT, TU, TV;
	       {
		    vec2 T1, T5, T3, T4, T2, T6;
		    T1 = x[0];
		    T4 = x[10];
		    T5 = VZMULJ(W[9],T4);
		    T2 = x[5];
		    T3 = VZMULJ(W[4],T2);
		    T1e = VSUB(T5, T3);
		    T6 = VADD(T3, T5);
		    T7 = VADD(T1, T6);
		    TP = VFNMS(vec2(KP500000000), T6, T1);
	       }
	       {
		    vec2 T9, Tq, Ty, Th, Te, T13, Tv, T10, TD, T11, Tm, T14;
		    {
			 vec2 T8, Tp, Tx, Tg;
			 T8 = x[3];
			 T9 = VZMULJ(W[2],T8);
			 Tp = x[6];
			 Tq = VZMULJ(W[5],Tp);
			 Tx = x[9];
			 Ty = VZMULJ(W[8],Tx);
			 Tg = x[12];
			 Th = VZMULJ(W[11],Tg);
		    }
		    {
			 vec2 Tb, Td, Ta, Tc;
			 Ta = x[8];
			 Tb = VZMULJ(W[7],Ta);
			 Tc = x[13];
			 Td = VZMULJ(W[12],Tc);
			 Te = VADD(Tb, Td);
			 T13 = VSUB(Td, Tb);
		    }
		    {
			 vec2 Ts, Tu, Tr, Tt;
			 Tr = x[11];
			 Ts = VZMULJ(W[10],Tr);
			 Tt = x[1];
			 Tu = VZMULJ(W[0],Tt);
			 Tv = VADD(Ts, Tu);
			 T10 = VSUB(Tu, Ts);
		    }
		    {
			 vec2 TA, TC, Tz, TB;
			 Tz = x[14];
			 TA = VZMULJ(W[13],Tz);
			 TB = x[4];
			 TC = VZMULJ(W[3],TB);
			 TD = VADD(TA, TC);
			 T11 = VSUB(TC, TA);
		    }
		    {
			 vec2 Tj, Tl, Ti, Tk;
			 Ti = x[2];
			 Tj = VZMULJ(W[1],Ti);
			 Tk = x[7];
			 Tl = VZMULJ(W[6],Tk);
			 Tm = VADD(Tj, Tl);
			 T14 = VSUB(Tl, Tj);
		    }
		    T12 = VSUB(T10, T11);
		    T15 = VSUB(T13, T14);
		    Tf = VADD(T9, Te);
		    Tn = VADD(Th, Tm);
		    To = VADD(Tf, Tn);
		    T1b = VADD(T13, T14);
		    T1c = VADD(T10, T11);
		    T1f = VADD(T1b, T1c);
		    TQ = VFNMS(vec2(KP500000000), Te, T9);
		    TR = VFNMS(vec2(KP500000000), Tm, Th);
		    TS = VADD(TQ, TR);
		    Tw = VADD(Tq, Tv);
		    TE = VADD(Ty, TD);
		    TF = VADD(Tw, TE);
		    TT = VFNMS(vec2(KP500000000), Tv, Tq);
		    TU = VFNMS(vec2(KP500000000), TD, Ty);
		    TV = VADD(TT, TU);
	       }
	       {
		    vec2 TI, TG, TH, TM, TO, TK, TL, TN, TJ;
		    TI = VMUL(vec2(KP559016994), VSUB(To, TF));
		    TG = VADD(To, TF);
		    TH = VFNMS(vec2(KP250000000), TG, T7);
		    TK = VSUB(Tw, TE);
		    TL = VSUB(Tf, Tn);
		    TM = VBYI(VFNMS(vec2(KP587785252), TL, VMUL(vec2(KP951056516), TK)));
		    TO = VBYI(VFMA(vec2(KP951056516), TL, VMUL(vec2(KP587785252), TK)));
		    x[0]=VADD(T7, TG);
		    TN = VFMA(vec2(KP559016994), VSUB(To, TF), TH);
		    x[6]=VSUB(TN, TO);
		    x[9]=VADD(TO, TN);
		    TJ = VFNMS(vec2(KP559016994), VSUB(To, TF), TH);
		    x[3]=VSUB(TJ, TM);
		    x[12]=VADD(TM, TJ);
	       }
	       {
		    vec2 T16, T1m, T1u, T1h, T1o, T1a, T1p, TZ, T1t, T1l, T1d, T1g;
		    T16 = VFNMS(vec2(KP509036960), T15, VMUL(vec2(KP823639103), T12));
		    T1m = VFMA(vec2(KP823639103), T15, VMUL(vec2(KP509036960), T12));
		    T1u = VBYI(VMUL(vec2(KP866025403), VADD(T1e, T1f)));
		    T1d = VMUL(vec2(KP484122918), VSUB(T1b, T1c));
		    T1g = VFNMS(vec2(KP216506350), T1f, VMUL(vec2(KP866025403), T1e));
		    T1h = VFMS(vec2(KP484122918), VSUB(T1b, T1c), T1g);
		    T1o = VFMA(vec2(KP484122918), VSUB(T1b, T1c), T1g);
		    {
			 vec2 T18, T19, TY, TW, TX;
			 T18 = VSUB(TT, TU);
			 T19 = VSUB(TQ, TR);
			 T1a = VFNMS(vec2(KP587785252), T19, VMUL(vec2(KP951056516), T18));
			 T1p = VFMA(vec2(KP951056516), T19, VMUL(vec2(KP587785252), T18));
			 TY = VMUL(vec2(KP559016994), VSUB(TS, TV));
			 TW = VADD(TS, TV);
			 TX = VFNMS(vec2(KP250000000), TW, TP);
			 TZ = VFNMS(vec2(KP559016994), VSUB(TS, TV), TX);
			 T1t = VADD(TP, TW);
			 T1l = VFMA(vec2(KP559016994), VSUB(TS, TV), TX);
		    }
		    {
			 vec2 T17, T1i, T1r, T1s;
			 x[5]=VSUB(T1t, T1u);
			 x[10]=VADD(T1t, T1u);
			 T17 = VSUB(TZ, T16);
			 T1i = VBYI(VSUB(T1a, T1h));
			 x[8]=VSUB(T17, T1i);
			 x[7]=VADD(T17, T1i);
			 T1r = VSUB(T1l, T1m);
			 T1s = VBYI(VADD(T1p, T1o));
			 x[11]=VSUB(T1r, T1s);
			 x[4]=VADD(T1r, T1s);
			 {
			      vec2 T1n, T1q, T1j, T1k;
			      T1n = VADD(T1l, T1m);
			      T1q = VBYI(VSUB(T1o, T1p));
			      x[14]=VSUB(T1n, T1q);
			      x[1]=VADD(T1n, T1q);
			      T1j = VADD(TZ, T16);
			      T1k = VBYI(VADD(T1a, T1h));
			      x[13]=VSUB(T1j, T1k);
			      x[2]=VADD(T1j, T1k);
			 }
		    }
	       }
	  }

std::vector<float> load(const std::filesystem::path& p){std::vector<float> v(std::filesystem::file_size(p)/4);std::ifstream f(p,std::ios::binary);f.read(reinterpret_cast<char*>(v.data()),v.size()*4);return v;}
int main(int argc,char**argv){try{if(argc!=4)throw std::runtime_error("Usage: fftw.dll diagnostic-root fresh-out");std::filesystem::path root=argv[2],out=argv[3];if(std::filesystem::exists(out)||!std::filesystem::create_directories(out))throw std::runtime_error("Fresh output required");auto input=load(root/"forward_input.f32"),expected=load(root/"forward_expected.f32"),gpu=load(root/"forward_actual.f32"),weights=load(root/"weights.f32");IQM::HDRVDP3::FFTWProviderSingle fft(argv[1]);using C=IQM::HDRVDP3::FFTWProviderSingle::Complex;auto make=fft.symbol<void*(*)(int,C*,C*,int,unsigned)>("fftwf_plan_dft_1d");IQM::HDRVDP3::FFTWProviderSingle::Memory src(fft,96*4),dst(fft,96*4);fft.forget();auto plan=make(48,static_cast<C*>(src.data),static_cast<C*>(dst.data),-1,82);if(!plan)throw std::runtime_error("Inner plan");std::vector<float> actual(input.size());for(size_t b=0;b<input.size()/1440;b++){vec2 intermediate[720];for(unsigned j=0;j<15;j++){auto p=static_cast<C*>(src.data);for(unsigned l=0;l<48;l++){p[l][0]=input[b*1440+2*(j+15*l)];p[l][1]=input[b*1440+2*(j+15*l)+1];}fft.execute(plan);auto q=static_cast<C*>(dst.data);for(unsigned k=0;k<48;k++)intermediate[j+15*k]={q[k][0],q[k][1]};}for(unsigned k=0;k<48;k++){vec2 x[15],W[14];for(unsigned j=0;j<15;j++)x[j]=intermediate[j+15*k];for(unsigned j=0;j<14;j++)W[j]={weights[2*(k*14+j)],weights[2*(k*14+j)+1]};twiddle15(x,W);for(unsigned j=0;j<15;j++){actual[b*1440+2*(k+48*j)]=x[j].x;actual[b*1440+2*(k+48*j)+1]=x[j].y;}}}fft.destroy(plan);size_t nativeBits=0,gpuBits=0;for(size_t i=0;i<actual.size();i++){nativeBits+=std::bit_cast<uint32_t>(actual[i])!=std::bit_cast<uint32_t>(expected[i]);gpuBits+=std::bit_cast<uint32_t>(actual[i])!=std::bit_cast<uint32_t>(gpu[i]);}std::ofstream(out/"reconstructed.f32",std::ios::binary).write(reinterpret_cast<const char*>(actual.data()),actual.size()*4);std::ofstream(out/"summary.json")<<"{\"cpu_graph_vs_native_bits\":"<<nativeBits<<",\"cpu_graph_vs_gpu_bits\":"<<gpuBits<<",\"values\":"<<actual.size()<<",\"scope\":\"diagnostic reconstruction only\"}";std::cout<<"native bits="<<nativeBits<<" GPU bits="<<gpuBits<<std::endl;return 0;}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
