/*
 WinMute
           Copyright (c) 2022, Alexander Steinhoefer

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

static const LPCWSTR LX_SYSTEMS_SUBKEY = _T("SOFTWARE\\lx-systems\\WinMute");
static const LPCWSTR LX_SYSTEMS_AUTOSTART_KEY = _T("LX-Systems WinMute");

static LPCWSTR KeyToStr(SettingsKey key)
{
   LPCWSTR keyStr = nullptr;
   switch (key) {
   case SettingsKey::MUTE_ON_LOCK:
      keyStr = _T("MuteOnLock");
      break;
   case SettingsKey::MUTE_ON_SCREENSAVER:
      keyStr = _T("MuteOnScreensaver");
      break;
   case SettingsKey::MUTE_ON_DISPLAYSTANDBY:
      keyStr = _T("MuteOnDisplayStandby");
      break;
   case SettingsKey::RESTORE_AUDIO:
      keyStr = _T("RestoreAudio");
      break;
   case SettingsKey::MUTE_ON_SUSPEND:
      keyStr = _T("MuteOnSuspend");
      break;
   case SettingsKey::MUTE_ON_SHUTDOWN:
      keyStr = _T("MuteOnShutdown");
      break;
   case SettingsKey::MUTE_ON_LOGOUT:
      keyStr = _T("MuteOnLogout");
      break;
   case SettingsKey::QUIETHOURS_ENABLE:
      keyStr = _T("QuietHoursEnabled");
      break;
   case SettingsKey::QUIETHOURS_FORCEUNMUTE:
      keyStr = _T("QuietHoursForceUnmute");
      break;
   case SettingsKey::QUIETHOURS_NOTIFICATIONS:
      keyStr = _T("QuietHoursNotifications");
      break;
   case SettingsKey::QUIETHOURS_START:
      keyStr = _T("QuietHoursStart");
      break;
   case SettingsKey::QUIETHOURS_END:
      keyStr = _T("QuietHoursEnd");
      break;
   case SettingsKey::LOGGING_ENABLED:
      keyStr = _T("Logging");
      break;
   case SettingsKey::NOTIFICATIONS_ENABLED:
      keyStr = _T("ShowNotifications");
      break;
   }
   return keyStr;
}

static bool ReadStringFromRegistry(HKEY hKey, const TCHAR* subKey, tstring& val)
{
   bool success = false;
   DWORD regError = 0;
   DWORD bufSize = 0;
   regError = RegQueryValueEx(hKey, subKey, nullptr, nullptr, nullptr, &bufSize);
   if (regError != ERROR_SUCCESS) {
      if (regError != ERROR_FILE_NOT_FOUND) {
         PrintWindowsError(_T("RegQueryValueEx"), regError);
      }
   } else {
      bufSize += 1; // Trailing '\0'
      TCHAR *buf = new TCHAR[bufSize];
      regError = RegQueryValueEx(hKey, subKey, nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(buf), &bufSize);
      if (regError != ERROR_SUCCESS) {
         PrintWindowsError(_T("RegQueryValueEx"), regError);
      } else {
         success = true;
         val = buf;
      }
      delete [] buf;
   }

   return success;
}

WMSettings::WMSettings() :
   hRegSettingsKey_(nullptr)
{
}

WMSettings::~WMSettings()
{
   Unload();
}

bool WMSettings::Init()
{
   if (hRegSettingsKey_ == nullptr) {
      DWORD regError = RegCreateKeyEx(
         HKEY_CURRENT_USER,
         LX_SYSTEMS_SUBKEY,
         0,
         nullptr,
         0,
         KEY_READ | KEY_WRITE,
         nullptr,
         &hRegSettingsKey_,
         nullptr);
      if (regError != ERROR_SUCCESS) {
         PrintWindowsError(_T("RegCreateKeyEx"), regError);
         return false;
      }
   }

   return true;
}

void WMSettings::Unload()
{
   RegCloseKey(hRegSettingsKey_);
   hRegSettingsKey_ = nullptr;
}

HKEY WMSettings::OpenAutostartKey(REGSAM samDesired)
{
   HKEY hRunKey = NULL;
   DWORD regError = RegOpenKeyEx(
      HKEY_CURRENT_USER,
      _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"),
      0,
      samDesired,
      &hRunKey);
   if (regError != ERROR_SUCCESS) {
      PrintWindowsError(_T("RegOpenKeyEx"), regError);
   }
   return hRunKey;
}

bool WMSettings::IsAutostartEnabled()
{
   bool isEnabled = false;
   WMLog& log = WMLog::GetInstance();
   TCHAR wmPath[MAX_PATH + 1];
   if (GetModuleFileName(NULL, wmPath, sizeof(wmPath) / sizeof(wmPath[0])) == 0) {
      log.Write(_T("Failed to get path of win mute"));
   } else {
      HKEY hRunKey = OpenAutostartKey(KEY_READ);
      if (hRunKey != NULL) {
         tstring path;
         if (ReadStringFromRegistry(hRunKey, LX_SYSTEMS_AUTOSTART_KEY, path)) {
            if (path != wmPath) {
               log.Write(_T("Autostart entry has wrong path"));
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
   if (hRunKey != NULL) {
      if (enable) {
         TCHAR wmPath[MAX_PATH + 1];
         if (GetModuleFileName(NULL, wmPath, sizeof(wmPath) / sizeof(wmPath[0])) == 0) {
            log.Write(_T("Failed to get path of win mute"));
         } else {
            DWORD regError = RegSetKeyValue(
               hRunKey,
               NULL,
               LX_SYSTEMS_AUTOSTART_KEY,
               REG_SZ,
               wmPath,
               (lstrlen(wmPath) + 1) * sizeof(TCHAR));
            if (regError != ERROR_SUCCESS) {
               PrintWindowsError(_T("RegOpenKeyEx"), regError);
            }
         }
      } else {
         DWORD regError = RegDeleteKeyValue(hRunKey, NULL, LX_SYSTEMS_AUTOSTART_KEY);
         if (regError != ERROR_SUCCESS) {
            PrintWindowsError(_T("RegOpenKeyEx"), regError);
         }
      }
      RegCloseKey(hRunKey);
   }
}

DWORD WMSettings::QueryValue(SettingsKey key, DWORD defValue)
{
   auto keyStr = KeyToStr(key);
   assert(keyStr != nullptr);

   DWORD value;
   DWORD size = sizeof(DWORD);
   DWORD regError = RegQueryValueEx(
      hRegSettingsKey_,
      keyStr,
      0,
      nullptr,
      reinterpret_cast<LPBYTE>(&value),
      &size);
   if (regError == ERROR_FILE_NOT_FOUND) {
      return defValue;
   } else if (regError != ERROR_SUCCESS) {
      PrintWindowsError(_T("RegCreateKeyEx"), regError);
      return defValue;
   }

   return value;
}

bool WMSettings::SetValue(SettingsKey key, DWORD value)
{
   auto keyStr = KeyToStr(key);
   assert(keyStr != nullptr);

   DWORD regError = RegSetValueEx(
      hRegSettingsKey_,
      keyStr,
      0,
      REG_DWORD,
      reinterpret_cast<BYTE*>(&value),
      sizeof(DWORD));
   if (regError != ERROR_SUCCESS) {
      PrintWindowsError(_T("RegCreateKeyEx"), regError);
      return false;
   }

   return true;
}
