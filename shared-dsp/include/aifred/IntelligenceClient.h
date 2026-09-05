#pragma once
#include <juce_core/juce_core.h>
#include <atomic>
#include <array>
#include <mutex>
#include <thread>

namespace aifred::core
{
struct HostHealth
{
    bool backendAvailable=false,providerAvailable=false;
    std::string provider,model,status;
    std::uint64_t revision=0;
};
struct HostReply
{
    bool success=false;
    std::string response,error;
    std::uint64_t revision=0;
};
class IntelligenceClient
{
public:
    explicit IntelligenceClient(juce::String channel):channel_(std::move(channel)){}
    ~IntelligenceClient();
    bool pingHealthAsync();
    bool askAsync(juce::String question,juce::String filteredContext);
    void saveSettingsAsync(juce::String provider,juce::String endpoint,juce::String apiKey,juce::String model);
    HostHealth health() const {std::lock_guard lock(mutex_);return health_;}
    HostReply lastChatResult() const {std::lock_guard lock(mutex_);return reply_;}
    bool healthInFlight() const noexcept {return healthBusy_;}
    bool chatInFlight() const noexcept {return chatBusy_;}
    bool isAvailable() const {return health().providerAvailable;}
    bool hasPendingChat() const noexcept {return chatInFlight();}
    juce::String statusText() const {return juce::String(health().status);}
    juce::String lastResponse() const {const auto r=lastChatResult();return juce::String(r.success?r.response:r.error);}
private:
    juce::var request(std::size_t slot,const juce::String& path,juce::var body,std::stop_token);
    juce::String channel_;
    mutable std::mutex mutex_;
    HostHealth health_;
    HostReply reply_;
    std::atomic<bool> healthBusy_=false,chatBusy_=false,settingsBusy_=false;
    std::array<std::shared_ptr<juce::WebInputStream>,3> streams_;
    std::array<std::jthread,3> workers_;
};
}
