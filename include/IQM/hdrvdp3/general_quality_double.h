#pragma once
#include <IQM/hdrvdp3/srgb_display.h>
#include <IQM/hdrvdp3/sp0_double.h>
#include <IQM/hdrvdp3/quality_double.h>
#include <functional>
namespace IQM::HDRVDP3 {
// Experimental CPU orchestration, separate from the accepted fixed GPU runtime.
// Double original input; no implicit precision conversion or reference fallback.
enum class GeneralEncodingDouble {NativeRGB,SRGBDisplay,BT709,BT2020,XYZ};
struct GeneralQualityConfigDouble {
 unsigned width=0,height=0;
 double ppd=30,age=24;
 GeneralEncodingDouble encoding=GeneralEncodingDouble::NativeRGB;
};
struct GeneralFFTBackendDouble {
 // Column-major padded image/filter -> column-major cropped real image.
 std::function<std::vector<double>(std::span<const double>,std::span<const double>,unsigned,unsigned,unsigned,unsigned)> convolve;
 // Column-major real image -> column-major interleaved full complex FFT.
 std::function<std::vector<double>(std::span<const double>,unsigned,unsigned)> forward;
};
struct GeneralSideTraceDouble {
 std::vector<double> rgb,optical,adaptation,response;
 SP0PyramidDouble pyramid;
};
struct GeneralQualityTraceDouble {
 GeneralSideTraceDouble reference,test;
 std::vector<double> mean_log;
 std::vector<QualityBandDouble> quality;
};
struct GeneralQualityResultDouble {
 QualityScoresDouble scores;
 std::vector<double> band_errors;
 double base_frequency=0;
};
// Initial domain: both axes >=16, ppd26..256 (FFT adaptation branch), age0..90,
// default quality/sp0/symmetric surround. Input and trace planes are row-major.
GeneralQualityResultDouble evaluate_general_quality_double(
 std::span<const double> reference,std::span<const double> test,
 const GeneralQualityConfigDouble&,const XYZDisplaySamples&,const RGBSpectralSamples&,
 const GeneralFFTBackendDouble&,GeneralQualityTraceDouble* trace=nullptr);
}
