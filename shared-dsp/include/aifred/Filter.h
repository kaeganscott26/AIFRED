#pragma once
#include "BufferHunter.h"
#include <string>

namespace aifred::core
{
enum class Relationship { unavailable, noReference, insufficient, inside, below, above };
enum class ReferenceCompatibility { noReference, compatible, unavailable, schemaMismatch, profileMismatch, sampleRateMismatch };
struct ReferenceDistribution
{
    std::uint32_t schema=schemaVersion,profileVersion=1;
    ProfileId profileId=ProfileId::mixBalanced;
    std::string id;
    bool available=false;
    double sampleRate=0;
    std::array<MetricObservation,metricCount> metrics {};
    std::array<MetricObservation,30> bands {};
};
struct FilteredMetric
{
    std::string_view name,displayName,unit,definition;
    MetricObservation observation;
    Relationship reference=Relationship::noReference;
    MetricValue referenceLow,referenceHigh;
    double centreHz=0,lowerHz=0,upperHz=0;
    std::string_view region;
    int decimals=0;
    MetricSource source=MetricSource::observed;
    ProfileId emphasizedBy=ProfileId::mixBalanced;
};
struct FilteredMixContext
{
    std::string_view schema="aifred.filtered-mix.v1";
    ObservationSnapshot observation;
    std::array<FilteredMetric,metricCount> metrics {};
    std::array<FilteredMetric,30> bands {};
    std::string referenceId;
    bool referenceCompatible=false;
    ReferenceCompatibility referenceCompatibility=ReferenceCompatibility::noReference;
};
class Filter
{
public:
    static FilteredMixContext apply(const ObservationSnapshot&,const ReferenceDistribution* reference=nullptr);
    static std::string_view frequencyRegion(double hz) noexcept;
    static double published(double value,int decimals) noexcept;
};
}
