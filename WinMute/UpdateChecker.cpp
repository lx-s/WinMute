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

#ifdef _DEBUG
const wchar_t *UPDATE_FILE_URL = L"https://raw.githubusercontent.com/lx-s/WinMute/dev/CURRENT_VERSION";
#else
const wchar_t *UPDATE_FILE_URL = L"https://raw.githubusercontent.com/lx-s/WinMute/main/CURRENT_VERSION";
#endif

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

UpdateChecker::UpdateChecker()
{
}

UpdateChecker::~UpdateChecker()
{
}

bool UpdateChecker::ParseVersionInfo(const nlohmann::json &json, UpdateVersionInfo &versInfo) const
{
   WMLog &log = WMLog::GetInstance();
   if (!json.contains("version") && !json.contains("downloadUrl")) {
      log.LogError(L"Missing json fields in update file");
      return false;
   }
   versInfo.version = ConvertStringToWideString(json["version"]);;
   versInfo.downloadUrl = ConvertStringToWideString(json["downloadUrl"]);
   return true;
}

bool UpdateChecker::ParseVersionFile(
   const std::string &fileContents,
   UpdateInfo &updateInfo) const
{
   // Structure
   // {
   //    "version": 1,
   //       "stable" : {
   //       "version": "2.4.1.0",
   //          "download_page" : "https://github.com/lx-s/WinMute/releases/"
   //    },
   //       "beta" : {
   //       "version": "2.4.1.0",
   //          "download_page" : "https://github.com/lx-s/WinMute/releases/"
   //    }
   // }
   WMLog &log = WMLog::GetInstance();
   try {
      auto json = nlohmann::json::parse(fileContents);
      if (!json.contains("version")) {
         log.LogError(L"Missing update-file version");
         return false;
      }
      int version = json["version"].get<int>();
      if (version != 1) {
         log.LogError(L"Update file has incompatible version number");
         return false;
      }
      if (!json.contains("stable") || !json.contains("beta")) {
         log.LogError(L"Missing stable and/or beta fields");
         return false;
      }
      const auto stableInfo = json["stable"];
      const auto betaInfo = json["beta"];

      if (!stableInfo.is_structured() || !betaInfo.is_structured()) {
         log.LogError(L"Missing stable and/or beta fields");
         return false;
      }

      if (!ParseVersionInfo(stableInfo, updateInfo.stable) ||
          !ParseVersionInfo(betaInfo, updateInfo.beta)) {
         log.LogError(L"Failed to parse stable and/or beta fields");
         return false;
      }

   } catch (const nlohmann::json::parse_error &pe) {
      log.LogError(L"Failed to parse update: %S", pe.what());
      return false;
   }
   return true;
}

bool UpdateChecker::ParseVersion(const std::wstring &vers, std::vector<int>& parsedVers) const
{
   size_t lastVersionPos = 0;
   size_t curPos = 0;
   for (; curPos <= vers.length(); ++curPos) {
      if (vers[curPos] == L'.' || vers[curPos] == L'\0') {
         try {
            const std::wstring versPart = vers.substr(lastVersionPos, curPos - lastVersionPos);
            parsedVers.push_back(std::stoi(versPart));
         } catch (...) {
            return false;
         }
         lastVersionPos = curPos + 1;
      }
   }
   return true;
}

std::optional<bool> UpdateChecker::IsVersionGreater(const std::wstring &newVers, const std::wstring &curVers) const
{
   std::vector<int> newVersParsed;
   std::vector<int> curVersParsed;
   WMLog &log = WMLog::GetInstance();
   if (!ParseVersion(newVers, newVersParsed)) {
      log.LogError(L"Failed to parse new version string \"%s\"", newVers.c_str());
      return std::nullopt;
   } else if (!ParseVersion(curVers, curVersParsed)) {
      log.LogError(L"Failed to parse current version string \"%s\"", curVers.c_str());
      return std::nullopt;
   } else if (newVersParsed.size() != curVersParsed.size()) {
      log.LogError(L"Version format mismatch \"%s\" / \"%s\"", curVers.c_str(), newVers.c_str());
      return std::nullopt;
   }
   for (size_t i = 0; i < newVersParsed.size(); ++i) {
      if (curVersParsed[i] < newVersParsed[i]) {
         return true;
      }
   }
   return false;
}

bool UpdateChecker::GetUpdateInfo(UpdateInfo &updateInfo) const
{
   if (!GetWinMuteVersion(updateInfo.currentVersion)) {
      return false;
   }

   std::string versionFile;
   if (!GetVersionFile(versionFile)) {
      return false;
   }

   if (!ParseVersionFile(versionFile, updateInfo)) {
      return false;
   }

   const auto shouldUpdateStable = IsVersionGreater(updateInfo.stable.version, updateInfo.currentVersion);
   const auto shouldUpdateBeta = IsVersionGreater(updateInfo.beta.version, updateInfo.currentVersion);
   if (!shouldUpdateStable || !shouldUpdateBeta) {
      return false;
   }
   updateInfo.stable.shouldUpdate = *shouldUpdateStable;
   updateInfo.beta.shouldUpdate = *shouldUpdateBeta;
   return true;
}

bool UpdateChecker::GetVersionFile(std::string &fileContents) const
{
   WMLog &log = WMLog::GetInstance();

   URL_COMPONENTS updateUrl{0};
   updateUrl.dwStructSize = sizeof(updateUrl);
   updateUrl.dwSchemeLength = (DWORD)-1;
   updateUrl.dwHostNameLength = (DWORD)-1;
   updateUrl.dwUrlPathLength = (DWORD)-1;
   if (!WinHttpCrackUrl(UPDATE_FILE_URL, 0, 0, &updateUrl)) {
      log.LogWinError(L"WinHttpCrackUrl", GetLastError());
      return false;
   }

   HInternetHolder hSession = WinHttpOpen(
      L"WinMute",
      WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
      WINHTTP_NO_PROXY_NAME,
      WINHTTP_NO_PROXY_BYPASS,
      WINHTTP_FLAG_SECURE_DEFAULTS);
   if (hSession == nullptr) {
      log.LogWinError(L"WinHttpOpen", GetLastError());
      return false;
   }

   const std::wstring updateHost(
      updateUrl.lpszHostName,
      updateUrl.lpszHostName + updateUrl.dwHostNameLength);
   HInternetHolder hConnect = WinHttpConnect(
      hSession,
      updateHost.c_str(),
      (updateUrl.nScheme == INTERNET_SCHEME_HTTPS) ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT,
      0);
   if (hConnect == nullptr) {
      log.LogWinError(L"WinHttpConnect", GetLastError());
      return false;
   }

   const std::wstring updateFilePath(
      updateUrl.lpszUrlPath,
      updateUrl.lpszUrlPath + updateUrl.dwUrlPathLength);
   HInternetHolder hRequest = WinHttpOpenRequest(
      hConnect,
      L"GET",
      updateFilePath.c_str(),
      nullptr,
      WINHTTP_NO_REFERER,
      WINHTTP_DEFAULT_ACCEPT_TYPES,
      WINHTTP_FLAG_REFRESH | WINHTTP_FLAG_SECURE);

   if (hRequest == nullptr) {
      log.LogWinError(L"WinHttpConnect", GetLastError());
      return false;
   }

   bool result = WinHttpSendRequest(
      hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
      WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

   if (!result) {
      log.LogWinError(L"WinHttpSendRequest", GetLastError());
      return false;
   }

   result = WinHttpReceiveResponse(hRequest, nullptr);
   if (!result) {
      log.LogWinError(L"WinHttpReceiveResponse", GetLastError());
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
      log.LogWinError(L"WinHttpQueryHeaders(WINHTTP_QUERY_STATUS_CODE)", GetLastError());
      return false;
   } else if (statusCode != HTTP_STATUS_OK) {
      log.LogError(L"Update-Server returned HTTP %d", HTTP_STATUS_OK);
      return false;
   }

   fileContents.clear();
   for (;;) {
      DWORD availBytes = 0;
      if (!WinHttpQueryDataAvailable(hRequest, &availBytes)) {
         log.LogWinError(L"WinHttpQueryDataAvailable", GetLastError());
         return false;
      }
      if (availBytes == 0) {
         break;
      }
      std::string chunk(availBytes, ' ');
      DWORD byRead = 0;
      if (!WinHttpReadData(hRequest, reinterpret_cast<LPVOID>(&chunk[0]), availBytes, &byRead)) {
         log.LogWinError(L"WinHttpReadData", GetLastError());
         return false;
      }
      fileContents.append(chunk.begin(), chunk.begin() + byRead);
   }

   return true;
}

bool UpdateChecker::IsUpdateCheckEnabled(const WMSettings &settings) const
{
   const auto updateCheckSetting = settings.QueryValue(SettingsKey::CHECK_FOR_UPDATE);
   if (updateCheckSetting == static_cast<int>(UpdateCheckInterval::DISABLED)) {
      return false;
   }

   wchar_t thisExePath[MAX_PATH * 2];
   const DWORD thisExePathSize = ARRAY_SIZE(thisExePath);
   const DWORD thisPathLen = GetModuleFileNameW(nullptr, thisExePath, thisExePathSize);
   if (thisPathLen == 0 || (thisPathLen == thisExePathSize && GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
      WMLog::GetInstance().LogWinError(L"GetModuleFileNameW", ERROR_INSUFFICIENT_BUFFER);
      return false;
   }
   fs::path updateDisableFile{ thisExePath };
   updateDisableFile.replace_filename(L"update-check-disabled");
   if (fs::exists(updateDisableFile)) {
      return false;
   }

   return true;
}
