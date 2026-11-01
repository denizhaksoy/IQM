#pragma once
#include <span>
#include <vector>
namespace IQM::HDRVDP3 {
struct SP0BandSingle {unsigned width=0,height=0;std::vector<float> values;};
struct SP0PyramidSingle {std::vector<SP0BandSingle> bands;float base_mean=0;};
SP0PyramidSingle build_sp0_pyramid_single(std::span<const float> response,
    unsigned width,unsigned height,double ppd);
}
