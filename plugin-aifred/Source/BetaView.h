#pragma once

#include "aifred/Pipeline.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace aifred
{
enum class AnalysisMode { Analyze, Reference, Compare };
enum class Domain { Tone, Stereo, Loudness, Dynamics };

inline float clamp01(float value) noexcept
{
    return std::clamp(value, 0.0f, 1.0f);
}

// Presentation ranges only. DSP values retain their physical units.
inline constexpr float crestFloorDb = 0.0f;
inline constexpr float crestCeilingDb = 24.0f;
inline constexpr float haloRmsFloorDbfs = -24.0f;
inline constexpr float haloRmsCeilingDbfs = 0.0f;
inline constexpr float truePeakFloorDbtp = -24.0f;
inline constexpr float truePeakCeilingDbtp = 0.0f;
inline constexpr float unavailableMetric = std::numeric_limits<float>::quiet_NaN();

inline float linearPresentation(float value, float minimum, float maximum) noexcept
{
    return maximum > minimum && std::isfinite(value)
        ? clamp01((value - minimum) / (maximum - minimum))
        : 0.0f;
}

inline float rmsPresentation(float value, core::SpectrumDisplayRange range) noexcept
{
    return linearPresentation(value, static_cast<float>(core::spectrumFloorDb(range)), 0.0f);
}

inline float haloRmsPresentation(float value) noexcept
{
    return linearPresentation(value, haloRmsFloorDbfs, haloRmsCeilingDbfs);
}

inline float truePeakPresentation(float value) noexcept
{
    return linearPresentation(value, truePeakFloorDbtp, truePeakCeilingDbtp);
}

inline float crestPresentation(float value) noexcept
{
    return linearPresentation(value, crestFloorDb, crestCeilingDb);
}

// Correlation is a physical ratio in [-1,+1]; spread is its UI inverse.
inline float stereoSpreadPresentation(float correlation) noexcept
{
    return std::isfinite(correlation) ? clamp01((1.0f - correlation) * 0.5f) : 0.0f;
}

struct ResponsiveCanvas
{
    float scale = 1.0f, x = 0.0f, y = 0.0f, width = 1360.0f, height = 820.0f;
};

inline ResponsiveCanvas responsiveCanvas(float width, float height) noexcept
{
    const auto scale = std::max(0.0f, std::min(width / 1360.0f, height / 820.0f));
    return { scale,
             (width - 1360.0f * scale) * 0.5f,
             (height - 820.0f * scale) * 0.5f,
             1360.0f * scale,
             820.0f * scale };
}

inline bool modalShouldDismiss(float x, float y, float width, float height,
                               float pointX, float pointY) noexcept
{
    return pointX < x || pointY < y || pointX > x + width || pointY > y + height;
}

struct ReferenceTarget
{
    core::ReferenceDistribution distribution;
    float rmsScale = 0.0f, widthScale = 0.0f, crestScale = 0.0f;
    float loudnessDb = unavailableMetric, crestDb = unavailableMetric;
    int poolSize = 0;
    std::string label = "No reference";
};

struct DisplayMetrics
{
    float rmsDb = unavailableMetric, peakDb = unavailableMetric;
    float truePeakDb = unavailableMetric, crestDb = unavailableMetric;
    float shortTermLufs = unavailableMetric, integratedLufs = unavailableMetric;
    float stereoWidth = unavailableMetric, correlation = unavailableMetric;
    float rmsScale = 0.0f, widthScale = 0.0f, crestScale = 0.0f, loudnessScale = 0.0f;
    std::array<float, core::bandCentres.size()> spectrumBands {};
    std::array<float, 96> waveform {};
    std::array<float, 10> sessionCandleOpen {}, sessionCandleHigh {}, sessionCandleLow {}, sessionCandleClose {};
    std::array<float, 10> minuteCandleOpen {}, minuteCandleHigh {}, minuteCandleLow {}, minuteCandleClose {};
    std::array<float, 10> liveCandleOpen {}, liveCandleHigh {}, liveCandleLow {}, liveCandleClose {};
    int sessionCandleCount = 0, minuteCandleCount = 0, liveCandleCount = 0;
};

struct MetricDetail
{
    core::MetricId id = core::MetricId::samplePeak;
    std::string_view displayName, unit;
    bool valid = false, isLive = false;
    double rawCurrent = std::numeric_limits<double>::quiet_NaN();
    double displayedValue = std::numeric_limits<double>::quiet_NaN();
    core::MetricObservation observed;
    core::ProfileId activeProfile = core::ProfileId::mixBalanced;
    core::ProfileId emphasizedBy = core::ProfileId::mixBalanced;
    std::uint32_t profileRevision = 1;
};

struct BetaView
{
    AnalysisMode mode = AnalysisMode::Analyze;
    DisplayMetrics metrics;
    ReferenceTarget reference;
    core::ObservationSnapshot observation;
    std::array<double, core::maximumBins> spectrumPower {};
    std::array<double, core::maximumBins> peakSpectrumPower {};
    std::array<MetricDetail, core::metricCount> metricDetails {};
    std::array<bool, core::metricCount> liveMetricValid {};
    std::size_t binCount = 0;
    double binWidthHz = 0.0;
    core::ProfileId activeProfile = core::ProfileId::mixBalanced;
    std::uint32_t profileRevision = 1;
    std::string_view measurementConfigurationId = "MIX_BALANCED.r1";
    core::PresentationConfiguration presentation = core::standardPresentation;
    bool hasSignal = false, hasReference = false, valuesValid = false, isStale = true;
};

inline BetaView makeBetaView(const core::EngineSnapshot& live,
                             const core::ObservationSnapshot& observation,
                             core::PresentationConfiguration presentation = core::standardPresentation)
{
    BetaView view;
    view.observation = observation;
    view.hasSignal = live.signalActive;
    view.valuesValid = live.valid;
    view.isStale = !observation.fresh;
    view.spectrumPower = live.averagePower;
    view.peakSpectrumPower = live.peakPower;
    view.binCount = live.binCount;
    view.binWidthHz = live.binWidthHz;
    view.activeProfile = live.profileId;
    view.profileRevision = live.profileVersion;
    view.measurementConfigurationId = core::profile(live.profileId).identity;
    view.presentation = presentation;

    const auto liveValue = [&](core::MetricId id) {
        const auto& metric = live.get(id);
        view.liveMetricValid[core::index(id)] = metric.valid;
        return metric.valid ? static_cast<float>(metric.value) : unavailableMetric;
    };

    auto& metrics = view.metrics;
    metrics.rmsDb = liveValue(core::MetricId::rms);
    metrics.peakDb = liveValue(core::MetricId::samplePeak);
    metrics.truePeakDb = liveValue(core::MetricId::truePeak);
    metrics.crestDb = liveValue(core::MetricId::crest);
    metrics.shortTermLufs = liveValue(core::MetricId::shortTerm);
    metrics.integratedLufs = liveValue(core::MetricId::integrated);
    const auto widthPercent = liveValue(core::MetricId::width);
    metrics.stereoWidth = std::isfinite(widthPercent) ? widthPercent / 100.0f : unavailableMetric;
    metrics.correlation = liveValue(core::MetricId::correlation);

    metrics.rmsScale = haloRmsPresentation(metrics.rmsDb);
    metrics.widthScale = stereoSpreadPresentation(metrics.correlation);
    metrics.crestScale = crestPresentation(metrics.crestDb);
    metrics.loudnessScale = linearPresentation(metrics.shortTermLufs, -24.0f, 0.0f);

    const auto spectrumFloor = static_cast<float>(core::spectrumFloorDb(presentation.spectrumRange));
    for (std::size_t i = 0; i < metrics.spectrumBands.size(); ++i)
    {
        const auto& band = observation.bands[i];
        metrics.spectrumBands[i] = band.valid
            ? linearPresentation(static_cast<float>(band.typical), spectrumFloor, 0.0f)
            : 0.0f;
    }
    for (std::size_t i = 0; i < std::min(live.vectorscopeCount, metrics.waveform.size()); ++i)
        metrics.waveform[i] = live.vectorscope[i][0];

    for (std::size_t i = 0; i < core::metricCount; ++i)
    {
        auto& detail = view.metricDetails[i];
        const auto& definition = core::metricDefinitions[i];
        detail.id = static_cast<core::MetricId>(i);
        detail.displayName = definition.displayName;
        detail.unit = definition.unit;
        detail.isLive = definition.source == core::MetricSource::live;
        detail.rawCurrent = live.metrics[i].valid
            ? live.metrics[i].value : std::numeric_limits<double>::quiet_NaN();
        detail.observed = observation.metrics[i];
        detail.activeProfile = live.profileId;
        detail.profileRevision = live.profileVersion;
        detail.emphasizedBy = definition.emphasizedBy;
        const auto shown = detail.isLive
            ? live.metrics[i]
            : core::MetricValue { detail.observed.typical, detail.observed.valid };
        detail.valid = shown.valid;
        detail.displayedValue = shown.valid
            ? shown.value : std::numeric_limits<double>::quiet_NaN();
    }
    return view;
}

inline float compareDelta(float mixA, float mixB) noexcept { return mixA - mixB; }

inline float compareSimilarity(const BetaView& a, const BetaView& b) noexcept
{
    float total = 0.0f;
    std::size_t count = 0;
    const auto add = [&](core::MetricId id, float difference) {
        if (a.liveMetricValid[core::index(id)] && b.liveMetricValid[core::index(id)]
            && std::isfinite(difference))
        {
            total += clamp01(difference);
            ++count;
        }
    };
    add(core::MetricId::rms, std::abs(a.metrics.rmsDb - b.metrics.rmsDb) / 24.0f);
    add(core::MetricId::truePeak, std::abs(a.metrics.truePeakDb - b.metrics.truePeakDb) / 24.0f);
    add(core::MetricId::crest, std::abs(a.metrics.crestDb - b.metrics.crestDb) / 24.0f);
    add(core::MetricId::shortTerm, std::abs(a.metrics.shortTermLufs - b.metrics.shortTermLufs) / 24.0f);
    add(core::MetricId::width, std::abs(a.metrics.stereoWidth - b.metrics.stereoWidth));
    add(core::MetricId::correlation, std::abs(a.metrics.correlation - b.metrics.correlation) / 2.0f);
    return count == 0 ? 0.0f : 100.0f * (1.0f - total / static_cast<float>(count));
}

inline void applyCandleHistory(BetaView& view, const core::CandleHistorySnapshot& history)
{
    view.metrics.sessionCandleOpen = history.sessionOpen;
    view.metrics.sessionCandleHigh = history.sessionHigh;
    view.metrics.sessionCandleLow = history.sessionLow;
    view.metrics.sessionCandleClose = history.sessionClose;
    view.metrics.minuteCandleOpen = history.minuteOpen;
    view.metrics.minuteCandleHigh = history.minuteHigh;
    view.metrics.minuteCandleLow = history.minuteLow;
    view.metrics.minuteCandleClose = history.minuteClose;
    view.metrics.liveCandleOpen = history.liveOpen;
    view.metrics.liveCandleHigh = history.liveHigh;
    view.metrics.liveCandleLow = history.liveLow;
    view.metrics.liveCandleClose = history.liveClose;
    view.metrics.sessionCandleCount = std::clamp(history.sessionCount, 0, 10);
    view.metrics.minuteCandleCount = std::clamp(history.minuteCount, 0, 10);
    view.metrics.liveCandleCount = std::clamp(history.liveCount, 0, 10);
}
}
