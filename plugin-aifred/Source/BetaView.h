#pragma once
#include "aifred/Pipeline.h"
#include <algorithm>

namespace aifred
{
enum class AnalysisMode {Analyze,Reference,Compare};
enum class Domain {Tone,Stereo,Loudness,Dynamics};

inline float clamp01(float value)
{
    return std::clamp(value,0.0f,1.0f);
}

// -----------------------------------------------------------------------------
// Halo presentation ranges
//
// These are UI presentation ranges only. They do not modify DSP values.
//
// Crest:      0 -> 24 dB
// RMS:       -24 ->  0 dBFS
// True Peak: -24 ->  0 dBTP
// Stereo:     +1 -> -1 correlation mapped to 0 -> 1 spread
// -----------------------------------------------------------------------------

inline constexpr float crestFloorDb=0.0f;
inline constexpr float crestCeilingDb=24.0f;

inline constexpr float haloRmsFloorDbfs=-24.0f;
inline constexpr float haloRmsCeilingDbfs=0.0f;

inline constexpr float truePeakFloorDbtp=-24.0f;
inline constexpr float truePeakCeilingDbtp=0.0f;

inline float linearPresentation(float value,float minimum,float maximum)
{
    return maximum>minimum
        ? clamp01((value-minimum)/(maximum-minimum))
        : 0.0f;
}

// -----------------------------------------------------------------------------
// EXISTING spectrum-aware RMS presentation.
//
// KEEP THIS FUNCTION SIGNATURE.
// Other code/tests in the Beta repository use it.
//
// This remains available for spectrum-range presentation and compatibility.
// The Halo itself uses haloRmsPresentation() below.
// -----------------------------------------------------------------------------

inline float rmsPresentation(float value,core::SpectrumDisplayRange range)
{
    return linearPresentation(
        value,
        static_cast<float>(core::spectrumFloorDb(range)),
        0.0f);
}

// -----------------------------------------------------------------------------
// HALO RMS PRESENTATION
//
// The Halo RMS lane is NOT a spectrum meter.
//
// -24 dBFS = 0%
// -12 dBFS = 50%
//   0 dBFS = 100%
//
// Example:
// -22.4 dBFS -> ~6.7%
// -12.0 dBFS -> 50%
//  -6.0 dBFS -> 75%
//   0.0 dBFS -> 100%
// -----------------------------------------------------------------------------

inline float haloRmsPresentation(float value)
{
    return linearPresentation(
        value,
        haloRmsFloorDbfs,
        haloRmsCeilingDbfs);
}

// -----------------------------------------------------------------------------
// True Peak
//
// -24 dBTP = 0%
// -12 dBTP = 50%
//   0 dBTP = 100%
//
// Hotter values move farther through the Halo toward 0 dBTP.
// -----------------------------------------------------------------------------

inline float truePeakPresentation(float value)
{
    return linearPresentation(
        value,
        truePeakFloorDbtp,
        truePeakCeilingDbtp);
}

// -----------------------------------------------------------------------------
// Crest
//
// 0 dB = 0%
// 12 dB = 50%
// 24 dB = 100%
// -----------------------------------------------------------------------------

inline float crestPresentation(float value)
{
    return linearPresentation(
        value,
        crestFloorDb,
        crestCeilingDb);
}

// -----------------------------------------------------------------------------
// Stereo correlation -> spread
//
// +1 correlation = 0 spread
//  0 correlation = 50%
// -1 correlation = 100% / phase
// -----------------------------------------------------------------------------

inline float stereoSpreadPresentation(float correlation)
{
    return clamp01((1.0f-correlation)*0.5f);
}

struct ResponsiveCanvas
{
    float scale=1,
    x=0,
    y=0,
    width=1360,
    height=820;
};

inline ResponsiveCanvas responsiveCanvas(float width,float height) noexcept
{
    const auto scale=std::min(width/1360.0f,height/820.0f);

    return {
        scale,
        (width-1360.0f*scale)*0.5f,
        (height-820.0f*scale)*0.5f,
        1360.0f*scale,
        820.0f*scale
    };
}

inline bool modalShouldDismiss(
    float x,
    float y,
    float width,
    float height,
    float pointX,
    float pointY) noexcept
{
    return pointX<x
        ||pointY<y
        ||pointX>x+width
        ||pointY>y+height;
}

struct ReferenceTarget
{
    core::ReferenceDistribution distribution;

    float rmsScale=0,
          widthScale=0,
          crestScale=0,
          loudnessDb=0,
          crestDb=0;

    int poolSize=0;

    std::string label="No reference";
};

struct DisplayMetrics
{
    float rmsDb=0,
          peakDb=0,
          truePeakDb=0,
          crestDb=0,
          shortTermLufs=0,
          integratedLufs=0,
          stereoWidth=0,
          correlation=0;

    float rmsScale=0,
          widthScale=0,
          crestScale=0,
          loudnessScale=0;

    std::array<float,core::bandCentres.size()> spectrumBands {};
    std::array<float,96> waveform {};

    std::array<float,10> sessionCandleOpen {},
                               sessionCandleHigh {},
                               sessionCandleLow {},
                               sessionCandleClose {};

    std::array<float,10> minuteCandleOpen {},
                               minuteCandleHigh {},
                               minuteCandleLow {},
                               minuteCandleClose {};

    std::array<float,10> liveCandleOpen {},
                               liveCandleHigh {},
                               liveCandleLow {},
                               liveCandleClose {};

    int sessionCandleCount=0,
        minuteCandleCount=0,
        liveCandleCount=0;
};

struct MetricDetail
{
    core::MetricId id=core::MetricId::samplePeak;

    std::string_view displayName,unit;

    bool valid=false,
         isLive=false;

    double rawCurrent=0,
           displayedValue=0;

    core::MetricObservation observed;

    core::ProfileId activeProfile=core::ProfileId::mixBalanced,
                    emphasizedBy=core::ProfileId::mixBalanced;

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

    bool hasSignal=false,
         hasReference=false,
         valuesValid=false,
         isStale=true;
};

inline BetaView makeBetaView(
    const core::EngineSnapshot& live,
    const core::ObservationSnapshot& observation,
    core::PresentationConfiguration presentation=core::standardPresentation)
{
    BetaView view;

    view.observation=observation;
    view.hasSignal=observation.signalActive;
    view.valuesValid=observation.valid;
    view.isStale=!observation.fresh;

    view.spectrumPower=live.averagePower;
    view.peakSpectrumPower=live.peakPower;
    view.binCount=live.binCount;
    view.binWidthHz=live.binWidthHz;

    view.activeProfile=live.profileId;
    view.profileRevision=live.profileVersion;

    view.measurementConfigurationId=
        core::profile(live.profileId).identity;

    view.presentation=presentation;

    const auto value=
        [&](core::MetricId id)
        {
            const auto& m=observation.get(id);

            return m.valid
                ? static_cast<float>(m.typical)
                : 0.0f;
        };

    auto& m=view.metrics;

    m.rmsDb=value(core::MetricId::rms);
    m.peakDb=value(core::MetricId::samplePeak);
    m.truePeakDb=value(core::MetricId::truePeak);
    m.crestDb=value(core::MetricId::crest);

    m.shortTermLufs=value(core::MetricId::shortTerm);
    m.integratedLufs=value(core::MetricId::integrated);

    m.stereoWidth=
        static_cast<float>(
            live.get(core::MetricId::width).value)/100;

    m.correlation=
        static_cast<float>(
            live.get(core::MetricId::correlation).value);

    // -------------------------------------------------------------------------
    // HALO SCALE MAPPINGS
    //
    // These are intentionally independent from the spectrum presentation.
    // -------------------------------------------------------------------------

    m.rmsScale=haloRmsPresentation(m.rmsDb);

    m.widthScale=
        stereoSpreadPresentation(m.correlation);

    m.crestScale=
        crestPresentation(m.crestDb);

    // Keep loudness presentation independent from the spectrum floor.
    // The Halo's loudness presentation uses the same -24 -> 0 LUFS span.
    m.loudnessScale=
        linearPresentation(
            m.shortTermLufs,
            -24.0f,
            0.0f);

    // -------------------------------------------------------------------------
    // SPECTRUM
    //
    // Spectrum continues to use its configured display range.
    // This is where -96 dB, -72 dB, etc. belong.
    // -------------------------------------------------------------------------

    const auto spectrumFloor=
        static_cast<float>(
            core::spectrumFloorDb(presentation.spectrumRange));

    for(std::size_t i=0;i<m.spectrumBands.size();++i)
    {
        const auto& b=observation.bands[i];

        m.spectrumBands[i]=
            b.valid
                ? linearPresentation(
                    static_cast<float>(b.typical),
                    spectrumFloor,
                    0.0f)
                : 0;
    }

    // -------------------------------------------------------------------------
    // VECTORSCOPE
    // -------------------------------------------------------------------------

    for(std::size_t i=0;i<live.vectorscopeCount;++i)
        m.waveform[i]=live.vectorscope[i][0];

    // -------------------------------------------------------------------------
    // LIVE CANDLE
    // -------------------------------------------------------------------------

    const auto& rms=observation.get(core::MetricId::rms);

    if(rms.valid)
    {
        // A section candle is a range of observed RMS measurements,
        // never a composite interpretation. Session/minute history
        // is unavailable until measured.

        m.liveCandleCount=1;

        m.liveCandleOpen[0]=
            static_cast<float>(rms.typical);

        m.liveCandleHigh[0]=
            static_cast<float>(rms.maximum);

        m.liveCandleLow[0]=
            static_cast<float>(rms.minimum);

        m.liveCandleClose[0]=
            static_cast<float>(rms.latest);
    }

    // -------------------------------------------------------------------------
    // METRIC DETAILS
    // -------------------------------------------------------------------------

    for(std::size_t i=0;i<core::metricCount;++i)
    {
        auto& detail=view.metricDetails[i];
        const auto& definition=core::metricDefinitions[i];

        detail.id=
            static_cast<core::MetricId>(i);

        detail.displayName=
            definition.displayName;

        detail.unit=
            definition.unit;

        detail.isLive=
            definition.source==core::MetricSource::live;

        detail.rawCurrent=
            live.metrics[i].value;

        detail.observed=
            observation.metrics[i];

        detail.activeProfile=
            live.profileId;

        detail.profileRevision=
            live.profileVersion;

        detail.emphasizedBy=
            definition.emphasizedBy;

        const auto shown=
            detail.isLive
                ? live.metrics[i]
                : core::MetricValue{
                    detail.observed.typical,
                    detail.observed.valid
                  };

        detail.valid=
            shown.valid;

        detail.displayedValue=
            shown.value;
    }

    return view;
}

inline float compareDelta(
    float mixA,
    float mixB) noexcept
{
    return mixA-mixB;
}

inline float compareSimilarity(
    const BetaView& a,
    const BetaView& b) noexcept
{
    // Display-only, deterministic similarity.
    // Every term is an existing DSP measurement normalized
    // by its physical presentation span.

    const std::array<float,6> differences {
        // RMS physical presentation span:
        // -24 dBFS -> 0 dBFS
        std::abs(a.metrics.rmsDb-b.metrics.rmsDb)
            /(haloRmsCeilingDbfs-haloRmsFloorDbfs),

        // True peak:
        // -24 dBTP -> 0 dBTP
        std::abs(a.metrics.truePeakDb-b.metrics.truePeakDb)
            /(truePeakCeilingDbtp-truePeakFloorDbtp),

        // Crest:
        // 0 dB -> 24 dB
        std::abs(a.metrics.crestDb-b.metrics.crestDb)
            /(crestCeilingDb-crestFloorDb),

        // Loudness:
        // -24 LUFS -> 0 LUFS
        std::abs(a.metrics.shortTermLufs-b.metrics.shortTermLufs)
            /24.0f,

        // Stereo width is already normalized.
        std::abs(a.metrics.stereoWidth-b.metrics.stereoWidth),

        // Correlation spans -1 -> +1.
        std::abs(a.metrics.correlation-b.metrics.correlation)
            /2.0f
    };

    float total=0.0f;

    for(const auto difference:differences)
        total+=clamp01(difference);

    return 100.0f*
        (1.0f-total/static_cast<float>(differences.size()));
}

inline void applyCandleHistory(
    BetaView& view,
    const core::CandleHistorySnapshot& history)
{
    view.metrics.sessionCandleOpen=history.sessionOpen;
    view.metrics.sessionCandleHigh=history.sessionHigh;
    view.metrics.sessionCandleLow=history.sessionLow;
    view.metrics.sessionCandleClose=history.sessionClose;

    view.metrics.minuteCandleOpen=history.minuteOpen;
    view.metrics.minuteCandleHigh=history.minuteHigh;
    view.metrics.minuteCandleLow=history.minuteLow;
    view.metrics.minuteCandleClose=history.minuteClose;

    view.metrics.liveCandleOpen=history.liveOpen;
    view.metrics.liveCandleHigh=history.liveHigh;
    view.metrics.liveCandleLow=history.liveLow;
    view.metrics.liveCandleClose=history.liveClose;

    view.metrics.sessionCandleCount=history.sessionCount;
    view.metrics.minuteCandleCount=history.minuteCount;
    view.metrics.liveCandleCount=history.liveCount;
}

}