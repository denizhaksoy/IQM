#include <IQM/hdrvdp3/linear_color_single.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace IQM::HDRVDP3 {
std::vector<float> prepare_linear_color_single(std::span<const float> input,
    const XYZDisplaySamples& display,LinearColorEncoding encoding) {
    if(input.empty()||input.size()%3)throw std::invalid_argument("Single color requires three nonempty planes");
    if(encoding!=LinearColorEncoding::BT709&&encoding!=LinearColorEncoding::BT2020&&encoding!=LinearColorEncoding::XYZ)
        throw std::invalid_argument("Unknown Single linear color encoding");
    for(float x:input)if(!std::isfinite(x)||x<0)throw std::invalid_argument("Single absolute input must be finite nonnegative");
    const auto matrix=linear_color_transform(display,encoding);
    const size_t n=input.size()/3;std::vector<float> output(input.size());
    for(size_t c=0;c<3;++c)for(size_t i=0;i<n;++i){
        const float first=input[i]*float(matrix[c][0]);
        const float second=std::fma(input[n+i],float(matrix[c][1]),first);
        output[c*n+i]=std::max(std::fma(input[2*n+i],float(matrix[c][2]),second),1e-6f);
    }
    return output;
}
}
