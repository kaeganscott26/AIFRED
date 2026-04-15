#pragma once

#include <string>
#include <vector>
#include <array>

namespace aifred {

enum class AnalysisMode { Analyze, Reference, Compare };
enum class Domain { Tone, Stereo, Loudness, Dynamics };
enum class Severity { Minor, Moderate, High, Critical };
enum class Confidence { Low, Moderate, High };

struct DomainAlignment {
  float error01 = 0.0f;
  float alignment01 = 1.0f;
  Severity severity = Severity::Minor;
  Confidence confidence = Confidence::Low;
  float rawPrimaryMetric = 0.0f;
  float rawSecondaryMetric = 0.0f;
  std::string summary;
};

struct DspMetrics {
  float tone01 = 0.0f;
  float width01 = 0.0f;
  float punch01 = 0.0f;
  float loudness01 = 0.0f;
  float rmsDb = -90.0f;
  float peakDb = -90.0f;
  float crestDb = 0.0f;
  float correlation = 1.0f;
  float transientDensity = 0.0f;
  float candleOpen = 0.0f;
  float candleHigh = 0.0f;
  float candleLow = 0.0f;
  float candleClose = 0.0f;
  std::array<float, 10> sessionCandleOpen {};
  std::array<float, 10> sessionCandleHigh {};
  std::array<float, 10> sessionCandleLow {};
  std::array<float, 10> sessionCandleClose {};
  std::array<float, 10> minuteCandleOpen {};
  std::array<float, 10> minuteCandleHigh {};
  std::array<float, 10> minuteCandleLow {};
  std::array<float, 10> minuteCandleClose {};
  int sessionCandleCount = 0;
  int minuteCandleCount = 0;
};

struct ReferenceTarget {
  float tone01 = 0.48f;
  float width01 = 0.52f;
  float punch01 = 0.58f;
  float loudnessDb = -17.32f;
  float crestDb = 15.25f;
  int poolSize = 25;
  std::string label = "PRO 25x6";
};

struct HaloState {
  AnalysisMode mode = AnalysisMode::Analyze;
  DomainAlignment tone;
  DomainAlignment stereo;
  DomainAlignment loudness;
  DomainAlignment dynamics;
  DspMetrics metrics;
  ReferenceTarget reference;
  float totalAlignment01 = 1.0f;
  std::string primaryTitle = "Signal idle";
  std::string primaryCause = "Play audio through AIFRED.";
  Severity primarySeverity = Severity::Minor;
  Confidence primaryConfidence = Confidence::Low;
};

float clamp01(float value);
Severity severityForError(float error01);
Confidence confidenceForPersistence(float persistence01);
const char* domainName(Domain domain);
const char* severityName(Severity severity);
const char* confidenceName(Confidence confidence);

} // namespace aifred
