#pragma once
#include <array>
#include <filesystem>
#include <vector>
#include <IQM/hdrvdp3/quality.h>
namespace IQM { class VulkanInstance; }
namespace IQM::HDRVDP3 {
struct PathwayMaskingAssets {std::filesystem::path map,product,base;bool variableBaseCount=false;unsigned mapABI=13;};
struct PathwayBandInput {std::vector<float> reference,test,self;std::vector<double> comparison;};
struct PathwayMaskingModel {double p,pf,q,selfScale,neighborScale;};
PathwayMaskingModel pathway_masking_model();
struct PathwayBandResult {std::vector<float> neighbor,denominator,difference,sensitivity,D,psychometric,masked;float error;};
struct PathwayMaskingResult {std::array<PathwayBandResult,5> bands;std::vector<float> baseAdaptation,baseLog;std::array<double,3> baseNoise;QualityScores scores;};
// Synchronous,exclusive instance. Row-major GPU-derived bands/log adaptation,
// double comparison masks,residual selection.80x96/96x80/96x96,30ppd age24.
// Fixed model/CPU double NCSF; no native intermediates or fixture reads.
PathwayMaskingResult run_pathway_masking(const IQM::VulkanInstance& instance,
 const std::array<PathwayBandInput,5>& bands,const std::vector<float>& logAdaptation,
 unsigned width,unsigned height,unsigned selectedIndex,const PathwayMaskingAssets& assets);
}
