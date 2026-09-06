#include "aifred/Pipeline.h"

namespace aifred::core
{
namespace
{
juce::String text(std::string_view s) {return juce::String::fromUTF8(s.data(),static_cast<int>(s.size()));}
juce::var number(double value,int decimals) {return std::isfinite(value)?juce::var(Filter::published(value,decimals)):juce::var();}
juce::var metricJson(const FilteredMetric& m)
{
    auto* obj=new juce::DynamicObject;juce::var value(obj);
    obj->setProperty("metric",text(m.name));obj->setProperty("display_name",text(m.displayName));obj->setProperty("unit",text(m.unit));obj->setProperty("definition",text(m.definition));
    obj->setProperty("context_value_source","observed");
    obj->setProperty("frontend_live_source",m.source==MetricSource::live);
    obj->setProperty("emphasized_by_profile",text(profile(m.emphasizedBy).name));
    obj->setProperty("available",m.observation.valid);obj->setProperty("publication_decimals",m.decimals);
    obj->setProperty("typical",m.observation.valid?number(m.observation.typical,m.decimals):juce::var());
    obj->setProperty("observed_p10",m.observation.valid?number(m.observation.low,m.decimals):juce::var());
    obj->setProperty("observed_p90",m.observation.valid?number(m.observation.high,m.decimals):juce::var());
    obj->setProperty("minimum",m.observation.valid?number(m.observation.minimum,m.decimals):juce::var());
    obj->setProperty("maximum",m.observation.valid?number(m.observation.maximum,m.decimals):juce::var());
    obj->setProperty("coverage_seconds",number(m.observation.coverageSeconds,1));obj->setProperty("count",static_cast<int>(m.observation.count));
    constexpr std::array<const char*,4> trends {"unavailable","stable","rising","falling"};
    obj->setProperty("trend",trends[static_cast<std::size_t>(m.observation.trend)]);
    constexpr std::array<const char*,6> relations {"unavailable","no_reference_available","insufficient_observation","inside_reference_distribution","below_reference_distribution","above_reference_distribution"};
    obj->setProperty("reference_relationship",relations[static_cast<std::size_t>(m.reference)]);
    const auto semantic=m.reference==Relationship::inside?"inside_reference_distribution":
        m.reference==Relationship::below||m.reference==Relationship::above?"outside_reference_distribution":
        m.reference==Relationship::insufficient?"insufficient_observation":"unavailable";
    obj->setProperty("semantic_state",semantic);
    obj->setProperty("reference_low",m.referenceLow.valid?number(m.referenceLow.value,m.decimals):juce::var());
    obj->setProperty("reference_high",m.referenceHigh.valid?number(m.referenceHigh.value,m.decimals):juce::var());
    obj->setProperty("standard_relationship","unavailable");
    if(m.centreHz>0)
    {
        obj->setProperty("centre_hz",m.centreHz);obj->setProperty("lower_hz",number(m.lowerHz,2));obj->setProperty("upper_hz",number(m.upperHz,2));obj->setProperty("region",text(m.region));
    }
    return value;
}
}
juce::var filteredContextJson(const FilteredMixContext& c)
{
    auto* obj=new juce::DynamicObject;juce::var result(obj);const auto& o=c.observation;
    obj->setProperty("schema",text(c.schema));obj->setProperty("shared_core_version",text(coreVersion));
    obj->setProperty("observation_id",juce::String(o.id));obj->setProperty("observation_epoch",juce::String(o.epoch));obj->setProperty("engine_epoch",juce::String(o.engineEpoch));
    const auto& activeProfile=profile(o.profileId);
    obj->setProperty("profile_id",text(activeProfile.name));obj->setProperty("profile_version",static_cast<int>(o.profileVersion));
    obj->setProperty("measurement_configuration_id",text(activeProfile.identity));
    obj->setProperty("profile_schema_version",static_cast<int>(profileSchemaVersion));
    obj->setProperty("rms_window_seconds",activeProfile.measurement.metering.rmsSeconds);
    obj->setProperty("stereo_window_seconds",activeProfile.measurement.metering.stereoSeconds);
    obj->setProperty("lra_provisional",o.sampleRate<=0||static_cast<double>(o.sampleEnd)/o.sampleRate<60);
    obj->setProperty("sample_rate_hz",o.sampleRate);obj->setProperty("sample_start",juce::String(o.sampleStart));obj->setProperty("sample_end",juce::String(o.sampleEnd));
    obj->setProperty("observation_seconds",number(o.durationSeconds,1));obj->setProperty("age_seconds",number(o.ageSeconds,1));
    obj->setProperty("available",o.valid);obj->setProperty("fresh",o.fresh);obj->setProperty("signal_active",o.signalActive);obj->setProperty("sufficient_observation",o.sufficient);
    obj->setProperty("transport_known",o.transportKnown);obj->setProperty("transport_playing",o.transportKnown?juce::var(o.transportPlaying):juce::var());
    obj->setProperty("correlation_below_zero_seconds",number(o.correlationBelowZeroSeconds,1));
    obj->setProperty("reference_id",text(c.referenceId));obj->setProperty("reference_compatible",c.referenceCompatible);
    constexpr std::array<const char*,6> compatibility {"no_reference","compatible","reference_unavailable","schema_mismatch","profile_mismatch","sample_rate_mismatch"};
    obj->setProperty("reference_compatibility",compatibility[static_cast<std::size_t>(c.referenceCompatibility)]);
    obj->setProperty("observation_state",!o.valid?"unavailable":!o.signalActive?"signal_inactive":!o.sufficient?"insufficient_observation":"available");
    juce::Array<juce::var> metrics,bands;
    for(const auto& m:c.metrics)metrics.add(metricJson(m));for(const auto& b:c.bands)bands.add(metricJson(b));
    obj->setProperty("metrics",metrics);obj->setProperty("bands",bands);return result;
}
Pipeline::Pipeline(std::string channel,std::string version)
    :engine_(std::make_unique<Engine>()),channel_(std::move(channel)),version_(std::move(version)),instanceId_(juce::Uuid().toString().toStdString()),sessionId_(juce::Uuid().toString().toStdString())
{startTimer(20);}
Pipeline::~Pipeline(){stopTimer();}
void Pipeline::hiResTimerCallback()
{
    std::lock_guard lock(mutex_);
    // Exactly seven usable slots: no draining loop can be extended indefinitely
    // by a concurrent producer. Neither this mutex nor JSON reaches the producer.
    for(int i=0;i<7;++i)
    {
        const auto previousEpoch=live_.epoch;
        if(!engine_->pop(live_))break;
        if(live_.epoch!=previousEpoch)
            juce::Logger::writeToLog("AIFRED " + text(channel_) + " profile=" + text(profile(live_.profileId).name)
                + " revision=" + juce::String(live_.profileVersion) + " epoch=" + juce::String(live_.epoch));
        hunter_.consume(live_,now());
    }
}
EngineSnapshot Pipeline::live() const {std::lock_guard lock(mutex_);return live_;}
ObservationSnapshot Pipeline::observation() const {std::lock_guard lock(mutex_);return hunter_.snapshot(now());}
juce::String Pipeline::contextForQuestion(const juce::String& question,const ReferenceDistribution* reference,juce::String mode,const ObservationSnapshot* compare)
{
    std::lock_guard lock(mutex_);
    auto context=filteredContextJson(Filter::apply(hunter_.snapshot(now()),mode=="reference"?reference:nullptr));auto* obj=context.getDynamicObject();
    obj->setProperty("product_channel",text(channel_));obj->setProperty("product_version",text(version_));obj->setProperty("plugin_instance_id",text(instanceId_));obj->setProperty("session_id",text(sessionId_));obj->setProperty("mode",mode);
    juce::Array<juce::var> history;for(const auto& item:history_)history.add(item);obj->setProperty("session_context",history);
    if(mode=="compare"&&compare)obj->setProperty("compare_b",filteredContextJson(Filter::apply(*compare)));
    auto* entry=new juce::DynamicObject;juce::var record(entry);
    entry->setProperty("observation",filteredContextJson(Filter::apply(hunter_.snapshot(now()))));
    entry->setProperty("user_statement",question.substring(0,2048));entry->setProperty("action_provenance","user_statement_not_verified_daw_action");
    history_.push_back(record);while(history_.size()>4)history_.pop_front();
    return juce::JSON::toString(context,true);
}
void Pipeline::recordResponse(const juce::String& response)
{
    std::lock_guard lock(mutex_);if(!history_.empty())history_.back().getDynamicObject()->setProperty("assistant_response",response.substring(0,4096));
}
}
