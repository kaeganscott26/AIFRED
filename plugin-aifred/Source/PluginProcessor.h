#pragma once

#include "AnalysisEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

namespace aifred {

struct PluginSettings {
  int themeId = 1;
  int layoutId = 3;
  int genreId = 1;
  double gate = 0.35;
  juce::String aiProvider = "ollama";
  juce::String apiEndpoint = "http://127.0.0.1:11434";
  juce::String apiKey;
  juce::String aiModel = "aifred:latest";
};

class AifredAudioProcessor : public juce::AudioProcessor {
public:
  AifredAudioProcessor();
  ~AifredAudioProcessor() override;

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
  void setReferenceTarget(const ReferenceTarget& target);
  void clearReferenceTarget();
  bool isSessionInitialized() const;
  void markSessionInitialized();

private:
  static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
  void loadLocalSettings();
  void saveLocalSettings() const;
  void loadSessionHistory();
  void saveSessionHistory();

  AnalysisEngine analysis_;
  AnalysisEngine compareAnalysis_;
  std::atomic<int> mode_ { static_cast<int>(AnalysisMode::Analyze) };
  PluginSettings settings_;
  juce::AudioProcessorValueTreeState parameters_;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AifredAudioProcessor)
};

} // namespace aifred
