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

struct QuietHoursEntry {
   DWORD start = 0;
   DWORD end = 0;
};

static bool IsValidTimeRange(
   const LPSYSTEMTIME start,
   const LPSYSTEMTIME end)
{
   bool isValid = true;
   if (start->wHour == end->wHour &&
      start->wMinute == end->wMinute &&
      start->wSecond == end->wSecond) {
      isValid = false;
   }
   return isValid;
}

static bool SaveQuietHours(
   WMSettings* settings,
   const int enabled,
   const int forceUnmute,
   const int showNotifications,
   const std::vector<std::pair<DWORD, DWORD>>& times)
{
   //if (enabled) {
   //   if (!IsValidTimeRange(start, end)) {
   //      WMi18n &i18n = WMi18n::GetInstance();
   //      TaskDialog(
   //         nullptr,
   //         nullptr,
   //         PROGRAM_NAME,
   //         i18n.GetTranslationW("settings.quiet-hours.error.invalid-time-range.title").c_str(),
   //         i18n.GetTranslationW("settings.quiet-hours.error.invalid-time-range.text").c_str(),
   //         TDCBF_OK_BUTTON,
   //         TD_WARNING_ICON,
   //         nullptr);
   //      return false;
   //   }
   //}

   if (!settings->SetValue(SettingsKey::QUIETHOURS_ENABLE, enabled)
       || !settings->SetValue(SettingsKey::QUIETHOURS_FORCEUNMUTE, forceUnmute)
       || !settings->SetValue(SettingsKey::QUIETHOURS_NOTIFICATIONS, showNotifications)
       || !settings->StoreQuietHoursTimes(times)) {
      WMi18n &i18n = WMi18n::GetInstance();

      TaskDialog(
         nullptr,
         nullptr,
         PROGRAM_NAME,
         i18n.GetTranslationW("settings.quiet-hours.error.error-while-saving.title").c_str(),
         i18n.GetTranslationW("settings.quiet-hours.error.error-while-saving.text").c_str(),
         TDCBF_OK_BUTTON,
         TD_ERROR_ICON,
         nullptr);
      return false;
   }

   return true;
}

static void LoadQuietHoursAddDlgTranslation(HWND hDlg, bool isEdit)
{
   WMi18n &i18n = WMi18n::GetInstance();
   if (isEdit) {
      i18n.SetItemText(hDlg, "settings.quiet-hours.add-edit.edit-title");
   } else {
      i18n.SetItemText(hDlg, "settings.quiet-hours.add-edit.add-title");
   }
   i18n.SetItemText(hDlg, IDC_QUIET_HOURS_START_LABEL, "settings.quiet-hours.start-time-label");
   i18n.SetItemText(hDlg, IDC_QUIET_HOURS_END_LABEL, "settings.quiet-hours.end-time-label");
   i18n.SetItemText(hDlg, IDOK, "settings.btn-save");
   i18n.SetItemText(hDlg, IDCANCEL, "settings.btn-cancel");
}

static void SetQuietHourPickerTime(HWND hPicker, DWORD savedTime)
{
   SYSTEMTIME tp{};
   if (savedTime > 0) {
      tp.wSecond = static_cast<WORD>(savedTime % 60);
      tp.wMinute = static_cast<WORD>(((savedTime - tp.wSecond) / 60) % 60);
      tp.wHour = static_cast<WORD>((savedTime - tp.wMinute - tp.wSecond) / 3600);
   } else {
      GetLocalTime(&tp);
   }
   DateTime_SetSystemtime(hPicker, GDT_VALID, &tp);
}

static INT_PTR CALLBACK Settings_QuietHoursAddDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) {
   case WM_INITDIALOG:
   {
      HWND hStart = GetDlgItem(hDlg, IDC_QUIET_HOURS_START_PICKER);
      HWND hEnd = GetDlgItem(hDlg, IDC_QUIET_HOURS_END_PICKER);

      QuietHoursEntry *qh_entry = reinterpret_cast<QuietHoursEntry *>(lParam);
      if (qh_entry == nullptr) {
         return FALSE;
      }
      SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(qh_entry));

      LoadQuietHoursAddDlgTranslation(hDlg, qh_entry != nullptr);
      if (qh_entry->start != 0) {
         SetQuietHourPickerTime(hStart, qh_entry->start);
      }
      if (qh_entry->end != 0) {
         SetQuietHourPickerTime(hEnd, qh_entry->start);
      }
      return TRUE;
   }
   case WM_COMMAND:
      if (LOWORD(wParam) == IDOK) {
         HWND hStart = GetDlgItem(hDlg, IDC_QUIET_HOURS_START_PICKER);
         HWND hEnd = GetDlgItem(hDlg, IDC_QUIET_HOURS_END_PICKER);
         SYSTEMTIME start{};
         SYSTEMTIME end{};
         DateTime_GetSystemtime(hStart, &start);
         DateTime_GetSystemtime(hEnd, &end);
         if (!IsValidTimeRange(&start, &end)) {
            WMi18n &i18n = WMi18n::GetInstance();
            TaskDialog(
               nullptr,
               nullptr,
               PROGRAM_NAME,
               i18n.GetTranslationW("settings.quiet-hours.error.invalid-time-range.title").c_str(),
               i18n.GetTranslationW("settings.quiet-hours.error.invalid-time-range.text").c_str(),
               TDCBF_OK_BUTTON,
               TD_WARNING_ICON,
               nullptr);
            return false;
         } else {
            auto *qh_entry = reinterpret_cast<QuietHoursEntry *>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
            qh_entry->start = start.wHour * 3600 + start.wMinute * 60 + start.wSecond;
            qh_entry->end = end.wHour * 3600 + end.wMinute * 60 + end.wSecond;
            EndDialog(hDlg, 0);
         }
      } else if (LOWORD(wParam) == IDCANCEL) {
         EndDialog(hDlg, 1);
      }
      return FALSE;
   case WM_CLOSE:
      EndDialog(hDlg, 1);
      return TRUE;
   default:
      break;
   }
   return FALSE;
}


// ===========================================================================
//    Quiet Hours Main Settings Dialog
// ===========================================================================

static void LoadQuietHoursDlgTranslation(HWND hDlg)
{
   WMi18n &i18n = WMi18n::GetInstance();

   i18n.SetItemText(hDlg, IDC_QUIET_HOURS_DESCRIPTION, "settings.quiet-hours.intro");
   i18n.SetItemText(hDlg, IDC_ENABLEQUIETHOURS, "settings.quiet-hours.enable");
   i18n.SetItemText(hDlg, IDC_ENDPOINT_ADD, "settings.btn-add");
   i18n.SetItemText(hDlg, IDC_ENDPOINT_EDIT, "settings.btn-edit");
   i18n.SetItemText(hDlg, IDC_ENDPOINT_REMOVE, "settings.btn-remove");
   i18n.SetItemText(hDlg, IDC_ENDPOINT_REMOVEALL, "settings.btn-remove-all");


   //IDC_QUIET_HOURS_ADD
//IDC_QUIET_HOURS_EDIT
//IDC_QUIET_HOURS_REMOVE
   i18n.SetItemText(hDlg, IDC_FORCEUNMUTE, "settings.quiet-hours.force-unmute");
   i18n.SetItemText(hDlg, IDC_QUIET_HOURS_FORCE_UNMUTE_DESCRIPTION, "settings.quiet-hours.force-unmute-description");
   i18n.SetItemText(hDlg, IDC_SHOWNOTIFICATIONS, "settings.quiet-hours.show-notifications");
}

INT_PTR CALLBACK Settings_QuietHoursDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   UNREFERENCED_PARAMETER(lParam);
   switch (msg) {
   case WM_INITDIALOG:
   {
      HWND hEnable = GetDlgItem(hDlg, IDC_ENABLEQUIETHOURS);
      HWND hForce = GetDlgItem(hDlg, IDC_FORCEUNMUTE);
      HWND hNotify = GetDlgItem(hDlg, IDC_SHOWNOTIFICATIONS);
      HWND hQuietHoursTimes = GetDlgItem(hDlg, IDC_QUIET_HOURS_TIMES);
      HWND hAdd = GetDlgItem(hDlg, IDC_QUIET_HOURS_ADD);
      HWND hEdit = GetDlgItem(hDlg, IDC_QUIET_HOURS_EDIT);
      HWND hDelete = GetDlgItem(hDlg, IDC_QUIET_HOURS_REMOVE);
      HWND hDeleteAll = GetDlgItem(hDlg, IDC_QUIET_HOURS_REMOVEALL);

      WMSettings* settings = reinterpret_cast<WMSettings*>(lParam);
      assert(settings != nullptr);
      SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(settings));

      if (IsAppThemed()) {
         EnableThemeDialogTexture(hDlg, ETDT_ENABLETAB);
      }
      LoadQuietHoursDlgTranslation(hDlg);

      DWORD qhEnabled = !!settings->QueryValue(SettingsKey::QUIETHOURS_ENABLE);
      Button_SetCheck(hEnable, qhEnabled ? BST_CHECKED : BST_UNCHECKED);

      DWORD qhForceUnmute = !!settings->QueryValue(SettingsKey::QUIETHOURS_FORCEUNMUTE);
      Button_SetCheck(hForce, qhForceUnmute ? BST_CHECKED : BST_UNCHECKED);

      DWORD qhNotifications = !!settings->QueryValue(SettingsKey::QUIETHOURS_NOTIFICATIONS);
      Button_SetCheck(hNotify, qhNotifications ? BST_CHECKED : BST_UNCHECKED);

      EnableWindow(hForce, qhEnabled);
      EnableWindow(hNotify, qhEnabled);
      EnableWindow(hQuietHoursTimes, qhEnabled);
      EnableWindow(hAdd, qhEnabled);
      EnableWindow(hEdit, qhEnabled);
      EnableWindow(hDelete, qhEnabled);
      EnableWindow(hDeleteAll, qhEnabled);

      return TRUE;
   }
   case WM_SAVESETTINGS:
   {
      HWND hEnable  = GetDlgItem(hDlg, IDC_ENABLEQUIETHOURS);
      HWND hForce  = GetDlgItem(hDlg, IDC_FORCEUNMUTE);
      HWND hNotify = GetDlgItem(hDlg, IDC_SHOWNOTIFICATIONS);
      HWND hQuietHoursTimes = GetDlgItem(hDlg, IDC_QUIET_HOURS_TIMES);
      WMSettings* settings = reinterpret_cast<WMSettings*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
      int qhEnabled       = Button_GetCheck(hEnable) == BST_CHECKED;
      int qhForceUnmute   = Button_GetCheck(hForce)  == BST_CHECKED;
      int qhNotifications = Button_GetCheck(hNotify) == BST_CHECKED;
      settings->SetValue(SettingsKey::QUIETHOURS_ENABLE, qhEnabled);

      // Get number of items from IDC_QUIET_HOURS_TIMES
      // Get each item from IDC_QUIET_HOURS_TIMES
      // Read meta data of each item
      const int entries = ListView_GetItemCount(hQuietHoursTimes);
      std::vector<std::pair<DWORD, DWORD>> times(entries);
      for (int i = 0; i < entries; ++i) {
         LVITEM lvi{};
         lvi.iItem = i;
         lvi.mask = LVIF_PARAM;
         if (ListView_GetItem(hQuietHoursTimes, &lvi)) {
            QuietHoursEntry* qh_entry = reinterpret_cast<QuietHoursEntry*>(lvi.lParam);
            if (qh_entry != nullptr) {
               times[i] = std::make_pair(qh_entry->start, qh_entry->end);
            }
         }
      }

      if (SaveQuietHours(settings, qhEnabled, qhForceUnmute, qhNotifications, times)) {
         EndDialog(hDlg, 0);
      }
      return 0;
   }
   case WM_COMMAND:
   {
      HWND hEnable = GetDlgItem(hDlg, IDC_ENABLEQUIETHOURS);
      HWND hForce  = GetDlgItem(hDlg, IDC_FORCEUNMUTE);
      HWND hNotify = GetDlgItem(hDlg, IDC_SHOWNOTIFICATIONS);
      HWND hQuietHoursTimes  = GetDlgItem(hDlg, IDC_QUIET_HOURS_TIMES);
      HWND hAdd = GetDlgItem(hDlg, IDC_QUIET_HOURS_ADD);
      HWND hEdit = GetDlgItem(hDlg, IDC_QUIET_HOURS_EDIT);
      HWND hDelete = GetDlgItem(hDlg, IDC_QUIET_HOURS_REMOVE);
      HWND hDeleteAll = GetDlgItem(hDlg, IDC_QUIET_HOURS_REMOVEALL);
      
      if (LOWORD(wParam) == IDC_ENABLEQUIETHOURS) {
         int qhEnabled = Button_GetCheck(hEnable) == BST_CHECKED;
         EnableWindow(hQuietHoursTimes, qhEnabled);
         EnableWindow(hAdd, qhEnabled);
         EnableWindow(hEdit, qhEnabled);
         EnableWindow(hDelete, qhEnabled);
         EnableWindow(hDeleteAll, qhEnabled);
         EnableWindow(hNotify, qhEnabled);
         EnableWindow(hForce, qhEnabled);
         
      } else if (LOWORD(wParam) == IDC_QUIET_HOURS_ADD) {
         QuietHoursEntry qh_entry;
         if (DialogBoxParam(
               nullptr,
               MAKEINTRESOURCE(IDD_SETTINGS_QUIETHOURS_ADD),
               hDlg,
               Settings_QuietHoursAddDlgProc,
               reinterpret_cast<LPARAM>(&qh_entry)) == 0) {
            // Add entry to ListView
            // TODO
         }
      } else if (LOWORD(wParam) == IDC_QUIET_HOURS_EDIT) {
         // Get selected item
         // Open dialog with selected item data
         // Update ListView item
         // TODO
      } else if (LOWORD(wParam) == IDC_QUIET_HOURS_REMOVE) {
         // Get selected item
         // Remove from ListView
         // TODO
      } else if (LOWORD(wParam) == IDC_QUIET_HOURS_REMOVEALL) {
         ListView_DeleteAllItems(hQuietHoursTimes);
      }
      return 0;
   }
   case WM_CLOSE:
      EndDialog(hDlg, 0);
      return TRUE;
   default:
      break;
   }
   return FALSE;
}
