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
float gLayoutScale = 1.0f;
float gFontScale = 1.0f;
float gPaddingScale = 1.0f;

void updateUiScale(juce::Rectangle<int> bounds) {
  constexpr float baseWidth = 1280.0f;
  constexpr float baseHeight = 760.0f;
  const auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForRect(bounds);
  const float dpiScale = juce::jlimit(0.85f, 1.35f, static_cast<float>(display != nullptr ? display->scale : 1.0));
  const float sizeScale = std::min(bounds.getWidth() / baseWidth, bounds.getHeight() / baseHeight);
  gLayoutScale = juce::jlimit(0.88f, 1.42f, sizeScale * dpiScale);
  gFontScale = juce::jlimit(1.0f, 1.30f, gLayoutScale);
  gPaddingScale = juce::jlimit(0.88f, 1.16f, gLayoutScale);
}

int scaledInt(int value) {
  return juce::roundToInt(static_cast<float>(value) * gPaddingScale);
}

juce::FontOptions uiFont(float basePx, float minPx, int style = juce::Font::plain) {
  return juce::FontOptions(juce::jmax(minPx, basePx * gFontScale), style);
}

juce::Colour accentForMode(AnalysisMode mode) {
  if (mode == AnalysisMode::Compare) return Colours::green;
  if (mode == AnalysisMode::Reference) return Colours::violet;
  return Colours::cyan;
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

juce::String cleanAiDisplayText(juce::String text) {
  text = text.trim();
  if ((text.startsWith("{") && text.endsWith("}")) || (text.startsWith("[") && text.endsWith("]"))) {
    return "I received structured model data instead of a normal answer. Ask again in plain language and I will respond conversationally from the current mix snapshot.";
  }
  text = text.replace("```json", "").replace("```", "");
  text = text.replace("\\n", "\n").replace("\\\"", "\"");
  text = text.replace("\\u2019", "'").replace("u2019", "'");
  text = text.replace("\\u2018", "'").replace("u2018", "'");
  text = text.replace("\\u201c", "\"").replace("u201c", "\"");
  text = text.replace("\\u201d", "\"").replace("u201d", "\"");
  text = text.replace("NaN", "unavailable").replace("undefined", "unavailable").replace("Infinity", "unavailable");
  return text.trim();
}

float metricValue(const HaloState& state, int index) {
  switch (index) {
    case 0: return state.toneScore01;
    case 1: return state.widthScore01;
    case 2: return state.punchScore01;
    default: return state.loudnessScore01;
  }
}

juce::String AifredAudioProcessorEditor::scoreText(float score, const HaloState& state, Domain domain) {
  if (!state.hasSignal) return "Waiting";
  if (!state.valuesValid) return "--";

  const auto mode = processor_.getMode();
  if (mode != AnalysisMode::Analyze) {
    if (!state.hasReference) return "No Ref";
    return pct(score);
  }

  // Raw technical metrics for Analyze mode
  switch (domain) {
    case Domain::Tone:
      return juce::String(state.metrics.spectralTilt, 2) + " tilt";
    case Domain::Stereo:
      return juce::String(state.metrics.stereoWidth, 2) + " width";
    case Domain::Dynamics:
      return juce::String(state.metrics.crestDb, 1) + " crest";
    case Domain::Loudness:
      return juce::String(state.metrics.integratedLufs, 1) + " LUFS";
  }
  return pct(score);
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
  for (auto* button : {&askAiButton_, &saveApiButton_, &chatFileButton_, &compareFileButton_}) {
    addAndMakeVisible(button);
    button->addListener(this);
  }
  for (auto& button : referenceFileButtons_) {
    addAndMakeVisible(button);
    button.addListener(this);
  }

  chatInput_.setMultiLine(true);
  chatInput_.setReturnKeyStartsNewLine(true);
  chatInput_.setTextToShowWhenEmpty("Type your question.", Colours::muted);
  addAndMakeVisible(chatInput_);

  chatOutput_.setMultiLine(true);
  chatOutput_.setReadOnly(true);
  chatOutput_.setScrollbarsShown(true);
  chatOutput_.setCaretVisible(false);
  chatOutput_.setTextToShowWhenEmpty("AIFRED answers will appear here.", Colours::muted);
  chatOutput_.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
  chatOutput_.setColour(juce::TextEditor::outlineColourId, Colours::line);
  chatOutput_.setColour(juce::TextEditor::focusedOutlineColourId, Colours::cyan);
  chatOutput_.setColour(juce::TextEditor::textColourId, Colours::ink);
  chatOutput_.setColour(juce::TextEditor::highlightColourId, Colours::cyan.withAlpha(0.22f));
  addAndMakeVisible(chatOutput_);

  apiEndpoint_.setTextToShowWhenEmpty("http://127.0.0.1:11434 or https://api.openai.com/v1", Colours::muted);
  apiKey_.setPasswordCharacter('*');
  apiKey_.setTextToShowWhenEmpty("OpenAI key or local proxy token", Colours::muted);
  aiModel_.setTextToShowWhenEmpty("aifred:latest, llama3.2:3b, or compatible model", Colours::muted);
  addAndMakeVisible(apiEndpoint_);
  addAndMakeVisible(apiKey_);
  addAndMakeVisible(aiModel_);

  providerMenu_.addItem("OpenAI", 1);
  providerMenu_.addItem("OpenAI-compatible", 2);
  providerMenu_.addItem("Ollama / Local", 3);
  genreMenu_.addItem("Hip-Hop / Trap", 1);
  genreMenu_.addItem("Boom Bap", 2);
  genreMenu_.addItem("Drill / Trap", 3);
  genreMenu_.addItem("Electronic / Dubstep", 4);
  genreMenu_.addItem("R&B", 5);
  genreMenu_.addItem("Same Genre", 6);
  gateSlider_.setRange(0.0, 1.0, 0.01);
  gateSlider_.setTextValueSuffix(" gate");
  addAndMakeVisible(providerMenu_);
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
  genreMenu_.setSelectedId(settings.genreId);
  providerMenu_.setSelectedId(settings.aiProvider == "ollama" ? 3 : (settings.aiProvider == "compatible" ? 2 : 1));
  gateSlider_.setValue(settings.gate, juce::dontSendNotification);
  apiEndpoint_.setText(settings.apiEndpoint, juce::dontSendNotification);
  apiKey_.setText(settings.apiKey, juce::dontSendNotification);
  aiModel_.setText(settings.aiModel, juce::dontSendNotification);

  setResizable(true, true);
  setResizeLimits(1080, 680, 1820, 1120);
  setSize(1360, 820);
  showTutorial_ = !processor_.isSessionInitialized() && !gTutorialShownThisSession;
  AifredEngineClient::instance().pingHealthAsync();
  startTimerHz(30);
}

AifredAudioProcessorEditor::~AifredAudioProcessorEditor() {
  for (auto* button : {&analyzeButton_, &referenceButton_, &compareButton_, &optionsButton_, &tutorialButton_, &centerModeButton_}) {
    button->removeListener(this);
  }
  for (auto* button : {&askAiButton_, &saveApiButton_, &chatFileButton_, &compareFileButton_}) {
    button->removeListener(this);
  }
  for (auto& button : referenceFileButtons_) {
    button.removeListener(this);
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
    if (prompt.isEmpty()) {
      chatOutputText_.clear();
    } else {
      chatOutputText_ = "You: " + prompt + "\n\nAIFRED: Reading current mix snapshot...";
      AifredEngineClient::instance().askAsync(prompt, diagnostic_.aiContextJson);
    }
    chatOutput_.setText(chatOutputText_, juce::dontSendNotification);
    chatOutput_.moveCaretToEnd();
  }
  if (button == &saveApiButton_) {
    pushSettingsToProcessor();
    apiStatus_ = apiEndpoint_.getText().trim().isNotEmpty() ? "API route set." : "API route not connected.";
  }
  if (button == &chatFileButton_ || button == &compareFileButton_) {
    fileChooser_ = std::make_unique<juce::FileChooser>("Select audio file", juce::File{}, "*.wav;*.aif;*.aiff;*.mp3;*.flac");
    fileChooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
      [this, button](const juce::FileChooser& chooser) {
        const auto file = chooser.getResult();
        if (!file.existsAsFile()) return;
        if (button == &chatFileButton_) chatFileStatus_ = "Chat file: " + file.getFileName();
        if (button == &compareFileButton_) compareStatus_ = "Compare file: " + file.getFileName();
        repaint();
      });
  }
  for (int i = 0; i < static_cast<int>(referenceFileButtons_.size()); ++i) {
    if (button == &referenceFileButtons_[static_cast<size_t>(i)]) {
      fileChooser_ = std::make_unique<juce::FileChooser>("Select reference " + juce::String(i + 1), juce::File{}, "*.wav;*.aif;*.aiff;*.mp3;*.flac");
      fileChooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, i](const juce::FileChooser& chooser) {
          const auto file = chooser.getResult();
          if (!file.existsAsFile()) return;
          referenceFileNames_[static_cast<size_t>(i)] = file.getFileName();
          referenceStatus_ = "Loaded " + juce::String(i + 1) + "/5 reference slots.";
          repaint();
        });
    }
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
  const auto engineResponse = AifredEngineClient::instance().lastResponse();
  if (engineResponse.isNotEmpty() && !AifredEngineClient::instance().hasPendingChat()) {
    const auto cleaned = cleanAiDisplayText(engineResponse);
    const auto nextText = chatOutputText_.contains("AIFRED: Reading current mix snapshot...")
      ? chatOutputText_.replace("AIFRED: Reading current mix snapshot...", "AIFRED: " + cleaned)
      : cleaned;
    if (nextText != chatOutputText_) {
      chatOutputText_ = nextText;
      chatOutput_.setText(chatOutputText_, juce::dontSendNotification);
      chatOutput_.moveCaretToEnd();
    }
  }
  repaint();
}

void AifredAudioProcessorEditor::pushSettingsToProcessor() {
  PluginSettings settings;
  settings.themeId = 1;
  settings.layoutId = 3;
  settings.genreId = genreMenu_.getSelectedId();
  settings.gate = gateSlider_.getValue();
  settings.aiProvider = providerMenu_.getSelectedId() == 3 ? "ollama" : (providerMenu_.getSelectedId() == 2 ? "compatible" : "openai");
  settings.apiEndpoint = apiEndpoint_.getText().trim();
  settings.apiKey = apiKey_.getText();
  settings.aiModel = aiModel_.getText().trim();
  processor_.setPluginSettings(settings);
}

void AifredAudioProcessorEditor::paint(juce::Graphics& g) {
  state_.uiRenderTimestampMs = juce::Time::getMillisecondCounterHiRes();
  auto bounds = getLocalBounds();
  const auto mode = processor_.getMode();
  auto accent = accentForMode(mode);
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

  drawHeader(g, bounds.removeFromTop(scaledInt(88)).reduced(scaledInt(18), scaledInt(12)));

  auto main = bounds.reduced(scaledInt(18), scaledInt(10));
  if (mode == AnalysisMode::Compare) {
    drawCompare(g, main);
  } else {
    const auto leftFraction = 0.23f;
    const auto rightFraction = 0.44f;
    auto left = main.removeFromLeft(juce::roundToInt(main.getWidth() * leftFraction));
    auto right = main.removeFromRight(juce::roundToInt(main.getWidth() * rightFraction));
    auto center = main.reduced(14, 0);

    drawMixSignature(g, left.removeFromTop(210).reduced(0, 0), state_);
    drawSpectrumMeter(g, left.removeFromTop(134).reduced(0, 10), state_);
    drawCorrelationMeter(g, left.removeFromTop(74).reduced(0, 10), state_);
    drawCandles(g, left.reduced(0, 12), state_);
    drawHalo(g, center.reduced(0, 10), state_, mode == AnalysisMode::Reference ? "REFERENCE HALO" : "ANALYZE HALO", mode == AnalysisMode::Reference);

    if (mode == AnalysisMode::Reference) {
      drawReferenceMixer(g, right.removeFromTop(236).reduced(0, 0));
    } else {
      drawDomainCard(g, right.removeFromTop(68).reduced(0, 0), "TONE", Domain::Tone, state_.tone, state_);
      drawDomainCard(g, right.removeFromTop(68).reduced(0, 8), "WIDTH", Domain::Stereo, state_.stereo, state_);
    }
    drawDomainCard(g, right.removeFromTop(68).reduced(0, 8), "PUNCH", Domain::Dynamics, state_.dynamics, state_);
    drawDomainCard(g, right.removeFromTop(68).reduced(0, 8), "LOUDNESS", Domain::Loudness, state_.loudness, state_);
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
    g.drawFittedText(apiStatus_ + "\nLayout: Chat Focus\n" + juce::String(genreName(genreMenu_.getSelectedId())), inner.removeFromTop(64), juce::Justification::topLeft, 3);
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
    g.drawFittedText("Meters continue to update. Local chat route: http://127.0.0.1:8787.", inner, juce::Justification::topLeft, 3);
  }
}

void AifredAudioProcessorEditor::resized() {
  updateUiScale(getLocalBounds());
  chatInput_.applyFontToAllText(juce::Font(uiFont(14.0f, 13.5f)));
  chatOutput_.applyFontToAllText(juce::Font(uiFont(14.0f, 13.5f)));
  apiEndpoint_.applyFontToAllText(juce::Font(uiFont(13.5f, 13.0f)));
  apiKey_.applyFontToAllText(juce::Font(uiFont(13.5f, 13.0f)));
  aiModel_.applyFontToAllText(juce::Font(uiFont(13.5f, 13.0f)));

  auto header = getLocalBounds().removeFromTop(scaledInt(88)).reduced(scaledInt(18), scaledInt(12));
  auto modes = header.removeFromRight(scaledInt(390)).reduced(scaledInt(4), scaledInt(14));
  analyzeButton_.setBounds(modes.removeFromLeft(scaledInt(122)).reduced(scaledInt(5), 0));
  referenceButton_.setBounds(modes.removeFromLeft(scaledInt(132)).reduced(scaledInt(5), 0));
  compareButton_.setBounds(modes.removeFromLeft(scaledInt(122)).reduced(scaledInt(5), 0));
  auto leftTools = header.removeFromRight(scaledInt(220)).reduced(scaledInt(4), scaledInt(14));
  auto centerTool = header.removeFromRight(scaledInt(100)).reduced(scaledInt(4), scaledInt(14));
  centerModeButton_.setBounds(centerTool.reduced(scaledInt(5), 0));
  optionsButton_.setBounds(leftTools.removeFromLeft(scaledInt(104)).reduced(scaledInt(5), 0));
  tutorialButton_.setBounds(leftTools.removeFromLeft(scaledInt(112)).reduced(scaledInt(5), 0));

  const auto mode = processor_.getMode();
  auto body = getLocalBounds().withTrimmedTop(scaledInt(88)).reduced(scaledInt(18), scaledInt(10));
  if (mode != AnalysisMode::Compare) {
    auto right = body.removeFromRight(juce::roundToInt(body.getWidth() * 0.44f)).reduced(0, 8);
    if (mode == AnalysisMode::Reference) {
      auto mixer = right.removeFromTop(236).reduced(16);
      mixer.removeFromTop(50); // Header + meta
      
      const auto totalLanes = 5;
      const auto laneW = mixer.getWidth() / totalLanes;
      
      auto buttonRow = mixer.removeFromTop(32);
      auto sliderArea = mixer;

      for (int i = 0; i < totalLanes; ++i) {
        auto laneX = mixer.getX() + (i * laneW);
        referenceFileButtons_[static_cast<size_t>(i)].setBounds(laneX + 4, buttonRow.getY(), laneW - 8, buttonRow.getHeight());
        referenceVolumeSliders_[static_cast<size_t>(i)].setBounds(laneX + 4, sliderArea.getY(), laneW - 8, sliderArea.getHeight());
      }
    }
    else {
      right.removeFromTop(68);
      right.removeFromTop(68);
      for (auto& button : referenceFileButtons_) button.setBounds({});
      for (auto& slider : referenceVolumeSliders_) slider.setBounds({});
    }
    right.removeFromTop(68);
    right.removeFromTop(68);
    auto chat = right.reduced(0, 8).reduced(16);
    chat.removeFromTop(30);
    chatInput_.setBounds(chat.removeFromTop(84));
    auto buttons = chat.removeFromTop(38);
    chatFileButton_.setBounds(buttons.removeFromLeft(buttons.getWidth() / 2).reduced(0, 4));
    askAiButton_.setBounds(buttons.reduced(6, 4));
    chatOutput_.setBounds(chat.reduced(0, 4));
  } else {
    chatInput_.setBounds({});
    chatOutput_.setBounds({});
    askAiButton_.setBounds({});
    chatFileButton_.setBounds({});
    for (auto& button : referenceFileButtons_) button.setBounds({});
    auto compareButtonArea = getLocalBounds().withTrimmedTop(104).removeFromRight(210).reduced(18, 0);
    compareFileButton_.setBounds(compareButtonArea.removeFromTop(36));
    for (auto& slider : referenceVolumeSliders_) slider.setBounds({});
  }
  if (mode != AnalysisMode::Compare) compareFileButton_.setBounds({});

  if (showOptions_) {
    auto panel = getLocalBounds().withSizeKeepingCentre(560, 420).reduced(22);
    panel.removeFromTop(110);
    providerMenu_.setBounds(panel.removeFromTop(30));
    genreMenu_.setBounds(panel.removeFromTop(34));
    gateSlider_.setBounds(panel.removeFromTop(42));
    apiEndpoint_.setBounds(panel.removeFromTop(30));
    apiKey_.setBounds(panel.removeFromTop(30));
    aiModel_.setBounds(panel.removeFromTop(30));
    saveApiButton_.setBounds(panel.removeFromTop(34).reduced(0, 4));
  } else {
    providerMenu_.setBounds({});
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
  auto accent = referenceOverlay ? genreColour(genreMenu_.getSelectedId()) : accentForMode(processor_.getMode());
  const auto dynamicPulse = clamp01(0.28f * state.loudnessScore01 + 0.26f * state.widthScore01 + 0.20f * state.punchScore01 + 0.18f * state.toneScore01);
  const auto pulse = 0.82f + dynamicPulse * 0.14f;

  g.setColour(accent.withAlpha(referenceOverlay ? 0.18f : 0.10f));
  g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
  g.setColour(accent.withAlpha(0.10f));
  g.drawEllipse(centre.x - radius * pulse, centre.y - radius * pulse, radius * 2.0f * pulse, radius * 2.0f * pulse, 3.0f + 3.0f * state.metrics.width01);

  const std::array<float, 4> values {state.punchScore01, state.toneScore01, state.loudnessScore01, state.widthScore01};
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
    g.setFont(uiFont(13.0f, 13.0f, juce::Font::bold));
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

  struct ScaleLabel { float angle; juce::String text; juce::Colour colour; };
  const std::array<ScaleLabel, 8> scaleLabels {{
    {-150.0f, "DRY", Colours::cyan},
    {-78.0f, "PUNCH " + juce::String(state.metrics.crestDb, 1) + " dB", Colours::cyan},
    {-60.0f, "DARK", Colours::green},
    {12.0f, "TONE " + scoreText(state.toneScore01, state, Domain::Tone), Colours::green},
    {30.0f, "-24", Colours::yellow},
    {102.0f, "ST " + juce::String(state.metrics.shortTermLufs, 1) + " / TP " + juce::String(state.metrics.truePeakDb, 1), Colours::yellow},
    {120.0f, "-1", Colours::violet},
    {192.0f, "CORR " + juce::String(state.metrics.correlation, 2), Colours::violet}
  }};
  for (const auto& item : scaleLabels) {
    const auto angle = juce::degreesToRadians(item.angle);
    const auto labelRadius = radius + 111.0f;
    const auto p = juce::Point<float>(centre.x + std::cos(angle) * labelRadius,
                                      centre.y + std::sin(angle) * labelRadius);
    g.setFont(uiFont(9.2f, 11.0f, juce::Font::bold));
    g.setColour(item.colour.withAlpha(0.88f));
    g.drawFittedText(item.text, juce::Rectangle<float>(p.x - 58.0f, p.y - 8.0f, 116.0f, 17.0f).toNearestInt(),
                     juce::Justification::centred, 1);
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
  g.setFont(uiFont(14.0f, 13.0f, juce::Font::bold));
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

void AifredAudioProcessorEditor::drawDomainCard(juce::Graphics& g, juce::Rectangle<int> bounds, const char* name, const DomainAlignment& alignment, const HaloState& state) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto inner = bounds.reduced(12, 8);
  g.setFont(uiFont(13.0f, 13.0f, juce::Font::bold));
  g.setColour(Colours::green);
  g.drawText(name, inner.removeFromTop(18), juce::Justification::centredLeft);
  auto bar = inner.removeFromBottom(12).toFloat();
  g.setColour(Colours::line.withAlpha(0.5f));
  g.fillRoundedRectangle(bar, 5.0f);
  g.setColour(colourForAlignment(alignment.alignment01));
  g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * alignment.alignment01), 5.0f);
  g.setFont(uiFont(22.0f, 18.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText(scoreText(alignment.alignment01, state), inner.removeFromTop(32), juce::Justification::centredLeft);
  g.setFont(uiFont(11.5f, 13.0f));
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
  g.drawText("Left: ten saved sessions. Right: current session timeline, one candle per minute.", inner.removeFromTop(20), juce::Justification::centredLeft);

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
  auto header = inner.removeFromTop(30);
  g.setFont(uiFont(18.0f, 18.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("CHAT", header, juce::Justification::centredLeft);

  const auto engineReady = AifredEngineClient::instance().isAvailable();
  auto metricRow = inner.removeFromTop(66);
  auto left = metricRow.removeFromLeft(metricRow.getWidth() / 2).reduced(0, 2);
  auto right = metricRow.reduced(8, 2);

  auto drawMetric = [&g](juce::Rectangle<int> area, const juce::String& label, const juce::String& value, juce::Colour accent) {
    g.setColour(Colours::line.withAlpha(0.55f));
    g.fillRoundedRectangle(area.toFloat(), 6.0f);
    g.setColour(accent.withAlpha(0.16f));
    g.fillRoundedRectangle(area.toFloat().reduced(1.0f), 6.0f);
    g.setColour(Colours::ink);
    g.setFont(uiFont(10.5f, 10.0f, juce::Font::bold));
    g.drawText(label, area.removeFromTop(18), juce::Justification::centredLeft);
    g.setFont(uiFont(14.0f, 13.0f, juce::Font::bold));
    g.drawText(value, area, juce::Justification::centredLeft);
  };

  auto topLeft = left.removeFromTop(30);
  auto bottomLeft = left.reduced(0, 0);
  drawMetric(topLeft, "Integrated LUFS", juce::String(state_.metrics.integratedLufs, 1), Colours::cyan);
  drawMetric(bottomLeft, "True Peak", juce::String(state_.metrics.truePeakDb, 1) + " dB", Colours::green);

  auto topRight = right.removeFromTop(30);
  auto bottomRight = right.reduced(0, 0);
  drawMetric(topRight, "Crest / Corr", juce::String(state_.metrics.crestDb, 1) + " dB / " + juce::String(state_.metrics.correlation, 2), Colours::yellow);
  drawMetric(bottomRight, "Tone / Width / Punch", pct(state_.metrics.tone01) + " / " + pct(state_.metrics.width01) + " / " + pct(state_.metrics.punch01), Colours::violet);

  auto summary = inner.removeFromTop(56);
  g.setColour(Colours::muted);
  g.setFont(uiFont(12.5f, 12.0f));
  g.drawFittedText(juce::String(state_.humanSummary).isNotEmpty() ? juce::String(state_.humanSummary) : juce::String(diagnostic_.summary), summary, juce::Justification::topLeft, 4);

  auto footer = inner.removeFromBottom(40);
  g.setColour(engineReady ? Colours::green : Colours::yellow);
  g.setFont(uiFont(12.0f, 12.0f, juce::Font::bold));
  g.drawFittedText(engineReady ? "Local AI ready." : AifredEngineClient::instance().statusText(), footer.removeFromTop(18), juce::Justification::bottomLeft, 1);
  g.setColour(Colours::muted);
  g.setFont(uiFont(11.0f, 11.0f));
  g.drawFittedText(juce::String(diagnostic_.fixList).isNotEmpty() ? diagnostic_.fixList : "Ask AI for a live mix readout. Output scrolls in the response panel below.", footer, juce::Justification::bottomLeft, 3);
}

void AifredAudioProcessorEditor::drawReferenceMixer(juce::Graphics& g, juce::Rectangle<int> bounds) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto inner = bounds.reduced(16);
  g.setFont(juce::FontOptions(17.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("REFERENCE MIXER", inner.removeFromTop(30), juce::Justification::centredLeft);
  
  // Header area for metadata
  auto header = inner.removeFromTop(20);
  g.setFont(juce::FontOptions(11.0f));
  g.setColour(Colours::muted);
  g.drawText("5-Track Parallel Reference Bus", header, juce::Justification::centredLeft);

  inner.removeFromTop(10); // Spacing

  const int lanes = 5;
  const auto laneWidth = inner.getWidth() / lanes;
  const auto accent = genreColour(genreMenu_.getSelectedId());

  for (int lane = 0; lane < lanes; ++lane) {
    auto strip = inner.removeFromLeft(laneWidth).reduced(4, 0).toFloat();
    
    // Track backing
    g.setColour(Colours::line.withAlpha(0.18f));
    g.fillRoundedRectangle(strip, 6.0f);
    
    auto content = strip.reduced(6.0f, 8.0f);
    
    // Slot Label
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(Colours::muted);
    g.drawText("SLOT " + juce::String(lane + 1), content.removeFromTop(16).toNearestInt(), juce::Justification::centred);
    
    // File Name (Middle)
    auto fileArea = content.removeFromTop(content.getHeight() * 0.4f).reduced(2);
    const auto loadedName = referenceFileNames_[static_cast<size_t>(lane)].isNotEmpty()
      ? referenceFileNames_[static_cast<size_t>(lane)]
      : "(empty)";
    g.setFont(juce::FontOptions(9.0f));
    g.setColour(loadedName == "(empty)" ? Colours::muted : Colours::ink);
    g.drawFittedText(loadedName, fileArea.toNearestInt(), juce::Justification::centred, 3);

    // Fader Area (Bottom)
    auto faderTrack = content.reduced(8.0f, 4.0f);
    g.setColour(Colours::line.withAlpha(0.3f));
    g.fillRoundedRectangle(faderTrack.withWidth(4.0f).withCentre({strip.getCentreX(), faderTrack.getCentreY()}), 2.0f);
    
    const auto volume = static_cast<float>(referenceVolumeSliders_[static_cast<size_t>(lane)].getValue() / 100.0);
    const auto pos = faderTrack.getBottom() - faderTrack.getHeight() * volume;
    
    g.setColour(accent.withAlpha(0.92f));
    g.fillRoundedRectangle(juce::Rectangle<float>(strip.getCentreX() - 15.0f, pos - 4.0f, 30.0f, 8.0f), 3.0f);
    g.setColour(Colours::ink);
    g.drawRoundedRectangle(juce::Rectangle<float>(strip.getCentreX() - 15.0f, pos - 4.0f, 30.0f, 8.0f), 3.0f, 1.0f);
  }
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
  const auto diff = 0.28f * std::abs(a.toneScore01 - b.toneScore01)
                  + 0.24f * std::abs(a.widthScore01 - b.widthScore01)
                  + 0.22f * std::abs(a.punchScore01 - b.punchScore01)
                  + 0.16f * std::abs(a.loudnessScore01 - b.loudnessScore01)
                  + 0.10f * clamp01(std::abs(a.metrics.peakDb - b.metrics.peakDb) / 12.0f);
  const auto closeness = (a.hasSignal && b.hasSignal && a.valuesValid && b.valuesValid) ? (1.0f - clamp01(diff)) : 0.0f;
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
  g.drawText((a.hasSignal && b.hasSignal && a.valuesValid && b.valuesValid) ? pct(closeness) : "--", inner.removeFromTop(34), juce::Justification::centred);
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
