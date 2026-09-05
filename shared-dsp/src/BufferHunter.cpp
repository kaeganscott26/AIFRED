#include "aifred/BufferHunter.h"
#include <algorithm>

namespace aifred::core
{
void BufferHunter::begin(const EngineSnapshot& s) noexcept
{
    position_=count_=0;observation_={};observation_.epoch=++epoch_;observation_.engineEpoch=s.epoch;
    observation_.profileId=s.profileId;observation_.profileVersion=s.profileVersion;observation_.sampleRate=s.sampleRate;
    lastEnd_=0;lastActive_=-1;
}
void BufferHunter::consume(const EngineSnapshot& s,double now) noexcept
{
    if(s.sequence<=lastSequence_) return;
    const bool incompatible=s.epoch!=observation_.engineEpoch||s.profileId!=observation_.profileId||s.profileVersion!=observation_.profileVersion||s.sampleRate!=observation_.sampleRate;
    const bool gap=lastEnd_>0 && s.sampleStart>lastEnd_;
    if(incompatible||gap) begin(s);
    lastSequence_=s.sequence;lastEnd_=s.sampleEnd;lastUpdate_=now;
    observation_.id=++nextId_;observation_.transportKnown=s.transportKnown;observation_.transportPlaying=s.transportPlaying;
    observation_.signalActive=s.valid&&s.signalActive;
    if(!s.valid) {observation_.valid=false;return;}
    // Silence retains the last useful observation. It never becomes a fabricated
    // sequence of zeros, and freshness still ages from the last active frame.
    if(!s.signalActive) return;
    lastActive_=now;
    frames_[position_]={s.sampleStart,s.sampleEnd,s.metrics,s.bands};position_=(position_+1)%capacity;count_=std::min(capacity,count_+1);
    const double windowSamples=profile(s.profileId).observationSeconds*s.sampleRate;
    while(count_>0)
    {
        const auto oldest=(position_+capacity-count_)%capacity;
        if(static_cast<double>(s.sampleEnd-frames_[oldest].start)<=windowSamples+.5) break;
        --count_;
    }
    summarize();
}
MetricObservation BufferHunter::summarizeMetric(std::size_t metric,bool spectrum) const noexcept
{
    MetricObservation result;
    std::array<double,capacity> values {};
    double sx=0,sy=0,sxx=0,sxy=0,syy=0;
    for(std::size_t i=0;i<count_;++i)
    {
        const auto& f=frames_[(position_+capacity-count_+i)%capacity];
        const auto& v=spectrum ? f.bands[metric] : f.metrics[metric];
        if(!v.valid||!std::isfinite(v.value)) continue;
        values[result.count++]=v.value;result.latest=v.value;
        result.coverageSeconds+=static_cast<double>(f.end-f.start)/observation_.sampleRate;
        const double x=static_cast<double>(f.end-observation_.sampleStart)/observation_.sampleRate;
        sx+=x;sy+=v.value;sxx+=x*x;sxy+=x*v.value;syy+=v.value*v.value;
    }
    if(result.count==0) return result;
    std::sort(values.begin(),values.begin()+static_cast<std::ptrdiff_t>(result.count));
    const auto quantile=[&](double p){const double t=p*static_cast<double>(result.count-1);const auto lo=static_cast<std::size_t>(t);const auto hi=std::min(lo+1,result.count-1);return values[lo]+(values[hi]-values[lo])*(t-static_cast<double>(lo));};
    result.valid=true;result.typical=quantile(.5);result.low=quantile(.1);result.high=quantile(.9);result.minimum=values[0];result.maximum=values[result.count-1];
    const double n=static_cast<double>(result.count),xx=sxx-sx*sx/n,xy=sxy-sx*sy/n;
    if(n>=30&&result.coverageSeconds>=5&&xx>0)
    {
        const double slope=xy/xx,residual=std::max(0.0,syy-sy*sy/n-slope*xy);
        // Serial correlation makes a naive per-frame confidence interval optimistic.
        // Require a full publication step as well as a conservative 3-sigma slope.
        const double error=3*std::sqrt(residual/std::max(1.0,n/10-2)/xx);
        const double precision=std::pow(10.0,-(spectrum?0:metricDefinitions[metric].decimals));
        result.trend=std::abs(slope)>error&&std::abs(slope)*result.coverageSeconds>=precision ? (slope>0?Trend::rising:Trend::falling) : Trend::stable;
    }
    return result;
}
void BufferHunter::summarize() noexcept
{
    if(count_==0)return;
    observation_.sampleStart=frames_[(position_+capacity-count_)%capacity].start;
    observation_.sampleEnd=frames_[(position_+capacity-1)%capacity].end;
    observation_.durationSeconds=0;observation_.correlationBelowZeroSeconds=0;
    for(std::size_t i=0;i<count_;++i)
    {
        const auto& f=frames_[(position_+capacity-count_+i)%capacity];
        const double duration=static_cast<double>(f.end-f.start)/observation_.sampleRate;
        observation_.durationSeconds+=duration;
        if(f.metrics[index(MetricId::correlation)].valid&&f.metrics[index(MetricId::correlation)].value<0)observation_.correlationBelowZeroSeconds+=duration;
    }
    for(std::size_t i=0;i<metricCount;++i)observation_.metrics[i]=summarizeMetric(i,false);
    for(std::size_t i=0;i<30;++i)observation_.bands[i]=summarizeMetric(i,true);
    // Programme measures retain their actual latest programme value, not a
    // median of integrated loudness/LRA readings over the observation section.
    for(auto id:{MetricId::integrated,MetricId::lra,MetricId::truePeak})
        observation_.metrics[index(id)].typical=observation_.metrics[index(id)].latest;
    observation_.valid=true;
    observation_.sufficient=observation_.durationSeconds+1e-6>=profile(observation_.profileId).observationSeconds;
}
ObservationSnapshot BufferHunter::snapshot(double now) const noexcept
{
    auto out=observation_;
    out.ageSeconds=lastActive_<0 ? 0 : std::max(0.0,now-lastActive_);
    out.fresh=lastActive_>=0&&out.ageSeconds<=1&&lastUpdate_>=0&&now-lastUpdate_<=1;
    if(lastUpdate_<0||now-lastUpdate_>1) out.signalActive=false;
    return out;
}
}
