#pragma once

#include "AifredLookAndFeel.h"


#include "PluginProcessor.h"
#include "ReferencePoolClient.h"
#include "MixMemoryIndex.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <memory>
#include <string>

namespace aifred {

class AifredAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer, private juce::Button::Listener {
public:
  explicit AifredAudioProcessorEditor(AifredAudioProcessor&);
  ~AifredAudioProcessorEditor() override;

  void paint(juce::Graphics&) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent&) override;
  bool keyPressed(const juce::KeyPress&) override;

private:
  enum class CandleStripType 
  {
    Session,
    Minute,
    Live
  };
  
  void timerCallback() override;
  void buttonClicked(juce::Button*) override;
  
  void drawHeader(juce::Graphics&, juce::Rectangle<int>);
  void drawAmbientBackground(juce::Graphics&, juce::Rectangle<int>, juce::Colour);
  
  void drawHalo(juce::Graphics&, juce::Rectangle<int>, const BetaView&, const char* title, bool referenceOverlay);
  void drawDomainCard(juce::Graphics&, juce::Rectangle<int>, const char*, Domain, const BetaView&);
  void drawCandles(juce::Graphics&, juce::Rectangle<int>, const BetaView&);
  void drawCandleStrip(juce::Graphics&, juce::Rectangle<int>, const BetaView&, CandleStripType type);
  void drawChatPanel(juce::Graphics&, juce::Rectangle<int>);
  void drawHaloSpectrometer(juce::Graphics&, juce::Rectangle<float>, const BetaView&);
  void drawReferencePanel(juce::Graphics&, juce::Rectangle<int>, const BetaView&);
  void drawCompare(juce::Graphics&, juce::Rectangle<int>);
  void drawCompareVu(juce::Graphics&, juce::Rectangle<int>, const BetaView&, const BetaView&);
  void drawMixSignature(juce::Graphics&, juce::Rectangle<int>, const BetaView&);
  void drawSpectrumMeter(juce::Graphics&, juce::Rectangle<int>, const BetaView&);
  void drawCorrelationMeter(juce::Graphics&, juce::Rectangle<int>, const BetaView&);
  void drawBrainPanel(juce::Graphics&,juce::Rectangle<int>);
  void pushSettingsToProcessor();
  bool analyzeReferenceFile(const juce::File& file);
  void clearLocalReference();
  void updateOfficialReferenceMenu(const ReferencePoolSnapshot&);
  void selectOfficialReference(int);
  juce::String metricText(const BetaView& state, Domain domain);
  juce::Rectangle<float> modalBounds(float width,float height) const;
  juce::Rectangle<int> scaledBounds(juce::Rectangle<int>) const;

  AifredAudioProcessor& processor_;
  AifredLookAndFeel lookAndFeel_;
  juce::TextButton analyzeButton_ {"ANALYZE"};
  juce::TextButton referenceButton_ {"REFERENCE"};
  juce::TextButton compareButton_ {"COMPARE"};
  juce::TextButton optionsButton_ {"OPTIONS"};
  juce::TextButton tutorialButton_ {"HELP"};
  juce::TextButton centerModeButton_ {"CENTER"};
  juce::TextButton askAiButton_ {"ASK AI"};
  juce::TextButton saveApiButton_ {"SAVE API"};
  juce::TextButton chatFileButton_ {"CHAT FILE"};
  juce::TextButton localReferenceButton_ {"LOAD LOCAL REFERENCE"};
  juce::TextButton compareFileButton_ {"COMPARE FILE"};
  juce::TextEditor chatInput_;
  juce::TextEditor chatOutput_;
  juce::TextEditor apiEndpoint_;
  juce::TextEditor apiKey_;
  juce::TextEditor aiModel_;
  juce::ComboBox providerMenu_;
  juce::ComboBox appearanceMenu_;
  juce::ComboBox profileMenu_;
  juce::ComboBox spectrumRangeMenu_;
  juce::ComboBox officialReferenceMenu_;
  juce::String referenceFileName_;
  ReferenceTarget localReferenceTarget_;
  bool localReferenceValid_ = false;
  juce::Image mascot_;
  std::unique_ptr<juce::FileChooser> fileChooser_;
  BetaView state_;
  BetaView compareState_;
  MixMemoryIndex memoryIndex_;

  juce::String chatOutputText_;
  juce::String apiStatus_ = "API route not connected.";
  juce::String referenceStatus_ = "No reference file selected.";
  juce::String officialReferencePoolStatus_ = "Official pool not loaded.";
  std::uint64_t officialReferencePoolRevision_ = 0;
  juce::String compareStatus_ = "No compare file selected.";
  juce::String chatFileStatus_ = "No chat file selected.";
  bool showTutorial_ = true;
  bool showOptions_ = false;
  int haloCenterMode_ = 0;
  float ambientPhase_ = 0.0f;
  float designScale_ = 1.0f;
  juce::Point<float> designOrigin_;
  std::string selectedOfficialReferenceId_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AifredAudioProcessorEditor)
};

} // namespace aifred
