#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <string_view>

namespace aifred::core
{
inline constexpr std::string_view coreVersion = "1.2.0";
inline constexpr std::uint32_t schemaVersion = 1;
inline constexpr std::uint32_t profileSchemaVersion = 2;
inline constexpr std::size_t maximumFftSize = 8192;
inline constexpr std::size_t maximumBins = maximumFftSize / 2 + 1;
inline constexpr std::array<double, 30> bandCentres {
    20, 30, 40, 50, 60, 70, 80, 90, 100, 150, 200, 250, 350, 450, 600,
    750, 850, 1000, 1500, 2000, 3000, 4000, 6000, 8000, 10000, 12000, 14000, 16000, 18000, 20000
};

struct MetricValue { double value = 0; bool valid = false; };
inline MetricValue powerDb(double power) noexcept
{
    return { power > 0 ? 10 * std::log10(power) : -std::numeric_limits<double>::infinity(), true };
}
enum class MetricId : std::uint8_t
{
    samplePeak, rms, truePeak, momentary, shortTerm, integrated, lra, crest,
    correlation, leftEnergy, rightEnergy, midEnergy, sideEnergy, balance, sideToMid, width, count
};
inline constexpr auto metricCount = static_cast<std::size_t>(MetricId::count);
enum class ProfileId : std::uint8_t { mixBalanced, spectrumSurgical, masteringPrecision, stereoPhase };
enum class SpectrumWindow : std::uint8_t { periodicHann };
enum class SpectrumDisplayRange : std::uint8_t { db120, db96, db72, db48 };
enum class SpectrumDrawing : std::uint8_t { lineAndFill, lineOnly };
enum class DisplayDensity : std::uint8_t { reduced, standard, detailed };
enum class MetricSource : std::uint8_t { observed, live };
enum class CpuCost : std::uint8_t { moderate, high };
enum class ReactionSpeed : std::uint8_t { fast, balanced, deliberate };

struct SpectrumMeasurementConfiguration
{
    std::size_t fftSize;
    double overlap, averageSeconds, releaseSeconds, peakHoldSeconds;
    SpectrumWindow window;
};
struct MeteringMeasurementConfiguration
{
    double rmsSeconds, stereoSeconds;
    double momentarySeconds, shortTermSeconds;
    bool truePeakEnabled, momentaryLoudnessEnabled, shortTermLoudnessEnabled, integratedLoudnessEnabled, lraEnabled;
};
struct ObservationConfiguration { double durationSeconds, freshnessSeconds; };
struct MeasurementConfiguration
{
    SpectrumMeasurementConfiguration spectrum;
    MeteringMeasurementConfiguration metering;
    ObservationConfiguration observation;
    double snapshotHz;
};
struct PresentationConfiguration
{
    SpectrumDisplayRange spectrumRange;
    float spectrumSmoothing;
    SpectrumDrawing spectrumDrawing;
    bool showPeakTrace;
    DisplayDensity gridDensity, labelDensity;
};
struct MetricPolicy
{
    std::uint32_t enabled, required;
    constexpr bool isEnabled(MetricId id) const noexcept { return (enabled & (1u << static_cast<unsigned>(id))) != 0; }
    constexpr bool isRequired(MetricId id) const noexcept { return (required & (1u << static_cast<unsigned>(id))) != 0; }
};
struct DspProfile
{
    ProfileId id;
    std::string_view name, identity, purpose;
    std::uint32_t version;
    MeasurementConfiguration measurement;
    PresentationConfiguration presentation;
    MetricPolicy metrics;
    CpuCost cpuCost;
    ReactionSpeed reactionSpeed;
};

inline constexpr std::uint32_t allMetrics = (1u << static_cast<unsigned>(MetricId::count)) - 1u;
constexpr std::uint32_t metricMask(std::initializer_list<MetricId> ids) noexcept
{
    std::uint32_t result=0; for(const auto id:ids) result|=1u<<static_cast<unsigned>(id); return result;
}
inline constexpr PresentationConfiguration standardPresentation {
    SpectrumDisplayRange::db96, .34f, SpectrumDrawing::lineAndFill, false,
    DisplayDensity::standard, DisplayDensity::standard
};
inline constexpr std::array<DspProfile, 4> profiles {{
    { ProfileId::mixBalanced, "MIX_BALANCED", "MIX_BALANCED.r1", "general-purpose mix analysis", 1,
      {{2048,.75,.4,.5,2,SpectrumWindow::periodicHann},{.4,.4,.4,3,true,true,true,true,true},{15,1},10},
      standardPresentation,
      {allMetrics,metricMask({MetricId::samplePeak,MetricId::rms,MetricId::truePeak,MetricId::momentary,MetricId::shortTerm,MetricId::integrated,MetricId::crest,MetricId::correlation})},
      CpuCost::moderate,ReactionSpeed::balanced },
    { ProfileId::spectrumSurgical, "SPECTRUM_SURGICAL", "SPECTRUM_SURGICAL.r1", "high-resolution frequency-domain inspection", 1,
      {{8192,.75,2,1.5,4,SpectrumWindow::periodicHann},{.4,.4,.4,3,true,true,true,true,true},{20,1},10},
      {SpectrumDisplayRange::db96,.24f,SpectrumDrawing::lineAndFill,true,DisplayDensity::detailed,DisplayDensity::detailed},
      {allMetrics,metricMask({MetricId::samplePeak,MetricId::rms,MetricId::truePeak,MetricId::shortTerm,MetricId::correlation})},
      CpuCost::high,ReactionSpeed::deliberate },
    { ProfileId::masteringPrecision, "MASTERING_PRECISION", "MASTERING_PRECISION.r1", "mastering and final-stage metering", 1,
      {{8192,.75,3,2,5,SpectrumWindow::periodicHann},{.4,.4,.4,3,true,true,true,true,true},{25,1},10},
      {SpectrumDisplayRange::db96,.28f,SpectrumDrawing::lineAndFill,true,DisplayDensity::detailed,DisplayDensity::standard},
      {allMetrics,metricMask({MetricId::samplePeak,MetricId::rms,MetricId::truePeak,MetricId::momentary,MetricId::shortTerm,MetricId::integrated,MetricId::lra,MetricId::crest,MetricId::correlation})},
      CpuCost::high,ReactionSpeed::deliberate },
    { ProfileId::stereoPhase, "STEREO_PHASE_DIAGNOSTIC", "STEREO_PHASE_DIAGNOSTIC.r2", "fast stereo and phase diagnosis", 2,
      {{2048,.75,.4,.5,2,SpectrumWindow::periodicHann},{.4,.1,.4,3,true,true,true,true,true},{15,1},10},
      standardPresentation,
      {allMetrics,metricMask({MetricId::correlation,MetricId::leftEnergy,MetricId::rightEnergy,MetricId::midEnergy,MetricId::sideEnergy,MetricId::balance,MetricId::sideToMid,MetricId::width})},
      CpuCost::moderate,ReactionSpeed::fast }
}};
inline const DspProfile& profile(ProfileId id) noexcept
{
    const auto i = static_cast<std::size_t>(id);
    return profiles[i < profiles.size() ? i : 0];
}
inline ProfileId profileFromName(std::string_view name) noexcept
{
    for (const auto& p : profiles) if (p.name == name) return p.id;
    return ProfileId::mixBalanced;
}
inline constexpr double spectrumFloorDb(SpectrumDisplayRange range) noexcept
{
    constexpr std::array<double,4> floors {-120,-96,-72,-48};
    const auto i=static_cast<std::size_t>(range); return floors[i<floors.size()?i:1];
}
inline constexpr std::string_view spectrumRangeName(SpectrumDisplayRange range) noexcept
{
    constexpr std::array<std::string_view,4> names {"-120_TO_0_DBFS","-96_TO_0_DBFS","-72_TO_0_DBFS","-48_TO_0_DBFS"};
    const auto i=static_cast<std::size_t>(range); return names[i<names.size()?i:1];
}
inline SpectrumDisplayRange spectrumRangeFromName(std::string_view name) noexcept
{
    for(const auto range:{SpectrumDisplayRange::db120,SpectrumDisplayRange::db96,SpectrumDisplayRange::db72,SpectrumDisplayRange::db48})
        if(spectrumRangeName(range)==name)return range;
    return SpectrumDisplayRange::db96;
}
inline constexpr std::string_view spectrumWindowName(SpectrumWindow window) noexcept
{
    return window==SpectrumWindow::periodicHann?"PERIODIC_HANN":"UNKNOWN";
}

struct MetricDefinition { std::string_view name, displayName, unit, definition; int decimals; MetricSource source; ProfileId emphasizedBy; };
inline constexpr std::array<MetricDefinition, metricCount> metricDefinitions {{
    {"sample_peak","Sample Peak","dBFS","maximum channel sample magnitude over the RMS window",0,MetricSource::observed,ProfileId::mixBalanced},
    {"rms","RMS","dBFS","mean channel energy over the profile RMS window; sine at full scale is -3.0103 dBFS",0,MetricSource::observed,ProfileId::mixBalanced},
    {"true_peak","True Peak","dBTP","maximum reconstructed channel magnitude since epoch start",1,MetricSource::observed,ProfileId::masteringPrecision},
    {"momentary_loudness","Momentary Loudness","LUFS","400 ms K-weighted channel-summed energy",0,MetricSource::observed,ProfileId::masteringPrecision},
    {"short_term_loudness","Short-Term Loudness","LUFS","3 s K-weighted channel-summed energy",0,MetricSource::observed,ProfileId::masteringPrecision},
    {"integrated_loudness","Integrated Loudness","LUFS","programme 400 ms blocks, 75 percent overlap, -70 LUFS and -10 LU gates",0,MetricSource::observed,ProfileId::masteringPrecision},
    {"loudness_range","Loudness Range","LU","3 s loudness sampled at 10 Hz, -70 LUFS and -20 LU gates, P95-P10",0,MetricSource::observed,ProfileId::masteringPrecision},
    {"broadband_crest","Crest","dB","sample_peak minus rms over the same profile window; broadband only",0,MetricSource::observed,ProfileId::mixBalanced},
    {"correlation","Correlation","ratio","sum(L*R)/sqrt(sum(L*L)*sum(R*R)) over the stereo window",2,MetricSource::live,ProfileId::stereoPhase},
    {"left_energy","Left Energy","dBFS","left channel mean square over the stereo window",0,MetricSource::observed,ProfileId::stereoPhase},
    {"right_energy","Right Energy","dBFS","right channel mean square over the stereo window",0,MetricSource::observed,ProfileId::stereoPhase},
    {"mid_energy","Mid Energy","dBFS","mean square of (L+R)/2 over the stereo window",0,MetricSource::observed,ProfileId::stereoPhase},
    {"side_energy","Side Energy","dBFS","mean square of (L-R)/2 over the stereo window",0,MetricSource::observed,ProfileId::stereoPhase},
    {"left_right_balance","Left/Right Balance","dB","10log10(right_energy/left_energy); positive is right",1,MetricSource::observed,ProfileId::stereoPhase},
    {"side_to_mid","Side/Mid","dB","10log10(side_energy/mid_energy)",1,MetricSource::observed,ProfileId::stereoPhase},
    {"width","Width","percent","100*side_energy/(mid_energy+side_energy); derived display, not a standard",0,MetricSource::live,ProfileId::stereoPhase}
}};
inline constexpr std::size_t index(MetricId id) noexcept { return static_cast<std::size_t>(id); }

struct EngineSnapshot
{
    std::uint32_t schema = schemaVersion, profileVersion = 1;
    ProfileId profileId = ProfileId::mixBalanced;
    std::uint64_t sequence = 0, epoch = 0, sampleStart = 0, sampleEnd = 0, droppedPublications = 0;
    double sampleRate = 0;
    bool valid = false, signalActive = false, transportKnown = false, transportPlaying = false;
    bool sampleClipActive = false;
    std::uint64_t sampleClipCount = 0;
    std::array<MetricValue, metricCount> metrics {};
    std::array<MetricValue, 2> channelPeaks {};
    std::size_t fftSize = 0, binCount = 0;
    double binWidthHz = 0;
    std::uint64_t spectrumSequence = 0;
    // One-sided mean-channel power per FFT bin, normalized by N*sum(window^2).
    std::array<double, maximumBins> spectrumPower {}, averagePower {}, peakPower {};
    std::array<MetricValue, 30> bands {};
    std::array<std::array<float, 2>, 96> vectorscope {};
    std::size_t vectorscopeCount = 0;
    const MetricValue& get(MetricId id) const noexcept { return metrics[index(id)]; }
    MetricValue& get(MetricId id) noexcept { return metrics[index(id)]; }
};
}
