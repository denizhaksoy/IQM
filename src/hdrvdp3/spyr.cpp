/*
 * Image Quality Metrics - HDR-VDP-3 Steerable Pyramid
 * Deniz Aksoy - 2026
 */

#include <IQM/hdrvdp3/spyr.h>
#include <IQM/hdrvdp3/corrdn.h>
#include <algorithm>
#include <stdexcept>
#include <cmath>

namespace IQM {
namespace HDRVDP3 {

Spyr::Spyr(const vk::raii::Device& device, const Filters& filters)
    : m_filters(filters)
    , m_corrdn(std::make_unique<CorrDn>(device))
{
}

unsigned Spyr::max_pyr_height(unsigned width, unsigned height, unsigned filter_size) {
    return max_pyr_height_rec(width, height, filter_size);
}

unsigned Spyr::max_pyr_height_rec(unsigned width, unsigned height, unsigned filter_size) {
    if (width < filter_size || height < filter_size) {
        return 0;
    }
    return 1 + max_pyr_height_rec(width / 2, height / 2, filter_size);
}

} // namespace HDRVDP3
} // namespace IQM