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

juce::String jsonEscape(juce::String text) {
  text = text.replace("\\", "\\\\").replace("\"", "\\\"");
  text = text.replace("\r", "").replace("\n", "\\n");
  return text;
}

juce::String extractResponse(juce::String json) {
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

#if JUCE_WINDOWS
bool healthRequest() {
  bool ok = false;
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
      }
      WinHttpCloseHandle(request);
    }
    WinHttpCloseHandle(connect);
  }
  WinHttpCloseHandle(session);
  return ok;
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
#else
bool healthRequest() { return false; }
juce::String chatRequest(const juce::String&, const juce::String&) { return {}; }
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
    const auto ok = healthRequest();
    available_.store(ok);
    {
      const juce::ScopedLock lock(statusLock_);
      lastStatus_ = ok ? "Local AI ready." : "Local AI route unavailable.";
    }
    requestInFlight_.store(false);
  }).detach();
}

void AifredEngineClient::askAsync(juce::String prompt, juce::String contextJson) {
  bool expected = false;
  if (!chatInFlight_.compare_exchange_strong(expected, true)) return;
  {
    const juce::ScopedLock lock(statusLock_);
    lastResponse_ = "Reading current mix snapshot...";
  }
  std::thread([this, prompt = std::move(prompt), contextJson = std::move(contextJson)] {
    const auto response = chatRequest(prompt, contextJson);
    {
      const juce::ScopedLock lock(statusLock_);
      if (response.isNotEmpty()) {
        lastResponse_ = response;
        lastStatus_ = "Local AI ready.";
        available_.store(true);
      } else {
        lastResponse_ = "Local model response was unavailable for this request.";
        lastStatus_ = "AI chat request failed.";
      }
    }
    chatInFlight_.store(false);
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
