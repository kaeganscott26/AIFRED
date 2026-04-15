#pragma once

#include "HaloState.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

namespace aifred {

class AnalysisEngine {
public:
  void prepare(double sampleRate);
  void reset();
  void pushAudioBlock(const juce::AudioBuffer<float>& buffer);
  HaloState snapshot() const;

private:
  struct FeatureFrame {
    float rmsDb = -90.0f;
    float peakDb = -90.0f;
    float crestDb = 0.0f;
    float stereoWidth = 0.0f;
    float correlation = 1.0f;
    float spectralTilt = 0.0f;
    float transientDensity = 0.0f;
    std::array<float, 8> spectrumBands {};
    float signal01 = 0.0f;
    float candleOpen = 0.0f;
    float candleHigh = 0.0f;
    float candleLow = 0.0f;
    float candleClose = 0.0f;
  };

  struct CandleFrame {
    float open = 0.0f;
    float high = 0.0f;
    float low = 0.0f;
    float close = 0.0f;
    bool active = false;
  };

  struct BiquadState {
    float b0 = 1.0f;
    float b1 = 0.0f;
    float b2 = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
    float z1 = 0.0f;
    float z2 = 0.0f;

    float process(float x);
    void reset();
  };

  FeatureFrame live_;
  FeatureFrame smoothed_;
  std::array<CandleFrame, 10> sessionCandles_ {};
  std::array<CandleFrame, 10> minuteCandles_ {};
  double sampleRate_ = 44100.0;
  float smoothing_ = 0.92f;
  double sessionWindowSamples_ = 0.0;
  double minuteWindowSamples_ = 0.0;
  int sessionCandleWrite_ = 0;
  int minuteCandleWrite_ = 0;
  int sessionCandleCount_ = 0;
  int minuteCandleCount_ = 0;
  float hpLeft_ = 0.0f;
  float hpRight_ = 0.0f;
  float hpPrevLeft_ = 0.0f;
  float hpPrevRight_ = 0.0f;
  BiquadState kHighPassLeft_;
  BiquadState kHighPassRight_;
  BiquadState kShelfLeft_;
  BiquadState kShelfRight_;

  static float linearToDb(float value);
  void configureKWeighting();
  static float deviationOutsideCorridor(float value, float target, float tolerance, float criticalRange);
  static DomainAlignment makeDomain(float error01, float primary, float secondary, std::string summary, float confidenceSeed);
  void updateCandle(CandleFrame& candle, float value);
  void commitCandle(std::array<CandleFrame, 10>& candles, int& writeIndex, int& count);
  HaloState buildHaloState() const;
};

} // namespace aifred
