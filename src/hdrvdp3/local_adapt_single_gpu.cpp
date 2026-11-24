#include <IQM/hdrvdp3/local_adaptation_resident_gpu.h>
#include <cmath>
#include <filesystem>
#include <utility>
#include <IQM/hdrvdp3/local_adapt_single_gpu.h>
#include <IQM/hdrvdp3/local_adapt.h>
#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <cstring>
#include <stdexcept>
namespace IQM::HDRVDP3 {
static OpticalNarrowingShaders make_resident_adaptation_shaders(const std::filesystem::path& runtime) {
    const auto fftDir = runtime / "fft";
    ResidentFFTShaders fwdSh{fftDir/"strided_copy.spv", fftDir/"fft720_forward.spv", {fftDir/"fft7680_gather4.spv", fftDir/"t1fv6.spv", fftDir/"t3fv16.spv", fftDir/"t3fv20.spv"}};
    ResidentFFTShaders invChild=fwdSh; invChild.fft720=fftDir/"fft720_inverse.spv";
    Inverse4320InplaceShaders inv4320{fftDir/"strided_copy.spv", {fftDir/"fft4320_inverse_gather2.spv", fftDir/"inplace4320_9.spv", fftDir/"inplace4320_12.spv", fftDir/"inplace4320_20.spv"}};
    OpticalNarrowingShaders sh;
    sh.base.padding=runtime/"padding.spv";
    sh.base.io=fftDir/"optical_io.spv";
    sh.base.forward.axis4320={fftDir/"fft4320_io.spv", fftDir/"real4320_outer.spv", fftDir/"complex4320_outer.spv", fwdSh};
    sh.base.forward.axis7680=fwdSh;
    sh.base.inverse.axis4320={fftDir/"fft4320_io.spv", fftDir/"real4320_outer.spv", fftDir/"complex4320_outer.spv", invChild};
    sh.base.inverse.axis7680={fftDir/"strided_copy.spv", {fftDir/"fft7680_inverse_gather4.spv", fftDir/"t1bv6.spv", fftDir/"t3bv16.spv", fftDir/"t3bv20.spv"}};
    sh.inverse4320=inv4320;
    sh.narrowing=fftDir/"complex_narrowing.spv";
    sh.select=fftDir/"optical_branch_select.spv";
    return sh;
}
GpuFloatBuffer local_adaptation_resident_gpu(const IQM::VulkanInstance& instance,
    vk::Buffer inputAdaptation, unsigned width, unsigned height, double ppd,
    const std::filesystem::path& runtime) {
    if (!inputAdaptation || width != 3840 || height != 2160 || ppd != 62.729130063099447)
        throw std::invalid_argument("Resident adaptation: unsupported input/profile");
    const auto& dev=*instance.device();
    const auto& phys=*instance.physicalDevice();
    constexpr size_t N=3840ull*2160, P=7680ull*4320, Half=7680ull*2161;
    constexpr size_t channelBytes=N*4, realBytes=P*4,
        complexBytes=P*8, halfBytes=Half*8, wsBytes=1024ull*4320*8;
    const auto limits=phys.getProperties().limits;
    if (complexBytes>limits.maxStorageBufferRange)
        throw std::runtime_error("Resident adaptation: unsupported device limits");
    struct OwnedBuffer { vk::raii::DeviceMemory second{nullptr}; vk::raii::Buffer first{nullptr}; };
    auto makeBuffer=[&](size_t bytes, bool host=false) {
        auto usage=vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst;
        vk::MemoryPropertyFlags props=host ? (vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent) : vk::MemoryPropertyFlags(vk::MemoryPropertyFlagBits::eDeviceLocal);
        auto b=IQM::Bin::VulkanResource::createBuffer(dev,phys,static_cast<unsigned>(bytes),usage,props);
        b.first.bindMemory(b.second,0);
        return OwnedBuffer{std::move(b.second),std::move(b.first)};
    };
    auto output=makeBuffer(channelBytes), logPlane=makeBuffer(channelBytes), crop=makeBuffer(channelBytes), filter=makeBuffer(realBytes), padded=makeBuffer(realBytes), half=makeBuffer(halfBytes);
    auto spectrum=makeBuffer(complexBytes), inverse=makeBuffer(complexBytes), intermediate=makeBuffer(complexBytes), alternateInverse=makeBuffer(complexBytes);
    auto status=makeBuffer(4), fftStatus=makeBuffer(4), productStatus=makeBuffer(4);
    auto ws0=makeBuffer(wsBytes), ws1=makeBuffer(wsBytes), ws2=makeBuffer(wsBytes), ws3=makeBuffer(wsBytes);
    auto staging=makeBuffer(realBytes,true);
    // Static Gaussian coefficients, same expression and layout as Phase 4B.
    {
        const double sigma=std::pow(10.0,-0.781367)*ppd;
        const double pi2=2*3.14159265358979323846;
        auto* mapped=static_cast<float*>(staging.second.mapMemory(0,realBytes));
        for(unsigned x=0;x<7680;++x) for(unsigned y=0;y<4320;++y){
            double kx=(std::fmod(0.5+double(x)/7680,1.0)-0.5)*pi2;
            double ky=(std::fmod(0.5+double(y)/4320,1.0)-0.5)*pi2;
            mapped[size_t(x)*4320+y]=float(std::exp((-0.5*(kx*kx+ky*ky))*(sigma*sigma)));
        }
        staging.second.unmapMemory();
    }
    auto reg=[](auto& b,size_t bytes){return FFTBufferRegion{*b.first,0,bytes};};
    const auto shaders=make_resident_adaptation_shaders(runtime);
    LocalAdaptationBuffers la;
    auto& b=la.convolution;
    b.base.input=reg(logPlane,channelBytes);
    b.base.output=reg(output,channelBytes);
    b.base.filter=reg(filter,realBytes); b.base.padded=reg(padded,realBytes);
    b.base.half=reg(half,halfBytes); b.base.spectrum=reg(spectrum,complexBytes);
    b.base.inverse=reg(inverse,complexBytes); b.base.fftIntermediate=reg(intermediate,complexBytes);
    b.base.status=reg(status,4);
    b.base.workspace={reg(ws0,wsBytes),reg(ws1,wsBytes),reg(ws2,wsBytes),reg(ws3,wsBytes)};
    b.alternateInverse=reg(alternateInverse,complexBytes);
    b.fftStatus=reg(fftStatus,4); b.productStatus=reg(productStatus,4);
    la.luminance={FFTBufferRegion{inputAdaptation,0,channelBytes},0,1,width};
    la.output={b.base.output,0,1,width};
    la.crop=reg(crop,channelBytes);
    const auto fftDir=runtime/"fft";
    ResidentLocalAdaptation job(dev,phys,la,shaders,fftDir/"local_adaptation_io.spv",fftDir/"local_adaptation_fft_io.spv");
    instance.cmdBuf()->reset();
    instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    vk::CommandBuffer cmd=*instance.cmdBuf();
    cmd.copyBuffer(*staging.first,*filter.first,{vk::BufferCopy{0,0,realBytes}});
    auto barrier=[&](){
        vk::MemoryBarrier b{.srcAccessMask=vk::AccessFlagBits::eMemoryRead|vk::AccessFlagBits::eMemoryWrite,
            .dstAccessMask=vk::AccessFlagBits::eMemoryRead|vk::AccessFlagBits::eMemoryWrite};
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,vk::PipelineStageFlagBits::eAllCommands,{}, {b},{},{});
    };
    barrier();
    job.record(cmd); barrier();
    instance.cmdBuf()->end();
    vk::raii::Fence fence(dev,vk::FenceCreateInfo{});
    instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);
    instance.waitForFence(fence);
    instance.cmdBuf()->reset();
    return GpuFloatBuffer{std::move(output.first),std::move(output.second),static_cast<unsigned>(channelBytes),static_cast<unsigned>(N)};
}
static IQM::Bin::VulkanImage makeR32(const IQM::VulkanInstance& inst,unsigned w,unsigned h){
  vk::ImageCreateInfo info{.imageType=vk::ImageType::e2D,.format=vk::Format::eR32Sfloat,.extent={w,h,1},.mipLevels=1,.arrayLayers=1,.samples=vk::SampleCountFlagBits::e1,.tiling=vk::ImageTiling::eOptimal,.usage=vk::ImageUsageFlagBits::eStorage|vk::ImageUsageFlagBits::eTransferSrc|vk::ImageUsageFlagBits::eTransferDst,.sharingMode=vk::SharingMode::eExclusive,.initialLayout=vk::ImageLayout::eUndefined};
  return IQM::Bin::VulkanResource::createImage(*inst.device(),*inst.physicalDevice(),info);
}
static void trans(vk::CommandBuffer cmd,vk::Image img,vk::ImageLayout oldL,vk::ImageLayout newL,vk::AccessFlags srcA,vk::AccessFlags dstA,vk::PipelineStageFlags srcS,vk::PipelineStageFlags dstS){
  vk::ImageMemoryBarrier b{.srcAccessMask=srcA,.dstAccessMask=dstA,.oldLayout=oldL,.newLayout=newL,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.image=img,.subresourceRange={vk::ImageAspectFlagBits::eColor,0,1,0,1}};
  cmd.pipelineBarrier(srcS,dstS,{},{},{},{b});
}
GpuFloatBuffer local_adaptation_single_gpu(const IQM::VulkanInstance& instance,
    vk::Buffer inputAdaptation, unsigned width, unsigned height, float ppd){
  if(!inputAdaptation||!width||!height) throw std::invalid_argument("local_adapt gpu: bad input");
  const size_t n=size_t(width)*height; const unsigned bytes=unsigned(n*4);
  auto out=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),bytes,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eDeviceLocal); out.first.bindMemory(out.second,0);
  auto inImg=makeR32(instance,width,height); auto outImg=makeR32(instance,width,height);
  LocalAdaptFFT fft(*instance.device(),*instance.physicalDevice());
  instance.cmdBuf()->reset(); instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  vk::CommandBuffer cmd=*instance.cmdBuf();
  trans(cmd,*inImg.image,vk::ImageLayout::eUndefined,vk::ImageLayout::eGeneral,{},vk::AccessFlagBits::eTransferWrite,vk::PipelineStageFlagBits::eTopOfPipe,vk::PipelineStageFlagBits::eTransfer);
  trans(cmd,*outImg.image,vk::ImageLayout::eUndefined,vk::ImageLayout::eGeneral,{},vk::AccessFlagBits::eShaderWrite,vk::PipelineStageFlagBits::eTopOfPipe,vk::PipelineStageFlagBits::eComputeShader);
  vk::BufferImageCopy bic{.bufferOffset=0,.bufferRowLength=0,.bufferImageHeight=0,.imageSubresource={vk::ImageAspectFlagBits::eColor,0,0,1},.imageOffset={0,0,0},.imageExtent={width,height,1}};
  cmd.copyBufferToImage(inputAdaptation,*inImg.image,vk::ImageLayout::eGeneral,{bic});
  trans(cmd,*inImg.image,vk::ImageLayout::eGeneral,vk::ImageLayout::eGeneral,vk::AccessFlagBits::eTransferWrite,vk::AccessFlagBits::eShaderRead,vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eComputeShader);
  LocalAdaptFFTInput in{.device=instance.device(),.physicalDevice=instance.physicalDevice(),.cmdBuf=instance.cmdBuf().get(),.cmdPool=instance.cmdPool().get(),.queue=instance.queue().get(),.ivInput=&inImg.imageView,.ivOutput=&outImg.imageView,.ppd=ppd,.mode=FFTFilterMode::LocalAdaptation,.columnFirst=false,.useTwiddleLUT=false,.referenceShaderPath="",.referenceMathDirectory="",.traceBuffer=nullptr,.width=width,.height=height};
  fft.compute(in);
  vk::ImageMemoryBarrier outReady{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eTransferRead,.oldLayout=vk::ImageLayout::eGeneral,.newLayout=vk::ImageLayout::eGeneral,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.image=*outImg.image,.subresourceRange={vk::ImageAspectFlagBits::eColor,0,1,0,1}};
  cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eTransfer,{},{},{},{outReady});
  cmd.copyImageToBuffer(*outImg.image,vk::ImageLayout::eGeneral,*out.first,{bic});
  vk::MemoryBarrier done{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eTransferRead};
  cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eTransfer,{},{done},{},{});
  instance.cmdBuf()->end(); vk::CommandBuffer c=*instance.cmdBuf(); vk::raii::Fence f(*instance.device(),vk::FenceCreateInfo{}); instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&c},f); instance.waitForFence(f);
  instance.cmdBuf()->reset();
  return GpuFloatBuffer{std::move(out.first),std::move(out.second),bytes,unsigned(n)};
}
std::vector<float> local_adaptation_single_gpu_readback(const IQM::VulkanInstance& instance,
    vk::Buffer inputAdaptation, unsigned width, unsigned height, float ppd){
  auto buf=local_adaptation_single_gpu(instance,inputAdaptation,width,height,ppd);
  auto staging=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),buf.bytes,vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent); staging.first.bindMemory(staging.second,0);
  instance.cmdBuf()->reset(); instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  instance.cmdBuf()->copyBuffer(*buf.buffer,*staging.first,{vk::BufferCopy{0,0,buf.bytes}});
  vk::MemoryBarrier hb{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};
  instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},{hb},{},{});
  instance.cmdBuf()->end(); vk::CommandBuffer c=*instance.cmdBuf(); vk::raii::Fence f(*instance.device(),vk::FenceCreateInfo{}); instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&c},f); instance.waitForFence(f);
  std::vector<float> out(buf.count); void* p=staging.second.mapMemory(0,buf.bytes); std::memcpy(out.data(),p,buf.bytes); staging.second.unmapMemory(); instance.cmdBuf()->reset(); return out;
}
}
