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
  juce::MemoryOutputStream stream(destData, true);
  stream.writeString("AIFRED_STATE_V1");
}

void AifredAudioProcessor::setStateInformation(const void*, int) {}

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

} // namespace aifred

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new aifred::AifredAudioProcessor();
}
