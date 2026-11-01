#include <IQM/hdrvdp3/pq_single.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace IQM::HDRVDP3 {
float normalize_pq_u16_single(std::uint16_t code) { return float(code)/65535.f; }
std::vector<float> pq_to_display_single(std::span<const float> encoded,
                                      const PQDisplaySingle& display) {
    if (!std::isfinite(display.ambient_lux) || display.ambient_lux<0 ||
        !std::isfinite(display.reflectivity) || display.reflectivity<0 || display.reflectivity>1 ||
        !std::isfinite(display.peak_cd_m2) || display.peak_cd_m2<=0)
        throw std::invalid_argument("Invalid explicit PQ display parameters");
    const double reflected=display.ambient_lux*display.reflectivity/3.14159265358979323846;
    const double cap=display.peak_cd_m2+reflected;
    if (!std::isfinite(cap) || cap>std::numeric_limits<float>::max())
        throw std::invalid_argument("PQ display limit exceeds Single range");
    const float ambient=float(reflected),limit=float(cap);
    std::vector<float> result;result.reserve(encoded.size());
    for (float unit:encoded) {
        if (!std::isfinite(unit) || unit<0 || unit>1)
            throw std::invalid_argument("PQ input must be finite normalized RGB in [0,1]");
        const float t=std::pow(unit,float(1./78.84375));
        const float numerator=std::max(t-.8359375f,0.f);
        const float denominator=18.8515625f-18.6875f*t;
        const float ratio=numerator/denominator;
        const float linear=10000.f*std::pow(ratio,float(1./.1593017578125));
        result.push_back(std::min(linear+ambient,limit));
    }
    return result;
}
}
