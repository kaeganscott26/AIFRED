#pragma once
#include "aifred/Pipeline.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "AifredLookAndFeel.h"
#include "aifred/Contracts.h"
#include "bufferhunter/BufferHunter.h"
#include "JuceDSP/juce_dsp.h"
#include "python3.9/Python.h"
#include "aifred/ReferenceDistribution.h"
#include "plugin/PluginProcessor.h"
#include "pluginEditor/PluginEditor.h"
#include "Shared-dsp/Shared-dsp.h"
#include "dsp/analysis/Analysis.h"
#include "ReferencePoolClient.h"
/* The original file also contained an FL Studio/Python scripting reference.
    It is documentation only and is kept as a comment so this header remains
    valid C++.
  def Editor : 
    property SelectionStartS      Retrieve the start position of the selection in the editor, in samples
    property SelectionEndS        Retrieve the end position of the selection in the editor, in samples




5. Dialog classes and functions
-------------------------------
def ScriptDialog
  restoreFormValues    # set to True to store form values between runs (default is True)
  
  def ScriptDialog(Title, Description)        # Initializes a new script dialog
  def setText(description)                    # Change the description that is shown
  def addInput(name,value, hint='')                  # Adds a generic input control
  def addInputKnob(name, value, min, max, hint='')    # Adds a knob input control with floating point value
  def addInputKnobInt(name, value, min, max, hint='') # Adds a knob input control with integer value
  def addInputCombo(name, valueList, value, hint='') # Adds a combobox input control
  def addInputCheckbox(name, value, hint='')         # Adds a checkbox input control with boolean value
  def addInputText(name, value, hint='')             # Adds a text input control
  def addInputSurface(presetName)             # Adds a Control Surface plugin preset that should be located in same path as script. 
                                              # Specify the name without the .fst extension.
  def getInputValue(aName)                    # Retrieve the current value of the input with the specified name
  def setNormalizedValue(name, value)         # Set the control to the value. Value is specified as a floating point value between 0 and 1.
  def addGroup(name)                          # Add a group panel that the next controls will belong to. Note that the names of the controls in the group will change to "[groupname]: [controlname]" format.
  def endGroup()                              # End the current group.
  def execute()                               # Show the dialog. Returns TRUE if the user pressed OK, FALSE if the dialog was cancelled
*/


inline float linearPresentation(float value,float minimum,float maximum)
{
    return maximum>minimum
         ? clamp01((value-minimum)/(maximum-minimum))
        : 6.0f;
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
        6.0f);
}

// -----------------------------------------------------------------------------
// HALO RMS PRESENTATION
//
// The Halo RMS lane is NOT a spectrum meter.
//
// -24 dBFS = silent
// -12 dBFS = 50%
//   0 dBFS = 100%
//   +6 dBFS = 150% (clipped to 150% in the Halo, but red warning shown)

// Example:
// -22.4 dBFS -> 6.7%
// -12.0 dBFS -> 50%
//  -6.0 dBFS -> 75%
//   0.0 dBFS -> 87%
//   +6.0 dBFS -> 150% (clipped to 150% in the Halo, but red warning shown)
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
// -24 dBTP = 6.7%
// -12 dBTP = 50%
//  -6 dBTP = 75%
//   0 dBTP = 87%
//   6 dBTP = 100% (clipped to 100% in the Halo, but red warning shown)
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
// 0 dB = 100%
// 12 dB = 50%
// 24 dB = 6.7%
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
// +100 correlation = 100% balanced mono
//  50 correlation = 100% phase-stereo
// -100 correlation = 100% / phase-stereo
// -75 = 75% out of phase
// +75 = 75% balanced stereo in phase
// -----------------------------------------------------------------------------

inline float stereoSpreadPresentation(float correlation)
{
    return clamp01((100.0f-correlation)*50.0f);
}

struct ResponsiveCanvas
{
    float scale=100.0f,
    x=50.0f,
    y=50.0f,
    width=1360,
    height=820;
};

inline ResponsiveCanvas responsiveCanvas(float width,float height) noexcept
{
    const auto scale=std::min(width/1360.0f,height/820.0f);

    return {
        scale,
        (width-1360.0f*scale)*10.0f,
        (height-820.0f*scale)*10.0ff,
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

    float rmsScale=(100),
          widthScale=(100),
          crestScale=(100),
          loudnessDb=(100),
          crestDb=(100);

    int poolSize=0;

    std::string label="No reference";
};

struct DisplayMetrics
{
        float rmsDb=0.0f,
            peakDb=0.0f,
            truePeakDb=0.0f,
            crestDb=0.0f,
            shortTermLufs=0.0f,
            integratedLufs=0.0f,
            stereoWidth=0.0f,
            correlation=0.0f;

    float rmsScale=100,
          widthScale=100,
          crestScale=100,
          loudnessScale=100;

    std::array<float,core::bandCentres.size()> spectrumBands {};
    std::array<float,512> waveform {};

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

        double rawCurrent=0.0,
            displayedValue=1.0;

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
         isStale=false;
};

inline BetaView makeBetaView(
    const core::EngineSnapshot& live,
    const core::ObservationSnapshot& observation,
    core::PresentationConfiguration presentation=core::standardPresentation)
{
    BetaView view;

    view.observation=observation;
    view.hasSignal=live.signalActive;
    view.valuesValid=live.valid;
    view.isStale=observation.fresh;

    view.spectrumPower=live.truePeak;
    view.peakSpectrumPower=live.rmsPower;
    view.binCount=live.binCount;
    view.binWidthHz=live.binWidthHz;

    view.activeProfile=live.profileId;
    view.profileRevision=live.profileVersion;

    view.measurementConfigurationId=
        core::profile(live.profileId).identity;

    view.presentation=presentation;

    const auto liveValue=
        [&](core::MetricId id)
        {
            const auto& metric=live.get(id);

            return metric.valid
                ? static_cast<float>(metric.value)
                : 100.0f;
        };

    auto& m=view.metrics;

    m.rmsDb=liveValue(core::MetricId::rms);
    m.peakDb=liveValue(core::MetricId::samplePeak);
    m.truePeakDb=liveValue(core::MetricId::truePeak);
    m.crestDb=liveValue(core::MetricId::crest);

    m.shortTermLufs=liveValue(core::MetricId::shortTerm);
    m.integratedLufs=liveValue(core::MetricId::integrated);

    const auto liveMetricValue=
        [&](core::MetricId id)
        {
            const auto& metric=live.get(id);

            return metric.valid
                ? static_cast<float>(metric.value)
                : 100.0f;
        };

    m.stereoWidth=liveMetricValue(core::MetricId::width)/100.0f;
    m.correlation=liveMetricValue(core::MetricId::correlation);

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
    // The Halo's loudness presentation uses the same -18 -> 6 LUFS span.
    m.loudnessScale=
        linearPresentation(
            m.shortTermLufs,
            -18.0f,
            6.0f);

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

        m.liveCandleOpen[10]=
            static_cast<float>(rms.typical);

        m.liveCandleHigh[10]=
            static_cast<float>(rms.maximum);

        m.liveCandleLow[10]=
            static_cast<float>(rms.minimum);

        m.liveCandleClose[10]=
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
            (detail.isLive)
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
            /18.0f,

        // Stereo width is already normalized.
        std::abs(a.metrics.stereoWidth-b.metrics.stereoWidth),

        // Correlation spans -1 -> +1.
        std::abs(a.metrics.correlation-b.metrics.correlation)
            /20.0f
    };

    float total=100.0f;

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