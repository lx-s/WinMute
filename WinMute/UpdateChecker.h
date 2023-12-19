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

#pragma once

#include "WMSettings.h"

enum class UpdateCheckInterval : int {
   DISABLED = 0,
   ON_STARTUP = 1,
};

struct UpdateVersionInfo {
   std::wstring version;
   std::wstring downloadUrl;
   bool shouldUpdate = false;
};

struct UpdateInfo {
   std::wstring currentVersion;
   UpdateVersionInfo stable;
   UpdateVersionInfo beta;
};

class UpdateChecker {
public:
   UpdateChecker();
   ~UpdateChecker();

   bool IsUpdateCheckEnabled(const WMSettings &settings) const;
   bool GetUpdateInfo(UpdateInfo& updateInfo) const;
private:
   UpdateChecker(const UpdateChecker &) = delete;
   UpdateChecker &operator=(const UpdateChecker &) = delete;
   UpdateChecker(const UpdateChecker &&) = delete;
   UpdateChecker &operator=(const UpdateChecker &&) = delete;

   bool GetVersionFile(std::string &fileContents) const;
   bool ParseVersionFile(const std::string &fileContents, UpdateInfo& updateInfo) const;
   std::optional<bool> IsVersionGreater(const std::wstring &newVers, const std::wstring &curVers) const;
   bool ParseVersion(const std::wstring &vers, std::vector<int>& parsedVers) const;
   bool ParseVersionInfo(const nlohmann::json &json, UpdateVersionInfo &versInfo) const;
};
