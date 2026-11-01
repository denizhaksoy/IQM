#include <IQM/hdrvdp3/pathway_runtime.h>
#include <iostream>
int main(int argc,char** argv){
 if(argc!=2&&argc!=3)return 2;bool reject=argc==3;
 try{
#if defined(IQM_TEST_BT2020_RUNTIME)
  auto assets=IQM::HDRVDP3::load_linear_color_pathway_runtime(argv[1],96,96,IQM::HDRVDP3::LinearColorEncoding::BT2020);
#elif defined(IQM_TEST_XYZ_RUNTIME)
  auto assets=IQM::HDRVDP3::load_linear_color_pathway_runtime(argv[1],96,96,IQM::HDRVDP3::LinearColorEncoding::XYZ);
#elif defined(IQM_TEST_RGB709_RUNTIME)
  auto assets=IQM::HDRVDP3::load_rgb709_pathway_runtime(argv[1],96,96);
#else
  auto assets=IQM::HDRVDP3::load_rgb_pathway_runtime(argv[1],96,96);
#endif
  if(reject){std::cerr<<"Malformed package accepted\n";return 1;}
  std::cout<<"RGB runtime PASS\n";return 0;
 }catch(const std::exception& e){if(reject){std::cout<<"Rejected: "<<e.what()<<'\n';return 0;}std::cerr<<e.what()<<'\n';return 1;}
}
