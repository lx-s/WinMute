/*
 WinMute
           Copyright (c) 2025, Alexander Steinhoefer

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

static constexpr int CURRENT_SETTINGS_VERSION = 0;

static const wchar_t *LX_SYSTEMS_SUBKEY
   = L"SOFTWARE\\lx-systems\\WinMute";
static const wchar_t* LX_SYSTEMS_WIFI_SUBKEY
   = L"SOFTWARE\\lx-systems\\WinMute\\WifiNetworks";
static const wchar_t* LX_SYSTEMS_BLUETOOTH_SUBKEY
   = L"SOFTWARE\\lx-systems\\WinMute\\BluetoothDevices";
static const wchar_t *LX_SYSTEMS_AUDIO_ENDPOINTS_SUBKEY
= L"SOFTWARE\\lx-systems\\WinMute\\ManagedAudioEndpoints";

static const wchar_t* LX_SYSTEMS_AUTOSTART_KEY
   = L"LX-Systems WinMute";

extern HINSTANCE hglobInstance;

static const wchar_t* KeyToStr(SettingsKey key)
{
   const wchar_t* keyStr = nullptr;
   switch (key) {
   case SettingsKey::SETTINGS_VERSION:
      keyStr = L"SettingsVersion";
      break;
   case SettingsKey::MUTE_ON_LOCK:
      keyStr = L"MuteOnLock";
      break;
   case SettingsKey::MUTE_ON_DISPLAYSTANDBY:
      keyStr = L"MuteOnDisplayStandby";
      break;
   case SettingsKey::MUTE_ON_RDP:
      keyStr = L"MuteOnRDP";
      break;
   case SettingsKey::RESTORE_AUDIO:
      keyStr = L"RestoreAudio";
      break;
   case SettingsKey::MUTE_ON_SUSPEND:
      keyStr = L"MuteOnSuspend";
      break;
   case SettingsKey::MUTE_ON_SHUTDOWN:
      keyStr = L"MuteOnShutdown";
      break;
   case SettingsKey::MUTE_ON_LOGOUT:
      keyStr = L"MuteOnLogout";
      break;
   case SettingsKey::MUTE_ON_BLUETOOTH:
      keyStr = L"MuteOnBluetooth";
      break;
   case SettingsKey::MUTE_ON_BLUETOOTH_DEVICELIST:
      keyStr = L"MuteOnBluetoothDeviceList";
      break;
   case SettingsKey::MUTE_ON_WLAN:
      keyStr = L"MuteOnWlan";
      break;
   case SettingsKey::MUTE_ON_WLAN_ALLOWLIST:
      keyStr = L"MuteOnWlanAllowList";
      break;
   case SettingsKey::MUTE_INDIVIDUAL_ENDPOINTS:
      keyStr = L"MuteIndividualEndpoints";
      break;
   case SettingsKey::MUTE_INDIVIDUAL_ENDPOINTS_MODE:
      keyStr = L"MuteIndividualEndpointsMode";
      break;
   case SettingsKey::MUTE_DELAY:
      keyStr = L"MuteDelay";
      break;
   case SettingsKey::QUIETHOURS_ENABLE:
      keyStr = L"QuietHoursEnabled";
      break;
   case SettingsKey::QUIETHOURS_FORCEUNMUTE:
      keyStr = L"QuietHoursForceUnmute";
      break;
   case SettingsKey::QUIETHOURS_NOTIFICATIONS:
      keyStr = L"QuietHoursNotifications";
      break;
   case SettingsKey::QUIETHOURS_START:
      keyStr = L"QuietHoursStart";
      break;
   case SettingsKey::QUIETHOURS_END:
      keyStr = L"QuietHoursEnd";
      break;
   case SettingsKey::LOGGING_ENABLED:
      keyStr = L"Logging";
      break;
   case SettingsKey::NOTIFICATIONS_ENABLED:
      keyStr = L"ShowNotifications";
      break;
   case SettingsKey::APP_LANGUAGE:
      keyStr = L"AppLanguage";
      break;
   case SettingsKey::CHECK_FOR_UPDATE:
      keyStr = L"CheckForUpdate";
      break;
   case SettingsKey::CHECK_FOR_BETA_UPDATE:
      keyStr = L"CheckForBetaUpdate";
      break;
   }
   return keyStr;
}

static DWORD GetDefaultSetting(SettingsKey key)
{
   switch (key) {
   case SettingsKey::SETTINGS_VERSION:
      return 0;
   case SettingsKey::MUTE_ON_LOCK:
      return 1;
   case SettingsKey::MUTE_ON_DISPLAYSTANDBY:
      return 1;
   case SettingsKey::MUTE_ON_RDP:
      return 0;
   case SettingsKey::RESTORE_AUDIO:
      return 1;
   case SettingsKey::MUTE_ON_SUSPEND:
      return 0;
   case SettingsKey::MUTE_ON_SHUTDOWN:
      return 0;
   case SettingsKey::MUTE_ON_LOGOUT:
      return 0;
   case SettingsKey::MUTE_ON_WLAN:
      return 0;
   case SettingsKey::MUTE_ON_WLAN_ALLOWLIST:
      return 0;
   case SettingsKey::MUTE_INDIVIDUAL_ENDPOINTS:
      return 0;
   case SettingsKey::MUTE_INDIVIDUAL_ENDPOINTS_MODE:
      return MUTE_ENDPOINT_MODE_INDIVIDUAL_ALLOW_LIST;
   case SettingsKey::MUTE_DELAY:
      return 0;
   case SettingsKey::QUIETHOURS_ENABLE:
      return 0;
   case SettingsKey::QUIETHOURS_FORCEUNMUTE:
      return 0;
   case SettingsKey::QUIETHOURS_NOTIFICATIONS:
      return 0;
   case SettingsKey::QUIETHOURS_START:
      return 0;
   case SettingsKey::QUIETHOURS_END:
      return 0;
   case SettingsKey::LOGGING_ENABLED:
      return 0;
   case SettingsKey::NOTIFICATIONS_ENABLED:
      return 0;
   case SettingsKey::APP_LANGUAGE:
      return 0;
   case SettingsKey::CHECK_FOR_UPDATE:
      return 0;
   case SettingsKey::CHECK_FOR_BETA_UPDATE:
      return 0;
   }
   return 0;
}

template<typename T>
static void NormalizeStringList(std::vector<std::basic_string<T>> &items)
{
   if (items.size() > 1) {
      std::sort(std::begin(items), std::end(items));
      auto it = std::unique(std::begin(items), std::end(items));
      items.resize(std::distance(std::begin(items), it));
   }
}

static bool ReadStringFromRegistry(HKEY hKey, const wchar_t *subKey, std::wstring& val)
{
   bool success = false;
   DWORD regError = 0;
   DWORD bufSize = 0;
   WMLog &log = WMLog::GetInstance();
   regError = RegQueryValueExW(hKey, subKey, nullptr, nullptr, nullptr, &bufSize);
   if (regError != ERROR_SUCCESS) {
      if (regError != ERROR_FILE_NOT_FOUND) {
         ShowWindowsError(L"RegQueryValueExW", regError);
         log.LogError(L"[Registry] Failed to read key \"%s\".", subKey);
      }
   } else {
      bufSize += 1; // Trailing '\0'
      wchar_t *buf = new wchar_t[bufSize];
      regError = RegQueryValueExW(hKey, subKey, nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(buf), &bufSize);
      if (regError != ERROR_SUCCESS) {
         ShowWindowsError(L"RegQueryValueExW", regError);
         log.LogError(L"[Registry] Failed to query value for key \"%s\".", subKey);
      } else {
         success = true;
         val = buf;
      }
      delete [] buf;
   }

   return success;
}

WMSettings::WMSettings() :
   hSettingsKey_(nullptr),
   hWifiKey_(nullptr),
   hBluetoothKey_(nullptr),
   hAudioEndpointsKey_(nullptr)
{
}

WMSettings::~WMSettings()
{
   Unload();
}

bool WMSettings::MigrateSettings()
{
   DWORD version = QueryValue(SettingsKey::SETTINGS_VERSION);
   if (version != CURRENT_SETTINGS_VERSION) {
      WMLog::GetInstance().LogError(L"Unexpected settings version: %d", version);
   }
   return true;
}

bool WMSettings::Init()
{
   if (hSettingsKey_ == nullptr) {
      DWORD regError = RegCreateKeyExW(
         HKEY_CURRENT_USER,
         LX_SYSTEMS_SUBKEY,
         0,
         nullptr,
         0,
         KEY_READ | KEY_WRITE,
         nullptr,
         &hSettingsKey_,
         nullptr);
      if (regError != ERROR_SUCCESS) {
         ShowWindowsError(L"RegCreateKeyEx", regError);
         return false;
      }
   }
   if (hWifiKey_ == nullptr) {
      DWORD regError = RegCreateKeyExW(
         HKEY_CURRENT_USER,
         LX_SYSTEMS_WIFI_SUBKEY,
         0,
         nullptr,
         0,
         KEY_READ | KEY_WRITE,
         nullptr,
         &hWifiKey_,
         nullptr);
      if (regError != ERROR_SUCCESS) {
         ShowWindowsError(L"RegCreateKeyEx", regError);
         RegCloseKey(hSettingsKey_);
         hSettingsKey_ = nullptr;
         return false;
      }
   }
   if (hBluetoothKey_ == nullptr) {
      DWORD regError = RegCreateKeyExW(
         HKEY_CURRENT_USER,
         LX_SYSTEMS_BLUETOOTH_SUBKEY,
         0,
         nullptr,
         0,
         KEY_READ | KEY_WRITE,
         nullptr,
         &hBluetoothKey_,
         nullptr);
      if (regError != ERROR_SUCCESS) {
         ShowWindowsError(L"RegCreateKeyEx", regError);
         RegCloseKey(hWifiKey_);
         RegCloseKey(hSettingsKey_);
         hSettingsKey_ = nullptr;
         hWifiKey_ = nullptr;
         return false;
      }
   }
   if (hAudioEndpointsKey_ == nullptr) {
      DWORD regError = RegCreateKeyExW(
         HKEY_CURRENT_USER,
         LX_SYSTEMS_AUDIO_ENDPOINTS_SUBKEY,
         0,
         nullptr,
         0,
         KEY_READ | KEY_WRITE,
         nullptr,
         &hAudioEndpointsKey_,
         nullptr);
      if (regError != ERROR_SUCCESS) {
         ShowWindowsError(L"RegCreateKeyEx", regError);
         RegCloseKey(hWifiKey_);
         RegCloseKey(hSettingsKey_);
         RegCloseKey(hBluetoothKey_);
         hSettingsKey_ = nullptr;
         hWifiKey_ = nullptr;
         hBluetoothKey_ = nullptr;
         return false;
      }
   }

   MigrateSettings();

   return true;
}

void WMSettings::Unload() noexcept
{
   RegCloseKey(hSettingsKey_);
   hSettingsKey_ = nullptr;
   RegCloseKey(hWifiKey_);
   hWifiKey_ = nullptr;
   RegCloseKey(hBluetoothKey_);
   hBluetoothKey_ = nullptr;
}

HKEY WMSettings::OpenAutostartKey(REGSAM samDesired)
{
   HKEY hRunKey = nullptr;
   DWORD regError = RegOpenKeyExW(
      HKEY_CURRENT_USER,
      L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
      0,
      samDesired,
      &hRunKey);
   if (regError != ERROR_SUCCESS) {
      ShowWindowsError(L"RegOpenKeyEx", regError);
   }
   return hRunKey;
}

bool WMSettings::IsAutostartEnabled()
{
   bool isEnabled = false;
   WMLog& log = WMLog::GetInstance();
   wchar_t wmPath[MAX_PATH + 1];
   if (GetModuleFileName(nullptr, wmPath, sizeof(wmPath) / sizeof(wmPath[0])) == 0) {
      log.LogError(L"Failed to get path of winmute");
   } else {
      HKEY hRunKey = OpenAutostartKey(KEY_READ);
      if (hRunKey != nullptr) {
         std::wstring path;
         if (ReadStringFromRegistry(hRunKey, LX_SYSTEMS_AUTOSTART_KEY, path)) {
            if (path != wmPath) {
               log.LogInfo(L"Autostart entry has wrong path");
            } else {
               isEnabled = true;
            }
         }
         RegCloseKey(hRunKey);
      }
   }
   return isEnabled;
}

void WMSettings::EnableAutostart(bool enable)
{
   WMLog& log = WMLog::GetInstance();
   HKEY hRunKey = OpenAutostartKey(KEY_WRITE);
   if (hRunKey != nullptr) {
      if (enable) {
         wchar_t wmPath[MAX_PATH + 1];
         if (GetModuleFileNameW(nullptr, wmPath, sizeof(wmPath) / sizeof(wmPath[0])) == 0) {
            log.LogError(L"Failed to get path of winmute");
         } else {
            DWORD regError = RegSetKeyValueW(
               hRunKey,
               nullptr,
               LX_SYSTEMS_AUTOSTART_KEY,
               REG_SZ,
               wmPath,
               (lstrlen(wmPath) + 1) * sizeof(wchar_t));
            if (regError != ERROR_SUCCESS) {
               ShowWindowsError(L"RegSetKeyValue", regError);
            }
         }
      } else {
         DWORD regError = RegDeleteKeyValueW(hRunKey, nullptr, LX_SYSTEMS_AUTOSTART_KEY);
         if (regError != ERROR_SUCCESS && regError != ERROR_FILE_NOT_FOUND) {
            ShowWindowsError(L"RegDeleteKeyValue", regError);
         }
      }
      RegCloseKey(hRunKey);
   }
}

DWORD WMSettings::QueryValue(SettingsKey key) const
{
   auto keyStr = KeyToStr(key);
   assert(keyStr != nullptr);

   DWORD value = 0;
   DWORD size = sizeof(DWORD);
   DWORD regError = RegQueryValueExW(
      hSettingsKey_,
      keyStr,
      0,
      nullptr,
      reinterpret_cast<LPBYTE>(&value),
      &size);
   if (regError != ERROR_SUCCESS) {
      if (regError != ERROR_FILE_NOT_FOUND) {
         ShowWindowsError(L"RegQueryValueExW", regError);
         WMLog &log = WMLog::GetInstance();
         log.LogError(L"[Registry] Failed to query value for key \"%s\"", keyStr);
      }
      return GetDefaultSetting(key);
   }

   return value;
}

std::optional<std::wstring> WMSettings::QueryStrValue(SettingsKey key) const
{
   std::wstring val;
   auto keyStr = KeyToStr(key);
   assert(keyStr != nullptr);
   if (!ReadStringFromRegistry(hSettingsKey_, keyStr, val)) {
      return std::nullopt;
   }
   return val;
}

bool WMSettings::SetValue(SettingsKey key, DWORD value)
{
   auto keyStr = KeyToStr(key);
   assert(keyStr != nullptr);

   DWORD regError = RegSetValueExW(
      hSettingsKey_,
      keyStr,
      0,
      REG_DWORD,
      reinterpret_cast<BYTE*>(&value),
      sizeof(DWORD));
   if (regError != ERROR_SUCCESS) {
      ShowWindowsError(L"RegSetValueExW", regError);
      return false;
   }

   return true;
}

bool WMSettings::SetValue(SettingsKey key, const std::wstring &value)
{
   auto keyStr = KeyToStr(key);
   assert(keyStr != nullptr);

   BYTE *strValue = reinterpret_cast<BYTE *>(const_cast<wchar_t *>(value.c_str()));
   DWORD strLen = static_cast<DWORD>((value.length() + 1) * sizeof(wchar_t));
   DWORD regError = RegSetValueExW(
      hSettingsKey_,
      keyStr,
      0,
      REG_SZ,
      strValue,
      strLen);
   if (regError != ERROR_SUCCESS) {
      ShowWindowsError(L"RegSetValueExW", regError);
      return false;
   }

   return true;
}

bool WMSettings::StoreWifiNetworks(std::vector<std::wstring>& networks)
{
   // Clear all stored keys
   for (;;) {
      wchar_t valueName[260] = { 0 };
      DWORD valueSize = ARRAY_SIZE(valueName);
      DWORD regError = RegEnumValueW(
         hWifiKey_,
         0,
         valueName,
         &valueSize,
         nullptr,
         nullptr,
         nullptr,
         nullptr);
      if (regError == ERROR_NO_MORE_ITEMS) {
         break;
      } else if (regError != ERROR_SUCCESS) {
         ShowWindowsError(L"RegEnumValue", regError);
         return false;
      } else {
         regError = RegDeleteValue(hWifiKey_, valueName);
         if (regError != ERROR_SUCCESS) {
            ShowWindowsError(L"RegDeleteValue", regError);
            return false;
         }
      }
   }

   NormalizeStringList(networks);

   for (size_t i = 0; i < networks.size(); ++i) {
      wchar_t valueName[25];
      StringCchPrintfW(valueName, ARRAY_SIZE(valueName), L"WiFi %03lld", i + 1);
      const std::wstring& v = networks[i];
      DWORD regError = RegSetValueExW(
         hWifiKey_,
         valueName,
         0,
         REG_SZ,
         reinterpret_cast<const BYTE*>(v.c_str()),
         static_cast<DWORD>(v.length() + 1) * sizeof(wchar_t));
      if (regError != ERROR_SUCCESS) {
         ShowWindowsError(L"RegSetValueEx", regError);
         return false;
      }
   }

   return true;
}

std::vector<std::wstring> WMSettings::GetWifiNetworks() const
{
   std::vector<std::wstring> networks;
   for (int valIdx = 0; ; ++valIdx) {
      wchar_t valueName[260] = { 0 };
      wchar_t dataBuf[260] = { 0 };
      DWORD valueSize = ARRAY_SIZE(valueName);
      DWORD valType = 0;
      DWORD dataLen = ARRAY_SIZE(dataBuf);

      DWORD regError = RegEnumValueW(
         hWifiKey_,
         valIdx,
         valueName,
         &valueSize,
         nullptr,
         &valType,
         reinterpret_cast<BYTE*>(dataBuf),
         &dataLen);
      if (regError == ERROR_NO_MORE_ITEMS) {
         break;
      } else if (regError != ERROR_SUCCESS) {
         ShowWindowsError(L"RegEnumValue", regError);
         return {};
      } else {
         networks.push_back(dataBuf);
      }
   }
   NormalizeStringList(networks);
   return networks;
}

bool WMSettings::StoreBluetoothDevices(std::vector<std::wstring>& devices)
{
   // Clear all stored keys
   for (;;) {
      wchar_t valueName[260] = { 0 };
      DWORD valueSize = ARRAY_SIZE(valueName);
      DWORD regError = RegEnumValueW(
         hBluetoothKey_,
         0,
         valueName,
         &valueSize,
         nullptr,
         nullptr,
         nullptr,
         nullptr);
      if (regError == ERROR_NO_MORE_ITEMS) {
         break;
      } else if (regError != ERROR_SUCCESS) {
         ShowWindowsError(L"RegEnumValue", regError);
         return false;
      } else {
         regError = RegDeleteValue(hBluetoothKey_, valueName);
         if (regError != ERROR_SUCCESS) {
            ShowWindowsError(L"RegDeleteValue", regError);
            return false;
         }
      }
   }

   NormalizeStringList(devices);

   for (size_t i = 0; i < devices.size(); ++i) {
      wchar_t valueName[25];
      StringCchPrintfW(
         valueName,
         ARRAY_SIZE(valueName),
         L"Bluetooth %03lld", i + 1);
      const std::wstring& v = devices[i];
      DWORD regError = RegSetValueEx(
         hBluetoothKey_,
         valueName,
         0,
         REG_SZ,
         reinterpret_cast<const BYTE*>(v.c_str()),
         static_cast<DWORD>(v.length() + 1) * sizeof(wchar_t));
      if (regError != ERROR_SUCCESS) {
         ShowWindowsError(L"RegSetValueEx", regError);
         return false;
      }
   }

   return true;
}

std::vector<std::string> WMSettings::GetBluetoothDevicesA() const
{
   std::vector<std::string> devices;
   for (int valIdx = 0; ; ++valIdx) {
      char valueName[260] = { 0 };
      char dataBuf[260] = { 0 };
      DWORD valueSize = ARRAY_SIZE(valueName);
      DWORD valType = 0;
      DWORD dataLen = ARRAY_SIZE(dataBuf);

      DWORD regError = RegEnumValueA(
         hBluetoothKey_,
         valIdx,
         valueName,
         &valueSize,
         nullptr,
         &valType,
         reinterpret_cast<BYTE*>(dataBuf),
         &dataLen);
      if (regError == ERROR_NO_MORE_ITEMS) {
         break;
      } else if (regError != ERROR_SUCCESS) {
         ShowWindowsError(L"RegEnumValue", regError);
         return {};
      } else {
         devices.push_back(dataBuf);
      }
   }
   NormalizeStringList(devices);
   return devices;
}

std::vector<std::wstring> WMSettings::GetBluetoothDevicesW() const
{
   std::vector<std::wstring> devices;
   for (int valIdx = 0; ; ++valIdx) {
      wchar_t valueName[260] = { 0 };
      wchar_t dataBuf[260] = { 0 };
      DWORD valueSize = ARRAY_SIZE(valueName);
      DWORD valType = 0;
      DWORD dataLen = ARRAY_SIZE(dataBuf);

      DWORD regError = RegEnumValueW(
         hBluetoothKey_,
         valIdx,
         valueName,
         &valueSize,
         nullptr,
         &valType,
         reinterpret_cast<BYTE*>(dataBuf),
         &dataLen);
      if (regError == ERROR_NO_MORE_ITEMS) {
         break;
      } else if (regError != ERROR_SUCCESS) {
         ShowWindowsError(L"RegEnumValue", regError);
         return {};
      } else {
         devices.push_back(dataBuf);
      }
   }
   NormalizeStringList(devices);
   return devices;
}


bool WMSettings::StoreManagedAudioEndpoints(std::vector<std::wstring> &endpoints)
{
   // Clear all stored keys
   for (;;) {
      wchar_t valueName[260] = { 0 };
      DWORD valueSize = ARRAY_SIZE(valueName);
      DWORD regError = RegEnumValueW(
         hAudioEndpointsKey_,
         0,
         valueName,
         &valueSize,
         nullptr,
         nullptr,
         nullptr,
         nullptr);
      if (regError == ERROR_NO_MORE_ITEMS) {
         break;
      } else if (regError != ERROR_SUCCESS) {
         ShowWindowsError(L"RegEnumValue", regError);
         return false;
      } else {
         regError = RegDeleteValue(hAudioEndpointsKey_, valueName);
         if (regError != ERROR_SUCCESS) {
            ShowWindowsError(L"RegDeleteValue", regError);
            return false;
         }
      }
   }

   NormalizeStringList(endpoints);

   for (size_t i = 0; i < endpoints.size(); ++i) {
      wchar_t valueName[25];
      StringCchPrintfW(
         valueName,
         ARRAY_SIZE(valueName),
         L"Endpoint %03lld", i + 1);
      const std::wstring &v = endpoints[i];
      DWORD regError = RegSetValueEx(
         hAudioEndpointsKey_,
         valueName,
         0,
         REG_SZ,
         reinterpret_cast<const BYTE *>(v.c_str()),
         static_cast<DWORD>(v.length() + 1) * sizeof(wchar_t));
      if (regError != ERROR_SUCCESS) {
         ShowWindowsError(L"RegSetValueEx", regError);
         return false;
      }
   }

   return true;
}

std::vector<std::wstring> WMSettings::GetManagedAudioEndpoints() const
{
   std::vector<std::wstring> devices;
   for (int valIdx = 0; ; ++valIdx) {
      wchar_t valueName[260] = { 0 };
      wchar_t dataBuf[260] = { 0 };
      DWORD valueSize = ARRAY_SIZE(valueName);
      DWORD valType = 0;
      DWORD dataLen = ARRAY_SIZE(dataBuf);

      DWORD regError = RegEnumValueW(
         hAudioEndpointsKey_,
         valIdx,
         valueName,
         &valueSize,
         nullptr,
         &valType,
         reinterpret_cast<BYTE *>(dataBuf),
         &dataLen);
      if (regError == ERROR_NO_MORE_ITEMS) {
         break;
      } else if (regError != ERROR_SUCCESS) {
         ShowWindowsError(L"RegEnumValue", regError);
         return {};
      } else {
         devices.push_back(dataBuf);
      }
   }
   NormalizeStringList(devices);
   return devices;
}

