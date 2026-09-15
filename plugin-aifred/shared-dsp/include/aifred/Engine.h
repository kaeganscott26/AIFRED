#pragma once
#include "Loudness.h"
#include "Spectrum.h"
#include "TruePeak.h"
#include <algorithm>
#include <atomic>

namespace aifred::core
{
template<class T,std::size_t Capacity> class SpscQueue
{
public:
    static_assert(Capacity>1);
    static_assert(std::atomic<std::size_t>::is_always_lock_free);
    bool push(const T& value) noexcept
    {
        const auto w=write_.load(std::memory_order_relaxed), next=(w+1)%Capacity;
        if(next==read_.load(std::memory_order_acquire)) return false;
        values_[w]=value; write_.store(next,std::memory_order_release); return true;
    }
    bool pop(T& value) noexcept
    {
        const auto r=read_.load(std::memory_order_relaxed);
        if(r==write_.load(std::memory_order_acquire)) return false;
        value=values_[r]; read_.store((r+1)%Capacity,std::memory_order_release); return true;
    }
private:
    std::array<T,Capacity> values_ {};
    std::atomic<std::size_t> write_ {0};
    std::atomic<std::size_t> read_ {0};
};

class Engine
{
public:
    // Call only while the host has suspended processBlock. Reset/profile requests
    // during processing are applied at the next block boundary by process().
    void prepare(double rate, int channels) noexcept;
    void requestProfile(ProfileId id) noexcept { requestedProfile_.store(profile(id).id,std::memory_order_relaxed); }
    ProfileId requestedProfile() const noexcept {return requestedProfile_.load(std::memory_order_relaxed);}
    void requestReset() noexcept {resetRequested_.store(true,std::memory_order_release);}
    void process(const float* const* channels,int channelCount,int samples,bool transportKnown=false,bool playing=false,
                 std::int64_t transportSample=-1) noexcept;
    bool pop(EngineSnapshot& destination) noexcept {return queue_.pop(destination);}
private:
    struct Block {double l=0,r=0,lr=0,weighted=0,peakL=0,peakR=0;};
    void reset() noexcept;
    void publish() noexcept;
    EngineSnapshot snapshot_ {};
    SpscQueue<EngineSnapshot,8> queue_;
    Spectrum spectrum_;
    Loudness loudness_;
    TruePeak truePeak_;
    std::array<Block,10> blocks_ {};
    Block block_;
    std::size_t blockPosition_=0, filled_=0, tickSamples_=4800, tickPosition_=0;
    std::uint64_t clock_=0, sequence_=0, epoch_=0, dropped_=0;
    double rate_=0;
    int channels_=2;
    std::int64_t expectedTransport_=-1;
    std::atomic<ProfileId> requestedProfile_ {ProfileId::mixBalanced};
    std::atomic<bool> resetRequested_ {false};
};
}

