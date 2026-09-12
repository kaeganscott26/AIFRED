#include "plugin-aifred/Source/PluginProcessor.h"
#include "plugin-aifred/Source/MixMemoryIndex.h"

#include <juce_graphics/juce_graphics.h>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>

void check(bool ok,const char* name){if(!ok){std::cerr<<name<<'\n';std::exit(1);}}

int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI runtime;
    const auto output=argc>1?juce::File(argv[1]):juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("aifred-gui-snapshots");
    output.createDirectory();
    const auto memoryPath=output.getChildFile("test-memory.sqlite").getFullPathName();
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
