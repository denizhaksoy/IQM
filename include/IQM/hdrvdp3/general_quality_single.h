#pragma once
#include <IQM/hdrvdp3/sp0_single.h>
#include <IQM/hdrvdp3/fft_single.h>
#include <IQM/hdrvdp3/quality.h>
#include <IQM/hdrvdp3/spectral.h>
#include <IQM/hdrvdp3/projection_single_gpu.h>
namespace IQM::HDRVDP3 {
// Explicit stage inputs permit numerical isolation without a reference fallback.
struct QualitySideSingle {
    std::vector<float> native_rgb,adaptation;
    SP0PyramidSingle pyramid;
};
struct GeneralQualityResultSingle {
    QualityScores scores;
    double base_frequency=0;
    std::vector<float> band_errors;
};
// Optional explicit matrix provider; FFT and remaining operations stay separate.
struct SingleMatrixBackend {
    std::function<std::vector<float>(std::span<const float>,std::span<const float>,SingleProjectionKind)> project;
};
// Default quality/sp0 at age24, OLED spectral front-end, ppd26..256.
// Callers must supply independently computed stages to claim end-to-end parity.
GeneralQualityResultSingle evaluate_quality_single(const QualitySideSingle& reference,
    const QualitySideSingle& test,unsigned width,unsigned height,double ppd,
    const FFTBackendSingle& backend);
// Full independent front-end for calibrated linear BT2020 Single RGB.
// Initial profile is quality/sp0, age24; caller supplies OLED model samples.
QualitySideSingle prepare_quality_side_single(std::span<const float> linear_bt2020,
    unsigned width,unsigned height,double ppd,const XYZDisplaySamples& xyz,
    const RGBSpectralSamples& spectral,const FFTBackendSingle& backend,
    const SingleMatrixBackend* matrices=nullptr);
GeneralQualityResultSingle evaluate_general_quality_single(
    std::span<const float> reference,std::span<const float> test,
    unsigned width,unsigned height,double ppd,const XYZDisplaySamples& xyz,
    const RGBSpectralSamples& spectral,const FFTBackendSingle& backend,
    const SingleMatrixBackend* matrices=nullptr);
}
