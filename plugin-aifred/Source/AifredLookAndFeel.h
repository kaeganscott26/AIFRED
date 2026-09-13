#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace aifred {

class AifredLookAndFeel : public juce::LookAndFeel_V4 {
public:
  AifredLookAndFeel();
  void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

namespace Colours {
  inline const auto ink = juce::Colour(0xffe8fbff);
  inline const auto muted = juce::Colour(0xff8ea8b8);
  inline const auto panel = juce::Colour(0xff07111d);
  inline const auto panel2 = juce::Colour(0xff0b1724);
  inline const auto line = juce::Colour(0xff1b5068);
  inline const auto cyan = juce::Colour(0xff22e5ff);
  inline const auto green = juce::Colour(0xff8cff45);
  inline const auto violet = juce::Colour(0xffa85cff);
  inline const auto yellow = juce::Colour(0xffffd84a);
  inline const auto red = juce::Colour(0xffff4f66);
}

} // namespace aifred
