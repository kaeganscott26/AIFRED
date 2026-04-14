#include "PluginEditor.h"

#include <BinaryData.h>

#include <array>
#include <cmath>

namespace aifred {
namespace {

juce::Colour colourForAlignment(float alignment) {
  if (alignment >= 0.80f) return Colours::green;
  if (alignment >= 0.60f) return Colours::cyan;
  if (alignment >= 0.40f) return Colours::yellow;
  return Colours::red;
}

void drawPanel(juce::Graphics& g, juce::Rectangle<float> bounds, float radius = 8.0f) {
  g.setGradientFill(juce::ColourGradient(Colours::panel2, bounds.getX(), bounds.getY(),
                                         juce::Colour(0xff03070c), bounds.getRight(), bounds.getBottom(), false));
  g.fillRoundedRectangle(bounds, radius);
  g.setColour(Colours::line.withAlpha(0.82f));
  g.drawRoundedRectangle(bounds, radius, 1.0f);
}

juce::String pct(float value) {
  return juce::String(juce::roundToInt(value * 100.0f)) + "%";
}

} // namespace

AifredAudioProcessorEditor::AifredAudioProcessorEditor(AifredAudioProcessor& processor)
  : AudioProcessorEditor(&processor), processor_(processor) {
  setLookAndFeel(&lookAndFeel_);
  mascot_ = juce::ImageFileFormat::loadFrom(BinaryData::aifredmascot_jpg, BinaryData::aifredmascot_jpgSize);
  setResizable(true, true);
  setResizeLimits(860, 520, 1600, 980);
  setSize(1180, 700);
  startTimerHz(30);
}

AifredAudioProcessorEditor::~AifredAudioProcessorEditor() {
  setLookAndFeel(nullptr);
}

void AifredAudioProcessorEditor::timerCallback() {
  state_ = processor_.getHaloState();
  repaint();
}

void AifredAudioProcessorEditor::paint(juce::Graphics& g) {
  auto bounds = getLocalBounds();
  g.fillAll(juce::Colour(0xff02060b));
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff07111d), 0, 0,
                                         juce::Colour(0xff02060b), static_cast<float>(getWidth()), static_cast<float>(getHeight()), false));
  g.fillRect(bounds);

  for (int x = 0; x < getWidth(); x += 42) {
    g.setColour(Colours::cyan.withAlpha(0.035f));
    g.drawVerticalLine(x, 0.0f, static_cast<float>(getHeight()));
  }
  for (int y = 0; y < getHeight(); y += 42) {
    g.setColour(Colours::green.withAlpha(0.025f));
    g.drawHorizontalLine(y, 0.0f, static_cast<float>(getWidth()));
  }

  drawHeader(g, bounds.removeFromTop(82).reduced(18, 12));

  auto main = bounds.reduced(18, 8);
  auto left = main.removeFromLeft(juce::roundToInt(main.getWidth() * 0.31f));
  auto right = main.removeFromRight(juce::roundToInt(main.getWidth() * 0.27f));
  auto center = main.reduced(14, 0);

  drawMixSignature(g, left.removeFromTop(left.getHeight() / 2).reduced(0, 0), state_);
  drawDomainCard(g, left.removeFromTop(88).reduced(0, 8), "TONE", state_.tone);
  drawDomainCard(g, left.removeFromTop(88).reduced(0, 8), "STEREO", state_.stereo);
  drawDomainCard(g, left.removeFromTop(88).reduced(0, 8), "LOUDNESS", state_.loudness);
  drawDomainCard(g, left.removeFromTop(88).reduced(0, 8), "DYNAMICS", state_.dynamics);

  drawHalo(g, center.reduced(0, 12), state_);
  drawFixList(g, right.reduced(0, 0), state_);
}

void AifredAudioProcessorEditor::resized() {}

void AifredAudioProcessorEditor::drawHeader(juce::Graphics& g, juce::Rectangle<int> bounds) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto logo = bounds.removeFromLeft(62).reduced(10);
  if (mascot_.isValid()) {
    g.drawImageWithin(mascot_, logo.getX(), logo.getY(), logo.getWidth(), logo.getHeight(),
                      juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
  }

  auto text = bounds.removeFromLeft(430);
  g.setFont(juce::FontOptions(25.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("AIFRED", text.removeFromTop(34), juce::Justification::centredLeft);
  g.setFont(juce::FontOptions(13.0f));
  g.setColour(Colours::green);
  g.drawText("North3rnLight3r mix alignment VST3", text, juce::Justification::centredLeft);

  auto mode = bounds.removeFromRight(360).reduced(4, 12);
  g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
  for (auto label : {"ANALYZE", "REFERENCE", "COMPARE"}) {
    auto cell = mode.removeFromLeft(112).reduced(5, 0);
    g.setColour(juce::Colour(0xff07131e));
    g.fillRoundedRectangle(cell.toFloat(), 6.0f);
    g.setColour(juce::String(label) == "ANALYZE" ? Colours::cyan : Colours::line);
    g.drawRoundedRectangle(cell.toFloat(), 6.0f, 1.0f);
    g.setColour(juce::String(label) == "ANALYZE" ? Colours::ink : Colours::muted);
    g.drawText(label, cell, juce::Justification::centred);
  }
}

void AifredAudioProcessorEditor::drawHalo(juce::Graphics& g, juce::Rectangle<int> bounds, const HaloState& state) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto haloBounds = bounds.reduced(28);
  const auto size = std::min(haloBounds.getWidth(), haloBounds.getHeight());
  auto square = juce::Rectangle<int>(0, 0, size, size).withCentre(haloBounds.getCentre());
  auto area = square.toFloat().reduced(24.0f);
  auto centre = area.getCentre();
  auto radius = std::min(area.getWidth(), area.getHeight()) * 0.47f;

  g.setColour(Colours::cyan.withAlpha(0.08f));
  g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

  const std::array<const DomainAlignment*, 4> domains = {&state.tone, &state.stereo, &state.loudness, &state.dynamics};
  const std::array<float, 4> starts = {-155.0f, -62.0f, 31.0f, 124.0f};
  for (int i = 0; i < 4; ++i) {
    auto alignment = domains[static_cast<size_t>(i)]->alignment01;
    auto colour = colourForAlignment(alignment);
    auto arcRadius = radius - static_cast<float>(i % 2) * 13.0f;
    juce::Path bg;
    bg.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                     juce::degreesToRadians(starts[static_cast<size_t>(i)]),
                     juce::degreesToRadians(starts[static_cast<size_t>(i)] + 72.0f), true);
    g.setColour(Colours::line.withAlpha(0.45f));
    g.strokePath(bg, juce::PathStrokeType(8.0f));

    juce::Path arc;
    arc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                      juce::degreesToRadians(starts[static_cast<size_t>(i)]),
                      juce::degreesToRadians(starts[static_cast<size_t>(i)] + 72.0f * alignment), true);
    g.setColour(colour.withAlpha(0.92f));
    g.strokePath(arc, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
  }

  auto card = juce::Rectangle<float>(centre.x - 190.0f, centre.y - 78.0f, 380.0f, 156.0f);
  drawPanel(g, card, 8.0f);
  g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText(state.primaryTitle, card.removeFromTop(38.0f), juce::Justification::centred);
  g.setFont(juce::FontOptions(43.0f, juce::Font::bold));
  g.setColour(colourForAlignment(state.totalAlignment01));
  g.drawText(pct(state.totalAlignment01), card.removeFromTop(58.0f), juce::Justification::centred);
  g.setFont(juce::FontOptions(13.0f));
  g.setColour(Colours::muted);
  g.drawFittedText(juce::String(state.primaryCause), card.toNearestInt().reduced(18, 0), juce::Justification::centred, 2);

  g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
  g.setColour(Colours::muted);
  g.drawText("Alignment Score + domain rings + ranked cause-based fixes", bounds.removeFromBottom(28), juce::Justification::centred);
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
  g.setFont(juce::FontOptions(23.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText(pct(alignment.alignment01), inner.removeFromTop(34), juce::Justification::centredLeft);
  g.setFont(juce::FontOptions(11.5f));
  g.setColour(Colours::muted);
  g.drawText(juce::String(severityName(alignment.severity)) + " / " + confidenceName(alignment.confidence), inner, juce::Justification::centredLeft);
}

void AifredAudioProcessorEditor::drawFixList(juce::Graphics& g, juce::Rectangle<int> bounds, const HaloState& state) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto inner = bounds.reduced(16);
  g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("FIX LIST", inner.removeFromTop(34), juce::Justification::centredLeft);
  g.setFont(juce::FontOptions(12.0f));
  g.setColour(Colours::muted);
  g.drawFittedText("Aifred ranks what matters first. No fake certainty, no meter panic.", inner.removeFromTop(48), juce::Justification::topLeft, 2);

  if (state.rankedFixes.empty()) {
    g.setColour(Colours::green);
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.drawFittedText("Play audio to start the read. When the mix lands, Aifred stays quiet.", inner, juce::Justification::centred, 4);
    return;
  }

  for (const auto& fix : state.rankedFixes) {
    auto card = inner.removeFromTop(128).reduced(0, 6);
    drawPanel(g, card.toFloat(), 8.0f);
    auto content = card.reduced(12, 8);
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.setColour(colourForAlignment(1.0f - fix.priority));
    g.drawText(juce::String(domainName(fix.impactDomain)) + " / " + severityName(fix.severity), content.removeFromTop(20), juce::Justification::centredLeft);
    g.setColour(Colours::ink);
    g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    g.drawText(fix.title, content.removeFromTop(24), juce::Justification::centredLeft);
    g.setColour(Colours::muted);
    g.setFont(juce::FontOptions(12.0f));
    g.drawFittedText(fix.nextStep, content, juce::Justification::topLeft, 3);
  }
}

void AifredAudioProcessorEditor::drawMixSignature(juce::Graphics& g, juce::Rectangle<int> bounds, const HaloState& state) {
  drawPanel(g, bounds.toFloat(), 8.0f);
  auto inner = bounds.reduced(16);
  g.setFont(juce::FontOptions(17.0f, juce::Font::bold));
  g.setColour(Colours::ink);
  g.drawText("MIX SIGNATURE", inner.removeFromTop(30), juce::Justification::centredLeft);
  auto graph = inner.reduced(4, 10).toFloat();
  g.setColour(Colours::line.withAlpha(0.6f));
  g.drawRoundedRectangle(graph, 8.0f, 1.0f);
  auto centre = graph.getCentre();
  const std::array<float, 4> values = {state.tone.alignment01, state.stereo.alignment01, state.loudness.alignment01, state.dynamics.alignment01};
  juce::Path path;
  for (int i = 0; i < 4; ++i) {
    const auto angle = juce::MathConstants<float>::twoPi * static_cast<float>(i) / 4.0f - juce::MathConstants<float>::halfPi;
    const auto radius = std::min(graph.getWidth(), graph.getHeight()) * 0.42f * values[static_cast<size_t>(i)];
    const auto point = juce::Point<float>(centre.x + std::cos(angle) * radius, centre.y + std::sin(angle) * radius);
    if (i == 0) path.startNewSubPath(point); else path.lineTo(point);
  }
  path.closeSubPath();
  g.setColour(Colours::cyan.withAlpha(0.18f));
  g.fillPath(path);
  g.setColour(Colours::cyan.withAlpha(0.92f));
  g.strokePath(path, juce::PathStrokeType(2.0f));
}

} // namespace aifred
