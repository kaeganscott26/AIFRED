#include "aifred/Filter.h"

namespace aifred::core
{
std::string_view Filter::frequencyRegion(double hz) noexcept
{
    if(hz<60)return "SUB";
    if(hz<250)return "BASS";
    if(hz<2000)return "MID";
    if(hz<6000)return "PRESENCE";
    if(hz<12000)return "HIGH";
    return "AIR";
}
double Filter::published(double value,int decimals) noexcept
{
    const double scale=std::pow(10.0,decimals);return std::round(value*scale)/scale;
}
FilteredMixContext Filter::apply(const ObservationSnapshot& o,const ReferenceDistribution* reference)
{
    FilteredMixContext context;context.observation=o;
    context.referenceCompatible=reference&&reference->available&&reference->schema==o.schema&&reference->profileId==o.profileId&&reference->profileVersion==o.profileVersion&&reference->sampleRate==o.sampleRate;
    if(reference)context.referenceId=reference->id.substr(0,128);
    const auto relate=[&](FilteredMetric& metric,const MetricObservation* r)
    {
        if(!reference){metric.reference=Relationship::noReference;return;}
        if(!context.referenceCompatible||!r||!r->valid){metric.reference=Relationship::unavailable;return;}
        if(!o.sufficient||!o.fresh||!metric.observation.valid){metric.reference=Relationship::insufficient;return;}
        metric.referenceLow={r->low,true};metric.referenceHigh={r->high,true};
        metric.reference=metric.observation.typical<r->low?Relationship::below:metric.observation.typical>r->high?Relationship::above:Relationship::inside;
    };
    for(std::size_t i=0;i<metricCount;++i)
    {
        auto& m=context.metrics[i];const auto& d=metricDefinitions[i];
        m.name=d.name;m.unit=d.unit;m.definition=d.definition;m.decimals=d.decimals;m.observation=o.metrics[i];
        relate(m,reference?&reference->metrics[i]:nullptr);
    }
    for(std::size_t i=0;i<30;++i)
    {
        auto& b=context.bands[i];b.name="band_energy";b.unit="dBFS";b.definition="integrated mean-channel FFT power over geometric midpoint region";
        b.observation=o.bands[i];b.centreHz=bandCentres[i];b.region=frequencyRegion(b.centreHz);
        b.lowerHz=i==0?bandCentres[0]*std::sqrt(bandCentres[0]/bandCentres[1]):std::sqrt(bandCentres[i-1]*bandCentres[i]);
        b.upperHz=i==29?bandCentres[29]*std::sqrt(bandCentres[29]/bandCentres[28]):std::sqrt(bandCentres[i]*bandCentres[i+1]);
        relate(b,reference?&reference->bands[i]:nullptr);
    }
    return context;
}
}
