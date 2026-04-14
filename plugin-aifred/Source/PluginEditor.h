#pragma once

#include "AifredLookAndFeel.h"
#include "PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

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
  void drawChatPanel(juce::Graphics&, juce::Rectangle<int>);
  void drawCompare(juce::Graphics&, juce::Rectangle<int>);
  void drawMixSignature(juce::Graphics&, juce::Rectangle<int>, const HaloState&);

  AifredAudioProcessor& processor_;
  AifredLookAndFeel lookAndFeel_;
  juce::TextButton analyzeButton_ {"ANALYZE"};
  juce::TextButton referenceButton_ {"REFERENCE"};
  juce::TextButton compareButton_ {"COMPARE"};
  juce::TextButton optionsButton_ {"OPTIONS"};
  juce::TextButton tutorialButton_ {"TUTORIAL"};
  juce::Image mascot_;
  HaloState state_;
  HaloState compareState_;
  bool showTutorial_ = true;
  bool showOptions_ = false;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AifredAudioProcessorEditor)
};

} // namespace aifred
