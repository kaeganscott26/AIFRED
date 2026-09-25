#include "plugin-aifred/Source/ReferencePoolClient.h"

#include <iostream>

int main(int argc, char** argv) {
  int failures = 0;
  const auto check = [&](bool condition, const char* message) {
    if (condition) return;
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  };
    const auto parsed = aifred::parseReferencePool(R"json({
    "contract_version":"aifred.references.v1",
    "references":[
      {"available":true,"id":"one","name":"C:\\\\private\\track.wav","version":"website-analyzer.v1","metrics":{"tone_balance":63,"integrated_lufs":-11.2,"peak_dbfs":-0.8,"crest_factor_db":9.4,"stereo_width":0.2,"low_end_control":34,"harshness_control":71,"spectral_centroid_hz":18154}},
      {"available":true,"id":"two","name":"public.wav","version":"website-analyzer.v1","metrics":{"integrated_lufs":{"typical":-12.0,"low":-13.0,"high":-11.0},"stereo_width":0.25}}
    ]
  })json");
  check(parsed.status == aifred::ReferencePoolSnapshot::Status::available, "canonical pool is available");
  check(parsed.contractVersion == "aifred.references.v1", "canonical contract version is required");
  check(parsed.entries.size() == 2, "all public pool records are read");
  check(parsed.entries[0].name == "track.wav", "path components are removed from display names");
  check(parsed.entries[0].distribution.available, "actual DSP measurements survive pool parsing");
  check(parsed.entries[0].distribution.metrics[aifred::core::index(aifred::core::MetricId::samplePeak)].valid &&
        parsed.entries[0].distribution.metrics[aifred::core::index(aifred::core::MetricId::samplePeak)].typical == -0.8,
        "peak_dbfs maps to canonical sample peak without claiming true peak");
  check(!parsed.entries[0].distribution.metrics[aifred::core::index(aifred::core::MetricId::truePeak)].valid,
        "missing true peak remains unavailable");
  check(parsed.entries[0].distribution.metrics[aifred::core::index(aifred::core::MetricId::width)].typical == 20.0,
        "stereo width ratio converts to canonical percent");
  check(parsed.entries[0].distribution.metrics[aifred::core::index(aifred::core::MetricId::integrated)].typical == -11.2 &&
        parsed.entries[0].distribution.metrics[aifred::core::index(aifred::core::MetricId::crest)].typical == 9.4,
        "integrated loudness and crest map to canonical DSP identities");
  check(parsed.entries[0].suppliedMeasurements.size() == 8 && parsed.entries[0].suppliedMeasurements.count("tone_balance") == 1,
        "non-canonical source measurements are preserved rather than discarded");
  check(!parsed.entries[0].distribution.compatibilityKnown,
        "live pool records do not claim absent profile and sample-rate metadata");
  const auto selected = parsed.entries[0].distribution;
  check(selected.available && selected.metrics[aifred::core::index(aifred::core::MetricId::integrated)].valid,
        "selecting a parsed Official entry yields a measured reference target");
  check(parsed.entries[1].distribution.metrics[aifred::core::index(aifred::core::MetricId::integrated)].hasDistribution &&
        parsed.entries[1].distribution.metrics[aifred::core::index(aifred::core::MetricId::integrated)].low == -13.0,
        "supplied reference bounds are preserved only when the payload supplies them");
  const auto stale = aifred::parseReferencePool(R"json({"contract_version":"old","references":[]})json");
  check(stale.status == aifred::ReferencePoolSnapshot::Status::error, "stale contracts are rejected");
  if (argc == 3 && std::string(argv[1]) == "--live") {
    int statusCode = 0;
    const juce::URL url(argv[2]);
    const auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
      .withConnectionTimeoutMs(10000).withNumRedirectsToFollow(2).withStatusCode(&statusCode);
    auto stream = url.createInputStream(options);
    check(stream != nullptr && statusCode >= 200 && statusCode < 300, "live Official pool responds successfully");
    if (stream != nullptr) {
      const auto live = aifred::parseReferencePool(stream->readEntireStreamAsString());
      check(live.status == aifred::ReferencePoolSnapshot::Status::available, "live Official pool parses as available");
      check(live.entries.size() >= 24, "live Official pool contains the current production reference records");
    }
  }
  if (failures != 0) return 1;
  std::cout << "Beta Official reference-pool contract tests: PASS\n";
  return 0;
}
