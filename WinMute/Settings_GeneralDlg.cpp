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

namespace fs = std::filesystem;

struct SettingsGeneralData {
   WMSettings *settings = nullptr;
   std::vector<LanguageModule> langModules;
};

extern HINSTANCE hglobInstance;

static void FillLanguageList(HWND hLanguageList, const SettingsGeneralData& dlgData)
{
   SendMessage(hLanguageList,
               CB_INITSTORAGE,
               static_cast<WPARAM>(dlgData.langModules.size() + 1),
               (MAX_PATH + 1) * sizeof(wchar_t));
   for (const auto &lang : dlgData.langModules) {
      const auto itemId = SendMessage(hLanguageList, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(lang.langName.c_str()));
      if (itemId == CB_ERR || itemId == CB_ERRSPACE) {
         WMLog::GetInstance().LogError(L"Failed to add language %ls to language selector", lang.langName.c_str());
      } else {
         ComboBox_SetItemData(hLanguageList, itemId, lang.fileName.c_str());
      }
   }
   ComboBox_SelectString(hLanguageList, 0, WMi18n::GetInstance().GetCurrentLanguageName().c_str());
}

static void LoadSettingsGeneralDlgTranslation(HWND hDlg)
{
   WMi18n &i18n = WMi18n::GetInstance();
   i18n.SetItemText(hDlg, IDC_SELECT_LANGUAGE_LABEL, "settings.general.select-language-label");
   i18n.SetItemText(hDlg, IDC_RUNONSTARTUP, "settings.general.run-on-startup");
   i18n.SetItemText(hDlg, IDC_CHECK_FOR_UPDATES_ON_STARTUP, "settings.general.check-for-updates-on-start");
   i18n.SetItemText(hDlg, IDC_CHECK_FOR_BETA_UPDATES, "settings.general.check-for-beta-updates-on-start");
   i18n.SetItemText(hDlg, IDC_ENABLELOGGING, "settings.general.enable-logging");
   i18n.SetItemText(hDlg, IDC_OPENLOG, "settings.general.btn-open-log-file");
}

INT_PTR CALLBACK Settings_GeneralDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) {
   case WM_INITDIALOG: {
      HWND hAutostart = GetDlgItem(hDlg, IDC_RUNONSTARTUP);
      HWND hUpdateCheck = GetDlgItem(hDlg, IDC_CHECK_FOR_UPDATES_ON_STARTUP);
      HWND hBetaUpdateCheck = GetDlgItem(hDlg, IDC_CHECK_FOR_BETA_UPDATES);
      HWND hLogging = GetDlgItem(hDlg, IDC_ENABLELOGGING);
      HWND hOpenLog = GetDlgItem(hDlg, IDC_OPENLOG);

      if (IsAppThemed()) {
         EnableThemeDialogTexture(hDlg, ETDT_ENABLETAB);
      }
      LoadSettingsGeneralDlgTranslation(hDlg);

      SettingsGeneralData *dlgData = new SettingsGeneralData;
      memset(dlgData, 0, sizeof(*dlgData));
      dlgData->langModules = WMi18n::GetInstance().GetAvailableLanguages();
      dlgData->settings = reinterpret_cast<WMSettings *>(lParam);
      assert(dlgData->settings != nullptr);

      SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dlgData));
     
      FillLanguageList(GetDlgItem(hDlg, IDC_LANGUAGE), *dlgData);

      DWORD enabled = dlgData->settings->IsAutostartEnabled();
      Button_SetCheck(hAutostart, enabled ? BST_CHECKED : BST_UNCHECKED);

      enabled = !!dlgData->settings->QueryValue(SettingsKey::CHECK_FOR_UPDATE);
      Button_SetCheck(hUpdateCheck, enabled ? BST_CHECKED : BST_UNCHECKED);

      EnableWindow(hBetaUpdateCheck, enabled);
      enabled = !!dlgData->settings->QueryValue(SettingsKey::CHECK_FOR_BETA_UPDATE);
      Button_SetCheck(hBetaUpdateCheck, enabled ? BST_CHECKED : BST_UNCHECKED);

      enabled = !!dlgData->settings->QueryValue(SettingsKey::LOGGING_ENABLED);
      Button_SetCheck(hLogging, enabled ? BST_CHECKED : BST_UNCHECKED);
      Button_Enable(hOpenLog, enabled);
      if (enabled) {
         WMLog& log = WMLog::GetInstance();
         const std::wstring filePath = log.GetLogFilePath().c_str();
         SendMessageW(GetDlgItem(hDlg, IDC_LOGFILEPATH), WM_SETTEXT, 0,
            reinterpret_cast<LPARAM>(filePath.c_str()));
                      
      } else {
         SendMessageW(GetDlgItem(hDlg, IDC_LOGFILEPATH), WM_SETTEXT, 0,
            reinterpret_cast<LPARAM>(L""));
      }

      return TRUE;
   }
   case WM_DESTROY: {
      SettingsGeneralData *dlgData = reinterpret_cast<SettingsGeneralData *>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
      if (dlgData != nullptr) {
         delete dlgData;
         SetWindowLongPtr(hDlg, GWLP_USERDATA, 0);
      }
      return FALSE;
   }
   case WM_COMMAND: {
      if (LOWORD(wParam) == IDC_ENABLELOGGING) {
         DWORD checked = Button_GetCheck(GetDlgItem(hDlg, IDC_ENABLELOGGING));
         Button_Enable(GetDlgItem(hDlg, IDC_OPENLOG), checked == BST_CHECKED);
         if (checked == BST_CHECKED) {
            WMLog& log = WMLog::GetInstance();
            const std::wstring filePath = log.GetLogFilePath().c_str();
            SendMessageW(GetDlgItem(hDlg, IDC_LOGFILEPATH), WM_SETTEXT, 0,
               reinterpret_cast<LPARAM>(filePath.c_str()));

         } else {
            SendMessageW(GetDlgItem(hDlg, IDC_LOGFILEPATH), WM_SETTEXT, 0,
               reinterpret_cast<LPARAM>(L""));
         }
      } else if (LOWORD(wParam) == IDC_CHECK_FOR_UPDATES_ON_STARTUP) {
         const int enabled = Button_GetCheck(GetDlgItem(hDlg, IDC_CHECK_FOR_UPDATES_ON_STARTUP));
         EnableWindow(GetDlgItem(hDlg, IDC_CHECK_FOR_BETA_UPDATES), enabled);
      } else if (LOWORD(wParam) == IDC_OPENLOG) {
         WMLog& log = WMLog::GetInstance();
         const std::wstring filePath = log.GetLogFilePath().c_str();
         ShellExecuteW(nullptr, L"open", filePath.c_str(), nullptr, nullptr, SW_SHOW);
      }
      return 0;
   }
   case WM_SAVESETTINGS: {
      SettingsGeneralData *dlgData = reinterpret_cast<SettingsGeneralData *>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
      assert(dlgData != nullptr);

      HWND hAutostart = GetDlgItem(hDlg, IDC_RUNONSTARTUP);
      HWND hLogging = GetDlgItem(hDlg, IDC_ENABLELOGGING);
      HWND hUpdateCheck = GetDlgItem(hDlg, IDC_CHECK_FOR_UPDATES_ON_STARTUP);
      HWND hBetaUpdateCheck = GetDlgItem(hDlg, IDC_CHECK_FOR_BETA_UPDATES);

      const int enableLog = Button_GetCheck(hLogging) == BST_CHECKED;
      dlgData->settings->SetValue(SettingsKey::LOGGING_ENABLED, enableLog);
      WMLog::GetInstance().SetEnabled(enableLog);

      HWND hLanguageSelector = GetDlgItem(hDlg, IDC_LANGUAGE);
      const auto curLangSel = ComboBox_GetCurSel(hLanguageSelector);
      if (curLangSel != CB_ERR) {
         const wchar_t *selectedLang = reinterpret_cast<const wchar_t *>(ComboBox_GetItemData(hLanguageSelector, curLangSel));
         if (selectedLang != nullptr) {
            if (!WMi18n::GetInstance().LoadLanguage(selectedLang)) {
               TaskDialog(
                  hDlg,
                  hglobInstance,
                  PROGRAM_NAME,
                  L"Failed to load selected language.",
                  L"Please report this error to the WinMute issue tracker.",
                  TDCBF_OK_BUTTON,
                  TD_ERROR_ICON,
                  nullptr);
            } else {
               dlgData->settings->SetValue(SettingsKey::APP_LANGUAGE, selectedLang);
            }
         }
      }

      const int enableUpdateCheck = Button_GetCheck(hUpdateCheck) == BST_CHECKED;
      dlgData->settings->SetValue(SettingsKey::CHECK_FOR_UPDATE, enableUpdateCheck);

      const int enableBetaUpdateCheck = Button_GetCheck(hBetaUpdateCheck) == BST_CHECKED;
      dlgData->settings->SetValue(SettingsKey::CHECK_FOR_BETA_UPDATE, enableBetaUpdateCheck);

      if (Button_GetCheck(hAutostart) == BST_CHECKED) {
         dlgData->settings->EnableAutostart(true);
      } else {
         dlgData->settings->EnableAutostart(false);
      }

      return 0;
   }
   default:
      break;
   }
   return FALSE;
}
