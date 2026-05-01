#include "PluginEditor.h"

#include <BinaryData.h>

#include <array>
#include <algorithm>
#include <cmath>

#ifndef AIFRED_VERSION_STRING
#define AIFRED_VERSION_STRING "dev"
#endif

namespace aifred {
namespace {

bool gTutorialShownThisSession = false;

juce::Colour accentForMode(AnalysisMode mode) {
  if (mode == AnalysisMode::Compare) return Colours::green;
  if (mode == AnalysisMode::Reference) return Colours::violet;
  return Colours::cyan;
}

juce::Colour accentForTheme(int themeId, AnalysisMode mode) {
  if (themeId == 2) return juce::Colour(0xfff0d447);
  if (themeId == 3) return juce::Colour(0xffff5f6d);
  return accentForMode(mode);
}

juce::Colour genreColour(int genreId) {
  switch (genreId) {
    case 2: return juce::Colour(0xffffcf33);
    case 3: return juce::Colour(0xffff4d5a);
    case 4: return juce::Colour(0xff23e3ff);
    case 5: return juce::Colour(0xffb86cff);
    case 6: return juce::Colour(0xfff7f2e7);
    default: return juce::Colour(0xff8cff45);
  }
}

const char* genreName(int genreId) {
  switch (genreId) {
    case 2: return "Boom Bap";
    case 3: return "Drill / Trap";
    case 4: return "Electronic / Dubstep";
    case 5: return "R&B";
    case 6: return "Same Genre";
    default: return "Hip-Hop / Trap";
  }
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

juce::String signedText(float value, const char* suffix) {
  return juce::String(value >= 0.0f ? "+" : "") + juce::String(value, 1) + " " + suffix;
}

juce::String perceptualBand(float value) {
  if (value < 0.20f) return "LOW";
  if (value < 0.45f) return "LEAN";
  if (value < 0.72f) return "BALANCED";
  if (value < 0.90f) return "FORWARD";
  return "HOT";
}

juce::String candleDb(float normalized) {
  return juce::String(-42.0f + clamp01(normalized) * 34.0f, 1) + " dB";
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
    case 0: return "Tone";
    case 1: return "Width";
    case 2: return "Punch";
    default: return "Loudness";
  }
}

} // namespace

AifredAudioProcessorEditor::AifredAudioProcessorEditor(AifredAudioProcessor& processor)
  : AudioProcessorEditor(&processor), processor_(processor) {
  setLookAndFeel(&lookAndFeel_);
  mascot_ = juce::ImageFileFormat::loadFrom(BinaryData::aifredmascot_jpg, BinaryData::aifredmascot_jpgSize);

  for (auto* button : {&analyzeButton_, &referenceButton_, &compareButton_, &optionsButton_, &tutorialButton_, &centerModeButton_}) {
    addAndMakeVisible(button);
    button->addListener(this);
  }
  for (auto* button : {&askAiButton_, &saveApiButton_, &chatFileButton_, &referenceFileButton_, &compareFileButton_}) {
    addAndMakeVisible(button);
    button->addListener(this);
  }

  chatInput_.setMultiLine(true);
  chatInput_.setReturnKeyStartsNewLine(true);
  chatInput_.setTextToShowWhenEmpty("Type your question.", Colours::muted);
  addAndMakeVisible(chatInput_);

  apiEndpoint_.setTextToShowWhenEmpty("http://127.0.0.1:11434 or https://api.openai.com/v1", Colours::muted);
  apiKey_.setPasswordCharacter('*');
  apiKey_.setTextToShowWhenEmpty("OpenAI key or local proxy token", Colours::muted);
  aiModel_.setTextToShowWhenEmpty("gpt-5.4-mini, llama3.1, or compatible model", Colours::muted);
  addAndMakeVisible(apiEndpoint_);
  addAndMakeVisible(apiKey_);
  addAndMakeVisible(aiModel_);

  fixList_.setText(fixListText_, juce::dontSendNotification);
  fixList_.setColour(juce::Label::textColourId, Colours::ink);
  fixList_.setJustificationType(juce::Justification::topLeft);
  addAndMakeVisible(fixList_);

  themeMenu_.addItem("Neon Cyan", 1);
  themeMenu_.addItem("Gold Meter", 2);
  themeMenu_.addItem("Redline", 3);
  providerMenu_.addItem("OpenAI", 1);
  providerMenu_.addItem("OpenAI-compatible", 2);
  providerMenu_.addItem("Ollama / Local", 3);
  layoutMenu_.addItem("Studio Wide", 1);
  layoutMenu_.addItem("Compact Metering", 2);
  layoutMenu_.addItem("Chat Focus", 3);
  genreMenu_.addItem("Hip-Hop / Trap", 1);
  genreMenu_.addItem("Boom Bap", 2);
  genreMenu_.addItem("Drill / Trap", 3);
  genreMenu_.addItem("Electronic / Dubstep", 4);
  genreMenu_.addItem("R&B", 5);
  genreMenu_.addItem("Same Genre", 6);
  gateSlider_.setRange(0.0, 1.0, 0.01);
  gateSlider_.setTextValueSuffix(" gate");
  addAndMakeVisible(providerMenu_);
  addAndMakeVisible(themeMenu_);
  addAndMakeVisible(layoutMenu_);
  addAndMakeVisible(genreMenu_);
  addAndMakeVisible(gateSlider_);
  for (int i = 0; i < static_cast<int>(referenceVolumeSliders_.size()); ++i) {
    auto& slider = referenceVolumeSliders_[static_cast<size_t>(i)];
    slider.setRange(0.0, 100.0, 1.0);
    slider.setValue(i == 0 ? 80.0 : 55.0, juce::dontSendNotification);
    slider.setSliderStyle(juce::Slider::LinearVertical);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 46, 18);
    slider.setTextValueSuffix("%");
    addAndMakeVisible(slider);
  }

  const auto settings = processor_.getPluginSettings();
  themeMenu_.setSelectedId(settings.themeId);
  layoutMenu_.setSelectedId(settings.layoutId);
  genreMenu_.setSelectedId(settings.genreId);
  providerMenu_.setSelectedId(settings.aiProvider == "ollama" ? 3 : (settings.aiProvider == "compatible" ? 2 : 1));
  gateSlider_.setValue(settings.gate, juce::dontSendNotification);
  apiEndpoint_.setText(settings.apiEndpoint, juce::dontSendNotification);
  apiKey_.setText(settings.apiKey, juce::dontSendNotification);
  aiModel_.setText(settings.aiModel, juce::dontSendNotification);

  setResizable(true, true);
  setResizeLimits(980, 620, 1700, 1040);
  setSize(1280, 760);
  showTutorial_ = !processor_.isSessionInitialized() && !gTutorialShownThisSession;
  AifredEngineClient::instance().pingHealthAsync();
  startTimerHz(30);
}

AifredAudioProcessorEditor::~AifredAudioProcessorEditor() {
  for (auto* button : {&analyzeButton_, &referenceButton_, &compareButton_, &optionsButton_, &tutorialButton_, &centerModeButton_}) {
    button->removeListener(this);
  }
  for (auto* button : {&askAiButton_, &saveApiButton_, &chatFileButton_, &referenceFileButton_, &compareFileButton_}) {
    button->removeListener(this);
  }
  setLookAndFeel(nullptr);
}

void AifredAudioProcessorEditor::buttonClicked(juce::Button* button) {
  if (button == &analyzeButton_) processor_.setMode(AnalysisMode::Analyze);
  if (button == &referenceButton_) processor_.setMode(AnalysisMode::Reference);
  if (button == &compareButton_) processor_.setMode(AnalysisMode::Compare);
  if (button == &optionsButton_) showOptions_ = !showOptions_;
  if (button == &centerModeButton_) haloCenterMode_ = (haloCenterMode_ + 1) % 3;
  if (button == &tutorialButton_) {
    if (showTutorial_) {
      showTutorial_ = false;
      splashDismissedThisEditor_ = true;
      gTutorialShownThisSession = true;
      processor_.markSessionInitialized();
    } else {
      showTutorial_ = true;
      splashDismissedThisEditor_ = false;
    }
  }
  if (button == &askAiButton_) {
    const auto prompt = chatInput_.getText().trim();
    diagnostic_ = DiagnosticInterpreter::instance().update(state_, providerMenu_.getText(), apiEndpoint_.getText(), apiKey_.getText(), aiModel_.getText());
    if (!AifredEngineClient::instance().isAvailable()) {
      fixListText_ = "AI engine unavailable - core analysis still active.\n" + diagnostic_.fixList;
    } else {
      fixListText_ = (prompt.isNotEmpty() ? "AI context prepared for: " + prompt : "AI context prepared from current DSP state.")
        + "\n\n" + diagnostic_.aiContextJson;
    }
    if (chatFileStatus_ != "No chat file selected.") fixListText_ += "\n" + chatFileStatus_;
    fixList_.setText(fixListText_, juce::dontSendNotification);
  }
  if (button == &saveApiButton_) {
    pushSettingsToProcessor();
    apiStatus_ = apiEndpoint_.getText().trim().isNotEmpty() ? "API route set." : "API route not connected.";
  }
  if (button == &chatFileButton_ || button == &referenceFileButton_ || button == &compareFileButton_) {
    fileChooser_ = std::make_unique<juce::FileChooser>("Select audio file", juce::File{}, "*.wav;*.aif;*.aiff;*.mp3;*.flac");
    fileChooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
      [this, button](const juce::FileChooser& chooser) {
        const auto file = chooser.getResult();
        if (!file.existsAsFile()) return;
        if (button == &chatFileButton_) chatFileStatus_ = "Chat file: " + file.getFileName();
        if (button == &referenceFileButton_) referenceStatus_ = "Reference file: " + file.getFileName();
        if (button == &compareFileButton_) compareStatus_ = "Compare file: " + file.getFileName();
        repaint();
      });
  }
  pushSettingsToProcessor();
  resized();
  repaint();
}

void AifredAudioProcessorEditor::timerCallback() {
  state_ = processor_.getHaloState();
  compareState_ = processor_.getCompareHaloState();
  diagnostic_ = DiagnosticInterpreter::instance().update(state_, providerMenu_.getText(), apiEndpoint_.getText(), apiKey_.getText(), aiModel_.getText());
  if (juce::Time::getMillisecondCounter() % 3000 < 40) {
    AifredEngineClient::instance().pingHealthAsync();
  }
  if (!chatInput_.hasKeyboardFocus(true)) {
    fixList_.setText(diagnostic_.fixList, juce::dontSendNotification);
  }
  repaint();
}

void AifredAudioProcessorEditor::pushSettingsToProcessor() {
  PluginSettings settings;
  settings.themeId = themeMenu_.getSelectedId();
  settings.layoutId = layoutMenu_.getSelectedId();
  settings.genreId = genreMenu_.getSelectedId();
  settings.gate = gateSlider_.getValue();
  settings.aiProvider = providerMenu_.getSelectedId() == 3 ? "ollama" : (providerMenu_.getSelectedId() == 2 ? "compatible" : "openai");
  settings.apiEndpoint = apiEndpoint_.getText().trim();
  settings.apiKey = apiKey_.getText();
  settings.aiModel = aiModel_.getText().trim();
  processor_.setPluginSettings(settings);
}

void AifredAudioProcessorEditor::paint(juce::Graphics& g) {
  auto bounds = getLocalBounds();
  const auto mode = processor_.getMode();
  auto accent = accentForTheme(themeMenu_.getSelectedId(), mode);
  if (mode == AnalysisMode::Reference) accent = genreColour(genreMenu_.getSelectedId());

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
    const auto layoutId = layoutMenu_.getSelectedId();
    const auto leftFraction = layoutId == 2 ? 0.38f : (layoutId == 3 ? 0.24f : 0.32f);
    const auto rightFraction = layoutId == 3 ? 0.36f : 0.28f;
    auto left = main.removeFromLeft(juce::roundToInt(main.getWidth() * leftFraction));
    auto right = main.removeFromRight(juce::roundToInt(main.getWidth() * rightFraction));
    auto center = main.reduced(14, 0);

    drawMixSignature(g, left.removeFromTop(210).reduced(0, 0), state_);
    drawSpectrumMeter(g, left.removeFromTop(134).reduced(0, 10), state_);
    drawCorrelationMeter(g, left.removeFromTop(74).reduced(0, 10), state_);
    drawCandles(g, left.reduced(0, 12), state_);
    drawHalo(g, center.reduced(0, 10), state_, mode == AnalysisMode::Reference ? "REFERENCE HALO" : "ANALYZE HALO", mode == AnalysisMode::Reference);

    if (mode == AnalysisMode::Reference) {
      drawReferenceMixer(g, right.removeFromTop(176).reduced(0, 0));
    } else {
      drawDomainCard(g, right.removeFromTop(76).reduced(0, 0), "TONE", state_.tone);
      drawDomainCard(g, right.removeFromTop(76).reduced(0, 8), "WIDTH", state_.stereo);
    }
    drawDomainCard(g, right.removeFromTop(76).reduced(0, 8), "PUNCH", state_.dynamics);
    drawDomainCard(g, right.removeFromTop(76).reduced(0, 8), "LOUDNESS", state_.loudness);
    drawChatPanel(g, right.reduced(0, 8));
  }

  if (showOptions_) {
    auto panel = getLocalBounds().toFloat().withSizeKeepingCentre(560.0f, 420.0f);
    drawPanel(g, panel, 8.0f);
    auto inner = panel.toNearestInt().reduced(22);
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.setColour(Colours::ink);
    g.drawText("PREFERENCES", inner.removeFromTop(34), juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(14.0f));
    g.setColour(Colours::muted);
    g.drawFittedText(apiStatus_ + "\n" + juce::String(genreName(genreMenu_.getSelectedId())), inner.removeFromTop(64), juce::Justification::topLeft, 3);
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
    g.drawFittedText("Analyze: main input.\nReference: selected target plus reference file.\nCompare: Mix A and Mix B sidechain routing.\nFL Studio compare routing: put AIFRED on the master or a bus, enable the Mix B sidechain input in the wrapper, then route the reference track to that sidechain from the mixer send.", inner, juce::Justification::topLeft, 8);
  }

  if (!AifredEngineClient::instance().isAvailable()) {
    auto panel = getLocalBounds().toFloat().withSizeKeepingCentre(600.0f, 170.0f).translated(0.0f, static_cast<float>(getHeight()) * 0.32f);
    drawPanel(g, panel, 8.0f);
    auto inner = panel.toNearestInt().reduced(18);
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.setColour(Colours::ink);
    g.drawText("LOCAL AI ENGINE UNAVAILABLE", inner.removeFromTop(28), juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(12.5f));
    g.setColour(Colours::muted);
    g.drawFittedText("Core analysis is still active. The installer normally starts AifredEngine.exe at login and exposes http://127.0.0.1:8787/health for local coaching.", inner, juce::Justification::topLeft, 4);
  }
}

void AifredAudioProcessorEditor::resized() {
  auto header = getLocalBounds().removeFromTop(88).reduced(18, 12);
  auto modes = header.removeFromRight(390).reduced(4, 14);
  analyzeButton_.setBounds(modes.removeFromLeft(122).reduced(5, 0));
  referenceButton_.setBounds(modes.removeFromLeft(132).reduced(5, 0));
  compareButton_.setBounds(modes.removeFromLeft(122).reduced(5, 0));
  auto leftTools = header.removeFromRight(220).reduced(4, 14);
  auto centerTool = header.removeFromRight(100).reduced(4, 14);
  centerModeButton_.setBounds(centerTool.reduced(5, 0));
  optionsButton_.setBounds(leftTools.removeFromLeft(104).reduced(5, 0));
  tutorialButton_.setBounds(leftTools.removeFromLeft(112).reduced(5, 0));

  const auto mode = processor_.getMode();
  auto body = getLocalBounds().withTrimmedTop(88).reduced(18, 10);
  if (mode != AnalysisMode::Compare) {
    auto right = body.removeFromRight(juce::roundToInt(body.getWidth() * 0.28f)).reduced(0, 8);
    if (mode == AnalysisMode::Reference) {
      auto mixer = right.removeFromTop(176).reduced(16);
      mixer.removeFromTop(30);
      referenceFileButton_.setBounds(mixer.removeFromTop(34).reduced(0, 4));
      auto sliderArea = mixer.reduced(0, 4);
      const auto laneWidth = sliderArea.getWidth() / 5;
      for (int i = 0; i < 5; ++i) {
        referenceVolumeSliders_[static_cast<size_t>(i)].setBounds(sliderArea.removeFromLeft(laneWidth).reduced(4, 0));
      }
    }
    else {
      right.removeFromTop(76);
      right.removeFromTop(76);
      referenceFileButton_.setBounds({});
      for (auto& slider : referenceVolumeSliders_) slider.setBounds({});
    }
    right.removeFromTop(76);
    right.removeFromTop(76);
    auto chat = right.reduced(0, 8).reduced(16);
    chat.removeFromTop(30);
    chatInput_.setBounds(chat.removeFromTop(82));
    auto buttons = chat.removeFromTop(38);
    chatFileButton_.setBounds(buttons.removeFromLeft(buttons.getWidth() / 2).reduced(0, 4));
    askAiButton_.setBounds(buttons.reduced(6, 4));
    fixList_.setBounds(chat);
  } else {
    chatInput_.setBounds({});
    askAiButton_.setBounds({});
    chatFileButton_.setBounds({});
    referenceFileButton_.setBounds({});
    fixList_.setBounds({});
    auto compareButtonArea = getLocalBounds().withTrimmedTop(104).removeFromRight(210).reduced(18, 0);
    compareFileButton_.setBounds(compareButtonArea.removeFromTop(36));
    for (auto& slider : referenceVolumeSliders_) slider.setBounds({});
  }
  if (mode != AnalysisMode::Compare) compareFileButton_.setBounds({});

  if (showOptions_) {
    auto panel = getLocalBounds().withSizeKeepingCentre(560, 420).reduced(22);
    panel.removeFromTop(110);
    providerMenu_.setBounds(panel.removeFromTop(30));
    themeMenu_.setBounds(panel.removeFromTop(30));
    layoutMenu_.setBounds(panel.removeFromTop(34));
    genreMenu_.setBounds(panel.removeFromTop(34));
    gateSlider_.setBounds(panel.removeFromTop(42));
    apiEndpoint_.setBounds(panel.removeFromTop(30));
    apiKey_.setBounds(panel.removeFromTop(30));
    aiModel_.setBounds(panel.removeFromTop(30));
    saveApiButton_.setBounds(panel.removeFromTop(34).reduced(0, 4));
  } else {
    providerMenu_.setBounds({});
    themeMenu_.setBounds({});
    layoutMenu_.setBounds({});
    genreMenu_.setBounds({});
    gateSlider_.setBounds({});
    apiEndpoint_.setBounds({});
    apiKey_.setBounds({});
    aiModel_.setBounds({});
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
  g.drawText(juce::String(genreName(genreMenu_.getSelectedId())) + " / " + referenceStatus_, text, juce::Justification::centredLeft);

  auto info = bounds.removeFromRight(260).reduced(8, 13);
  g.setFont(juce::FontOptions(11.5f));
  g.setColour(Colours::muted);
  g.drawFittedText(juce::String("v" AIFRED_VERSION_STRING " / center ") + juce::String(haloCenterMode_ + 1), info, juce::Justification::centredRight, 1);
}

void AifredAudioProcessorEditor::drawHalo(juce::Graphics& g, juce::Rectangle<int> bounds, const HaloState& state, const char* title, bool referenceOverlay) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto area = bounds.reduced(28).toFloat();
  auto centre = area.getCentre();
  const auto radius = std::min(area.getWidth(), area.getHeight()) * 0.36f;
  auto accent = referenceOverlay ? genreColour(genreMenu_.getSelectedId()) : accentForTheme(themeMenu_.getSelectedId(), processor_.getMode());
  const auto dynamicPulse = clamp01(0.28f * state.metrics.loudness01 + 0.26f * state.metrics.width01 + 0.20f * state.metrics.punch01 + 0.18f * (1.0f - std::abs(state.metrics.crestDb - 11.0f) / 18.0f));
  const auto pulse = 0.82f + dynamicPulse * 0.14f;

  g.setColour(accent.withAlpha(referenceOverlay ? 0.18f : 0.10f));
  g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
  g.setColour(accent.withAlpha(0.10f));
  g.drawEllipse(centre.x - radius * pulse, centre.y - radius * pulse, radius * 2.0f * pulse, radius * 2.0f * pulse, 3.0f + 3.0f * state.metrics.width01);

  const std::array<float, 4> values {state.metrics.punch01, state.metrics.tone01, state.metrics.loudness01, state.metrics.width01};
  const std::array<juce::Colour, 4> colours {Colours::cyan, Colours::green, Colours::yellow, Colours::violet};
  const std::array<const char*, 4> labels {"Punch", "Tone", "Loudness", "Width"};
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
    const auto labelAngle = juce::degreesToRadians(start + 36.0f);
    const auto labelRadius = radius + 58.0f;
    const auto labelCentre = juce::Point<float>(centre.x + std::cos(labelAngle) * labelRadius,
                                                centre.y + std::sin(labelAngle) * labelRadius);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.setColour(Colours::ink);
    g.drawText(labels[static_cast<size_t>(i)], juce::Rectangle<float>(labelCentre.x - 42.0f, labelCentre.y - 11.0f, 84.0f, 22.0f).toNearestInt(), juce::Justification::centred);
  }

  for (int tick = 0; tick < 40; ++tick) {
    const auto angle = juce::degreesToRadians(-180.0f + tick * 9.0f);
    const auto major = tick % 5 == 0;
    const auto inner = radius + (major ? 78.0f : 84.0f);
    const auto outer = radius + 92.0f;
    g.setColour((major ? Colours::ink : Colours::muted).withAlpha(major ? 0.62f : 0.34f));
    g.drawLine(centre.x + std::cos(angle) * inner, centre.y + std::sin(angle) * inner,
               centre.x + std::cos(angle) * outer, centre.y + std::sin(angle) * outer,
               major ? 1.5f : 1.0f);
  }

  if (referenceOverlay) {
    const std::array<juce::Colour, 6> ringColours {Colours::green, Colours::cyan, Colours::violet, Colours::yellow, Colours::red, juce::Colour(0xfff7f2e7)};
    for (int ring = 0; ring < 6; ++ring) {
      const auto scale = 0.68f + static_cast<float>(ring) * 0.075f;
      g.setColour(ringColours[static_cast<size_t>(ring)].withAlpha(ring == 0 ? 0.28f : 0.18f));
      g.drawEllipse(centre.x - radius * scale, centre.y - radius * scale, radius * scale * 2.0f, radius * scale * 2.0f, ring == 0 ? 2.3f : 1.4f);
    }
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText(juce::String(genreName(genreMenu_.getSelectedId())) + " / pool + refs 1-5", juce::Rectangle<float>(centre.x - 150.0f, centre.y + radius * 0.72f, 300.0f, 22.0f).toNearestInt(), juce::Justification::centred);
  }

  drawHaloSpectrometer(g, juce::Rectangle<float>(centre.x - radius * 0.74f, centre.y - radius * 0.26f, radius * 1.48f, radius * 0.52f), state);
  g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
  g.setColour(colourForAlignment(state.totalAlignment01));
  g.drawText(title, juce::Rectangle<float>(centre.x - 130.0f, centre.y - radius * 0.58f, 260.0f, 22.0f).toNearestInt(), juce::Justification::centred);
}

void AifredAudioProcessorEditor::drawHaloSpectrometer(juce::Graphics& g, juce::Rectangle<float> bounds, const HaloState& state) {
  g.setColour(juce::Colour(0xff02060b).withAlpha(0.82f));
  g.fillRoundedRectangle(bounds, 8.0f);
  g.setColour(Colours::line.withAlpha(0.72f));
  g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

  auto plot = bounds.reduced(10.0f, 8.0f);
  if (haloCenterMode_ == 0 || haloCenterMode_ == 2) {
    const std::array<const char*, 8> labels {"40", "90", "180", "400", "1k", "3k", "8k", "S"};
    const auto columns = static_cast<int>(state.metrics.spectrumBands.size());
    const auto colWidth = plot.getWidth() / static_cast<float>(columns);
    for (int i = 0; i < columns; ++i) {
      const auto value = clamp01(state.metrics.spectrumBands[static_cast<size_t>(i)]);
      auto col = juce::Rectangle<float>(plot.getX() + static_cast<float>(i) * colWidth + 1.0f, plot.getBottom() - plot.getHeight() * value,
                                        std::max(2.0f, colWidth - 2.0f), std::max(3.0f, plot.getHeight() * value));
      const auto hue = i < 2 ? Colours::green : (i < 5 ? Colours::cyan : (i == 6 ? Colours::violet : Colours::yellow));
      g.setColour(hue.withAlpha(haloCenterMode_ == 2 ? 0.50f : 0.86f));
      g.fillRoundedRectangle(col, 3.0f);
      g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
      g.setColour(Colours::muted);
      g.drawText(labels[static_cast<size_t>(i)], juce::Rectangle<float>(col.getX(), plot.getBottom() - 12.0f, col.getWidth(), 10.0f).toNearestInt(), juce::Justification::centred);
    }
  }
  if (haloCenterMode_ == 1 || haloCenterMode_ == 2) {
    juce::Path wave;
    const auto mid = plot.getCentreY();
    for (size_t i = 0; i < state.metrics.waveform.size(); ++i) {
      const auto x = plot.getX() + plot.getWidth() * static_cast<float>(i) / static_cast<float>(state.metrics.waveform.size() - 1);
      const auto y = mid - std::clamp(state.metrics.waveform[i], -1.0f, 1.0f) * plot.getHeight() * 0.42f;
      if (i == 0) wave.startNewSubPath(x, y); else wave.lineTo(x, y);
    }
    g.setColour(Colours::ink.withAlpha(0.88f));
    g.strokePath(wave, juce::PathStrokeType(haloCenterMode_ == 2 ? 1.6f : 2.3f));
  }
  g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
  g.setColour(Colours::muted);
  g.drawText(haloCenterMode_ == 0 ? "MULTIBAND" : (haloCenterMode_ == 1 ? "WAVEFORM" : "BANDS + WAVE"), bounds.toNearestInt().removeFromTop(14), juce::Justification::centred);
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
  juce::String units = juce::String(alignment.rawPrimaryMetric, 2) + " primary / " + juce::String(alignment.rawSecondaryMetric, 2) + " secondary";
  if (juce::String(name) == "LOUDNESS") units = dbText(alignment.rawPrimaryMetric, "LUFS ST") + " / " + dbText(alignment.rawSecondaryMetric, "dBTP");
  if (juce::String(name) == "PUNCH") units = dbText(alignment.rawPrimaryMetric, "dB crest") + " / transient " + juce::String(alignment.rawSecondaryMetric, 2);
  if (juce::String(name) == "WIDTH") units = "width " + juce::String(alignment.rawPrimaryMetric, 2) + " / corr HP150 " + juce::String(alignment.rawSecondaryMetric, 2);
  if (juce::String(name) == "TONE") units = "tilt " + juce::String(alignment.rawPrimaryMetric, 2) + " / " + dbText(alignment.rawSecondaryMetric, "LUFS K");
  g.drawText(perceptualBand(alignment.alignment01) + " / " + units, inner, juce::Justification::centredLeft);
}

void AifredAudioProcessorEditor::drawCandles(juce::Graphics& g, juce::Rectangle<int> bounds, const HaloState& state) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto inner = bounds.reduced(16);
  g.setFont(juce::FontOptions(17.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("CANDLESTICK METERS", inner.removeFromTop(28), juce::Justification::centredLeft);
  g.setFont(juce::FontOptions(11.5f));
  g.setColour(Colours::muted);
  g.drawText("Left: current session open/close. Right: ten minute history, one candle per minute.", inner.removeFromTop(20), juce::Justification::centredLeft);

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
  g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("O " + candleDb(state.metrics.candleOpen) + "  C " + candleDb(state.metrics.candleClose) + "  Δ " + juce::String((-42.0f + state.metrics.candleClose * 34.0f) - (-42.0f + state.metrics.candleOpen * 34.0f), 1) + " dB",
             inner.removeFromBottom(18), juce::Justification::centred);
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

  if (!minuteView) {
    const auto mapY = [&](float value) { return plot.getBottom() - clamp01(value) * plot.getHeight(); };
    const auto open = state.metrics.sessionCandleOpen[9] > 0.0f ? state.metrics.sessionCandleOpen[9] : state.metrics.candleOpen;
    const auto high = state.metrics.sessionCandleHigh[9] > 0.0f ? state.metrics.sessionCandleHigh[9] : state.metrics.candleHigh;
    const auto low = state.metrics.sessionCandleLow[9] > 0.0f ? state.metrics.sessionCandleLow[9] : state.metrics.candleLow;
    const auto close = state.metrics.sessionCandleClose[9] > 0.0f ? state.metrics.sessionCandleClose[9] : state.metrics.candleClose;
    const auto x = plot.getCentreX();
    g.setColour(Colours::ink.withAlpha(0.42f));
    g.drawLine(x, mapY(high), x, mapY(low), 2.0f);
    auto body = juce::Rectangle<float>(x - 22.0f, std::min(mapY(open), mapY(close)), 44.0f, std::max(5.0f, std::abs(mapY(close) - mapY(open))));
    g.setColour(close >= open ? Colours::green.withAlpha(0.78f) : Colours::red.withAlpha(0.78f));
    g.fillRoundedRectangle(body, 4.0f);
    g.setColour(Colours::muted);
    g.setFont(juce::FontOptions(9.0f));
    g.drawText("O " + candleDb(open), plot.toNearestInt().removeFromTop(14), juce::Justification::centredLeft);
    g.drawText("C " + candleDb(close), plot.toNearestInt().removeFromBottom(14), juce::Justification::centredRight);
    return;
  }

  const auto count = state.metrics.minuteCandleCount;
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
  g.drawText("CHAT", inner.removeFromTop(30), juce::Justification::centredLeft);
  g.setFont(juce::FontOptions(12.0f));
  const auto engineReady = AifredEngineClient::instance().isAvailable();
  g.setColour(engineReady ? Colours::green : Colours::yellow);
  g.drawFittedText(engineReady ? "AI ready: local AIFRED engine." : AifredEngineClient::instance().statusText(), inner.removeFromBottom(38), juce::Justification::bottomLeft, 2);
  g.setColour(Colours::muted);
  g.drawFittedText(diagnostic_.summary, inner.removeFromBottom(44), juce::Justification::bottomLeft, 2);
}

void AifredAudioProcessorEditor::drawReferenceMixer(juce::Graphics& g, juce::Rectangle<int> bounds) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto inner = bounds.reduced(16);
  g.setFont(juce::FontOptions(17.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("REFERENCE MIXER", inner.removeFromTop(30), juce::Justification::centredLeft);
  inner.removeFromTop(38);

  const int lanes = 5;
  const auto laneWidth = inner.getWidth() / lanes;
  for (int lane = 0; lane < lanes; ++lane) {
    auto strip = inner.removeFromLeft(laneWidth).reduced(5, 0).toFloat();
    g.setColour(Colours::line.withAlpha(0.24f));
    g.fillRoundedRectangle(strip, 6.0f);
    auto fader = strip.reduced(8.0f, 10.0f);
    g.setColour(Colours::muted.withAlpha(0.32f));
    g.fillRoundedRectangle(fader.withWidth(5.0f).withCentre({strip.getCentreX(), fader.getCentreY()}), 2.5f);
    const auto volume = static_cast<float>(referenceVolumeSliders_[static_cast<size_t>(lane)].getValue() / 100.0);
    const auto pos = fader.getBottom() - fader.getHeight() * volume;
    g.setColour(genreColour(genreMenu_.getSelectedId()).withAlpha(0.9f));
    g.fillRoundedRectangle(juce::Rectangle<float>(strip.getCentreX() - 18.0f, pos - 5.0f, 36.0f, 10.0f), 4.0f);
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.setColour(Colours::ink);
    g.drawText("REF " + juce::String(lane + 1), strip.toNearestInt().removeFromTop(18), juce::Justification::centred);
  }
  g.setFont(juce::FontOptions(11.0f));
  g.setColour(Colours::muted);
  g.drawFittedText(referenceStatus_, bounds.reduced(16).removeFromBottom(24), juce::Justification::centredLeft, 1);
}

void AifredAudioProcessorEditor::drawCompare(juce::Graphics& g, juce::Rectangle<int> bounds) {
  auto top = bounds.removeFromTop(juce::roundToInt(bounds.getHeight() * 0.66f));
  auto left = top.removeFromLeft(juce::roundToInt(top.getWidth() * 0.40f)).reduced(0, 0);
  auto vu = top.removeFromLeft(juce::jlimit(150, 230, juce::roundToInt(top.getWidth() * 0.30f))).reduced(12, 38);
  auto right = top.reduced(0, 0);
  drawHalo(g, left, state_, "MIX A", false);
  drawCompareVu(g, vu, state_, compareState_);
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
    g.drawText(signedText((b - a) * 100.0f, "pts"), row, juce::Justification::centredRight);
  }
}

void AifredAudioProcessorEditor::drawCompareVu(juce::Graphics& g, juce::Rectangle<int> bounds, const HaloState& a, const HaloState& b) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto inner = bounds.reduced(14);
  const auto diff = 0.28f * std::abs(a.metrics.tone01 - b.metrics.tone01)
                  + 0.24f * std::abs(a.metrics.width01 - b.metrics.width01)
                  + 0.22f * std::abs(a.metrics.punch01 - b.metrics.punch01)
                  + 0.16f * std::abs(a.metrics.loudness01 - b.metrics.loudness01)
                  + 0.10f * clamp01(std::abs(a.metrics.peakDb - b.metrics.peakDb) / 12.0f);
  const auto closeness = 1.0f - clamp01(diff);
  g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("MATCH VU", inner.removeFromTop(24), juce::Justification::centred);
  auto dial = inner.removeFromTop(std::min(inner.getWidth(), inner.getHeight())).toFloat();
  const auto centre = dial.getCentre();
  const auto radius = std::min(dial.getWidth(), dial.getHeight()) * 0.42f;
  g.setColour(Colours::line.withAlpha(0.56f));
  juce::Path arc;
  arc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, juce::degreesToRadians(-132.0f), juce::degreesToRadians(132.0f), true);
  g.strokePath(arc, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
  juce::Path hot;
  hot.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, juce::degreesToRadians(-132.0f), juce::degreesToRadians(-132.0f + 264.0f * closeness), true);
  g.setColour(colourForAlignment(closeness).withAlpha(0.94f));
  g.strokePath(hot, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
  const auto needleAngle = juce::degreesToRadians(-132.0f + 264.0f * closeness);
  g.setColour(Colours::ink);
  g.drawLine(centre.x, centre.y, centre.x + std::cos(needleAngle) * radius * 0.82f, centre.y + std::sin(needleAngle) * radius * 0.82f, 2.0f);
  g.setColour(Colours::cyan.withAlpha(0.50f));
  g.fillEllipse(centre.x - 5.0f, centre.y - 5.0f, 10.0f, 10.0f);
  g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
  g.setColour(colourForAlignment(closeness));
  g.drawText(pct(closeness), inner.removeFromTop(34), juce::Justification::centred);
  g.setFont(juce::FontOptions(11.0f));
  g.setColour(Colours::muted);
  g.drawFittedText("A/B distance: tone, width, punch, LUFS, dBFS peak", inner, juce::Justification::centred, 3);
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
  juce::Path referencePath;
  const std::array<float, 4> referenceValues {
    state.reference.tone01,
    state.reference.width01,
    state.reference.punch01,
    clamp01((state.reference.loudnessDb + 24.0f) / 18.0f)
  };
  for (int i = 0; i < 4; ++i) {
    const auto angle = juce::MathConstants<float>::twoPi * static_cast<float>(i) / 4.0f - juce::MathConstants<float>::halfPi;
    const auto radius = std::min(graph.getWidth(), graph.getHeight()) * 0.42f * referenceValues[static_cast<size_t>(i)];
    const auto point = juce::Point<float>(centre.x + std::cos(angle) * radius, centre.y + std::sin(angle) * radius);
    if (i == 0) referencePath.startNewSubPath(point); else referencePath.lineTo(point);
  }
  referencePath.closeSubPath();
  g.setColour(Colours::violet.withAlpha(0.13f));
  g.fillPath(referencePath);
  g.setColour(Colours::green.withAlpha(0.56f));
  g.strokePath(referencePath, juce::PathStrokeType(1.6f));

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
  g.setFont(juce::FontOptions(10.5f));
  g.setColour(Colours::muted);
  auto graphInt = graph.toNearestInt();
  g.drawText("Aurora underlay = pro reference signature", graphInt.removeFromBottom(18), juce::Justification::centred);
}

void AifredAudioProcessorEditor::drawSpectrumMeter(juce::Graphics& g, juce::Rectangle<int> bounds, const HaloState& state) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto inner = bounds.reduced(14, 10);
  g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("SPECTROMETER", inner.removeFromTop(24), juce::Justification::centredLeft);
  auto plot = inner.reduced(0, 4).toFloat();
  const std::array<const char*, 8> labels {"40Hz", "90Hz", "180Hz", "400Hz", "1k", "3k", "8k", "SIDE"};
  const auto barHeight = plot.getHeight() / static_cast<float>(labels.size());
  for (size_t i = 0; i < labels.size(); ++i) {
    const auto value = clamp01(state.metrics.spectrumBands[i]);
    auto row = juce::Rectangle<float>(plot.getX(), plot.getY() + static_cast<float>(i) * barHeight + 1.0f, plot.getWidth(), std::max(3.0f, barHeight - 2.0f));
    auto label = row.removeFromLeft(42.0f);
    auto slot = row.reduced(2.0f, 2.0f);
    g.setColour(Colours::line.withAlpha(0.42f));
    g.fillRoundedRectangle(slot, 4.0f);
    auto fill = slot.withWidth(std::max(2.0f, slot.getWidth() * value));
    const auto colour = i < 2 ? Colours::green : (i < 5 ? Colours::cyan : (i == 6 ? Colours::violet : Colours::yellow));
    g.setColour(colour.withAlpha(0.86f));
    g.fillRoundedRectangle(fill, 4.0f);
    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
    g.setColour(Colours::muted);
    g.drawText(labels[i], label.toNearestInt(), juce::Justification::centredRight);
  }
}

void AifredAudioProcessorEditor::drawCorrelationMeter(juce::Graphics& g, juce::Rectangle<int> bounds, const HaloState& state) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto inner = bounds.reduced(14, 10);
  g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("CORRELATION", inner.removeFromTop(22), juce::Justification::centredLeft);
  auto meter = inner.removeFromTop(18).toFloat();
  g.setColour(Colours::line.withAlpha(0.48f));
  g.fillRoundedRectangle(meter, 6.0f);
  const auto zeroX = meter.getX() + meter.getWidth() * 0.5f;
  const auto valueX = meter.getX() + meter.getWidth() * clamp01((state.metrics.correlation + 1.0f) * 0.5f);
  g.setColour(state.metrics.correlation < 0.0f ? Colours::red : Colours::green);
  g.fillRoundedRectangle(juce::Rectangle<float>(std::min(zeroX, valueX), meter.getY(), std::abs(valueX - zeroX), meter.getHeight()), 6.0f);
  g.setColour(Colours::cyan.withAlpha(0.9f));
  g.drawVerticalLine(juce::roundToInt(valueX), meter.getY() - 3.0f, meter.getBottom() + 3.0f);
  g.setFont(juce::FontOptions(11.0f));
  g.setColour(Colours::muted);
  g.drawText("HPF 150 Hz     -1 phase risk        0 mono-safe        +1 locked", inner.removeFromTop(18), juce::Justification::centred);
  g.setColour(Colours::ink);
  g.drawText("corr " + juce::String(state.metrics.correlation, 2), inner, juce::Justification::centred);
}

} // namespace aifred
