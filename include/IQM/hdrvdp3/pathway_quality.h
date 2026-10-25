#pragma once
#include <IQM/hdrvdp3/mixed_filter.h>
#include <IQM/hdrvdp3/pathway_spyr.h>
#include <IQM/hdrvdp3/pathway_pool.h>
#include <IQM/hdrvdp3/pathway_comparison.h>
#include <IQM/hdrvdp3/pathway_masking.h>

namespace IQM::HDRVDP3 {
struct PathwayQualityAssets {
    MixedFilterAssets filter;
    PathwaySPYRAssets spyr;
    PathwaySPYRFilters filters;
    PathwayPoolAssets pool;
    std::filesystem::path comparison;
    PathwayMaskingAssets masking;
};
struct PathwayQualityTrace {
    std::array<std::vector<float>,2> retinal,adaptation,P;
    std::vector<float> logAdaptation;
    PathwaySPYRResult spyr;
    std::array<PathwayPoolResult,5> pool;
    std::array<PathwayComparisonResult,5> comparison;
    PathwayMaskingResult masking;
};
// Isolated synchronous quality profile: positive finite row-major cd/m^2,
// 80x96/96x80,30ppd,age24,D65,explicit sp0 model/shader assets. No environment
// or image/intermediate file reads. CPU model/LUTs,double NCSF and host-vector
// transfers remain. Exclusive instance use required; public API guards unchanged.
QualityScores evaluate_pathway_quality(const IQM::VulkanInstance& instance,
    const std::vector<float>& reference,const std::vector<float>& test,
    unsigned width,unsigned height,const PathwayQualityAssets& assets,
    PathwayQualityTrace* trace=nullptr);
}
