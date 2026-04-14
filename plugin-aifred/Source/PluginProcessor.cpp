#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace aifred {

AifredAudioProcessor::AifredAudioProcessor()
  : AudioProcessor(BusesProperties()
      .withInput("Input", juce::AudioChannelSet::stereo(), true)
      .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

void AifredAudioProcessor::prepareToPlay(double sampleRate, int) {
  analysis_.prepare(sampleRate);
}

void AifredAudioProcessor::releaseResources() {
  analysis_.reset();
}

bool AifredAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  const auto input = layouts.getMainInputChannelSet();
  const auto output = layouts.getMainOutputChannelSet();
  return input == output && (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo());
}

void AifredAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
  juce::ScopedNoDenormals noDenormals;
  analysis_.pushAudioBlock(buffer);
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
  return analysis_.snapshot();
}

} // namespace aifred

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new aifred::AifredAudioProcessor();
}
