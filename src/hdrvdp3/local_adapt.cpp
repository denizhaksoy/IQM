/*

 * Image Quality Metrics - HDR-VDP-3 Local Adaptation

 * Deniz Aksoy - 2026

 */

#include <fstream>

#include <vkFFT.h>

#include <stdexcept>

#include <string>

#include <IQM/hdrvdp3/local_adapt.h>
#include <IQM/hdrvdp3/mtf.h>

#include <IQM/base/vulkan_runtime.h>

#include <iostream>

#include <cmath>

#include <vector>

#include <cstring>

#include <array>

#include <numeric>

namespace IQM {

namespace HDRVDP3 {

static int computeKernelSize(float sigma) {

    // Gaussian kernel radius should cover enough of the Gaussian for good accuracy

    // with circular convolution on 128x128. The convergence experiment showed:

    // - sigma=4.96 (ppd=30): 51 taps optimal (31 too small, >51 no float32 improvement)

    // - Higher sigma: need larger kernel

    // Use radius = ceil(5 * sigma) clamped to [15, 40] -> ksize in [31, 81]

    int radius = static_cast<int>(std::ceil(5.0f * sigma));

    int ksize = 2 * radius + 1;

    ksize = std::max(31, ksize);

    ksize = std::min(81, ksize);

    // Ensure odd

    if (ksize % 2 == 0) ksize++;

    return ksize;

}

static std::vector<float> generateGaussianKernel2D(double sigma, unsigned width, unsigned height) {

    // Generate 2D frequency-domain Gaussian kernel matching fast_gauss.m exactly

    // KX0 = mod(0.5 + (0:(M-1))/M, 1) - 0.5

    // KX1 = KX0 * (2*pi)

    // KY0 = mod(0.5 + (0:(N-1))/N, 1) - 0.5

    // KY1 = KY0 * (2*pi)

    // K = exp(-0.5 * (XX.^2 + YY.^2) * sigma^2)

    //

    // This creates a 2D filter in the frequency domain (NOT fftshifted)

    // DC component is at index (0, 0)

    const double two_pi = 2.0 * 3.14159265358979323846;

    const double sigma2 = sigma * sigma;

    std::vector<float> kernel(width * height);

    for (unsigned y = 0; y < height; ++y) {

        double ky0 = std::fmod(0.5 + static_cast<double>(y) / height, 1.0) - 0.5;

        double ky1 = ky0 * two_pi;

        for (unsigned x = 0; x < width; ++x) {

            double kx0 = std::fmod(0.5 + static_cast<double>(x) / width, 1.0) - 0.5;

            double kx1 = kx0 * two_pi;

            float val = static_cast<float>(std::exp(-0.5 * (kx1 * kx1 + ky1 * ky1) * sigma2));

            kernel[y * width + x] = val;

        }

    }

    return kernel;

}

static uint32_t findMemoryType(

    const vk::raii::PhysicalDevice& physicalDevice,

    uint32_t typeBits,

    vk::MemoryPropertyFlags requirementsMask)

{

    auto memoryProperties = physicalDevice.getMemoryProperties();

    uint32_t typeIndex = static_cast<uint32_t>(~0);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {

        if ((typeBits & 1) && ((memoryProperties.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask)) {

            typeIndex = i;

            break;

        }

        typeBits >>= 1;

    }

    return typeIndex;

}

static std::vector<float> generateGaussianKernel1D(float sigma, int ksize) {

    const int radius = ksize / 2;

    // Generate 1D Gaussian kernel (matching the 1D slice used for separable convolution)

    // For separable convolution, we need a 1D kernel that sums to 1

    std::vector<float> kernel1D(ksize);

    float sum1D = 0.0f;

    for (int i = 0; i < ksize; ++i) {

        float dx = static_cast<float>(i - radius);

        float val = std::exp(-0.5f * dx * dx / (sigma * sigma));

        kernel1D[i] = val;

        sum1D += val;

    }

    for (float& v : kernel1D) {

        v /= sum1D;

    }

    return kernel1D;

}

LocalAdapt::LocalAdapt(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice)

    : descPool(IQM::GPU::VulkanRuntime::createDescPool(device, 5, {

        vk::DescriptorPoolSize{.type = vk::DescriptorType::eStorageImage, .descriptorCount = 10},

        vk::DescriptorPoolSize{.type = vk::DescriptorType::eStorageBuffer, .descriptorCount = 5},

    }))

{

    // Generate Gaussian kernels for sigma = 10^-0.781367 * ppd

    // For ppd=30, sigma = 4.9631, ksize = 51 (based on convergence experiment)

    const float sigma = std::pow(10.0f, -0.781367f) * 30.0f;

    const int ksize = computeKernelSize(sigma);

    auto kernel1D = generateGaussianKernel1D(sigma, ksize);

    // Create 1D kernel buffer for shaders (dynamic size based on sigma)

    const unsigned kernel1DBufferSize = ksize * sizeof(float);

    vk::BufferCreateInfo bufferCreateInfo{

        .size = kernel1DBufferSize,

        .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,

    };

    vk::raii::Buffer buffer{device, bufferCreateInfo};

    auto memReqs = buffer.getMemoryRequirements();

    const auto memType = findMemoryType(

        physicalDevice,

        memReqs.memoryTypeBits,

        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent

    );

    vk::MemoryAllocateInfo memoryAllocateInfo{

        .allocationSize = memReqs.size,

        .memoryTypeIndex = memType

    };

    vk::raii::DeviceMemory memory{device, memoryAllocateInfo};

    buffer.bindMemory(memory, 0);

    void* mapped = memory.mapMemory(0, kernel1DBufferSize, {});

    std::memcpy(mapped, kernel1D.data(), kernel1DBufferSize);

    memory.unmapMemory();

    kernelBuffer = std::move(buffer);

    kernelMemory = std::move(memory);

    kernelSize = ksize;  // Store for push constant

    // Descriptor pool - need enough sets for 5 descriptor sets + kernel buffer

    descPool = IQM::GPU::VulkanRuntime::createDescPool(device, 5, {

        vk::DescriptorPoolSize{.type = vk::DescriptorType::eStorageImage, .descriptorCount = 10}, // 2 per set * 5 sets

        vk::DescriptorPoolSize{.type = vk::DescriptorType::eStorageBuffer, .descriptorCount = 5}, // 1 per set

    });

    // Descriptor set layout with explicit binding indices:

    // binding 0: input image (STORAGE_IMAGE)

    // binding 1: output image (STORAGE_IMAGE)

    // binding 6: kernel buffer (STORAGE_BUFFER)

    vk::DescriptorSetLayoutBinding bindings[] = {

        { .binding = 0, .descriptorType = vk::DescriptorType::eStorageImage, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute },

        { .binding = 1, .descriptorType = vk::DescriptorType::eStorageImage, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute },

        { .binding = 6, .descriptorType = vk::DescriptorType::eStorageBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute },

    };

    vk::DescriptorSetLayoutCreateInfo layoutInfo{

        .bindingCount = 3,

        .pBindings = bindings,

    };

    descSetLayout = vk::raii::DescriptorSetLayout{device, layoutInfo};

    // Create 5 descriptor sets (one per pass)

    std::array<vk::DescriptorSetLayout, 5> setLayouts = {*descSetLayout, *descSetLayout, *descSetLayout, *descSetLayout, *descSetLayout};

    vk::DescriptorSetAllocateInfo allocInfo{

        .descriptorPool = descPool,

        .descriptorSetCount = 5,

        .pSetLayouts = setLayouts.data(),

    };

    descSets = std::move(vk::raii::DescriptorSets{device, allocInfo});

    // Pipeline layout with push constants for ppd and kernel_size

    struct PushConstants {

        float ppd;

        int kernel_size;

    };

    vk::PushConstantRange pushConstantRange{

        .stageFlags = vk::ShaderStageFlagBits::eCompute,

        .offset = 0,

        .size = sizeof(PushConstants),

    };

    layout = IQM::GPU::VulkanRuntime::createPipelineLayout(device, {*descSetLayout}, {pushConstantRange});

    // Load shaders

    static std::vector<uint32_t> logClampSrc =

#include <hdrvdp3/hdrvdp3_local_adapt_logclamp.inc>

    ;

    static std::vector<uint32_t> padSrc =

#include <hdrvdp3/hdrvdp3_local_adapt_pad.inc>

    ;

    static std::vector<uint32_t> gaussHSrc =

#include <hdrvdp3/hdrvdp3_local_adapt_gaussian_h.inc>

    ;

    static std::vector<uint32_t> gaussVSrc =

#include <hdrvdp3/hdrvdp3_local_adapt_gaussian_v.inc>

    ;

    static std::vector<uint32_t> cropExpSrc =

#include <hdrvdp3/hdrvdp3_local_adapt_crop_exp.inc>

    ;

    auto logClampModule = IQM::GPU::VulkanRuntime::createShaderModule(device, logClampSrc);

    auto padModule = IQM::GPU::VulkanRuntime::createShaderModule(device, padSrc);

    auto gaussHModule = IQM::GPU::VulkanRuntime::createShaderModule(device, gaussHSrc);

    auto gaussVModule = IQM::GPU::VulkanRuntime::createShaderModule(device, gaussVSrc);

    auto cropExpModule = IQM::GPU::VulkanRuntime::createShaderModule(device, cropExpSrc);

    pipelineLogClamp = IQM::GPU::VulkanRuntime::createComputePipeline(device, logClampModule, layout);

    pipelinePad = IQM::GPU::VulkanRuntime::createComputePipeline(device, padModule, layout);

    pipelineGaussH = IQM::GPU::VulkanRuntime::createComputePipeline(device, gaussHModule, layout);

    pipelineGaussV = IQM::GPU::VulkanRuntime::createComputePipeline(device, gaussVModule, layout);

    pipelineCropExp = IQM::GPU::VulkanRuntime::createComputePipeline(device, cropExpModule, layout);

    // Update each descriptor set with kernel buffer at binding 6

    vk::DescriptorBufferInfo kernelBufInfo{

        .buffer = kernelBuffer,

        .offset = 0,

        .range = VK_WHOLE_SIZE,

    };

    for (size_t i = 0; i < 5; ++i) {

        vk::WriteDescriptorSet kernelWrite{

            .dstSet = descSets[i],

            .dstBinding = 6,

            .dstArrayElement = 0,

            .descriptorCount = 1,

            .descriptorType = vk::DescriptorType::eStorageBuffer,

            .pBufferInfo = &kernelBufInfo,

        };

        device.updateDescriptorSets({kernelWrite}, {});

    }

}

void LocalAdapt::compute(const LocalAdaptInput& input) const {

    vk::CommandBuffer cmdBuf = *input.cmdBuf;

    struct PushConstants {

        float ppd;

        int kernel_size;

    };

    PushConstants pc{input.ppd, kernelSize};

    cmdBuf.pushConstants<PushConstants>(layout, vk::ShaderStageFlagBits::eCompute, 0, {pc});

    // Helper lambda to update descriptor set bindings 0 and 1

    auto updateBindings01 = [&](const vk::raii::DescriptorSet& descSet, 

                                 const vk::raii::ImageView* inView, 

                                 const vk::raii::ImageView* outView,

                                 const vk::raii::Device* device) {

        auto inInfo = IQM::GPU::VulkanRuntime::createImageInfos({inView});

        auto outInfo = IQM::GPU::VulkanRuntime::createImageInfos({outView});

        vk::WriteDescriptorSet writes[] = {

            { .dstSet = descSet, .dstBinding = 0, .dstArrayElement = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageImage, .pImageInfo = inInfo.data() },

            { .dstSet = descSet, .dstBinding = 1, .dstArrayElement = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageImage, .pImageInfo = outInfo.data() },

        };

        device->updateDescriptorSets(writes, {});

    };

    vk::MemoryBarrier barrier{

        .srcAccessMask = vk::AccessFlagBits::eShaderWrite,

        .dstAccessMask = vk::AccessFlagBits::eShaderRead,

    };

    // Pass 1: Log-clamp (64x64 -> 64x64) - descSets[0]

    updateBindings01(descSets[0], input.ivInput, input.ivLogClamp, input.device);

    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, layout, 0, {descSets[0]}, {});

    cmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, pipelineLogClamp);

    cmdBuf.dispatch(4, 4, 1);

    cmdBuf.pipelineBarrier(

        vk::PipelineStageFlagBits::eComputeShader,

        vk::PipelineStageFlagBits::eComputeShader,

        {}, {barrier}, {}, {}

    );

    // Pass 2: Pad 64x64 -> 128x128 (replicate on bottom/right) - descSets[1]

    updateBindings01(descSets[1], input.ivLogClamp, input.ivPadded, input.device);

    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, layout, 0, {descSets[1]}, {});

    cmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, pipelinePad);

    cmdBuf.dispatch(8, 8, 1);

    cmdBuf.pipelineBarrier(

        vk::PipelineStageFlagBits::eComputeShader,

        vk::PipelineStageFlagBits::eComputeShader,

        {}, {barrier}, {}, {}

    );

    // Pass 3: Horizontal Gaussian (128x128 -> 128x128) - descSets[2]

    updateBindings01(descSets[2], input.ivPadded, input.ivGaussH, input.device);

    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, layout, 0, {descSets[2]}, {});

    cmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, pipelineGaussH);

    cmdBuf.dispatch(8, 8, 1);

    cmdBuf.pipelineBarrier(

        vk::PipelineStageFlagBits::eComputeShader,

        vk::PipelineStageFlagBits::eComputeShader,

        {}, {barrier}, {}, {}

    );

    // Pass 4: Vertical Gaussian (128x128 -> 128x128) - descSets[3]

    updateBindings01(descSets[3], input.ivGaussH, input.ivGaussV, input.device);

    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, layout, 0, {descSets[3]}, {});

    cmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, pipelineGaussV);

    cmdBuf.dispatch(8, 8, 1);

    cmdBuf.pipelineBarrier(

        vk::PipelineStageFlagBits::eComputeShader,

        vk::PipelineStageFlagBits::eComputeShader,

        {}, {barrier}, {}, {}

    );

    // Pass 5: Crop 128x128 -> 64x64 and Exp - descSets[4]

    updateBindings01(descSets[4], input.ivGaussV, input.ivOutput, input.device);

    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, layout, 0, {descSets[4]}, {});

    cmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, pipelineCropExp);

    cmdBuf.dispatch(4, 4, 1);

    // Final barrier for output

    vk::MemoryBarrier finalBarrier{

        .srcAccessMask = vk::AccessFlagBits::eShaderWrite,

        .dstAccessMask = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eTransferRead,

    };

    cmdBuf.pipelineBarrier(

        vk::PipelineStageFlagBits::eComputeShader,

        vk::PipelineStageFlagBits::eComputeShader | vk::PipelineStageFlagBits::eTransfer,

        {}, {finalBarrier}, {}, {}

    );

}

// ============================================================================

// FFT-based Local Adaptation

// ============================================================================

struct LocalAdaptFFT::Impl {

    vk::raii::DeviceMemory parityScratchMemory = nullptr;
    vk::raii::Buffer parityScratch = nullptr;
    vk::raii::DescriptorSetLayout paritySetLayout = nullptr;
    vk::raii::DescriptorPool parityPool = nullptr;
    vk::raii::PipelineLayout parityLayout = nullptr;
    vk::raii::Pipeline parityPipeline = nullptr;
    std::vector<vk::raii::DescriptorSet> paritySets;
    std::string parityShaderPath;
    std::string parityMathDirectory;

    void initializeParity(const std::string& path) {
        const bool rectangular=fftWidth!=fftHeight;
        const unsigned expectedABI=rectangular?64256u:256u;
        if(fftWidth==256||fftHeight==256){
            const auto properties=physicalDevice.getProperties2<vk::PhysicalDeviceProperties2,vk::PhysicalDeviceFloatControlsProperties>();
            const auto& controls=properties.get<vk::PhysicalDeviceFloatControlsProperties>();
            if(!controls.shaderDenormPreserveFloat32||!controls.shaderRoundingModeRTEFloat32)
                throw std::invalid_argument("FFT256 reference requires Float32 preserve/RTE controls");
        }
        const unsigned bytes=fftWidth*fftHeight*2*sizeof(float);
        createBuffer(bytes,vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal,parityScratch,parityScratchMemory);
        std::ifstream file(path,std::ios::binary|std::ios::ate);
        const auto size=file.tellg();
        if(size<=0 || size%4) throw std::runtime_error("Invalid reference FFT shader: "+path);
        std::vector<uint32_t> code(static_cast<size_t>(size)/4); file.seekg(0);
        if(!file.read(reinterpret_cast<char*>(code.data()),size)) throw std::runtime_error("Truncated reference FFT shader");
        if(fftWidth==256||fftHeight==256){
            if(code.size()<5||code[0]!=0x07230203u)throw std::invalid_argument("Invalid FFT256 SPIR-V header");
            uint32_t contract=0;bool matched=false;
            for(size_t i=5;i<code.size();){const auto count=code[i]>>16,op=code[i]&65535u;
                if(!count||i+count>code.size())throw std::invalid_argument("Invalid FFT256 SPIR-V instruction");
                if(op==71&&count==4&&code[i+2]==1&&code[i+3]==31)contract=code[i+1];i+=count;
            }
            for(size_t i=5;i<code.size();i+=code[i]>>16)
                if((code[i]&65535u)==50&&(code[i]>>16)==4&&code[i+2]==contract&&code[i+3]==expectedABI)matched=true;
            if(!contract||!matched)throw std::invalid_argument(rectangular?
                "Rectangular FFT reference requires shader ABI64256":"FFT256 reference requires shader ABI256");
        }
        auto module=IQM::GPU::VulkanRuntime::createShaderModule(device,code);
        paritySetLayout=IQM::GPU::VulkanRuntime::createDescLayout(device,
            {{vk::DescriptorType::eStorageBuffer,1},{vk::DescriptorType::eStorageBuffer,1}});
        parityPool=IQM::GPU::VulkanRuntime::createDescPool(device,4,{{vk::DescriptorType::eStorageBuffer,8}});
        parityLayout=IQM::GPU::VulkanRuntime::createPipelineLayout(device,{*paritySetLayout},
            IQM::GPU::VulkanRuntime::createPushConstantRange(16));
        parityPipeline=IQM::GPU::VulkanRuntime::createComputePipeline(device,module,parityLayout);
        const std::array<vk::DescriptorSetLayout,4> layouts{*paritySetLayout,*paritySetLayout,*paritySetLayout,*paritySetLayout};
        auto sets=vk::raii::DescriptorSets(device,vk::DescriptorSetAllocateInfo{
            .descriptorPool=*parityPool,.descriptorSetCount=4,.pSetLayouts=layouts.data()});
        for(auto& set:sets) paritySets.push_back(std::move(set));
        const std::array<vk::Buffer,4> sources{*workBuffer,*parityScratch,*outputBuffer,*parityScratch};
        const std::array<vk::Buffer,4> destinations{*parityScratch,*workBuffer,*parityScratch,*workBuffer};
        for(unsigned i=0;i<4;++i) {
            const std::vector<vk::DescriptorBufferInfo> src{{sources[i],0,bytes}},dst{{destinations[i],0,bytes}};
            device.updateDescriptorSets({IQM::GPU::VulkanRuntime::createWriteSet(*paritySets[i],0,src),
                IQM::GPU::VulkanRuntime::createWriteSet(*paritySets[i],1,dst)},{});
        }
        parityShaderPath=path;
    }

    void recordParity(vk::CommandBuffer cmd,bool inverse) const {
        cmd.bindPipeline(vk::PipelineBindPoint::eCompute,*parityPipeline);
        for(unsigned axis=0;axis<2;++axis) {
            const unsigned set=(inverse?2:0)+axis;
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute,*parityLayout,0,{*paritySets[set]},{});
            const bool rectangular=fftWidth!=fftHeight;
            const unsigned length=rectangular?(axis==0?fftHeight:fftWidth):fftWidth;
            const unsigned batches=rectangular?(axis==0?fftWidth:fftHeight):
                ((!inverse&&axis==1)?fftWidth/2+1:fftWidth);
            const std::array<uint32_t,4> push{length,batches,
                inverse?(axis==0?3u:4u):(axis==0?8u:9u),0};
            cmd.pushConstants(*parityLayout,vk::ShaderStageFlagBits::eCompute,0,16,push.data());
            cmd.dispatch((push[1]+31)/32,1,1);
            vk::MemoryBarrier barrier{.srcAccessMask=vk::AccessFlagBits::eShaderWrite,
                .dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eShaderWrite};
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                vk::PipelineStageFlagBits::eComputeShader,{}, {barrier},{},{});
        }
    }

void recordPrepare(const vk::raii::CommandBuffer& commandBuffer, FFTFilterMode mode, bool columnFirst) {

    vk::CommandBuffer cmd = *commandBuffer;

    cmd.bindPipeline(

        vk::PipelineBindPoint::eCompute,

        *preparePipeline);

    cmd.bindDescriptorSets(

        vk::PipelineBindPoint::eCompute,

        *preparePipelineLayout,

        0,

        {*prepareDescriptorSets[0]},

        {});

    std::array<uint32_t, 4> pushConstants{

        fftWidth,

        fftHeight, static_cast<uint32_t>(mode), columnFirst ? 1u : 0u

    };

    cmd.pushConstants(

        *preparePipelineLayout,

        vk::ShaderStageFlagBits::eCompute,

        0,

        static_cast<uint32_t>(sizeof(pushConstants)),

        pushConstants.data());

    cmd.dispatch(

        (fftWidth + 15) / 16,

        (fftHeight + 15) / 16,

        1);

}

void recordFinish(const vk::raii::CommandBuffer& commandBuffer, FFTFilterMode mode, bool columnFirst) {

    vk::CommandBuffer cmd = *commandBuffer;

    cmd.bindPipeline(

        vk::PipelineBindPoint::eCompute,

        *finishPipeline);

    cmd.bindDescriptorSets(

        vk::PipelineBindPoint::eCompute,

        *finishPipelineLayout,

        0,

        {*finishDescriptorSets[0]},

        {});

    std::array<uint32_t, 4> pushConstants{

        fftWidth,

        fftHeight, static_cast<uint32_t>(mode), columnFirst ? 1u : 0u

    };

    cmd.pushConstants(

        *finishPipelineLayout,

        vk::ShaderStageFlagBits::eCompute,

        0,

        static_cast<uint32_t>(sizeof(pushConstants)),

        pushConstants.data());

    // Output image is 64x64.

    const uint32_t outputWidth = fftWidth/2;

    const uint32_t outputHeight = fftHeight/2;

    cmd.dispatch(

        (outputWidth + 15) / 16,

        (outputHeight + 15) / 16,

        1);

}

    void createBuffer(

    vk::DeviceSize size,

    vk::BufferUsageFlags usage,

    vk::MemoryPropertyFlags properties,

    vk::raii::Buffer& buffer,

    vk::raii::DeviceMemory& memory

) {

    vk::BufferCreateInfo bufferInfo{

        .size = size,

        .usage = usage,

        .sharingMode = vk::SharingMode::eExclusive

    };

    vk::raii::Buffer newBuffer(device, bufferInfo);

    auto requirements = newBuffer.getMemoryRequirements();

    uint32_t memoryType = findMemoryType(

        physicalDevice,

        requirements.memoryTypeBits,

        properties

    );

    if (memoryType == UINT32_MAX) {

        throw std::runtime_error(

            "LocalAdaptFFT: suitable Vulkan memory type not found"

        );

    }

    vk::MemoryAllocateInfo allocInfo{

        .allocationSize = requirements.size,

        .memoryTypeIndex = memoryType

    };

    vk::raii::DeviceMemory newMemory(device, allocInfo);

    newBuffer.bindMemory(newMemory, 0);

    buffer = std::move(newBuffer);

    memory = std::move(newMemory);

}

    void initializeBuffers(uint32_t width, uint32_t height) {

    if (width == 0 || height == 0) {

        throw std::invalid_argument(

            "LocalAdaptFFT: FFT dimensions must be non-zero"

        );

    }

    const vk::DeviceSize pixelCount =

        static_cast<vk::DeviceSize>(width) * height;

    const vk::DeviceSize complexBufferSize =

        pixelCount * 2 * sizeof(float);

    const vk::DeviceSize realBufferSize =

        pixelCount * sizeof(float);

    createBuffer(

        complexBufferSize,

        vk::BufferUsageFlagBits::eStorageBuffer |

        vk::BufferUsageFlagBits::eTransferSrc |

        vk::BufferUsageFlagBits::eTransferDst,

        vk::MemoryPropertyFlagBits::eDeviceLocal,

        workBuffer,

        workMemory

    );

    createBuffer(

        realBufferSize,

        vk::BufferUsageFlagBits::eStorageBuffer |

        vk::BufferUsageFlagBits::eTransferDst,

        vk::MemoryPropertyFlagBits::eDeviceLocal,

        kernelBuffer,

        kernelMemory

    );

    createBuffer(

        complexBufferSize,

        vk::BufferUsageFlagBits::eStorageBuffer |

        vk::BufferUsageFlagBits::eTransferSrc |

        vk::BufferUsageFlagBits::eTransferDst,

        vk::MemoryPropertyFlagBits::eDeviceLocal,

        outputBuffer,

        outputMemory

    );

    fftWidth = width;

    fftHeight = height;

}

    const vk::raii::Device& device;

    const vk::raii::PhysicalDevice& physicalDevice;

    VkFFTApplication fftApp{};

    bool fftInitialized = false;
    bool fftUsesTwiddleLUT = false;

    // Stable Vulkan handles: VkFFT stores pointers to these values.

    VkDevice fftDeviceHandle = VK_NULL_HANDLE;

    VkPhysicalDevice fftPhysicalDeviceHandle = VK_NULL_HANDLE;

    VkQueue fftQueueHandle = VK_NULL_HANDLE;

    VkCommandPool fftCommandPoolHandle = VK_NULL_HANDLE;

    VkFence fftFenceHandle = VK_NULL_HANDLE;
    vk::raii::Fence fftUploadFence = nullptr;

    VkBuffer fftWorkBufferHandle = VK_NULL_HANDLE;

    uint64_t fftWorkBufferSize = 0;

    // FFT dimensions

    uint32_t fftWidth = 0;

    uint32_t fftHeight = 0;

    // Memory must be declared BEFORE its associated buffer.

    // C++ destroys members in reverse declaration order.

    // FFT working buffer (complex float2)

    vk::raii::DeviceMemory workMemory = nullptr;

    vk::raii::Buffer workBuffer = nullptr;

    // Gaussian frequency-domain filter (float)

    vk::raii::DeviceMemory kernelMemory = nullptr;

    vk::raii::Buffer kernelBuffer = nullptr;

    // Filtered complex spectrum (float2)

    vk::raii::DeviceMemory outputMemory = nullptr;

    vk::raii::Buffer outputBuffer = nullptr;

    // Host-visible staging buffer must outlive command-buffer execution.

    vk::raii::DeviceMemory kernelStagingMemory = nullptr;

    vk::raii::Buffer kernelStagingBuffer = nullptr;

    // Gaussian multiplication compute pipeline

    vk::raii::DescriptorPool descriptorPool = nullptr;

    vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;

    vk::raii::PipelineLayout pipelineLayout = nullptr;

    vk::raii::Pipeline computePipeline = nullptr;

    std::vector<vk::raii::DescriptorSet> descriptorSets;

    // FFT prepare: input image -> complex work buffer

    vk::raii::DescriptorSetLayout prepareDescriptorSetLayout = nullptr;

    vk::raii::PipelineLayout preparePipelineLayout = nullptr;

    vk::raii::Pipeline preparePipeline = nullptr;

    vk::raii::DescriptorPool prepareDescriptorPool = nullptr;

    std::vector<vk::raii::DescriptorSet> prepareDescriptorSets;

    // FFT finish: complex work buffer -> output image

    vk::raii::DescriptorSetLayout finishDescriptorSetLayout = nullptr;

    vk::raii::PipelineLayout finishPipelineLayout = nullptr;

    vk::raii::Pipeline finishPipeline = nullptr;

    vk::raii::DescriptorPool finishDescriptorPool = nullptr;

    std::vector<vk::raii::DescriptorSet> finishDescriptorSets;

    // Record a host-to-device upload of the Gaussian frequency response.

    // Call after initializeBuffers(), before the FFT multiplication dispatch.

    // The caller owns command-buffer submission and GPU synchronization.

    void recordGaussianKernelUpload(

        const vk::raii::CommandBuffer& commandBuffer,

        float ppd, FFTFilterMode mode

    ) {

        if (!std::isfinite(ppd) || ppd <= 0.0f) {

            throw std::invalid_argument("LocalAdaptFFT: ppd must be positive and finite");

        }

        if (fftWidth == 0 || fftHeight == 0 || *kernelBuffer == VK_NULL_HANDLE) {

            throw std::logic_error("LocalAdaptFFT: initializeBuffers must be called first");

        }

        const double sigma = std::pow(10.0, -0.781367) * ppd;
        auto kernel = generateGaussianKernel2D(sigma, fftWidth, fftHeight);
        if (mode == FFTFilterMode::OpticalMTF) {
            generate_mtf_filter(kernel.data(), fftWidth, fftHeight, ppd);
        }
        if(fftWidth!=fftHeight&&!parityShaderPath.empty()) {
            // Reference spectra use column-major storage. Rectangular radial
            // filters must be transposed explicitly before upload.
            const auto rowMajor=kernel;
            for(unsigned y=0;y<fftHeight;++y)for(unsigned x=0;x<fftWidth;++x)
                kernel[x*fftHeight+y]=rowMajor[y*fftWidth+x];
        }

        const vk::DeviceSize bytes =

            static_cast<vk::DeviceSize>(kernel.size()) * sizeof(float);

        // Do not replace a staging buffer referenced by an in-flight command.

        // Reuse this allocation for uploads of the same dimensions.

        if (*kernelStagingBuffer == VK_NULL_HANDLE) {

            createBuffer(

                bytes,

                vk::BufferUsageFlagBits::eTransferSrc,

                vk::MemoryPropertyFlagBits::eHostVisible |

                    vk::MemoryPropertyFlagBits::eHostCoherent,

                kernelStagingBuffer,

                kernelStagingMemory

            );

        }

        void* mapped = kernelStagingMemory.mapMemory(0, bytes, {});

        std::memcpy(mapped, kernel.data(), static_cast<size_t>(bytes));

        kernelStagingMemory.unmapMemory();

        const vk::BufferCopy copyRegion{0, 0, bytes};

        commandBuffer.copyBuffer(*kernelStagingBuffer, *kernelBuffer, {copyRegion});

        const vk::BufferMemoryBarrier barrier{

            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,

            .dstAccessMask = vk::AccessFlagBits::eShaderRead,

            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

            .buffer = *kernelBuffer,

            .offset = 0,

            .size = bytes

        };

        commandBuffer.pipelineBarrier(

            vk::PipelineStageFlagBits::eTransfer,

            vk::PipelineStageFlagBits::eComputeShader,

            {}, {}, {barrier}, {}

        );

    }

    void initializeMultiplyPipeline(const std::string& mathDirectory={}) {

    // Three storage buffers: FFT input, Gaussian kernel, filtered output.

    std::array<vk::DescriptorSetLayoutBinding, 3> bindings{{

        {0, vk::DescriptorType::eStorageBuffer, 1,

         vk::ShaderStageFlagBits::eCompute},

        {1, vk::DescriptorType::eStorageBuffer, 1,

         vk::ShaderStageFlagBits::eCompute},

        {2, vk::DescriptorType::eStorageBuffer, 1,

         vk::ShaderStageFlagBits::eCompute}

    }};

    vk::DescriptorSetLayoutCreateInfo layoutInfo{

        .bindingCount = static_cast<uint32_t>(bindings.size()),

        .pBindings = bindings.data()

    };

    descriptorSetLayout =

        vk::raii::DescriptorSetLayout(device, layoutInfo);

    vk::PushConstantRange pushRange{

        .stageFlags = vk::ShaderStageFlagBits::eCompute,

        .offset = 0,

        .size = 2 * sizeof(uint32_t)

    };

    vk::DescriptorSetLayout rawLayout = *descriptorSetLayout;

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{

        .setLayoutCount = 1,

        .pSetLayouts = &rawLayout,

        .pushConstantRangeCount = 1,

        .pPushConstantRanges = &pushRange

    };

    pipelineLayout =

        vk::raii::PipelineLayout(device, pipelineLayoutInfo);

    // Descriptor pool: one set containing three storage buffers.

    vk::DescriptorPoolSize poolSize{

        .type = vk::DescriptorType::eStorageBuffer,

        .descriptorCount = 3

    };

    vk::DescriptorPoolCreateInfo poolInfo{

        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 1,

        .poolSizeCount = 1,

        .pPoolSizes = &poolSize

    };

    descriptorPool = vk::raii::DescriptorPool(device, poolInfo);

    // Allocate one descriptor set.

    vk::DescriptorSetAllocateInfo allocInfo{

        .descriptorPool = *descriptorPool,

        .descriptorSetCount = 1,

        .pSetLayouts = &rawLayout

    };

    auto allocatedSets = vk::raii::DescriptorSets(device, allocInfo);

    descriptorSets.clear();

    for (auto& set : allocatedSets) {

        descriptorSets.push_back(std::move(set));

    }

    // Buffer sizes for the 128x128 FFT.

    const vk::DeviceSize pixelCount =

        static_cast<vk::DeviceSize>(fftWidth) * fftHeight;

    const vk::DeviceSize complexBytes =

        pixelCount * 2 * sizeof(float);

    const vk::DeviceSize realBytes =

        pixelCount * sizeof(float);

    std::array<vk::DescriptorBufferInfo, 3> bufferInfos{{

        {*workBuffer, 0, complexBytes},

        {*kernelBuffer, 0, realBytes},

        {*outputBuffer, 0, complexBytes}

    }};

    std::array<vk::WriteDescriptorSet, 3> writes{};

    for (uint32_t i = 0; i < 3; ++i) {

        writes[i].dstSet = *descriptorSets[0];

        writes[i].dstBinding = i;

        writes[i].descriptorCount = 1;

        writes[i].descriptorType = vk::DescriptorType::eStorageBuffer;

        writes[i].pBufferInfo = &bufferInfos[i];

    }

    device.updateDescriptorSets(writes, {});

    // Load compiled Gaussian multiplication shader.

    const std::string shaderPath = mathDirectory.empty()?"shaders/hdrvdp3/mtf_multiply.spv":mathDirectory+"/multiply.spv";

    std::ifstream shaderFile(shaderPath, std::ios::binary | std::ios::ate);

    if (!shaderFile) {

        throw std::runtime_error(

            "LocalAdaptFFT: cannot open shader: " + shaderPath);

    }

    const std::streamsize shaderSize = shaderFile.tellg();

    if (shaderSize <= 0 ||

        shaderSize % sizeof(uint32_t) != 0) {

        throw std::runtime_error(

            "LocalAdaptFFT: invalid SPIR-V shader size");

    }

    std::vector<uint32_t> shaderCode(

        static_cast<size_t>(shaderSize) / sizeof(uint32_t));

    shaderFile.seekg(0);

    shaderFile.read(

        reinterpret_cast<char*>(shaderCode.data()),

        shaderSize);

    if (!shaderFile) {

        throw std::runtime_error(

            "LocalAdaptFFT: failed to read shader");

    }

    vk::ShaderModuleCreateInfo shaderInfo{

        .codeSize = static_cast<size_t>(shaderSize),

        .pCode = shaderCode.data()

    };

    vk::raii::ShaderModule shaderModule(device, shaderInfo);

    vk::PipelineShaderStageCreateInfo stageInfo{

        .stage = vk::ShaderStageFlagBits::eCompute,

        .module = *shaderModule,

        .pName = "main"

    };

    vk::ComputePipelineCreateInfo pipelineInfo{

        .stage = stageInfo,

        .layout = *pipelineLayout

    };

    computePipeline = vk::raii::Pipeline(

        device,

        nullptr,

        pipelineInfo);

}

void initializeImagePipelines(const std::string& mathDirectory={}) {

    auto loadShader = [&](const std::string& path) {

        std::ifstream file(path, std::ios::binary | std::ios::ate);

        if (!file) {

            throw std::runtime_error(

                "LocalAdaptFFT: cannot open shader: " + path);

        }

        const std::streamsize size = file.tellg();

        if (size <= 0 || size % sizeof(uint32_t) != 0) {

            throw std::runtime_error(

                "LocalAdaptFFT: invalid SPIR-V size: " + path);

        }

        std::vector<uint32_t> code(

            static_cast<size_t>(size) / sizeof(uint32_t));

        file.seekg(0);

        file.read(reinterpret_cast<char*>(code.data()), size);

        if (!file) {

            throw std::runtime_error(

                "LocalAdaptFFT: failed to read shader: " + path);

        }

        vk::ShaderModuleCreateInfo info{

            .codeSize = static_cast<size_t>(size),

            .pCode = code.data()

        };

        return vk::raii::ShaderModule(device, info);

    };

    auto createImagePipeline = [&](

        const std::string& shaderPath,

        vk::raii::DescriptorSetLayout& setLayout,

        vk::raii::PipelineLayout& layout,

        vk::raii::Pipeline& pipeline,

        vk::raii::DescriptorPool& pool,

        std::vector<vk::raii::DescriptorSet>& sets

    ) {

        std::array<vk::DescriptorSetLayoutBinding, 2> bindings{{

            {0, vk::DescriptorType::eStorageImage, 1,

                vk::ShaderStageFlagBits::eCompute},

            {1, vk::DescriptorType::eStorageBuffer, 1,

                vk::ShaderStageFlagBits::eCompute}

        }};

        // Finish shader has the reverse descriptor types.

        if (shaderPath.find("finish") != std::string::npos) {

            bindings[0].descriptorType =

                vk::DescriptorType::eStorageBuffer;

            bindings[1].descriptorType =

                vk::DescriptorType::eStorageImage;

        }

        vk::DescriptorSetLayoutCreateInfo setInfo{

            .bindingCount = static_cast<uint32_t>(bindings.size()),

            .pBindings = bindings.data()

        };

        setLayout = vk::raii::DescriptorSetLayout(device, setInfo);

        vk::PushConstantRange pushRange{

            .stageFlags = vk::ShaderStageFlagBits::eCompute,

            .offset = 0,

            .size = 4 * sizeof(uint32_t)

        };

        vk::DescriptorSetLayout rawLayout = *setLayout;

        vk::PipelineLayoutCreateInfo layoutInfo{

            .setLayoutCount = 1,

            .pSetLayouts = &rawLayout,

            .pushConstantRangeCount = 1,

            .pPushConstantRanges = &pushRange

        };

        layout = vk::raii::PipelineLayout(device, layoutInfo);

        std::array<vk::DescriptorPoolSize, 2> poolSizes{{

            {vk::DescriptorType::eStorageImage, 1},

            {vk::DescriptorType::eStorageBuffer, 1}

        }};

        vk::DescriptorPoolCreateInfo poolInfo{

            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = 1,

            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),

            .pPoolSizes = poolSizes.data()

        };

        pool = vk::raii::DescriptorPool(device, poolInfo);

        vk::DescriptorSetAllocateInfo allocInfo{

            .descriptorPool = *pool,

            .descriptorSetCount = 1,

            .pSetLayouts = &rawLayout

        };

        auto allocatedSets = vk::raii::DescriptorSets(

            device, allocInfo);

        sets.clear();

        for (auto& set : allocatedSets) {

            sets.push_back(std::move(set));

        }

        auto shaderModule = loadShader(shaderPath);

        vk::PipelineShaderStageCreateInfo stageInfo{

            .stage = vk::ShaderStageFlagBits::eCompute,

            .module = *shaderModule,

            .pName = "main"

        };

        vk::ComputePipelineCreateInfo pipelineInfo{

            .stage = stageInfo,

            .layout = *layout

        };

        pipeline = vk::raii::Pipeline(

            device, nullptr, pipelineInfo);

    };

    createImagePipeline(

        mathDirectory.empty()?"shaders/hdrvdp3/hdrvdp3_fft_prepare.spv":mathDirectory+"/prepare.spv",

        prepareDescriptorSetLayout,

        preparePipelineLayout,

        preparePipeline,

        prepareDescriptorPool,

        prepareDescriptorSets

    );

    createImagePipeline(

        mathDirectory.empty()?"shaders/hdrvdp3/hdrvdp3_fft_finish.spv":mathDirectory+"/finish.spv",

        finishDescriptorSetLayout,

        finishPipelineLayout,

        finishPipeline,

        finishDescriptorPool,

        finishDescriptorSets

    );

}

void updateImageDescriptors(const LocalAdaptFFTInput& input) {

    if (!input.ivInput || !input.ivOutput) {

        throw std::invalid_argument(

            "LocalAdaptFFT: input or output image view is null");

    }

    if (prepareDescriptorSets.empty() ||

        finishDescriptorSets.empty()) {

        throw std::logic_error(

            "LocalAdaptFFT: image descriptor sets not initialized");

    }

    const vk::DeviceSize bufferSize =

        static_cast<vk::DeviceSize>(fftWidth) *

        fftHeight * 2 * sizeof(float);

    vk::DescriptorImageInfo inputImageInfo{

        .imageView = **input.ivInput,

        .imageLayout = vk::ImageLayout::eGeneral

    };

    vk::DescriptorImageInfo outputImageInfo{

        .imageView = **input.ivOutput,

        .imageLayout = vk::ImageLayout::eGeneral

    };

    vk::DescriptorBufferInfo prepareBufferInfo{

        .buffer = *workBuffer,

        .offset = 0,

        .range = bufferSize

    };

    vk::DescriptorBufferInfo finishBufferInfo{

        .buffer = *workBuffer,

        .offset = 0,

        .range = bufferSize

    };

    std::array<vk::WriteDescriptorSet, 4> writes{};

    writes[0].dstSet = *prepareDescriptorSets[0];

    writes[0].dstBinding = 0;

    writes[0].descriptorCount = 1;

    writes[0].descriptorType = vk::DescriptorType::eStorageImage;

    writes[0].pImageInfo = &inputImageInfo;

    writes[1].dstSet = *prepareDescriptorSets[0];

    writes[1].dstBinding = 1;

    writes[1].descriptorCount = 1;

    writes[1].descriptorType = vk::DescriptorType::eStorageBuffer;

    writes[1].pBufferInfo = &prepareBufferInfo;

    writes[2].dstSet = *finishDescriptorSets[0];

    writes[2].dstBinding = 0;

    writes[2].descriptorCount = 1;

    writes[2].descriptorType = vk::DescriptorType::eStorageBuffer;

    writes[2].pBufferInfo = &finishBufferInfo;

    writes[3].dstSet = *finishDescriptorSets[0];

    writes[3].dstBinding = 1;

    writes[3].descriptorCount = 1;

    writes[3].descriptorType = vk::DescriptorType::eStorageImage;

    writes[3].pImageInfo = &outputImageInfo;

    device.updateDescriptorSets(writes, {});

}

    void recordGaussianMultiply(

    const vk::raii::CommandBuffer& commandBuffer

) {

    if (*computePipeline == VK_NULL_HANDLE ||

        descriptorSets.empty()) {

        throw std::logic_error(

            "LocalAdaptFFT: multiply pipeline not initialized");

    }

    vk::CommandBuffer cmd = *commandBuffer;

    cmd.bindPipeline(

        vk::PipelineBindPoint::eCompute,

        *computePipeline);

    cmd.bindDescriptorSets(

        vk::PipelineBindPoint::eCompute,

        *pipelineLayout,

        0,

        {*descriptorSets[0]},

        {});

    std::array<uint32_t, 2> pushConstants{

        fftWidth,

        fftHeight

    };

    cmd.pushConstants(

        *pipelineLayout,

        vk::ShaderStageFlagBits::eCompute,

        0,

        static_cast<uint32_t>(sizeof(pushConstants)),

        pushConstants.data());

    cmd.dispatch(

        (fftWidth + 15) / 16,

        (fftHeight + 15) / 16,

        1);

}

    // Initialize the VkFFT plan after allocating the working buffers.

    // Do not reallocate workBuffer or destroy the Vulkan context while this

    // plan is alive. The caller must synchronize any in-flight GPU work.

    void initializeFFT(

        const vk::raii::Queue& queue,

        const vk::raii::CommandPool& commandPool, bool useTwiddleLUT

    ) {

        if (fftInitialized) {

            throw std::logic_error("LocalAdaptFFT: VkFFT already initialized");

        }

        if (fftWidth == 0 || fftHeight == 0 ||

            *workBuffer == VK_NULL_HANDLE) {

            throw std::logic_error("LocalAdaptFFT: initializeBuffers must run first");

        }

        fftDeviceHandle = static_cast<VkDevice>(*device);

        fftPhysicalDeviceHandle = static_cast<VkPhysicalDevice>(*physicalDevice);

        fftQueueHandle = static_cast<VkQueue>(*queue);

        fftCommandPoolHandle = static_cast<VkCommandPool>(*commandPool);

        // VkFFT's LUT upload submits, waits and resets this fence during plan
        // initialization. A nonnull pointer to VK_NULL_HANDLE is not sufficient.
        fftUploadFence = vk::raii::Fence(device, vk::FenceCreateInfo{});
        fftFenceHandle = static_cast<VkFence>(*fftUploadFence);

        fftWorkBufferHandle = static_cast<VkBuffer>(*workBuffer);

        fftWorkBufferSize = static_cast<uint64_t>(fftWidth) * fftHeight *

                            2u * sizeof(float);

        VkFFTConfiguration config{};

        config.FFTdim = 2;

        config.size[0] = fftWidth;

        config.size[1] = fftHeight;

        config.numberBatches = 1;

        config.bufferNum = 1;

        config.buffer = &fftWorkBufferHandle;

        config.bufferSize = &fftWorkBufferSize;

        config.physicalDevice = &fftPhysicalDeviceHandle;

        config.device = &fftDeviceHandle;

        config.queue = &fftQueueHandle;

        config.commandPool = &fftCommandPoolHandle;

        config.fence = &fftFenceHandle;

        config.isCompilerInitialized = 0;

        config.normalize = 1;

        config.performR2C = 0;  // Complex-to-complex, float2 per pixel.

        config.doublePrecision = 0;

        config.reorderFourStep = 1;

        config.useLUT = useTwiddleLUT ? 1 : -1;

        config.aimThreads = 128;

        config.makeForwardPlanOnly = 0;

        config.makeInversePlanOnly = 0;

        const VkFFTResult result = initializeVkFFT(&fftApp, config);

        if (result != VKFFT_SUCCESS) {

            // VkFFT may have partially allocated resources on failure.

            // Do not mark the plan initialized; investigate the returned code.

            throw std::runtime_error(

                "LocalAdaptFFT: initializeVkFFT failed with code " +

                std::to_string(static_cast<int>(result)));

        }

        fftInitialized = true;
        fftUsesTwiddleLUT = useTwiddleLUT;

    }

    explicit Impl(

        const vk::raii::Device& dev,

        const vk::raii::PhysicalDevice& physDev

    )

        : device(dev),

          physicalDevice(physDev)

    {

    }

    ~Impl() {

        // The caller must ensure GPU work has completed before destruction.

        if (fftInitialized) {

            deleteVkFFT(&fftApp);

            fftInitialized = false;

        }

        // Release image-pipeline descriptor sets before their pools.

        prepareDescriptorSets.clear();

        finishDescriptorSets.clear();

        preparePipeline = nullptr;

        finishPipeline = nullptr;

        preparePipelineLayout = nullptr;

        finishPipelineLayout = nullptr;

        prepareDescriptorSetLayout = nullptr;

        finishDescriptorSetLayout = nullptr;

        prepareDescriptorPool = nullptr;

        finishDescriptorPool = nullptr;

        // Descriptor sets must be released before their pool.

        descriptorSets.clear();

        computePipeline = nullptr;

        pipelineLayout = nullptr;

        descriptorSetLayout = nullptr;

        descriptorPool = nullptr;

        kernelStagingBuffer = nullptr;

        kernelStagingMemory = nullptr;

        // Destroy buffers before freeing the memory bound to them.

        outputBuffer = nullptr;

        outputMemory = nullptr;

        kernelBuffer = nullptr;

        kernelMemory = nullptr;

        workBuffer = nullptr;

        workMemory = nullptr;

    }

};

LocalAdaptFFT::LocalAdaptFFT(

    const vk::raii::Device& device,

    const vk::raii::PhysicalDevice& physicalDevice

)

    : impl(std::make_unique<Impl>(device, physicalDevice))

{

}

LocalAdaptFFT::~LocalAdaptFFT() = default;

// Record the complete FFT-based local adaptation pass into the caller's

// already-recording command buffer. The caller owns submission and synchronization.

void LocalAdaptFFT::compute(const LocalAdaptFFTInput& input) {
    const bool parity=!input.referenceShaderPath.empty();
    const bool rectangular=parity&&input.ppd==30.0f&&!input.referenceMathDirectory.empty()&&
        ((input.width==128&&input.height==64)||(input.width==64&&input.height==128));
    const bool supported_size=input.width==32||input.width==64||
        (parity&&input.width==128&&input.ppd==30.0f);
    if(!rectangular&&(input.width!=input.height || !supported_size))
        throw std::invalid_argument("Unvalidated FFT filter input dimensions");
    if(input.width==32&&!parity)
        throw std::invalid_argument("32x32 FFT filter currently requires the reference backend");
    if(impl->fftWidth && (impl->fftWidth!=2*input.width||impl->fftHeight!=2*input.height))
        throw std::invalid_argument("FFT filter dimensions cannot change after initialization");
    if(parity && (!input.columnFirst || input.useTwiddleLUT))
        throw std::invalid_argument("Reference FFT requires column axis order and native twiddle option");
    if(!parity && !input.referenceMathDirectory.empty())
        throw std::invalid_argument("Reference math probe requires reference FFT backend");

    if (!input.cmdBuf || !input.cmdPool || !input.queue ||

        !input.ivInput || !input.ivOutput) {

        throw std::invalid_argument("LocalAdaptFFT: missing Vulkan input resource");

    }

    if (!std::isfinite(input.ppd) || input.ppd <= 0.0f) {

        throw std::invalid_argument("LocalAdaptFFT: ppd must be positive and finite");

    }

    if (input.mode != FFTFilterMode::LocalAdaptation && input.mode != FFTFilterMode::OpticalMTF)
        throw std::invalid_argument("Unknown FFT filter mode");
    if (input.mode == FFTFilterMode::LocalAdaptation &&
        std::pow(10.0, -0.781367) * input.ppd < 4.3)
        throw std::invalid_argument("LocalAdaptFFT requires sigma >= 4.3; reference uses spatial filtering below it");

    // The first integration test uses a 64x64 R32_SFLOAT input/output

    // and a 128x128 complex FFT. Both images must already be in GENERAL.

    if (impl->fftInitialized && impl->fftUsesTwiddleLUT != input.useTwiddleLUT)
        throw std::invalid_argument("FFT twiddle mode cannot change after plan initialization");
    if (impl->fftWidth && (parity!=!impl->parityShaderPath.empty() ||
        (parity && (impl->parityShaderPath!=input.referenceShaderPath || impl->parityMathDirectory!=input.referenceMathDirectory))))
        throw std::invalid_argument("FFT backend cannot change after initialization");
    if (!impl->fftWidth) {

        impl->initializeBuffers(2*input.width, 2*input.height);

        impl->initializeMultiplyPipeline(input.referenceMathDirectory);

        impl->initializeImagePipelines(input.referenceMathDirectory);
        impl->parityMathDirectory=input.referenceMathDirectory;

        if(parity) impl->initializeParity(input.referenceShaderPath);
        else impl->initializeFFT(*input.queue, *input.cmdPool, input.useTwiddleLUT);

    }

    // Descriptor updates and staging-buffer writes require the previous use

    // of this LocalAdaptFFT instance to have finished on the GPU.

    impl->updateImageDescriptors(input);

    const vk::raii::CommandBuffer& commandBuffer = *input.cmdBuf;

    vk::CommandBuffer cmd = *input.cmdBuf;

    const vk::DeviceSize complexBufferSize =

        static_cast<vk::DeviceSize>(impl->fftWidth) * impl->fftHeight *

        2 * sizeof(float);

    auto bufferBarrier = [&](vk::Buffer buffer,

                             vk::AccessFlags srcAccess,

                             vk::AccessFlags dstAccess,

                             vk::PipelineStageFlags srcStage,

                             vk::PipelineStageFlags dstStage) {

        vk::BufferMemoryBarrier barrier{

            .srcAccessMask = srcAccess,

            .dstAccessMask = dstAccess,

            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

            .buffer = buffer,

            .offset = 0,

            .size = complexBufferSize

        };

        cmd.pipelineBarrier(srcStage, dstStage, {}, {}, {barrier}, {});

    };

    auto snapshot = [&](vk::Buffer source, unsigned stage) {
        if (!input.traceBuffer) return;
        bufferBarrier(source, vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eTransferRead,
            vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eTransfer);
        cmd.copyBuffer(source, **input.traceBuffer,
            {vk::BufferCopy{0, complexBufferSize * stage, complexBufferSize}});
        bufferBarrier(source, vk::AccessFlagBits::eTransferRead,
            vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
            vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader);
    };

    // Kernel upload includes its own transfer-write -> compute-read barrier.

    impl->recordGaussianKernelUpload(commandBuffer, input.ppd, input.mode);

    // Image -> padded log-domain complex work buffer.

    impl->recordPrepare(commandBuffer, input.mode, input.columnFirst);
    snapshot(*impl->workBuffer, 0);

    bufferBarrier(*impl->workBuffer,

                  vk::AccessFlagBits::eShaderWrite,

                  vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,

                  vk::PipelineStageFlagBits::eComputeShader,

                  vk::PipelineStageFlagBits::eComputeShader);

    VkCommandBuffer rawCmd = static_cast<VkCommandBuffer>(*commandBuffer);

    VkBuffer rawWorkBuffer = static_cast<VkBuffer>(*impl->workBuffer);

    VkFFTLaunchParams launchParams{};

    launchParams.commandBuffer = &rawCmd;

    launchParams.buffer = &rawWorkBuffer;

    VkFFTResult result = VKFFT_SUCCESS;
    if(parity) impl->recordParity(cmd,false);
    else result=VkFFTAppend(&impl->fftApp, -1, &launchParams);

    if (result != VKFFT_SUCCESS) {

        throw std::runtime_error("LocalAdaptFFT: forward FFT failed: " +

                                 std::to_string(static_cast<int>(result)));

    }

    bufferBarrier(*impl->workBuffer,

                  vk::AccessFlagBits::eShaderWrite,

                  vk::AccessFlagBits::eShaderRead,

                  vk::PipelineStageFlagBits::eComputeShader,

                  vk::PipelineStageFlagBits::eComputeShader);

    snapshot(*impl->workBuffer, 1);
    // Multiply FFT coefficients by the selected frequency response.

    impl->recordGaussianMultiply(commandBuffer);
    snapshot(*impl->outputBuffer, 2);

    if(parity) {
        bufferBarrier(*impl->outputBuffer,vk::AccessFlagBits::eShaderWrite,vk::AccessFlagBits::eShaderRead,
            vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader);
        impl->recordParity(cmd,true);
    } else {
    bufferBarrier(*impl->outputBuffer,

                  vk::AccessFlagBits::eShaderWrite,

                  vk::AccessFlagBits::eTransferRead,

                  vk::PipelineStageFlagBits::eComputeShader,

                  vk::PipelineStageFlagBits::eTransfer);

    // The VkFFT inverse plan operates in-place on workBuffer.

    vk::BufferCopy copyRegion{

        .srcOffset = 0,

        .dstOffset = 0,

        .size = complexBufferSize

    };

    if (input.columnFirst) {
        // VkFFT's inverse plan reverses its forward axis traversal. Restore
        // row-major storage here so the inverse also processes logical columns
        // first. Transfer copies introduce no floating-point operations.
        std::vector<vk::BufferCopy> transpose;
        transpose.reserve(static_cast<std::size_t>(impl->fftWidth) * impl->fftHeight);
        for (uint32_t y = 0; y < impl->fftHeight; ++y) {
            for (uint32_t x = 0; x < impl->fftWidth; ++x) {
                transpose.push_back(vk::BufferCopy{
                    (static_cast<vk::DeviceSize>(y) * impl->fftWidth + x) * 2 * sizeof(float),
                    (static_cast<vk::DeviceSize>(x) * impl->fftHeight + y) * 2 * sizeof(float),
                    2 * sizeof(float)});
            }
        }
        cmd.copyBuffer(*impl->outputBuffer, *impl->workBuffer, transpose);
    } else {
        cmd.copyBuffer(*impl->outputBuffer, *impl->workBuffer, {copyRegion});
    }

    bufferBarrier(*impl->workBuffer,

                  vk::AccessFlagBits::eTransferWrite,

                  vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,

                  vk::PipelineStageFlagBits::eTransfer,

                  vk::PipelineStageFlagBits::eComputeShader);

    result = VkFFTAppend(&impl->fftApp, 1, &launchParams);

    if (result != VKFFT_SUCCESS) {

        throw std::runtime_error("LocalAdaptFFT: inverse FFT failed: " +

                                 std::to_string(static_cast<int>(result)));

    }

    bufferBarrier(*impl->workBuffer,

                  vk::AccessFlagBits::eShaderWrite,

                  vk::AccessFlagBits::eShaderRead,

                  vk::PipelineStageFlagBits::eComputeShader,

                  vk::PipelineStageFlagBits::eComputeShader);

    }
    snapshot(*impl->workBuffer, 3);
    if (input.traceBuffer) {
        vk::MemoryBarrier host{.srcAccessMask=vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask=vk::AccessFlagBits::eHostRead};
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eHost, {}, {host}, {}, {});
    }
    // Top-left 64x64 crop, then exp for adaptation or clamp for MTF.

    impl->recordFinish(commandBuffer, input.mode, parity);

    // Make output image writes visible to subsequent compute or transfer reads.

    vk::MemoryBarrier outputBarrier{

        .srcAccessMask = vk::AccessFlagBits::eShaderWrite,

        .dstAccessMask = vk::AccessFlagBits::eShaderRead |

                         vk::AccessFlagBits::eTransferRead

    };

    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,

                        vk::PipelineStageFlagBits::eComputeShader |

                            vk::PipelineStageFlagBits::eTransfer,

                        {}, {outputBarrier}, {}, {});

}

} // namespace HDRVDP3

} // namespace IQM
