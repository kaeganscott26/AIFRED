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
    live->profileId=ProfileId::mixBalanced;live->profileVersion=profile(ProfileId::mixBalanced).version;
    live->get(MetricId::rms)={-3.125,true};live->get(MetricId::correlation)={-.463742,true};live->get(MetricId::width)={71.638,true};
    observation->metrics[index(MetricId::rms)].valid=true;observation->metrics[index(MetricId::rms)].typical=-2.347123;
    observation->metrics[index(MetricId::correlation)].valid=true;observation->metrics[index(MetricId::correlation)].typical=.8;
    live->binCount=4097;live->averagePower[300]=1e-12;live->peakPower[300]=1e-10;
    for(std::size_t i=0;i<observation->bands.size();++i){observation->bands[i].valid=true;observation->bands[i].typical=-96.0+static_cast<double>(i);}
    auto view=std::make_unique<aifred::BetaView>(aifred::makeBetaView(*live,*observation));
    check(view->metrics.rmsDb==static_cast<float>(-2.347123),"meter retains fractional observation");
    check(view->metrics.correlation==static_cast<float>(-.463742),"correlation comes from live DSP");
    check(view->metrics.stereoWidth==static_cast<float>(71.638)/100,"width comes from live DSP");
    check(view->spectrumPower[300]==1e-12,"display floor cannot mutate analytical bins");
    check(view->peakSpectrumPower[300]==1e-10,"peak trace retains analytical power");
    check(spectrumFloorDb(view->presentation.spectrumRange)==-96,"professional presentation default");
    check(view->metricDetails[index(MetricId::correlation)].isLive&&view->metricDetails[index(MetricId::correlation)].displayedValue==-.463742,"click-ready live metric metadata");
    check(aifred::rmsPresentation(-96,SpectrumDisplayRange::db96)==0&&aifred::rmsPresentation(-48,SpectrumDisplayRange::db96)==.5f&&aifred::rmsPresentation(0,SpectrumDisplayRange::db96)==1,"RMS -96 presentation mapping");
    check(aifred::rmsPresentation(-72,SpectrumDisplayRange::db72)==0&&aifred::rmsPresentation(-36,SpectrumDisplayRange::db72)==.5f&&aifred::rmsPresentation(0,SpectrumDisplayRange::db72)==1,"RMS -72 presentation mapping");
    check(aifred::truePeakPresentation(-24)==0&&aifred::truePeakPresentation(-12)==.5f&&aifred::truePeakPresentation(0)==1&&aifred::truePeakPresentation(-2)>aifred::truePeakPresentation(-12),"true peak moves hotter toward zero without -6 saturation");
    check(aifred::crestPresentation(0)==0&&aifred::crestPresentation(12)==.5f&&aifred::crestPresentation(24)==1,"crest difference presentation mapping");
    check(aifred::stereoSpreadPresentation(1)==0&&aifred::stereoSpreadPresentation(0)==.5f&&aifred::stereoSpreadPresentation(-1)==1,"correlation phase spread mapping");
    check(aifred::compareDelta(-6,-8)==2,"compare delta is A minus B");
    for(const auto size:std::array<std::array<float,2>,8>{{{{360,280}},{{640,480}},{{1080,680}},{{1280,760}},{{1360,820}},{{1600,900}},{{1920,1080}},{{1920,1780}}}})
    {
        const auto canvas=aifred::responsiveCanvas(size[0],size[1]);
        check(canvas.scale>0&&canvas.x>=-.1f&&canvas.y>=-.1f&&canvas.x+canvas.width<=size[0]+.1f&&canvas.y+canvas.height<=size[1]+.1f,"responsive design canvas remains contained");
    }
    check(!aifred::modalShouldDismiss(10,10,100,100,50,50)&&aifred::modalShouldDismiss(10,10,100,100,5,50),"modal inside remains and outside dismisses");
    for(std::size_t i=0;i<view->metrics.spectrumBands.size();++i)check(view->metrics.spectrumBands[i]==(observation->bands[i].valid?aifred::linearPresentation(static_cast<float>(observation->bands[i].typical),-96,0):0),"every spectrum bar maps its measured band");
    check(!view->metricDetails[index(MetricId::rms)].isLive&&view->metricDetails[index(MetricId::rms)].rawCurrent==-3.125&&view->metricDetails[index(MetricId::rms)].displayedValue==-2.347123,"click-ready observed metric metadata");
    auto first=std::make_unique<Pipeline>("beta","0.3.6"),second=std::make_unique<Pipeline>("official","4.0.0-alpha.2");
    check(first->instanceId()!=second->instanceId(),"instance isolation");
    first->prepare(48000,2);
    first->setProfile(ProfileId::stereoPhase);
    std::array<float,480> audio{};const float* channels[]={audio.data(),audio.data()};
    first->process(channels,2,480);
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
    while(first->live().profileId!=ProfileId::stereoPhase&&std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
    const auto measurementEpoch=first->live().epoch;
    first->setSpectrumDisplayRange(SpectrumDisplayRange::db48);
    check(first->presentation().spectrumRange==SpectrumDisplayRange::db48&&first->live().epoch==measurementEpoch,"presentation change does not reset measurement epoch");
    auto request=juce::JSON::parse(first->contextForQuestion("I changed 6 kHz"));
    check(request["profile_id"].toString()=="STEREO_PHASE_DIAGNOSTIC"&&(int)request["profile_version"]==2,"context profile epoch propagation");
    check(request["measurement_configuration_id"].toString()=="STEREO_PHASE_DIAGNOSTIC.r2"&&(int)request["profile_schema_version"]==2,"context configuration identity");
    const auto& correlation=request["metrics"].getArray()->getReference(index(MetricId::correlation));
    check(correlation["context_value_source"].toString()=="observed"&&(bool)correlation["frontend_live_source"],"serialized metric ownership");
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
