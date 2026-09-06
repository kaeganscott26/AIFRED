#pragma once
#include "aifred/Pipeline.h"
#include <algorithm>

namespace aifred
{
enum class AnalysisMode {Analyze,Reference,Compare};
enum class Domain {Tone,Stereo,Loudness,Dynamics};
inline float clamp01(float value) {return std::clamp(value,0.0f,1.0f);}
struct ReferenceTarget
{
    core::ReferenceDistribution distribution;
    float rmsScale=0,widthScale=0,crestScale=0,loudnessDb=0,crestDb=0;
    int poolSize=0;
    std::string label="No reference";
};
struct DisplayMetrics
{
    float rmsDb=0,peakDb=0,truePeakDb=0,crestDb=0,shortTermLufs=0,integratedLufs=0,stereoWidth=0,correlation=0;
    float rmsScale=0,widthScale=0,crestScale=0,loudnessScale=0;
    std::array<float,8> spectrumBands {};
    std::array<float,96> waveform {};
    std::array<float,10> sessionCandleOpen {},sessionCandleHigh {},sessionCandleLow {},sessionCandleClose {};
    std::array<float,10> minuteCandleOpen {},minuteCandleHigh {},minuteCandleLow {},minuteCandleClose {};
    std::array<float,10> liveCandleOpen {},liveCandleHigh {},liveCandleLow {},liveCandleClose {};
    int sessionCandleCount=0,minuteCandleCount=0,liveCandleCount=0;
};
struct MetricDetail
{
    core::MetricId id=core::MetricId::samplePeak;
    std::string_view displayName,unit;
    bool valid=false,isLive=false;
    double rawCurrent=0,displayedValue=0;
    core::MetricObservation observed;
    core::ProfileId activeProfile=core::ProfileId::mixBalanced,emphasizedBy=core::ProfileId::mixBalanced;
    std::uint32_t profileRevision=1;
};
struct BetaView
{
    AnalysisMode mode=AnalysisMode::Analyze;
    DisplayMetrics metrics;
    ReferenceTarget reference;
    core::ObservationSnapshot observation;
    std::array<double,core::maximumBins> spectrumPower {};
    std::array<double,core::maximumBins> peakSpectrumPower {};
    std::array<MetricDetail,core::metricCount> metricDetails {};
    std::size_t binCount=0;
    double binWidthHz=0;
    core::ProfileId activeProfile=core::ProfileId::mixBalanced;
    std::uint32_t profileRevision=1;
    std::string_view measurementConfigurationId="MIX_BALANCED.r1";
    core::PresentationConfiguration presentation=core::standardPresentation;
    bool hasSignal=false,hasReference=false,valuesValid=false,isStale=true;
};
inline BetaView makeBetaView(const core::EngineSnapshot& live,const core::ObservationSnapshot& observation,
                            core::PresentationConfiguration presentation=core::standardPresentation)
{
    BetaView view;view.observation=observation;view.hasSignal=observation.signalActive;view.valuesValid=observation.valid;view.isStale=!observation.fresh;
    view.spectrumPower=live.averagePower;view.peakSpectrumPower=live.peakPower;view.binCount=live.binCount;view.binWidthHz=live.binWidthHz;
    view.activeProfile=live.profileId;view.profileRevision=live.profileVersion;view.measurementConfigurationId=core::profile(live.profileId).identity;view.presentation=presentation;
    const auto value=[&](core::MetricId id){const auto& m=observation.get(id);return m.valid?static_cast<float>(m.typical):0.0f;};
    auto& m=view.metrics;
    m.rmsDb=value(core::MetricId::rms);m.peakDb=value(core::MetricId::samplePeak);m.truePeakDb=value(core::MetricId::truePeak);m.crestDb=value(core::MetricId::crest);
    m.shortTermLufs=value(core::MetricId::shortTerm);m.integratedLufs=value(core::MetricId::integrated);m.stereoWidth=static_cast<float>(live.get(core::MetricId::width).value)/100; m.correlation=static_cast<float>(live.get(core::MetricId::correlation).value);
    m.rmsScale=clamp01((m.rmsDb+60)/60);m.widthScale=m.stereoWidth;m.crestScale=clamp01(m.crestDb/24);m.loudnessScale=clamp01((m.shortTermLufs+60)/60);
    constexpr std::array<std::size_t,8> displayedBands {2,7,10,13,17,20,23,27};
    const auto spectrumFloor=static_cast<float>(core::spectrumFloorDb(presentation.spectrumRange));
    for(std::size_t i=0;i<8;++i) {const auto& b=observation.bands[displayedBands[i]];m.spectrumBands[i]=b.valid?clamp01(static_cast<float>((b.typical-spectrumFloor)/-spectrumFloor)):0;}
    for(std::size_t i=0;i<live.vectorscopeCount;++i)m.waveform[i]=live.vectorscope[i][0];
    const auto& rms=observation.get(core::MetricId::rms);
    if(rms.valid)
    {
        // A section candle is a range of observed RMS measurements, never a
        // composite interpretation. Session/minute history is unavailable until measured.
        m.liveCandleCount=1;m.liveCandleOpen[0]=static_cast<float>(rms.typical);m.liveCandleHigh[0]=static_cast<float>(rms.maximum);
        m.liveCandleLow[0]=static_cast<float>(rms.minimum);m.liveCandleClose[0]=static_cast<float>(rms.latest);
    }
    for(std::size_t i=0;i<core::metricCount;++i)
    {
        auto& detail=view.metricDetails[i];const auto& definition=core::metricDefinitions[i];
        detail.id=static_cast<core::MetricId>(i);detail.displayName=definition.displayName;detail.unit=definition.unit;
        detail.isLive=definition.source==core::MetricSource::live;detail.rawCurrent=live.metrics[i].value;detail.observed=observation.metrics[i];
        detail.activeProfile=live.profileId;detail.profileRevision=live.profileVersion;detail.emphasizedBy=definition.emphasizedBy;
        const auto shown=detail.isLive?live.metrics[i]:core::MetricValue{detail.observed.typical,detail.observed.valid};
        detail.valid=shown.valid;detail.displayedValue=shown.value;
    }
    return view;
}
}
