#pragma once
#include <IQM/hdrvdp3/srgb_quality96_gpu_double.h>
#include <filesystem>
namespace IQM::HDRVDP3 {
struct SRGBQuality96RuntimeDouble {
    SRGBQuality96GPUDouble::Shaders shaders;
    std::vector<uint32_t> projection,mutual,neighbor,pyramid,comparison,jnd,csf,quality,logarithm;
    XYZDisplaySamples xyz;
    RGBSpectralSamples spectral;
};
// Separate fixed Double96 profile; verifies all13(v1)/14(v2)/15(v3)/16(v4)/17(v5)/18(v6)/19(v7)/20(v8)/21(v9)/22(v10) payload digests and confined
// paths before loading model samples. Digests provide integrity,not signatures.
SRGBQuality96RuntimeDouble load_srgb_quality96_runtime_double(
    const std::filesystem::path& directory,const std::string& display);
}
