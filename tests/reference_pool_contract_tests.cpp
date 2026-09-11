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
      {"available":true,"id":"one","name":"C:\\\\private\\track.wav","version":"website-analyzer.v1","metrics":{"integrated_lufs":-11.2}},
      {"available":true,"id":"two","name":"public.wav","version":"website-analyzer.v1","metrics":{"integrated_lufs":-12.0}}
    ]
  })json");
  check(parsed.status == aifred::ReferencePoolSnapshot::Status::available, "canonical pool is available");
  check(parsed.contractVersion == "aifred.references.v1", "canonical contract version is required");
  check(parsed.entries.size() == 2, "all public pool records are read");
  check(parsed.entries[0].name == "track.wav", "path components are removed from display names");
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
      check(live.entries.size() >= 22, "live Official pool contains the migrated reference records");
    }
  }
  if (failures != 0) return 1;
  std::cout << "Beta Official reference-pool contract tests: PASS\n";
  return 0;
}
