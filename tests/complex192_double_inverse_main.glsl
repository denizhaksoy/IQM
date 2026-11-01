// Diagnostic FFTW3.3.10 graph composition,GPL-2.0-or-later.
double divideCorrected(double a,double d){if(a==0.0lf)return a;precise double q=a/d;precise double r=fma(-q,d,a);precise double c=r/d;precise double result=q+c;return result;}
void main(){
 uint id=gl_GlobalInvocationID.x;if(id>=p.batches*192u)return;uint matrix=id/192u,batch=id%192u;
 dvec2 intermediate[192];
 for(int j=0;j<8;++j){dvec2 inner[24];
  for(int a=0;a<12;++a){dvec2 x=src.value[matrix*36864u+batch*192u+uint(j+8*a)],y=src.value[matrix*36864u+batch*192u+uint(j+8*(a+12))];inner[a]=VADD(x,y);inner[a+12]=VSUB(x,y);}
  for(int b=0;b<2;++b){dvec2 x[12],W[11];for(int a=0;a<12;++a)x[a]=inner[a+12*b];for(int w=0;w<11;++w)W[w]=twiddle24[b*11+w];graph12(x,W);for(int a=0;a<12;++a)intermediate[j+8*(b+2*a)]=x[a];}
 }
 for(int k=0;k<24;++k){dvec2 x[8],W[3];for(int j=0;j<8;++j)x[j]=intermediate[j+8*k];for(int w=0;w<3;++w)W[w]=twiddle192[k*3+w];graph8(x,W);for(int j=0;j<8;++j){uint frequency=uint(k+24*j);dst.value[matrix*36864u+frequency*192u+batch]=dvec2(divideCorrected(x[j].x,36864.0lf),0.0lf);}}
}
