#include "AnalysisEngine.h"

#include <algorithm>
#include <cmath>

namespace aifred {
namespace {

float safeSqrt(float value) {
  return std::sqrt(std::max(value, 0.0f));
}

float smooth(float previous, float next, float amount) {
  return previous * amount + next * (1.0f - amount);
}

float confidenceWeight(Confidence confidence) {
  switch (confidence) {
    case Confidence::High: return 1.0f;
    case Confidence::Moderate: return 0.68f;
    case Confidence::Low: return 0.38f;
  }
  return 0.38f;
}

} // namespace

void AnalysisEngine::prepare(double sampleRate) {
  sampleRate_ = sampleRate > 0.0 ? sampleRate : 44100.0;
  reset();
}

void AnalysisEngine::reset() {
  live_ = {};
  smoothed_ = {};
}

float AnalysisEngine::linearToDb(float value) {
  return 20.0f * std::log10(std::max(value, 0.0000316f));
}

float AnalysisEngine::deviationOutsideCorridor(float value, float target, float tolerance, float criticalRange) {
  const auto miss = std::max(std::abs(value - target) - tolerance, 0.0f);
  const auto normalized = miss / std::max(criticalRange, 0.0001f);
  return clamp01(normalized * normalized * (3.0f - 2.0f * clamp01(normalized)));
}

DomainAlignment AnalysisEngine::makeDomain(float error01, float primary, float secondary, std::string summary, float confidenceSeed) {
  DomainAlignment domain;
  domain.error01 = clamp01(error01);
  domain.alignment01 = 1.0f - domain.error01;
  domain.severity = severityForError(domain.error01);
  domain.confidence = confidenceForPersistence(confidenceSeed);
  domain.rawPrimaryMetric = primary;
  domain.rawSecondaryMetric = secondary;
  domain.summary = std::move(summary);
  return domain;
}

void AnalysisEngine::pushAudioBlock(const juce::AudioBuffer<float>& buffer) {
  const auto channels = buffer.getNumChannels();
  const auto samples = buffer.getNumSamples();
  if (channels == 0 || samples == 0) return;

  double sumSquares = 0.0;
  double peak = 0.0;
  double leftEnergy = 0.0;
  double rightEnergy = 0.0;
  double midEnergy = 0.0;
  double sideEnergy = 0.0;
  double corrNum = 0.0;
  double transient = 0.0;
  double lowMotion = 0.0;
  double highMotion = 0.0;
  float prevMono = 0.0f;
  float slowMono = 0.0f;

  const auto* left = buffer.getReadPointer(0);
  const auto* right = channels > 1 ? buffer.getReadPointer(1) : left;

  for (int i = 0; i < samples; ++i) {
    const auto l = left[i];
    const auto r = right[i];
    const auto mono = 0.5f * (l + r);
    const auto side = 0.5f * (l - r);
    sumSquares += 0.5 * (static_cast<double>(l) * l + static_cast<double>(r) * r);
    leftEnergy += static_cast<double>(l) * l;
    rightEnergy += static_cast<double>(r) * r;
    midEnergy += static_cast<double>(mono) * mono;
    sideEnergy += static_cast<double>(side) * side;
    corrNum += static_cast<double>(l) * r;
    peak = std::max({peak, std::abs(static_cast<double>(l)), std::abs(static_cast<double>(r))});
    transient += std::abs(mono - prevMono);
    prevMono = mono;

    slowMono = 0.985f * slowMono + 0.015f * mono;
    lowMotion += std::abs(slowMono);
    highMotion += std::abs(mono - slowMono);
  }

  const auto sampleCount = std::max(samples, 1);
  const auto rms = safeSqrt(static_cast<float>(sumSquares / sampleCount));
  const auto leftRms = safeSqrt(static_cast<float>(leftEnergy / sampleCount));
  const auto rightRms = safeSqrt(static_cast<float>(rightEnergy / sampleCount));
  const auto denom = std::max(leftRms * rightRms * static_cast<float>(sampleCount), 0.000001f);
  const auto correlation = std::clamp(static_cast<float>(corrNum) / denom, -1.0f, 1.0f);
  const auto width = clamp01(safeSqrt(static_cast<float>(sideEnergy / std::max(midEnergy + sideEnergy, 0.000001))));
  const auto crest = linearToDb(static_cast<float>(peak)) - linearToDb(rms);
  const auto tilt = clamp01(static_cast<float>(highMotion / std::max(lowMotion + highMotion, 0.000001)));

  live_.rmsDb = linearToDb(rms);
  live_.peakDb = linearToDb(static_cast<float>(peak));
  live_.crestDb = std::clamp(crest, 0.0f, 24.0f);
  live_.stereoWidth = width;
  live_.correlation = correlation;
  live_.spectralTilt = tilt;
  live_.transientDensity = clamp01(static_cast<float>(transient / sampleCount) * 9.0f);
  live_.signal01 = clamp01((live_.rmsDb + 60.0f) / 48.0f);

  const auto amount = live_.signal01 > 0.02f ? smoothing_ : 0.98f;
  smoothed_.rmsDb = smooth(smoothed_.rmsDb, live_.rmsDb, amount);
  smoothed_.peakDb = smooth(smoothed_.peakDb, live_.peakDb, amount);
  smoothed_.crestDb = smooth(smoothed_.crestDb, live_.crestDb, amount);
  smoothed_.stereoWidth = smooth(smoothed_.stereoWidth, live_.stereoWidth, amount);
  smoothed_.correlation = smooth(smoothed_.correlation, live_.correlation, amount);
  smoothed_.spectralTilt = smooth(smoothed_.spectralTilt, live_.spectralTilt, amount);
  smoothed_.transientDensity = smooth(smoothed_.transientDensity, live_.transientDensity, amount);
  smoothed_.signal01 = smooth(smoothed_.signal01, live_.signal01, amount);
}

HaloState AnalysisEngine::snapshot() const {
  return buildHaloState();
}

HaloState AnalysisEngine::buildHaloState() const {
  HaloState state;
  const auto confidence = clamp01(smoothed_.signal01 * 1.35f);

  const auto toneError = deviationOutsideCorridor(smoothed_.spectralTilt, 0.48f, 0.16f, 0.36f);
  const auto loudnessError = 0.55f * deviationOutsideCorridor(smoothed_.rmsDb, -16.0f, 5.0f, 18.0f)
                           + 0.45f * deviationOutsideCorridor(smoothed_.peakDb, -1.0f, 1.5f, 10.0f);
  const auto dynamicsError = 0.58f * deviationOutsideCorridor(smoothed_.crestDb, 10.0f, 4.0f, 12.0f)
                           + 0.42f * deviationOutsideCorridor(smoothed_.transientDensity, 0.42f, 0.25f, 0.65f);
  const auto stereoWidthError = deviationOutsideCorridor(smoothed_.stereoWidth, 0.52f, 0.24f, 0.5f);
  const auto phaseError = smoothed_.correlation < 0.0f ? clamp01(-smoothed_.correlation) : 0.0f;
  const auto stereoError = 0.72f * stereoWidthError + 0.28f * phaseError;

  state.tone = makeDomain(toneError, smoothed_.spectralTilt, smoothed_.rmsDb, "Spectral shape against a balanced production corridor.", confidence);
  state.stereo = makeDomain(stereoError, smoothed_.stereoWidth, smoothed_.correlation, "Width and mono safety above the protected low end.", confidence);
  state.loudness = makeDomain(loudnessError, smoothed_.rmsDb, smoothed_.peakDb, "Working loudness and headroom relative to a mix-stage target.", confidence);
  state.dynamics = makeDomain(dynamicsError, smoothed_.crestDb, smoothed_.transientDensity, "Crest, transient motion, and controlled punch.", confidence);

  const auto totalError = 0.35f * state.tone.error01 + 0.25f * state.stereo.error01
                        + 0.20f * state.loudness.error01 + 0.20f * state.dynamics.error01;
  state.totalAlignment01 = 1.0f - clamp01(totalError);

  auto addFix = [&](Domain domain, const char* title, const char* cause, const char* nextStep, const DomainAlignment& alignment) {
    FixItem item;
    item.title = title;
    item.cause = cause;
    item.nextStep = nextStep;
    item.severity = alignment.severity;
    item.confidence = alignment.confidence;
    item.impactDomain = domain;
    item.priority = alignment.error01 * confidenceWeight(alignment.confidence);
    state.rankedFixes.push_back(std::move(item));
  };

  if (state.stereo.error01 > 0.18f) addFix(Domain::Stereo, "Stereo Image Needs Shape", "Width and mono safety are outside the reference corridor.", "Widen upper detail or narrow unstable side content.", state.stereo);
  if (state.tone.error01 > 0.18f) addFix(Domain::Tone, "Tone Balance Drift", "The spectral shape is leaning away from the target corridor.", "Trim the dominant buildup before adding more top or sub.", state.tone);
  if (state.loudness.error01 > 0.18f) addFix(Domain::Loudness, "Headroom Relationship Off", "Level and peak behavior are not sitting in the mix-stage pocket.", "Set the mix bus for cleaner headroom before final limiting.", state.loudness);
  if (state.dynamics.error01 > 0.18f) addFix(Domain::Dynamics, "Punch Needs Rebalancing", "Crest and transient motion are outside the controlled range.", "Ease compression or restore attack on the sources carrying movement.", state.dynamics);

  std::sort(state.rankedFixes.begin(), state.rankedFixes.end(), [](const auto& a, const auto& b) {
    return a.priority > b.priority;
  });

  if (smoothed_.signal01 < 0.02f) {
    state.primaryTitle = "Waiting For Signal";
    state.primaryCause = "Route audio through Aifred to start the Halo read.";
    state.rankedFixes.clear();
    return state;
  }

  if (state.totalAlignment01 >= 0.82f) {
    state.primaryTitle = "Close To Reference";
    state.primaryCause = "Tone, stereo, loudness, and dynamics are staying inside the working corridor.";
    state.primarySeverity = Severity::Minor;
    state.primaryConfidence = Confidence::High;
    return state;
  }

  if (!state.rankedFixes.empty()) {
    const auto& top = state.rankedFixes.front();
    state.primaryTitle = top.title;
    state.primaryCause = top.cause;
    state.primarySeverity = top.severity;
    state.primaryConfidence = top.confidence;
  }

  return state;
}

} // namespace aifred
