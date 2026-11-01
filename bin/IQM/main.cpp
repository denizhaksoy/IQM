/*
 * Image Quality Metrics
 * Petr Volf - 2025
 */

#include <iostream>
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "args.h"
#include "file_matcher.h"
#include "vulkan_instance.h"

#if COMPILE_SSIM
#include "../shared/wrappers/ssim.h"
#endif

#if COMPILE_SVD
#include "../shared/wrappers/svd.h"
#endif

#if COMPILE_FSIM
#include "../shared/wrappers/fsim.h"
#endif

#if COMPILE_FLIP
#include "../shared/wrappers/flip.h"
#endif

#if COMPILE_PSNR
#include "../shared/wrappers/psnr.h"
#endif

#if COMPILE_LPIPS
#include "../shared/wrappers/lpips.h"
#endif

#if COMPILE_HDRVDP3
#include "../shared/wrappers/hdrvdp3.h"
#include "srgb_quality.h"
#endif

void printHelp() {
    std::cout << "IQM - Application for computing image quality metrics.\n"
    << "Usage: IQM --method METHOD --input INPUT --ref REF [--output OUTPUT]\n\n"
    << "Arguments:\n"
    << "    --method <METHOD> : selects method to compute, one of SSIM, FSIM, FLIP, PSNR, LPIPS, HDRVDP3, LOCAL_ADAPT\n"
    << "    --input <INPUT>   : path to tested image\n"
    << "    --ref <REF>       : path to reference image\n"
    << "    --output <OUTPUT> : path to output image, optional\n\n"
    << "    -v, --verbose     : enables more detailed output\n"
    << "    -c, --colorize    : colorize final output\n"
    << "    --enable-validation : enable Vulkan validation layers\n"
    << "    -h, --help        : prints help\n\n"
    << "Method specific arguments:\n"
    << "HDRVDP3 luminance quality (positive absolute-luminance PFM pair):\n"
    << "    --input <TEST.pfm> --ref <REFERENCE.pfm> --hdrvdp3-runtime <DIR>\n"
    << "    --hdrvdp3-ppd <30|60>       : defaults to 30\n"
    << "    --hdrvdp3-age <24|60|60.001>: defaults to 24\n"
    << "    --hdrvdp3-spectral-data <DIR>: D65 spectral tables; required for nondefault age\n"
    << "    --hdrvdp3-export <FRESH_DIR>: optional diagnostic output directory\n"
    << "    Sizes: square32/64; square128 and64x128/128x64 only at30ppd/age24.\n"
    << "    80x96/96x80: v1 pathway runtime,30ppd/age24,fixed D65; no spectral overrides.\n"
    << "    96x96: v2 square96 pathway runtime,same viewing profile.\n"
    << "    80x80: v3 square80 pathway runtime,same viewing profile.\n"
    << "    128x128: v4 square128 pathway runtime,map14,same viewing profile.\n"
    << "HDRVDP3 RGB quality (96x96 color PF,linear native display primaries):\n"
    << "    --hdrvdp3-encoding rgb-native --hdrvdp3-runtime <RGB_DIR>\n"
    << "    --hdrvdp3-display <led-lcd-srgb|led-lcd-wcg|oled> --hdrvdp3-spectral-data <DIR>\n"
    << "    Requires --input/--ref color PF;30ppd/age24;no gamma or color conversion.\n"
    << "    Linear BT.709: --hdrvdp3-encoding rgb-bt.709 with a distinct709 runtime.\n"
    << "    No sRGB EOTF;709 is converted to display primaries with native gamut clamp.\n"
    << "    Also --hdrvdp3-encoding rgb-bt.2020|xyz with its matching linear-color runtime.\n"
    << "    Encoded sRGB: --hdrvdp3-encoding srgb-display --hdrvdp3-runtime <DOUBLE96_DIR>\n"
    << "    Requires96x96 colorPF in[0,1],30ppd/age24;model data is inside runtime.\n"
    << "    --hdrvdp3-device <SUBSTRING>: optional Float64 GPU selection for sRGB.\n"
    << "PSNR:\n"
    << "    --psnr-variant <VAR> : One of `rgb`, `luma` or `yuv`\n"
    << "FLIP:\n"
    << "    --flip-width <WIDTH>       : Width of display in meters\n"
    << "    --flip-res <RES>           : Resolution of display in pixels\n"
    << "    --flip-distance <DISTANCE> : Distance to display in meters\n"
    << std::endl;
}

int main(const int argc, const char **argv) {
    std::optional<IQM::Bin::Args> args;

    try {
        args = IQM::Bin::Args(argc, argv);
    } catch (std::exception& e) {
        std::cout << "Error parsing arguments: " << e.what() << std::endl;
        printHelp();
        return -1;
    }

    if (args->printHelp) {
        printHelp();
        return 0;
    }

    if (args->verbose) {
        std::cout << "Selected method: " << IQM::method_name(args->method) << std::endl;
    }
    std::cout << "[DEBUG] Method parsed: " << static_cast<int>(args->method) << std::endl;

#if COMPILE_HDRVDP3
    if(args->hdrvdp3Quality&&args->hdrvdp3Quality->encoding=="srgb-display"){
        try{return IQM::Bin::run_srgb_quality96_cli(*args);}
        catch(const std::exception& e){std::cerr<<e.what()<<'\n';return -1;}
    }
#endif
    IQM::Bin::FileMatcher matcher;
    const auto matches = matcher.match(args.value());

    const IQM::Bin::VulkanInstance vulkan(args->enableValidation);

    if (args->verbose) {
        std::cout << "Selected device: "<< vulkan.selectedDevice << std::endl;
    }

    try {
        switch (args->method) {
            case IQM::Method::SSIM:
#ifdef COMPILE_SSIM
                IQM::Bin::ssim_run(args.value(), vulkan, matches);
#else
                throw std::runtime_error("SSIM support is not compiled");
#endif
                break;
            case IQM::Method::CW_SSIM_CPU:
                throw std::runtime_error("CW-SSIM is not implemented");
            case IQM::Method::SVD:
#ifdef COMPILE_SVD
                IQM::Bin::svd_run(args.value(), vulkan, matches);
#else
                throw std::runtime_error("M-SVD support is not compiled");
#endif
                break;
            case IQM::Method::FSIM:
#ifdef COMPILE_FSIM
                fsim_run(args.value(), vulkan, matches);
#else
                throw std::runtime_error("FSIM support is not compiled");
#endif
                break;
            case IQM::Method::FLIP:
#ifdef COMPILE_FLIP
                flip_run(args.value(), vulkan, matches);
#else
                throw std::runtime_error("FLIP support is not compiled");
#endif
                break;
            case IQM::Method::PSNR:
#ifdef COMPILE_PSNR
                psnr_run(args.value(), vulkan, matches);
#else
                throw std::runtime_error("PSNR support is not compiled");
#endif
            break;
            case IQM::Method::LPIPS:
#ifdef COMPILE_LPIPS
                lpips_run(args.value(), vulkan, matches);
#else
                throw std::runtime_error("LPIPS support is not compiled");
#endif
            break;
            case IQM::Method::HDRVDP3:
#ifdef COMPILE_HDRVDP3
                hdrvdp3_test_run(args.value(), vulkan);
#else
                throw std::runtime_error("HDRVDP3 support is not compiled");
#endif
            break;
            case IQM::Method::LOCAL_ADAPT:
#ifdef COMPILE_HDRVDP3
                local_adapt_test_run(args.value(), vulkan);
#else
                throw std::runtime_error("HDRVDP3 support is not compiled");
#endif
            break;
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}
