#include "PluginEditor.h"

#include <BinaryData.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include <array>
#include <algorithm>
#include <cmath>

#ifndef AIFRED_VERSION_STRING
#define AIFRED_VERSION_STRING "dev"
#endif
#ifndef AIFRED_BUILD_ID
#define AIFRED_BUILD_ID "unknown"
#endif

namespace aifred {
namespace {

constexpr int kDesignWidth = 1360;
constexpr int kDesignHeight = 820;
constexpr int kHeaderHeight = 76;
constexpr int kRightCardHeight = 54;

juce::FontOptions uiFont(float basePx, float minPx, int style = juce::Font::plain) {
  return juce::FontOptions(juce::jmax(minPx, basePx), style);
}

juce::Colour accentForMode(AnalysisMode mode) {
  if (mode == AnalysisMode::Compare) return Colours::green;
  if (mode == AnalysisMode::Reference) return Colours::violet;
  return Colours::cyan;
}

juce::Colour appearanceColour(int appearanceId) {
  switch (appearanceId) {
    case 2: return juce::Colour(0xffffcf33);
    case 3: return juce::Colour(0xff23e3ff);
    case 4: return juce::Colour(0xffb86cff);
    case 5: return juce::Colour(0xff496170);
    default: return juce::Colour(0xff8cff45);
  }
}

const char* appearanceName(int appearanceId) {
  switch (appearanceId) {
    case 2: return "Pulse";
    case 3: return "Spectrum Glow";
    case 4: return "Minimal";
    case 5: return "Dark / Static";
    default: return "Aurora";
  }
}

void drawPanel(juce::Graphics& g, juce::Rectangle<float> bounds, float radius = 8.0f) {
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff08131f), bounds.getX(), bounds.getY(),
                                         juce::Colour(0xff03070c), bounds.getRight(), bounds.getBottom(), false));
  g.fillRoundedRectangle(bounds, radius);
  g.setColour(Colours::line.withAlpha(0.82f));
  g.drawRoundedRectangle(bounds, radius, 1.0f);
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

float metricValue(const BetaView& state,int index) {
    const std::array<float,4> values {state.metrics.rmsScale,state.metrics.widthScale,state.metrics.crestScale,state.metrics.loudnessScale};
    return values[static_cast<std::size_t>(std::clamp(index,0,3))];
  }
  float metricRawValue(const BetaView& state,int index) {
    const std::array<float,4> values {state.metrics.rmsDb,state.metrics.stereoWidth*100.0f,state.metrics.crestDb,state.metrics.shortTermLufs};
    return values[static_cast<std::size_t>(std::clamp(index,0,3))];
  }
  const char* metricDeltaUnit(int index) {
    const std::array<const char*,4> units {"dB","pp","dB","LU"};
    return units[static_cast<std::size_t>(std::clamp(index,0,3))];
  }
  const char* metricLabel(int index) {
  switch (index) {
    case 0: return "RMS";
    case 1: return "Width";
    case 2: return "Crest";
    default: return "Loudness";
  }
}

} // namespace

juce::String AifredAudioProcessorEditor::metricText(const BetaView& state,Domain domain) {
    const auto id=domain==Domain::Tone?core::MetricId::rms:domain==Domain::Stereo?core::MetricId::width:domain==Domain::Dynamics?core::MetricId::crest:core::MetricId::shortTerm;
    const auto& m=state.observation.get(id);const auto& d=core::metricDefinitions[core::index(id)];
    return m.valid?juce::String(core::Filter::published(m.typical,d.decimals),d.decimals)+" "+juce::String(d.unit.data()):"--";
  }
  AifredAudioProcessorEditor::AifredAudioProcessorEditor(AifredAudioProcessor& ownerProcessor)
  : AudioProcessorEditor(&ownerProcessor), processor_(ownerProcessor) {
  addAndMakeVisible(profileMenu_);
  profileMenu_.setTooltip("DSP SETUP / PROFILE");
  for(std::size_t i=0;i<core::profiles.size();++i)profileMenu_.addItem(juce::String(core::profiles[i].name.data()).replaceCharacter('_',' '),static_cast<int>(i)+1);
  profileMenu_.setSelectedId(static_cast<int>(processor_.pipeline().selectedProfile())+1,juce::dontSendNotification);
  profileMenu_.onChange=[this]{processor_.setDspProfile(static_cast<core::ProfileId>(profileMenu_.getSelectedId()-1));};
  addAndMakeVisible(officialReferenceMenu_);
  officialReferenceMenu_.setTooltip("Official reference pool. Only compatible DSP reference data can affect the reference target.");
  officialReferenceMenu_.addItem("No Official Reference", 1);
  officialReferenceMenu_.setSelectedId(1, juce::dontSendNotification);
  officialReferenceMenu_.onChange=[this]{selectOfficialReference(officialReferenceMenu_.getSelectedId()-2);};
  addAndMakeVisible(spectrumRangeMenu_);
  spectrumRangeMenu_.setTooltip("Presentation-only spectrum viewport; authoritative FFT values are unchanged.");
  spectrumRangeMenu_.addItem("DISPLAY -120 TO 0 dB",1);
  spectrumRangeMenu_.addItem("DISPLAY -96 TO 0 dB",2);
  spectrumRangeMenu_.addItem("DISPLAY -72 TO 0 dB",3);
  spectrumRangeMenu_.addItem("DISPLAY -48 TO 0 dB",4);
  spectrumRangeMenu_.setSelectedId(static_cast<int>(processor_.pipeline().presentation().spectrumRange)+1,juce::dontSendNotification);
  spectrumRangeMenu_.onChange=[this]{processor_.setSpectrumDisplayRange(static_cast<core::SpectrumDisplayRange>(spectrumRangeMenu_.getSelectedId()-1));repaint();};
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
  addAndMakeVisible(localReferenceButton_);
  localReferenceButton_.addListener(this);

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
  aiModel_.setTextToShowWhenEmpty("aifred:latest, gpt-5.6-luna, or compatible model", Colours::muted);
  addAndMakeVisible(apiEndpoint_);
  addAndMakeVisible(apiKey_);
  addAndMakeVisible(aiModel_);

  providerMenu_.addItem("OpenAI", 1);
  providerMenu_.addItem("OpenAI-compatible", 2);
  providerMenu_.addItem("Ollama / Local", 3);
  appearanceMenu_.addItem("Aurora", 1);
  appearanceMenu_.addItem("Pulse", 2);
  appearanceMenu_.addItem("Spectrum Glow", 3);
  appearanceMenu_.addItem("Minimal", 4);
  appearanceMenu_.addItem("Dark / Static", 5);
  appearanceMenu_.onChange=[this]{pushSettingsToProcessor();repaint();};
  addAndMakeVisible(providerMenu_);
  addAndMakeVisible(appearanceMenu_);

  const auto settings = processor_.getPluginSettings();
  appearanceMenu_.setSelectedId(settings.visualizerId,juce::dontSendNotification);
  providerMenu_.setSelectedId(settings.aiProvider == "ollama" ? 3 : (settings.aiProvider == "compatible" ? 2 : 1));
  apiEndpoint_.setText(settings.apiEndpoint, juce::dontSendNotification);
  apiKey_.setText(settings.apiKey, juce::dontSendNotification);
  aiModel_.setText(settings.aiModel, juce::dontSendNotification);

  setResizable(true, true);
  setResizeLimits(360, 280, 1920, 1780);
  setSize(1360, 820);
  showTutorial_ = !processor_.hasSeenHelp();
  resized();
  processor_.intelligence().pingHealthAsync();
  ReferencePoolClient::instance().refreshAsync();
  startTimerHz(30);
}

AifredAudioProcessorEditor::~AifredAudioProcessorEditor() {
  for (auto* button : {&analyzeButton_, &referenceButton_, &compareButton_, &optionsButton_, &tutorialButton_, &centerModeButton_}) {
    button->removeListener(this);
  }
  for (auto* button : {&askAiButton_, &saveApiButton_, &chatFileButton_, &compareFileButton_}) {
    button->removeListener(this);
  }
  localReferenceButton_.removeListener(this);
  setLookAndFeel(nullptr);
}

void AifredAudioProcessorEditor::buttonClicked(juce::Button* button) {
  if(showTutorial_)
  {
    showTutorial_=false;processor_.markHelpSeen();resized();repaint();return;
  }
  if(showOptions_&&button!=&saveApiButton_)
  {
    showOptions_=false;resized();repaint();return;
  }
  if (button == &analyzeButton_) processor_.setMode(AnalysisMode::Analyze);
  if (button == &referenceButton_) {
    processor_.setMode(AnalysisMode::Reference);
    ReferencePoolClient::instance().refreshAsync();
  }
  if (button == &compareButton_) processor_.setMode(AnalysisMode::Compare);
  if (button == &optionsButton_) { showOptions_ = true; showTutorial_ = false; }
  if (button == &centerModeButton_) haloCenterMode_ = (haloCenterMode_ + 1) % 3;
  if (button == &tutorialButton_) {
    showTutorial_ = true;
    showOptions_ = false;
  }
  if (button == &askAiButton_) {
    const auto prompt = chatInput_.getText().trim();
    const auto reference=processor_.referenceTarget();const auto compareObservation=processor_.comparePipeline().observation();
    const auto mode=processor_.getMode()==AnalysisMode::Reference?"reference":processor_.getMode()==AnalysisMode::Compare?"compare":"analyze";

    if (prompt.isEmpty()) {
      chatOutputText_.clear();
    } else {
      if (processor_.intelligence().askAsync(prompt, processor_.pipeline().contextForQuestion(prompt,&reference.distribution,mode,&compareObservation))) {
        chatOutputText_ = "You: " + prompt + "\n\nAIFRED: Reading current mix snapshot...";
      } else {
        chatOutputText_ = "You: " + prompt + "\n\nAIFRED: Chat request already in progress.";
      }
    }
    chatOutput_.setText(chatOutputText_, juce::dontSendNotification);
    chatOutput_.moveCaretToEnd();
  }
  if (button == &saveApiButton_) {
    pushSettingsToProcessor();
    processor_.intelligence().saveSettingsAsync(providerMenu_.getSelectedId() == 3 ? "ollama" : (providerMenu_.getSelectedId() == 2 ? "compatible" : "openai"),
                                                     apiEndpoint_.getText().trim(),
                                                     apiKey_.getText(),
                                                     aiModel_.getText().trim());
    apiStatus_ = apiEndpoint_.getText().trim().isNotEmpty() ? "API route set." : "API route not connected.";
    showOptions_ = false;
  }
  if (button == &chatFileButton_ || button == &compareFileButton_) {
    fileChooser_ = std::make_unique<juce::FileChooser>("Select audio file", juce::File{}, "*.wav;*.aif;*.aiff;*.mp3;*.flac");
    fileChooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
      [this, button](const juce::FileChooser& chooser) {
        const auto file = chooser.getResult();
        if (!file.existsAsFile()) return;
        if (button == &chatFileButton_) chatFileStatus_ = "Selected file not analyzed; chat uses live mix snapshot.";
        if (button == &compareFileButton_) compareStatus_ = "Compare file selected; use Mix B sidechain for live comparison.";
        repaint();
      });
  }
  if (button == &localReferenceButton_) {
    fileChooser_ = std::make_unique<juce::FileChooser>("Select local reference", juce::File{}, "*.wav;*.aif;*.aiff;*.mp3;*.flac");
    fileChooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
      [this](const juce::FileChooser& chooser) {
        const auto file = chooser.getResult();
        if (!file.existsAsFile()) return;
        if (analyzeReferenceFile(file)) {
          referenceFileName_ = file.getFileName();
          selectedOfficialReferenceId_.clear();
          officialReferenceMenu_.setSelectedId(1, juce::dontSendNotification);
        }
        repaint();
      });
  }
  pushSettingsToProcessor();
  resized();
  repaint();
}

void AifredAudioProcessorEditor::timerCallback() {
  state_ = processor_.getView();
  compareState_ = processor_.getCompareView();
  memoryIndex_.observe(state_,processor_.getMode(),juce::String(state_.reference.label),processor_.intelligence().isAvailable());
  if (isShowing()&&appearanceMenu_.getSelectedId()!=5) ambientPhase_ = std::fmod(ambientPhase_ + 0.018f, juce::MathConstants<float>::twoPi);

  const auto officialPool = ReferencePoolClient::instance().state();
  if (officialPool.revision != officialReferencePoolRevision_) {
    officialReferencePoolRevision_ = officialPool.revision;
    officialReferencePoolStatus_ = juce::String(officialPool.message);
    updateOfficialReferenceMenu(officialPool);
  }

  if (juce::Time::getMillisecondCounter() % 3000 < 40) {
    processor_.intelligence().pingHealthAsync();
  }
  const auto engineResponse = processor_.intelligence().lastResponse();
  if (engineResponse.isNotEmpty() && !processor_.intelligence().hasPendingChat()) {
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
  settings.visualizerId = appearanceMenu_.getSelectedId();
  settings.helpSeen = processor_.hasSeenHelp();
  settings.aiProvider = providerMenu_.getSelectedId() == 3 ? "ollama" : (providerMenu_.getSelectedId() == 2 ? "compatible" : "openai");
  settings.apiEndpoint = apiEndpoint_.getText().trim();
  settings.apiKey = apiKey_.getText();
  settings.aiModel = aiModel_.getText().trim();
  processor_.setPluginSettings(settings);
}

bool AifredAudioProcessorEditor::analyzeReferenceFile(const juce::File& file) {
  juce::AudioFormatManager formats;
  formats.registerBasicFormats();
  std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
  if (reader == nullptr) {
    localReferenceValid_ = false;
    localReferenceTarget_ = {};
    referenceFileName_.clear();
    referenceStatus_ = "Reference file could not be read.";
    processor_.clearReferenceTarget();
    return false;
  }

  auto referenceAnalysis=std::make_unique<core::Engine>();auto hunter=std::make_unique<core::BufferHunter>();
  auto snapshot=std::make_unique<core::EngineSnapshot>();referenceAnalysis->requestProfile(processor_.pipeline().selectedProfile());
  referenceAnalysis->prepare(reader->sampleRate,2);
  constexpr int blockSize = 4096;
  juce::AudioBuffer<float> block(2, blockSize);
  for (juce::int64 position = 0; position < reader->lengthInSamples; position += blockSize) {
    const auto samplesThisBlock = static_cast<int>(std::min<juce::int64>(blockSize, reader->lengthInSamples - position));
    block.clear();
    reader->read(&block, 0, samplesThisBlock, position, true, true);
    if (samplesThisBlock < blockSize) {
      juce::AudioBuffer<float> trimmed(block.getArrayOfWritePointers(), 2, samplesThisBlock);
      referenceAnalysis->process(trimmed.getArrayOfReadPointers(),2,trimmed.getNumSamples());
    } else {
      referenceAnalysis->process(block.getArrayOfReadPointers(),2,block.getNumSamples());
    }
    while(referenceAnalysis->pop(*snapshot))hunter->consume(*snapshot,static_cast<double>(snapshot->sampleEnd)/reader->sampleRate);
  }

  while(referenceAnalysis->pop(*snapshot))hunter->consume(*snapshot,static_cast<double>(snapshot->sampleEnd)/reader->sampleRate);
  const auto observed=hunter->snapshot(static_cast<double>(snapshot->sampleEnd)/reader->sampleRate);
  const auto analyzed=makeBetaView(*snapshot,observed);
  if (!analyzed.hasSignal || !analyzed.valuesValid) {
    localReferenceValid_ = false;
    localReferenceTarget_ = {};
    referenceFileName_.clear();
    referenceStatus_ = "Reference file had no usable signal.";
    processor_.clearReferenceTarget();
    return false;
  }

  ReferenceTarget target;target.distribution.available=observed.sufficient;target.distribution.profileId=observed.profileId;target.distribution.profileVersion=observed.profileVersion;
  target.distribution.sampleRate=observed.sampleRate;target.distribution.metrics=observed.metrics;target.distribution.bands=observed.bands;target.distribution.id=file.getFileName().toStdString();
  target.rmsScale = analyzed.metrics.rmsScale;
  target.widthScale = analyzed.metrics.widthScale;
  target.crestScale = analyzed.metrics.crestScale;
  target.loudnessDb = analyzed.metrics.integratedLufs;
  target.crestDb = analyzed.metrics.crestDb;
  target.poolSize = 1;
  target.label = file.getFileName().toStdString();
  localReferenceTarget_ = target;
  localReferenceValid_ = true;
  processor_.setReferenceTarget(target);
  referenceStatus_ = "Selected local: " + file.getFileName();
  return true;
}

void AifredAudioProcessorEditor::clearLocalReference() {
  localReferenceTarget_ = {};
  localReferenceValid_ = false;
  referenceFileName_.clear();
  processor_.clearReferenceTarget();
  referenceStatus_ = "No compatible reference selected.";
}

  void AifredAudioProcessorEditor::selectOfficialReference(int index) {
    const auto pool = ReferencePoolClient::instance().state();
    if (index < 0 || index >= static_cast<int>(pool.entries.size())) {
      selectedOfficialReferenceId_.clear();
      if(localReferenceValid_) { processor_.setReferenceTarget(localReferenceTarget_); referenceStatus_="Selected local: "+referenceFileName_; }
      else clearLocalReference();
      repaint();
      return;
    }
    selectedOfficialReferenceId_ = pool.entries[static_cast<std::size_t>(index)].id;
    ReferenceTarget target;
    target.distribution.id=selectedOfficialReferenceId_;
    target.distribution.available=false;
    target.poolSize=static_cast<int>(pool.entries.size());
    target.label="Official / "+pool.entries[static_cast<std::size_t>(index)].name;
    processor_.setReferenceTarget(target);
    referenceStatus_="Selected: "+juce::String(target.label)+" (metadata only; load local audio for measured deltas)";
    repaint();
  }

  void AifredAudioProcessorEditor::updateOfficialReferenceMenu(const ReferencePoolSnapshot& pool) {
    const auto selected = selectedOfficialReferenceId_;
    officialReferenceMenu_.clear(juce::dontSendNotification);
    officialReferenceMenu_.addItem("No Official Reference", 1);
    int selectedId = 1;
    for (std::size_t i = 0; i < pool.entries.size(); ++i) {
      const auto& entry = pool.entries[i];
      officialReferenceMenu_.addItem(juce::String(entry.name), static_cast<int>(i) + 2);
      if (entry.id == selected) selectedId = static_cast<int>(i) + 2;
    }
    officialReferenceMenu_.setSelectedId(selectedId, juce::dontSendNotification);
    if (selectedId == 1 && !selected.empty()) {
      selectedOfficialReferenceId_.clear();
      if(localReferenceValid_) {processor_.setReferenceTarget(localReferenceTarget_);referenceStatus_="Selected local: "+referenceFileName_;}
      else clearLocalReference();
    }
  }

void AifredAudioProcessorEditor::paint(juce::Graphics& g) {
  auto physicalBounds = getLocalBounds();
  const auto mode = processor_.getMode();
  auto accent = accentForMode(mode);
  if (mode == AnalysisMode::Reference) accent = appearanceColour(appearanceMenu_.getSelectedId());

  g.fillAll(juce::Colour(0xff02060b));
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff07111d), 0, 0,
                                         juce::Colour(0xff02060b), static_cast<float>(getWidth()), static_cast<float>(getHeight()), false));
  g.fillRect(physicalBounds);
  g.saveState();
  g.addTransform(juce::AffineTransform::scale(designScale_).translated(designOrigin_.x,designOrigin_.y));
  auto bounds=juce::Rectangle<int>(0,0,kDesignWidth,kDesignHeight);
  if(appearanceMenu_.getSelectedId()!=5)drawAmbientBackground(g,bounds,accent);

  for (int x = 0; x < kDesignWidth; x += 44) {
    g.setColour(accent.withAlpha(0.035f));
    g.drawVerticalLine(x, 0.0f, static_cast<float>(kDesignHeight));
  }
  for (int y = 0; y < kDesignHeight; y += 44) {
    g.setColour(Colours::green.withAlpha(0.022f));
    g.drawHorizontalLine(y, 0.0f, static_cast<float>(kDesignWidth));
  }

  drawHeader(g, bounds.removeFromTop(kHeaderHeight).reduced(18,12));

  auto main = bounds.reduced(18,10);
  if (mode == AnalysisMode::Compare) {
    drawCompare(g, main);
  } else {
    auto left = main.removeFromLeft(292);
    main.removeFromLeft(12);
    auto right = main.removeFromRight(450);
    main.removeFromRight(12);
    auto center = main;

    drawMixSignature(g,left.removeFromTop(184),state_);left.removeFromTop(10);
    drawSpectrumMeter(g,left.removeFromTop(130),state_);left.removeFromTop(10);
    drawCorrelationMeter(g,left.removeFromTop(70),state_);left.removeFromTop(10);
    drawCandles(g,left,state_);
    drawHalo(g,center,state_,mode==AnalysisMode::Reference?"REFERENCE HALO":"ANALYZE HALO",mode==AnalysisMode::Reference);

    if (mode == AnalysisMode::Reference) {
      drawReferencePanel(g,right.removeFromTop(350),state_);right.removeFromTop(10);
    } else {
      drawDomainCard(g,right.removeFromTop(kRightCardHeight),"RMS",Domain::Tone,state_);right.removeFromTop(8);
      drawDomainCard(g,right.removeFromTop(kRightCardHeight),"STEREO",Domain::Stereo,state_);right.removeFromTop(8);
      drawDomainCard(g,right.removeFromTop(kRightCardHeight),"CREST",Domain::Dynamics,state_);right.removeFromTop(8);
      drawDomainCard(g,right.removeFromTop(kRightCardHeight),"LOUDNESS",Domain::Loudness,state_);right.removeFromTop(8);
      drawBrainPanel(g,right.removeFromTop(150));right.removeFromTop(10);
    }
    drawChatPanel(g,right);
  }

  if (showOptions_) {
    g.setColour(juce::Colours::black.withAlpha(.66f));g.fillRect(juce::Rectangle<float>(0,0,kDesignWidth,kDesignHeight));
    auto panel = modalBounds(650.0f,520.0f);
    drawPanel(g, panel, 8.0f);
    auto inner = panel.toNearestInt().reduced(22);
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.setColour(Colours::ink);
    g.drawText("OPTIONS", inner.removeFromTop(34), juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(14.0f));
    g.setColour(Colours::muted);
    const auto& activeProfile=core::profile(processor_.pipeline().selectedProfile());
    g.drawFittedText("DSP profile: "+juce::String(activeProfile.name.data()).replaceCharacter('_',' ')+"\nAppearance: "+juce::String(appearanceName(appearanceMenu_.getSelectedId()))+"\n"+apiStatus_,inner.removeFromTop(82),juce::Justification::topLeft,4);
  }

  if (showTutorial_) {
    g.setColour(juce::Colours::black.withAlpha(.66f));g.fillRect(juce::Rectangle<float>(0,0,kDesignWidth,kDesignHeight));
    auto panel = modalBounds(760.0f,590.0f);
    drawPanel(g, panel, 8.0f);
    auto inner = panel.toNearestInt().reduced(24);
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.setColour(Colours::ink);
    g.drawText("HELP", inner.removeFromTop(36), juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(13.0f));
    g.setColour(Colours::muted);
    g.drawFittedText("ANALYZE — measures the live Mix A input with the selected DSP profile.\n\nREFERENCE — keeps the live Halo visible and compares it with a compatible analyzed local reference. Official Pool entries identify released references; measured deltas require compatible DSP data.\n\nCOMPARE — measures Mix A and Mix B side by side. DELTA is always A - B.\n\nFL STUDIO ROUTING — insert AIFRED on the master or analysis bus. Route the main mix to Mix A. Enable the wrapper sidechain input, then route the comparison track to Mix B without sending it to the master twice.\n\nINTELLIGENCE — DSP meters remain fully functional without AI. Chat additionally needs AifredIntelligenceHost. Run scripts/windows/start-host.ps1, or launch AifredIntelligenceHost.exe from the installed Runtime folder. For Ollama, start `ollama serve`, install/select a model, use http://127.0.0.1:11434, and leave the key blank. For OpenAI or a compatible provider, verify endpoint, model, key, and network access in OPTIONS.\n\nHEALTH — if chat is unavailable, check the host is running on Beta port 8787, then check Ollama/provider health. A host/provider failure never invalidates the DSP measurements.",inner,juce::Justification::topLeft,28);
  }
  g.restoreState();
}

void AifredAudioProcessorEditor::resized() {
  const auto canvas=responsiveCanvas(static_cast<float>(getWidth()),static_cast<float>(getHeight()));
  designScale_=canvas.scale;designOrigin_={canvas.x,canvas.y};
  const auto set=[this](juce::Component& component,juce::Rectangle<int> design){component.setBounds(scaledBounds(design));};
  const auto scaledFont=juce::jmax(4.0f,14.0f*designScale_);
  chatInput_.applyFontToAllText(juce::Font(juce::FontOptions(scaledFont)));
  chatOutput_.applyFontToAllText(juce::Font(juce::FontOptions(scaledFont)));
  apiEndpoint_.applyFontToAllText(juce::Font(juce::FontOptions(scaledFont)));
  apiKey_.applyFontToAllText(juce::Font(juce::FontOptions(scaledFont)));
  aiModel_.applyFontToAllText(juce::Font(juce::FontOptions(scaledFont)));

  if(!showOptions_&&!showTutorial_){auto nav=juce::Rectangle<int>(470,20,870,36);const int gap=10,buttonWidth=136;
    set(centerModeButton_,nav.removeFromLeft(buttonWidth));nav.removeFromLeft(gap);
    set(analyzeButton_,nav.removeFromLeft(buttonWidth));nav.removeFromLeft(gap);
    set(referenceButton_,nav.removeFromLeft(buttonWidth));nav.removeFromLeft(gap);
    set(compareButton_,nav.removeFromLeft(buttonWidth));nav.removeFromLeft(gap);
    set(optionsButton_,nav.removeFromLeft(buttonWidth));nav.removeFromLeft(gap);
    set(tutorialButton_,nav.removeFromLeft(buttonWidth));}
  else for(auto* button:{&centerModeButton_,&analyzeButton_,&referenceButton_,&compareButton_,&optionsButton_,&tutorialButton_})button->setBounds({});

  for(auto* component:{static_cast<juce::Component*>(&chatInput_),static_cast<juce::Component*>(&chatOutput_),static_cast<juce::Component*>(&askAiButton_),static_cast<juce::Component*>(&chatFileButton_),static_cast<juce::Component*>(&compareFileButton_),static_cast<juce::Component*>(&officialReferenceMenu_),static_cast<juce::Component*>(&localReferenceButton_),static_cast<juce::Component*>(&profileMenu_),static_cast<juce::Component*>(&spectrumRangeMenu_),static_cast<juce::Component*>(&providerMenu_),static_cast<juce::Component*>(&appearanceMenu_),static_cast<juce::Component*>(&apiEndpoint_),static_cast<juce::Component*>(&apiKey_),static_cast<juce::Component*>(&aiModel_),static_cast<juce::Component*>(&saveApiButton_)})component->setBounds({});

  const auto mode=processor_.getMode();
  if(!showOptions_&&!showTutorial_&&mode!=AnalysisMode::Compare)
  {
    int chatY=mode==AnalysisMode::Reference?446:494;
    if(mode==AnalysisMode::Reference){set(officialReferenceMenu_,{910,128,414,30});set(localReferenceButton_,{910,168,414,32});}
    auto chat=juce::Rectangle<int>(908,chatY+46,418,820-chatY-72);
    set(chatInput_,chat.removeFromTop(62));auto buttons=chat.removeFromTop(40);
    set(chatFileButton_,buttons.removeFromLeft(202).reduced(0,4));set(askAiButton_,buttons.reduced(8,4));
    chat.removeFromBottom(42);set(chatOutput_,chat.reduced(0,4));
  }
  else if(!showOptions_&&!showTutorial_)
  {
    auto chat=juce::Rectangle<int>(928,562,398,222);
    set(chatInput_,chat.removeFromTop(56));auto buttons=chat.removeFromTop(38);
    set(chatFileButton_,buttons.removeFromLeft(190).reduced(0,4));set(askAiButton_,buttons.reduced(8,4));
    chat.removeFromBottom(42);set(chatOutput_,chat.reduced(0,4));
    set(compareFileButton_,{476,758,408,34});
  }

  if(showOptions_)
  {
    auto panel=modalBounds(650,520).toNearestInt().reduced(22);panel.removeFromTop(112);
    set(profileMenu_,panel.removeFromTop(34));panel.removeFromTop(8);
    set(spectrumRangeMenu_,panel.removeFromTop(34));panel.removeFromTop(8);
    set(appearanceMenu_,panel.removeFromTop(34));panel.removeFromTop(8);
    set(providerMenu_,panel.removeFromTop(34));panel.removeFromTop(8);
    set(apiEndpoint_,panel.removeFromTop(34));panel.removeFromTop(8);
    set(apiKey_,panel.removeFromTop(34));panel.removeFromTop(8);
    set(aiModel_,panel.removeFromTop(34));panel.removeFromTop(8);
    set(saveApiButton_,panel.removeFromTop(36));
  }
}

juce::Rectangle<float> AifredAudioProcessorEditor::modalBounds(float width,float height) const
{
  return juce::Rectangle<float>(0,0,width,height).withCentre({kDesignWidth*0.5f,kDesignHeight*0.5f});
}

juce::Rectangle<int> AifredAudioProcessorEditor::scaledBounds(juce::Rectangle<int> design) const
{
  return juce::Rectangle<float>(designOrigin_.x+design.getX()*designScale_,designOrigin_.y+design.getY()*designScale_,
                                design.getWidth()*designScale_,design.getHeight()*designScale_).toNearestInt();
}

void AifredAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
  const auto designPoint=(event.position-designOrigin_)/designScale_;
  const auto helpPanel=modalBounds(760,590);const auto optionsPanel=modalBounds(650,520);
  if(showTutorial_&&modalShouldDismiss(helpPanel.getX(),helpPanel.getY(),helpPanel.getWidth(),helpPanel.getHeight(),designPoint.x,designPoint.y))
  {
    showTutorial_=false;processor_.markHelpSeen();resized();repaint();
  }
  else if(showOptions_&&modalShouldDismiss(optionsPanel.getX(),optionsPanel.getY(),optionsPanel.getWidth(),optionsPanel.getHeight(),designPoint.x,designPoint.y))
  {
    showOptions_=false;resized();repaint();
  }
}

bool AifredAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
  if(key==juce::KeyPress::escapeKey&&(showTutorial_||showOptions_))
  {
    if(showTutorial_)processor_.markHelpSeen();
    showTutorial_=false;showOptions_=false;resized();repaint();return true;
  }
  return AudioProcessorEditor::keyPressed(key);
}

void AifredAudioProcessorEditor::drawHeader(juce::Graphics& g, juce::Rectangle<int> bounds) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto logo = bounds.removeFromLeft(64).reduced(10);
  if (mascot_.isValid()) {
    g.drawImageWithin(mascot_, logo.getX(), logo.getY(), logo.getWidth(), logo.getHeight(),
                      juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
  }

  auto text = bounds.removeFromLeft(470);
  g.setFont(juce::FontOptions(26.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("AIFRED VST", text.removeFromTop(34), juce::Justification::centredLeft);
  g.setFont(juce::FontOptions(13.0f));
  g.setColour(Colours::green);
  g.drawFittedText(juce::String("BETA v" AIFRED_VERSION_STRING "  " AIFRED_BUILD_ID " / ")+appearanceName(appearanceMenu_.getSelectedId())+" / "+referenceStatus_,text.removeFromTop(24).withWidth(360),juce::Justification::centredLeft,1);
}

void AifredAudioProcessorEditor::drawAmbientBackground(juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour accent) {
  const auto width = static_cast<float>(bounds.getWidth());
  const auto height = static_cast<float>(bounds.getHeight());
  const auto phase = ambientPhase_;
  const std::array<juce::Point<float>, 3> centres {
    juce::Point<float>(width * (0.18f + 0.05f * std::sin(phase * 0.71f)), height * (0.30f + 0.06f * std::cos(phase * 0.53f))),
    juce::Point<float>(width * (0.72f + 0.07f * std::cos(phase * 0.43f)), height * (0.20f + 0.08f * std::sin(phase * 0.61f))),
    juce::Point<float>(width * (0.55f + 0.08f * std::sin(phase * 0.37f)), height * (0.82f + 0.04f * std::cos(phase * 0.83f)))
  };
  const std::array<juce::Colour, 3> colours {accent, Colours::violet, Colours::green};
  const std::array<float, 3> sizes {260.0f, 220.0f, 180.0f};
  for (std::size_t i = 0; i < centres.size(); ++i) {
    const auto size = sizes[i];
    g.setColour(colours[i].withAlpha(0.035f));
    g.fillEllipse(centres[i].x - size * 0.5f, centres[i].y - size * 0.5f, size, size);
  }
  for (int i = 0; i < 12; ++i) {
    const auto t = phase * (0.17f + static_cast<float>(i % 3) * 0.03f) + static_cast<float>(i) * 1.7f;
    const auto x = width * (0.08f + 0.84f * (0.5f + 0.5f * std::sin(t * 0.73f + i)));
    const auto y = height * (0.10f + 0.82f * (0.5f + 0.5f * std::cos(t * 0.57f + i * 0.4f)));
    g.setColour(colours[static_cast<std::size_t>(i) % colours.size()].withAlpha(0.055f));
    g.fillEllipse(x, y, 2.0f + static_cast<float>(i % 2), 2.0f + static_cast<float>(i % 2));
  }
}

void AifredAudioProcessorEditor::drawHalo(juce::Graphics& g, juce::Rectangle<int> bounds, const BetaView& state, const char* title, bool referenceOverlay) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto area = bounds.reduced(28).toFloat();
  auto centre = area.getCentre();
  const auto radius = std::min(area.getWidth(), area.getHeight()) * 0.275f;
  const auto hasValidLiveData = state.hasSignal && state.valuesValid;
  auto accent = referenceOverlay ? appearanceColour(appearanceMenu_.getSelectedId()) : accentForMode(processor_.getMode());
  const auto dynamics01 = hasValidLiveData ? state.metrics.crestScale : 0.0f;
  const auto rmsScale = hasValidLiveData ? clamp01(state.metrics.rmsScale) : 0.0f;
  const auto truePeak01=hasValidLiveData?truePeakPresentation(state.metrics.truePeakDb):0.0f;
  const auto widthScale=hasValidLiveData?stereoSpreadPresentation(state.metrics.correlation):0.0f;
  const auto canonicalLabel = [&](core::MetricId id) {
    const auto& detail = state.metricDetails[core::index(id)];
    if (!detail.valid) return juce::String(detail.displayName.data()) + " --";
    return juce::String(detail.displayName.data()) + " " + juce::String(detail.displayedValue, detail.id == core::MetricId::correlation ? 2 : static_cast<int>(core::metricDefinitions[core::index(id)].decimals)) + " " + juce::String(detail.unit.data());
  };
  const std::array<float, 4> values { 
    dynamics01,
    rmsScale,
    truePeak01,
    widthScale
  };
  const auto dynamicPulse = clamp01(0.28f * values[2] + 0.26f * values[3] + 0.20f * values[0] + 0.18f * values[1]);
  const auto pulse = hasValidLiveData ? 0.82f + dynamicPulse * 0.14f : 1.0f;

  g.setColour(accent.withAlpha(referenceOverlay ? 0.18f : 0.10f));
  g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
  g.setColour(accent.withAlpha(0.10f));
  g.drawEllipse(centre.x - radius * pulse, centre.y - radius * pulse, radius * 2.0f * pulse, radius * 2.0f * pulse, 3.0f + 3.0f * state.metrics.widthScale);

  const std::array<juce::Colour, 4> colours {Colours::cyan, Colours::green, Colours::yellow, Colours::violet};
  const std::array<juce::String, 4> labels {
    canonicalLabel(core::MetricId::crest),
    canonicalLabel(core::MetricId::rms),
    canonicalLabel(core::MetricId::truePeak),
    state.metricDetails[core::index(core::MetricId::width)].valid
      ? "Stereo  W "+juce::String(state.metrics.stereoWidth*100.0f,1)+"%  C "+juce::String(state.metrics.correlation,2)
      : "Stereo --"
  };
  for (int i = 0; i < 4; ++i) {
    const auto lane = static_cast<float>(i);
    const float start = -150.0f + lane * 90.0f;
    juce::Path bg;
    bg.addCentredArc(centre.x, centre.y, radius + 18.0f + lane * 8.0f, radius + 18.0f + lane * 8.0f, 0.0f,
                     juce::degreesToRadians(start), juce::degreesToRadians(start + 72.0f), true);
    g.setColour(Colours::line.withAlpha(0.45f));
    g.strokePath(bg, juce::PathStrokeType(7.0f));
    const auto value =
        clamp01(values[static_cast<size_t>(i)]);
    const auto arcStart=start;
    juce::Path arc;
    arc.addCentredArc(centre.x, centre.y, radius + 18.0f + lane * 8.0f, radius + 18.0f + lane * 8.0f, 0.0f,
                      juce::degreesToRadians(arcStart),juce::degreesToRadians(start+72.0f*value),true);
    g.setColour(colours[static_cast<size_t>(i)].withAlpha(0.95f));
    g.strokePath(arc, juce::PathStrokeType(7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const auto labelAngle = juce::degreesToRadians(start + 36.0f);
    const auto labelRadius = radius + 58.0f;
    const auto labelCentre = juce::Point<float>(centre.x + std::cos(labelAngle) * labelRadius,
                                                centre.y + std::sin(labelAngle) * labelRadius);
    g.setFont(uiFont(10.5f, 11.0f, juce::Font::bold));
    g.setColour(Colours::ink);
    g.drawText(labels[static_cast<size_t>(i)], juce::Rectangle<float>(labelCentre.x - 65.0f, labelCentre.y - 10.0f, 130.0f, 20.0f).toNearestInt(), juce::Justification::centred);
  }

  for (int lane = 0; lane < 4; ++lane) {
    const auto start = -150.0f + static_cast<float>(lane) * 90.0f;
    for (int tick = 0; tick <= 4; ++tick) {
      const auto angle = juce::degreesToRadians(start + 72.0f * static_cast<float>(tick) / 4.0f);
      const auto major = tick == 0 || tick == 2 || tick == 4;
      const auto inner = radius + (major ? 78.0f : 83.0f);
      const auto outer = radius + 92.0f;
      g.setColour((major ? Colours::ink : Colours::muted).withAlpha(major ? 0.62f : 0.34f));
      g.drawLine(centre.x + std::cos(angle) * inner, centre.y + std::sin(angle) * inner,
                 centre.x + std::cos(angle) * outer, centre.y + std::sin(angle) * outer,
                 major ? 1.5f : 1.0f);
    }
  }

  const auto rmsFloor=static_cast<float>(core::spectrumFloorDb(state.presentation.spectrumRange));
  const std::array<std::array<juce::String,3>,4> scaleLabels {{
    {juce::String("0 dB"),juce::String("12 dB"),juce::String("24 dB")},
    {juce::String(juce::roundToInt(rmsFloor))+" dBFS",juce::String(juce::roundToInt(rmsFloor*.5f))+" dBFS",juce::String("0 dBFS")},
    {juce::String("-24 dBTP"),juce::String("-12 dBTP"),juce::String("0 dBTP")},
    {juce::String("+1 mono"),juce::String("0 spread"),juce::String("-1 phase")}
  }};
  for(int lane=0;lane<4;++lane)for(int tick:{0,2,4}) {
    const auto degrees=-150.0f+static_cast<float>(lane)*90.0f+72.0f*static_cast<float>(tick)/4.0f;
    const auto angle=juce::degreesToRadians(degrees);const auto labelRadius=radius+102.0f;
    const auto p=juce::Point<float>(centre.x+std::cos(angle)*labelRadius,centre.y+std::sin(angle)*labelRadius);
    g.setFont(uiFont(8.2f,8.2f,juce::Font::bold));g.setColour(colours[static_cast<std::size_t>(lane)].withAlpha(.88f));
    g.drawFittedText(scaleLabels[static_cast<std::size_t>(lane)][static_cast<std::size_t>(tick/2)],
                     juce::Rectangle<float>(p.x-43.0f,p.y-7.0f,86.0f,15.0f).toNearestInt(),juce::Justification::centred,1);
  }

  if (referenceOverlay) {
    const std::array<juce::Colour, 6> ringColours {Colours::green, Colours::cyan, Colours::violet, Colours::yellow, Colours::red, juce::Colour(0xfff7f2e7)};
    for (int ring = 0; ring < 6; ++ring) {
      const auto scale = 0.68f + static_cast<float>(ring) * 0.075f;
      g.setColour(ringColours[static_cast<size_t>(ring)].withAlpha(ring == 0 ? 0.28f : 0.18f));
      g.drawEllipse(centre.x - radius * scale, centre.y - radius * scale, radius * scale * 2.0f, radius * scale * 2.0f, ring == 0 ? 2.3f : 1.4f);
    }
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    const auto referenceText = state.hasReference
      ? juce::String("REFERENCE / ") + juce::String(state.reference.label)
      : "No analyzed reference target";
    g.drawText(referenceText, juce::Rectangle<float>(centre.x - 150.0f, centre.y + radius * 0.72f, 300.0f, 22.0f).toNearestInt(), juce::Justification::centred);
  }

  drawHaloSpectrometer(g, juce::Rectangle<float>(centre.x - radius * 0.74f, centre.y - radius * 0.26f, radius * 1.48f, radius * 0.52f), state);
  g.setFont(uiFont(14.0f, 13.0f, juce::Font::bold));
  g.setColour(hasValidLiveData ? Colours::cyan : Colours::muted);
  g.drawText(title, juce::Rectangle<float>(centre.x - 130.0f, centre.y - radius * 0.58f, 260.0f, 22.0f).toNearestInt(), juce::Justification::centred);
}

void AifredAudioProcessorEditor::drawHaloSpectrometer(juce::Graphics& g, juce::Rectangle<float> bounds, const BetaView& state) {
  g.setColour(juce::Colour(0xff02060b).withAlpha(0.82f));
  g.fillRoundedRectangle(bounds, 8.0f);
  const auto appearance=appearanceMenu_.getSelectedId();
  if(appearance!=5)
  {
    g.saveState();g.reduceClipRegion(bounds.toNearestInt());
    const auto activity=state.hasSignal&&state.valuesValid?truePeakPresentation(state.metrics.truePeakDb):0.0f;
    const auto phase=appearance==4?0.0f:ambientPhase_*(appearance==2?1.7f:.55f);
    for(int layer=0;layer<(appearance==4?1:4);++layer){const auto y=bounds.getCentreY()+std::sin(phase+layer*1.4f)*bounds.getHeight()*(.08f+.025f*layer);
      const auto height=bounds.getHeight()*(.34f+.08f*layer+.12f*activity);const auto x=bounds.getX()+std::sin(phase*.37f+layer)*bounds.getWidth()*.08f;
      const auto colour=layer%2?Colours::violet:appearanceColour(appearance);g.setColour(colour.withAlpha(.035f+.025f*activity));g.fillEllipse(x,y-height*.5f,bounds.getWidth()*1.06f,height);}
    if(state.hasSignal&&appearance==3){g.setColour(Colours::cyan.withAlpha(.08f+.08f*activity));g.drawRoundedRectangle(bounds.reduced(4),8,1.5f+activity*2);}
    g.restoreState();
  }
  g.setColour(Colours::line.withAlpha(0.72f));
  g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

  if (!state.hasSignal || !state.valuesValid) {
    g.setFont(uiFont(16.0f, 14.0f, juce::Font::bold));
    g.setColour(Colours::ink);
    g.drawText("No Signal", bounds.toNearestInt(), juce::Justification::centred);
    return;
  }

  auto plot = bounds.reduced(10.0f, 8.0f);
  const auto spectrumFloor=static_cast<float>(core::spectrumFloorDb(state.presentation.spectrumRange));
  if (haloCenterMode_ == 0 || haloCenterMode_ == 2) {
    juce::Path spectrum;bool started=false;
    for(std::size_t i=1;i<state.binCount;++i){const double hz=static_cast<double>(i)*state.binWidthHz;if(hz<20||hz>20000)continue;
      const float x=plot.getX()+plot.getWidth()*static_cast<float>(std::log(hz/20)/std::log(1000.0));
      const float db=state.spectrumPower[i]>0?static_cast<float>(10*std::log10(state.spectrumPower[i])):-120;
      const float y=plot.getBottom()-plot.getHeight()*clamp01((db-spectrumFloor)/-spectrumFloor);
      if(!started){spectrum.startNewSubPath(x,y);started=true;}else spectrum.lineTo(x,y);
    }
    g.setColour(Colours::cyan);g.strokePath(spectrum,juce::PathStrokeType(1.6f));
    if(state.presentation.showPeakTrace)
    {
      juce::Path peak;bool peakStarted=false;
      for(std::size_t i=1;i<state.binCount;++i){const double hz=static_cast<double>(i)*state.binWidthHz;if(hz<20||hz>20000)continue;
        const float x=plot.getX()+plot.getWidth()*static_cast<float>(std::log(hz/20)/std::log(1000.0));
        const float db=state.peakSpectrumPower[i]>0?static_cast<float>(10*std::log10(state.peakSpectrumPower[i])):spectrumFloor;
        const float y=plot.getBottom()-plot.getHeight()*clamp01((db-spectrumFloor)/-spectrumFloor);
        if(!peakStarted){peak.startNewSubPath(x,y);peakStarted=true;}else peak.lineTo(x,y);
      }
      g.setColour(Colours::green.withAlpha(.75f));g.strokePath(peak,juce::PathStrokeType(1.0f));
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
  g.drawText(haloCenterMode_ == 0 ? "FFT POWER / "+juce::String(juce::roundToInt(spectrumFloor))+" TO 0 dB" : (haloCenterMode_ == 1 ? "WAVEFORM" : "FFT + WAVE"), bounds.toNearestInt().removeFromTop(14), juce::Justification::centred);
}

void AifredAudioProcessorEditor::drawDomainCard(juce::Graphics& g,juce::Rectangle<int> bounds,const char* name,Domain domain,const BetaView& state) {
    drawPanel(g,bounds.toFloat(),8);
    auto inner=bounds.reduced(12,8);
    const auto id=domain==Domain::Tone?core::MetricId::rms:domain==Domain::Stereo?core::MetricId::width:domain==Domain::Dynamics?core::MetricId::crest:core::MetricId::shortTerm;
    const auto detail=state.metricDetails[core::index(id)];
    const auto colour=domain==Domain::Tone?Colours::green:domain==Domain::Stereo?Colours::violet:domain==Domain::Dynamics?Colours::yellow:Colours::cyan;
    g.setColour(colour);g.setFont(juce::FontOptions(13.0f,juce::Font::bold));g.drawText(name,inner.removeFromTop(20),juce::Justification::centredLeft);
    g.setColour(Colours::ink);g.setFont(juce::FontOptions(20.0f));g.drawText(metricText(state,domain),inner.removeFromTop(28),juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(10.0f));g.setColour(Colours::muted);
    const auto status=domain==Domain::Stereo&&detail.valid?"Correlation "+juce::String(state.metrics.correlation,2):juce::String(state.observation.durationSeconds,1)+" s / "+(state.isStale?"retained / stale":state.observation.sufficient?"observed":"collecting");
    g.drawText(status,inner,juce::Justification::centredLeft);
  }

  void AifredAudioProcessorEditor::drawCandles(
    juce::Graphics& g,
    juce::Rectangle<int> bounds,
    const BetaView& state) {
  drawPanel(g, bounds.toFloat(), 8.0f);

  auto inner = bounds.reduced(16);

  g.setFont(juce::FontOptions(17.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("CANDLESTICK METERS",
             inner.removeFromTop(28),
             juce::Justification::centredLeft);

  g.setFont(juce::FontOptions(11.5f));
  g.setColour(Colours::muted);
  g.drawFittedText(
      "Saved sessions, one-minute history, and a rolling 30-second live view.",
      inner.removeFromTop(20),
      juce::Justification::centredLeft,
      1);

  auto topRow = inner.removeFromTop(
      juce::roundToInt(static_cast<float>(inner.getHeight()) * 0.46f));

  auto sessionBounds = topRow.removeFromLeft(topRow.getWidth() / 2);
  auto minuteBounds = topRow;

  drawCandleStrip(
      g,
      sessionBounds.reduced(0, 8),
      state,
      CandleStripType::Session);

  drawCandleStrip(
      g,
      minuteBounds.reduced(8, 8),
      state,
      CandleStripType::Minute);

  drawCandleStrip(
      g,
      inner.reduced(4, 8),
      state,
      CandleStripType::Live);
}

void AifredAudioProcessorEditor::drawCandleStrip(
    juce::Graphics& g,
    juce::Rectangle<int> bounds,
    const BetaView& state,
    CandleStripType type) {
  auto plot = bounds.toFloat();

  g.setColour(Colours::line.withAlpha(0.45f));
  g.drawRoundedRectangle(plot, 6.0f, 1.0f);

  juce::String title;
  juce::String emptyMessage;
  int count = 0;

  switch (type) {
    case CandleStripType::Session:
      title = "CURRENT SESSION";
      emptyMessage = "waiting for session data";
      count = state.metrics.sessionCandleCount;
      break;

    case CandleStripType::Minute:
      title = "10 MINUTE";
      emptyMessage = "no minute candles yet";
      count = state.metrics.minuteCandleCount;
      break;

    case CandleStripType::Live:
      title = "10 LIVE · 3 SEC EACH";
      emptyMessage = "waiting for live candles";
      count = state.metrics.liveCandleCount;
      break;
  }

  g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
  g.setColour(Colours::muted);
  g.drawText(title,
             bounds.removeFromTop(16),
             juce::Justification::centred);

  plot = bounds.reduced(6, 4).toFloat();

  for (int grid = 1; grid <= 3; ++grid) {
    const auto y =
        plot.getY()
        + plot.getHeight() * static_cast<float>(grid) / 4.0f;

    g.setColour(Colours::line.withAlpha(0.24f));
    g.drawHorizontalLine(
        juce::roundToInt(y),
        plot.getX(),
        plot.getRight());
  }

  if (count <= 0) {
    g.setFont(juce::FontOptions(10.0f));
    g.setColour(Colours::muted);
    g.drawText(emptyMessage,
               plot.toNearestInt(),
               juce::Justification::centred);
    return;
  }

  const auto candleWidth = plot.getWidth() / 10.0f;

  for (int i = 0; i < 10; ++i) {
    if (i < 10 - count && count < 10)
      continue;

    float open = 0.0f;
    float high = 0.0f;
    float low = 0.0f;
    float close = 0.0f;

    switch (type) {
      case CandleStripType::Session:
        open = state.metrics.sessionCandleOpen[static_cast<size_t>(i)];
        high = state.metrics.sessionCandleHigh[static_cast<size_t>(i)];
        low = state.metrics.sessionCandleLow[static_cast<size_t>(i)];
        close = state.metrics.sessionCandleClose[static_cast<size_t>(i)];
        break;

      case CandleStripType::Minute:
        open = state.metrics.minuteCandleOpen[static_cast<size_t>(i)];
        high = state.metrics.minuteCandleHigh[static_cast<size_t>(i)];
        low = state.metrics.minuteCandleLow[static_cast<size_t>(i)];
        close = state.metrics.minuteCandleClose[static_cast<size_t>(i)];
        break;

      case CandleStripType::Live:
        open = state.metrics.liveCandleOpen[static_cast<size_t>(i)];
        high = state.metrics.liveCandleHigh[static_cast<size_t>(i)];
        low = state.metrics.liveCandleLow[static_cast<size_t>(i)];
        close = state.metrics.liveCandleClose[static_cast<size_t>(i)];
        break;
    }

    const auto x =
        plot.getX()
        + static_cast<float>(i) * candleWidth
        + candleWidth * 0.5f;

    const auto mapY = [&](float value) {
      // Candle values are the published RMS dBFS observations. This only maps
      // that value to the stable presentation scale used by the RMS card.
      return plot.getBottom() - clamp01((value + 60.0f) / 60.0f) * plot.getHeight();
    };

    const auto openY = mapY(open);
    const auto highY = mapY(high);
    const auto lowY = mapY(low);
    const auto closeY = mapY(close);

    g.setColour(Colours::ink.withAlpha(0.34f));
    g.drawLine(x, highY, x, lowY, 1.4f);

    auto body = juce::Rectangle<float>(
        x - candleWidth * 0.26f,
        std::min(openY, closeY),
        candleWidth * 0.52f,
        std::max(3.0f, std::abs(closeY - openY)));

    const auto candleColour =
        close >= open
            ? Colours::green.withAlpha(0.75f)
            : Colours::red.withAlpha(0.75f);

    g.setColour(candleColour);
    g.fillRoundedRectangle(body, 3.0f);

    if (type == CandleStripType::Live) {
      g.setColour(Colours::cyan.withAlpha(0.50f));
      g.drawRoundedRectangle(body.expanded(0.5f), 3.0f, 1.0f);
    }
  }
}

void AifredAudioProcessorEditor::drawChatPanel(juce::Graphics& g, juce::Rectangle<int> bounds) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto inner = bounds.reduced(16);
  auto header = inner.removeFromTop(30);
  g.setFont(uiFont(18.0f, 18.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("CHAT", header, juce::Justification::centredLeft);

  const auto engineReady = processor_.intelligence().isAvailable();

  auto footer = inner.removeFromBottom(42);
  g.setColour(engineReady ? Colours::green : Colours::yellow);
  g.setFont(uiFont(12.0f, 12.0f, juce::Font::bold));
  g.drawFittedText(processor_.intelligence().statusText(), footer.removeFromTop(18), juce::Justification::bottomLeft, 1);
  g.setColour(Colours::muted);
  g.setFont(uiFont(11.0f, 11.0f));
  const auto fileStatus = chatFileStatus_ != "No chat file selected." ? chatFileStatus_ : "Live mix snapshot only.";
  g.drawFittedText(fileStatus + " Output scrolls below.", footer, juce::Justification::bottomLeft, 2);
}

void AifredAudioProcessorEditor::drawReferencePanel(juce::Graphics& g, juce::Rectangle<int> bounds,const BetaView& state) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto inner = bounds.reduced(16);
  g.setFont(juce::FontOptions(17.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("REFERENCE",inner.removeFromTop(28),juce::Justification::centredLeft);
  g.setFont(juce::FontOptions(11.0f));
  g.setColour(Colours::muted);
  g.drawFittedText("Official Pool identity or one measured local file. No mixer weighting.",inner.removeFromTop(22),juce::Justification::centredLeft,1);
  inner.removeFromTop(76);
  g.setColour(state.hasReference?Colours::green:Colours::yellow);
  g.setFont(juce::FontOptions(11.0f,juce::Font::bold));
  g.drawFittedText(referenceStatus_,inner.removeFromTop(20),juce::Justification::centredLeft,1);
  g.setColour(Colours::muted);g.setFont(juce::FontOptions(10.0f));g.drawFittedText(officialReferencePoolStatus_,inner.removeFromTop(18),juce::Justification::centredLeft,1);
  struct RefRow{const char* label;core::MetricId id;float live;const char* valueUnit;const char* deltaUnit;};
  const std::array<RefRow,5> rows {{{"RMS",core::MetricId::rms,state.metrics.rmsDb," dBFS","dB"},
    {"WIDTH",core::MetricId::width,state.metrics.stereoWidth*100.0f,"%","pp"},
    {"CREST",core::MetricId::crest,state.metrics.crestDb," dB","dB"},
    {"LOUDNESS",core::MetricId::shortTerm,state.metrics.shortTermLufs," LUFS","LU"},
    {"TRUE PEAK",core::MetricId::truePeak,state.metrics.truePeakDb," dBTP","dB"}}};
  g.setFont(juce::FontOptions(10.5f,juce::Font::bold));
  for(const auto& row:rows){auto line=inner.removeFromTop(30);const auto& observed=state.reference.distribution.metrics[core::index(row.id)];
    auto label=line.removeFromLeft(74);g.setColour(Colours::muted);g.drawText(row.label,label,juce::Justification::centredLeft);
    if(state.hasReference&&observed.valid){const auto reference=static_cast<float>(observed.typical);const auto third=line.getWidth()/3;
      g.setColour(Colours::cyan);g.drawFittedText("LIVE "+juce::String(row.live,1)+row.valueUnit,line.removeFromLeft(third),juce::Justification::centredLeft,1);
      g.setColour(Colours::violet);g.drawFittedText("REF "+juce::String(reference,1)+row.valueUnit,line.removeFromLeft(third),juce::Justification::centredLeft,1);
      g.setColour(Colours::ink);g.drawFittedText("D "+signedText(compareDelta(row.live,reference),row.deltaUnit),line,juce::Justification::centredRight,1);
    }else{g.setColour(Colours::muted);g.drawText("Measured reference value unavailable",line,juce::Justification::centredLeft);}}
}

void AifredAudioProcessorEditor::drawCompare(juce::Graphics& g, juce::Rectangle<int> bounds) {
  auto left=bounds.removeFromLeft(430);bounds.removeFromLeft(12);
  auto right=bounds.removeFromRight(430);bounds.removeFromRight(12);auto middle=bounds;
  drawHalo(g,left,state_,"MIX A",false);
  auto rightHalo=right.removeFromTop(420);drawHalo(g,rightHalo,compareState_,"MIX B",false);right.removeFromTop(10);drawChatPanel(g,right);
  drawPanel(g,middle.toFloat(),8.0f);auto inner=middle.reduced(16);
  g.setFont(juce::FontOptions(17.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("A / B MEASURED VALUES",inner.removeFromTop(30),juce::Justification::centredLeft);
  g.setFont(juce::FontOptions(11.0f));
  g.setColour(Colours::muted);
  g.drawFittedText("DELTA = A - B  /  "+compareStatus_,inner.removeFromTop(22),juce::Justification::centredLeft,1);
  struct Row{const char* label;float a,b;const char* valueUnit;const char* deltaUnit;int decimals;};
  const std::array<Row,6> rows {{{"RMS",state_.metrics.rmsDb,compareState_.metrics.rmsDb," dBFS","dB",1},
    {"WIDTH",state_.metrics.stereoWidth*100,compareState_.metrics.stereoWidth*100,"%","pp",1},
    {"CREST",state_.metrics.crestDb,compareState_.metrics.crestDb," dB","dB",1},
    {"LOUDNESS",state_.metrics.shortTermLufs,compareState_.metrics.shortTermLufs," LUFS","LU",1},
    {"TRUE PEAK",state_.metrics.truePeakDb,compareState_.metrics.truePeakDb," dBTP","dB",1},
    {"CORRELATION",state_.metrics.correlation,compareState_.metrics.correlation,"","ratio",2}}};
  for(const auto& item:rows){auto row=inner.removeFromTop(62);g.setColour(Colours::line.withAlpha(.28f));g.drawHorizontalLine(row.getBottom(),static_cast<float>(row.getX()),static_cast<float>(row.getRight()));
    auto label=row.removeFromLeft(92);g.setFont(juce::FontOptions(11.0f,juce::Font::bold));g.setColour(Colours::muted);g.drawText(item.label,label,juce::Justification::centredLeft);
    const auto column=row.getWidth()/3;auto a=row.removeFromLeft(column);auto b=row.removeFromLeft(column);
    g.setColour(Colours::cyan);g.drawText("A  "+juce::String(item.a,item.decimals)+item.valueUnit,a,juce::Justification::centredLeft);
    g.setColour(Colours::green);g.drawText("B  "+juce::String(item.b,item.decimals)+item.valueUnit,b,juce::Justification::centredLeft);
    g.setColour(Colours::ink);g.drawText(signedText(compareDelta(item.a,item.b),item.deltaUnit),row,juce::Justification::centredRight);}
  inner.removeFromTop(8);drawCompareVu(g,inner.removeFromTop(164),state_,compareState_);
}

void AifredAudioProcessorEditor::drawCompareVu(juce::Graphics& g, juce::Rectangle<int> bounds, const BetaView& a, const BetaView& b) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto inner = bounds.reduced(14);
  g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("MIX A  <->  MIX B",inner.removeFromTop(26),juce::Justification::centred);
  const auto similarity=(a.valuesValid&&b.valuesValid)?compareSimilarity(a,b):0.0f;
  auto meter=inner.removeFromTop(46).reduced(10,12).toFloat();g.setColour(Colours::line.withAlpha(.55f));g.fillRoundedRectangle(meter,8);
  if(a.valuesValid&&b.valuesValid){g.setColour(Colours::green);g.fillRoundedRectangle(meter.withWidth(meter.getWidth()*similarity/100.0f),8);}
  g.setFont(juce::FontOptions(25.0f,juce::Font::bold));g.setColour(Colours::ink);
  g.drawText((a.valuesValid&&b.valuesValid)?juce::String(juce::roundToInt(similarity))+"% SIMILAR":"SIMILARITY --",inner.removeFromTop(38),juce::Justification::centred);
  g.setFont(juce::FontOptions(9.5f));g.setColour(Colours::muted);
  g.drawFittedText("Deterministic mean distance across RMS, true peak, crest, loudness, width, and correlation.",inner,juce::Justification::centred,2);
}

void AifredAudioProcessorEditor::drawMixSignature(juce::Graphics& g, juce::Rectangle<int> bounds, const BetaView& state) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto inner = bounds.reduced(16);
  g.setFont(juce::FontOptions(17.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("MIX SIGNATURE", inner.removeFromTop(30), juce::Justification::centredLeft);
  auto graph = inner.reduced(8, 12).toFloat();
  g.setColour(Colours::line.withAlpha(0.6f));
  g.drawRoundedRectangle(graph, 8.0f, 1.0f);
  auto centre = graph.getCentre();
  if (!state.hasSignal || !state.valuesValid) {
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.setColour(Colours::ink);
    g.drawText("live signature unavailable", graph.toNearestInt(), juce::Justification::centred);
    return;
  }
  if (state.hasReference) {
    juce::Path referencePath;
    const std::array<float, 4> referenceValues {
      state.reference.rmsScale,
      state.reference.widthScale,
      state.reference.crestScale,
      clamp01((state.reference.loudnessDb + 24.0f) / 18.0f)
    };
    for (int i = 0; i < 4; ++i) {
      const auto angle = juce::MathConstants<float>::twoPi * static_cast<float>(i) / 4.0f - juce::MathConstants<float>::halfPi;
      const auto radius = std::min(graph.getWidth(), graph.getHeight()) * 0.42f * referenceValues[static_cast<size_t>(i)];
      const auto point = juce::Point<float>(centre.x + std::cos(angle) * radius, centre.y + std::sin(angle) * radius);
      if (i == 0) referencePath.startNewSubPath(point); else referencePath.lineTo(point);
    }
    referencePath.closeSubPath();
    for(int depth=5;depth>=1;--depth){auto shadow=referencePath;shadow.applyTransform(juce::AffineTransform::translation(depth*1.2f,depth*1.5f));g.setColour(Colours::violet.withAlpha(.025f));g.strokePath(shadow,juce::PathStrokeType(1.0f));}
    g.setColour(Colours::violet.withAlpha(0.13f));
    g.fillPath(referencePath);
    g.setColour(Colours::green.withAlpha(0.56f));
    g.strokePath(referencePath, juce::PathStrokeType(1.6f));
  }

  juce::Path path;
  for (int i = 0; i < 4; ++i) {
    const auto rawValue = i == 0 ? state.metrics.rmsScale
      : (i == 1 ? state.metrics.widthScale
        : (i == 2 ? state.metrics.crestScale : state.metrics.loudnessScale));
    const auto angle = juce::MathConstants<float>::twoPi * static_cast<float>(i) / 4.0f - juce::MathConstants<float>::halfPi;
    const auto radius = std::min(graph.getWidth(), graph.getHeight()) * 0.42f * clamp01(rawValue);
    const auto point = juce::Point<float>(centre.x + std::cos(angle) * radius, centre.y + std::sin(angle) * radius);
    if (i == 0) path.startNewSubPath(point); else path.lineTo(point);
    g.setColour(Colours::muted);
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.drawText(metricLabel(i), juce::Rectangle<float>(point.x - 34.0f, point.y - 10.0f, 68.0f, 20.0f).toNearestInt(), juce::Justification::centred);
  }
  path.closeSubPath();
  for(int depth=7;depth>=1;--depth){auto shadow=path;shadow.applyTransform(juce::AffineTransform::translation(depth*1.15f,depth*1.5f));g.setColour(Colours::cyan.withAlpha(.035f));g.strokePath(shadow,juce::PathStrokeType(1.2f));}
  g.setColour(Colours::cyan.withAlpha(0.18f));
  g.fillPath(path);
  g.setColour(Colours::cyan.withAlpha(0.92f));
  g.strokePath(path, juce::PathStrokeType(2.0f));
  g.setFont(juce::FontOptions(10.5f));
  g.setColour(Colours::muted);
  auto graphInt = graph.toNearestInt();
  g.drawText(state.hasReference ? "Underlay = analyzed reference signature" : "Live signature only - no analyzed reference loaded", graphInt.removeFromBottom(18), juce::Justification::centred);
}

void AifredAudioProcessorEditor::drawBrainPanel(juce::Graphics& g,juce::Rectangle<int> bounds)
{
  drawPanel(g,bounds.toFloat(),8.0f);auto inner=bounds.reduced(14,10);const auto telemetry=memoryIndex_.telemetry();
  auto title=inner.removeFromTop(22);g.setFont(juce::FontOptions(14.0f,juce::Font::bold));g.setColour(Colours::ink);g.drawText("AIFRED MEMORY",title,juce::Justification::centredLeft);
  auto visual=inner.removeFromLeft(126).toFloat();const auto centre=visual.getCentre();
  for(int ring=0;ring<3;++ring){const auto radius=18.0f+ring*11.0f;g.setColour((ring==0?Colours::cyan:Colours::violet).withAlpha(.2f+.08f*ring));g.drawEllipse(centre.x-radius,centre.y-radius,radius*2,radius*2,1.2f);}
  const auto nodes=std::min(10,telemetry.transientSnapshots);for(int i=0;i<nodes;++i){const auto angle=ambientPhase_*.08f+juce::MathConstants<float>::twoPi*i/std::max(1,nodes);const auto radius=22.0f+(i%3)*10.0f;
    g.setColour((i==nodes-1&&telemetry.updated?Colours::green:Colours::cyan).withAlpha(i==nodes-1?.95f:.55f));g.fillEllipse(centre.x+std::cos(angle)*radius-2.5f,centre.y+std::sin(angle)*radius-2.5f,5,5);}
  g.setColour(telemetry.hostAvailable?Colours::green:Colours::yellow);g.fillEllipse(centre.x-5,centre.y-5,10,10);
  g.setFont(juce::FontOptions(10.5f,juce::Font::bold));g.setColour(Colours::ink);
  inner.removeFromTop(2);g.drawText("INDEXED  "+juce::String(telemetry.transientSnapshots),inner.removeFromTop(20),juce::Justification::centredLeft);
  g.drawText("SESSIONS  "+juce::String(telemetry.persistedSessions)+" / 10",inner.removeFromTop(20),juce::Justification::centredLeft);
  g.drawText("AGE  "+juce::String(telemetry.newestAgeSeconds,1)+" s",inner.removeFromTop(20),juce::Justification::centredLeft);
  g.drawText(juce::String(core::profile(telemetry.activeProfile).name.data()).replaceCharacter('_',' '),inner.removeFromTop(20),juce::Justification::centredLeft);
  g.setFont(juce::FontOptions(9.5f));g.setColour(telemetry.storageAvailable?Colours::green:Colours::yellow);
  g.drawFittedText((telemetry.storageAvailable?"SQLite local / ":"Transient only / ")+(telemetry.hostAvailable?juce::String("Host healthy"):juce::String("Host offline")),inner,juce::Justification::centredLeft,2);
}

void AifredAudioProcessorEditor::drawSpectrumMeter(juce::Graphics& g, juce::Rectangle<int> bounds, const BetaView& state) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto inner = bounds.reduced(14, 10);
  g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("SPECTROMETER", inner.removeFromTop(24), juce::Justification::centredLeft);
  auto plot=inner.reduced(0,4).toFloat();auto labels=plot.removeFromBottom(16.0f);
  const auto hasSpectrum = state.hasSignal && state.valuesValid;
  const auto bandWidth=plot.getWidth()/static_cast<float>(state.metrics.spectrumBands.size());
  for(size_t i=0;i<state.metrics.spectrumBands.size();++i) {
    const auto value=clamp01(state.metrics.spectrumBands[i]);
    auto slot=juce::Rectangle<float>(plot.getX()+static_cast<float>(i)*bandWidth+1.0f,plot.getY(),std::max(1.0f,bandWidth-2.0f),plot.getHeight());
    g.setColour(Colours::line.withAlpha(0.42f));
    g.fillRoundedRectangle(slot,2.0f);
    if (hasSpectrum && value > 0.0f) {
      auto fill=slot.withTop(slot.getBottom()-slot.getHeight()*value);
      const auto colour=i<9?Colours::green:(i<20?Colours::cyan:(i<26?Colours::violet:Colours::yellow));
      g.setColour(colour.withAlpha(0.86f));
      g.fillRoundedRectangle(fill,2.0f);
    }
  }
  struct Anchor{size_t index;const char* text;};
  constexpr std::array<Anchor,6> anchors {{{0,"20"},{8,"100"},{17,"1k"},{22,"6k"},{24,"10k"},{29,"20k"}}};
  g.setFont(juce::FontOptions(7.5f,juce::Font::bold));g.setColour(Colours::muted);
  for(const auto& anchor:anchors){const auto x=labels.getX()+(static_cast<float>(anchor.index)+.5f)*bandWidth;
    g.drawFittedText(anchor.text,juce::Rectangle<float>(x-14,labels.getY(),28,labels.getHeight()).toNearestInt(),juce::Justification::centred,1);}
  if (!hasSpectrum) {
    g.setFont(juce::FontOptions(11.0f));
    g.setColour(Colours::ink);
    g.drawText("spectrum unavailable", plot.toNearestInt(), juce::Justification::centred);
  }
}

void AifredAudioProcessorEditor::drawCorrelationMeter(juce::Graphics& g, juce::Rectangle<int> bounds, const BetaView& state) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto inner = bounds.reduced(14, 10);
  g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("CORRELATION", inner.removeFromTop(22), juce::Justification::centredLeft);
  auto meter = inner.removeFromTop(18).toFloat();
  g.setColour(Colours::line.withAlpha(0.48f));
  g.fillRoundedRectangle(meter, 6.0f);
  const auto hasCorrelation = state.hasSignal && state.valuesValid;
  if (!hasCorrelation) {
    g.setFont(juce::FontOptions(11.0f));
    g.setColour(Colours::muted);
    g.drawFittedText("HPF 150 Hz     -1 phase risk        0 mono-safe        +1 locked", inner.removeFromTop(18), juce::Justification::centred, 1);
    g.setColour(Colours::ink);
    g.drawText("corr unavailable", inner, juce::Justification::centred);
    return;
  }
  const auto zeroX = meter.getX() + meter.getWidth() * 0.5f;
  const auto valueX = meter.getX() + meter.getWidth() * clamp01((state.metrics.correlation + 1.0f) * 0.5f);
  g.setColour(state.metrics.correlation < 0.0f ? Colours::red : Colours::green);
  g.fillRoundedRectangle(juce::Rectangle<float>(std::min(zeroX, valueX), meter.getY(), std::abs(valueX - zeroX), meter.getHeight()), 6.0f);
  g.setColour(Colours::cyan.withAlpha(0.9f));
  g.drawVerticalLine(juce::roundToInt(valueX), meter.getY() - 3.0f, meter.getBottom() + 3.0f);
  g.setFont(juce::FontOptions(11.0f));
  g.setColour(Colours::muted);
  g.drawFittedText("HPF 150 Hz     -1 phase risk        0 mono-safe        +1 locked", inner.removeFromTop(18), juce::Justification::centred, 1);
  g.setColour(Colours::ink);
  g.drawText("corr " + juce::String(state.metrics.correlation, 2), inner, juce::Justification::centred);
}

} // namespace aifred
