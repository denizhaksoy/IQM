#include <IQM/hdrvdp3/pathway_quality.h>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan.h>
#include <limits>
#include <iostream>
#include <stdexcept>
class NoGPU final:public IQM::VulkanInstance {
public:
 const vk::raii::Device* device()const override{throw std::logic_error("Unexpected GPU access");}
 const vk::raii::PhysicalDevice* physicalDevice()const override{throw std::logic_error("Unexpected GPU access");}
 const std::shared_ptr<const vk::raii::CommandPool> cmdPool()const override{throw std::logic_error("Unexpected GPU access");}
 const std::shared_ptr<const vk::raii::CommandBuffer> cmdBuf()const override{throw std::logic_error("Unexpected GPU access");}
 const std::shared_ptr<const vk::raii::CommandBuffer> cmdBufTransfer()const override{throw std::logic_error("Unexpected GPU access");}
 const std::shared_ptr<const vk::raii::Queue> queue()const override{throw std::logic_error("Unexpected GPU access");}
 const std::shared_ptr<const vk::raii::Queue> queueTransfer()const override{throw std::logic_error("Unexpected GPU access");}
 void waitForFence(const vk::raii::Fence&)const override{throw std::logic_error("Unexpected GPU access");}
};
int main(){try{
 NoGPU instance;const IQM::HDRVDP3::PathwayQualityAssets assets{};std::vector<float> ref(7680,1),test(ref);IQM::HDRVDP3::PathwayQualityTrace trace;trace.logAdaptation={42};unsigned rejected=0;
 auto reject=[&](const std::vector<float>& a,const std::vector<float>& b,unsigned w,unsigned h){bool caught=false;try{IQM::HDRVDP3::evaluate_pathway_quality(instance,a,b,w,h,assets,&trace);}catch(const std::invalid_argument&){caught=true;}if(!caught||trace.logAdaptation!=std::vector<float>{42})throw std::runtime_error("Invalid input accepted or output trace changed");++rejected;};
 reject(ref,test,0,80);reject(ref,test,94,80);reject(ref,test,64,120);reject({},test,96,80);auto shortTest=test;shortTest.pop_back();reject(ref,shortTest,96,80);
 for(float bad:{0.0f,-1.0f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){auto a=ref;a[17]=bad;reject(a,test,96,80);reject(ref,a,96,80);}
 if(rejected!=13)throw std::runtime_error("Incomplete contract checks");std::cout<<"PASS:13 invalid inputs rejected before GPU access; output trace preserved.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
