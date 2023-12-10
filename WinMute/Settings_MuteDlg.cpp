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

extern INT_PTR CALLBACK Settings_ManageEndpointsDlgProc(HWND, UINT, WPARAM, LPARAM);

static void SetCheckButton(HWND hBtn, const WMSettings& settings, SettingsKey key)
{
   const DWORD enabled = !!settings.QueryValue(key);
   Button_SetCheck(hBtn, enabled ? BST_CHECKED : BST_UNCHECKED);
}

static void SetOption(HWND hBtn, WMSettings& settings, SettingsKey key)
{
   const int enable = Button_GetCheck(hBtn) == BST_CHECKED;
   settings.SetValue(key, enable);
}

static void LoadMuteDlgTranslation(HWND hDlg)
{
   WMi18n &i18n = WMi18n::GetInstance();

   i18n.SetItemText(hDlg, IDC_GROUP_GENERAL, IDS_SETTINGS_MUTE_TITLE_GENERAL);
   i18n.SetItemText(hDlg, IDC_SHOWNOTIFICATIONS, IDS_SETTINGS_MUTE_SHOW_MUTE_EVENT_NOTIFICATIONS);
   i18n.SetItemText(hDlg, IDC_MANAGE_AUDIO_ENDPOINTS_INDIVIDUALLY, IDS_SETTINGS_MUTE_MANAGE_ENDPOINTS_INDIVIDUALLY);
   i18n.SetItemText(hDlg, IDC_MANAGE_ENDPOINTS, IDS_SETTINGS_MUTE_MANAGE_ENDPOINTS_BTN);

   i18n.SetItemText(hDlg, IDC_GROUP_MUTE_WITH_RESTORE, IDS_SETTINGS_MUTE_MUTE_WITH_RESTORE_TITLE);
   i18n.SetItemText(hDlg, IDC_MUTE_WHEN_WS_LOCKED, IDS_SETTINGS_MUTE_WHEN_WORKSTATION_IS_LOCKED);
   i18n.SetItemText(hDlg, IDC_MUTE_WHEN_SCREEN_OFF, IDS_SETTINGS_MUTE_WHEN_SCREEN_TURNS_OFF);
   i18n.SetItemText(hDlg, IDC_RESTOREVOLUME, IDS_SETTINGS_MUTE_RESTORE_VOLUME);
   i18n.SetItemText(hDlg, IDC_DELAY_MUTING_LABEL, IDS_SETTINGS_MUTE_RESTORE_VOLUME_DELAY_LABEL);

   i18n.SetItemText(hDlg, IDC_GROUP_MUTE_WITHOUT_RESTORE, IDS_SETTINGS_MUTE_MUTE_WITHOUT_RESTORE_TITLE);
   i18n.SetItemText(hDlg, IDC_MUTE_WHEN_SHUTDOWN, IDS_SETTINGS_MUTE_WHEN_COMPUTER_SHUTS_DOWN);
   i18n.SetItemText(hDlg, IDC_MUTE_WHEN_SLEEP, IDS_SETTINGS_MUTE_WHEN_COMPUTER_GOES_TO_SLEEP);
   i18n.SetItemText(hDlg, IDC_MUTE_WHEN_LOGOUT, IDS_SETTINGS_MUTE_WHEN_USER_LOGS_OUT);
   i18n.SetItemText(hDlg, IDC_MUTE_WHEN_RDP_SESSION, IDS_SETTINGS_MUTE_WHEN_RDP_SESSION_STARTS);
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

      LoadMuteDlgTranslation(hDlg);

      HWND hNotify = GetDlgItem(hDlg, IDC_SHOWNOTIFICATIONS);
      HWND hManageEndpoints = GetDlgItem(hDlg, IDC_MANAGE_AUDIO_ENDPOINTS_INDIVIDUALLY);

      HWND hMuteOnLock = GetDlgItem(hDlg, IDC_MUTE_WHEN_WS_LOCKED);
      HWND hMuteOnScreenOff = GetDlgItem(hDlg, IDC_MUTE_WHEN_SCREEN_OFF);
      HWND hMuteOnRDP = GetDlgItem(hDlg, IDC_MUTE_WHEN_RDP_SESSION);
      HWND hRestoreVolume = GetDlgItem(hDlg, IDC_RESTOREVOLUME);

      HWND hMuteOnShutdown = GetDlgItem(hDlg, IDC_MUTE_WHEN_SHUTDOWN);
      HWND hMuteOnSleep = GetDlgItem(hDlg, IDC_MUTE_WHEN_SLEEP);
      HWND hMuteOnLogout = GetDlgItem(hDlg, IDC_MUTE_WHEN_LOGOUT);

      WMSettings* settings = reinterpret_cast<WMSettings*>(lParam);
      assert(settings != nullptr);
      SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(settings));

      // General
      SetCheckButton(hNotify, *settings, SettingsKey::NOTIFICATIONS_ENABLED);
      SetCheckButton(hManageEndpoints, *settings, SettingsKey::MUTE_INDIVIDUAL_ENDPOINTS);
      Button_Enable(GetDlgItem(hDlg, IDC_MANAGE_ENDPOINTS), Button_GetCheck(hManageEndpoints) == BST_CHECKED);
      const DWORD muteDelay = settings->QueryValue(SettingsKey::MUTE_DELAY);
      SetDlgItemInt(hDlg, IDC_MUTEDELAY, (muteDelay < 0) ? 0 : muteDelay, false);

      // With restore
      SetCheckButton(hMuteOnLock, *settings, SettingsKey::MUTE_ON_LOCK);
      SetCheckButton(hMuteOnScreenOff, *settings, SettingsKey::MUTE_ON_DISPLAYSTANDBY);
      SetCheckButton(hMuteOnRDP, *settings, SettingsKey::MUTE_ON_RDP);
      SetCheckButton(hRestoreVolume, *settings, SettingsKey::RESTORE_AUDIO);

      // Without restore
      SetCheckButton(hMuteOnShutdown, *settings, SettingsKey::MUTE_ON_SHUTDOWN);
      SetCheckButton(hMuteOnSleep, *settings, SettingsKey::MUTE_ON_SUSPEND);
      SetCheckButton(hMuteOnLogout, *settings, SettingsKey::MUTE_ON_LOGOUT);

      return TRUE;
   }
   case WM_COMMAND:
      if (LOWORD(wParam) == IDC_MANAGE_AUDIO_ENDPOINTS_INDIVIDUALLY) {
         int isEnabled = Button_GetCheck(GetDlgItem(hDlg, IDC_MANAGE_AUDIO_ENDPOINTS_INDIVIDUALLY));
         Button_Enable(GetDlgItem(hDlg, IDC_MANAGE_ENDPOINTS), isEnabled);
      } else if (LOWORD(wParam) == IDC_MANAGE_ENDPOINTS) {
         WMSettings *settings = reinterpret_cast<WMSettings *>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
         if (!DialogBoxParam(
               nullptr,
               MAKEINTRESOURCE(IDD_MANAGE_ENDPOINTS),
               hDlg,
               Settings_ManageEndpointsDlgProc,
               reinterpret_cast<LPARAM>(settings)) == 0) {
            PrintWindowsError(L"DialogBoxParam", GetLastError());
         }
      }
      return 0;
   case WM_SAVESETTINGS:
   {
      WMSettings* settings = reinterpret_cast<WMSettings*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));

      HWND hNotify = GetDlgItem(hDlg, IDC_SHOWNOTIFICATIONS);
      HWND hManageEndpoints = GetDlgItem(hDlg, IDC_MANAGE_AUDIO_ENDPOINTS_INDIVIDUALLY);

      HWND hMuteOnLock = GetDlgItem(hDlg, IDC_MUTE_WHEN_WS_LOCKED);
      HWND hMuteOnScreenOff = GetDlgItem(hDlg, IDC_MUTE_WHEN_SCREEN_OFF);
      HWND hMuteOnRDP = GetDlgItem(hDlg, IDC_MUTE_WHEN_RDP_SESSION);
      HWND hRestoreVolume = GetDlgItem(hDlg, IDC_RESTOREVOLUME);

      HWND hMuteOnShutdown = GetDlgItem(hDlg, IDC_MUTE_WHEN_SHUTDOWN);
      HWND hMuteOnSleep = GetDlgItem(hDlg, IDC_MUTE_WHEN_SLEEP);
      HWND hMuteOnLogout = GetDlgItem(hDlg, IDC_MUTE_WHEN_LOGOUT);

      // General
      SetOption(hNotify, *settings, SettingsKey::NOTIFICATIONS_ENABLED);
      SetOption(hManageEndpoints, *settings, SettingsKey::MUTE_INDIVIDUAL_ENDPOINTS);
      const DWORD muteDelay = GetDlgItemInt(hDlg, IDC_MUTEDELAY, nullptr, TRUE);
      settings->SetValue(SettingsKey::MUTE_DELAY, (muteDelay < 0) ? 0 : muteDelay);

      // With restore
      SetOption(hMuteOnLock, *settings, SettingsKey::MUTE_ON_LOCK);
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
