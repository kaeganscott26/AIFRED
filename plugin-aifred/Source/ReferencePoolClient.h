#pragma once

#include <juce_core/juce_core.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace aifred {

struct ReferencePoolEntry {
  std::string id;
  std::string name;
  std::string version;
};

struct ReferencePoolSnapshot {
  enum class Status { idle, loading, available, unavailable, error };
  Status status = Status::idle;
  std::string contractVersion;
  std::string message;
  std::vector<ReferencePoolEntry> entries;
  std::uint64_t revision = 0;
};

[[nodiscard]] ReferencePoolSnapshot parseReferencePool(const juce::String& json);

class ReferencePoolClient final {
public:
  static ReferencePoolClient& instance();
  bool refreshAsync();
  [[nodiscard]] ReferencePoolSnapshot state() const;

private:
  ReferencePoolClient() = default;
  mutable std::mutex mutex_;
  ReferencePoolSnapshot state_;
  std::atomic<bool> requestInFlight_ {false};
  std::jthread worker_;
};

} // namespace aifred
