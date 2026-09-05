#include "plugin-aifred/Source/BetaView.h"
#include "aifred/Pipeline.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
using namespace aifred::core;
void check(bool ok,const char* name) {if(!ok){std::cerr<<name<<'\n';std::exit(1);}}
int main()
{
    auto live=std::make_unique<EngineSnapshot>(); auto observation=std::make_unique<ObservationSnapshot>();
    live->get(MetricId::correlation)={-.463742,true};live->get(MetricId::width)={71.638,true};
    observation->metrics[index(MetricId::rms)].valid=true;observation->metrics[index(MetricId::rms)].typical=-2.347123;
    observation->metrics[index(MetricId::correlation)].valid=true;observation->metrics[index(MetricId::correlation)].typical=.8;
    live->binCount=4097;live->averagePower[300]=1e-12;
    auto view=std::make_unique<aifred::BetaView>(aifred::makeBetaView(*live,*observation));
    check(view->metrics.rmsDb==static_cast<float>(-2.347123),"meter retains fractional observation");
    check(view->metrics.correlation==static_cast<float>(-.463742),"correlation comes from live DSP");
    check(view->metrics.stereoWidth==static_cast<float>(71.638)/100,"width comes from live DSP");
    check(view->spectrumPower[300]==1e-12,"display floor cannot mutate analytical bins");
    auto first=std::make_unique<Pipeline>("beta","0.3.6"),second=std::make_unique<Pipeline>("official","4.0.0-alpha.2");
    check(first->instanceId()!=second->instanceId(),"instance isolation");
    first->prepare(48000,2);
    first->setProfile(ProfileId::stereoPhase);
    std::array<float,480> audio{};const float* channels[]={audio.data(),audio.data()};
    first->process(channels,2,480);
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
    while(first->live().profileId!=ProfileId::stereoPhase&&std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
    auto request=juce::JSON::parse(first->contextForQuestion("I changed 6 kHz"));
    check(request["profile_id"].toString()=="STEREO_PHASE_DIAGNOSTIC"&&(int)request["profile_version"]==2,"context profile epoch propagation");
    first->recordResponse("A response");
    request=juce::JSON::parse(first->contextForQuestion("how about now?"));
    auto* history=request["session_context"].getArray();
    check(history&&history->size()==1,"previous observation retained");
    check((*history)[0]["user_statement"].toString()=="I changed 6 kHz","action question continuity");
    check((*history)[0]["assistant_response"].toString()=="A response","response continuity");
    check(!request.hasProperty("mastered"),"mastered is not a measurement");
    for(int i=0;i<8;++i)request=juce::JSON::parse(first->contextForQuestion("next"));
    check(request["session_context"].getArray()->size()==4,"bounded history");
    check(juce::JSON::parse(second->contextForQuestion("new instance"))["session_context"].getArray()->isEmpty(),"no merged session state");
    check(request["metrics"].getArray()->getReference(3)["unit"].toString()=="LUFS","serialized LUFS unit");
    check(request["bands"].getArray()->getReference(16)["centre_hz"]==juce::var(850.0),"serialized 850 Hz");
    std::cout<<"Frontend and context contracts: PASS\n";
}
