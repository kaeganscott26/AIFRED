#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <numbers>

namespace aifred::core
{
// Causal windowed-sinc waveform reconstruction. 64 input taps, 4 phases below
// 96 kHz, 2 below 192 kHz, sample evaluation at 192 kHz. Delay is 32 input samples.
class TruePeak
{
public:
    void prepare(double rate) noexcept
    {
        samples_={}; position_=0; maximum_=0;
        phases_=rate<96000 ? 4 : rate<192000 ? 2 : 1;
        for(int phase=0;phase<phases_;++phase)
        {
            double sum=0;
            for(std::size_t k=0;k<taps;++k)
            {
                const double x=static_cast<double>(k)-32+static_cast<double>(phase)/phases_;
                const double sinc=std::abs(x)<1e-12 ? 1 : std::sin(std::numbers::pi*x)/(std::numbers::pi*x);
                const double window=.42+.5*std::cos(std::numbers::pi*x/32)+.08*std::cos(2*std::numbers::pi*x/32);
                coefficients_[static_cast<std::size_t>(phase)][k]=std::abs(x)<=32 ? sinc*window : 0;
                sum+=coefficients_[static_cast<std::size_t>(phase)][k];
            }
            for(auto& c:coefficients_[static_cast<std::size_t>(phase)]) c/=sum;
        }
    }
    void push(double left,double right,bool stereo) noexcept
    {
        samples_[0][position_]=left; samples_[1][position_]=right;
        maximum_=std::max(maximum_,std::max(std::abs(left),stereo ? std::abs(right) : 0));
        for(int ch=0;ch<(stereo?2:1);++ch)
            for(int phase=1;phase<phases_;++phase)
            {
                double sample=0;
                for(std::size_t k=0;k<taps;++k)
                    sample+=samples_[static_cast<std::size_t>(ch)][(position_+taps-k)%taps]*coefficients_[static_cast<std::size_t>(phase)][k];
                maximum_=std::max(maximum_,std::abs(sample));
            }
        position_=(position_+1)%taps;
    }
    double maximum() const noexcept {return maximum_;}
private:
    static constexpr std::size_t taps=64;
    std::array<std::array<double,taps>,2> samples_ {};
    std::array<std::array<double,taps>,4> coefficients_ {};
    std::size_t position_=0;
    int phases_=4;
    double maximum_=0;
};
}
