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

// The pages below the "General" node of the settings navigation tree:
// Language, Startup & Updates, Hotkeys and Logging.

#include "common.h"

namespace fs = std::filesystem;

extern void ShowLogDialog(HWND hParent);
extern HINSTANCE hglobInstance;

static WMSettings* GetPageSettings(HWND hDlg)
{
    return reinterpret_cast<WMSettings*>(GetWindowLongPtr(hDlg, DWLP_USER));
}

// =============================================================================
// Language

struct SettingsLanguageData {
    WMSettings* settings = nullptr;
    std::vector<LanguageModule> langModules;
};

static void FillLanguageList(HWND hLanguageList,
                             const SettingsLanguageData& dlgData)
{
    SendMessage(hLanguageList, CB_INITSTORAGE,
                static_cast<WPARAM>(dlgData.langModules.size() + 1),
                (MAX_PATH + 1) * sizeof(wchar_t));
    for (const auto& lang : dlgData.langModules) {
        const auto itemId =
            SendMessage(hLanguageList, CB_ADDSTRING, 0,
                        reinterpret_cast<LPARAM>(lang.langName.c_str()));
        if (itemId == CB_ERR || itemId == CB_ERRSPACE) {
            WMLog::GetInstance().LogError(
                L"Failed to add language {} to language selector",
                lang.langName);
        } else {
            ComboBox_SetItemData(hLanguageList, itemId, lang.fileName.c_str());
        }
    }
    ComboBox_SelectString(
        hLanguageList, 0,
        WMi18n::GetInstance().GetCurrentLanguageName().c_str());
}

static void LoadLanguageDlgTranslation(HWND hDlg)
{
    WMi18n& i18n = WMi18n::GetInstance();

    const auto helpTranslateLink = std::vformat(
        L"<a "
        L"href=\"https://github.com/lx-s/WinMute/blob/main/"
        L"CONTRIBUTING.md#translations\">{}</a>",
        std::make_wformat_args(
            i18n.GetTranslationW("settings.general.help-translating")));

    SetDlgItemText(hDlg, IDC_LINK_HELP_TRANSLATING, helpTranslateLink.c_str());
    i18n.SetItemText(hDlg, IDC_SELECT_LANGUAGE_LABEL,
                     "settings.general.select-language-label");
}

INT_PTR CALLBACK Settings_LanguageDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                          LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    switch (msg) {
        case WM_INITDIALOG: {
            LoadLanguageDlgTranslation(hDlg);

            SettingsLanguageData* dlgData = new SettingsLanguageData;
            dlgData->langModules =
                WMi18n::GetInstance().GetAvailableLanguages();
            dlgData->settings = reinterpret_cast<WMSettings*>(lParam);
            assert(dlgData->settings != nullptr);
            SetWindowLongPtr(hDlg, DWLP_USER,
                             reinterpret_cast<LONG_PTR>(dlgData));

            FillLanguageList(GetDlgItem(hDlg, IDC_LANGUAGE), *dlgData);

            return TRUE;
        }
        case WM_DESTROY: {
            delete reinterpret_cast<SettingsLanguageData*>(
                GetWindowLongPtr(hDlg, DWLP_USER));
            SetWindowLongPtr(hDlg, DWLP_USER, 0);
            return FALSE;
        }
        case WM_NOTIFY: {
            const PNMLINK pNmLink = reinterpret_cast<PNMLINK>(lParam);
#pragma warning(push)
#pragma warning(disable : 26454)  // Disable arithmetic overflow warning for
                                  // NM_CLICK and NM_RETURN
            if (pNmLink->hdr.code == NM_CLICK || pNmLink->hdr.code == NM_RETURN)
            {
#pragma warning(pop)
                const UINT_PTR ctrlId = pNmLink->hdr.idFrom;
                const LITEM item = pNmLink->item;
                if ((ctrlId == IDC_LINK_HELP_TRANSLATING) && item.iLink == 0) {
                    LaunchBrowser(hDlg, item.szUrl);
                }
            }
            return TRUE;
        }
        case WM_SAVESETTINGS: {
            SettingsLanguageData* dlgData =
                reinterpret_cast<SettingsLanguageData*>(
                    GetWindowLongPtr(hDlg, DWLP_USER));
            assert(dlgData != nullptr);

            HWND hLanguageSelector = GetDlgItem(hDlg, IDC_LANGUAGE);
            const auto curLangSel = ComboBox_GetCurSel(hLanguageSelector);
            if (curLangSel != CB_ERR) {
                const wchar_t* selectedLang = reinterpret_cast<const wchar_t*>(
                    ComboBox_GetItemData(hLanguageSelector, curLangSel));
                if (selectedLang != nullptr) {
                    if (!WMi18n::GetInstance().LoadLanguage(selectedLang)) {
                        TaskDialog(hDlg, hglobInstance, PROGRAM_NAME,
                                   L"Failed to load selected language.",
                                   L"Please report this error to the WinMute "
                                   L"issue tracker.",
                                   TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
                    } else {
                        dlgData->settings->SetValue(SettingsKey::APP_LANGUAGE,
                                                    selectedLang);
                    }
                }
            }
            return 0;
        }
        default:
            break;
    }
    return FALSE;
}

// =============================================================================
// Startup & Updates

static void LoadUpdatesDlgTranslation(HWND hDlg)
{
    WMi18n& i18n = WMi18n::GetInstance();

    i18n.SetItemText(hDlg, IDC_RUNONSTARTUP, "settings.general.run-on-startup");
    i18n.SetItemText(hDlg, IDC_CHECK_FOR_UPDATES_ON_STARTUP,
                     "settings.general.check-for-updates-on-start");
    i18n.SetItemText(hDlg, IDC_CHECK_FOR_BETA_UPDATES,
                     "settings.general.check-for-beta-updates-on-start");
    i18n.SetItemText(hDlg, IDC_UPDATE_OPTIONS_DISABLED,
                     "settings.general.updates-handled-externally");
}

INT_PTR CALLBACK Settings_UpdatesDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                         LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG: {
            LoadUpdatesDlgTranslation(hDlg);

            WMSettings* settings = reinterpret_cast<WMSettings*>(lParam);
            assert(settings != nullptr);
            SetWindowLongPtr(hDlg, DWLP_USER,
                             reinterpret_cast<LONG_PTR>(settings));

            HWND hAutostart = GetDlgItem(hDlg, IDC_RUNONSTARTUP);
            HWND hUpdateCheck =
                GetDlgItem(hDlg, IDC_CHECK_FOR_UPDATES_ON_STARTUP);
            HWND hBetaUpdateCheck =
                GetDlgItem(hDlg, IDC_CHECK_FOR_BETA_UPDATES);
            HWND hUpdatesDisabledNotice =
                GetDlgItem(hDlg, IDC_UPDATE_OPTIONS_DISABLED);

            const AutostartState autostart = settings->GetAutostartState();
            Button_SetCheck(hAutostart,
                            (autostart == AutostartState::Enabled ||
                             autostart == AutostartState::EnabledByPolicy)
                                ? BST_CHECKED
                                : BST_UNCHECKED);
            // Group policy has the final say in a packaged install, so show the
            // state it forces but do not pretend WinMute can change it.
            // DisabledByUser stays clickable on purpose: the attempt is what
            // produces the hint pointing at the Task Manager.
            if (autostart == AutostartState::EnabledByPolicy ||
                autostart == AutostartState::DisabledByPolicy)
            {
                EnableWindow(hAutostart, FALSE);
            }

            DWORD enabled =
                !!settings->QueryValue(SettingsKey::CHECK_FOR_UPDATE);
            Button_SetCheck(hUpdateCheck,
                            enabled ? BST_CHECKED : BST_UNCHECKED);

            EnableWindow(hBetaUpdateCheck, enabled);
            enabled =
                !!settings->QueryValue(SettingsKey::CHECK_FOR_BETA_UPDATE);
            Button_SetCheck(hBetaUpdateCheck,
                            enabled ? BST_CHECKED : BST_UNCHECKED);

            // If the disable-update file is present, then also hide all options
            UpdateChecker updateChecker;
            if (updateChecker.IsUpdateCheckDisabledViaFile()) {
                EnableWindow(hUpdateCheck, FALSE);
                EnableWindow(hBetaUpdateCheck, FALSE);
                Button_SetCheck(hUpdateCheck, BST_UNCHECKED);
                Button_SetCheck(hBetaUpdateCheck, BST_UNCHECKED);
                ShowWindow(hUpdatesDisabledNotice, SW_SHOW);
            } else {
                ShowWindow(hUpdatesDisabledNotice, SW_HIDE);
            }

            return TRUE;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == IDC_CHECK_FOR_UPDATES_ON_STARTUP) {
                const int enabled = Button_GetCheck(
                    GetDlgItem(hDlg, IDC_CHECK_FOR_UPDATES_ON_STARTUP));
                EnableWindow(GetDlgItem(hDlg, IDC_CHECK_FOR_BETA_UPDATES),
                             enabled);
            }
            return 0;
        }
        case WM_SAVESETTINGS: {
            WMSettings* settings = GetPageSettings(hDlg);
            assert(settings != nullptr);

            HWND hAutostart = GetDlgItem(hDlg, IDC_RUNONSTARTUP);
            HWND hUpdateCheck =
                GetDlgItem(hDlg, IDC_CHECK_FOR_UPDATES_ON_STARTUP);
            HWND hBetaUpdateCheck =
                GetDlgItem(hDlg, IDC_CHECK_FOR_BETA_UPDATES);

            const int enableUpdateCheck =
                Button_GetCheck(hUpdateCheck) == BST_CHECKED;
            settings->SetValue(SettingsKey::CHECK_FOR_UPDATE,
                               enableUpdateCheck);

            const int enableBetaUpdateCheck =
                Button_GetCheck(hBetaUpdateCheck) == BST_CHECKED;
            settings->SetValue(SettingsKey::CHECK_FOR_BETA_UPDATE,
                               enableBetaUpdateCheck);

            if (IsWindowEnabled(hAutostart)) {
                const bool wantAutostart =
                    Button_GetCheck(hAutostart) == BST_CHECKED;
                const AutostartState autostart =
                    settings->EnableAutostart(wantAutostart);
                if (wantAutostart &&
                    autostart == AutostartState::DisabledByUser)
                {
                    // Windows refuses to undo a startup entry the user switched
                    // off in the Task Manager; only they can turn it back on.
                    const WMi18n& i18n = WMi18n::GetInstance();
                    TaskDialog(
                        hDlg, hglobInstance, PROGRAM_NAME,
                        i18n.GetTranslationW(
                                "settings.general.autostart-blocked.title")
                            .c_str(),
                        i18n.GetTranslationW(
                                "settings.general.autostart-blocked.text")
                            .c_str(),
                        TDCBF_OK_BUTTON, TD_WARNING_ICON, nullptr);
                    Button_SetCheck(hAutostart, BST_UNCHECKED);
                }
            }

            return 0;
        }
        default:
            break;
    }
    return FALSE;
}

// =============================================================================
// Hotkeys

INT_PTR CALLBACK Settings_HotkeysDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                         LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG: {
            WMi18n::GetInstance().SetItemText(
                hDlg, IDC_ENABLE_GLOBAL_MUTE_HOTKEY,
                "settings.general.enable-global-mute-hotkey");

            WMSettings* settings = reinterpret_cast<WMSettings*>(lParam);
            assert(settings != nullptr);
            SetWindowLongPtr(hDlg, DWLP_USER,
                             reinterpret_cast<LONG_PTR>(settings));

            HWND hEnableGlobalMuteHotkey =
                GetDlgItem(hDlg, IDC_ENABLE_GLOBAL_MUTE_HOTKEY);
            HWND hGlobalMuteHotkey = GetDlgItem(hDlg, IDC_GLOBAL_MUTE_HOTKEY);

            const auto hotkey =
                settings->QueryValue(SettingsKey::GLOBAL_MUTE_HOTKEY);
            SendMessage(hGlobalMuteHotkey, HKM_SETHOTKEY, hotkey, 0);
            const DWORD enabled =
                !!settings->QueryValue(SettingsKey::ENABLE_GLOBAL_MUTE_HOTKEY);
            Button_SetCheck(hEnableGlobalMuteHotkey,
                            enabled ? BST_CHECKED : BST_UNCHECKED);
            EnableWindow(hGlobalMuteHotkey, enabled);

            return TRUE;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == IDC_ENABLE_GLOBAL_MUTE_HOTKEY) {
                const auto checked = Button_GetCheck(
                    GetDlgItem(hDlg, IDC_ENABLE_GLOBAL_MUTE_HOTKEY));
                EnableWindow(GetDlgItem(hDlg, IDC_GLOBAL_MUTE_HOTKEY), checked);
            }
            return 0;
        }
        case WM_SAVESETTINGS: {
            WMSettings* settings = GetPageSettings(hDlg);
            assert(settings != nullptr);

            HWND hEnableGlobalMuteHotkey =
                GetDlgItem(hDlg, IDC_ENABLE_GLOBAL_MUTE_HOTKEY);
            HWND hGlobalMuteHotkey = GetDlgItem(hDlg, IDC_GLOBAL_MUTE_HOTKEY);

            const int enableGlobalMuteHotkey =
                Button_GetCheck(hEnableGlobalMuteHotkey) == BST_CHECKED;
            settings->SetValue(SettingsKey::ENABLE_GLOBAL_MUTE_HOTKEY,
                               enableGlobalMuteHotkey);
            const auto hotkey =
                SendMessage(hGlobalMuteHotkey, HKM_GETHOTKEY, 0, 0);
            settings->SetValue(SettingsKey::GLOBAL_MUTE_HOTKEY,
                               static_cast<DWORD>(hotkey));

            return 0;
        }
        default:
            break;
    }
    return FALSE;
}

// =============================================================================
// Logging

static void UpdateLogFilePath(HWND hDlg, bool loggingEnabled)
{
    const std::wstring filePath = loggingEnabled
                                      ? WMLog::GetInstance().GetLogFilePath()
                                      : std::wstring();
    SendMessageW(GetDlgItem(hDlg, IDC_LOGFILEPATH), WM_SETTEXT, 0,
                 reinterpret_cast<LPARAM>(filePath.c_str()));
}

static void LoadLoggingDlgTranslation(HWND hDlg)
{
    WMi18n& i18n = WMi18n::GetInstance();

    i18n.SetItemText(hDlg, IDC_ENABLELOGGING,
                     "settings.general.enable-logging");
    i18n.SetItemText(hDlg, IDC_OPENLOG, "settings.general.btn-open-log-file");
    i18n.SetItemText(hDlg, IDC_OPENLOGDLG,
                     "settings.general.btn-open-log-window");
}

INT_PTR CALLBACK Settings_LoggingDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                         LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG: {
            LoadLoggingDlgTranslation(hDlg);

            WMSettings* settings = reinterpret_cast<WMSettings*>(lParam);
            assert(settings != nullptr);
            SetWindowLongPtr(hDlg, DWLP_USER,
                             reinterpret_cast<LONG_PTR>(settings));

            HWND hLogging = GetDlgItem(hDlg, IDC_ENABLELOGGING);
            const DWORD enabled =
                !!settings->QueryValue(SettingsKey::LOGGING_ENABLED);
            Button_SetCheck(hLogging, enabled ? BST_CHECKED : BST_UNCHECKED);
            Button_Enable(GetDlgItem(hDlg, IDC_OPENLOG), enabled);
            UpdateLogFilePath(hDlg, !!enabled);

            return TRUE;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == IDC_ENABLELOGGING) {
                const DWORD checked =
                    Button_GetCheck(GetDlgItem(hDlg, IDC_ENABLELOGGING));
                Button_Enable(GetDlgItem(hDlg, IDC_OPENLOG),
                              checked == BST_CHECKED);
                UpdateLogFilePath(hDlg, checked == BST_CHECKED);
            } else if (LOWORD(wParam) == IDC_OPENLOGDLG) {
                ShowLogDialog(hDlg);
            } else if (LOWORD(wParam) == IDC_OPENLOG) {
                const std::wstring filePath =
                    WMLog::GetInstance().GetLogFilePath();
                ShellExecuteW(nullptr, L"open", filePath.c_str(), nullptr,
                              nullptr, SW_SHOW);
            }
            return 0;
        }
        case WM_SAVESETTINGS: {
            WMSettings* settings = GetPageSettings(hDlg);
            assert(settings != nullptr);

            const int enableLog =
                Button_GetCheck(GetDlgItem(hDlg, IDC_ENABLELOGGING)) ==
                BST_CHECKED;
            settings->SetValue(SettingsKey::LOGGING_ENABLED, enableLog);
            WMLog::GetInstance().EnableLogFile(enableLog);

            return 0;
        }
        default:
            break;
    }
    return FALSE;
}
