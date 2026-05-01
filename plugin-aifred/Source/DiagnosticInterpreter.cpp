#include "DiagnosticInterpreter.h"

#include <cmath>

namespace aifred {
namespace {

juce::String boolJson(bool value) {
  return value ? "true" : "false";
}

juce::String numberJson(float value, int decimals) {
  return std::isfinite(value) ? juce::String(value, decimals) : "null";
}

juce::String percentJson(int value) {
  return juce::String(juce::jlimit(0, 100, value));
}

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
    samples.add("{\"window\":\"5s_history\",\"alignment\":" + numberJson(sample.alignment, 3)
      + ",\"short_term_lufs\":" + numberJson(sample.lufs, 1)
      + ",\"true_peak_dbtp\":" + numberJson(sample.truePeak, 1)
      + ",\"crest_db\":" + numberJson(sample.crest, 1)
      + ",\"correlation\":" + numberJson(sample.correlation, 3) + "}");
  }

  return "{"
    "\"provider\":\"" + escapeJson(provider) + "\","
    "\"endpoint\":\"" + escapeJson(endpoint) + "\","
    "\"model\":\"" + escapeJson(model) + "\","
    "\"mode\":" + juce::String(static_cast<int>(state.mode)) + ","
    "\"analysis_snapshot\":{"
      "\"human_summary\":\"" + escapeJson(state.humanSummary.c_str()) + "\","
      "\"has_signal\":" + boolJson(state.hasSignal) + ","
      "\"has_reference\":" + boolJson(state.hasReference) + ","
      "\"values_valid\":" + boolJson(state.valuesValid) + ","
      "\"is_stale\":" + boolJson(state.isStale) + ","
      "\"is_using_fallback_score\":" + boolJson(state.isUsingFallbackScore) + ","
      "\"current_snapshot_timestamp_ms\":" + juce::String(state.currentSnapshotTimestampMs, 0) + ","
      "\"history_window_ms\":" + juce::String(state.historyWindowMs, 0) + ","
      "\"tone_score01\":" + numberJson(state.toneScore01, 3) + ","
      "\"width_score01\":" + numberJson(state.widthScore01, 3) + ","
      "\"punch_score01\":" + numberJson(state.punchScore01, 3) + ","
      "\"loudness_score01\":" + numberJson(state.loudnessScore01, 3) + ","
      "\"overall_score01\":" + numberJson(state.totalAlignment01, 3) + ","
      "\"displayed_tone_percent\":" + percentJson(state.displayedTonePercent) + ","
      "\"displayed_width_percent\":" + percentJson(state.displayedWidthPercent) + ","
      "\"displayed_punch_percent\":" + percentJson(state.displayedPunchPercent) + ","
      "\"displayed_loudness_percent\":" + percentJson(state.displayedLoudnessPercent)
    + "},"
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
      "\"window\":\"current_analysis_window\","
      "\"momentary_lufs\":" + numberJson(state.metrics.momentaryLufs, 1) + ","
      "\"short_term_lufs\":" + numberJson(state.metrics.shortTermLufs, 1) + ","
      "\"integrated_lufs\":" + numberJson(state.metrics.integratedLufs, 1) + ","
      "\"lra\":" + numberJson(state.metrics.loudnessRange, 1) + ","
      "\"sample_peak_dbfs\":" + numberJson(state.metrics.peakDb, 1) + ","
      "\"true_peak_dbtp\":" + numberJson(state.metrics.truePeakDb, 1) + ","
      "\"crest_db\":" + numberJson(state.metrics.crestDb, 1) + ","
      "\"correlation\":" + numberJson(state.metrics.correlation, 3) + ","
      "\"spectral_tilt\":" + numberJson(state.metrics.tone01, 3) + ","
      "\"stereo_width\":" + numberJson(state.metrics.width01, 3) + ","
      "\"punch\":" + numberJson(state.metrics.punch01, 3)
    + "},\"mix_health_5s\":[" + samples.joinIntoString(",") + "]}";
}

} // namespace aifred
