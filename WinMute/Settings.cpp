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
#include "Settings.h"

static const LPCWSTR LX_SYSTEMS_SUBKEY = _T("SOFTWARE\\lx-systems\\WinMute");


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
   }
   return keyStr;
}


Settings::Settings() :
   hRegSettingsKey_(nullptr)
{
}

Settings::~Settings()
{
   Unload();
}

bool Settings::Init()
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

void Settings::Unload()
{
   RegCloseKey(hRegSettingsKey_);
   hRegSettingsKey_ = nullptr;
}

DWORD Settings::QueryValue(SettingsKey key, DWORD defValue)
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

bool Settings::SetValue(SettingsKey key, DWORD value)
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
