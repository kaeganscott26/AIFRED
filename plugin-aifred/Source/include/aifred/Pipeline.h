#pragma once
#include "Engine.h"
#include "Filter.h"
#include <juce_core/juce_core.h>
#include <atomic>
#include <deque>
#include <memory>
#include <mutex>

namespace aifred::core
{
struct CandleHistorySnapshot
{
    std::array<float,10> sessionOpen {},sessionHigh {},sessionLow {},sessionClose {};
    std::array<float,10> minuteOpen {},minuteHigh {},minuteLow {},minuteClose {};
    std::array<float,10> liveOpen {},liveHigh {},liveLow {},liveClose {};
    int sessionCount=0,minuteCount=0,liveCount=0;
};

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
    void setProfile(ProfileId id) noexcept
    {
        engine_->requestProfile(id);
        if(!presentationCustomized_.load(std::memory_order_relaxed))
            displayRange_.store(profile(id).presentation.spectrumRange,std::memory_order_relaxed);
    }
    ProfileId selectedProfile() const noexcept {return engine_->requestedProfile();}
    void setSpectrumDisplayRange(SpectrumDisplayRange range) noexcept
    {
        displayRange_.store(range,std::memory_order_relaxed);
        presentationCustomized_.store(true,std::memory_order_relaxed);
    }
    void restorePresentation(SpectrumDisplayRange range,bool customized) noexcept
    {
        displayRange_.store(range,std::memory_order_relaxed);
        presentationCustomized_.store(customized,std::memory_order_relaxed);
    }
    PresentationConfiguration presentation() const noexcept
    {
        auto result=profile(selectedProfile()).presentation;
        result.spectrumRange=displayRange_.load(std::memory_order_relaxed);
        return result;
    }
    bool presentationCustomized() const noexcept {return presentationCustomized_.load(std::memory_order_relaxed);}
    void reset() noexcept {engine_->requestReset();}
    EngineSnapshot live() const;
    ObservationSnapshot observation() const;
    CandleHistorySnapshot candleHistory() const;
    juce::String contextForQuestion(const juce::String&,const ReferenceDistribution* reference=nullptr,juce::String mode="analyze",const ObservationSnapshot* compare=nullptr);
    void recordResponse(const juce::String& response);
    const std::string& instanceId() const noexcept {return instanceId_;}
private:
    struct CandleFrame { double open=0,high=0,low=0,close=0; bool active=false; };
    struct CandleSeries
    {
        std::array<CandleFrame,10> committed {};
        CandleFrame current {};
        int write=0,count=0;
        double elapsed=0;
    };
    void hiResTimerCallback() override;
    void updateCandleHistory(const ObservationSnapshot&,double) noexcept;
    static void updateSeries(CandleSeries&,const MetricObservation&,double,double,bool) noexcept;
    static void copySeries(const CandleSeries&,std::array<float,10>&,std::array<float,10>&,
                           std::array<float,10>&,std::array<float,10>&,int&) noexcept;
    static double now() noexcept {return juce::Time::getMillisecondCounterHiRes()/1000;}
    std::unique_ptr<Engine> engine_;
    BufferHunter hunter_;
    mutable std::mutex mutex_;
    EngineSnapshot live_;
    std::string channel_,version_,instanceId_,sessionId_;
    std::deque<juce::var> history_;
    CandleSeries sessionCandles_,minuteCandles_,liveCandles_;
    double lastCandleUpdate_=-1;
    std::atomic<SpectrumDisplayRange> displayRange_ {SpectrumDisplayRange::db96};
    std::atomic<bool> presentationCustomized_ {false};
};
juce::var filteredContextJson(const FilteredMixContext&);
}
