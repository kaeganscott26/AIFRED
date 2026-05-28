#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace aifred {

AifredAudioProcessor::AifredAudioProcessor()
  : AudioProcessor(BusesProperties()
      .withInput("Mix A", juce::AudioChannelSet::stereo(), true)
      .withInput("Mix B", juce::AudioChannelSet::stereo(), false)
      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    parameters_(*this, nullptr, "AIFRED_PARAMETERS", createParameterLayout()) {
  loadLocalSettings();
}

AifredAudioProcessor::~AifredAudioProcessor() {
  analysis_.finalizeCurrentSession();
  saveSessionHistory();
}

juce::AudioProcessorValueTreeState::ParameterLayout AifredAudioProcessor::createParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
  params.push_back(std::make_unique<juce::AudioParameterBool>(
    juce::ParameterID("session_initialized", 1), "Session Initialized", false));
  return { params.begin(), params.end() };
}

void AifredAudioProcessor::prepareToPlay(double sampleRate, int) {
  analysis_.prepare(sampleRate);
  loadSessionHistory();
  compareAnalysis_.prepare(sampleRate);
}

void AifredAudioProcessor::releaseResources() {
  compareAnalysis_.reset();
}

bool AifredAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  const auto input = layouts.getChannelSet(true, 0);
  const auto output = layouts.getMainOutputChannelSet();
  if (input != output || !(input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo())) return false;
  if (layouts.inputBuses.size() > 1) {
    const auto compare = layouts.getChannelSet(true, 1);
    if (!compare.isDisabled() && compare != juce::AudioChannelSet::mono() && compare != juce::AudioChannelSet::stereo()) return false;
  }
  return true;
}

void AifredAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
  juce::ScopedNoDenormals noDenormals;
  auto mixA = getBusBuffer(buffer, true, 0);
  analysis_.pushAudioBlock(mixA);
  if (getBusCount(true) > 1 && !getBus(true, 1)->isEnabled()) {
    compareAnalysis_.reset();
  } else if (getBusCount(true) > 1) {
    auto mixB = getBusBuffer(buffer, true, 1);
    compareAnalysis_.pushAudioBlock(mixB);
  }
}

juce::AudioProcessorEditor* AifredAudioProcessor::createEditor() {
  return new AifredAudioProcessorEditor(*this);
}

void AifredAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
  juce::XmlElement state("AIFRED_STATE");
  state.setAttribute("version", 2);
  state.setAttribute("mode", mode_.load());
  state.setAttribute("theme", settings_.themeId);
  state.setAttribute("layout", settings_.layoutId);
  state.setAttribute("genre", settings_.genreId);
  state.setAttribute("gate", settings_.gate);
  state.setAttribute("aiProvider", settings_.aiProvider);
  state.setAttribute("apiEndpoint", settings_.apiEndpoint);
  state.setAttribute("aiModel", settings_.aiModel);
  state.setAttribute("session_initialized", isSessionInitialized());
  copyXmlToBinary(state, destData);
}

void AifredAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
  auto state = getXmlFromBinary(data, sizeInBytes);
  if (!state || !state->hasTagName("AIFRED_STATE")) return;
  mode_.store(juce::jlimit(0, 2, state->getIntAttribute("mode", static_cast<int>(AnalysisMode::Analyze))));
  settings_.themeId = 1;
  settings_.layoutId = 3;
  settings_.genreId = juce::jlimit(1, 6, state->getIntAttribute("genre", settings_.genreId));
  settings_.gate = juce::jlimit(0.0, 1.0, state->getDoubleAttribute("gate", settings_.gate));
  settings_.aiProvider = state->getStringAttribute("aiProvider", settings_.aiProvider).substring(0, 32);
  settings_.apiEndpoint = state->getStringAttribute("apiEndpoint", settings_.apiEndpoint).substring(0, 256);
  settings_.aiModel = state->getStringAttribute("aiModel", settings_.aiModel).substring(0, 80);
  if (auto* param = parameters_.getParameter("session_initialized")) {
    param->setValueNotifyingHost(state->getBoolAttribute("session_initialized", false) ? 1.0f : 0.0f);
  }
}

HaloState AifredAudioProcessor::getHaloState() const {
  auto state = analysis_.snapshot();
  state.mode = getMode();
  return state;
}

HaloState AifredAudioProcessor::getCompareHaloState() const {
  auto state = compareAnalysis_.snapshot();
  state.mode = AnalysisMode::Compare;
  return state;
}

AnalysisMode AifredAudioProcessor::getMode() const {
  return static_cast<AnalysisMode>(mode_.load());
}

void AifredAudioProcessor::setMode(AnalysisMode mode) {
  mode_.store(static_cast<int>(mode));
}

PluginSettings AifredAudioProcessor::getPluginSettings() const {
  return settings_;
}

void AifredAudioProcessor::setPluginSettings(const PluginSettings& settings) {
  settings_.themeId = 1;
  settings_.layoutId = 3;
  settings_.genreId = juce::jlimit(1, 6, settings.genreId);
  settings_.gate = juce::jlimit(0.0, 1.0, settings.gate);
  settings_.aiProvider = settings.aiProvider.substring(0, 32);
  settings_.apiEndpoint = settings.apiEndpoint.substring(0, 256);
  settings_.apiKey = settings.apiKey.substring(0, 256);
  settings_.aiModel = settings.aiModel.substring(0, 80);
  saveLocalSettings();
}

void AifredAudioProcessor::setReferenceTarget(const ReferenceTarget& target) {
  analysis_.setReferenceTarget(target);
}

void AifredAudioProcessor::clearReferenceTarget() {
  analysis_.clearReferenceTarget();
}

bool AifredAudioProcessor::isSessionInitialized() const {
  if (const auto* value = parameters_.getRawParameterValue("session_initialized")) {
    return value->load() > 0.5f;
  }
  return false;
}

void AifredAudioProcessor::markSessionInitialized() {
  if (auto* param = parameters_.getParameter("session_initialized")) {
    param->setValueNotifyingHost(1.0f);
  }
}

void AifredAudioProcessor::loadLocalSettings() {
  juce::PropertiesFile::Options options;
  options.applicationName = "AIFRED";
  options.filenameSuffix = "config";
  options.folderName = "AIFRED";
  options.osxLibrarySubFolder = "Application Support";
  options.storageFormat = juce::PropertiesFile::storeAsXML;
  juce::PropertiesFile file(options);
  settings_.aiProvider = file.getValue("aiProvider", settings_.aiProvider);
  settings_.apiEndpoint = file.getValue("apiEndpoint", settings_.apiEndpoint);
  settings_.apiKey = file.getValue("apiKey", settings_.apiKey);
  settings_.aiModel = file.getValue("aiModel", settings_.aiModel);
  settings_.genreId = file.getIntValue("genreId", settings_.genreId);
}

void AifredAudioProcessor::saveLocalSettings() const {
  juce::PropertiesFile::Options options;
  options.applicationName = "AIFRED";
  options.filenameSuffix = "config";
  options.folderName = "AIFRED";
  options.osxLibrarySubFolder = "Application Support";
  options.storageFormat = juce::PropertiesFile::storeAsXML;
  juce::PropertiesFile file(options);
  file.setValue("aiProvider", settings_.aiProvider);
  file.setValue("apiEndpoint", settings_.apiEndpoint);
  file.setValue("apiKey", settings_.apiKey);
  file.setValue("aiModel", settings_.aiModel);
  file.setValue("genreId", settings_.genreId);
  file.saveIfNeeded();
}

void AifredAudioProcessor::loadSessionHistory() {
  juce::PropertiesFile::Options options;
  options.applicationName = "AIFRED";
  options.filenameSuffix = "config";
  options.folderName = "AIFRED";
  options.osxLibrarySubFolder = "Application Support";
  options.storageFormat = juce::PropertiesFile::storeAsXML;
  juce::PropertiesFile file(options);
  DspMetrics metrics;
  metrics.sessionCandleCount = juce::jlimit(0, 10, file.getIntValue("sessionCandleCount", 0));
  for (int i = 0; i < 10; ++i) {
    metrics.sessionCandleOpen[static_cast<size_t>(i)] = static_cast<float>(file.getDoubleValue("sessionOpen" + juce::String(i), 0.0));
    metrics.sessionCandleHigh[static_cast<size_t>(i)] = static_cast<float>(file.getDoubleValue("sessionHigh" + juce::String(i), 0.0));
    metrics.sessionCandleLow[static_cast<size_t>(i)] = static_cast<float>(file.getDoubleValue("sessionLow" + juce::String(i), 0.0));
    metrics.sessionCandleClose[static_cast<size_t>(i)] = static_cast<float>(file.getDoubleValue("sessionClose" + juce::String(i), 0.0));
  }
  analysis_.importSessionCandles(metrics);
}

void AifredAudioProcessor::saveSessionHistory() {
  auto metrics = analysis_.exportSessionCandles();
  juce::PropertiesFile::Options options;
  options.applicationName = "AIFRED";
  options.filenameSuffix = "config";
  options.folderName = "AIFRED";
  options.osxLibrarySubFolder = "Application Support";
  options.storageFormat = juce::PropertiesFile::storeAsXML;
  juce::PropertiesFile file(options);
  file.setValue("sessionCandleCount", metrics.sessionCandleCount);
  for (int i = 0; i < 10; ++i) {
    file.setValue("sessionOpen" + juce::String(i), metrics.sessionCandleOpen[static_cast<size_t>(i)]);
    file.setValue("sessionHigh" + juce::String(i), metrics.sessionCandleHigh[static_cast<size_t>(i)]);
    file.setValue("sessionLow" + juce::String(i), metrics.sessionCandleLow[static_cast<size_t>(i)]);
    file.setValue("sessionClose" + juce::String(i), metrics.sessionCandleClose[static_cast<size_t>(i)]);
  }
  file.saveIfNeeded();
}

} // namespace aifred

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new aifred::AifredAudioProcessor();
}
