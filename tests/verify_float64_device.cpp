#include <vulkan_instance.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("Usage: fresh-output");std::filesystem::path out=argv[1];if(std::filesystem::exists(out)||!std::filesystem::create_directories(out))throw std::runtime_error("Fresh output required");
 std::ofstream log(out/"devices.txt");
 {IQM::Bin::VulkanInstance instance(true);if(instance.float64Enabled)throw std::runtime_error("Legacy default enables Float64 unexpectedly");log<<"legacy default: "<<instance.selectedDevice<<";Float64 disabled\n";}
 {IQM::Bin::VulkanInstance instance(true,"NVIDIA",true);if(!instance.float64Enabled||!instance.physicalDevice()->getFeatures().shaderFloat64)throw std::runtime_error("Float64 not enabled");log<<"explicit Float64: "<<instance.selectedDevice<<'\n';}
 unsigned rejected=0;for(std::string name:{"Intel","IQM-nonexistent-device"}){bool caught=false;try{IQM::Bin::VulkanInstance instance(true,name,true);}catch(const std::runtime_error&e){if(std::string(e.what()).find("No matching Vulkan compute device supports shaderFloat64")==std::string::npos)throw;caught=true;}if(!caught)throw std::runtime_error("Unsupported explicit device accepted");++rejected;}
 std::ofstream(out/"PASS.txt")<<"Legacy selection/disabled Float64 preserved;explicit NVIDIA Float64 enabled;"<<rejected<<" unsupported/missing device rejections. Current two-device machine scope.\n";std::cout<<"Device selection PASS\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
