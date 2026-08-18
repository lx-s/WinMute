/*
 WinMute
           Copyright (c) 2011-2026 Alexander Steinhoefer

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

// The "Mute events" and "Media control" pages below the "Muting" node of the
// settings navigation tree. The endpoint list lives in
// Settings_ManageEndpointsDlg.cpp.

#include "common.h"

static void SetCheckButton(HWND hBtn, const WMSettings& settings,
                           SettingsKey key)
{
    const DWORD enabled = !!settings.QueryValue(key);
    Button_SetCheck(hBtn, enabled ? BST_CHECKED : BST_UNCHECKED);
}

static void SetOption(HWND hBtn, WMSettings& settings, SettingsKey key)
{
    const int enable = Button_GetCheck(hBtn) == BST_CHECKED;
    settings.SetValue(key, enable);
}

static WMSettings* GetPageSettings(HWND hDlg)
{
    return reinterpret_cast<WMSettings*>(GetWindowLongPtr(hDlg, DWLP_USER));
}

// =============================================================================
// Mute events

static void LoadMuteDlgTranslation(HWND hDlg)
{
    WMi18n& i18n = WMi18n::GetInstance();

    i18n.SetItemText(hDlg, IDC_SHOWNOTIFICATIONS,
                     "settings.mute.show-mute-event-notifications");

    i18n.SetItemText(hDlg, IDC_GROUP_MUTE_WITH_RESTORE,
                     "settings.mute.mute-with-restore.title");
    i18n.SetItemText(
        hDlg, IDC_MUTE_WHEN_WS_LOCKED,
        "settings.mute.mute-with-restore.when-workstation-is-locked");
    i18n.SetItemText(hDlg, IDC_MUTE_WHEN_SCREEN_OFF,
                     "settings.mute.mute-with-restore.when-screen-turns-off");
    i18n.SetItemText(hDlg, IDC_MUTE_WHEN_LID_CLOSE,
                     "settings.mute.mute-with-restore.when-lid-closes");
    i18n.SetItemText(hDlg, IDC_RESTOREVOLUME,
                     "settings.mute.mute-with-restore.restore-volume");
    i18n.SetItemText(
        hDlg, IDC_DELAY_MUTING_LABEL,
        "settings.mute.mute-with-restore.restore-volume-delay-label");

    i18n.SetItemText(hDlg, IDC_GROUP_MUTE_WITHOUT_RESTORE,
                     "settings.mute.mute-without-restore.title");
    i18n.SetItemText(
        hDlg, IDC_MUTE_WHEN_SHUTDOWN,
        "settings.mute.mute-without-restore.when-computer-shuts-down");
    i18n.SetItemText(
        hDlg, IDC_MUTE_WHEN_SLEEP,
        "settings.mute.mute-without-restore.when-computer-goes-to-sleep");
    i18n.SetItemText(hDlg, IDC_MUTE_WHEN_LOGOUT,
                     "settings.mute.mute-without-restore.when-user-logs-out");
    i18n.SetItemText(
        hDlg, IDC_MUTE_WHEN_RDP_SESSION,
        "settings.mute.mute-without-restore.when-rdp-session-starts");
}

INT_PTR CALLBACK Settings_MuteDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                      LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    switch (msg) {
        case WM_INITDIALOG: {
            LoadMuteDlgTranslation(hDlg);

            HWND hNotify = GetDlgItem(hDlg, IDC_SHOWNOTIFICATIONS);

            HWND hMuteOnLock = GetDlgItem(hDlg, IDC_MUTE_WHEN_WS_LOCKED);
            HWND hMuteOnScreenOff = GetDlgItem(hDlg, IDC_MUTE_WHEN_SCREEN_OFF);
            HWND hMuteOnLidClose = GetDlgItem(hDlg, IDC_MUTE_WHEN_LID_CLOSE);
            HWND hMuteOnRDP = GetDlgItem(hDlg, IDC_MUTE_WHEN_RDP_SESSION);
            HWND hRestoreVolume = GetDlgItem(hDlg, IDC_RESTOREVOLUME);

            HWND hMuteOnShutdown = GetDlgItem(hDlg, IDC_MUTE_WHEN_SHUTDOWN);
            HWND hMuteOnSleep = GetDlgItem(hDlg, IDC_MUTE_WHEN_SLEEP);
            HWND hMuteOnLogout = GetDlgItem(hDlg, IDC_MUTE_WHEN_LOGOUT);

            WMSettings* settings = reinterpret_cast<WMSettings*>(lParam);
            assert(settings != nullptr);
            SetWindowLongPtr(hDlg, DWLP_USER,
                             reinterpret_cast<LONG_PTR>(settings));

            SetCheckButton(hNotify, *settings,
                           SettingsKey::NOTIFICATIONS_ENABLED);
            const DWORD muteDelay =
                settings->QueryValue(SettingsKey::MUTE_DELAY);
            SetDlgItemInt(hDlg, IDC_MUTEDELAY, (muteDelay < 0) ? 0 : muteDelay,
                          false);

            // With restore
            SetCheckButton(hMuteOnLock, *settings, SettingsKey::MUTE_ON_LOCK);
            SetCheckButton(hMuteOnScreenOff, *settings,
                           SettingsKey::MUTE_ON_DISPLAYSTANDBY);
            SetCheckButton(hMuteOnLidClose, *settings,
                           SettingsKey::MUTE_ON_LIDCLOSE);
            SetCheckButton(hMuteOnRDP, *settings, SettingsKey::MUTE_ON_RDP);
            SetCheckButton(hRestoreVolume, *settings,
                           SettingsKey::RESTORE_AUDIO);

            // Without restore
            SetCheckButton(hMuteOnShutdown, *settings,
                           SettingsKey::MUTE_ON_SHUTDOWN);
            SetCheckButton(hMuteOnSleep, *settings,
                           SettingsKey::MUTE_ON_SUSPEND);
            SetCheckButton(hMuteOnLogout, *settings,
                           SettingsKey::MUTE_ON_LOGOUT);

            return TRUE;
        }
        case WM_SAVESETTINGS: {
            WMSettings* settings = GetPageSettings(hDlg);
            assert(settings != nullptr);

            HWND hNotify = GetDlgItem(hDlg, IDC_SHOWNOTIFICATIONS);

            HWND hMuteOnLock = GetDlgItem(hDlg, IDC_MUTE_WHEN_WS_LOCKED);
            HWND hMuteOnScreenOff = GetDlgItem(hDlg, IDC_MUTE_WHEN_SCREEN_OFF);
            HWND hMuteOnLidClose = GetDlgItem(hDlg, IDC_MUTE_WHEN_LID_CLOSE);
            HWND hMuteOnRDP = GetDlgItem(hDlg, IDC_MUTE_WHEN_RDP_SESSION);
            HWND hRestoreVolume = GetDlgItem(hDlg, IDC_RESTOREVOLUME);

            HWND hMuteOnShutdown = GetDlgItem(hDlg, IDC_MUTE_WHEN_SHUTDOWN);
            HWND hMuteOnSleep = GetDlgItem(hDlg, IDC_MUTE_WHEN_SLEEP);
            HWND hMuteOnLogout = GetDlgItem(hDlg, IDC_MUTE_WHEN_LOGOUT);

            SetOption(hNotify, *settings, SettingsKey::NOTIFICATIONS_ENABLED);
            const DWORD muteDelay =
                GetDlgItemInt(hDlg, IDC_MUTEDELAY, nullptr, TRUE);
            settings->SetValue(SettingsKey::MUTE_DELAY,
                               (muteDelay < 0) ? 0 : muteDelay);

            // With restore
            SetOption(hMuteOnLock, *settings, SettingsKey::MUTE_ON_LOCK);
            SetOption(hMuteOnScreenOff, *settings,
                      SettingsKey::MUTE_ON_DISPLAYSTANDBY);
            SetOption(hMuteOnLidClose, *settings,
                      SettingsKey::MUTE_ON_LIDCLOSE);

            SetOption(hMuteOnRDP, *settings, SettingsKey::MUTE_ON_RDP);
            SetOption(hRestoreVolume, *settings, SettingsKey::RESTORE_AUDIO);

            // Without restore
            SetOption(hMuteOnShutdown, *settings,
                      SettingsKey::MUTE_ON_SHUTDOWN);
            SetOption(hMuteOnSleep, *settings, SettingsKey::MUTE_ON_SUSPEND);
            SetOption(hMuteOnLogout, *settings, SettingsKey::MUTE_ON_LOGOUT);

            return 0;
        }
        default:
            break;
    }
    return FALSE;
}

// =============================================================================
// Media control

INT_PTR CALLBACK Settings_MediaDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                       LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    switch (msg) {
        case WM_INITDIALOG: {
            WMi18n& i18n = WMi18n::GetInstance();
            i18n.SetItemText(hDlg, IDC_MUTE_TRY_PAUSE_MEDIA,
                             "settings.mute.try-pause-media-on-mute");
            i18n.SetItemText(hDlg, IDC_MUTE_TRY_RESUME_MEDIA,
                             "settings.mute.try-resume-media-on-unmute");

            WMSettings* settings = reinterpret_cast<WMSettings*>(lParam);
            assert(settings != nullptr);
            SetWindowLongPtr(hDlg, DWLP_USER,
                             reinterpret_cast<LONG_PTR>(settings));

            SetCheckButton(GetDlgItem(hDlg, IDC_MUTE_TRY_PAUSE_MEDIA),
                           *settings, SettingsKey::MUTE_TRY_PAUSE_MEDIA);
            SetCheckButton(GetDlgItem(hDlg, IDC_MUTE_TRY_RESUME_MEDIA),
                           *settings, SettingsKey::MUTE_TRY_RESUME_MEDIA);

            return TRUE;
        }
        case WM_SAVESETTINGS: {
            WMSettings* settings = GetPageSettings(hDlg);
            assert(settings != nullptr);

            SetOption(GetDlgItem(hDlg, IDC_MUTE_TRY_PAUSE_MEDIA), *settings,
                      SettingsKey::MUTE_TRY_PAUSE_MEDIA);
            SetOption(GetDlgItem(hDlg, IDC_MUTE_TRY_RESUME_MEDIA), *settings,
                      SettingsKey::MUTE_TRY_RESUME_MEDIA);

            return 0;
        }
        default:
            break;
    }
    return FALSE;
}
