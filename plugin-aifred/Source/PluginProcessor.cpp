#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace aifred {

AifredAudioProcessor::AifredAudioProcessor()
  : AudioProcessor(BusesProperties()
      .withInput("Mix A", juce::AudioChannelSet::stereo(), true)
      .withInput("Mix B", juce::AudioChannelSet::stereo(), false)
      .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

void AifredAudioProcessor::prepareToPlay(double sampleRate, int) {
  analysis_.prepare(sampleRate);
  compareAnalysis_.prepare(sampleRate);
}

void AifredAudioProcessor::releaseResources() {
  analysis_.reset();
  compareAnalysis_.reset();
}

bool AifredAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  const auto input = layouts.getChannelSet(true, 0);
  const auto output = layouts.getMainOutputChannelSet();
  if (input != output || !(input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo())) return false;
  if (layouts.inputBuses.size() > 1) {
    const auto compare = layouts.getChannelSet(true, 1);
    if (!compare.isDisabled() && compare != juce::AudioChannelSet::mono() && compare != juce::AudioChannelSet::stereo()) return false;
  }
  return true;
}

void AifredAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
  juce::ScopedNoDenormals noDenormals;
  auto mixA = getBusBuffer(buffer, true, 0);
  analysis_.pushAudioBlock(mixA);
  if (getBusCount(true) > 1 && !getBus(true, 1)->isEnabled()) {
    compareAnalysis_.reset();
  } else if (getBusCount(true) > 1) {
    auto mixB = getBusBuffer(buffer, true, 1);
    compareAnalysis_.pushAudioBlock(mixB);
  }
}

juce::AudioProcessorEditor* AifredAudioProcessor::createEditor() {
  return new AifredAudioProcessorEditor(*this);
}

void AifredAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
  juce::XmlElement state("AIFRED_STATE");
  state.setAttribute("version", 2);
  state.setAttribute("mode", mode_.load());
  state.setAttribute("theme", settings_.themeId);
  state.setAttribute("layout", settings_.layoutId);
  state.setAttribute("gate", settings_.gate);
  state.setAttribute("apiEndpoint", settings_.apiEndpoint);
  state.setAttribute("apiKey", settings_.apiKey);
  copyXmlToBinary(state, destData);
}

void AifredAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
  auto state = getXmlFromBinary(data, sizeInBytes);
  if (!state || !state->hasTagName("AIFRED_STATE")) return;
  mode_.store(juce::jlimit(0, 2, state->getIntAttribute("mode", static_cast<int>(AnalysisMode::Analyze))));
  settings_.themeId = juce::jlimit(1, 3, state->getIntAttribute("theme", settings_.themeId));
  settings_.layoutId = juce::jlimit(1, 3, state->getIntAttribute("layout", settings_.layoutId));
  settings_.gate = juce::jlimit(0.0, 1.0, state->getDoubleAttribute("gate", settings_.gate));
  settings_.apiEndpoint = state->getStringAttribute("apiEndpoint", settings_.apiEndpoint).substring(0, 256);
  settings_.apiKey = state->getStringAttribute("apiKey", settings_.apiKey).substring(0, 256);
}

HaloState AifredAudioProcessor::getHaloState() const {
  auto state = analysis_.snapshot();
  state.mode = getMode();
  return state;
}

HaloState AifredAudioProcessor::getCompareHaloState() const {
  auto state = compareAnalysis_.snapshot();
  state.mode = AnalysisMode::Compare;
  return state;
}

AnalysisMode AifredAudioProcessor::getMode() const {
  return static_cast<AnalysisMode>(mode_.load());
}

void AifredAudioProcessor::setMode(AnalysisMode mode) {
  mode_.store(static_cast<int>(mode));
}

PluginSettings AifredAudioProcessor::getPluginSettings() const {
  return settings_;
}

void AifredAudioProcessor::setPluginSettings(const PluginSettings& settings) {
  settings_.themeId = juce::jlimit(1, 3, settings.themeId);
  settings_.layoutId = juce::jlimit(1, 3, settings.layoutId);
  settings_.gate = juce::jlimit(0.0, 1.0, settings.gate);
  settings_.apiEndpoint = settings.apiEndpoint.substring(0, 256);
  settings_.apiKey = settings.apiKey.substring(0, 256);
}

} // namespace aifred

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new aifred::AifredAudioProcessor();
}
