#pragma once

#include "HaloState.h"

#include <juce_core/juce_core.h>
#include <array>

namespace aifred {

struct DiagnosticPresentation {
  juce::String status = "Signal idle";
  juce::String summary = "Route audio through AIFRED.";
  juce::String fixList = "No measured issue yet.";
  juce::String aiContextJson = "{}";
  bool aiConfigured = false;
  bool setupRequired = true;
};

class DiagnosticInterpreter {
public:
  static DiagnosticInterpreter& instance();

  DiagnosticPresentation update(const HaloState& state, const juce::String& provider,
                                const juce::String& endpoint, const juce::String& apiKey,
                                const juce::String& model);

private:
  struct HealthSample {
    double timeMs = 0.0;
    float alignment = 0.0f;
    float lufs = -90.0f;
    float truePeak = -90.0f;
    float crest = 0.0f;
    float correlation = 1.0f;
  };

  std::array<HealthSample, 50> buffer_ {};
  int writeIndex_ = 0;
  int count_ = 0;
  double lastPollMs_ = 0.0;

  DiagnosticInterpreter() = default;
  void pushSample(const HaloState& state, double nowMs);
  static juce::String escapeJson(juce::String text);
  juce::String buildContextJson(const HaloState& state, juce::String provider, juce::String endpoint, juce::String model) const;
  static juce::String buildFixList(const HaloState& state);
};

} // namespace aifred
