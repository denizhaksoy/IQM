#include <IQM/hdrvdp3/pathway_quality.h>
#include <IQM/hdrvdp3/map_gpu.h>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace IQM::HDRVDP3 {
QualityScores evaluate_pathway_quality(const IQM::VulkanInstance& instance,
    const std::vector<float>& reference,const std::vector<float>& test,
    unsigned w,unsigned h,const PathwayQualityAssets& assets,
    PathwayQualityTrace* trace) {
    if(!((h==80&&w==96)||(h==96&&w==80)) || reference.size()!=size_t(h)*w || test.size()!=reference.size())
        throw std::invalid_argument("Unvalidated pathway quality dimensions");
    for(const auto* image:{&reference,&test})
        for(float value:*image)
            if(!std::isfinite(value)||value<=0)
                throw std::invalid_argument("Positive finite luminance required");
    PathwayQualityTrace result;
    MapGPU gpu(instance,assets.masking.map.string());
    const auto lut=create_jnd_lut();
    const std::array<const std::vector<float>*,2> originals{&reference,&test};
    for(unsigned image=0;image<2;++image) {
        result.retinal[image]=run_mixed_filter(instance,*originals[image],w,h,
            MixedFilterMode::OpticalMTF,assets.filter).output;
        const auto input=gpu.adaptation_input(result.retinal[image],D65_AGE24_LMR);
        result.adaptation[image]=run_mixed_filter(instance,input,w,h,
            MixedFilterMode::LocalAdaptation,assets.filter).output;
        const auto pupil=gpu.pupil_trace(result.adaptation[image],w,h,double(w)*h/(30.0*30.0),24);
        const auto response=gpu.jnd_response(result.retinal[image],lut,pupil[3],D65_AGE24_LMR);
        result.P[image].reserve(response.size());
        for(const auto& pixel:response)result.P[image].push_back(pixel.response);
    }
    result.logAdaptation=gpu.log_adaptation(gpu.mean_adaptation(result.adaptation[0],result.adaptation[1]));
    result.spyr=run_pathway_spyr(instance,result.P[0],result.P[1],w,h,assets.filters,assets.spyr);
    const std::array<unsigned,5> widths{w,w,w/2,w/4,w/8},heights{h,h,h/2,h/4,h/8};
    std::array<PathwayBandInput,5> bands;
    for(unsigned band=0;band<5;++band) {
        const size_t count=size_t(widths[band])*heights[band];
        const auto& words=result.spyr.stages[band<4?band+4:8];
        const size_t refOffset=band<4?0:1,testOffset=band<4?count:122;
        if(words.size()!=(band<4?2*count:242))throw std::runtime_error("SPYR band size");
        auto& input=bands[band];
        for(size_t i=0;i<count;++i) {
            input.reference.push_back(std::bit_cast<float>(words[refOffset+i]));
            input.test.push_back(std::bit_cast<float>(words[testOffset+i]));
        }
        result.pool[band]=run_pathway_pool(instance,input.reference,input.test,widths[band],heights[band],assets.pool);
        result.comparison[band]=run_pathway_comparison(instance,reference,test,w,h,widths[band],heights[band],assets.comparison);
        input.self=result.pool[band].pooled;
        input.comparison=result.comparison[band].resized;
    }
    const auto& residual=result.spyr.stages[10];
    if(residual.size()!=361)throw std::runtime_error("Residual selection size");
    const float selected=std::bit_cast<float>(residual[360]);
    if(!std::isfinite(selected)||selected<0||selected>=120||selected!=std::trunc(selected))
        throw std::runtime_error("Invalid residual selection");
    result.masking=run_pathway_masking(instance,bands,result.logAdaptation,w,h,unsigned(selected),assets.masking);
    const auto scores=result.masking.scores;
    if(trace)*trace=std::move(result);
    return scores;
}
}
