#include "aifred/IntelligenceClient.h"

namespace aifred::core
{
IntelligenceClient::~IntelligenceClient()
{
    for(auto& worker:workers_)worker.request_stop();
    {std::lock_guard lock(mutex_);for(auto& stream:streams_)if(stream)stream->cancel();}
    for(auto& worker:workers_)if(worker.joinable())worker.join();
}
juce::var IntelligenceClient::request(std::size_t slot,const juce::String& path,juce::var body,std::stop_token token)
{
    const auto origin=channel_=="official"?"http://127.0.0.1:8788":"http://127.0.0.1:8787";
    auto url=juce::URL(juce::String(origin)+path);if(!body.isVoid())url=url.withPOSTData(juce::JSON::toString(body,true));
    auto stream=std::make_shared<juce::WebInputStream>(url,!body.isVoid());
    stream->withConnectionTimeout(180000).withNumRedirectsToFollow(0);
    if(!body.isVoid())stream->withExtraHeaders("Content-Type: application/json\r\n");
    {std::lock_guard lock(mutex_);streams_[slot]=stream;}
    juce::var result;
    if(!token.stop_requested()&&stream->connect(nullptr))
    {
        juce::MemoryOutputStream output;std::array<char,4096> bytes {};
        while(!token.stop_requested()&&!stream->isExhausted()&&output.getDataSize()<524288)
        {
            const auto n=stream->read(bytes.data(),static_cast<int>(bytes.size()));if(n<=0)break;output.write(bytes.data(),static_cast<std::size_t>(n));
        }
        if(output.getDataSize()<524288)result=juce::JSON::parse(output.toString());
    }
    {std::lock_guard lock(mutex_);streams_[slot].reset();}
    return result;
}
bool IntelligenceClient::pingHealthAsync()
{
    if(healthBusy_.exchange(true))return false;
    workers_[0]=std::jthread([this](std::stop_token stop){
        const auto response=request(0,"/health",{},stop);HostHealth next;
        next.backendAvailable=response["host_identity"].toString()=="AifredIntelligenceHost"&&response["product_channel"].toString()==channel_;
        next.providerAvailable=next.backendAvailable&&static_cast<bool>(response["ai_available"]);
        next.provider=response["provider"].toString().toStdString();next.model=response["model_name"].toString().toStdString();
        next.status=next.providerAvailable?"Provider ready":next.backendAvailable?"Provider unavailable":"Intelligence Host unavailable";
        {std::lock_guard lock(mutex_);next.revision=health_.revision+1;health_=std::move(next);}healthBusy_=false;
    });return true;
}
bool IntelligenceClient::askAsync(juce::String question,juce::String context)
{
    const auto parsed=juce::JSON::parse(context);
    if(question.trim().isEmpty()||parsed["schema"].toString()!="aifred.filtered-mix.v1"||parsed["product_channel"].toString()!=channel_)return false;
    if(chatBusy_.exchange(true))return false;
    workers_[1]=std::jthread([this,question=std::move(question),parsed](std::stop_token stop){
        auto* object=new juce::DynamicObject;juce::var body(object);object->setProperty("message",question.substring(0,2048));object->setProperty("context",parsed);
        const auto response=request(1,"/chat",body,stop);HostReply next;
        const bool identity=response["plugin_instance_id"].toString()==parsed["plugin_instance_id"].toString()&&response["session_id"].toString()==parsed["session_id"].toString();
        next.response=response["response"].toString().toStdString();next.success=identity&&!next.response.empty();
        next.error=response["error"].toString().toStdString();if(!next.success&&next.error.empty())next.error="Intelligence Host unavailable or response identity mismatch.";
        {std::lock_guard lock(mutex_);next.revision=reply_.revision+1;reply_=std::move(next);}chatBusy_=false;
    });return true;
}
void IntelligenceClient::saveSettingsAsync(juce::String provider,juce::String endpoint,juce::String apiKey,juce::String model)
{
    if(settingsBusy_.exchange(true))return;
    workers_[2]=std::jthread([this,provider,endpoint,apiKey,model](std::stop_token stop){
        auto* object=new juce::DynamicObject;juce::var body(object);object->setProperty("provider",provider=="compatible"?"openai-compatible":provider);
        object->setProperty("endpoint",endpoint);object->setProperty("api_key",apiKey);object->setProperty("model",model);
        const auto result=request(2,"/v1/settings",body,stop);
        if(!static_cast<bool>(result["ok"])) {std::lock_guard lock(mutex_);health_.status="Settings could not be saved.";++health_.revision;}
        settingsBusy_=false;
    });
}
}
