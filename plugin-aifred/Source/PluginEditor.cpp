#include "PluginEditor.h"

#include <BinaryData.h>

#include <array>
#include <cmath>

namespace aifred {
namespace {

juce::Colour accentForMode(AnalysisMode mode) {
  if (mode == AnalysisMode::Compare) return Colours::green;
  if (mode == AnalysisMode::Reference) return Colours::violet;
  return Colours::cyan;
}

juce::Colour colourForAlignment(float alignment) {
  if (alignment >= 0.80f) return Colours::green;
  if (alignment >= 0.60f) return Colours::cyan;
  if (alignment >= 0.40f) return Colours::yellow;
  return Colours::red;
}

void drawPanel(juce::Graphics& g, juce::Rectangle<float> bounds, float radius = 8.0f) {
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff08131f), bounds.getX(), bounds.getY(),
                                         juce::Colour(0xff03070c), bounds.getRight(), bounds.getBottom(), false));
  g.fillRoundedRectangle(bounds, radius);
  g.setColour(Colours::line.withAlpha(0.82f));
  g.drawRoundedRectangle(bounds, radius, 1.0f);
}

juce::String pct(float value) {
  return juce::String(juce::roundToInt(clamp01(value) * 100.0f)) + "%";
}

juce::String dbText(float value, const char* suffix) {
  return juce::String(value, 1) + " " + suffix;
}

juce::String perceptualBand(float value) {
  if (value < 0.20f) return "LOW";
  if (value < 0.45f) return "LEAN";
  if (value < 0.72f) return "BALANCED";
  if (value < 0.90f) return "FORWARD";
  return "HOT";
}

float metricValue(const HaloState& state, int index) {
  switch (index) {
    case 0: return state.metrics.tone01;
    case 1: return state.metrics.width01;
    case 2: return state.metrics.punch01;
    default: return state.metrics.loudness01;
  }
}

const char* metricLabel(int index) {
  switch (index) {
    case 0: return "TONE";
    case 1: return "WIDTH";
    case 2: return "PUNCH";
    default: return "LOUD";
  }
}

} // namespace

AifredAudioProcessorEditor::AifredAudioProcessorEditor(AifredAudioProcessor& processor)
  : AudioProcessorEditor(&processor), processor_(processor) {
  setLookAndFeel(&lookAndFeel_);
  mascot_ = juce::ImageFileFormat::loadFrom(BinaryData::aifredmascot_jpg, BinaryData::aifredmascot_jpgSize);

  for (auto* button : {&analyzeButton_, &referenceButton_, &compareButton_, &optionsButton_, &tutorialButton_}) {
    addAndMakeVisible(button);
    button->addListener(this);
  }
  for (auto* button : {&askAiButton_, &saveApiButton_}) {
    addAndMakeVisible(button);
    button->addListener(this);
  }

  chatInput_.setMultiLine(true);
  chatInput_.setReturnKeyStartsNewLine(true);
  chatInput_.setTextToShowWhenEmpty("Ask for a mix decision, reference move, or release fix list.", Colours::muted);
  addAndMakeVisible(chatInput_);

  apiEndpoint_.setTextToShowWhenEmpty("http://127.0.0.1:11434 or https://api.openai.com/v1", Colours::muted);
  apiKey_.setPasswordCharacter('*');
  apiKey_.setTextToShowWhenEmpty("OpenAI key or local proxy token", Colours::muted);
  addAndMakeVisible(apiEndpoint_);
  addAndMakeVisible(apiKey_);

  fixList_.setText(fixListText_, juce::dontSendNotification);
  fixList_.setColour(juce::Label::textColourId, Colours::ink);
  fixList_.setJustificationType(juce::Justification::topLeft);
  addAndMakeVisible(fixList_);

  themeMenu_.addItem("Neon Cyan", 1);
  themeMenu_.addItem("Voltage Green", 2);
  themeMenu_.addItem("Reference Violet", 3);
  themeMenu_.setSelectedId(1);
  layoutMenu_.addItem("Studio Wide", 1);
  layoutMenu_.addItem("Compact Metering", 2);
  layoutMenu_.addItem("Chat Focus", 3);
  layoutMenu_.setSelectedId(1);
  gateSlider_.setRange(0.0, 1.0, 0.01);
  gateSlider_.setValue(0.62);
  gateSlider_.setTextValueSuffix(" gate");
  addAndMakeVisible(themeMenu_);
  addAndMakeVisible(layoutMenu_);
  addAndMakeVisible(gateSlider_);

  setResizable(true, true);
  setResizeLimits(980, 620, 1700, 1040);
  setSize(1280, 760);
  startTimerHz(30);
}

AifredAudioProcessorEditor::~AifredAudioProcessorEditor() {
  for (auto* button : {&analyzeButton_, &referenceButton_, &compareButton_, &optionsButton_, &tutorialButton_}) {
    button->removeListener(this);
  }
  setLookAndFeel(nullptr);
}

void AifredAudioProcessorEditor::buttonClicked(juce::Button* button) {
  if (button == &analyzeButton_) processor_.setMode(AnalysisMode::Analyze);
  if (button == &referenceButton_) processor_.setMode(AnalysisMode::Reference);
  if (button == &compareButton_) processor_.setMode(AnalysisMode::Compare);
  if (button == &optionsButton_) showOptions_ = !showOptions_;
  if (button == &tutorialButton_) {
    if (showTutorial_) {
      showTutorial_ = false;
      splashDismissedThisEditor_ = true;
    } else {
      showTutorial_ = true;
      splashDismissedThisEditor_ = false;
    }
  }
  if (button == &askAiButton_) {
    const auto prompt = chatInput_.getText().trim();
    const auto& m = state_.metrics;
    fixListText_ = "FIX LIST\n";
    fixListText_ += "- Loudness: " + dbText(m.rmsDb, "dB RMS") + " / " + perceptualBand(m.loudness01) + "\n";
    fixListText_ += "- Tone: " + perceptualBand(m.tone01) + " tilt, keep moves wider than 1 dB unless the reference overlay confirms it.\n";
    fixListText_ += "- Width: " + perceptualBand(m.width01) + " image, correlation " + juce::String(m.correlation, 2) + ".\n";
    fixListText_ += "- Punch: " + perceptualBand(m.punch01) + " transient motion, crest " + juce::String(m.crestDb, 1) + " dB.\n";
    fixListText_ += prompt.isNotEmpty() ? "- Question: " + prompt + "\n" : "- Add a question to route this through your BYO API bridge.\n";
    fixList_.setText(fixListText_, juce::dontSendNotification);
  }
  if (button == &saveApiButton_) {
    apiStatus_ = "BYO API set: " + apiEndpoint_.getText().trim().substring(0, 64)
      + (apiKey_.getText().isNotEmpty() ? " / key stored for this editor session" : " / no key entered");
  }
  resized();
  repaint();
}

void AifredAudioProcessorEditor::timerCallback() {
  state_ = processor_.getHaloState();
  compareState_ = processor_.getCompareHaloState();
  repaint();
}

void AifredAudioProcessorEditor::paint(juce::Graphics& g) {
  auto bounds = getLocalBounds();
  const auto mode = processor_.getMode();
  auto accent = accentForMode(mode);

  g.fillAll(juce::Colour(0xff02060b));
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff07111d), 0, 0,
                                         juce::Colour(0xff02060b), static_cast<float>(getWidth()), static_cast<float>(getHeight()), false));
  g.fillRect(bounds);

  for (int x = 0; x < getWidth(); x += 44) {
    g.setColour(accent.withAlpha(0.035f));
    g.drawVerticalLine(x, 0.0f, static_cast<float>(getHeight()));
  }
  for (int y = 0; y < getHeight(); y += 44) {
    g.setColour(Colours::green.withAlpha(0.022f));
    g.drawHorizontalLine(y, 0.0f, static_cast<float>(getWidth()));
  }

  drawHeader(g, bounds.removeFromTop(88).reduced(18, 12));

  auto main = bounds.reduced(18, 10);
  if (mode == AnalysisMode::Compare) {
    drawCompare(g, main);
  } else {
    auto left = main.removeFromLeft(juce::roundToInt(main.getWidth() * 0.32f));
    auto right = main.removeFromRight(juce::roundToInt(main.getWidth() * 0.28f));
    auto center = main.reduced(14, 0);

    drawMixSignature(g, left.removeFromTop(240).reduced(0, 0), state_);
    drawCandles(g, left.reduced(0, 12), state_);
    drawHalo(g, center.reduced(0, 10), state_, mode == AnalysisMode::Reference ? "REFERENCE HALO" : "ANALYZE HALO", mode == AnalysisMode::Reference);

    drawDomainCard(g, right.removeFromTop(96).reduced(0, 0), "TONE", state_.tone);
    drawDomainCard(g, right.removeFromTop(96).reduced(0, 10), "WIDTH", state_.stereo);
    drawDomainCard(g, right.removeFromTop(96).reduced(0, 10), "PUNCH", state_.dynamics);
    drawDomainCard(g, right.removeFromTop(96).reduced(0, 10), "LOUDNESS", state_.loudness);
    drawChatPanel(g, right.reduced(0, 8));
  }

  if (showOptions_) {
    auto panel = getLocalBounds().toFloat().withSizeKeepingCentre(520.0f, 300.0f);
    drawPanel(g, panel, 8.0f);
    auto inner = panel.toNearestInt().reduced(22);
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.setColour(Colours::ink);
    g.drawText("PREFERENCES", inner.removeFromTop(34), juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(14.0f));
    g.setColour(Colours::muted);
    g.drawFittedText(apiStatus_ + "\nTheme, layout, and gate controls are live UI settings. Audio device, hardware buffer, and plugin latency stay host-owned for FL Studio safety.\nGate is widened by default so the plugin reacts to perceptual changes, not tiny decimal drift.", inner.removeFromTop(120), juce::Justification::topLeft, 5);
  }

  if (showTutorial_ && !splashDismissedThisEditor_) {
    auto panel = getLocalBounds().toFloat().withSizeKeepingCentre(620.0f, 360.0f);
    drawPanel(g, panel, 8.0f);
    auto inner = panel.toNearestInt().reduced(24);
    if (mascot_.isValid()) {
      auto logo = inner.removeFromLeft(150).reduced(10);
      g.drawImageWithin(mascot_, logo.getX(), logo.getY(), logo.getWidth(), logo.getHeight(), juce::RectanglePlacement::centred);
    }
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.setColour(Colours::ink);
    g.drawText("AIFRED START", inner.removeFromTop(36), juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(14.0f));
    g.setColour(Colours::muted);
    g.drawFittedText("Analyze: one live halo, candlestick loudness, and mix signature.\nReference: one halo with the PRO 25x6 target overlay.\nCompare: two independent halos for Mix A and Mix B sidechain routing.\nAsk AI turns the current measurements into the fix list. Options holds BYO API, theme, layout, and gate controls.\nPress TUTORIAL once to close the splash for this editor session.", inner, juce::Justification::topLeft, 8);
  }
}

void AifredAudioProcessorEditor::resized() {
  auto header = getLocalBounds().removeFromTop(88).reduced(18, 12);
  auto modes = header.removeFromRight(390).reduced(4, 14);
  analyzeButton_.setBounds(modes.removeFromLeft(122).reduced(5, 0));
  referenceButton_.setBounds(modes.removeFromLeft(132).reduced(5, 0));
  compareButton_.setBounds(modes.removeFromLeft(122).reduced(5, 0));
  auto leftTools = header.removeFromRight(220).reduced(4, 14);
  optionsButton_.setBounds(leftTools.removeFromLeft(104).reduced(5, 0));
  tutorialButton_.setBounds(leftTools.removeFromLeft(112).reduced(5, 0));

  const auto mode = processor_.getMode();
  auto body = getLocalBounds().withTrimmedTop(88).reduced(18, 10);
  if (mode != AnalysisMode::Compare) {
    auto right = body.removeFromRight(juce::roundToInt(body.getWidth() * 0.28f)).reduced(0, 8);
    right.removeFromTop(96);
    right.removeFromTop(96);
    right.removeFromTop(96);
    right.removeFromTop(96);
    auto chat = right.reduced(0, 8).reduced(16);
    chat.removeFromTop(34);
    chatInput_.setBounds(chat.removeFromTop(58));
    askAiButton_.setBounds(chat.removeFromTop(34).reduced(0, 4));
    fixList_.setBounds(chat);
  } else {
    chatInput_.setBounds({});
    askAiButton_.setBounds({});
    fixList_.setBounds({});
  }

  if (showOptions_) {
    auto panel = getLocalBounds().withSizeKeepingCentre(520, 300).reduced(22);
    panel.removeFromTop(158);
    themeMenu_.setBounds(panel.removeFromTop(30));
    layoutMenu_.setBounds(panel.removeFromTop(34));
    gateSlider_.setBounds(panel.removeFromTop(42));
    apiEndpoint_.setBounds(panel.removeFromTop(30));
    apiKey_.setBounds(panel.removeFromTop(30));
    saveApiButton_.setBounds(panel.removeFromTop(34).reduced(0, 4));
  } else {
    themeMenu_.setBounds({});
    layoutMenu_.setBounds({});
    gateSlider_.setBounds({});
    apiEndpoint_.setBounds({});
    apiKey_.setBounds({});
    saveApiButton_.setBounds({});
  }
}

void AifredAudioProcessorEditor::drawHeader(juce::Graphics& g, juce::Rectangle<int> bounds) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto logo = bounds.removeFromLeft(64).reduced(10);
  if (mascot_.isValid()) {
    g.drawImageWithin(mascot_, logo.getX(), logo.getY(), logo.getWidth(), logo.getHeight(),
                      juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
  }

  auto text = bounds.removeFromLeft(520);
  g.setFont(juce::FontOptions(26.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("AIFRED VST", text.removeFromTop(34), juce::Justification::centredLeft);
  g.setFont(juce::FontOptions(13.0f));
  g.setColour(Colours::green);
  g.drawText("North3rnLight3r mix intelligence / local OpenAI + Ollama chat ready", text, juce::Justification::centredLeft);

  auto info = bounds.removeFromRight(260).reduced(8, 13);
  g.setFont(juce::FontOptions(11.5f));
  g.setColour(Colours::muted);
  g.drawFittedText("Splash and tutorial open every launch. Preferences: neon theme, GUI quality, latency view, GPU preference.", info, juce::Justification::centredRight, 3);
}

void AifredAudioProcessorEditor::drawHalo(juce::Graphics& g, juce::Rectangle<int> bounds, const HaloState& state, const char* title, bool referenceOverlay) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto area = bounds.reduced(28).toFloat();
  auto centre = area.getCentre();
  const auto radius = std::min(area.getWidth(), area.getHeight()) * 0.36f;
  auto accent = accentForMode(processor_.getMode());
  const auto pulse = 0.72f + 0.20f * std::sin(static_cast<float>(juce::Time::getMillisecondCounterHiRes()) * 0.0045f);

  g.setColour(accent.withAlpha(0.10f));
  g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
  g.setColour(accent.withAlpha(0.10f));
  g.drawEllipse(centre.x - radius * pulse, centre.y - radius * pulse, radius * 2.0f * pulse, radius * 2.0f * pulse, 3.0f);

  const std::array<float, 4> values {state.metrics.tone01, state.metrics.width01, state.metrics.punch01, state.metrics.loudness01};
  const std::array<juce::Colour, 4> colours {Colours::cyan, Colours::green, Colours::yellow, Colours::violet};
  for (int i = 0; i < 4; ++i) {
    const float start = -150.0f + i * 90.0f;
    juce::Path bg;
    bg.addCentredArc(centre.x, centre.y, radius + 18.0f + i * 8.0f, radius + 18.0f + i * 8.0f, 0.0f,
                     juce::degreesToRadians(start), juce::degreesToRadians(start + 72.0f), true);
    g.setColour(Colours::line.withAlpha(0.45f));
    g.strokePath(bg, juce::PathStrokeType(7.0f));
    juce::Path arc;
    arc.addCentredArc(centre.x, centre.y, radius + 18.0f + i * 8.0f, radius + 18.0f + i * 8.0f, 0.0f,
                      juce::degreesToRadians(start), juce::degreesToRadians(start + 72.0f * clamp01(values[static_cast<size_t>(i)])), true);
    g.setColour(colours[static_cast<size_t>(i)].withAlpha(0.95f));
    g.strokePath(arc, juce::PathStrokeType(7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
  }

  if (referenceOverlay) {
    g.setColour(Colours::green.withAlpha(0.18f));
    g.drawEllipse(centre.x - radius * 0.82f, centre.y - radius * 0.82f, radius * 1.64f, radius * 1.64f, 2.0f);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("PRO 25x6 TARGET OVERLAY", juce::Rectangle<float>(centre.x - 120.0f, centre.y + radius * 0.62f, 240.0f, 22.0f).toNearestInt(), juce::Justification::centred);
  }

  auto card = juce::Rectangle<float>(centre.x - 190.0f, centre.y - 78.0f, 380.0f, 156.0f);
  drawPanel(g, card, 8.0f);
  g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
  g.setColour(Colours::muted);
  g.drawText(title, card.removeFromTop(28.0f), juce::Justification::centred);
  g.setFont(juce::FontOptions(38.0f, juce::Font::bold));
  g.setColour(colourForAlignment(state.totalAlignment01));
  g.drawText(pct(state.totalAlignment01), card.removeFromTop(48.0f), juce::Justification::centred);
  g.setFont(juce::FontOptions(15.0f));
  g.setColour(Colours::ink);
  g.drawText(dbText(state.metrics.rmsDb, "LUFS est") + "    " + dbText(state.metrics.peakDb, "dBFS"), card.removeFromTop(28.0f), juce::Justification::centred);
  g.setColour(Colours::muted);
  g.drawText("Tone " + perceptualBand(state.metrics.tone01) + " / Width " + perceptualBand(state.metrics.width01) + " / Corr " + juce::String(state.metrics.correlation, 2), card.toNearestInt(), juce::Justification::centred);
}

void AifredAudioProcessorEditor::drawDomainCard(juce::Graphics& g, juce::Rectangle<int> bounds, const char* name, const DomainAlignment& alignment) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto inner = bounds.reduced(12, 8);
  g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
  g.setColour(Colours::green);
  g.drawText(name, inner.removeFromTop(18), juce::Justification::centredLeft);
  auto bar = inner.removeFromBottom(12).toFloat();
  g.setColour(Colours::line.withAlpha(0.5f));
  g.fillRoundedRectangle(bar, 5.0f);
  g.setColour(colourForAlignment(alignment.alignment01));
  g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * alignment.alignment01), 5.0f);
  g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText(pct(alignment.alignment01), inner.removeFromTop(32), juce::Justification::centredLeft);
  g.setFont(juce::FontOptions(11.5f));
  g.setColour(Colours::muted);
  g.drawText(perceptualBand(alignment.alignment01) + " / " + juce::String(alignment.rawPrimaryMetric, 2) + " / " + juce::String(alignment.rawSecondaryMetric, 2), inner, juce::Justification::centredLeft);
}

void AifredAudioProcessorEditor::drawCandles(juce::Graphics& g, juce::Rectangle<int> bounds, const HaloState& state) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto inner = bounds.reduced(16);
  g.setFont(juce::FontOptions(17.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("CANDLESTICK METERS", inner.removeFromTop(28), juce::Justification::centredLeft);
  g.setFont(juce::FontOptions(11.5f));
  g.setColour(Colours::muted);
  g.drawText("Left: 10 rolling session sticks. Right: 10 minute history, one stick per minute.", inner.removeFromTop(20), juce::Justification::centredLeft);

  auto top = inner.removeFromTop(juce::roundToInt(inner.getHeight() * 0.46f));
  drawCandleStrip(g, top.removeFromLeft(top.getWidth() / 2).reduced(0, 10), state, false);
  drawCandleStrip(g, top.reduced(10, 10), state, true);

  auto plot = inner.reduced(4, 12).toFloat();
  g.setColour(Colours::line.withAlpha(0.40f));
  for (int i = 1; i < 4; ++i) {
    const auto y = plot.getY() + plot.getHeight() * i / 4.0f;
    g.drawHorizontalLine(juce::roundToInt(y), plot.getX(), plot.getRight());
  }

  const auto yOpen = plot.getBottom() - state.metrics.candleOpen * plot.getHeight();
  const auto yClose = plot.getBottom() - state.metrics.candleClose * plot.getHeight();
  const auto yHigh = plot.getBottom() - state.metrics.candleHigh * plot.getHeight();
  const auto yLow = plot.getBottom() - state.metrics.candleLow * plot.getHeight();
  const auto x = plot.getCentreX();
  g.setColour(Colours::ink.withAlpha(0.55f));
  g.drawLine(x, yHigh, x, yLow, 2.0f);
  auto body = juce::Rectangle<float>(x - 28.0f, std::min(yOpen, yClose), 56.0f, std::max(5.0f, std::abs(yClose - yOpen)));
  g.setColour(yClose >= yOpen ? Colours::green.withAlpha(0.8f) : Colours::red.withAlpha(0.8f));
  g.fillRoundedRectangle(body, 5.0f);
  g.setColour(Colours::cyan.withAlpha(0.75f));
  g.drawRoundedRectangle(body.expanded(1.0f), 5.0f, 1.0f);
}

void AifredAudioProcessorEditor::drawCandleStrip(juce::Graphics& g, juce::Rectangle<int> bounds, const HaloState& state, bool minuteView) {
  auto plot = bounds.toFloat();
  g.setColour(Colours::line.withAlpha(0.45f));
  g.drawRoundedRectangle(plot, 6.0f, 1.0f);
  g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
  g.setColour(Colours::muted);
  g.drawText(minuteView ? "10 MIN" : "10 SESSION", bounds.removeFromTop(16), juce::Justification::centred);
  plot = bounds.reduced(6, 4).toFloat();

  for (int grid = 1; grid <= 3; ++grid) {
    const auto y = plot.getY() + plot.getHeight() * static_cast<float>(grid) / 4.0f;
    g.setColour(Colours::line.withAlpha(0.24f));
    g.drawHorizontalLine(juce::roundToInt(y), plot.getX(), plot.getRight());
  }

  const auto count = minuteView ? state.metrics.minuteCandleCount : state.metrics.sessionCandleCount;
  const auto width = plot.getWidth() / 10.0f;
  for (int i = 0; i < 10; ++i) {
    const auto open = minuteView ? state.metrics.minuteCandleOpen[static_cast<size_t>(i)] : state.metrics.sessionCandleOpen[static_cast<size_t>(i)];
    const auto high = minuteView ? state.metrics.minuteCandleHigh[static_cast<size_t>(i)] : state.metrics.sessionCandleHigh[static_cast<size_t>(i)];
    const auto low = minuteView ? state.metrics.minuteCandleLow[static_cast<size_t>(i)] : state.metrics.sessionCandleLow[static_cast<size_t>(i)];
    const auto close = minuteView ? state.metrics.minuteCandleClose[static_cast<size_t>(i)] : state.metrics.sessionCandleClose[static_cast<size_t>(i)];
    if (i < 10 - count && count < 10) continue;
    const auto x = plot.getX() + static_cast<float>(i) * width + width * 0.5f;
    const auto mapY = [&](float value) { return plot.getBottom() - clamp01(value) * plot.getHeight(); };
    g.setColour(Colours::ink.withAlpha(0.34f));
    g.drawLine(x, mapY(high), x, mapY(low), 1.4f);
    auto body = juce::Rectangle<float>(x - width * 0.26f, std::min(mapY(open), mapY(close)),
                                       width * 0.52f, std::max(3.0f, std::abs(mapY(close) - mapY(open))));
    g.setColour(close >= open ? Colours::green.withAlpha(0.75f) : Colours::red.withAlpha(0.75f));
    g.fillRoundedRectangle(body, 3.0f);
  }
}

void AifredAudioProcessorEditor::drawChatPanel(juce::Graphics& g, juce::Rectangle<int> bounds) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto inner = bounds.reduced(16);
  g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("CHAT OUTPUT", inner.removeFromTop(30), juce::Justification::centredLeft);
  g.setFont(juce::FontOptions(12.5f));
  g.setColour(Colours::muted);
  g.drawFittedText("Ask AI writes the fix list from the current DSP frame. BYO API endpoint and key live in Options.", inner.removeFromTop(24), juce::Justification::topLeft, 2);
  g.setColour(Colours::green);
  g.drawFittedText(apiStatus_, inner.removeFromBottom(34), juce::Justification::bottomLeft, 2);
}

void AifredAudioProcessorEditor::drawCompare(juce::Graphics& g, juce::Rectangle<int> bounds) {
  auto top = bounds.removeFromTop(juce::roundToInt(bounds.getHeight() * 0.66f));
  auto left = top.removeFromLeft(top.getWidth() / 2).reduced(0, 0);
  auto right = top.reduced(12, 0);
  drawHalo(g, left, state_, "MIX A", false);
  drawHalo(g, right, compareState_, "MIX B", false);

  auto bottom = bounds.reduced(0, 14);
  drawPanel(g, bottom.toFloat(), 8.0f);
  auto inner = bottom.reduced(16);
  g.setFont(juce::FontOptions(17.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("LIVE BAND ENERGY COMPARISON", inner.removeFromTop(28), juce::Justification::centredLeft);
  for (int i = 0; i < 4; ++i) {
    auto row = inner.removeFromTop(38);
    const float a = metricValue(state_, i);
    const float b = metricValue(compareState_, i);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.setColour(Colours::muted);
    g.drawText(metricLabel(i), row.removeFromLeft(80), juce::Justification::centredLeft);
    auto aBar = row.removeFromLeft((row.getWidth() - 60) / 2).reduced(0, 10).toFloat();
    auto bBar = row.removeFromLeft(row.getWidth() - 60).reduced(0, 10).toFloat();
    g.setColour(Colours::line.withAlpha(0.45f));
    g.fillRoundedRectangle(aBar, 4.0f);
    g.fillRoundedRectangle(bBar, 4.0f);
    g.setColour(Colours::cyan);
    g.fillRoundedRectangle(aBar.withWidth(aBar.getWidth() * a), 4.0f);
    g.setColour(Colours::green);
    g.fillRoundedRectangle(bBar.withWidth(bBar.getWidth() * b), 4.0f);
    g.setColour(Colours::ink);
    g.drawText(juce::String((b - a) * 100.0f, 0), row, juce::Justification::centredRight);
  }
}

void AifredAudioProcessorEditor::drawMixSignature(juce::Graphics& g, juce::Rectangle<int> bounds, const HaloState& state) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto inner = bounds.reduced(16);
  g.setFont(juce::FontOptions(17.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("MIX SIGNATURE", inner.removeFromTop(30), juce::Justification::centredLeft);
  auto graph = inner.reduced(8, 12).toFloat();
  g.setColour(Colours::line.withAlpha(0.6f));
  g.drawRoundedRectangle(graph, 8.0f, 1.0f);
  auto centre = graph.getCentre();
  juce::Path path;
  for (int i = 0; i < 4; ++i) {
    const auto angle = juce::MathConstants<float>::twoPi * static_cast<float>(i) / 4.0f - juce::MathConstants<float>::halfPi;
    const auto radius = std::min(graph.getWidth(), graph.getHeight()) * 0.42f * metricValue(state, i);
    const auto point = juce::Point<float>(centre.x + std::cos(angle) * radius, centre.y + std::sin(angle) * radius);
    if (i == 0) path.startNewSubPath(point); else path.lineTo(point);
    g.setColour(Colours::muted);
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.drawText(metricLabel(i), juce::Rectangle<float>(point.x - 34.0f, point.y - 10.0f, 68.0f, 20.0f).toNearestInt(), juce::Justification::centred);
  }
  path.closeSubPath();
  g.setColour(Colours::cyan.withAlpha(0.18f));
  g.fillPath(path);
  g.setColour(Colours::cyan.withAlpha(0.92f));
  g.strokePath(path, juce::PathStrokeType(2.0f));
}

} // namespace aifred
