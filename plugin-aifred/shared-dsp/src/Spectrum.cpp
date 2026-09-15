#include "aifred/Spectrum.h"
#include <algorithm>
#include <numbers>

namespace aifred::core
{
void Spectrum::prepare(double rate, const DspProfile& p) noexcept
{
    const auto& configuration=p.measurement.spectrum;
    rate_ = rate; size_ = configuration.fftSize; hop_ = static_cast<std::size_t>(static_cast<double>(size_) * (1-configuration.overlap));
    position_ = filled_ = sinceHop_ = 0; averaged_ = false;
    left_.fill(0); right_.fill(0); average_.fill(0); peak_.fill(0); hold_.fill(0);
    double windowEnergy = 0;
    for (std::size_t i = 0; i < size_; ++i)
    {
        const double phase = 2 * std::numbers::pi * static_cast<double>(i) / static_cast<double>(size_);
        switch(configuration.window)
        {
            case SpectrumWindow::periodicHann: window_[i] = .5 - .5 * std::cos(phase); break;
        }
        windowEnergy += window_[i]*window_[i];
        roots_[i] = {std::cos(phase), -std::sin(phase)};
    }
    normalization_ = 1 / (static_cast<double>(size_) * windowEnergy);
    const double hopSeconds = static_cast<double>(hop_) / rate;
    averageAlpha_ = std::exp(-hopSeconds/configuration.averageSeconds);
    releaseAlpha_ = std::exp(-hopSeconds/configuration.releaseSeconds);
    holdSeconds_ = configuration.peakHoldSeconds;
}

void Spectrum::transform(std::array<std::complex<double>, maximumFftSize>& data) noexcept
{
    for (std::size_t i=1, j=0; i<size_; ++i)
    {
        auto bit = size_ >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i<j) std::swap(data[i],data[j]);
    }
    for (std::size_t length=2; length<=size_; length <<= 1)
        for (std::size_t start=0; start<size_; start+=length)
            for (std::size_t j=0; j<length/2; ++j)
            {
                const auto even = data[start+j];
                const auto odd = data[start+j+length/2]*roots_[j*(size_/length)];
                data[start+j] = even+odd;
                data[start+j+length/2] = even-odd;
            }
}

std::array<MetricValue,30> Spectrum::extractBands(const double* power, std::size_t count, double width) noexcept
{
    std::array<MetricValue,30> result {};
    if (count<2 || width<=0) return result;
    const double nyquist = static_cast<double>(count-1)*width;
    for (std::size_t b=0; b<bandCentres.size(); ++b)
    {
        const double lo = b==0 ? bandCentres[0]*std::sqrt(bandCentres[0]/bandCentres[1])
                              : std::sqrt(bandCentres[b-1]*bandCentres[b]);
        const double hi = b+1==bandCentres.size() ? bandCentres[b]*std::sqrt(bandCentres[b]/bandCentres[b-1])
                                                : std::sqrt(bandCentres[b]*bandCentres[b+1]);
        // A partial region is unavailable; silently truncating it would change its definition.
        if (hi>nyquist) continue;
        double energy=0;
        for (std::size_t k=0; k<count; ++k)
        {
            const double centre=static_cast<double>(k)*width;
            const double a=std::max(0.0,centre-width/2), z=std::min(nyquist,centre+width/2);
            const double overlap=std::max(0.0,std::min(z,hi)-std::max(a,lo));
            if (z>a) energy+=power[k]*overlap/(z-a);
        }
        result[b]=powerDb(energy);
    }
    return result;
}

void Spectrum::push(double left, double right, bool stereo, EngineSnapshot& out) noexcept
{
    left_[position_]=left; right_[position_]=right; position_=(position_+1)%size_;
    filled_=std::min(size_,filled_+1); ++sinceHop_;
    if (filled_<size_ || sinceHop_<hop_) return;
    sinceHop_=0;
    const auto count=size_/2+1;
    power_.fill(0);
    for (int channel=0; channel<(stereo ? 2 : 1); ++channel)
    {
        const auto& samples=channel==0 ? left_ : right_;
        for (std::size_t i=0; i<size_; ++i) fft_[i]={samples[(position_+i)%size_]*window_[i],0};
        transform(fft_);
        for (std::size_t k=0; k<count; ++k)
            power_[k]+=std::norm(fft_[k])*normalization_*(k==0 || k+1==count ? 1 : 2)/(stereo ? 2 : 1);
    }
    out.fftSize=size_; out.binCount=count; out.binWidthHz=rate_/static_cast<double>(size_); ++out.spectrumSequence;
    for (std::size_t k=0; k<count; ++k)
    {
        average_[k]=averaged_ ? averageAlpha_*average_[k]+(1-averageAlpha_)*power_[k] : power_[k];
        if (power_[k]>=peak_[k]) { peak_[k]=power_[k]; hold_[k]=holdSeconds_; }
        else if (hold_[k]>0) hold_[k]-=static_cast<double>(hop_)/rate_;
        else peak_[k]=std::max(power_[k],peak_[k]*releaseAlpha_);
        out.spectrumPower[k]=power_[k]; out.averagePower[k]=average_[k]; out.peakPower[k]=peak_[k];
    }
    averaged_=true;
    out.bands=extractBands(power_.data(),count,out.binWidthHz);
}
}
