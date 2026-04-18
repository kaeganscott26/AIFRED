#pragma once

#include "AnalysisEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

namespace aifred {

struct PluginSettings {
  int themeId = 1;
  int layoutId = 1;
  int genreId = 1;
  double gate = 0.35;
  juce::String aiProvider = "openai";
  juce::String apiEndpoint;
  juce::String apiKey;
  juce::String aiModel = "gpt-5.4-mini";
};

class AifredAudioProcessor : public juce::AudioProcessor {
public:
  AifredAudioProcessor();
  ~AifredAudioProcessor() override = default;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override { return "Aifred"; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String&) override {}

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  HaloState getHaloState() const;
  HaloState getCompareHaloState() const;
  AnalysisMode getMode() const;
  void setMode(AnalysisMode mode);
  PluginSettings getPluginSettings() const;
  void setPluginSettings(const PluginSettings& settings);
  bool isSessionInitialized() const;
  void markSessionInitialized();

private:
  static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
  void loadLocalSettings();
  void saveLocalSettings() const;

  AnalysisEngine analysis_;
  AnalysisEngine compareAnalysis_;
  std::atomic<int> mode_ { static_cast<int>(AnalysisMode::Analyze) };
  PluginSettings settings_;
  juce::AudioProcessorValueTreeState parameters_;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AifredAudioProcessor)
};

} // namespace aifred
