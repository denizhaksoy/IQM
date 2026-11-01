#include <IQM/hdrvdp3/spectral.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
using Matrix=IQM::HDRVDP3::ColorMatrix3;
Matrix solve(Matrix a,unsigned flags,Matrix b){
 unsigned pivot[3];
 auto update=[](double value,double x,double y,bool fused){return fused?std::fma(-x,y,value):value-x*y;};
 for(unsigned k=0;k<3;++k){unsigned p=k;for(unsigned i=k+1;i<3;++i)if(std::abs(a[i][k])>std::abs(a[p][k]))p=i;pivot[k]=p;std::swap(a[k],a[p]);
  double reciprocal=1/a[k][k];for(unsigned i=k+1;i<3;++i)a[i][k]=(flags&1)?a[i][k]*reciprocal:a[i][k]/a[k][k];
  for(unsigned j=k+1;j<3;++j)for(unsigned i=k+1;i<3;++i)a[i][j]=update(a[i][j],a[i][k],a[k][j],flags&2);
 }
 for(unsigned k=0;k<3;++k)std::swap(b[k],b[pivot[k]]);
 for(unsigned j=0;j<3;++j){
  if(flags&64){for(unsigned i=0;i<3;++i){double sum=0;for(unsigned k=0;k<i;++k)sum=(flags&4)?std::fma(a[i][k],b[k][j],sum):sum+a[i][k]*b[k][j];b[i][j]-=sum;}}
  else for(unsigned k=0;k<3;++k)for(unsigned i=k+1;i<3;++i)b[i][j]=update(b[i][j],a[i][k],b[k][j],flags&4);
  if(flags&32){for(int i=2;i>=0;--i){double sum=0;for(int k=i+1;k<3;++k)sum=(flags&16)?std::fma(a[i][k],b[k][j],sum):sum+a[i][k]*b[k][j];b[i][j]-=sum;b[i][j]=(flags&8)?b[i][j]*(1/a[i][i]):b[i][j]/a[i][i];}}
  else for(int k=2;k>=0;--k){b[k][j]=(flags&8)?b[k][j]*(1/a[k][k]):b[k][j]/a[k][k];for(int i=0;i<k;++i)b[i][j]=update(b[i][j],a[i][k],b[k][j],flags&16);}
 }
 return b;
}
int main(int argc,char** argv){try{
 if(argc!=4&&argc!=5)throw std::runtime_error("Usage: native raw-data fresh-output [encoding]");namespace fs=std::filesystem;fs::path native(argv[1]),raw(argv[2]),out(argv[3]);if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 Matrix target{{{.4124,.3576,.1805},{.2126,.7152,.0722},{.0193,.1192,.9505}}};
 if(argc==5){std::string encoding=argv[4];if(encoding=="rgb-bt.2020")target={{{.6370,.1446,.1689},{.2627,.6780,.0593},{0,.0281,1.0610}}};else if(encoding=="xyz")target={{{1,0,0},{0,1,0},{0,0,1}}};else if(encoding!="rgb-bt.709")throw std::runtime_error("Unknown encoding");}
 size_t errors[128]{},singleErrors[128]{};
 for(std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"}){
  auto samples=IQM::HDRVDP3::load_xyz_display_samples(raw/"ciexyz31.csv",raw/("emission_spectra_"+display+".csv"));auto a=IQM::HDRVDP3::calibrate_display_xyz(samples).native2XYZ;
  Matrix expected;std::ifstream f(native/(display+"_matrices.f64"),std::ios::binary);f.seekg(18*8);for(unsigned col=0;col<3;++col)for(unsigned row=0;row<3;++row)if(!f.read(reinterpret_cast<char*>(&expected[row][col]),8))throw std::runtime_error("Missing transform");
  for(unsigned flags=0;flags<128;++flags){auto m=solve(a,flags,target);for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col){errors[flags]+=std::bit_cast<uint64_t>(m[row][col])!=std::bit_cast<uint64_t>(expected[row][col]);singleErrors[flags]+=std::bit_cast<uint32_t>(float(m[row][col]))!=std::bit_cast<uint32_t>(float(expected[row][col]));}}
 }
 std::ofstream log(out/"candidates.tsv");log<<"flags\tdouble_bit_errors\tcoefficient_float_bit_errors\n";for(unsigned flags=0;flags<128;++flags){log<<flags<<'\t'<<errors[flags]<<'\t'<<singleErrors[flags]<<'\n';if(errors[flags]<=3)std::cout<<flags<<" DoubleErrors="<<errors[flags]<<" FloatErrors="<<singleErrors[flags]<<" /27\n";}
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
