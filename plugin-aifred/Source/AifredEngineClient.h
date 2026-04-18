#pragma once

#include <juce_core/juce_core.h>
#include <atomic>

namespace aifred {

class AifredEngineClient {
public:
  static AifredEngineClient& instance();

  void pingHealthAsync();
  bool isAvailable() const { return available_.load(); }
  juce::String statusText() const;

private:
  AifredEngineClient() = default;

  std::atomic<bool> available_ { false };
  std::atomic<bool> requestInFlight_ { false };
  juce::String lastStatus_ = "AI engine unavailable - core analysis still active.";
  mutable juce::CriticalSection statusLock_;
};

} // namespace aifred
