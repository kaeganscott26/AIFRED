#include "AnalysisEngine.h"

#include <cmath>

namespace aifred {
namespace {

float safeSqrt(float value) {
  return std::sqrt(std::max(value, 0.0f));
}

float smooth(float previous, float next, float amount) {
  return previous * amount + next * (1.0f - amount);
}

float toLoudness01(float rmsDb) {
  return clamp01((rmsDb + 42.0f) / 34.0f);
}

} // namespace

void AnalysisEngine::prepare(double sampleRate) {
  sampleRate_ = sampleRate > 0.0 ? sampleRate : 44100.0;
  reset();
}

void AnalysisEngine::reset() {
  live_ = {};
  smoothed_ = {};
  sessionCandles_ = {};
  minuteCandles_ = {};
  sessionWindowSamples_ = 0.0;
  minuteWindowSamples_ = 0.0;
  sessionCandleWrite_ = 0;
  minuteCandleWrite_ = 0;
  sessionCandleCount_ = 0;
  minuteCandleCount_ = 0;
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

void AnalysisEngine::updateCandle(CandleFrame& candle, float value) {
  value = clamp01(value);
  if (!candle.active) {
    candle.open = value;
    candle.high = value;
    candle.low = value;
    candle.close = value;
    candle.active = true;
    return;
  }
  candle.high = std::max(candle.high, value);
  candle.low = std::min(candle.low, value);
  candle.close = value;
}

void AnalysisEngine::commitCandle(std::array<CandleFrame, 10>& candles, int& writeIndex, int& count) {
  auto& candle = candles[static_cast<size_t>(writeIndex)];
  candle.active = false;
  writeIndex = (writeIndex + 1) % static_cast<int>(candles.size());
  count = std::min(static_cast<int>(candles.size()), count + 1);
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
  float lp0 = 0.0f;
  float lp1 = 0.0f;
  float lp2 = 0.0f;
  float lp3 = 0.0f;
  float lp4 = 0.0f;
  std::array<double, 8> spectrum {};

  const auto* left = buffer.getReadPointer(0);
  const auto* right = channels > 1 ? buffer.getReadPointer(1) : left;

  for (int i = 0; i < samples; ++i) {
    const auto l = left[i];
    const auto r = right[i];
    const auto mono = 0.5f * (l + r);
    const auto side = 0.5f * (l - r);
    const auto monoDelta = mono - prevMono;
    sumSquares += 0.5 * (static_cast<double>(l) * l + static_cast<double>(r) * r);
    leftEnergy += static_cast<double>(l) * l;
    rightEnergy += static_cast<double>(r) * r;
    midEnergy += static_cast<double>(mono) * mono;
    sideEnergy += static_cast<double>(side) * side;
    corrNum += static_cast<double>(l) * r;
    peak = std::max({peak, std::abs(static_cast<double>(l)), std::abs(static_cast<double>(r))});
    transient += std::abs(monoDelta);
    prevMono = mono;

    slowMono = 0.985f * slowMono + 0.015f * mono;
    lowMotion += std::abs(slowMono);
    highMotion += std::abs(mono - slowMono);

    lp0 += 0.0025f * (mono - lp0);
    lp1 += 0.0075f * (mono - lp1);
    lp2 += 0.0220f * (mono - lp2);
    lp3 += 0.0640f * (mono - lp3);
    lp4 += 0.1600f * (mono - lp4);
    const std::array<float, 8> bands {
      lp0,
      lp1 - lp0,
      lp2 - lp1,
      lp3 - lp2,
      lp4 - lp3,
      mono - lp4,
      side,
      monoDelta
    };
    for (size_t band = 0; band < bands.size(); ++band) {
      spectrum[band] += static_cast<double>(bands[band]) * bands[band];
    }
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
  double spectrumSum = 0.000001;
  for (const auto band : spectrum) spectrumSum += safeSqrt(static_cast<float>(band / sampleCount));
  for (size_t band = 0; band < live_.spectrumBands.size(); ++band) {
    const auto energy = safeSqrt(static_cast<float>(spectrum[band] / sampleCount));
    live_.spectrumBands[band] = clamp01(static_cast<float>(energy / spectrumSum) * 3.35f);
  }
  live_.signal01 = clamp01((live_.rmsDb + 60.0f) / 48.0f);
  live_.candleOpen = clamp01((smoothed_.rmsDb + 42.0f) / 34.0f);
  live_.candleClose = toLoudness01(live_.rmsDb);
  live_.candleHigh = std::max(live_.candleOpen, clamp01((live_.peakDb + 18.0f) / 18.0f));
  live_.candleLow = std::min(live_.candleOpen, clamp01((live_.rmsDb - live_.crestDb + 42.0f) / 34.0f));

  const auto amount = live_.signal01 > 0.02f ? smoothing_ : 0.98f;
  smoothed_.rmsDb = smooth(smoothed_.rmsDb, live_.rmsDb, amount);
  smoothed_.peakDb = smooth(smoothed_.peakDb, live_.peakDb, amount);
  smoothed_.crestDb = smooth(smoothed_.crestDb, live_.crestDb, amount);
  smoothed_.stereoWidth = smooth(smoothed_.stereoWidth, live_.stereoWidth, amount);
  smoothed_.correlation = smooth(smoothed_.correlation, live_.correlation, amount);
  smoothed_.spectralTilt = smooth(smoothed_.spectralTilt, live_.spectralTilt, amount);
  smoothed_.transientDensity = smooth(smoothed_.transientDensity, live_.transientDensity, amount);
  for (size_t band = 0; band < smoothed_.spectrumBands.size(); ++band) {
    smoothed_.spectrumBands[band] = smooth(smoothed_.spectrumBands[band], live_.spectrumBands[band], amount);
  }
  smoothed_.signal01 = smooth(smoothed_.signal01, live_.signal01, amount);
  smoothed_.candleOpen = smooth(smoothed_.candleOpen, live_.candleOpen, amount);
  smoothed_.candleHigh = smooth(smoothed_.candleHigh, live_.candleHigh, amount);
  smoothed_.candleLow = smooth(smoothed_.candleLow, live_.candleLow, amount);
  smoothed_.candleClose = smooth(smoothed_.candleClose, live_.candleClose, amount);

  updateCandle(sessionCandles_[static_cast<size_t>(sessionCandleWrite_)], live_.candleClose);
  updateCandle(minuteCandles_[static_cast<size_t>(minuteCandleWrite_)], live_.candleClose);
  sessionWindowSamples_ += samples;
  minuteWindowSamples_ += samples;
  if (sessionWindowSamples_ >= sampleRate_ * 0.75) {
    sessionWindowSamples_ = 0.0;
    commitCandle(sessionCandles_, sessionCandleWrite_, sessionCandleCount_);
  }
  if (minuteWindowSamples_ >= sampleRate_ * 60.0) {
    minuteWindowSamples_ = 0.0;
    commitCandle(minuteCandles_, minuteCandleWrite_, minuteCandleCount_);
  }
}

HaloState AnalysisEngine::snapshot() const {
  return buildHaloState();
}

HaloState AnalysisEngine::buildHaloState() const {
  HaloState state;
  const auto confidence = clamp01(smoothed_.signal01 * 1.35f);
  state.reference = ReferenceTarget{};
  state.metrics.tone01 = smoothed_.spectralTilt;
  state.metrics.width01 = smoothed_.stereoWidth;
  state.metrics.punch01 = smoothed_.transientDensity;
  state.metrics.loudness01 = toLoudness01(smoothed_.rmsDb);
  state.metrics.rmsDb = smoothed_.rmsDb;
  state.metrics.peakDb = smoothed_.peakDb;
  state.metrics.crestDb = smoothed_.crestDb;
  state.metrics.correlation = smoothed_.correlation;
  state.metrics.transientDensity = smoothed_.transientDensity;
  state.metrics.spectrumBands = smoothed_.spectrumBands;
  state.metrics.candleOpen = smoothed_.candleOpen;
  state.metrics.candleHigh = smoothed_.candleHigh;
  state.metrics.candleLow = smoothed_.candleLow;
  state.metrics.candleClose = smoothed_.candleClose;
  state.metrics.sessionCandleCount = sessionCandleCount_;
  state.metrics.minuteCandleCount = minuteCandleCount_;
  for (int i = 0; i < 10; ++i) {
    const auto sessionIndex = (sessionCandleWrite_ + i) % 10;
    const auto minuteIndex = (minuteCandleWrite_ + i) % 10;
    const auto& s = sessionCandles_[static_cast<size_t>(sessionIndex)];
    const auto& m = minuteCandles_[static_cast<size_t>(minuteIndex)];
    state.metrics.sessionCandleOpen[static_cast<size_t>(i)] = s.open;
    state.metrics.sessionCandleHigh[static_cast<size_t>(i)] = s.high;
    state.metrics.sessionCandleLow[static_cast<size_t>(i)] = s.low;
    state.metrics.sessionCandleClose[static_cast<size_t>(i)] = s.close;
    state.metrics.minuteCandleOpen[static_cast<size_t>(i)] = m.open;
    state.metrics.minuteCandleHigh[static_cast<size_t>(i)] = m.high;
    state.metrics.minuteCandleLow[static_cast<size_t>(i)] = m.low;
    state.metrics.minuteCandleClose[static_cast<size_t>(i)] = m.close;
  }

  const auto toneError = deviationOutsideCorridor(smoothed_.spectralTilt, 0.48f, 0.30f, 0.56f);
  const auto loudnessError = 0.58f * deviationOutsideCorridor(smoothed_.rmsDb, state.reference.loudnessDb, 4.5f, 16.0f)
                           + 0.42f * deviationOutsideCorridor(smoothed_.peakDb, -1.0f, 1.2f, 8.0f);
  const auto dynamicsError = 0.58f * deviationOutsideCorridor(smoothed_.crestDb, state.reference.crestDb, 4.6f, 17.0f)
                           + 0.42f * deviationOutsideCorridor(smoothed_.transientDensity, 0.42f, 0.42f, 0.86f);
  const auto stereoWidthError = deviationOutsideCorridor(smoothed_.stereoWidth, 0.52f, 0.42f, 0.74f);
  const auto phaseError = smoothed_.correlation < 0.0f ? clamp01(-smoothed_.correlation) : 0.0f;
  const auto stereoError = 0.72f * stereoWidthError + 0.28f * phaseError;

  state.tone = makeDomain(toneError, smoothed_.spectralTilt, smoothed_.rmsDb, "Spectral shape against a balanced production corridor.", confidence);
  state.stereo = makeDomain(stereoError, smoothed_.stereoWidth, smoothed_.correlation, "Width and mono safety above the protected low end.", confidence);
  state.loudness = makeDomain(loudnessError, smoothed_.rmsDb, smoothed_.peakDb, "Working loudness and headroom relative to a mix-stage target.", confidence);
  state.dynamics = makeDomain(dynamicsError, smoothed_.crestDb, smoothed_.transientDensity, "Crest, transient motion, and controlled punch.", confidence);

  const auto totalError = 0.35f * state.tone.error01 + 0.25f * state.stereo.error01
                        + 0.20f * state.loudness.error01 + 0.20f * state.dynamics.error01;
  state.totalAlignment01 = 1.0f - clamp01(totalError);

  if (smoothed_.signal01 < 0.02f) {
    state.primaryTitle = "Signal idle";
    state.primaryCause = "Route audio through AIFRED.";
    return state;
  }

  if (state.totalAlignment01 >= 0.82f) {
    state.primaryTitle = "Locked";
    state.primaryCause = "Tone, width, punch, and loudness are inside the pro corridor.";
    state.primarySeverity = Severity::Minor;
    state.primaryConfidence = Confidence::High;
    return state;
  }

  const DomainAlignment* dominant = &state.tone;
  const char* title = "Tone";
  if (state.stereo.error01 > dominant->error01) { dominant = &state.stereo; title = "Width"; }
  if (state.dynamics.error01 > dominant->error01) { dominant = &state.dynamics; title = "Punch"; }
  if (state.loudness.error01 > dominant->error01) { dominant = &state.loudness; title = "Loudness"; }
  state.primaryTitle = title;
  state.primaryCause = "Chat output handles the detailed move list.";
  state.primarySeverity = dominant->severity;
  state.primaryConfidence = dominant->confidence;

  return state;
}

} // namespace aifred
