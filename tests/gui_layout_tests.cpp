#include "plugin-aifred/Source/PluginProcessor.h"
#include "plugin-aifred/Source/PluginEditor.h"
#include "plugin-aifred/Source/MixMemoryIndex.h"

#include <juce_graphics/juce_graphics.h>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>

void check(bool ok,const char* name){if(!ok){std::cerr<<name<<'\n';std::exit(1);}}

namespace aifred {
struct HaloPaintTest {
    static int paintedPixels(AifredAudioProcessorEditor& editor, float crestDb, float truePeakDb,
                             int lane, juce::Colour colour)
    {
        BetaView state;
        state.hasSignal = state.valuesValid = true;
        state.metrics.crestDb = crestDb;
        state.metrics.truePeakDb = truePeakDb;
        state.metrics.rmsDb = -24.0f;
        state.metrics.correlation = 1.0f;
        state.metrics.stereoWidth = 0.0f;
        for (std::size_t i = 0; i < state.metricDetails.size(); ++i) {
            auto& detail = state.metricDetails[i];
            detail.id = static_cast<core::MetricId>(i);
            detail.displayName = core::metricDefinitions[i].displayName;
            detail.unit = core::metricDefinitions[i].unit;
        }
        state.metricDetails[core::index(core::MetricId::crest)].rawCurrent = crestDb;
        state.metricDetails[core::index(core::MetricId::truePeak)].rawCurrent = truePeakDb;
        juce::Image image(juce::Image::ARGB, 500, 500, true);
        juce::Graphics graphics(image);
        editor.drawHalo(graphics, {0, 0, 500, 500}, state, "HALO", false);

        const auto laneIndex = static_cast<float>(lane);
        const auto radius = 444.0f * 0.275f + 18.0f + laneIndex * 8.0f;
        const auto start = -150.0f + laneIndex * 90.0f;
        int painted = 0;
        for (int y = 0; y < 500; ++y)
            for (int x = 0; x < 500; ++x) {
                const auto distance = std::hypot(static_cast<float>(x - 250), static_cast<float>(y - 250));
                if (std::abs(distance - radius) > 2.0f) continue;
                const auto angle = juce::radiansToDegrees(std::atan2(static_cast<float>(y - 250),
                                                                      static_cast<float>(x - 250)));
                if (angle < start - 2.0f || angle > start + 74.0f) continue;
                const auto pixel = image.getPixelAt(x, y);
                if (std::abs(static_cast<int>(pixel.getRed()) - colour.getRed()) < 45 &&
                    std::abs(static_cast<int>(pixel.getGreen()) - colour.getGreen()) < 45 &&
                    std::abs(static_cast<int>(pixel.getBlue()) - colour.getBlue()) < 45)
                    ++painted;
            }
        return painted;
    }
};
}

int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI runtime;
    const auto output=argc>1?juce::File(argv[1]):juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("aifred-gui-snapshots");
    output.createDirectory();
    const auto memoryPath=output.getChildFile("test-memory.sqlite").getFullPathName();
    juce::File(memoryPath).deleteFile();
#if JUCE_WINDOWS
    _putenv_s("AIFRED_MEMORY_DB_PATH",memoryPath.toRawUTF8());
#else
    setenv("AIFRED_MEMORY_DB_PATH",memoryPath.toRawUTF8(),1);
#endif
    auto processor=std::make_unique<aifred::AifredAudioProcessor>();
    juce::XmlElement restoredState("AIFRED_STATE");restoredState.setAttribute("help_seen",true);
    juce::MemoryBlock state;juce::AudioProcessor::copyXmlToBinary(restoredState,state);
    processor->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    auto editor=std::unique_ptr<juce::AudioProcessorEditor>(processor->createEditor());
    auto& paintedHalo = static_cast<aifred::AifredAudioProcessorEditor&>(*editor);
    const auto violet = aifred::Colours::violet;
    const auto green = aifred::Colours::green;
    const auto truePeakEmpty = aifred::HaloPaintTest::paintedPixels(paintedHalo, 0, -24, 2, violet);
    const auto truePeakQuiet = aifred::HaloPaintTest::paintedPixels(paintedHalo, 0, -20, 2, violet);
    const auto truePeakHalf = aifred::HaloPaintTest::paintedPixels(paintedHalo, 0, -12, 2, violet);
    const auto truePeakHot = aifred::HaloPaintTest::paintedPixels(paintedHalo, 0, -2, 2, violet);
    check(truePeakEmpty == 0 && truePeakHot > truePeakHalf && truePeakHalf > truePeakQuiet && truePeakQuiet > 0,
          "painted purple dBTP arc grows from -24 toward 0");
    const auto crestEmpty = aifred::HaloPaintTest::paintedPixels(paintedHalo, 0, -24, 0, green);
    const auto crestHalf = aifred::HaloPaintTest::paintedPixels(paintedHalo, 12, -24, 0, green);
    const auto crestFull = aifred::HaloPaintTest::paintedPixels(paintedHalo, 24, -24, 0, green);
    check(crestEmpty == 0 && crestFull > crestHalf && crestHalf > 0,
          "painted green Crest arc grows from 0 to 24 dB");
    const std::array<std::array<int,2>,8> sizes {{{{360,280}},{{640,480}},{{1080,680}},{{1280,760}},{{1360,820}},{{1600,900}},{{1920,1080}},{{1920,1780}}}};
    juce::PNGImageFormat png;
    {
        aifred::MixMemoryIndex memory; aifred::BetaView measured;measured.hasSignal=true;measured.valuesValid=true;measured.metrics.liveCandleCount=1;
        measured.metrics.rmsDb=-18;measured.metrics.truePeakDb=-2;measured.metrics.crestDb=12;measured.metrics.shortTermLufs=-16;measured.metrics.stereoWidth=.42f;measured.metrics.correlation=.63f;
        memory.observe(measured,aifred::AnalysisMode::Analyze,"No reference",true);memory.observe(measured,aifred::AnalysisMode::Analyze,"No reference",true);
        check(memory.telemetry().transientSnapshots==1,"memory deduplicates within one candle boundary");
        measured.metrics.liveCandleOpen[0]=-18;memory.observe(measured,aifred::AnalysisMode::Analyze,"No reference",true);
        check(memory.telemetry().transientSnapshots==2&&memory.telemetry().storageAvailable,"memory indexes real candle changes into bounded local SQLite");
    }
    for(const auto& size:sizes)
    {
        editor->setSize(size[0],size[1]);
        for(int i=0;i<editor->getNumChildComponents();++i)
        {
            const auto* child=editor->getChildComponent(i);
            if(child->getWidth()==0||child->getHeight()==0)continue;
            check(editor->getLocalBounds().contains(child->getBounds()),"visible child remains inside editor");
            for(int j=i+1;j<editor->getNumChildComponents();++j)
            {
                const auto* other=editor->getChildComponent(j);
                if(other->getWidth()==0||other->getHeight()==0)continue;
                check(!child->getBounds().intersects(other->getBounds()),"interactive controls do not overlap");
            }
        }
        const auto image=editor->createComponentSnapshot(editor->getLocalBounds());
        check(image.isValid()&&image.getWidth()==size[0]&&image.getHeight()==size[1],"responsive snapshot has requested dimensions");
        auto stream=output.getChildFile("analyze-"+juce::String(size[0])+"x"+juce::String(size[1])+".png").createOutputStream();
        check(stream!=nullptr&&png.writeImageToStream(image,*stream),"responsive snapshot written");
    }
    for(const auto mode:{aifred::AnalysisMode::Reference,aifred::AnalysisMode::Compare})
    {
        processor->setMode(mode);editor->setSize(360,280);editor->resized();
        const auto image=editor->createComponentSnapshot(editor->getLocalBounds());
        check(image.isValid(),"minimum mode snapshot renders");
        auto stream=output.getChildFile(mode==aifred::AnalysisMode::Reference?"reference-360x280.png":"compare-360x280.png").createOutputStream();
        check(stream!=nullptr&&png.writeImageToStream(image,*stream),"minimum mode snapshot written");
    }
    std::cout<<"Responsive GUI snapshots: PASS ("<<output.getFullPathName()<<")\n";
}
