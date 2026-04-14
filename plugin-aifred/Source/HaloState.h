#pragma once

#include <string>
#include <vector>

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

struct FixItem {
  std::string title;
  std::string cause;
  std::string nextStep;
  Severity severity = Severity::Minor;
  Confidence confidence = Confidence::Low;
  Domain impactDomain = Domain::Tone;
  float priority = 0.0f;
};

struct HaloState {
  AnalysisMode mode = AnalysisMode::Analyze;
  DomainAlignment tone;
  DomainAlignment stereo;
  DomainAlignment loudness;
  DomainAlignment dynamics;
  float totalAlignment01 = 1.0f;
  std::string primaryTitle = "Waiting For Signal";
  std::string primaryCause = "Play audio through Aifred to begin mix alignment.";
  Severity primarySeverity = Severity::Minor;
  Confidence primaryConfidence = Confidence::Low;
  std::vector<FixItem> rankedFixes;
};

float clamp01(float value);
Severity severityForError(float error01);
Confidence confidenceForPersistence(float persistence01);
const char* domainName(Domain domain);
const char* severityName(Severity severity);
const char* confidenceName(Confidence confidence);

} // namespace aifred
