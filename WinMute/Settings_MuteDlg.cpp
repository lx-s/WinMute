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

static void SetCheckButton(HWND hBtn, const WMSettings& settings, SettingsKey key)
{
   DWORD enabled = !!settings.QueryValue(key);
   Button_SetCheck(hBtn, enabled ? BST_CHECKED : BST_UNCHECKED);
}

static void SetOption(HWND hBtn, WMSettings& settings, SettingsKey key)
{
   int enable = Button_GetCheck(hBtn) == BST_CHECKED;
   settings.SetValue(key, enable);
}

INT_PTR CALLBACK Settings_MuteDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   UNREFERENCED_PARAMETER(wParam);

   switch (msg) {
   case WM_INITDIALOG:
   {
      if (IsAppThemed()) {
         EnableThemeDialogTexture(hDlg, ETDT_ENABLETAB);
      }

      HWND hMuteOnLock = GetDlgItem(hDlg, IDC_MUTE_WHEN_WS_LOCKED);
      HWND hMuteOnScreensaver = GetDlgItem(hDlg, IDC_MUTE_WHEN_SCREENSAVER_STARTS);
      HWND hMuteOnScreenOff = GetDlgItem(hDlg, IDC_MUTE_WHEN_SCREEN_OFF);
      HWND hMuteOnRDP = GetDlgItem(hDlg, IDC_MUTE_WHEN_RDP_SESSION);
      HWND hRestoreVolume = GetDlgItem(hDlg, IDC_RESTOREVOLUME);

      HWND hMuteOnShutdown = GetDlgItem(hDlg, IDC_MUTE_WHEN_SHUTDOWN);
      HWND hMuteOnSleep = GetDlgItem(hDlg, IDC_MUTE_WHEN_SLEEP);
      HWND hMuteOnLogout = GetDlgItem(hDlg, IDC_MUTE_WHEN_LOGOUT);

      WMSettings* settings = reinterpret_cast<WMSettings*>(lParam);
      assert(settings != NULL);
      SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(settings));

      // With restore
      SetCheckButton(hMuteOnLock, *settings, SettingsKey::MUTE_ON_LOCK);
      SetCheckButton(hMuteOnScreensaver, *settings, SettingsKey::MUTE_ON_SCREENSAVER);
      SetCheckButton(hMuteOnScreenOff, *settings, SettingsKey::MUTE_ON_DISPLAYSTANDBY);
      SetCheckButton(hMuteOnRDP, *settings, SettingsKey::MUTE_ON_RDP);
      SetCheckButton(hRestoreVolume, *settings, SettingsKey::RESTORE_AUDIO);

      // Without restore
      SetCheckButton(hMuteOnShutdown, *settings, SettingsKey::MUTE_ON_SHUTDOWN);
      SetCheckButton(hMuteOnSleep, *settings, SettingsKey::MUTE_ON_SUSPEND);
      SetCheckButton(hMuteOnLogout, *settings, SettingsKey::MUTE_ON_LOGOUT);

      return TRUE;
   }
   case WM_SAVESETTINGS:
   {
      WMSettings* settings = reinterpret_cast<WMSettings*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));

      HWND hMuteOnLock = GetDlgItem(hDlg, IDC_MUTE_WHEN_WS_LOCKED);
      HWND hMuteOnScreensaver = GetDlgItem(hDlg, IDC_MUTE_WHEN_SCREENSAVER_STARTS);
      HWND hMuteOnScreenOff = GetDlgItem(hDlg, IDC_MUTE_WHEN_SCREEN_OFF);
      HWND hMuteOnRDP = GetDlgItem(hDlg, IDC_MUTE_WHEN_RDP_SESSION);
      HWND hRestoreVolume = GetDlgItem(hDlg, IDC_RESTOREVOLUME);

      HWND hMuteOnShutdown = GetDlgItem(hDlg, IDC_MUTE_WHEN_SHUTDOWN);
      HWND hMuteOnSleep = GetDlgItem(hDlg, IDC_MUTE_WHEN_SLEEP);
      HWND hMuteOnLogout = GetDlgItem(hDlg, IDC_MUTE_WHEN_LOGOUT);

      // With restore
      SetOption(hMuteOnLock, *settings, SettingsKey::MUTE_ON_LOCK);
      SetOption(hMuteOnScreensaver, *settings, SettingsKey::MUTE_ON_SCREENSAVER);
      SetOption(hMuteOnScreenOff, *settings, SettingsKey::MUTE_ON_DISPLAYSTANDBY);
      SetOption(hMuteOnRDP, *settings, SettingsKey::MUTE_ON_RDP);
      SetOption(hRestoreVolume, *settings, SettingsKey::RESTORE_AUDIO);

      // Without restore
      SetOption(hMuteOnShutdown, *settings, SettingsKey::MUTE_ON_SHUTDOWN);
      SetOption(hMuteOnSleep, *settings, SettingsKey::MUTE_ON_SUSPEND);
      SetOption(hMuteOnLogout, *settings, SettingsKey::MUTE_ON_LOGOUT);

      return 0;
   }
   default:
      break;
   }
   return FALSE;
}