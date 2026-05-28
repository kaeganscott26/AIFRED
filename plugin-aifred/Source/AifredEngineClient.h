#pragma once

#include <juce_core/juce_core.h>
#include <atomic>

namespace aifred {

class AifredEngineClient {
public:
  static AifredEngineClient& instance();

  void pingHealthAsync();
  void askAsync(juce::String prompt, juce::String contextJson);
  void saveSettingsAsync(juce::String provider, juce::String endpoint, juce::String apiKey, juce::String model);
  bool isAvailable() const { return available_.load(); }
  juce::String statusText() const;
  juce::String lastResponse() const;
  bool hasPendingChat() const { return chatInFlight_.load(); }

private:
  AifredEngineClient() = default;

  std::atomic<bool> available_ { false };
  std::atomic<bool> requestInFlight_ { false };
  std::atomic<bool> chatInFlight_ { false };
  juce::String lastStatus_ = "Local AI status unavailable.";
  juce::String lastResponse_;
  mutable juce::CriticalSection statusLock_;
};

} // namespace aifred
