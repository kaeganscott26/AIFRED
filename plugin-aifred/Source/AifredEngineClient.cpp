#include "AifredEngineClient.h"

#if JUCE_WINDOWS
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

#include <thread>

namespace aifred {
namespace {

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
#else
bool healthRequest() { return false; }
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
      lastStatus_ = ok ? "AI engine ready." : "AI engine unavailable - core analysis still active.";
    }
    requestInFlight_.store(false);
  }).detach();
}

juce::String AifredEngineClient::statusText() const {
  const juce::ScopedLock lock(statusLock_);
  return lastStatus_;
}

} // namespace aifred
