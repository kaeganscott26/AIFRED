#include "aifred/Engine.h"

namespace aifred::core
{
void Engine::prepare(double rate,int channels) noexcept
{
    rate_=std::isfinite(rate) && rate>=32000 && rate<=192000 && std::fmod(rate,10)==0 ? rate : 0;
    channels_=channels==1 ? 1 : 2;
    reset();
}
void Engine::reset() noexcept
{
    ++epoch_; clock_=0; expectedTransport_=-1; blocks_={}; block_={}; filled_=blockPosition_=tickPosition_=0;
    snapshot_={}; snapshot_.epoch=epoch_; snapshot_.sampleRate=rate_; snapshot_.profileId=requestedProfile();
    snapshot_.profileVersion=profile(snapshot_.profileId).version;
    tickSamples_=static_cast<std::size_t>(rate_>0 ? rate_/profile(snapshot_.profileId).measurement.snapshotHz : 4800);
    if(rate_>0)
    {
        spectrum_.prepare(rate_,profile(snapshot_.profileId)); loudness_.prepare(rate_); truePeak_.prepare(rate_);
    }
    snapshot_.sequence=++sequence_;
    if(!queue_.push(snapshot_)) ++dropped_;
}
void Engine::process(const float* const* data,int channels,int samples,bool known,bool playing,std::int64_t position) noexcept
{
    if(resetRequested_.exchange(false,std::memory_order_acq_rel) || requestedProfile()!=snapshot_.profileId) reset();
    if(rate_<=0 || data==nullptr || samples<=0 || channels<1) return;
    if(channels!=channels_) {channels_=channels==1?1:2;reset();}
    // A seek/loop larger than one second starts a new programme epoch. Ordinary
    // stop/resume at the expected position retains the accumulated programme.
    if(known && playing && position>=0 && expectedTransport_>=0 && std::abs(position-expectedTransport_)>static_cast<std::int64_t>(rate_)) reset();
    if(known && playing && position>=0) expectedTransport_=position+samples;
    snapshot_.transportKnown=known; snapshot_.transportPlaying=playing;
    for(int i=0;i<samples;++i)
    {
        const double l=data[0][i],r=channels_>1 ? data[1][i] : l;
        if(!std::isfinite(l)||!std::isfinite(r)) {reset();return;}
        block_.l+=l*l; block_.r+=r*r; block_.lr+=l*r;
        block_.peakL=std::max(block_.peakL,std::abs(l)); block_.peakR=std::max(block_.peakR,std::abs(r));
        const double kl=loudness_.weight(l,0),kr=channels_>1 ? loudness_.weight(r,1) : 0;
        block_.weighted+=kl*kl+kr*kr;
        if(std::abs(l)>=1) ++snapshot_.sampleClipCount;
        if(channels_>1 && std::abs(r)>=1) ++snapshot_.sampleClipCount;
        truePeak_.push(l,r,channels_>1); spectrum_.push(l,r,channels_>1,snapshot_);
        const auto scopeIndex=(clock_/16)%snapshot_.vectorscope.size();
        if(clock_%16==0) snapshot_.vectorscope[scopeIndex]={static_cast<float>(l),static_cast<float>(r)};
        snapshot_.vectorscopeCount=std::min(snapshot_.vectorscope.size(),static_cast<std::size_t>(clock_/16+1));
        ++clock_;
        if(++tickPosition_==tickSamples_) {publish();tickPosition_=0;block_={};}
    }
}
void Engine::publish() noexcept
{
    blocks_[blockPosition_]=block_; blockPosition_=(blockPosition_+1)%blocks_.size(); filled_=std::min(blocks_.size(),filled_+1);
    const auto& configuration=profile(snapshot_.profileId).measurement;
    const auto window=static_cast<std::size_t>(std::round(configuration.metering.rmsSeconds*configuration.snapshotHz));
    Block sum;
    for(std::size_t i=0;i<std::min(window,filled_);++i)
    {
        const auto& b=blocks_[(blockPosition_+blocks_.size()-1-i)%blocks_.size()];
        sum.l+=b.l;sum.r+=b.r;sum.lr+=b.lr;sum.peakL=std::max(sum.peakL,b.peakL);sum.peakR=std::max(sum.peakR,b.peakR);
    }
    if(filled_>=window)
    {
        const double n=static_cast<double>(window*tickSamples_),l=sum.l/n,r=sum.r/n;
        const double energy=(l+r)/2;
        const auto peak=std::max(sum.peakL,sum.peakR);
        snapshot_.get(MetricId::samplePeak)=powerDb(peak*peak); snapshot_.get(MetricId::rms)=powerDb(energy);
        snapshot_.channelPeaks={powerDb(sum.peakL*sum.peakL),powerDb(sum.peakR*sum.peakR)};
        snapshot_.get(MetricId::crest)=energy>0 ? MetricValue{10*std::log10(peak*peak/energy),true} : MetricValue{};
    }
    // Stereo integration is independent of the level window. Diagnostic mode
    // resolves a phase change within 100 ms; loudness and RMS keep their definitions.
    const auto stereoWindow=static_cast<std::size_t>(std::round(configuration.metering.stereoSeconds*configuration.snapshotHz));
    if(filled_>=stereoWindow)
    {
        Block stereo;
        for(std::size_t i=0;i<stereoWindow;++i)
        {
            const auto& b=blocks_[(blockPosition_+blocks_.size()-1-i)%blocks_.size()];
            stereo.l+=b.l; stereo.r+=b.r; stereo.lr+=b.lr;
        }
        const double n=static_cast<double>(stereoWindow*tickSamples_);
        const double l=stereo.l/n,r=stereo.r/n,lr=stereo.lr/n;
        const double energy=(l+r)/2,mid=std::max(0.0,(l+r+2*lr)/4),side=std::max(0.0,(l+r-2*lr)/4);
        snapshot_.get(MetricId::correlation)=l>0 && r>0 ? MetricValue{std::clamp(lr/std::sqrt(l*r),-1.0,1.0),true} : MetricValue{};
        snapshot_.get(MetricId::leftEnergy)=powerDb(l); snapshot_.get(MetricId::rightEnergy)=channels_>1?powerDb(r):MetricValue{};
        snapshot_.get(MetricId::midEnergy)=powerDb(mid); snapshot_.get(MetricId::sideEnergy)=powerDb(side);
        snapshot_.get(MetricId::balance)=l>0 && r>0 ? MetricValue{10*std::log10(r/l),true}:MetricValue{};
        snapshot_.get(MetricId::sideToMid)=mid>0 && side>0 ? MetricValue{10*std::log10(side/mid),true}:MetricValue{};
        snapshot_.get(MetricId::width)=energy>0 ? MetricValue{100*side/(mid+side),true}:MetricValue{};
    }
    loudness_.add100ms(block_.weighted/static_cast<double>(tickSamples_),snapshot_);
    snapshot_.get(MetricId::truePeak)=powerDb(truePeak_.maximum()*truePeak_.maximum());
    snapshot_.valid=true; snapshot_.signalActive=std::max(block_.peakL,block_.peakR)>1e-7;
    snapshot_.sampleClipActive=snapshot_.sampleClipCount>0;
    snapshot_.sequence=++sequence_; snapshot_.sampleStart=clock_-tickSamples_; snapshot_.sampleEnd=clock_; snapshot_.droppedPublications=dropped_;
    if(!queue_.push(snapshot_)) ++dropped_;
}
}
