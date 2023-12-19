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

#include "common.h"

enum MuteEndPointMode {
   MUTE_ENDPOINT_MODE_INDIVIDUAL_ALLOW_LIST = 0,
   MUTE_ENDPOINT_MODE_INDIVIDUAL_BLOCK_LIST = 1
};

enum class SettingsKey {
     SETTINGS_VERSION
   , MUTE_ON_LOCK
   , MUTE_ON_DISPLAYSTANDBY
   , MUTE_ON_RDP
   , RESTORE_AUDIO
   , MUTE_ON_SUSPEND
   , MUTE_ON_SHUTDOWN
   , MUTE_ON_LOGOUT
   , MUTE_ON_BLUETOOTH
   , MUTE_ON_BLUETOOTH_DEVICELIST
   , MUTE_ON_WLAN
   , MUTE_ON_WLAN_ALLOWLIST
   , MUTE_INDIVIDUAL_ENDPOINTS
   // 1 = mute specific (allowlist), 2 = mute specific (blocklist)
   , MUTE_INDIVIDUAL_ENDPOINTS_MODE
   , MUTE_DELAY
   , QUIETHOURS_ENABLE
   , QUIETHOURS_FORCEUNMUTE
   , QUIETHOURS_NOTIFICATIONS
   , QUIETHOURS_START
   , QUIETHOURS_END
   , NOTIFICATIONS_ENABLED
   , LOGGING_ENABLED
   , APP_LANGUAGE
   , CHECK_FOR_UPDATE
   , CHECK_FOR_BETA_UPDATE
};

class WMSettings {
public:
   WMSettings();
   ~WMSettings();

   WMSettings(const WMSettings&) = delete;
   WMSettings& operator=(const WMSettings&) = delete;

   bool Init();
   void Unload() noexcept;

   bool IsAutostartEnabled();
   void EnableAutostart(bool enable);

   bool StoreWifiNetworks(std::vector<std::wstring>& networks);
   std::vector<std::wstring> GetWifiNetworks() const;

   bool StoreBluetoothDevices(std::vector<std::wstring>& networks);
   std::vector<std::wstring> GetBluetoothDevicesW() const;
   std::vector<std::string> GetBluetoothDevicesA() const;

   bool StoreManagedAudioEndpoints(std::vector<std::wstring> &endpoints);
   std::vector<std::wstring> GetManagedAudioEndpoints() const;

   DWORD QueryValue(SettingsKey key) const;
   bool  SetValue(SettingsKey key, DWORD value);

   std::optional<std::wstring> QueryStrValue(SettingsKey key) const;
   bool SetValue(SettingsKey key, const std::wstring &value);

private:
   HKEY hSettingsKey_;
   HKEY hWifiKey_;
   HKEY hBluetoothKey_;
   HKEY hAudioEndpointsKey_;

   bool MigrateSettings();
   HKEY OpenAutostartKey(REGSAM samDesired);
};
