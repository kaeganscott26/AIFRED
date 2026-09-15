#pragma once
#include "Contracts.h"

namespace aifred::core
{
enum class Trend { unavailable, stable, rising, falling };
struct MetricObservation
{
    bool valid=false;
    double typical=0, low=0, high=0, minimum=0, maximum=0, latest=0, coverageSeconds=0;
    std::size_t count=0;
    Trend trend=Trend::unavailable;
};
struct ObservationSnapshot
{
    std::uint32_t schema=schemaVersion,profileVersion=1;
    ProfileId profileId=ProfileId::mixBalanced;
    std::uint64_t id=0,epoch=0,engineEpoch=0,sampleStart=0,sampleEnd=0;
    double sampleRate=0,durationSeconds=0,ageSeconds=0;
    bool valid=false,fresh=false,signalActive=false,sufficient=false,transportKnown=false,transportPlaying=false;
    std::array<MetricObservation,metricCount> metrics {};
    std::array<MetricObservation,30> bands {};
    double correlationBelowZeroSeconds=0;
    const MetricObservation& get(MetricId id_) const noexcept {return metrics[index(id_)];}
};
class BufferHunter
{
public:
    static constexpr std::size_t capacity=300;
    void consume(const EngineSnapshot&,double monotonicSeconds) noexcept;
    ObservationSnapshot snapshot(double monotonicSeconds) const noexcept;
    std::size_t storedFrames() const noexcept {return count_;}
private:
    struct Frame
    {
        std::uint64_t start=0,end=0;
        std::array<MetricValue,metricCount> metrics {};
        std::array<MetricValue,30> bands {};
    };
    void begin(const EngineSnapshot&) noexcept;
    void summarize() noexcept;
    MetricObservation summarizeMetric(std::size_t metric,bool spectrum) const noexcept;
    std::array<Frame,capacity> frames_ {};
    std::size_t position_=0,count_=0;
    ObservationSnapshot observation_;
    std::uint64_t lastSequence_=0,lastEnd_=0,nextId_=0,epoch_=0;
    double lastUpdate_=-1,lastActive_=-1;
};
}
