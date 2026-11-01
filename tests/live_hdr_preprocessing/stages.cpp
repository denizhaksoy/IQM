#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <limits>
#include <IQM/hdrvdp3/pq_single.h>
uint32_t word(float f){uint32_t u;std::memcpy(&u,&f,4);return u;}
int main(int argc,char**argv){try{
 if(argc!=3||std::filesystem::exists(argv[2]))throw std::runtime_error("Usage: official-stages.f32 fresh-result.json");
 std::vector<float> reference(65536*5);std::ifstream in(argv[1],std::ios::binary);
 in.read(reinterpret_cast<char*>(reference.data()),reference.size()*4);
 if(!in||in.peek()!=std::char_traits<char>::eof())throw std::runtime_error("Stage input byte count");
 uint64_t mismatches[5]={};uint32_t ulp[5]={};double absolute[5]={};
 std::vector<float> unit(65536);for(unsigned i=0;i<65536;++i)unit[i]=IQM::HDRVDP3::normalize_pq_u16_single(std::uint16_t(i));
 const auto api=IQM::HDRVDP3::pq_to_display_single(unit,{200.,.005,2000.});
 unsigned rejected=0;
 for(float bad:{-1.f,1.01f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
   try{IQM::HDRVDP3::pq_to_display_single(std::span<const float>(&bad,1),{200.,.005,2000.});}
   catch(const std::invalid_argument&){++rejected;}
 }
 if(rejected!=4)throw std::runtime_error("Invalid PQ input was accepted");
 for(unsigned code=0;code<65536;++code){
   float x[5];x[0]=float(code)/65535.f;
   x[1]=std::pow(x[0],float(1./78.84375));
   float numerator=std::max(x[1]-.8359375f,0.f);
   float denominator=18.8515625f-18.6875f*x[1];
   x[2]=numerator/denominator;
   x[3]=10000.f*std::pow(x[2],float(1./.1593017578125));
   x[4]=api[code];
   for(unsigned s=0;s<5;++s){float r=reference[s*65536+code];auto a=word(x[s]),b=word(r);
     mismatches[s]+=a!=b;ulp[s]=std::max(ulp[s],a>b?a-b:b-a);absolute[s]=std::max(absolute[s],std::abs(double(x[s])-r));}
 }
 const char*names[]={"unit","pow_m","ratio","linear","final"};
 std::ofstream out(argv[2]);out.precision(17);out<<"{\"codes\":65536,\"invalid_inputs_rejected\":"<<rejected<<",\"stages\":[";
 for(unsigned s=0;s<5;++s){if(s)out<<',';out<<"{\"name\":\""<<names[s]<<"\",\"bit_mismatches\":"<<mismatches[s]<<",\"max_ulp\":"<<ulp[s]<<",\"max_abs\":"<<absolute[s]<<'}';}
 out<<"]}\n";if(!out)throw std::runtime_error("Output write failed");return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
