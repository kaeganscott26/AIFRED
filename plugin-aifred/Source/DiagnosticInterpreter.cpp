#include "DiagnosticInterpreter.h"

#include <cmath>

namespace aifred {
namespace {

juce::String band(float value, float low, float high, const char* below, const char* inside, const char* above) {
  if (value < low) return below;
  if (value > high) return above;
  return inside;
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
  const auto normalizedProvider = provider.trim().isNotEmpty() ? provider.trim() : "openai";
  output.aiConfigured = endpoint.trim().isNotEmpty()
    && (normalizedProvider.equalsIgnoreCase("ollama") || apiKey.trim().isNotEmpty());
  output.setupRequired = !output.aiConfigured;
  output.aiContextJson = buildContextJson(state, normalizedProvider, endpoint.trim(), model.trim());

  if (state.metrics.loudness01 < 0.02f) {
    output.status = "Signal idle";
    output.summary = "DSP is armed. AI stays disabled until provider setup is complete.";
    output.fixList = output.aiConfigured ? "Play audio to build the 5-second context buffer." : "Configure provider, endpoint, model, and API key to enable AI interpretation.";
    return output;
  }

  const auto loudnessBand = band(state.metrics.shortTermLufs, -14.0f, -9.0f, "below streaming target", "inside -14 to -9 LUFS", "hot for streaming targets");
  const auto truePeakBand = state.metrics.truePeakDb <= 0.0f ? "true peak under ceiling" : "true peak over 0 dBTP";
  const auto crestBand = state.metrics.crestDb < 8.0f ? "over-compressed" : (state.metrics.crestDb > 18.0f ? "very spiky" : "controlled punch");
  const auto phaseBand = state.metrics.correlation < 0.2f ? "phase cancellation risk" : "mono-compatible correlation";

  output.status = state.primaryTitle.c_str();
  output.summary = juce::String(loudnessBand) + ", " + truePeakBand + ", " + crestBand + ", " + phaseBand + ".";
  output.fixList = buildFixList(state);
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
      "\"stereo_width\":" + juce::String(state.metrics.width01, 3)
    + "},\"mix_health_5s\":[" + samples.joinIntoString(",") + "]}";
}

juce::String DiagnosticInterpreter::buildFixList(const HaloState& state) {
  juce::StringArray lines;
  if (state.metrics.truePeakDb > 0.0f) {
    lines.add("CRITICAL - True peak is above 0 dBTP. Lower limiter output or reduce clip gain before export.");
  } else if (state.metrics.truePeakDb > -1.0f) {
    lines.add("WATCH - True peak is close to the ceiling. Leave more room if the target is streaming delivery.");
  }

  if (state.metrics.shortTermLufs > -9.0f) {
    lines.add("HIGH - Short-term loudness is hot. Check limiter gain and compare against the selected genre target.");
  } else if (state.metrics.shortTermLufs < -14.0f && state.metrics.loudness01 > 0.05f) {
    lines.add("MID - Short-term loudness is below the main target zone. Raise level only after tone and crest are stable.");
  }

  if (state.metrics.crestDb < 8.0f && state.metrics.loudness01 > 0.05f) {
    lines.add("HIGH - Crest factor is under 8 dB. The mix is likely over-compressed or clipped.");
  } else if (state.metrics.crestDb > 18.0f) {
    lines.add("MID - Crest factor is very high. Control isolated spikes before pushing loudness.");
  }

  if (state.metrics.correlation < 0.2f) {
    lines.add("HIGH - Stereo correlation is below 0.2. Check side-heavy low mids and mono compatibility.");
  }

  if (state.tone.error01 > 0.35f) {
    lines.add("MID - Spectral tilt is outside the reference corridor. Balance low-end weight against high-mid brightness.");
  }

  if (lines.isEmpty()) {
    lines.add("PASS - No critical measured issue. Keep checking after the loudest section plays through.");
  }
  return lines.joinIntoString("\n");
}

} // namespace aifred
