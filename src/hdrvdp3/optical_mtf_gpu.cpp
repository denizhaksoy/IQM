#include <IQM/hdrvdp3/optical_narrowing_gpu.h>
#include <IQM/hdrvdp3/mtf.h>
#include <array>
#include <cmath>
#include <filesystem>
#include <memory>
#include <cstdint>
#include <IQM/hdrvdp3/optical_mtf_gpu.h>
#include <IQM/hdrvdp3/pq_display_single_gpu.h>
#include <IQM/hdrvdp3/local_adapt.h>
#include <IQM/base/vulkan_runtime.h>
#include <vulkan_instance.h>
#include <vulkan_res.h>
#include <cstring>
#include <stdexcept>
#include <vector>
namespace IQM::HDRVDP3 {
static bool supports_resident_optical(unsigned width, unsigned height, double ppd) {
    return width == 3840 && height == 2160 && std::isfinite(ppd)
        && ppd == 62.729130063099447;
}
static OpticalNarrowingShaders make_resident_optical_shaders(const std::filesystem::path& runtime) {
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
GpuFloatBuffer filter_optical_rgb_resident_gpu(const IQM::VulkanInstance& instance,
    vk::Buffer inputRgb, unsigned width, unsigned height, double ppd,
    const std::filesystem::path& runtime) {
    if (!inputRgb || !supports_resident_optical(width,height,ppd))
        throw std::invalid_argument("Resident optical: unsupported input/profile");
    const auto& dev=*instance.device();
    const auto& phys=*instance.physicalDevice();
    constexpr size_t N=3840ull*2160, P=7680ull*4320, Half=7680ull*2161;
    constexpr size_t channelBytes=N*4, rgbBytes=3*channelBytes, realBytes=P*4,
        complexBytes=P*8, halfBytes=Half*8, wsBytes=1024ull*4320*8;
    const auto limits=phys.getProperties().limits;
    if (complexBytes>limits.maxStorageBufferRange || channelBytes%limits.minStorageBufferOffsetAlignment)
        throw std::runtime_error("Resident optical: unsupported device limits");
    auto makeBuffer=[&](size_t bytes, bool host=false) {
        auto usage=vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst;
        vk::MemoryPropertyFlags props=host ? (vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent) : vk::MemoryPropertyFlags(vk::MemoryPropertyFlagBits::eDeviceLocal);
        auto b=IQM::Bin::VulkanResource::createBuffer(dev,phys,static_cast<unsigned>(bytes),usage,props);
        b.first.bindMemory(b.second,0);
        return b;
    };
    auto output=makeBuffer(rgbBytes), filter=makeBuffer(realBytes), padded=makeBuffer(realBytes), half=makeBuffer(halfBytes);
    auto spectrum=makeBuffer(complexBytes), inverse=makeBuffer(complexBytes), intermediate=makeBuffer(complexBytes), alternateInverse=makeBuffer(complexBytes);
    auto status=makeBuffer(4), fftStatus=makeBuffer(4), productStatus=makeBuffer(4);
    auto ws0=makeBuffer(wsBytes), ws1=makeBuffer(wsBytes), ws2=makeBuffer(wsBytes), ws3=makeBuffer(wsBytes);
    auto staging=makeBuffer(realBytes,true);
    // Static model coefficients only; no input image readback.
    {
        std::vector<double> mtfRow(P);
        generate_mtf_filter(mtfRow.data(),7680,4320,ppd);
        auto* mapped=static_cast<float*>(staging.second.mapMemory(0,realBytes));
        for(unsigned x=0;x<7680;++x) for(unsigned y=0;y<4320;++y)
            mapped[size_t(x)*4320+y]=static_cast<float>(mtfRow[size_t(y)*7680+x]);
        staging.second.unmapMemory();
    }
    auto reg=[](auto& b,size_t bytes){return FFTBufferRegion{*b.first,0,bytes};};
    const auto shaders=make_resident_optical_shaders(runtime);
    std::vector<std::unique_ptr<ResidentOpticalNarrowing>> jobs;
    jobs.reserve(3);
    for(unsigned ch=0;ch<3;++ch){
        OpticalNarrowingBuffers b;
        b.base.input={inputRgb,size_t(ch)*channelBytes,channelBytes};
        b.base.output={*output.first,size_t(ch)*channelBytes,channelBytes};
        b.base.filter=reg(filter,realBytes); b.base.padded=reg(padded,realBytes);
        b.base.half=reg(half,halfBytes); b.base.spectrum=reg(spectrum,complexBytes);
        b.base.inverse=reg(inverse,complexBytes); b.base.fftIntermediate=reg(intermediate,complexBytes);
        b.base.status=reg(status,4);
        b.base.workspace={reg(ws0,wsBytes),reg(ws1,wsBytes),reg(ws2,wsBytes),reg(ws3,wsBytes)};
        b.alternateInverse=reg(alternateInverse,complexBytes);
        b.fftStatus=reg(fftStatus,4); b.productStatus=reg(productStatus,4);
        jobs.push_back(std::make_unique<ResidentOpticalNarrowing>(dev,phys,b,shaders));
    }
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
    for(const auto& job:jobs){job->record(cmd); barrier();}
    instance.cmdBuf()->end();
    vk::raii::Fence fence(dev,vk::FenceCreateInfo{});
    instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&cmd},fence);
    instance.waitForFence(fence);
    instance.cmdBuf()->reset();
    return GpuFloatBuffer{std::move(output.first),std::move(output.second),static_cast<unsigned>(rgbBytes),static_cast<unsigned>(3*N)};
}

static IQM::Bin::VulkanImage makeR32Image(const IQM::VulkanInstance& inst, unsigned w, unsigned h){
  vk::ImageCreateInfo info{.imageType=vk::ImageType::e2D,.format=vk::Format::eR32Sfloat,.extent={w,h,1},.mipLevels=1,.arrayLayers=1,.samples=vk::SampleCountFlagBits::e1,.tiling=vk::ImageTiling::eOptimal,.usage=vk::ImageUsageFlagBits::eStorage|vk::ImageUsageFlagBits::eTransferSrc|vk::ImageUsageFlagBits::eTransferDst,.sharingMode=vk::SharingMode::eExclusive,.initialLayout=vk::ImageLayout::eUndefined};
  return IQM::Bin::VulkanResource::createImage(*inst.device(),*inst.physicalDevice(),info);
}
static void transition(vk::CommandBuffer cmd, vk::Image img, vk::ImageLayout oldL, vk::ImageLayout newL, vk::AccessFlags srcA, vk::AccessFlags dstA, vk::PipelineStageFlags srcS, vk::PipelineStageFlags dstS){
  vk::ImageMemoryBarrier b{.srcAccessMask=srcA,.dstAccessMask=dstA,.oldLayout=oldL,.newLayout=newL,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.image=img,.subresourceRange={vk::ImageAspectFlagBits::eColor,0,1,0,1}};
  std::array<vk::ImageMemoryBarrier,1> bs{b}; cmd.pipelineBarrier(srcS,dstS,{}, {}, {}, bs);
}
GpuFloatBuffer filter_optical_rgb_single_gpu(const IQM::VulkanInstance& instance,
    vk::Buffer inputRgb, unsigned width, unsigned height, float ppd){
  if(!inputRgb||!width||!height) throw std::invalid_argument("optical gpu: bad input");
  const size_t n=size_t(width)*height; const unsigned bytes=unsigned(n*4);
  auto out=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),bytes*3,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eDeviceLocal); out.first.bindMemory(out.second,0);
  LocalAdaptFFT fft(*instance.device(),*instance.physicalDevice());
  for(unsigned ch=0; ch<3; ++ch){
    auto inImg=makeR32Image(instance,width,height); auto outImg=makeR32Image(instance,width,height);
    instance.cmdBuf()->reset(); instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    vk::CommandBuffer cmd=*instance.cmdBuf();
    transition(cmd,*inImg.image,vk::ImageLayout::eUndefined,vk::ImageLayout::eGeneral,{},vk::AccessFlagBits::eTransferWrite,vk::PipelineStageFlagBits::eTopOfPipe,vk::PipelineStageFlagBits::eTransfer);
    transition(cmd,*outImg.image,vk::ImageLayout::eUndefined,vk::ImageLayout::eGeneral,{},vk::AccessFlagBits::eShaderWrite,vk::PipelineStageFlagBits::eTopOfPipe,vk::PipelineStageFlagBits::eComputeShader);
    vk::BufferImageCopy bic{.bufferOffset=size_t(ch)*bytes,.bufferRowLength=0,.bufferImageHeight=0,.imageSubresource={vk::ImageAspectFlagBits::eColor,0,0,1},.imageOffset={0,0,0},.imageExtent={width,height,1}};
    cmd.copyBufferToImage(inputRgb,*inImg.image,vk::ImageLayout::eGeneral,{bic});
    transition(cmd,*inImg.image,vk::ImageLayout::eGeneral,vk::ImageLayout::eGeneral,vk::AccessFlagBits::eTransferWrite,vk::AccessFlagBits::eShaderRead,vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eComputeShader);
    LocalAdaptFFTInput in{.device=instance.device(),.physicalDevice=instance.physicalDevice(),.cmdBuf=instance.cmdBuf().get(),.cmdPool=instance.cmdPool().get(),.queue=instance.queue().get(),.ivInput=&inImg.imageView,.ivOutput=&outImg.imageView,.ppd=ppd,.mode=FFTFilterMode::OpticalMTF,.columnFirst=false,.useTwiddleLUT=false,.referenceShaderPath="",.referenceMathDirectory="",.traceBuffer=nullptr,.width=width,.height=height};
    fft.compute(in);
    vk::ImageMemoryBarrier outReady{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,.dstAccessMask=vk::AccessFlagBits::eTransferRead,.oldLayout=vk::ImageLayout::eGeneral,.newLayout=vk::ImageLayout::eGeneral,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.image=*outImg.image,.subresourceRange={vk::ImageAspectFlagBits::eColor,0,1,0,1}};
    std::array<vk::ImageMemoryBarrier,1> ors{outReady}; cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eTransfer,{}, {}, {}, ors);
    cmd.copyImageToBuffer(*outImg.image,vk::ImageLayout::eGeneral,out.first,{bic});
    vk::MemoryBarrier hostDone{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eTransferRead};
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eTransfer,{},{hostDone},{},{});
    instance.cmdBuf()->end(); vk::CommandBuffer c=*instance.cmdBuf(); vk::raii::Fence f(*instance.device(),vk::FenceCreateInfo{}); instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&c},f); instance.waitForFence(f);
  }
  instance.cmdBuf()->reset();
  return GpuFloatBuffer{std::move(out.first),std::move(out.second),bytes*3,unsigned(n*3)};
}
std::vector<float> filter_optical_rgb_single_gpu_readback(const IQM::VulkanInstance& instance,
    vk::Buffer inputRgb, unsigned width, unsigned height, float ppd){
  auto buf=filter_optical_rgb_single_gpu(instance,inputRgb,width,height,ppd);
  auto staging=IQM::Bin::VulkanResource::createBuffer(*instance.device(),*instance.physicalDevice(),buf.bytes,vk::BufferUsageFlagBits::eTransferDst,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent); staging.first.bindMemory(staging.second,0);
  instance.cmdBuf()->reset(); instance.cmdBuf()->begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  instance.cmdBuf()->copyBuffer(*buf.buffer,*staging.first,{vk::BufferCopy{0,0,buf.bytes}});
  vk::MemoryBarrier hb{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,.dstAccessMask=vk::AccessFlagBits::eHostRead};
  instance.cmdBuf()->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},{hb},{},{});
  instance.cmdBuf()->end(); vk::CommandBuffer c=*instance.cmdBuf(); vk::raii::Fence f(*instance.device(),vk::FenceCreateInfo{}); instance.queue()->submit(vk::SubmitInfo{.commandBufferCount=1,.pCommandBuffers=&c},f); instance.waitForFence(f);
  std::vector<float> out(buf.count); void* p=staging.second.mapMemory(0,buf.bytes); std::memcpy(out.data(),p,buf.bytes); staging.second.unmapMemory(); instance.cmdBuf()->reset(); return out;
}
}