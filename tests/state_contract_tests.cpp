#include "plugin-aifred/Source/PluginProcessor.h"
#include <iostream>
#include <memory>
#include <cstdlib>
void check(bool ok,const char* name){if(!ok){std::cerr<<name<<'\n';std::exit(1);}}
int main()
{
    juce::ScopedJuceInitialiser_GUI runtime;
    auto original=std::make_unique<aifred::AifredAudioProcessor>();
    for(const auto& profile:aifred::core::profiles)
    {
        original->pipeline().setProfile(profile.id);
        juce::MemoryBlock state;original->getStateInformation(state);
        auto restored=std::make_unique<aifred::AifredAudioProcessor>();
        restored->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
        check(restored->pipeline().selectedProfile()==profile.id,"profile state roundtrip");
        restored->prepareToPlay(48000,256);
        juce::AudioBuffer<float> audio(2,256);audio.clear();audio.setSample(0,50,.375f);audio.setSample(1,50,-.25f);
        juce::MidiBuffer midi;restored->processBlock(audio,midi);
        check(audio.getSample(0,50)==.375f&&audio.getSample(1,50)==-.25f,"analyzer audio pass-through");
        restored->releaseResources();
        check(restored->pipeline().selectedProfile()==profile.id,"host suspension retains profile");
    }
    juce::XmlElement prior("AIFRED_STATE");prior.setAttribute("version",2);
    juce::MemoryBlock oldState;juce::AudioProcessor::copyXmlToBinary(prior,oldState);
    original->setStateInformation(oldState.getData(),static_cast<int>(oldState.getSize()));
    check(original->pipeline().selectedProfile()==aifred::core::ProfileId::mixBalanced,"state without profile defaults safely");
    std::cout<<"Plugin state and pass-through contracts: PASS\n";
}
