#include "ReferencePoolClient.h"

#include <cmath>
#include <optional>

#ifndef AIFRED_REFERENCE_POOL_URL
#define AIFRED_REFERENCE_POOL_URL "https://north3rnlight3r.com/api/v1/reference/pool"
#endif

namespace aifred {
namespace {
constexpr auto expectedContract = "aifred.references.v1";

juce::var property(const juce::var& object, const char* name) {
  return object.isObject() ? object.getProperty(name, juce::var()) : juce::var();
}

std::optional<double> finiteNumber(const juce::var& value) {
  if (!(value.isDouble() || value.isInt() || value.isInt64())) return std::nullopt;
  const auto number = static_cast<double>(value);
  return std::isfinite(number) ? std::optional<double>(number) : std::nullopt;
}

core::MetricObservation parseMeasurement(const juce::var& raw) {
  core::MetricObservation result;
  const auto typicalValue = raw.isObject()
    ? property(raw, "typical").isVoid() ? property(raw, "value") : property(raw, "typical")
    : raw;
  const auto typical = finiteNumber(typicalValue);
  if (!typical) return result;
  result.valid = true;
  result.typical = result.latest = *typical;
  result.count = 1;

  const auto latest = finiteNumber(property(raw, "latest"));
  if (latest) result.latest = *latest;
  const auto low = finiteNumber(property(raw, "low").isVoid() ? property(raw, "minimum") : property(raw, "low"));
  const auto high = finiteNumber(property(raw, "high").isVoid() ? property(raw, "maximum") : property(raw, "high"));
  if (low) result.low = *low;
  if (high) result.high = *high;
  const auto minimum = finiteNumber(property(raw, "minimum"));
  const auto maximum = finiteNumber(property(raw, "maximum"));
  if (minimum) result.minimum = *minimum;
  if (maximum) result.maximum = *maximum;
  result.hasDistribution = low.has_value() && high.has_value();
  return result;
}

core::MetricObservation scaleMeasurement(core::MetricObservation value, double scale) {
  if (!value.valid || !std::isfinite(scale)) return {};
  value.typical *= scale;
  value.low *= scale;
  value.high *= scale;
  value.minimum *= scale;
  value.maximum *= scale;
  value.latest *= scale;
  return value;
}

void assignMetric(ReferencePoolEntry& entry, const juce::var& metrics,
                  core::MetricId id, std::initializer_list<const char*> names,
                  double scale = 1.0) {
  for (const auto* name : names) {
    const auto measurement = parseMeasurement(property(metrics, name));
    if (!measurement.valid) continue;
    entry.distribution.metrics[core::index(id)] = scaleMeasurement(measurement, scale);
    return;
  }
}

void readCompatibilityMetadata(ReferencePoolEntry& entry, const juce::var& value) {
  const auto schema = finiteNumber(property(value, "schema_version"));
  const auto profileVersion = finiteNumber(property(value, "profile_version"));
  const auto sampleRate = finiteNumber(property(value, "sample_rate_hz"));
  const auto profileName = property(value, "profile_id").toString().toStdString();
  if (!schema || !profileVersion || !sampleRate || profileName.empty()) return;
  const auto profileId = core::profileFromName(profileName);
  if (core::profile(profileId).name != profileName) return;
  entry.distribution.schema = static_cast<std::uint32_t>(*schema);
  entry.distribution.profileId = profileId;
  entry.distribution.profileVersion = static_cast<std::uint32_t>(*profileVersion);
  entry.distribution.sampleRate = *sampleRate;
  entry.distribution.compatibilityKnown = true;
}

ReferencePoolEntry parseEntry(const juce::var& value) {
  if (!value.isObject() || !static_cast<bool>(value.getProperty("available", false))) return {};
  ReferencePoolEntry result;
  result.id = value.getProperty("id", "").toString().trim().toStdString();
  auto name = value.getProperty("name", "").toString().trim();
  name = name.replaceCharacter('\\', '/');
  if (name.containsChar('/')) name = name.fromLastOccurrenceOf("/", false, false);
  result.name = name.toStdString();
  result.version = value.getProperty("version", "").toString().trim().toStdString();
  if (result.id.empty() || result.name.empty()) return {};
  result.distribution.compatibilityKnown = false;

  const auto metrics = property(value, "metrics");
  if (const auto* object = metrics.getDynamicObject()) {
    const auto& properties = object->getProperties();
    for (int i = 0; i < properties.size(); ++i) {
      const auto metric = parseMeasurement(properties.getValueAt(i));
      if (metric.valid)
        result.suppliedMeasurements.emplace(properties.getName(i).toString().toStdString(), metric.typical);
    }
  }
  // These are the fields actually present in the production public pool. The
  // API's peak_dbfs is a sample peak, not a true peak, and stereo_width is a
  // 0..1 share while the shared core canonical width is percent.
  assignMetric(result, metrics, core::MetricId::samplePeak, {"sample_peak", "sample_peak_dbfs", "peak_dbfs"});
  assignMetric(result, metrics, core::MetricId::rms, {"rms", "rms_dbfs"});
  assignMetric(result, metrics, core::MetricId::truePeak, {"true_peak", "true_peak_dbtp"});
  assignMetric(result, metrics, core::MetricId::momentary, {"momentary_loudness", "momentary_lufs"});
  assignMetric(result, metrics, core::MetricId::shortTerm, {"short_term_loudness", "short_term_lufs"});
  assignMetric(result, metrics, core::MetricId::integrated, {"integrated_loudness", "integrated_lufs"});
  assignMetric(result, metrics, core::MetricId::lra, {"loudness_range", "loudness_range_lu"});
  assignMetric(result, metrics, core::MetricId::crest, {"broadband_crest", "crest_factor_db", "crest_db"});
  assignMetric(result, metrics, core::MetricId::correlation, {"correlation"});
  assignMetric(result, metrics, core::MetricId::leftEnergy, {"left_energy"});
  assignMetric(result, metrics, core::MetricId::rightEnergy, {"right_energy"});
  assignMetric(result, metrics, core::MetricId::midEnergy, {"mid_energy"});
  assignMetric(result, metrics, core::MetricId::sideEnergy, {"side_energy"});
  assignMetric(result, metrics, core::MetricId::balance, {"left_right_balance"});
  assignMetric(result, metrics, core::MetricId::sideToMid, {"side_to_mid"});
  assignMetric(result, metrics, core::MetricId::width, {"width"});
  assignMetric(result, metrics, core::MetricId::width, {"stereo_width"}, 100.0);
  for (const auto& metric : result.distribution.metrics)
    if (metric.valid) { result.distribution.available = true; break; }
  result.distribution.id = result.id;
  readCompatibilityMetadata(result, value);
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
