#include "aifred/Engine.h"
#include "aifred/Filter.h"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <numbers>
#include <random>
#include <thread>

using namespace aifred::core;
namespace
{
int checks=0;
void require(bool value,const char* description)
{
    ++checks; if(!value) {std::cerr<<"FAIL: "<<description<<'\n';std::exit(1);}
}
void near(double value,double expected,double tolerance,const char* description)
{
    if(!std::isfinite(value)||std::abs(value-expected)>tolerance)
        std::cerr<<description<<": got "<<value<<", expected "<<expected<<'\n';
    require(std::isfinite(value)&&std::abs(value-expected)<=tolerance,description);
}
struct Fixture
{
    std::unique_ptr<Engine> engine=std::make_unique<Engine>();
    EngineSnapshot latest;
    double rate=48000;
    std::uint64_t clock=0;
    Fixture() {engine->prepare(rate,2);}
    void tone(double seconds,double amplitude,double frequency=1000,double rightScale=1,int blockSize=257)
    {
        std::array<float,2048> l {},r {};
        const float* data[]={l.data(),r.data()};
        auto remaining=static_cast<int>(std::round(seconds*rate));
        while(remaining>0)
        {
            const auto n=std::min(remaining,blockSize);
            for(int i=0;i<n;++i)
            {
                const auto v=amplitude*std::sin(2*std::numbers::pi*frequency*static_cast<double>(clock++)/rate);
                l[static_cast<std::size_t>(i)]=static_cast<float>(v);r[static_cast<std::size_t>(i)]=static_cast<float>(v*rightScale);
            }
            engine->process(data,2,n);
            while(engine->pop(latest)) {}
            remaining-=n;
        }
    }
};
}
int main()
{
    const auto start=std::chrono::steady_clock::now();
    const auto& mix=profile(ProfileId::mixBalanced);
    const auto& surgical=profile(ProfileId::spectrumSurgical);
    const auto& mastering=profile(ProfileId::masteringPrecision);
    const auto& stereoProfile=profile(ProfileId::stereoPhase);
    require(profileSchemaVersion==2,"typed profile schema revision");
    require(profileFromName("UNKNOWN")==ProfileId::mixBalanced,"unknown profile falls back to mix balanced");
    require(mix.measurement.spectrum.fftSize==2048&&mix.measurement.observation.durationSeconds==15,"mix balanced moderate defaults");
    require(mix.cpuCost==CpuCost::moderate&&mix.reactionSpeed==ReactionSpeed::balanced,"mix balanced cost and response");
    require(surgical.measurement.spectrum.fftSize==maximumFftSize,"surgical maximum FFT resolution");
    require(surgical.measurement.spectrum.averageSeconds>mix.measurement.spectrum.averageSeconds&&surgical.presentation.showPeakTrace,"surgical stable average and peak trace");
    require(mastering.metrics.isRequired(MetricId::truePeak)&&mastering.metrics.isRequired(MetricId::momentary)&&mastering.metrics.isRequired(MetricId::shortTerm)&&mastering.metrics.isRequired(MetricId::integrated)&&mastering.metrics.isRequired(MetricId::lra),"mastering required loudness and true-peak suite");
    require(mastering.measurement.observation.durationSeconds==25&&mastering.measurement.metering.shortTermSeconds==3,"mastering observation is separate from programme windows");
    require(stereoProfile.measurement.metering.stereoSeconds==.1&&stereoProfile.measurement.metering.rmsSeconds==.4,"stereo diagnostic fast stereo and standard RMS");
    require(stereoProfile.metrics.isRequired(MetricId::correlation)&&stereoProfile.metrics.isRequired(MetricId::width),"stereo diagnostic required metrics");
    for(const auto& configured:profiles)
    {
        require(configured.measurement.spectrum.window==SpectrumWindow::periodicHann,"profile window is explicit periodic Hann");
        require(configured.measurement.spectrum.overlap==.75&&configured.measurement.snapshotHz==10,"profile overlap and publication cadence");
        require(configured.metrics.enabled==allMetrics,"all core engineering metrics remain enabled");
        require(configured.presentation.spectrumRange==SpectrumDisplayRange::db96,"professional default spectrum viewport");
    }
    auto f=std::make_unique<Fixture>();
    f->tone(.3,.5);
    require(!f->latest.get(MetricId::rms).valid,"RMS full-window warmup");
    require(!f->latest.get(MetricId::momentary).valid,"400 ms momentary warmup");
    f->tone(.1,.5);
    near(f->latest.get(MetricId::samplePeak).value,-6.0206,.001,"half-scale sample peak");
    near(f->latest.get(MetricId::rms).value,-9.0309,.001,"sine RMS");
    near(f->latest.get(MetricId::crest).value,3.0103,.001,"paired broadband crest");
    near(f->latest.get(MetricId::correlation).value,1,1e-10,"mono correlation");
    near(f->latest.get(MetricId::width).value,0,1e-10,"mono side share");
    require(!f->latest.get(MetricId::shortTerm).valid,"short-term requires 3 s");
    f->tone(2.6,.5);
    near(f->latest.get(MetricId::shortTerm).value,-6.02,.12,"1 kHz stereo K-weighted loudness");
    near(f->latest.get(MetricId::integrated).value,-6.02,.12,"integrated energy and channel weighting");
    require(f->latest.binCount==1025,"2048 FFT retains 1025 bins");
    const auto peak=std::max_element(f->latest.spectrumPower.begin(),f->latest.spectrumPower.begin()+static_cast<std::ptrdiff_t>(f->latest.binCount));
    near(static_cast<double>(peak-f->latest.spectrumPower.begin())*f->latest.binWidthHz,1000,f->latest.binWidthHz,"FFT frequency mapping");
    double energy=0;for(std::size_t i=0;i<f->latest.binCount;++i)energy+=f->latest.spectrumPower[i];
    near(10*std::log10(energy),-9.0309,.01,"Parseval spectral power");
    require(bandCentres[16]==850,"850 Hz telemetry centre");
    f->tone(1,0);
    require(!f->latest.signalActive,"silence is inactive");
    require(std::isinf(f->latest.get(MetricId::rms).value),"silence RMS is negative infinity");
    require(!f->latest.get(MetricId::correlation).valid,"silence correlation unavailable");
    f->engine->requestReset();f->tone(.4,1.1);
    require(f->latest.sampleClipActive&&f->latest.sampleClipCount>0,"unrounded sample overs");
    f->engine->requestReset();f->tone(1,.4,1000,-1);
    near(f->latest.get(MetricId::correlation).value,-1,1e-10,"anti-phase correlation");
    near(f->latest.get(MetricId::width).value,100,1e-8,"anti-phase M/S");
    double antiEnergy=0;for(std::size_t i=0;i<f->latest.binCount;++i)antiEnergy+=f->latest.spectrumPower[i];
    require(antiEnergy>.07,"anti-phase spectrum does not cancel");
    f->engine->requestReset();f->tone(1,.4,1000,.5);
    near(f->latest.get(MetricId::balance).value,-6.0206,.001,"L/R power balance");
    f->engine->requestReset();f->tone(1,.8,12000);
    // A quarter-rate sine at pi/4 phase has sample maxima A/sqrt(2), with reconstructed A.
    auto tp=std::make_unique<TruePeak>();tp->prepare(48000);
    double samplePeak=0;
    for(int i=0;i<4800;++i)
    {
        const double x=.9*std::sin(std::numbers::pi*.5*i+std::numbers::pi/4);
        samplePeak=std::max(samplePeak,std::abs(x));tp->push(x,x,true);
    }
    require(tp->maximum()>samplePeak*1.35,"true peak distinguishes intersample peak");
    near(20*std::log10(tp->maximum()),20*std::log10(.9),.3,"reconstructed peak accuracy");
    const auto previousEpoch=f->latest.epoch;
    f->engine->requestProfile(ProfileId::spectrumSurgical);f->tone(1,.1,850);
    require(f->latest.epoch>previousEpoch&&f->latest.profileId==ProfileId::spectrumSurgical,"profile starts epoch");
    require(f->latest.binCount==4097,"surgical hero has 4097 bins");
    require(f->latest.bands[16].value>f->latest.bands[15].value,"850 Hz band integration");
    f->engine->requestReset();f->tone(1,.00001,850);
    require(f->latest.bands[16].value < -90,"analytical spectrum not GUI-clamped");
    f->engine->requestProfile(ProfileId::masteringPrecision);f->tone(20,.1);f->tone(20,std::pow(10,-30.0/20));
    near(f->latest.get(MetricId::lra).value,10,1,"EBU Tech 3342 two-plateau LRA");
    const double integrated=f->latest.get(MetricId::integrated).value;
    f->tone(5,0);
    near(f->latest.get(MetricId::integrated).value,integrated,.15,"integrated gates silence");
    for(double rate:{32000.0,44100.0,96000.0,192000.0})
    {
        f->rate=rate;f->clock=0;f->engine->prepare(rate,2);f->tone(3,.1,1000);
        near(f->latest.get(MetricId::shortTerm).value,-20,.15,"sample-rate K weighting");
    }
    f->rate=48000; f->engine->prepare(f->rate,2);
    f->engine->requestProfile(ProfileId::stereoPhase);
    f->tone(.1,.5);
    near(f->latest.get(MetricId::correlation).value,1,1e-10,"diagnostic stereo warmup 100 ms");
    require(!f->latest.get(MetricId::rms).valid,"fast stereo preserves 400 ms RMS warmup");
    f->tone(.1,.5,1000,-1);
    near(f->latest.get(MetricId::correlation).value,-1,1e-10,"phase reversal resolves in 100 ms");
    near(f->latest.get(MetricId::width).value,100,1e-10,"live side share follows phase reversal");
    require(f->latest.profileVersion==2,"changed stereo integration has a new profile revision");
    // Fixed independent pseudo-random streams test neutrality without a platform-dependent distribution.
    std::mt19937 leftNoise(12345),rightNoise(67890);
    std::array<float,480> noiseL {},noiseR {};
    const float* noise[]={noiseL.data(),noiseR.data()};
    for(int block=0;block<100;++block)
    {
        for(std::size_t i=0;i<noiseL.size();++i)
        {
            noiseL[i]=static_cast<float>(static_cast<double>(leftNoise())/4294967295.0-.5);
            noiseR[i]=static_cast<float>(static_cast<double>(rightNoise())/4294967295.0-.5);
        }
        f->engine->process(noise,2,480);
        while(f->engine->pop(f->latest)) {}
    }
    near(f->latest.get(MetricId::correlation).value,0,.05,"unrelated signals have neutral correlation");
    near(f->latest.get(MetricId::width).value,50,3,"unrelated signals have equal mid/side share");
    auto transport=std::make_unique<Fixture>();
    noiseL.fill(.1f);noiseR.fill(.1f);
    std::int64_t position=0;
    const auto transportBlocks=[&](bool playing,int count)
    {
        for(int i=0;i<count;++i)
        {
            transport->engine->process(noise,2,480,true,playing,position);
            if(playing)position+=480;
            while(transport->engine->pop(transport->latest)) {}
        }
    };
    transportBlocks(true,40);
    const auto transportEpoch=transport->latest.epoch;
    noiseL.fill(0);noiseR.fill(0);transportBlocks(false,10);
    require(transport->latest.epoch==transportEpoch&&!transport->latest.transportPlaying,"stop retains engine epoch");
    noiseL.fill(.1f);noiseR.fill(.1f);transportBlocks(true,10);
    require(transport->latest.epoch==transportEpoch&&transport->latest.transportPlaying,"resume retains compatible programme");
    position+=96000;transportBlocks(true,10);
    require(transport->latest.epoch>transportEpoch,"major transport seek starts epoch");
    transport->engine->requestReset();const auto seekEpoch=transport->latest.epoch;transportBlocks(true,10);
    require(transport->latest.epoch>seekEpoch,"manual reset starts epoch");
    auto hunter=std::make_unique<BufferHunter>();
    auto measured=std::make_unique<EngineSnapshot>();
    measured->sampleRate=48000;measured->epoch=1;measured->valid=measured->signalActive=true;
    for(std::uint64_t i=1;i<=200;++i)
    {
        measured->sequence=i;measured->sampleStart=(i-1)*4800;measured->sampleEnd=i*4800;
        measured->get(MetricId::rms)={-52,true};measured->get(MetricId::shortTerm)={-11+static_cast<double>(i%3)-1,true};
        measured->get(MetricId::correlation)={-.5,true};hunter->consume(*measured,static_cast<double>(i)/10);
    }
    auto observed=hunter->snapshot(20);
    require(observed.sufficient&&observed.fresh,"observation sufficiency and freshness");
    near(observed.durationSeconds,15,1e-6,"bounded sample-time observation duration");
    near(observed.get(MetricId::shortTerm).typical,-11,.01,"median typical loudness");
    near(observed.correlationBelowZeroSeconds,15,1e-6,"negative correlation persistence");
    require(hunter->storedFrames()==150,"bounded rolling storage");
    for(const auto& configured:profiles)
    {
        BufferHunter profileHunter;EngineSnapshot frame;frame.sampleRate=48000;frame.epoch=1;frame.profileId=configured.id;
        frame.profileVersion=configured.version;frame.valid=frame.signalActive=true;frame.get(MetricId::rms)={-20,true};
        const auto frameCount=static_cast<std::uint64_t>(configured.measurement.observation.durationSeconds*10+5);
        for(std::uint64_t i=1;i<=frameCount;++i)
        {
            frame.sequence=i;frame.sampleStart=(i-1)*4800;frame.sampleEnd=i*4800;profileHunter.consume(frame,static_cast<double>(i)/10);
        }
        const auto profileObservation=profileHunter.snapshot(static_cast<double>(frameCount)/10);
        require(profileObservation.sufficient,"profile observation reaches configured sufficiency");
        near(profileObservation.durationSeconds,configured.measurement.observation.durationSeconds,1e-6,"profile observation duration is enforced");
    }
    hunter->consume(*measured,20.1);
    require(hunter->snapshot(20.1).id==observed.id,"repeated snapshot is not re-counted");
    measured->sequence++;measured->sampleStart=measured->sampleEnd;measured->sampleEnd+=4800;measured->signalActive=false;
    hunter->consume(*measured,20.1);
    require(hunter->snapshot(20.1).get(MetricId::rms).typical==-52,"silence retains observation");
    require(!hunter->snapshot(22).fresh&&!hunter->snapshot(22).signalActive,"halted host becomes stale");
    const auto retainedEpoch=hunter->snapshot(22).epoch;
    measured->sequence++;measured->sampleStart=measured->sampleEnd;measured->sampleEnd+=4800;measured->signalActive=true;
    measured->transportKnown=measured->transportPlaying=true;hunter->consume(*measured,22.1);
    require(hunter->snapshot(22.1).fresh&&hunter->snapshot(22.1).epoch==retainedEpoch,"resume refreshes compatible retained observation");
    auto context=Filter::apply(observed);
    require(context.metrics[index(MetricId::rms)].observation.typical<0,"negative dBFS ordering");
    require(context.metrics[index(MetricId::shortTerm)].unit=="LUFS","LUFS remains LUFS");
    require(context.metrics[index(MetricId::truePeak)].unit=="dBTP","true peak unit");
    require(context.metrics[index(MetricId::crest)].centreHz==0,"broadband crest has no frequency band");
    require(context.bands[25].region=="AIR"&&context.bands[0].region=="SUB","frequency semantics");
    require(context.metrics[0].reference==Relationship::noReference,"no invented reference target");
    ReferenceDistribution reference;reference.available=true;reference.sampleRate=48000;reference.profileId=ProfileId::masteringPrecision;
    require(!Filter::apply(observed,&reference).referenceCompatible,"incompatible profile reference unavailable");
    require(Filter::apply(observed,&reference).referenceCompatibility==ReferenceCompatibility::profileMismatch,"profile incompatibility is explicit");
    reference.profileId=observed.profileId;reference.metrics[index(MetricId::rms)]={true,-52,-55,-50};
    require(Filter::apply(observed,&reference).metrics[index(MetricId::rms)].reference==Relationship::inside,"compatible distribution comparison");
    measured->epoch++;measured->profileId=ProfileId::spectrumSurgical;measured->sequence++;measured->signalActive=true;measured->sampleStart=0;measured->sampleEnd=4800;
    hunter->consume(*measured,22);
    require(!hunter->snapshot(22).sufficient&&hunter->storedFrames()==1,"profile epoch has no incompatible history");
    measured->sequence++;measured->sampleRate=44100;hunter->consume(*measured,22.1);
    require(hunter->storedFrames()==1,"sample-rate transition resets observation");
    near(Filter::published(-10.719438876073538,0),-11,0,"human-scale loudness precision");
    near(Filter::published(-.84,1),-.8,1e-9,"true peak tenth precision");
    SpscQueue<std::uint64_t,8> queue;
    for(std::uint64_t i=1;i<=7;++i)require(queue.push(i),"bounded queue usable capacity");
    require(!queue.push(8),"full queue rejects without overwriting or waiting");
    std::uint64_t retained=0;
    for(std::uint64_t i=1;i<=7;++i)require(queue.pop(retained)&&retained==i,"full queue preserves published order");
    require(!queue.pop(retained),"empty queue returns immediately");
    std::atomic<bool> complete=false;
    std::jthread producer([&]{for(std::uint64_t i=1;i<=100000;++i) {while(!queue.push(i))std::this_thread::yield();}complete=true;});
    std::uint64_t expected=1,value=0;
    while(!complete||expected<=100000) if(queue.pop(value)) {require(value==expected,"SPSC ordering");++expected;}
    std::cout<<"PASS "<<checks<<" checks; seconds="<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<'\n';
}
