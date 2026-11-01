#include <IQM/hdrvdp3/pathway_runtime.h>
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
namespace IQM::HDRVDP3 {
namespace {
std::vector<unsigned char> bytes(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary|std::ios::ate);const auto size=f.tellg();if(size<0||size>64*1024*1024)throw std::runtime_error("Missing/oversized runtime asset:"+p.string());std::vector<unsigned char> v(static_cast<size_t>(size));f.seekg(0);if(!f.read(reinterpret_cast<char*>(v.data()),v.size()))throw std::runtime_error("Truncated runtime asset");return v;}
std::string sha256(std::vector<unsigned char> data){
 constexpr std::array<uint32_t,64> k{0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
 std::array<uint32_t,8> h{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};const uint64_t bits=uint64_t(data.size())*8;data.push_back(0x80);while(data.size()%64!=56)data.push_back(0);for(int i=7;i>=0;--i)data.push_back(static_cast<unsigned char>(bits>>(i*8)));
 for(size_t off=0;off<data.size();off+=64){std::array<uint32_t,64>w{};for(unsigned i=0;i<16;++i)for(unsigned j=0;j<4;++j)w[i]=(w[i]<<8)|data[off+4*i+j];for(unsigned i=16;i<64;++i){const auto a=w[i-15],b=w[i-2];w[i]=w[i-16]+(std::rotr(a,7)^std::rotr(a,18)^(a>>3))+w[i-7]+(std::rotr(b,17)^std::rotr(b,19)^(b>>10));}auto [a,b,c,d,e,f,g,z]=h;for(unsigned i=0;i<64;++i){const uint32_t t1=z+(std::rotr(e,6)^std::rotr(e,11)^std::rotr(e,25))+((e&f)^(~e&g))+k[i]+w[i],t2=(std::rotr(a,2)^std::rotr(a,13)^std::rotr(a,22))+((a&b)^(a&c)^(b&c));z=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;}h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=z;}
 std::ostringstream out;out<<std::hex<<std::setfill('0');for(auto v:h)out<<std::setw(8)<<v;return out.str();
}
std::vector<uint32_t> model(const std::vector<unsigned char>& data,size_t count){std::istringstream f(std::string(data.begin(),data.end()));unsigned h,w;if(!(f>>h>>w)||!h||!w||size_t(h)*w!=count)throw std::runtime_error("Invalid sp0 model shape");std::vector<uint32_t> words(count);for(auto& v:words){uint64_t n;if(!(f>>n)||n>UINT32_MAX)throw std::runtime_error("Invalid model word");v=uint32_t(n);if(!std::isfinite(std::bit_cast<float>(v)))throw std::runtime_error("Nonfinite model word");}std::string extra;if(f>>extra)throw std::runtime_error("Trailing model data");return words;}
}
static RGB709Runtime load_linear_overlay(const std::filesystem::path& directory,unsigned w,unsigned h,
 const char* filename,const std::string& expectedMagic,const std::string& expected,const std::string& name){
 if(w!=96||h!=96)throw std::invalid_argument("Unsupported"+name+" runtime geometry");
 const auto root=std::filesystem::canonical(directory);std::ifstream manifest(root/filename);std::string header,profile;unsigned version;
 if(!(manifest>>header>>version>>profile)||header!=expectedMagic||version!=1||profile!=expected)throw std::runtime_error("Unsupported"+name+" runtime profile");
 std::map<std::string,std::filesystem::path> paths;std::string key,file,digest;
 while(manifest>>key){
  if(!(manifest>>std::quoted(file)>>digest)||(key!="native"&&key!="conversion")||paths.contains(key))throw std::runtime_error("Malformed709 runtime asset");
  const std::filesystem::path relative(file);if(relative.empty()||relative.is_absolute()||relative.has_root_name())throw std::runtime_error("709 asset must be relative");for(const auto& part:relative)if(part=="..")throw std::runtime_error("709 asset path traversal");
  const auto resolved=std::filesystem::canonical(root/relative);auto a=root.begin(),b=resolved.begin();for(;a!=root.end();++a,++b)if(b==resolved.end()||*a!=*b)throw std::runtime_error("709 asset escapes package");
  auto data=bytes(resolved);if(digest.size()!=64||sha256(data)!=digest)throw std::runtime_error("709 SHA256 mismatch:"+key);
  if(key=="native"){if(resolved.filename()!="rgb.runtime")throw std::runtime_error("Invalid709 native manifest");}
  else{uint32_t magic=0;if(data.size()>=4)std::memcpy(&magic,data.data(),4);if(data.size()<20||data.size()%4||magic!=0x07230203)throw std::runtime_error("Invalid709 SPIR-V");}paths[key]=resolved;
 }
 if(!manifest.eof()||paths.size()!=2)throw std::runtime_error("Incomplete709 runtime");
 return {load_rgb_pathway_runtime(paths.at("native").parent_path(),w,h),paths.at("conversion")};
}
RGB709Runtime load_rgb709_pathway_runtime(const std::filesystem::path& directory,unsigned w,unsigned h){
 return load_linear_overlay(directory,w,h,"rgb709.runtime","IQM_HDRVDP3_709","rgb-bt.709-30ppd-age24-sp0-square96","709");
}
LinearColorRuntime load_linear_color_pathway_runtime(const std::filesystem::path& directory,unsigned w,unsigned h,LinearColorEncoding encoding){
 if(encoding==LinearColorEncoding::BT709)return load_rgb709_pathway_runtime(directory,w,h);
 std::string prefix;if(encoding==LinearColorEncoding::BT2020)prefix="rgb-bt.2020";else if(encoding==LinearColorEncoding::XYZ)prefix="xyz";else throw std::invalid_argument("Unknown linear runtime encoding");
 return load_linear_overlay(directory,w,h,"linear.runtime","IQM_HDRVDP3_LINEAR",prefix+"-30ppd-age24-sp0-square96"," linear-color");
}
RGBQualityAssets load_rgb_pathway_runtime(const std::filesystem::path& directory,unsigned w,unsigned h){
 if(w!=96||h!=96)throw std::invalid_argument("Unsupported RGB runtime geometry");
 const auto root=std::filesystem::canonical(directory);std::ifstream manifest(root/"rgb.runtime");
 std::string header,profile;unsigned version;
 if(!(manifest>>header>>version>>profile)||header!="IQM_HDRVDP3_RGB"||version!=1||profile!="rgb-native-30ppd-age24-sp0-square96")
  throw std::runtime_error("Unsupported RGB runtime profile");
 const std::set<std::string> required{"base","projection","adaptation","jnd","comparison"};
 std::map<std::string,std::filesystem::path> paths;std::string key,file,digest;
 while(manifest>>key){
  if(!(manifest>>std::quoted(file)>>digest)||!required.contains(key)||paths.contains(key))throw std::runtime_error("Malformed RGB runtime asset");
  const std::filesystem::path relative(file);if(relative.empty()||relative.is_absolute()||relative.has_root_name())throw std::runtime_error("RGB asset must be relative");
  for(const auto& part:relative)if(part=="..")throw std::runtime_error("RGB asset path traversal");
  const auto resolved=std::filesystem::canonical(root/relative);auto a=root.begin(),b=resolved.begin();for(;a!=root.end();++a,++b)if(b==resolved.end()||*a!=*b)throw std::runtime_error("RGB asset escapes package");
  auto data=bytes(resolved);if(digest.size()!=64||sha256(data)!=digest)throw std::runtime_error("RGB SHA256 mismatch:"+key);
  if(key=="base"){if(resolved.filename()!="pathway.runtime")throw std::runtime_error("Invalid RGB base manifest");}
  else{uint32_t magic=0;if(data.size()>=4)std::memcpy(&magic,data.data(),4);if(data.size()<20||data.size()%4||magic!=0x07230203)throw std::runtime_error("Invalid RGB SPIR-V:"+key);}
  paths[key]=resolved;
 }
 if(!manifest.eof()||paths.size()!=required.size())throw std::runtime_error("Incomplete RGB runtime");
 auto quality=load_pathway_runtime(paths.at("base").parent_path(),w,h);quality.comparison=paths.at("comparison");
 return {std::move(quality),paths.at("projection"),paths.at("adaptation"),paths.at("jnd")};
}
PathwayQualityAssets load_pathway_runtime(const std::filesystem::path& directory,unsigned w,unsigned h){
 if(!((h==128&&w==128)||(h==80&&w==80)||(h==80&&w==96)||(h==96&&w==80)||(h==96&&w==96)))throw std::invalid_argument("Unsupported pathway runtime geometry");
 const auto root=std::filesystem::canonical(directory);std::ifstream manifest(root/"pathway.runtime");std::string header,profile;unsigned version;if(!(manifest>>header>>version>>profile)||header!="IQM_HDRVDP3_PATHWAY")throw std::runtime_error("Unsupported pathway runtime manifest/profile");
 const bool square96=version==2&&profile=="luminance-30ppd-age24-D65-sp0-map13-square96";
 const bool square80=version==3&&profile=="luminance-30ppd-age24-D65-sp0-map13-square80";
 const bool square128=version==4&&profile=="luminance-30ppd-age24-D65-sp0-map14-square128";
 const bool square=square80||square96||square128;
 const bool variableCount=square80||square128;
 const bool rectangular=version==1&&profile=="luminance-30ppd-age24-D65-sp0-map13";
 if(!square&&!rectangular)throw std::runtime_error("Unsupported pathway runtime manifest/profile");
 if(!((square128&&h==128&&w==128)||(square80&&h==80&&w==80)||(square96&&h==96&&w==96)||(rectangular&&((h==80&&w==96)||(h==96&&w==80)))))throw std::invalid_argument("Runtime profile does not support requested geometry");
 std::set<std::string> required{"forward80","inverse80","forward96","inverse96","io","spyr_fma","spyr_separate","mean","difference","residual","pool_fma","pool_separate","comparison","map","product","base","lo0","lo","high","orientation"};
 if(square){for(const auto* key:{"forward80","inverse80","forward96","inverse96"})required.erase(key);required.insert("forward");required.insert("inverse");}
 std::map<std::string,std::filesystem::path> paths;std::map<std::string,std::vector<uint32_t>> models;std::string key,file,digest;
 while(manifest>>key){if(!(manifest>>std::quoted(file)>>digest)||!required.contains(key)||paths.contains(key))throw std::runtime_error("Malformed/duplicate runtime asset");const std::filesystem::path relative(file);if(relative.empty()||relative.is_absolute()||relative.has_root_name())throw std::runtime_error("Runtime asset must be relative");for(const auto& part:relative)if(part=="..")throw std::runtime_error("Runtime asset path traversal");const auto resolved=std::filesystem::canonical(root/relative);auto a=root.begin(),b=resolved.begin();for(;a!=root.end();++a,++b)if(b==resolved.end()||*a!=*b)throw std::runtime_error("Runtime asset escapes package");const auto data=bytes(resolved);if(digest.size()!=64||sha256(data)!=digest)throw std::runtime_error("Runtime SHA256 mismatch:"+key);
 if(key=="lo0"||key=="lo"||key=="high"||key=="orientation")models[key]=model(data,key=="lo0"?49:key=="lo"?169:81);
 else{uint32_t magic=0;if(data.size()>=4)std::memcpy(&magic,data.data(),4);if(data.size()<20||data.size()%4||magic!=0x07230203)throw std::runtime_error("Invalid SPIR-V asset:"+key);}paths[key]=resolved;
 }
 if(!manifest.eof()||paths.size()!=required.size())throw std::runtime_error("Incomplete pathway runtime");const auto suffix=h==80?"80":"96";
 return {{paths.at(square?"forward":std::string("forward")+suffix),paths.at(square?"inverse":std::string("inverse")+suffix),paths.at("io"),square128},{paths.at("spyr_fma"),paths.at("spyr_separate"),paths.at("mean"),paths.at("difference"),paths.at("residual"),variableCount},{models.at("lo0"),models.at("lo"),models.at("high"),models.at("orientation")},{paths.at("pool_fma"),paths.at("pool_separate")},paths.at("comparison"),{paths.at("map"),paths.at("product"),paths.at("base"),variableCount,square128?14u:13u}};
}
}
