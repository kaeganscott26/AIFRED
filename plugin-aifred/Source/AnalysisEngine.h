#pragma once

#include "HaloState.h"

#include <juce_audio_basics/juce_audio_basics.h>

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
    float signal01 = 0.0f;
    float candleOpen = 0.0f;
    float candleHigh = 0.0f;
    float candleLow = 0.0f;
    float candleClose = 0.0f;
  };

  FeatureFrame live_;
  FeatureFrame smoothed_;
  double sampleRate_ = 44100.0;
  float smoothing_ = 0.92f;

  static float linearToDb(float value);
  static float deviationOutsideCorridor(float value, float target, float tolerance, float criticalRange);
  static DomainAlignment makeDomain(float error01, float primary, float secondary, std::string summary, float confidenceSeed);
  HaloState buildHaloState() const;
};

} // namespace aifred
