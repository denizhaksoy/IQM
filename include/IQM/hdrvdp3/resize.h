#pragma once
#include <vector>
namespace IQM::HDRVDP3 {
// Double coefficients are prepared once; image arithmetic retains its input type.
struct ResizeAxisPlan {
    int taps=0;
    std::vector<int> indices;
    std::vector<double> weights,normalization;
};
ResizeAxisPlan make_resize_axis_plan(int input_size,int output_size);
// Row-major single bicubic resize, antialiasing and symmetric edges.
std::vector<float> resize_bicubic(const std::vector<float>& input,int h,int w,int nh,int nw);
std::vector<double> resize_bicubic(const std::vector<double>& input,int h,int w,int nh,int nw);
}
