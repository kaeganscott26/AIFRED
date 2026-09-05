#pragma once
#include "aifred/IntelligenceClient.h"

#include "BetaView.h"

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

  BetaView getView() const;
  BetaView getCompareView() const;
  AnalysisMode getMode() const;
  void setMode(AnalysisMode mode);
  PluginSettings getPluginSettings() const;
  void setPluginSettings(const PluginSettings& settings);
  void setReferenceTarget(const ReferenceTarget& target);
  void clearReferenceTarget();
  bool isSessionInitialized() const;
  void markSessionInitialized();
  core::Pipeline& pipeline() noexcept {return analysis_;}
  core::Pipeline& comparePipeline() noexcept {return compareAnalysis_;}
  ReferenceTarget referenceTarget() const {return reference_;}

    aifred::core::IntelligenceClient& intelligence() noexcept {return intelligence_;}

private:
  static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
  void loadLocalSettings();
  void saveLocalSettings() const;


  core::Pipeline analysis_ {"beta","0.3.6"};
  core::Pipeline compareAnalysis_ {"beta","0.3.6"};
  // Client joins before the context owner is destroyed, independently of editor lifetime.
  core::IntelligenceClient intelligence_ {"beta", [this](juce::String response){analysis_.recordResponse(response);} };
  ReferenceTarget reference_;
  std::atomic<int> mode_ { static_cast<int>(AnalysisMode::Analyze) };
  PluginSettings settings_;
  juce::AudioProcessorValueTreeState parameters_;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AifredAudioProcessor)
};

} // namespace aifred
