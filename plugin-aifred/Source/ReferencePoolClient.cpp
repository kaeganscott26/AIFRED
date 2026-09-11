#include "ReferencePoolClient.h"

#ifndef AIFRED_REFERENCE_POOL_URL
#define AIFRED_REFERENCE_POOL_URL "https://north3rnlight3r.com/api/v1/reference/pool"
#endif

namespace aifred {
namespace {
constexpr auto expectedContract = "aifred.references.v1";

ReferencePoolEntry parseEntry(const juce::var& value) {
  if (!value.isObject() || !static_cast<bool>(value.getProperty("available", false))) return {};
  ReferencePoolEntry result;
  result.id = value.getProperty("id", "").toString().trim().toStdString();
  auto name = value.getProperty("name", "").toString().trim();
  if (name.containsChar('\\') || name.containsChar('/')) name = juce::File(name).getFileName();
  result.name = name.toStdString();
  result.version = value.getProperty("version", "").toString().trim().toStdString();
  if (result.id.empty() || result.name.empty()) return {};
  return result;
}
} // namespace

ReferencePoolSnapshot parseReferencePool(const juce::String& json) {
  ReferencePoolSnapshot result;
  const auto root = juce::JSON::parse(json);
  if (!root.isObject()) {
    result.status = ReferencePoolSnapshot::Status::error;
    result.message = "Official reference pool returned invalid JSON.";
    return result;
  }
  result.contractVersion = root.getProperty("contract_version", "").toString().toStdString();
  if (result.contractVersion != expectedContract) {
    result.status = ReferencePoolSnapshot::Status::error;
    result.message = "Official reference pool contract is unsupported.";
    return result;
  }
  if (const auto* entries = root.getProperty("references", {}).getArray()) {
    result.entries.reserve(static_cast<std::size_t>(entries->size()));
    for (const auto& value : *entries) {
      auto entry = parseEntry(value);
      if (!entry.id.empty()) result.entries.push_back(std::move(entry));
    }
  }
  result.status = result.entries.empty() ? ReferencePoolSnapshot::Status::unavailable
                                         : ReferencePoolSnapshot::Status::available;
  result.message = result.entries.empty()
    ? root.getProperty("reason", "No Official references are available.").toString().toStdString()
    : std::to_string(result.entries.size()) + " Official reference records available.";
  return result;
}

ReferencePoolClient& ReferencePoolClient::instance() {
  static ReferencePoolClient client;
  return client;
}

bool ReferencePoolClient::refreshAsync() {
  bool expected = false;
  if (!requestInFlight_.compare_exchange_strong(expected, true)) return false;
  {
    const std::scoped_lock lock(mutex_);
    state_.status = ReferencePoolSnapshot::Status::loading;
    state_.message = "Loading Official reference pool...";
    state_.entries.clear();
    ++state_.revision;
  }
  worker_ = std::jthread([this] {
    int statusCode = 0;
    const juce::URL url(AIFRED_REFERENCE_POOL_URL);
    const auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
      .withConnectionTimeoutMs(5000).withNumRedirectsToFollow(2).withStatusCode(&statusCode);
    ReferencePoolSnapshot next;
    if (auto stream = url.createInputStream(options)) {
      next = parseReferencePool(stream->readEntireStreamAsString());
      if (statusCode < 200 || statusCode >= 300) {
        next = {};
        next.status = ReferencePoolSnapshot::Status::error;
        next.message = "Official reference pool request failed.";
      }
    } else {
      next.status = ReferencePoolSnapshot::Status::error;
      next.message = "Official reference pool is unavailable.";
    }
    {
      const std::scoped_lock lock(mutex_);
      next.revision = state_.revision + 1;
      state_ = std::move(next);
    }
    requestInFlight_.store(false);
  });
  return true;
}

ReferencePoolSnapshot ReferencePoolClient::state() const {
  const std::scoped_lock lock(mutex_);
  return state_;
}

} // namespace aifred
