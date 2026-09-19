#pragma once
#include "Contracts.h"
#include <complex>

namespace aifred::core
{
class Spectrum
{
public:
    void prepare(double rate, const DspProfile& configuration) noexcept;
    void push(double left, double right, bool stereo, EngineSnapshot& destination) noexcept;
    static std::array<MetricValue, 30> extractBands(const double* power, std::size_t count, double binWidth) noexcept;
private:
    void transform(std::array<std::complex<double>, maximumFftSize>& data) noexcept;
    std::array<double, maximumFftSize> left_ {}, right_ {}, window_ {};
    std::array<std::complex<double>, maximumFftSize> fft_ {}, roots_ {};
    std::array<double, maximumBins> power_ {}, average_ {}, peak_ {}, hold_ {};
    std::size_t size_ = 2048, hop_ = 512, position_ = 0, filled_ = 0, sinceHop_ = 0;
    double rate_ = 48000, normalization_ = 1, averageAlpha_ = 0, releaseAlpha_ = 0, holdSeconds_ = 0;
    bool averaged_ = false;
};
}
