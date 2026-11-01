#include <bit>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace fs=std::filesystem;
std::vector<double> load(const fs::path& path,size_t count){
 if(fs::file_size(path)!=count*sizeof(double))throw std::runtime_error("Shape: "+path.string());
 std::vector<double> values(count);std::ifstream f(path,std::ios::binary);
 if(!f.read(reinterpret_cast<char*>(values.data()),std::streamsize(count*sizeof(double))))throw std::runtime_error("Read failed");
 return values;
}
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("Usage: native-cpu-root fresh-output");
 const fs::path source=argv[1],out=argv[2];if(fs::exists(out)||!fs::create_directories(out))throw std::runtime_error("Fresh output required");
 std::ofstream binary(out/"queries.f64",std::ios::binary),index(out/"groups.tsv");
 index<<"group\toffset_records\tcount\n";uint64_t count=0,nonfinite=0,negativeZero=0;
 auto emit=[&](double x,double y){double result=std::pow(x,y);double values[]{x,y,result};binary.write(reinterpret_cast<char*>(values),sizeof values);++count;if(!std::isfinite(result))++nonfinite;if(result==0&&std::signbit(result))++negativeZero;return result;};
 const unsigned widths[]{96,96,48,24,12};
 for(const std::string native:{"native","native_ramps","native_identity_v2"})for(const std::string display:{"led-lcd-srgb","led-lcd-wcg","oled"})for(unsigned b=0;b<5;++b){
  const size_t n=size_t(widths[b])*widths[b];const auto prefix=display+"_quality_band"+std::to_string(b+1)+"_";
  const auto params=load(source/native/(prefix+"parameters.f64"),6);
  const std::string fields[]{"self","neighbor","band_diff","noise","difference","masked"};
  const double exponents[]{params[1],params[1],params[0],2*params[0],params[2],.8};
  for(size_t f=0;f<6;++f){const auto values=load(source/native/(prefix+fields[f]+".f64"),n);index<<native<<'/'<<prefix<<fields[f]<<'\t'<<count<<'\t'<<n<<'\n';for(double v:values)emit(std::abs(v),exponents[f]);}
  // Preserve the exact native column-major sequential normalized pooling order.
  const auto masked=load(source/native/(prefix+"masked.f64"),n);double sum=0;
  for(double v:masked)sum+=std::pow(std::abs(v),.8)/double(n);
  index<<native<<'/'<<prefix<<"final_error\t"<<count<<"\t1\n";
  const auto actual=emit(sum,1./.8),expected=load(source/native/(prefix+"error.f64"),1)[0];
  if(std::bit_cast<uint64_t>(actual)!=std::bit_cast<uint64_t>(expected))throw std::runtime_error("Final native error differs: "+prefix);
 }
 index<<"controls\t"<<count<<"\t35\n";
 for(double x:{0.,-0.,1e-300,1e-50,1.,1e50,double(INFINITY)})for(double y:{.8,1.25,2.,2.2,4.4})emit(x,y);
 binary.close();index.close();if(!binary||!index)throw std::runtime_error("Write failed");
 std::ofstream(out/"results.txt")<<count<<" pow query triples;45 final native band errors bit-exact;"<<nonfinite<<" nonfinite outputs;"<<negativeZero<<" negative zero outputs\n";
 std::cout<<count<<" query triples exported;45 final native errors exact\n";
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
