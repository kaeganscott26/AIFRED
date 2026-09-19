#pragma once
#include "Contracts.h"

namespace aifred::core
{
class Loudness
{
public:
    void prepare(double sampleRate) noexcept;
    double weight(double sample, std::size_t channel) noexcept;
    void add100ms(double weightedMeanSquare, EngineSnapshot&) noexcept;
private:
    struct Biquad
    {
        double b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
        double process(double x) noexcept
        {
            const auto y=b0*x+z1; z1=b1*x-a1*y+z2; z2=b2*x-a2*y; return y;
        }
    };
    // 0.01 LU histogram cells bound programme storage. Power sums remain full precision.
    struct Histogram
    {
        static constexpr std::size_t cells=20001;
        std::array<std::uint64_t,cells> counts {};
        std::array<double,cells> energy {};
        double totalEnergy=0;
        std::uint64_t count=0;
        bool overflow=false;
        void add(double meanSquare) noexcept;
        MetricValue integrated() const noexcept;
        MetricValue range() const noexcept;
    };
    std::array<Biquad,2> shelf_, highpass_;
    std::array<double,30> blocks_ {};
    std::size_t position_=0, filled_=0;
    Histogram integrated_, range_;
};
}
