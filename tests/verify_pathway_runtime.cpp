#include <IQM/hdrvdp3/pathway_runtime.h>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv){try{
 if(argc==4){const auto a=IQM::HDRVDP3::load_pathway_runtime(argv[1],std::stoul(argv[2]),std::stoul(argv[3]));if(a.filters.lo0.size()!=49||a.filters.lo.size()!=169||a.filters.high.size()!=81||a.filters.orientation.size()!=81)throw std::runtime_error("Invalid runtime model");std::cout<<"Runtime requested geometry/model/SHA256 PASS;no GPU\n";return 0;}
 if(argc!=2)throw std::runtime_error("Usage:runtime-folder [width height]");const auto a=IQM::HDRVDP3::load_pathway_runtime(argv[1],96,80),b=IQM::HDRVDP3::load_pathway_runtime(argv[1],80,96);if(a.filter.forward==b.filter.forward||a.filter.inverse==b.filter.inverse||a.filters.lo0.size()!=49||a.filters.lo.size()!=169||a.filters.high.size()!=81||a.filters.orientation.size()!=81)throw std::runtime_error("Invalid runtime selection/model");std::cout<<"Runtime manifest PASS:two orientations,20 SHA256 assets each,no GPU.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
