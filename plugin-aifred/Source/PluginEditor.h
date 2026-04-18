#pragma once

#include "AifredLookAndFeel.h"
#include "AifredEngineClient.h"
#include "DiagnosticInterpreter.h"
#include "PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace aifred {

class AifredAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer, private juce::Button::Listener {
public:
  explicit AifredAudioProcessorEditor(AifredAudioProcessor&);
  ~AifredAudioProcessorEditor() override;

  void paint(juce::Graphics&) override;
  void resized() override;

private:
  void timerCallback() override;
  void buttonClicked(juce::Button*);
  void drawHeader(juce::Graphics&, juce::Rectangle<int>);
  void drawHalo(juce::Graphics&, juce::Rectangle<int>, const HaloState&, const char* title, bool referenceOverlay);
  void drawDomainCard(juce::Graphics&, juce::Rectangle<int>, const char*, const DomainAlignment&);
  void drawCandles(juce::Graphics&, juce::Rectangle<int>, const HaloState&);
  void drawCandleStrip(juce::Graphics&, juce::Rectangle<int>, const HaloState&, bool minuteView);
  void drawChatPanel(juce::Graphics&, juce::Rectangle<int>);
  void drawHaloSpectrometer(juce::Graphics&, juce::Rectangle<float>, const HaloState&);
  void drawReferenceMixer(juce::Graphics&, juce::Rectangle<int>);
  void drawCompare(juce::Graphics&, juce::Rectangle<int>);
  void drawCompareVu(juce::Graphics&, juce::Rectangle<int>, const HaloState&, const HaloState&);
  void drawMixSignature(juce::Graphics&, juce::Rectangle<int>, const HaloState&);
  void drawSpectrumMeter(juce::Graphics&, juce::Rectangle<int>, const HaloState&);
  void drawCorrelationMeter(juce::Graphics&, juce::Rectangle<int>, const HaloState&);
  void pushSettingsToProcessor();

  AifredAudioProcessor& processor_;
  AifredLookAndFeel lookAndFeel_;
  juce::TextButton analyzeButton_ {"ANALYZE"};
  juce::TextButton referenceButton_ {"REFERENCE"};
  juce::TextButton compareButton_ {"COMPARE"};
  juce::TextButton optionsButton_ {"OPTIONS"};
  juce::TextButton tutorialButton_ {"TUTORIAL"};
  juce::TextButton askAiButton_ {"ASK AI"};
  juce::TextButton saveApiButton_ {"SAVE API"};
  juce::TextButton chatFileButton_ {"CHAT FILE"};
  juce::TextButton referenceFileButton_ {"REFERENCE FILE"};
  juce::TextButton compareFileButton_ {"COMPARE FILE"};
  juce::TextEditor chatInput_;
  juce::TextEditor apiEndpoint_;
  juce::TextEditor apiKey_;
  juce::TextEditor aiModel_;
  juce::Label fixList_;
  juce::ComboBox providerMenu_;
  juce::ComboBox themeMenu_;
  juce::ComboBox layoutMenu_;
  juce::ComboBox genreMenu_;
  juce::Slider gateSlider_;
  juce::Image mascot_;
  std::unique_ptr<juce::FileChooser> fileChooser_;
  HaloState state_;
  HaloState compareState_;
  DiagnosticPresentation diagnostic_;
  juce::String fixListText_ = "Chat output.";
  juce::String apiStatus_ = "API route not connected.";
  juce::String referenceStatus_ = "No reference file selected.";
  juce::String compareStatus_ = "No compare file selected.";
  juce::String chatFileStatus_ = "No chat file selected.";
  bool showTutorial_ = true;
  bool showOptions_ = false;
  bool splashDismissedThisEditor_ = false;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AifredAudioProcessorEditor)
};

} // namespace aifred
