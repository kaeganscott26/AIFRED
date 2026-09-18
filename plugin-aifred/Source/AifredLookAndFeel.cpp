#include "AifredLookAndFeel.h"

namespace aifred {

AifredLookAndFeel::AifredLookAndFeel() {
  setColour(juce::ResizableWindow::backgroundColourId, Colours::panel);
  setColour(juce::Label::textColourId, Colours::ink);
  setColour(juce::TextButton::textColourOffId, juce::Colours::black);
  setColour(juce::TextButton::buttonColourId, Colours::cyan);
}

void AifredLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                             bool highlighted, bool down) {
  auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
  auto base = down ? Colours::green : Colours::cyan;
  if (highlighted) base = base.brighter(0.18f);
  g.setGradientFill(juce::ColourGradient(base, bounds.getX(), bounds.getY(), Colours::green, bounds.getRight(), bounds.getBottom(), false));
  g.fillRoundedRectangle(bounds, 6.0f);
  g.setColour(base.withAlpha(0.52f));
  g.drawRoundedRectangle(bounds, 6.0f, 1.2f);
}

} // namespace aifred
