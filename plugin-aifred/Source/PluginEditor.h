#pragma once

#include "AifredLookAndFeel.h"
#include "PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace aifred {

class AifredAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
  explicit AifredAudioProcessorEditor(AifredAudioProcessor&);
  ~AifredAudioProcessorEditor() override;

  void paint(juce::Graphics&) override;
  void resized() override;

private:
  void timerCallback() override;
  void drawHeader(juce::Graphics&, juce::Rectangle<int>);
  void drawHalo(juce::Graphics&, juce::Rectangle<int>, const HaloState&);
  void drawDomainCard(juce::Graphics&, juce::Rectangle<int>, const char*, const DomainAlignment&);
  void drawFixList(juce::Graphics&, juce::Rectangle<int>, const HaloState&);
  void drawMixSignature(juce::Graphics&, juce::Rectangle<int>, const HaloState&);

  AifredAudioProcessor& processor_;
  AifredLookAndFeel lookAndFeel_;
  juce::Image mascot_;
  HaloState state_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AifredAudioProcessorEditor)
};

} // namespace aifred
