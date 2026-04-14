#include "HaloState.h"

#include <algorithm>

namespace aifred {

float clamp01(float value) {
  return std::clamp(value, 0.0f, 1.0f);
}

Severity severityForError(float error01) {
  const auto error = clamp01(error01);
  if (error >= 0.65f) return Severity::Critical;
  if (error >= 0.40f) return Severity::High;
  if (error >= 0.20f) return Severity::Moderate;
  return Severity::Minor;
}

Confidence confidenceForPersistence(float persistence01) {
  const auto persistence = clamp01(persistence01);
  if (persistence >= 0.72f) return Confidence::High;
  if (persistence >= 0.42f) return Confidence::Moderate;
  return Confidence::Low;
}

const char* domainName(Domain domain) {
  switch (domain) {
    case Domain::Tone: return "Tone";
    case Domain::Stereo: return "Stereo";
    case Domain::Loudness: return "Loudness";
    case Domain::Dynamics: return "Dynamics";
  }
  return "Domain";
}

const char* severityName(Severity severity) {
  switch (severity) {
    case Severity::Minor: return "Minor";
    case Severity::Moderate: return "Moderate";
    case Severity::High: return "High";
    case Severity::Critical: return "Critical";
  }
  return "Minor";
}

const char* confidenceName(Confidence confidence) {
  switch (confidence) {
    case Confidence::Low: return "Low";
    case Confidence::Moderate: return "Moderate";
    case Confidence::High: return "High";
  }
  return "Low";
}

} // namespace aifred
