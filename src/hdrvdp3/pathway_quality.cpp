#include <IQM/hdrvdp3/pathway_quality.h>
#include <IQM/hdrvdp3/map_gpu.h>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace IQM::HDRVDP3 {
#if defined(IQM_HDRVDP3_SQUARE_QUALITY_DIAGNOSTIC)
// Private diagnostic entry points; normal public geometry remains unchanged.
MixedFilterResult run_square_filter_diagnostic(const IQM::VulkanInstance&,const std::vector<float>&,unsigned,unsigned,MixedFilterMode,const MixedFilterAssets&,bool);
PathwaySPYRResult run_square_spyr_diagnostic(const IQM::VulkanInstance&,const std::vector<float>&,const std::vector<float>&,unsigned,unsigned,const PathwaySPYRFilters&,const PathwaySPYRAssets&);
PathwayPoolResult run_square_pool_diagnostic(const IQM::VulkanInstance&,const std::vector<float>&,const std::vector<float>&,unsigned,unsigned,const PathwayPoolAssets&);
PathwayComparisonResult run_square_comparison_diagnostic(const IQM::VulkanInstance&,const std::vector<float>&,const std::vector<float>&,unsigned,unsigned,unsigned,unsigned,const std::filesystem::path&);
PathwayMaskingResult run_square_masking_diagnostic(const IQM::VulkanInstance&,const std::array<PathwayBandInput,5>&,const std::vector<float>&,unsigned,unsigned,unsigned,const PathwayMaskingAssets&);
#define evaluate_pathway_quality evaluate_square_quality_diagnostic
#define run_mixed_filter run_square_filter_diagnostic
#define run_pathway_spyr run_square_spyr_diagnostic
#define run_pathway_pool run_square_pool_diagnostic
#define run_pathway_comparison run_square_comparison_diagnostic
#define run_pathway_masking run_square_masking_diagnostic
#endif

static QualityScores finish_quality(const IQM::VulkanInstance& instance,
 const std::vector<float>& reference,const std::vector<float>& test,unsigned w,unsigned h,
 const PathwayQualityAssets& assets,PathwayQualityTrace result,PathwayQualityTrace* trace,bool rgb);
QualityScores evaluate_pathway_quality(const IQM::VulkanInstance& instance,
    const std::vector<float>& reference,const std::vector<float>& test,
    unsigned w,unsigned h,const PathwayQualityAssets& assets,
    PathwayQualityTrace* trace) {
#if defined(IQM_HDRVDP3_SQUARE128_QUALITY_DIAGNOSTIC)
    const bool shapeOK=h==128&&w==128;constexpr size_t baseCount=256;
#elif defined(IQM_HDRVDP3_SQUARE80_QUALITY_DIAGNOSTIC)
    const bool shapeOK=h==80&&w==80;constexpr size_t baseCount=100;
#elif defined(IQM_HDRVDP3_SQUARE_QUALITY_DIAGNOSTIC)
    const bool shapeOK=h==96&&w==96;constexpr size_t baseCount=144;
#else
    const bool shapeOK=(h==128&&w==128)||(h==80&&w==96)||(h==80&&w==80)||(h==96&&w==80)||(h==96&&w==96);const size_t baseCount=size_t(h/8)*(w/8);
#endif
    if(!shapeOK || reference.size()!=size_t(h)*w || test.size()!=reference.size())
        throw std::invalid_argument("Unvalidated pathway quality dimensions");
#if !defined(IQM_HDRVDP3_SQUARE_QUALITY_DIAGNOSTIC)
    if(((h==80&&w==80)||(h==128&&w==128))&&(!assets.spyr.variableMeanCount||!assets.masking.variableBaseCount))
        throw std::invalid_argument("Square80/128 require variable-count runtime assets");
    if(assets.filter.reference256Graph!=(h==128&&w==128)||assets.masking.mapABI!=(h==128&&w==128?14u:13u))
        throw std::invalid_argument("Runtime FFT/map ABI does not match geometry");
#endif
    for(const auto* image:{&reference,&test})
        for(float value:*image)
            if(!std::isfinite(value)||value<=0)
                throw std::invalid_argument("Positive finite luminance required");
    PathwayQualityTrace result;
#if defined(IQM_HDRVDP3_SQUARE128_QUALITY_DIAGNOSTIC)
    MapGPU gpu(instance,assets.masking.map.string(),MapGPUProfile::Extended128Components);
#else
    MapGPU gpu(instance,assets.masking.map.string(),static_cast<MapGPUProfile>(assets.masking.mapABI));
#endif
    const auto lut=create_jnd_lut();
    const std::array<const std::vector<float>*,2> originals{&reference,&test};
    for(unsigned image=0;image<2;++image) {
        result.retinal[image]=run_mixed_filter(instance,*originals[image],w,h,
            MixedFilterMode::OpticalMTF,assets.filter,false).output;
        const auto input=gpu.adaptation_input(result.retinal[image],D65_AGE24_LMR);
        result.adaptation[image]=run_mixed_filter(instance,input,w,h,
            MixedFilterMode::LocalAdaptation,assets.filter,false).output;
        const auto pupil=gpu.pupil_trace(result.adaptation[image],w,h,double(w)*h/(30.0*30.0),24);
        const auto response=gpu.jnd_response(result.retinal[image],lut,pupil[3],D65_AGE24_LMR);
        result.P[image].reserve(response.size());
        for(const auto& pixel:response)result.P[image].push_back(pixel.response);
    }
    return finish_quality(instance,reference,test,w,h,assets,std::move(result),trace,false);
}
static QualityScores finish_quality(const IQM::VulkanInstance& instance,
 const std::vector<float>& reference,const std::vector<float>& test,unsigned w,unsigned h,
 const PathwayQualityAssets& assets,PathwayQualityTrace result,PathwayQualityTrace* trace,bool rgb) {
 const size_t baseCount=size_t(h/8)*(w/8);
#if defined(IQM_HDRVDP3_SQUARE128_QUALITY_DIAGNOSTIC)
 MapGPU gpu(instance,assets.masking.map.string(),MapGPUProfile::Extended128Components);
#else
 MapGPU gpu(instance,assets.masking.map.string(),static_cast<MapGPUProfile>(assets.masking.mapABI));
#endif
    result.logAdaptation=gpu.log_adaptation(gpu.mean_adaptation(result.adaptation[0],result.adaptation[1]));
    result.spyr=run_pathway_spyr(instance,result.P[0],result.P[1],w,h,assets.filters,assets.spyr);
    const std::array<unsigned,5> widths{w,w,w/2,w/4,w/8},heights{h,h,h/2,h/4,h/8};
    std::array<PathwayBandInput,5> bands;
    for(unsigned band=0;band<5;++band) {
        const size_t count=size_t(widths[band])*heights[band];
        const auto& words=result.spyr.stages[band<4?band+4:8];
        const size_t refOffset=band<4?0:1,testOffset=band<4?count:baseCount+2;
        if(words.size()!=(band<4?2*count:2*(baseCount+1)))throw std::runtime_error("SPYR band size");
        auto& input=bands[band];
        for(size_t i=0;i<count;++i) {
            input.reference.push_back(std::bit_cast<float>(words[refOffset+i]));
            input.test.push_back(std::bit_cast<float>(words[testOffset+i]));
        }
        result.pool[band]=run_pathway_pool(instance,input.reference,input.test,widths[band],heights[band],assets.pool);
        result.comparison[band]=rgb?run_pathway_comparison_rgb(instance,reference,test,w,h,widths[band],heights[band],assets.comparison):run_pathway_comparison(instance,reference,test,w,h,widths[band],heights[band],assets.comparison);
        input.self=result.pool[band].pooled;
        input.comparison=result.comparison[band].resized;
    }
    const auto& residual=result.spyr.stages[10];
    if(residual.size()!=3*baseCount+1)throw std::runtime_error("Residual selection size");
    const float selected=std::bit_cast<float>(residual[3*baseCount]);
    if(!std::isfinite(selected)||selected<0||selected>=baseCount||selected!=std::trunc(selected))
        throw std::runtime_error("Invalid residual selection");
    result.masking=run_pathway_masking(instance,bands,result.logAdaptation,w,h,unsigned(selected),assets.masking);
    const auto scores=result.masking.scores;
    if(trace)*trace=std::move(result);
    return scores;
}
QualityScores evaluate_pathway_rgb_response_quality(const IQM::VulkanInstance& instance,
 const std::vector<float>& reference,const std::vector<float>& test,
 const std::array<std::vector<float>,2>& P,const std::array<std::vector<float>,2>& adaptation,
 unsigned w,unsigned h,const PathwayQualityAssets& assets,PathwayQualityTrace* trace) {
 if(w!=96||h!=96||reference.size()!=3*w*h||test.size()!=reference.size()||assets.masking.mapABI!=13||assets.filter.reference256Graph)
  throw std::invalid_argument("Unvalidated RGB response quality profile");
 for(const auto* image:{&reference,&test})for(float v:*image)if(!std::isfinite(v)||v<0)throw std::invalid_argument("Invalid RGB input");
 for(unsigned i=0;i<2;++i){
  if(P[i].size()!=w*h||adaptation[i].size()!=w*h)throw std::invalid_argument("RGB response dimensions");
  for(float v:P[i])if(!std::isfinite(v))throw std::invalid_argument("Invalid P response");
  for(float v:adaptation[i])if(!std::isfinite(v)||v<=0)throw std::invalid_argument("Invalid adaptation");
 }
 PathwayQualityTrace result;result.P=P;result.adaptation=adaptation;
 return finish_quality(instance,reference,test,w,h,assets,std::move(result),trace,true);
}
}