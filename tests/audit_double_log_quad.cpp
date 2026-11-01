#include <quadmath.h>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
// Independent GCC/libquadmath diagnostic, only disputed values plus samples.
// Binary128 agreement is additional evidence, not a native parity pass.
std::vector<double> read(const std::string&p){std::ifstream f(p,std::ios::binary|std::ios::ate);auto n=f.tellg();if(n<=0||n%8)throw std::runtime_error("Shape "+p);std::vector<double>v(size_t(n)/8);f.seekg(0);if(!f.read(reinterpret_cast<char*>(v.data()),n))throw std::runtime_error("Read");return v;}
uint64_t bits(double x){uint64_t v;std::memcpy(&v,&x,8);return v;}
int main(int argc,char**argv){try{if(argc!=3)throw std::runtime_error("Usage: run-directory fresh-report");std::string root=argv[1];std::ifstream exists(argv[2]);if(exists.good())throw std::runtime_error("Fresh report required");auto input=read(root+"/input.f64"),actual=read(root+"/actual.f64"),cpu=read(root+"/expected.f64");if(actual.size()!=input.size()*2||cpu.size()!=actual.size())throw std::runtime_error("Shape");
 uint64_t tested[2]{},disputed[2]{},gpuWins[2]{},cpuWins[2]{},neither[2]{},matchedFailures[2]{};std::ofstream report(argv[2]);report<<"index\top\tx_bits\tcpu_bits\tgpu_bits\tquad_rounded_bits\n";
 for(size_t i=0;i<input.size();++i)for(unsigned op=0;op<2;++op){size_t j=2*i+op;bool different=bits(actual[j])!=bits(cpu[j]);if(!different&&i%10000!=0&&i<input.size()-28)continue;++tested[op];__float128 x=static_cast<__float128>(input[i]);double reference=static_cast<double>(op?log10q(x):logq(x));if(different){++disputed[op];bool gpu=bits(actual[j])==bits(reference),host=bits(cpu[j])==bits(reference);gpuWins[op]+=gpu;cpuWins[op]+=host;neither[op]+=!gpu&&!host;report<<std::dec<<i<<'\t'<<op<<'\t'<<std::hex<<bits(input[i])<<'\t'<<bits(cpu[j])<<'\t'<<bits(actual[j])<<'\t'<<bits(reference)<<'\n';}else matchedFailures[op]+=bits(reference)!=bits(actual[j]);}
 for(unsigned op=0;op<2;++op)std::cout<<(op?"log10":"log")<<": checked="<<tested[op]<<" disputed="<<disputed[op]<<" GPU_matches_quad="<<gpuWins[op]<<" CPU_matches_quad="<<cpuWins[op]<<" neither="<<neither[op]<<" matched_sample_failures="<<matchedFailures[op]<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
