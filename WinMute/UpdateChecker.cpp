/*
 WinMute
           Copyright (c) 2023, Alexander Steinhoefer

-----------------------------------------------------------------------------
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of the author nor the names of its contributors may
      be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------
*/

#include "common.h"

class HInternetHolder {
public:
   HInternetHolder(HINTERNET hInternet)
      : hInternet_(hInternet)
   {}
   ~HInternetHolder()
   {
      if (hInternet_ != nullptr) {
         WinHttpCloseHandle(hInternet_);
      }
   }
   operator HINTERNET()
   {
      return hInternet_;
   }
private:
   HINTERNET hInternet_;
};


UpdateChecker::UpdateChecker(const WMSettings &settings):
   settings_(settings)
{

}

UpdateChecker::~UpdateChecker()
{

}

bool UpdateChecker::ShouldUpdate(std::wstring& , std::wstring& ) const
{
   //if (!IsUpdateCheckEnabled()) {
     // return false;
   //}

   WMLog &log = WMLog::GetInstance();
   HInternetHolder hSession = WinHttpOpen(
      L"WinMute",
      WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
      WINHTTP_NO_PROXY_NAME,
      WINHTTP_NO_PROXY_BYPASS,
      WINHTTP_FLAG_SECURE_DEFAULTS);
   if (hSession == nullptr) {
      log.WriteWindowsError(L"WinHttpOpen", GetLastError());
      return false;
   }

   HInternetHolder hConnect = WinHttpConnect(
      hSession,
      L"raw.githubusercontent.com",
      INTERNET_DEFAULT_HTTPS_PORT,
      0);
   if (hConnect == nullptr) {
      log.WriteWindowsError(L"WinHttpConnect", GetLastError());
      return false;
   }

#ifdef _DEBUG
   const wchar_t *updateFileUrl = L"/lx-s/WinMute/dev/CURRENT_VERSION";
#else
   const wchar_t *updateFileUrl = L"/lx-s/WinMute/main/CURRENT_VERSION";
#endif
   HInternetHolder hRequest = WinHttpOpenRequest(
      hConnect,
      L"GET",
      updateFileUrl,
      nullptr,
      WINHTTP_NO_REFERER,
      WINHTTP_DEFAULT_ACCEPT_TYPES,
      WINHTTP_FLAG_REFRESH | WINHTTP_FLAG_SECURE);

   if (hRequest == nullptr) {
      log.WriteWindowsError(L"WinHttpConnect", GetLastError());
      return false;
   }

   bool result = WinHttpSendRequest(
         hRequest,
         WINHTTP_NO_ADDITIONAL_HEADERS,
         0,
         WINHTTP_NO_REQUEST_DATA,
         0,
         0,
         0);

   if (!result) {
      log.WriteWindowsError(L"WinHttpSendRequest", GetLastError());
      return false;
   }

   result = WinHttpReceiveResponse(hRequest, nullptr);
   if (!result) {
      log.WriteWindowsError(L"WinHttpReceiveResponse", GetLastError());
      return false;
   }

   DWORD statusCode = 0;
   DWORD statusCodeSize = static_cast<DWORD>(sizeof(statusCode));
   result = WinHttpQueryHeaders(
      hRequest,
      WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
      WINHTTP_HEADER_NAME_BY_INDEX,
      reinterpret_cast<LPVOID>(&statusCode),
      &statusCodeSize,
      WINHTTP_NO_HEADER_INDEX);
   if (!result) {
      log.WriteWindowsError(L"WinHttpQueryHeaders(WINHTTP_QUERY_STATUS_CODE)", GetLastError());
      return false;
   } else if (statusCode != HTTP_STATUS_OK) {
      log.Write(L"Update-Server returned HTTP %d", HTTP_STATUS_OK);
      return false;
   }

   std::string versionFileContents;
   for (;;) {
      DWORD availBytes = 0;
      if (!WinHttpQueryDataAvailable(hRequest, &availBytes)) {
         log.WriteWindowsError(L"WinHttpQueryDataAvailable", GetLastError());
         return false;
      }
      if (availBytes == 0) {
         break;
      }
      std::string chunk(availBytes, ' ');
      DWORD byRead = 0;
      if (!WinHttpReadData(hRequest, reinterpret_cast<LPVOID>(&chunk[0]), availBytes, &byRead)) {
         log.WriteWindowsError(L"WinHttpReadData", GetLastError());
         return false;
      }
      versionFileContents.append(chunk.begin(), chunk.begin() + byRead);
   }

   // Check contents of version file
   return true;
}

bool UpdateChecker::IsUpdateCheckEnabled() const
{
   const auto updateCheckSetting = settings_.QueryValue(SettingsKey::CHECK_FOR_UPDATE);
   if (updateCheckSetting == static_cast<int>(UpdateCheckInterval::DISABLED)) {
      return false;
   }

   wchar_t thisExePath[MAX_PATH * 2];
   const DWORD thisExePathSize = ARRAY_SIZE(thisExePath);
   const DWORD thisPathLen = GetModuleFileNameW(nullptr, thisExePath, thisExePathSize);
   if (thisPathLen == 0 || (thisPathLen == thisExePathSize && GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
      WMLog::GetInstance().WriteWindowsError(L"GetModuleFileNameW", ERROR_INSUFFICIENT_BUFFER);
      return false;
   }
   fs::path updateDisableFile{ thisExePath };
   updateDisableFile.replace_filename(L"update-check-disabled");
   if (fs::exists(updateDisableFile)) {
      return false;
   }

   return true;
}
