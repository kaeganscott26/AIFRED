#include "AifredEngineClient.h"

#if JUCE_WINDOWS
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

#include <thread>
#include <cstring>

namespace aifred {
namespace {

constexpr const char* kGatewayBaseUrl = "http://127.0.0.1:8787";
constexpr int kHealthTimeoutMs = 750;
constexpr int kChatTimeoutMs = 420000;

juce::String jsonEscape(juce::String text) {
  text = text.replace("\\", "\\\\").replace("\"", "\\\"");
  text = text.replace("\r", "").replace("\n", "\\n");
  return text;
}

juce::String extractResponse(juce::String json) {
  auto parsed = juce::JSON::parse(json);
  if (auto* object = parsed.getDynamicObject()) {
    const auto response = object->getProperty("response").toString().trim();
    if (response.isNotEmpty()) return response;
    const auto message = object->getProperty("message").toString().trim();
    if (message.isNotEmpty()) return message;
    const auto answer = object->getProperty("answer").toString().trim();
    if (answer.isNotEmpty()) return answer;
    const auto error = object->getProperty("error").toString().trim();
    if (error.isNotEmpty()) return "AI unavailable: " + error;
  }

  const auto key = json.indexOf("\"response\"");
  if (key < 0) return {};
  auto start = json.indexOfChar(key, ':');
  if (start < 0) return {};
  start = json.indexOfChar(start, '"');
  if (start < 0) return {};
  juce::String out;
  bool escape = false;
  for (int i = start + 1; i < json.length(); ++i) {
    const auto c = json[i];
    if (escape) {
      if (c == 'n') out << "\n";
      else out << c;
      escape = false;
      continue;
    }
    if (c == '\\') {
      escape = true;
      continue;
    }
    if (c == '"') break;
    out << c;
  }
  return out.trim();
}

juce::String healthStatusFromJson(const juce::String& json, bool& localAiReady) {
  localAiReady = false;
  auto parsed = juce::JSON::parse(json);
  if (auto* object = parsed.getDynamicObject()) {
    const auto engineRunning = static_cast<bool>(object->getProperty("engine_running"));
    const auto provider = object->getProperty("provider").toString().trim();
    const auto ollamaReachable = static_cast<bool>(object->getProperty("ollama_reachable"));
    const auto modelPresent = static_cast<bool>(object->getProperty("model_present"));
    const auto modelName = object->getProperty("model_name").toString().trim().isNotEmpty()
      ? object->getProperty("model_name").toString().trim()
      : juce::String("aifred:latest");
    const auto openAiConfigured = static_cast<bool>(object->getProperty("openai_configured"));
    const auto lastError = object->getProperty("last_error").toString().trim();
    localAiReady = static_cast<bool>(object->getProperty("local_ai_ready"));

    if (localAiReady) return "Local AI ready.";
    if (!engineRunning) return "AifredEngine not running.";
    if (provider.containsIgnoreCase("ollama")) {
      if (!ollamaReachable) return "Ollama not running or not reachable.";
      if (!modelPresent) return "AIFRED model missing: " + modelName + ".";
    }
    if (provider.containsIgnoreCase("openai") && !openAiConfigured) {
      return "OpenAI optional; API key not configured.";
    }
    if (lastError.isNotEmpty()) return lastError;
    return "Local AI route unavailable.";
  }
  return "AifredEngine health response invalid.";
}

#if JUCE_WINDOWS
bool healthRequest(juce::String& statusText) {
  bool ok = false;
  juce::String result;
  HINTERNET session = WinHttpOpen(L"AIFRED/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (!session) return false;
  HINTERNET connect = WinHttpConnect(session, L"127.0.0.1", 8787, 0);
  if (connect) {
    HINTERNET request = WinHttpOpenRequest(connect, L"GET", L"/health", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (request) {
      DWORD timeout = 750;
      WinHttpSetOption(request, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
      WinHttpSetOption(request, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
      WinHttpSetOption(request, WINHTTP_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
      if (WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
          && WinHttpReceiveResponse(request, nullptr)) {
        DWORD status = 0;
        DWORD size = sizeof(status);
        WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &status, &size, nullptr);
        ok = status >= 200 && status < 300;
        if (ok) {
          DWORD available = 0;
          do {
            available = 0;
            if (!WinHttpQueryDataAvailable(request, &available) || available == 0) break;
            juce::HeapBlock<char> buffer(available + 1);
            ZeroMemory(buffer.getData(), available + 1);
            DWORD read = 0;
            if (WinHttpReadData(request, buffer.getData(), available, &read) && read > 0) {
              result += juce::String::fromUTF8(buffer.getData(), static_cast<int>(read));
            }
          } while (available > 0);
        }
      }
      WinHttpCloseHandle(request);
    }
    WinHttpCloseHandle(connect);
  }
  WinHttpCloseHandle(session);
  bool localAiReady = false;
  statusText = ok ? healthStatusFromJson(result, localAiReady) : "AifredEngine not running.";
  return ok && localAiReady;
}

juce::String chatRequest(const juce::String& prompt, const juce::String& contextJson) {
  juce::String result;
  HINTERNET session = WinHttpOpen(L"AIFRED/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (!session) return {};
  HINTERNET connect = WinHttpConnect(session, L"127.0.0.1", 8787, 0);
  if (connect) {
    HINTERNET request = WinHttpOpenRequest(connect, L"POST", L"/chat", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (request) {
      DWORD timeout = 420000;
      WinHttpSetOption(request, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
      WinHttpSetOption(request, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
      WinHttpSetOption(request, WINHTTP_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
      const auto body = "{\"message\":\"" + jsonEscape(prompt) + "\",\"context\":" + (contextJson.trim().startsWith("{") ? contextJson : "{}") + "}";
      const auto utf8 = body.toRawUTF8();
      const auto bodyBytes = static_cast<DWORD>(std::strlen(utf8));
      const wchar_t* headers = L"Content-Type: application/json\r\n";
      if (WinHttpSendRequest(request, headers, static_cast<DWORD>(-1), (LPVOID) utf8, bodyBytes, bodyBytes, 0)
          && WinHttpReceiveResponse(request, nullptr)) {
        DWORD status = 0;
        DWORD size = sizeof(status);
        WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &status, &size, nullptr);
        if (status >= 200 && status < 300) {
          DWORD available = 0;
          do {
            available = 0;
            if (!WinHttpQueryDataAvailable(request, &available) || available == 0) break;
            juce::HeapBlock<char> buffer(available + 1);
            ZeroMemory(buffer.getData(), available + 1);
            DWORD read = 0;
            if (WinHttpReadData(request, buffer.getData(), available, &read) && read > 0) {
              result += juce::String::fromUTF8(buffer.getData(), static_cast<int>(read));
            }
          } while (available > 0);
        }
      }
      WinHttpCloseHandle(request);
    }
    WinHttpCloseHandle(connect);
  }
  WinHttpCloseHandle(session);
  return extractResponse(result);
}

bool settingsRequest(const juce::String& body) {
  bool ok = false;
  HINTERNET session = WinHttpOpen(L"AIFRED/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (!session) return false;
  HINTERNET connect = WinHttpConnect(session, L"127.0.0.1", 8787, 0);
  if (connect) {
    HINTERNET request = WinHttpOpenRequest(connect, L"POST", L"/v1/settings", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (request) {
      DWORD timeout = 3000;
      WinHttpSetOption(request, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
      WinHttpSetOption(request, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
      WinHttpSetOption(request, WINHTTP_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
      const auto utf8 = body.toRawUTF8();
      const auto bodyBytes = static_cast<DWORD>(std::strlen(utf8));
      const wchar_t* headers = L"Content-Type: application/json\r\n";
      if (WinHttpSendRequest(request, headers, static_cast<DWORD>(-1), (LPVOID) utf8, bodyBytes, bodyBytes, 0)
          && WinHttpReceiveResponse(request, nullptr)) {
        DWORD status = 0;
        DWORD size = sizeof(status);
        WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &status, &size, nullptr);
        ok = status >= 200 && status < 300;
      }
      WinHttpCloseHandle(request);
    }
    WinHttpCloseHandle(connect);
  }
  WinHttpCloseHandle(session);
  return ok;
}
#else
bool httpRequest(const juce::String& method,
                 const juce::String& path,
                 const juce::String& body,
                 int timeoutMs,
                 juce::String& responseBody) {
  auto url = juce::URL(juce::String(kGatewayBaseUrl) + path);
  if (body.isNotEmpty()) {
    url = url.withPOSTData(body);
  }

  int statusCode = 0;
  auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
    .withConnectionTimeoutMs(timeoutMs)
    .withStatusCode(&statusCode)
    .withHttpRequestCmd(method);

  auto stream = body.isNotEmpty()
    ? url.createInputStream(options.withExtraHeaders("Content-Type: application/json\r\n"))
    : url.createInputStream(options);
  if (stream == nullptr) return false;

  responseBody = stream->readEntireStreamAsString();
  return statusCode >= 200 && statusCode < 300;
}

bool healthRequest(juce::String& statusText) {
  juce::String response;
  const auto ok = httpRequest("GET", "/health", {}, kHealthTimeoutMs, response);
  bool localAiReady = false;
  statusText = ok ? healthStatusFromJson(response, localAiReady) : "AifredEngine not running.";
  return ok && localAiReady;
}

juce::String chatRequest(const juce::String& prompt, const juce::String& contextJson) {
  const auto body = "{\"message\":\"" + jsonEscape(prompt) + "\",\"context\":" + (contextJson.trim().startsWith("{") ? contextJson : "{}") + "}";
  juce::String response;
  if (!httpRequest("POST", "/chat", body, kChatTimeoutMs, response)) return {};
  return extractResponse(response);
}

bool settingsRequest(const juce::String& body) {
  juce::String response;
  return httpRequest("POST", "/v1/settings", body, 3000, response);
}
#endif

} // namespace

AifredEngineClient& AifredEngineClient::instance() {
  static AifredEngineClient client;
  return client;
}

void AifredEngineClient::pingHealthAsync() {
  bool expected = false;
  if (!requestInFlight_.compare_exchange_strong(expected, true)) return;

  std::thread([this] {
    juce::String status;
    const auto ok = healthRequest(status);
    available_.store(ok);
    {
      const juce::ScopedLock lock(statusLock_);
      lastStatus_ = status.isNotEmpty() ? status : (ok ? "Local AI ready." : "Local AI route unavailable.");
    }
    requestInFlight_.store(false);
  }).detach();
}

bool AifredEngineClient::askAsync(juce::String prompt, juce::String contextJson) {
  bool expected = false;
  if (!chatInFlight_.compare_exchange_strong(expected, true)) return false;
  {
    const juce::ScopedLock lock(statusLock_);
    lastResponse_ = "Reading current mix snapshot...";
  }
  std::thread([this, promptText = std::move(prompt), contextText = std::move(contextJson)] {
    const auto response = chatRequest(promptText, contextText);
    {
      const juce::ScopedLock lock(statusLock_);
      if (response.isNotEmpty()) {
        lastResponse_ = response;
        lastStatus_ = "Local AI ready.";
        available_.store(true);
      } else {
        lastResponse_ = "Local model response was unavailable for this request.";
        lastStatus_ = "AI chat request failed.";
        available_.store(false);
      }
    }
    chatInFlight_.store(false);
  }).detach();
  return true;
}

void AifredEngineClient::saveSettingsAsync(juce::String provider, juce::String endpoint, juce::String apiKey, juce::String model) {
  const auto ollamaUrl = provider.containsIgnoreCase("ollama") ? endpoint : juce::String("http://127.0.0.1:11434");
  const auto body = "{\"provider_override_enabled\":true,\"provider_mode\":\"" + jsonEscape(provider)
    + "\",\"api_key\":\"" + jsonEscape(apiKey)
    + "\",\"custom_endpoint\":\"" + jsonEscape(endpoint)
    + "\",\"ollama_url\":\"" + jsonEscape(ollamaUrl)
    + "\",\"model_name\":\"" + jsonEscape(model) + "\"}";

  std::thread([this, body] {
    const auto ok = settingsRequest(body);
    const juce::ScopedLock lock(statusLock_);
    lastStatus_ = ok ? "AI route saved to local engine." : "AI route saved locally; engine settings unavailable.";
    if (ok) available_.store(true);
    else available_.store(false);
  }).detach();
}

juce::String AifredEngineClient::statusText() const {
  const juce::ScopedLock lock(statusLock_);
  return lastStatus_;
}

juce::String AifredEngineClient::lastResponse() const {
  const juce::ScopedLock lock(statusLock_);
  return lastResponse_;
}

} // namespace aifred
