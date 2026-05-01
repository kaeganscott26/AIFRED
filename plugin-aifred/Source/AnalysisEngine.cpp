#include "AnalysisEngine.h"

#include <cmath>
#include <algorithm>
#include <numeric>

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

bool finite(float value) {
  return std::isfinite(value);
}

float distanceError(float value, float target, float tolerance) {
  if (!finite(value) || !finite(target) || tolerance <= 0.0f) return 1.0f;
  return clamp01(std::abs(value - target) / tolerance);
}

float capNearPerfect(float score, float value, float target, float perfectDelta) {
  if (!finite(value) || !finite(target)) return 0.0f;
  score = clamp01(score);
  if (score > 0.99f && std::abs(value - target) > perfectDelta) return 0.99f;
  return score;
}

int percentFromScore(float score, bool valid) {
  if (!valid) return 0;
  return juce::roundToInt(clamp01(score) * 100.0f);
}

} // namespace

void AnalysisEngine::prepare(double sampleRate) {
  sampleRate_ = sampleRate > 0.0 ? sampleRate : 44100.0;
  momentaryWindows_.reserve(64);
  shortTermWindows_.reserve(384);
  configureKWeighting();
  reset();
}

void AnalysisEngine::reset() {
  live_ = {};
  smoothed_ = {};
  liveSessionCandle_ = {};
  minuteCandles_ = {};
  sessionWindowSamples_ = 0.0;
  minuteWindowSamples_ = 0.0;
  integratedWeightedSquares_ = 0.0;
  integratedSamples_ = 0.0;
  momentaryWeightedSquares_ = 0.0;
  momentarySamples_ = 0.0;
  shortTermWeightedSquares_ = 0.0;
  shortTermSamples_ = 0.0;
  previousTruePeakLeft_ = 0.0f;
  previousTruePeakRight_ = 0.0f;
  sessionCandleWrite_ = 0;
  minuteCandleWrite_ = 0;
  sessionCandleCount_ = 0;
  minuteCandleCount_ = 0;
  hpLeft_ = 0.0f;
  hpRight_ = 0.0f;
  hpPrevLeft_ = 0.0f;
  hpPrevRight_ = 0.0f;
  kHighPassLeft_.reset();
  kHighPassRight_.reset();
  kShelfLeft_.reset();
  kShelfRight_.reset();
  momentaryWindows_.clear();
  shortTermWindows_.clear();
  shortTermHistory_ = {};
  shortTermHistoryWrite_ = 0;
  shortTermHistoryCount_ = 0;
}

float AnalysisEngine::BiquadState::process(float x) {
  const auto y = b0 * x + z1;
  z1 = b1 * x - a1 * y + z2;
  z2 = b2 * x - a2 * y;
  return y;
}

void AnalysisEngine::BiquadState::reset() {
  z1 = 0.0f;
  z2 = 0.0f;
}

void AnalysisEngine::configureKWeighting() {
  const auto sr = static_cast<float>(std::max(sampleRate_, 1000.0));
  const auto setHighPass = [sr](BiquadState& filter) {
    const auto q = 0.5f;
    const auto omega = juce::MathConstants<float>::twoPi * 38.0f / sr;
    const auto alpha = std::sin(omega) / (2.0f * q);
    const auto cosw = std::cos(omega);
    const auto a0 = 1.0f + alpha;
    filter.b0 = (1.0f + cosw) * 0.5f / a0;
    filter.b1 = -(1.0f + cosw) / a0;
    filter.b2 = (1.0f + cosw) * 0.5f / a0;
    filter.a1 = -2.0f * cosw / a0;
    filter.a2 = (1.0f - alpha) / a0;
  };
  const auto setHighShelf = [sr](BiquadState& filter) {
    const auto gainDb = 4.0f;
    const auto slope = 1.0f;
    const auto a = std::pow(10.0f, gainDb / 40.0f);
    const auto omega = juce::MathConstants<float>::twoPi * 1681.974f / sr;
    const auto sinw = std::sin(omega);
    const auto cosw = std::cos(omega);
    const auto alpha = sinw * 0.5f * std::sqrt((a + 1.0f / a) * (1.0f / slope - 1.0f) + 2.0f);
    const auto twoSqrtAAlpha = 2.0f * std::sqrt(a) * alpha;
    const auto b0 = a * ((a + 1.0f) + (a - 1.0f) * cosw + twoSqrtAAlpha);
    const auto b1 = -2.0f * a * ((a - 1.0f) + (a + 1.0f) * cosw);
    const auto b2 = a * ((a + 1.0f) + (a - 1.0f) * cosw - twoSqrtAAlpha);
    const auto a0 = (a + 1.0f) - (a - 1.0f) * cosw + twoSqrtAAlpha;
    const auto a1 = 2.0f * ((a - 1.0f) - (a + 1.0f) * cosw);
    const auto a2 = (a + 1.0f) - (a - 1.0f) * cosw - twoSqrtAAlpha;
    filter.b0 = b0 / a0;
    filter.b1 = b1 / a0;
    filter.b2 = b2 / a0;
    filter.a1 = a1 / a0;
    filter.a2 = a2 / a0;
  };
  setHighPass(kHighPassLeft_);
  setHighPass(kHighPassRight_);
  setHighShelf(kShelfLeft_);
  setHighShelf(kShelfRight_);
}

float AnalysisEngine::linearToDb(float value) {
  return 20.0f * std::log10(std::max(value, 0.0000316f));
}

float AnalysisEngine::squareMeanToLufs(double weightedSquares, double samples) {
  if (samples <= 0.0) return -90.0f;
  return -0.691f + 10.0f * std::log10(static_cast<float>(std::max(weightedSquares / samples, 0.000000001)));
}

void AnalysisEngine::pushLoudnessWindow(std::vector<std::pair<double, double>>& windows, double squares, double samples, double maxSamples) {
  windows.emplace_back(squares, samples);
  double total = 0.0;
  for (const auto& item : windows) total += item.second;
  while (!windows.empty() && total > maxSamples) {
    total -= windows.front().second;
    windows.erase(windows.begin());
  }
}

float AnalysisEngine::loudnessRangeFromHistory(const std::array<float, 64>& history, int count) {
  if (count < 4) return 0.0f;
  std::vector<float> values;
  values.reserve(static_cast<size_t>(count));
  for (int i = 0; i < count; ++i) {
    const auto value = history[static_cast<size_t>(i)];
    if (value > -70.0f) values.push_back(value);
  }
  if (values.size() < 4) return 0.0f;
  std::sort(values.begin(), values.end());
  const auto low = values[static_cast<size_t>(std::floor((values.size() - 1) * 0.10))];
  const auto high = values[static_cast<size_t>(std::floor((values.size() - 1) * 0.95))];
  return std::max(0.0f, high - low);
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

void AnalysisEngine::pushSessionHistory(const CandleFrame& candle) {
  if (!candle.active && candle.high <= 0.0f && candle.close <= 0.0f) return;
  sessionCandles_[static_cast<size_t>(sessionCandleWrite_)] = candle;
  sessionCandles_[static_cast<size_t>(sessionCandleWrite_)].active = false;
  sessionCandleWrite_ = (sessionCandleWrite_ + 1) % static_cast<int>(sessionCandles_.size());
  sessionCandleCount_ = std::min(static_cast<int>(sessionCandles_.size()), sessionCandleCount_ + 1);
}

void AnalysisEngine::finalizeCurrentSession() {
  pushSessionHistory(liveSessionCandle_);
}

DspMetrics AnalysisEngine::exportSessionCandles() const {
  DspMetrics metrics;
  metrics.sessionCandleCount = sessionCandleCount_;
  for (int i = 0; i < 10; ++i) {
    const auto index = (sessionCandleWrite_ + i) % 10;
    const auto& c = sessionCandles_[static_cast<size_t>(index)];
    metrics.sessionCandleOpen[static_cast<size_t>(i)] = c.open;
    metrics.sessionCandleHigh[static_cast<size_t>(i)] = c.high;
    metrics.sessionCandleLow[static_cast<size_t>(i)] = c.low;
    metrics.sessionCandleClose[static_cast<size_t>(i)] = c.close;
  }
  return metrics;
}

void AnalysisEngine::importSessionCandles(const DspMetrics& metrics) {
  sessionCandles_ = {};
  sessionCandleWrite_ = 0;
  sessionCandleCount_ = juce::jlimit(0, 10, metrics.sessionCandleCount);
  const auto first = 10 - sessionCandleCount_;
  for (int i = first; i < 10; ++i) {
    CandleFrame candle;
    candle.open = clamp01(metrics.sessionCandleOpen[static_cast<size_t>(i)]);
    candle.high = clamp01(metrics.sessionCandleHigh[static_cast<size_t>(i)]);
    candle.low = clamp01(metrics.sessionCandleLow[static_cast<size_t>(i)]);
    candle.close = clamp01(metrics.sessionCandleClose[static_cast<size_t>(i)]);
    if (candle.high > 0.0f || candle.close > 0.0f) {
      sessionCandles_[static_cast<size_t>(sessionCandleWrite_)] = candle;
      sessionCandleWrite_ = (sessionCandleWrite_ + 1) % 10;
    }
  }
}

void AnalysisEngine::pushAudioBlock(const juce::AudioBuffer<float>& buffer) {
  const auto channels = buffer.getNumChannels();
  const auto samples = buffer.getNumSamples();
  if (channels == 0 || samples == 0) return;

  double sumSquares = 0.0;
  double kWeightedSquares = 0.0;
  double peak = 0.0;
  double truePeak = 0.0;
  double leftEnergy = 0.0;
  double rightEnergy = 0.0;
  double hpLeftEnergy = 0.0;
  double hpRightEnergy = 0.0;
  double midEnergy = 0.0;
  double sideEnergy = 0.0;
  double hpCorrNum = 0.0;
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
  std::array<double, 96> waveformSum {};
  std::array<int, 96> waveformCount {};

  const auto* left = buffer.getReadPointer(0);
  const auto* right = channels > 1 ? buffer.getReadPointer(1) : left;
  const auto hpCutoff = 150.0f;
  const auto rc = 1.0f / (juce::MathConstants<float>::twoPi * hpCutoff);
  const auto dt = 1.0f / static_cast<float>(std::max(sampleRate_, 1.0));
  const auto hpAlpha = rc / (rc + dt);

  for (int i = 0; i < samples; ++i) {
    const auto l = left[i];
    const auto r = right[i];
    const auto hpL = hpAlpha * (hpLeft_ + l - hpPrevLeft_);
    const auto hpR = hpAlpha * (hpRight_ + r - hpPrevRight_);
    const auto kL = kShelfLeft_.process(kHighPassLeft_.process(l));
    const auto kR = kShelfRight_.process(kHighPassRight_.process(r));
    hpPrevLeft_ = l;
    hpPrevRight_ = r;
    hpLeft_ = hpL;
    hpRight_ = hpR;
    const auto mono = 0.5f * (l + r);
    const auto side = 0.5f * (l - r);
    const auto waveformIndex = static_cast<size_t>(juce::jlimit(0, 95, (i * 96) / std::max(samples, 1)));
    waveformSum[waveformIndex] += mono;
    waveformCount[waveformIndex] += 1;
    const auto monoDelta = mono - prevMono;
    sumSquares += 0.5 * (static_cast<double>(l) * l + static_cast<double>(r) * r);
    kWeightedSquares += 0.5 * (static_cast<double>(kL) * kL + static_cast<double>(kR) * kR);
    leftEnergy += static_cast<double>(l) * l;
    rightEnergy += static_cast<double>(r) * r;
    hpLeftEnergy += static_cast<double>(hpL) * hpL;
    hpRightEnergy += static_cast<double>(hpR) * hpR;
    midEnergy += static_cast<double>(mono) * mono;
    sideEnergy += static_cast<double>(side) * side;
    hpCorrNum += static_cast<double>(hpL) * hpR;
    peak = std::max({peak, std::abs(static_cast<double>(l)), std::abs(static_cast<double>(r))});
    for (int step = 1; step <= 4; ++step) {
      const auto t = static_cast<float>(step) * 0.25f;
      const auto interpL = previousTruePeakLeft_ + (l - previousTruePeakLeft_) * t;
      const auto interpR = previousTruePeakRight_ + (r - previousTruePeakRight_) * t;
      truePeak = std::max({truePeak, std::abs(static_cast<double>(interpL)), std::abs(static_cast<double>(interpR))});
    }
    previousTruePeakLeft_ = l;
    previousTruePeakRight_ = r;
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
  const auto kMeanSquare = static_cast<float>(kWeightedSquares / sampleCount);
  const auto loudness = -0.691f + 10.0f * std::log10(std::max(kMeanSquare, 0.000000001f));
  pushLoudnessWindow(momentaryWindows_, kWeightedSquares, sampleCount, sampleRate_ * 0.400);
  pushLoudnessWindow(shortTermWindows_, kWeightedSquares, sampleCount, sampleRate_ * 3.000);
  momentaryWeightedSquares_ = 0.0;
  momentarySamples_ = 0.0;
  for (const auto& window : momentaryWindows_) {
    momentaryWeightedSquares_ += window.first;
    momentarySamples_ += window.second;
  }
  shortTermWeightedSquares_ = 0.0;
  shortTermSamples_ = 0.0;
  for (const auto& window : shortTermWindows_) {
    shortTermWeightedSquares_ += window.first;
    shortTermSamples_ += window.second;
  }
  if (loudness > -70.0f) {
    integratedWeightedSquares_ += kWeightedSquares;
    integratedSamples_ += sampleCount;
  }
  const auto momentary = squareMeanToLufs(momentaryWeightedSquares_, momentarySamples_);
  const auto shortTerm = squareMeanToLufs(shortTermWeightedSquares_, shortTermSamples_);
  const auto integrated = squareMeanToLufs(integratedWeightedSquares_, integratedSamples_);
  shortTermHistory_[static_cast<size_t>(shortTermHistoryWrite_)] = shortTerm;
  shortTermHistoryWrite_ = (shortTermHistoryWrite_ + 1) % static_cast<int>(shortTermHistory_.size());
  shortTermHistoryCount_ = std::min(static_cast<int>(shortTermHistory_.size()), shortTermHistoryCount_ + 1);
  const auto leftRms = safeSqrt(static_cast<float>((hpLeftEnergy > 0.0000001 ? hpLeftEnergy : leftEnergy) / sampleCount));
  const auto rightRms = safeSqrt(static_cast<float>((hpRightEnergy > 0.0000001 ? hpRightEnergy : rightEnergy) / sampleCount));
  const auto denom = std::max(leftRms * rightRms * static_cast<float>(sampleCount), 0.000001f);
  const auto correlation = std::clamp(static_cast<float>(hpCorrNum) / denom, -1.0f, 1.0f);
  const auto width = clamp01(safeSqrt(static_cast<float>(sideEnergy / std::max(midEnergy + sideEnergy, 0.000001))));
  const auto crest = linearToDb(static_cast<float>(peak)) - loudness;
  const auto tilt = clamp01(static_cast<float>(highMotion / std::max(lowMotion + highMotion, 0.000001)));

  live_.rmsDb = loudness;
  live_.momentaryLufs = momentary;
  live_.shortTermLufs = shortTerm;
  live_.integratedLufs = integrated;
  live_.loudnessRange = loudnessRangeFromHistory(shortTermHistory_, shortTermHistoryCount_);
  live_.peakDb = linearToDb(static_cast<float>(peak));
  live_.truePeakDb = linearToDb(static_cast<float>(truePeak));
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
  for (size_t i = 0; i < live_.waveform.size(); ++i) {
    const auto count = std::max(1, waveformCount[i]);
    live_.waveform[i] = std::clamp(static_cast<float>(waveformSum[i] / count), -1.0f, 1.0f);
  }
  live_.signal01 = clamp01((live_.rmsDb + 60.0f) / 48.0f);
  live_.candleOpen = clamp01((smoothed_.rmsDb + 42.0f) / 34.0f);
  live_.candleClose = toLoudness01(live_.rmsDb);
  live_.candleHigh = std::max(live_.candleOpen, clamp01((live_.peakDb + 18.0f) / 18.0f));
  live_.candleLow = std::min(live_.candleOpen, clamp01((live_.rmsDb - live_.crestDb + 42.0f) / 34.0f));

  const auto amount = live_.signal01 > 0.02f ? smoothing_ : 0.98f;
  smoothed_.rmsDb = smooth(smoothed_.rmsDb, live_.rmsDb, amount);
  smoothed_.momentaryLufs = smooth(smoothed_.momentaryLufs, live_.momentaryLufs, amount);
  smoothed_.shortTermLufs = smooth(smoothed_.shortTermLufs, live_.shortTermLufs, amount);
  smoothed_.integratedLufs = smooth(smoothed_.integratedLufs, live_.integratedLufs, amount);
  smoothed_.loudnessRange = smooth(smoothed_.loudnessRange, live_.loudnessRange, amount);
  smoothed_.peakDb = smooth(smoothed_.peakDb, live_.peakDb, amount);
  smoothed_.truePeakDb = smooth(smoothed_.truePeakDb, live_.truePeakDb, amount);
  smoothed_.crestDb = smooth(smoothed_.crestDb, live_.crestDb, amount);
  smoothed_.stereoWidth = smooth(smoothed_.stereoWidth, live_.stereoWidth, amount);
  smoothed_.correlation = smooth(smoothed_.correlation, live_.correlation, amount);
  smoothed_.spectralTilt = smooth(smoothed_.spectralTilt, live_.spectralTilt, amount);
  smoothed_.transientDensity = smooth(smoothed_.transientDensity, live_.transientDensity, amount);
  for (size_t band = 0; band < smoothed_.spectrumBands.size(); ++band) {
    smoothed_.spectrumBands[band] = smooth(smoothed_.spectrumBands[band], live_.spectrumBands[band], amount);
  }
  for (size_t i = 0; i < smoothed_.waveform.size(); ++i) {
    smoothed_.waveform[i] = smooth(smoothed_.waveform[i], live_.waveform[i], 0.72f);
  }
  smoothed_.signal01 = smooth(smoothed_.signal01, live_.signal01, amount);
  smoothed_.candleOpen = smooth(smoothed_.candleOpen, live_.candleOpen, amount);
  smoothed_.candleHigh = smooth(smoothed_.candleHigh, live_.candleHigh, amount);
  smoothed_.candleLow = smooth(smoothed_.candleLow, live_.candleLow, amount);
  smoothed_.candleClose = smooth(smoothed_.candleClose, live_.candleClose, amount);

  updateCandle(liveSessionCandle_, live_.candleClose);
  updateCandle(minuteCandles_[static_cast<size_t>(minuteCandleWrite_)], live_.candleClose);
  if (live_.signal01 > 0.02f) minuteCandleCount_ = std::max(1, minuteCandleCount_);
  minuteWindowSamples_ += samples;
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
  state.currentSnapshotTimestampMs = juce::Time::getMillisecondCounterHiRes();
  state.historyWindowMs = 5000.0;
  state.hasSignal = smoothed_.signal01 >= 0.02f;
  state.hasReference = state.reference.poolSize > 0;
  state.valuesValid = finite(smoothed_.shortTermLufs) && finite(smoothed_.integratedLufs)
    && finite(smoothed_.truePeakDb) && finite(smoothed_.peakDb) && finite(smoothed_.crestDb)
    && finite(smoothed_.stereoWidth) && finite(smoothed_.correlation)
    && finite(smoothed_.spectralTilt) && finite(smoothed_.transientDensity);
  state.isStale = !state.hasSignal || !state.valuesValid;
  state.isUsingFallbackScore = !state.hasSignal || !state.hasReference || !state.valuesValid;
  state.metrics.tone01 = smoothed_.spectralTilt;
  state.metrics.width01 = smoothed_.stereoWidth;
  state.metrics.punch01 = smoothed_.transientDensity;
  state.metrics.loudness01 = toLoudness01(smoothed_.rmsDb);
  state.metrics.rmsDb = smoothed_.rmsDb;
  state.metrics.momentaryLufs = smoothed_.momentaryLufs;
  state.metrics.shortTermLufs = smoothed_.shortTermLufs;
  state.metrics.integratedLufs = smoothed_.integratedLufs;
  state.metrics.loudnessRange = smoothed_.loudnessRange;
  state.metrics.peakDb = smoothed_.peakDb;
  state.metrics.truePeakDb = smoothed_.truePeakDb;
  state.metrics.crestDb = smoothed_.crestDb;
  state.metrics.correlation = smoothed_.correlation;
  state.metrics.transientDensity = smoothed_.transientDensity;
  state.metrics.spectrumBands = smoothed_.spectrumBands;
  state.metrics.waveform = smoothed_.waveform;
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

  const bool canScore = state.hasSignal && state.hasReference && state.valuesValid;
  const auto toneError = canScore ? distanceError(smoothed_.spectralTilt, state.reference.tone01, 0.42f) : 1.0f;
  const auto lufsWindowMismatch = canScore ? distanceError(smoothed_.shortTermLufs, smoothed_.integratedLufs, 7.0f) : 1.0f;
  const auto loudnessError = canScore
    ? clamp01(0.58f * distanceError(smoothed_.integratedLufs, state.reference.loudnessDb, 8.0f)
            + 0.24f * distanceError(smoothed_.truePeakDb, -1.0f, 7.0f)
            + 0.18f * lufsWindowMismatch)
    : 1.0f;
  const auto dynamicsError = canScore
    ? clamp01(0.62f * distanceError(smoothed_.crestDb, state.reference.crestDb, 10.0f)
            + 0.38f * distanceError(smoothed_.transientDensity, state.reference.punch01, 0.62f))
    : 1.0f;
  const auto phaseRisk = smoothed_.correlation <= 0.02f ? 0.70f : (smoothed_.correlation < 0.20f ? 0.35f : 0.0f);
  const auto stereoError = canScore
    ? clamp01(0.70f * distanceError(smoothed_.stereoWidth, state.reference.width01, 0.54f) + 0.30f * phaseRisk)
    : 1.0f;

  state.tone = makeDomain(toneError, smoothed_.spectralTilt, smoothed_.rmsDb, "Spectral shape against a balanced production corridor.", confidence);
  state.stereo = makeDomain(stereoError, smoothed_.stereoWidth, smoothed_.correlation, "Width and mono safety above the protected low end.", confidence);
  state.loudness = makeDomain(loudnessError, smoothed_.shortTermLufs, smoothed_.truePeakDb, "Short-term LUFS, integrated trend, LRA, and 4x estimated dBTP headroom.", confidence);
  state.dynamics = makeDomain(dynamicsError, smoothed_.crestDb, smoothed_.transientDensity, "Crest, transient motion, and controlled punch.", confidence);
  state.tone.alignment01 = capNearPerfect(state.tone.alignment01, smoothed_.spectralTilt, state.reference.tone01, 0.005f);
  state.stereo.alignment01 = capNearPerfect(state.stereo.alignment01, smoothed_.stereoWidth, state.reference.width01, 0.005f);
  state.loudness.alignment01 = capNearPerfect(state.loudness.alignment01, smoothed_.integratedLufs, state.reference.loudnessDb, 0.05f);
  state.dynamics.alignment01 = capNearPerfect(state.dynamics.alignment01, smoothed_.crestDb, state.reference.crestDb, 0.05f);
  state.tone.error01 = 1.0f - state.tone.alignment01;
  state.stereo.error01 = 1.0f - state.stereo.alignment01;
  state.loudness.error01 = 1.0f - state.loudness.alignment01;
  state.dynamics.error01 = 1.0f - state.dynamics.alignment01;
  state.toneScore01 = state.tone.alignment01;
  state.widthScore01 = state.stereo.alignment01;
  state.punchScore01 = state.dynamics.alignment01;
  state.loudnessScore01 = state.loudness.alignment01;
  state.displayedTonePercent = percentFromScore(state.toneScore01, canScore);
  state.displayedWidthPercent = percentFromScore(state.widthScore01, canScore);
  state.displayedPunchPercent = percentFromScore(state.punchScore01, canScore);
  state.displayedLoudnessPercent = percentFromScore(state.loudnessScore01, canScore);
  state.humanSummary = ("Current short-term LUFS " + juce::String(smoothed_.shortTermLufs, 1)
    + ", integrated LUFS " + juce::String(smoothed_.integratedLufs, 1)
    + ", true peak " + juce::String(smoothed_.truePeakDb, 1) + " dBTP"
    + ", sample peak " + juce::String(smoothed_.peakDb, 1) + " dBFS"
    + ", crest " + juce::String(smoothed_.crestDb, 1) + " dB"
    + ", width " + juce::String(smoothed_.stereoWidth, 3)
    + ", correlation " + juce::String(smoothed_.correlation, 3)
    + ", 5s history tracked separately.").toStdString();

  const auto totalError = 0.35f * state.tone.error01 + 0.25f * state.stereo.error01
                        + 0.20f * state.loudness.error01 + 0.20f * state.dynamics.error01;
  state.totalAlignment01 = canScore ? (1.0f - clamp01(totalError)) : 0.0f;

  if (!state.hasSignal) {
    state.primaryTitle = "Signal idle";
    state.primaryCause = "Route audio through AIFRED.";
    return state;
  }
  if (!state.valuesValid) {
    state.primaryTitle = "Metrics invalid";
    state.primaryCause = "Current analysis window contains missing or invalid values.";
    state.primarySeverity = Severity::High;
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
  if (dominant == &state.loudness) state.primaryCause = "Short-term loudness or true peak is outside the target zone.";
  else if (dominant == &state.stereo) state.primaryCause = "Width or correlation is creating translation risk.";
  else if (dominant == &state.dynamics) state.primaryCause = "Crest factor or transient density is outside the reference corridor.";
  else state.primaryCause = "Spectral tilt is outside the balanced production corridor.";
  state.primarySeverity = dominant->severity;
  state.primaryConfidence = dominant->confidence;

  return state;
}

} // namespace aifred
