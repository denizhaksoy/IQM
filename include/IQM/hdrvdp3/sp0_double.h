#pragma once
#include <span>
#include <vector>
namespace IQM::HDRVDP3 {
struct SP0FiltersDouble {std::vector<double> lo0,high,lo,orientation;};
SP0FiltersDouble create_sp0_model_double();
struct SP0BandDouble {unsigned width=0,height=0;std::vector<double> values;};
struct SP0PyramidDouble {std::vector<SP0BandDouble> bands;std::vector<double> base_pre;double base_mean=0;};
// Default sp0,96x96,30ppd: row-major Double input and normalized bands.
SP0PyramidDouble build_sp0_pyramid_double(std::span<const double> response);
}
