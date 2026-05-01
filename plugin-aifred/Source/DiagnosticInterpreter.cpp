#include "DiagnosticInterpreter.h"

#include <cmath>

namespace aifred {
namespace {

} // namespace

DiagnosticInterpreter& DiagnosticInterpreter::instance() {
  static DiagnosticInterpreter interpreter;
  return interpreter;
}

DiagnosticPresentation DiagnosticInterpreter::update(const HaloState& state, const juce::String& provider,
                                                     const juce::String& endpoint, const juce::String& apiKey,
                                                     const juce::String& model) {
  const auto nowMs = static_cast<double>(juce::Time::getMillisecondCounterHiRes());
  if (nowMs - lastPollMs_ >= 100.0) {
    pushSample(state, nowMs);
    lastPollMs_ = nowMs;
  }

  DiagnosticPresentation output;
  const auto normalizedProvider = provider.trim().isNotEmpty() ? provider.trim() : "ollama";
  output.aiConfigured = endpoint.trim().isNotEmpty()
    && (normalizedProvider.equalsIgnoreCase("ollama") || apiKey.trim().isNotEmpty());
  output.setupRequired = !output.aiConfigured;
  output.aiContextJson = buildContextJson(state, normalizedProvider, endpoint.trim(), model.trim());
  output.status = "metrics";
  output.summary = "live DSP snapshot";
  output.fixList = "";
  return output;
}

void DiagnosticInterpreter::pushSample(const HaloState& state, double nowMs) {
  buffer_[static_cast<size_t>(writeIndex_)] = {
    nowMs,
    state.totalAlignment01,
    state.metrics.shortTermLufs,
    state.metrics.truePeakDb,
    state.metrics.crestDb,
    state.metrics.correlation
  };
  writeIndex_ = (writeIndex_ + 1) % static_cast<int>(buffer_.size());
  count_ = std::min(static_cast<int>(buffer_.size()), count_ + 1);
}

juce::String DiagnosticInterpreter::escapeJson(juce::String text) {
  text = text.replace("\\", "\\\\");
  text = text.replace("\"", "\\\"");
  text = text.replace("\n", "\\n");
  return text;
}

juce::String DiagnosticInterpreter::buildContextJson(const HaloState& state, juce::String provider, juce::String endpoint, juce::String model) const {
  juce::StringArray samples;
  const auto nowMs = static_cast<double>(juce::Time::getMillisecondCounterHiRes());
  for (int i = 0; i < count_; ++i) {
    const auto index = (writeIndex_ - count_ + i + static_cast<int>(buffer_.size())) % static_cast<int>(buffer_.size());
    const auto& sample = buffer_[static_cast<size_t>(index)];
    if (nowMs - sample.timeMs > 5000.0) continue;
    samples.add("{\"alignment\":" + juce::String(sample.alignment, 3)
      + ",\"short_term_lufs\":" + juce::String(sample.lufs, 2)
      + ",\"true_peak_dbtp\":" + juce::String(sample.truePeak, 2)
      + ",\"crest_db\":" + juce::String(sample.crest, 2)
      + ",\"correlation\":" + juce::String(sample.correlation, 3) + "}");
  }

  return "{"
    "\"provider\":\"" + escapeJson(provider) + "\","
    "\"endpoint\":\"" + escapeJson(endpoint) + "\","
    "\"model\":\"" + escapeJson(model) + "\","
    "\"mode\":" + juce::String(static_cast<int>(state.mode)) + ","
    "\"reference_pool\":{"
      "\"label\":\"" + escapeJson(state.reference.label.c_str()) + "\","
      "\"pool_size\":" + juce::String(state.reference.poolSize) + ","
      "\"target_tone\":" + juce::String(state.reference.tone01, 3) + ","
      "\"target_width\":" + juce::String(state.reference.width01, 3) + ","
      "\"target_punch\":" + juce::String(state.reference.punch01, 3) + ","
      "\"target_loudness_lufs\":" + juce::String(state.reference.loudnessDb, 2) + ","
      "\"target_crest_db\":" + juce::String(state.reference.crestDb, 2)
    + "},"
    "\"current\":{"
      "\"momentary_lufs\":" + juce::String(state.metrics.momentaryLufs, 2) + ","
      "\"short_term_lufs\":" + juce::String(state.metrics.shortTermLufs, 2) + ","
      "\"integrated_lufs\":" + juce::String(state.metrics.integratedLufs, 2) + ","
      "\"lra\":" + juce::String(state.metrics.loudnessRange, 2) + ","
      "\"sample_peak_dbfs\":" + juce::String(state.metrics.peakDb, 2) + ","
      "\"true_peak_dbtp\":" + juce::String(state.metrics.truePeakDb, 2) + ","
      "\"crest_db\":" + juce::String(state.metrics.crestDb, 2) + ","
      "\"correlation\":" + juce::String(state.metrics.correlation, 3) + ","
      "\"spectral_tilt\":" + juce::String(state.metrics.tone01, 3) + ","
      "\"stereo_width\":" + juce::String(state.metrics.width01, 3) + ","
      "\"punch\":" + juce::String(state.metrics.punch01, 3)
    + "},\"mix_health_5s\":[" + samples.joinIntoString(",") + "]}";
}

} // namespace aifred
