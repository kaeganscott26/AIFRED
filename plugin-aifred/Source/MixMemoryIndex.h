#pragma once

#include "BetaView.h"
#include <juce_core/juce_core.h>
#include <deque>

namespace aifred {

struct MemoryTelemetry {
  int transientSnapshots=0,persistedSessions=0,expiredSnapshots=0;
  double newestAgeSeconds=0;
  core::ProfileId activeProfile=core::ProfileId::mixBalanced;
  bool updated=false,storageAvailable=false,hostAvailable=false;
};

class MixMemoryIndex final {
public:
  MixMemoryIndex();
  ~MixMemoryIndex();
  void observe(const BetaView&,AnalysisMode,const juce::String& reference,bool hostAvailable);
  MemoryTelemetry telemetry() const noexcept;
private:
  struct Snapshot {double timestamp=0;std::uint64_t fingerprint=0;};
  std::uint64_t candleFingerprint(const BetaView&) const noexcept;
  void prune(double now);
  void persist(const BetaView&,AnalysisMode,const juce::String&,double);
  void* database_=nullptr;
  std::deque<Snapshot> transient_;
  juce::String sessionId_;
  std::uint64_t lastFingerprint_=0;
  int expired_=0,persistedSessions_=0;
  bool storageAvailable_=false,hostAvailable_=false,updated_=false;
  core::ProfileId profile_=core::ProfileId::mixBalanced;
};

} // namespace aifred
