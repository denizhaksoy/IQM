#pragma once
#include <IQM/hdrvdp3/luminance.h>
#include <IQM/hdrvdp3/quality.h>
#include <IQM/hdrvdp3/spectral.h>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace IQM { class VulkanInstance; }
namespace IQM::HDRVDP3 {
struct FFTExecutionConfig {
    bool columnFirst = true;
    bool useTwiddleLUT = false;
    std::string referenceShaderPath;
    std::string referenceMathDirectory;
};
struct EvaluationOptions {
    double pixelsPerDegree = 30;
    double observerAge = 24;
    FFTExecutionConfig fft;
    std::filesystem::path mapShader;
    std::filesystem::path baseFFTShader;
    // Mutually exclusive; omission uses the historical age24 L/M/R constants.
    std::filesystem::path spectralDataDirectory;
    std::optional<SpectralSamples> spectralSamples;
};
// This first parity API requires a new diagnostic directory. Tags are exported
// provenance only; they never select or transform input pixels.
struct EvaluationTrace {
    std::filesystem::path directory;
    std::string inputSource = "luminance_image";
    std::string pattern = "input";
    std::string transform = "input";
};
struct EvaluationResult {
    QualityScores scores;
    std::vector<float> bandErrors;
};
// Synchronous quality path: positive finite top-down row-major cd/m^2 inputs,
// equal square32/64 dimensions, ppd30/60, ages24/60/60.001. Explicit native
// reference FFT and Vulkan map/base FFT assets are required. No environment
// reads, synthetic generation, implicit IO conversion or automatic resizing.
// Exceptions propagate to the caller. VulkanInstance must be used exclusively
// for this call because its command buffers are reused synchronously.
EvaluationResult evaluate_quality(const IQM::VulkanInstance& instance,
    const LuminanceImage& reference, const LuminanceImage& test,
    const EvaluationOptions& options, const EvaluationTrace& trace);
}
