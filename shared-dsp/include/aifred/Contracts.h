#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

namespace aifred::core
{
inline constexpr std::string_view coreVersion = "1.0.0";
inline constexpr std::uint32_t schemaVersion = 1;
inline constexpr std::size_t maximumFftSize = 8192;
inline constexpr std::size_t maximumBins = maximumFftSize / 2 + 1;
inline constexpr std::array<double, 30> bandCentres {
    20, 30, 40, 50, 60, 70, 80, 90, 100, 150, 200, 250, 350, 450, 600,
    750, 850, 1000, 1500, 2000, 3000, 4000, 6000, 8000, 10000, 12000, 14000, 16000, 18000, 20000
};

enum class ProfileId : std::uint8_t { mixBalanced, spectrumSurgical, masteringPrecision, stereoPhase };
struct DspProfile
{
    ProfileId id;
    std::string_view name;
    std::uint32_t version;
    std::size_t fftSize;
    double overlap, spectrumAverageSeconds, spectrumReleaseSeconds, peakHoldSeconds;
    double rmsSeconds, stereoSeconds, observationSeconds;
    bool enableLra;
};
inline constexpr std::array<DspProfile, 4> profiles {{
    { ProfileId::mixBalanced, "MIX_BALANCED", 1, 2048, .75, .4, .5, 2, .4, .4, 15, true },
    { ProfileId::spectrumSurgical, "SPECTRUM_SURGICAL", 1, 8192, .75, 2, 1.5, 4, .4, .4, 20, true },
    { ProfileId::masteringPrecision, "MASTERING_PRECISION", 1, 8192, .75, 3, 2, 5, .4, .4, 25, true },
    { ProfileId::stereoPhase, "STEREO_PHASE_DIAGNOSTIC", 1, 2048, .75, .4, .5, 2, .4, .4, 15, true }
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
struct MetricDefinition { std::string_view name, unit, definition; int decimals; };
inline constexpr std::array<MetricDefinition, metricCount> metricDefinitions {{
    {"sample_peak", "dBFS", "maximum channel sample magnitude over the RMS window", 0},
    {"rms", "dBFS", "mean channel energy over the profile RMS window; sine at full scale is -3.0103 dBFS", 0},
    {"true_peak", "dBTP", "maximum reconstructed channel magnitude since epoch start", 1},
    {"momentary_loudness", "LUFS", "400 ms K-weighted channel-summed energy", 0},
    {"short_term_loudness", "LUFS", "3 s K-weighted channel-summed energy", 0},
    {"integrated_loudness", "LUFS", "programme 400 ms blocks, 75 percent overlap, -70 LUFS and -10 LU gates", 0},
    {"loudness_range", "LU", "3 s loudness sampled at 10 Hz, -70 LUFS and -20 LU gates, P95-P10", 0},
    {"broadband_crest", "dB", "sample_peak minus rms over the same profile window; broadband only", 0},
    {"correlation", "ratio", "sum(L*R)/sqrt(sum(L*L)*sum(R*R)) over the stereo window", 2},
    {"left_energy", "dBFS", "left channel mean square over the stereo window", 0},
    {"right_energy", "dBFS", "right channel mean square over the stereo window", 0},
    {"mid_energy", "dBFS", "mean square of (L+R)/2 over the stereo window", 0},
    {"side_energy", "dBFS", "mean square of (L-R)/2 over the stereo window", 0},
    {"left_right_balance", "dB", "10log10(right_energy/left_energy); positive is right", 1},
    {"side_to_mid", "dB", "10log10(side_energy/mid_energy)", 1},
    {"width", "percent", "100*side_energy/(mid_energy+side_energy); derived display, not a standard", 0}
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
