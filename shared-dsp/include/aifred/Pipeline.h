#pragma once
#include "Engine.h"
#include "Filter.h"
#include <juce_core/juce_core.h>
#include <deque>
#include <memory>
#include <mutex>

namespace aifred::core
{
// Processor-owned lifetime. The 20 ms service cadence bounds consumer latency;
// correctness/overflow is governed by SPSC ownership, never by timer scheduling.
class Pipeline final : private juce::HighResolutionTimer
{
public:
    Pipeline(std::string channel,std::string version);
    ~Pipeline() override;
    void prepare(double rate,int channels) noexcept {engine_->prepare(rate,channels);}
    void process(const float* const* data,int channels,int samples,bool known=false,bool playing=false,std::int64_t position=-1) noexcept
    {engine_->process(data,channels,samples,known,playing,position);}
    void setProfile(ProfileId id) noexcept {engine_->requestProfile(id);}
    ProfileId selectedProfile() const noexcept {return engine_->requestedProfile();}
    void reset() noexcept {engine_->requestReset();}
    EngineSnapshot live() const;
    ObservationSnapshot observation() const;
    juce::String contextForQuestion(const juce::String&,const ReferenceDistribution* reference=nullptr,juce::String mode="analyze",const ObservationSnapshot* compare=nullptr);
    void recordResponse(const juce::String& response);
    const std::string& instanceId() const noexcept {return instanceId_;}
private:
    void hiResTimerCallback() override;
    static double now() noexcept {return juce::Time::getMillisecondCounterHiRes()/1000;}
    std::unique_ptr<Engine> engine_;
    BufferHunter hunter_;
    mutable std::mutex mutex_;
    EngineSnapshot live_;
    std::string channel_,version_,instanceId_,sessionId_;
    std::deque<juce::var> history_;
};
juce::var filteredContextJson(const FilteredMixContext&);
}
