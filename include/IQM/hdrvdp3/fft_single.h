#pragma once
#include <functional>
#include <span>
#include <vector>
namespace IQM::HDRVDP3 {
// CPU/GPU providers must preserve Single image/complex operations and cast each
// Double filter coefficient to Single before the complex-real product.
// Input/filter and cropped output use column-major layout.
// Row-major -> column-major doubled postpadding, explicit optional GPU provider.
struct SinglePaddingBackend {
    std::function<std::vector<float>(std::span<const float>,unsigned,unsigned,bool)> pad;
};
struct FFTBackendSingle {
    std::function<std::vector<float>(std::span<const float>,std::span<const double>,unsigned,unsigned,unsigned,unsigned)> convolve;
    // Real column-major input -> full interleaved complex column-major output.
    std::function<std::vector<float>(std::span<const float>,unsigned,unsigned)> forward;
};
}
