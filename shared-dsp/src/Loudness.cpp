#include "aifred/Loudness.h"
#include <algorithm>
#include <numbers>

namespace aifred::core
{
namespace
{
double lufs(double power) noexcept { return power>0 ? -0.691+10*std::log10(power) : -std::numeric_limits<double>::infinity(); }
double cellLufs(std::size_t i) noexcept { return -70+static_cast<double>(i)*.01; }
std::size_t thresholdCell(double threshold) noexcept
{
    return static_cast<std::size_t>(std::clamp(std::ceil((threshold+70)*100),0.0,20001.0));
}
}
void Loudness::prepare(double rate) noexcept
{
    blocks_.fill(0); position_=filled_=0; integrated_.counts.fill(0); integrated_.energy.fill(0); integrated_.count=0; integrated_.totalEnergy=0; integrated_.overflow=false; range_.counts.fill(0); range_.energy.fill(0); range_.count=0; range_.totalEnergy=0; range_.overflow=false;
    // BS.1770 K weighting: bilinear high shelf followed by RLB high pass.
    const double k=std::tan(std::numbers::pi*1681.974450955533/rate);
    const double q=.7071752369554196, vh=std::pow(10.0,3.999843853973347/20), vb=std::pow(vh,.4996667741545416);
    const double a0=1+k/q+k*k;
    for (auto& f:shelf_) f={ (vh+vb*k/q+k*k)/a0, 2*(k*k-vh)/a0, (vh-vb*k/q+k*k)/a0,
                            2*(k*k-1)/a0, (1-k/q+k*k)/a0,0,0 };
    const double h=std::tan(std::numbers::pi*38.13547087602444/rate), hq=.5003270373238773;
    const double h0=1+h/hq+h*h;
    for (auto& f:highpass_) f={1,-2,1,2*(h*h-1)/h0,(1-h/hq+h*h)/h0,0,0};
}
double Loudness::weight(double x,std::size_t channel) noexcept { return highpass_[channel].process(shelf_[channel].process(x)); }
void Loudness::Histogram::add(double power) noexcept
{
    const double level=lufs(power);
    if (level < -70) return;
    if (!std::isfinite(level) || level>130) { overflow=true; return; }
    const auto bin=static_cast<std::size_t>(std::clamp(std::round((level+70)*100),0.0,20000.0));
    ++counts[bin]; energy[bin]+=power; ++count; totalEnergy+=power;
}
MetricValue Loudness::Histogram::integrated() const noexcept
{
    if (count==0 || overflow) return {};
    const auto first=thresholdCell(std::max(-70.0,lufs(totalEnergy/static_cast<double>(count))-10));
    double sum=0; std::uint64_t n=0;
    for (auto i=first;i<cells;++i) { sum+=energy[i]; n+=counts[i]; }
    return n ? MetricValue{lufs(sum/static_cast<double>(n)),true} : MetricValue{};
}
MetricValue Loudness::Histogram::range() const noexcept
{
    if (count<2 || overflow) return {};
    const auto first=thresholdCell(std::max(-70.0,lufs(totalEnergy/static_cast<double>(count))-20));
    std::uint64_t n=0;
    for (auto i=first;i<cells;++i) n+=counts[i];
    if(n<2) return {};
    const auto lowRank=static_cast<std::uint64_t>(std::llround(static_cast<double>(n-1)*.10));
    const auto highRank=static_cast<std::uint64_t>(std::llround(static_cast<double>(n-1)*.95));
    std::uint64_t seen=0; double low=0;
    bool found=false;
    for(auto i=first;i<cells;++i)
    {
        seen+=counts[i];
        if(!found && seen>lowRank) {low=cellLufs(i);found=true;}
        if(seen>highRank) return {cellLufs(i)-low,true};
    }
    return {};
}
void Loudness::add100ms(double power,EngineSnapshot& out) noexcept
{
    blocks_[position_]=power; position_=(position_+1)%blocks_.size(); filled_=std::min(blocks_.size(),filled_+1);
    double momentary=0,shortTerm=0;
    for(std::size_t i=0;i<filled_;++i)
    {
        const double e=blocks_[(position_+blocks_.size()-1-i)%blocks_.size()];
        shortTerm+=e; if(i<4) momentary+=e;
    }
    if(filled_>=4)
    {
        momentary/=4; out.get(MetricId::momentary)={lufs(momentary),true};
        integrated_.add(momentary); out.get(MetricId::integrated)=integrated_.integrated();
    }
    if(filled_==30)
    {
        shortTerm/=30; out.get(MetricId::shortTerm)={lufs(shortTerm),true};
        range_.add(shortTerm); out.get(MetricId::lra)=range_.range();
    }
}
}

