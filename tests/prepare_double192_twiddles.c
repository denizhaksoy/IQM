/* Diagnostic build-time coefficients. Octant reduction follows FFTW3.3.10
 * kernel/trig.c (GPL-2.0-or-later; tests/fftw_reference/COPYING).
 * https://github.com/FFTW/fftw3/blob/fftw-3.3.10/kernel/trig.c
 * Independent coefficients: no native plan memory or exported FFT is read. */
#include <math.h>
#include <stdio.h>
#include <string.h>
static void reduced_order(int phase,int order,double out[2]){
 int m=4*phase,n=4*order,q=order,flags=0;
 if(m<0)m+=n;if(m>n-m){m=n-m;flags|=4;}if(m>q){m-=q;flags|=2;}if(m>q-m){m=q-m;flags|=1;}
 const double angle=(6.2831853071795864769252867665590057683943388*m)/n;
 double c=cos(angle),s=sin(angle),t;if(flags&1){t=c;c=s;s=t;}if(flags&2){t=c;c=-s;s=t;}if(flags&4)s=-s;out[0]=c;out[1]=s;
}
static void reduced_double(int phase,double out[2]){reduced_order(phase,192,out);}
int main(int argc,char**argv){
 if(argc!=2&&argc!=3)return 1;if(argc==3&&strcmp(argv[2],"direct")&&strcmp(argv[2],"square")&&strcmp(argv[2],"longdouble")&&strcmp(argv[2],"complex")&&strcmp(argv[2],"inverse"))return 1;FILE* exists=fopen(argv[1],"rb");if(exists){fclose(exists);return 1;}FILE* f=fopen(argv[1],"wb");if(!f)return 1;
 if(argc==3&&!strcmp(argv[2],"inverse")){const int powers[4]={1,3,9,15};for(int k=0;k<12;++k)for(int p=0;p<4;++p){double v[2];reduced_order((k*powers[p])%192,192,v);if(fwrite(v,8,2,f)!=2)return 1;}fclose(f);puts("96 Double inverse192 first-axis twiddle words");return 0;}
 if(argc==3&&!strcmp(argv[2],"complex")){for(int k=0;k<2;++k)for(int power=1;power<=11;++power){double v[2];reduced_order(k*power,24,v);if(fwrite(v,8,2,f)!=2)return 1;}const int powers[3]={1,3,7};for(int k=0;k<24;++k)for(int p=0;p<3;++p){double v[2];reduced_order((k*powers[p])%192,192,v);if(fwrite(v,8,2,f)!=2)return 1;}fclose(f);puts("188 Double complex192 twiddle words");return 0;}
 for(int row=1;row<=11;++row)for(int col=1;col<=7;++col){
  int m=4*((row*col)%192),n=768,quarter=192,octant=0;
  if(m<0)m+=n;if(m>n-m){m=n-m;octant|=4;}if(m-quarter>0){m-=quarter;octant|=2;}if(m>quarter-m){m=quarter-m;octant|=1;}
  const long double angle=(6.2831853071795864769252867665590057683943388L*m)/n;
  long double c=cosl(angle),s=sinl(angle),t;
  if(octant&1){t=c;c=s;s=t;}if(octant&2){t=c;c=-s;s=t;}if(octant&4)s=-s;
  double values[2]={(double)c,(double)s};if(argc==2||strcmp(argv[2],"longdouble")){int phase=(row*col)%192;if(argc==3&&argv[2][0]=='s'){double a[2],b[2];reduced_double(phase&15,a);reduced_double((phase>>4)*16,b);values[0]=b[0]*a[0]-b[1]*a[1];values[1]=b[1]*a[0]+b[0]*a[1];}else reduced_double(phase,values);}if(fwrite(values,sizeof(double),2,f)!=2)return 1;
 }
 fclose(f);printf("154 Double twiddle words;mode=%s;long double size=%u\n",argc==3?argv[2]:"direct",(unsigned)sizeof(long double));return 0;
}
