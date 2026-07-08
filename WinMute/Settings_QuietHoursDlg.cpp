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

struct QuietHoursEntry
{
   DWORD start = 0;
   DWORD end = 0;
};

struct QuietHoursDlgData
{
   WMSettings *settings = nullptr;
};

static bool IsValidTimeRange(
    const LPSYSTEMTIME start,
    const LPSYSTEMTIME end)
{
   return !(start->wHour == end->wHour && start->wMinute == end->wMinute && start->wSecond == end->wSecond);
}

static std::wstring FormatQuietHoursTime(DWORD seconds)
{
   const auto h = seconds / 3600;
   const auto m = (seconds % 3600) / 60;
   const auto s = (seconds % 3600) % 60;
   return std::format(L"{:02}:{:02}:{:02}", h,m,s);
}

static void AddQuietHoursEntryToListView(HWND hListView, QuietHoursEntry *entry)
{
   auto startText = FormatQuietHoursTime(entry->start);
   auto endText = FormatQuietHoursTime(entry->end);

   LVITEM lvi{};
   lvi.mask = LVIF_TEXT | LVIF_PARAM;
   lvi.iItem = ListView_GetItemCount(hListView);
   lvi.iSubItem = 0;
   lvi.pszText = const_cast<LPWSTR>(startText.c_str());
   lvi.lParam = reinterpret_cast<LPARAM>(entry);
   const int idx = ListView_InsertItem(hListView, &lvi);
   ListView_SetItemText(hListView, idx, 1, const_cast<LPWSTR>(endText.c_str()));
}

static void UpdateListViewItem(HWND hListView, int idx, const QuietHoursEntry *entry)
{
   auto startText = FormatQuietHoursTime(entry->start);
   auto endText = FormatQuietHoursTime(entry->end);
   ListView_SetItemText(hListView, idx, 0, const_cast<LPWSTR>(startText.c_str()));
   ListView_SetItemText(hListView, idx, 1, const_cast<LPWSTR>(endText.c_str()));
}

static void FreeListViewEntries(HWND hListView)
{
   const int count = ListView_GetItemCount(hListView);
   for (int i = 0; i < count; ++i)
   {
      LVITEM lvi{};
      lvi.mask = LVIF_PARAM;
      lvi.iItem = i;
      if (ListView_GetItem(hListView, &lvi) && lvi.lParam != 0) {
         delete reinterpret_cast<QuietHoursEntry *>(lvi.lParam);
      }
   }
}

static bool TimeInWindow(DWORD t, DWORD wStart, DWORD wEnd) noexcept
{
   if (wStart < wEnd) {
      return t >= wStart && t < wEnd;
   } else { // wraps midnight
      return t >= wStart || t < wEnd;
   }
}

static bool TimeWindowsOverlap(DWORD s1, DWORD e1, DWORD s2, DWORD e2) noexcept
{
   // Two circular intervals overlap iff one contains the start of the other
   return TimeInWindow(s1, s2, e2) || TimeInWindow(s2, s1, e1);
}

static bool ShowOverlapError(HWND hParent)
{
   WMi18n &i18n = WMi18n::GetInstance();
   TaskDialog(
       hParent,
       nullptr,
       PROGRAM_NAME,
       i18n.GetTranslationW("settings.quiet-hours.error.overlapping-time-range.title").c_str(),
       i18n.GetTranslationW("settings.quiet-hours.error.overlapping-time-range.text").c_str(),
       TDCBF_OK_BUTTON,
       TD_WARNING_ICON,
       nullptr);
   return false;
}

static bool HasOverlapWithExistingEntries(
    HWND hListView,
    const QuietHoursEntry *newEntry,
    int excludeIdx)
{
   const int count = ListView_GetItemCount(hListView);
   for (int i = 0; i < count; ++i) {
      if (i == excludeIdx) {
         continue;
      }
      LVITEM lvi{};
      lvi.mask = LVIF_PARAM;
      lvi.iItem = i;
      if (ListView_GetItem(hListView, &lvi) && lvi.lParam != 0) {
         const auto *existing = reinterpret_cast<const QuietHoursEntry *>(lvi.lParam);
         if (TimeWindowsOverlap(newEntry->start, newEntry->end, existing->start, existing->end)) {
            return true;
         }
      }
   }
   return false;
}

static bool SaveQuietHours(
    WMSettings *settings,
    const int enabled,
    const int forceUnmute,
    const int showNotifications,
    const std::vector<std::pair<DWORD, DWORD>> &times)
{
   if (!settings->SetValue(SettingsKey::QUIETHOURS_ENABLE, enabled) ||
       !settings->SetValue(SettingsKey::QUIETHOURS_FORCEUNMUTE, forceUnmute) ||
       !settings->SetValue(SettingsKey::QUIETHOURS_NOTIFICATIONS, showNotifications) ||
       !settings->StoreQuietHoursTimes(times)) {
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

// ===========================================================================
//    Add / Edit sub-dialog
// ===========================================================================

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
   GetLocalTime(&tp);
   if (savedTime > 0) {
      tp.wSecond = static_cast<WORD>(savedTime % 60);
      tp.wMinute = static_cast<WORD>(((savedTime - tp.wSecond) / 60) % 60);
      tp.wHour = static_cast<WORD>((savedTime - tp.wMinute * 60 - tp.wSecond) / 3600);
   }
   DateTime_SetSystemtime(hPicker, GDT_VALID, &tp);
}

static INT_PTR CALLBACK Settings_QuietHoursAddDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg)
   {
   case WM_INITDIALOG:
   {
      HWND hStart = GetDlgItem(hDlg, IDC_QUIET_HOURS_START_PICKER);
      HWND hEnd = GetDlgItem(hDlg, IDC_QUIET_HOURS_END_PICKER);

      QuietHoursEntry *qh_entry = reinterpret_cast<QuietHoursEntry *>(lParam);
      if (qh_entry == nullptr) {
         return FALSE;
      }
      SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(qh_entry));

      const bool isEdit = qh_entry->start != 0 || qh_entry->end != 0;
      LoadQuietHoursAddDlgTranslation(hDlg, isEdit);

      if (qh_entry->start != 0) {
         SetQuietHourPickerTime(hStart, qh_entry->start);
      }
      if (qh_entry->end != 0) {
         SetQuietHourPickerTime(hEnd, qh_entry->end);
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
            return FALSE;
         } else {
            auto *qh_entry = reinterpret_cast<QuietHoursEntry *>(GetWindowLongPtr(hDlg, DWLP_USER));
            qh_entry->start = start.wHour * 3600 + start.wMinute * 60 + start.wSecond;
            qh_entry->end = end.wHour * 3600 + end.wMinute * 60 + end.wSecond;
            EndDialog(hDlg, 0);
         }
      }
      else if (LOWORD(wParam) == IDCANCEL)
      {
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
   i18n.SetItemText(hDlg, IDC_QUIET_HOURS_ADD, "settings.btn-add");
   i18n.SetItemText(hDlg, IDC_QUIET_HOURS_EDIT, "settings.btn-edit");
   i18n.SetItemText(hDlg, IDC_QUIET_HOURS_REMOVE, "settings.btn-remove");
   i18n.SetItemText(hDlg, IDC_QUIET_HOURS_REMOVEALL, "settings.btn-remove-all");
   i18n.SetItemText(hDlg, IDC_FORCEUNMUTE, "settings.quiet-hours.force-unmute");
   i18n.SetItemText(hDlg, IDC_QUIET_HOURS_FORCE_UNMUTE_DESCRIPTION, "settings.quiet-hours.force-unmute-description");
   i18n.SetItemText(hDlg, IDC_SHOWNOTIFICATIONS, "settings.quiet-hours.show-notifications");
}

static void SetupListView(HWND hListView)
{
   ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT);

   auto &i18n = WMi18n::GetInstance();
   const auto textBegin = i18n.GetTranslationW("settings.quiet-hours.start-time-label");
   const auto textEnd = i18n.GetTranslationW("settings.quiet-hours.end-time-label");

   LVCOLUMN lvCol{};
   lvCol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_ORDER;
   lvCol.fmt = LVCFMT_LEFT;
   lvCol.cx = 75;
   lvCol.pszText = const_cast<LPWSTR>(textBegin.c_str());
   lvCol.cchTextMax = static_cast<int>(textBegin.length());
   lvCol.iOrder = 0;
   ListView_InsertColumn(hListView, 0, &lvCol);

   lvCol.cx = 75;
   lvCol.pszText = const_cast<LPWSTR>(textEnd.c_str());
   lvCol.cchTextMax = static_cast<int>(textEnd.length());
   lvCol.iOrder = 1;
   ListView_InsertColumn(hListView, 1, &lvCol);
}

static void UpdateButtonStates(HWND hDlg, HWND hListView, bool qhEnabled)
{
   const bool hasSelection = ListView_GetNextItem(hListView, -1, LVNI_SELECTED) != -1;
   const bool hasItems = ListView_GetItemCount(hListView) > 0;
   EnableWindow(GetDlgItem(hDlg, IDC_QUIET_HOURS_ADD), qhEnabled);
   EnableWindow(GetDlgItem(hDlg, IDC_QUIET_HOURS_EDIT), qhEnabled && hasSelection);
   EnableWindow(GetDlgItem(hDlg, IDC_QUIET_HOURS_REMOVE), qhEnabled && hasSelection);
   EnableWindow(GetDlgItem(hDlg, IDC_QUIET_HOURS_REMOVEALL), qhEnabled && hasItems);
}

INT_PTR CALLBACK Settings_QuietHoursDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg)
   {
   case WM_INITDIALOG:
   {
      HWND hEnable = GetDlgItem(hDlg, IDC_ENABLEQUIETHOURS);
      HWND hForce = GetDlgItem(hDlg, IDC_FORCEUNMUTE);
      HWND hNotify = GetDlgItem(hDlg, IDC_SHOWNOTIFICATIONS);
      HWND hQuietHoursTimes = GetDlgItem(hDlg, IDC_QUIET_HOURS_TIMES);

      auto settings = reinterpret_cast<WMSettings *>(lParam);
      assert(settings != nullptr);

      auto qhdata = new QuietHoursDlgData();
      assert(qhdata != nullptr);
      qhdata->settings = settings;
      SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(qhdata));

      if (IsAppThemed())
      {
         EnableThemeDialogTexture(hDlg, ETDT_ENABLETAB);
      }
      LoadQuietHoursDlgTranslation(hDlg);

      const DWORD qhEnabled = !!settings->QueryValue(SettingsKey::QUIETHOURS_ENABLE);
      Button_SetCheck(hEnable, qhEnabled ? BST_CHECKED : BST_UNCHECKED);

      const DWORD qhForceUnmute = !!settings->QueryValue(SettingsKey::QUIETHOURS_FORCEUNMUTE);
      Button_SetCheck(hForce, qhForceUnmute ? BST_CHECKED : BST_UNCHECKED);

      const DWORD qhNotifications = !!settings->QueryValue(SettingsKey::QUIETHOURS_NOTIFICATIONS);
      Button_SetCheck(hNotify, qhNotifications ? BST_CHECKED : BST_UNCHECKED);

      SetupListView(hQuietHoursTimes);

      for (const auto &t : settings->GetQuietHoursTimes()) {
         auto *entry = new QuietHoursEntry();
         entry->start = t.first;
         entry->end = t.second;
         AddQuietHoursEntryToListView(hQuietHoursTimes, entry);
      }

      EnableWindow(hForce, qhEnabled);
      EnableWindow(hNotify, qhEnabled);
      EnableWindow(hQuietHoursTimes, qhEnabled);
      UpdateButtonStates(hDlg, hQuietHoursTimes, !!qhEnabled);

      return TRUE;
   }
   case WM_NOTIFY:
   {
      const NMHDR *pnmh = reinterpret_cast<const NMHDR *>(lParam);
      if (pnmh->idFrom == IDC_QUIET_HOURS_TIMES && pnmh->code == LVN_ITEMCHANGED) {
         HWND hQuietHoursTimes = GetDlgItem(hDlg, IDC_QUIET_HOURS_TIMES);
         const bool qhEnabled = Button_GetCheck(GetDlgItem(hDlg, IDC_ENABLEQUIETHOURS)) == BST_CHECKED;
         UpdateButtonStates(hDlg, hQuietHoursTimes, qhEnabled);
      } else if (pnmh->idFrom == IDC_QUIET_HOURS_TIMES && pnmh->code == NM_DBLCLK) {
         auto nmItemActivate = reinterpret_cast<const LPNMITEMACTIVATE>(lParam);
         if (nmItemActivate->iItem != -1) {
            SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_QUIET_HOURS_EDIT, 0), 0);
         }
      }
      return 0;
   }
   case WM_SAVESETTINGS:
   {
      HWND hEnable = GetDlgItem(hDlg, IDC_ENABLEQUIETHOURS);
      HWND hForce = GetDlgItem(hDlg, IDC_FORCEUNMUTE);
      HWND hNotify = GetDlgItem(hDlg, IDC_SHOWNOTIFICATIONS);
      HWND hQuietHoursTimes = GetDlgItem(hDlg, IDC_QUIET_HOURS_TIMES);
      auto qhdata = reinterpret_cast<QuietHoursDlgData *>(GetWindowLongPtr(hDlg, DWLP_USER));

      const int qhEnabled = Button_GetCheck(hEnable) == BST_CHECKED;
      const int qhForceUnmute = Button_GetCheck(hForce) == BST_CHECKED;
      const int qhNotifications = Button_GetCheck(hNotify) == BST_CHECKED;

      const int entries = ListView_GetItemCount(hQuietHoursTimes);
      std::vector<std::pair<DWORD, DWORD>> times;
      times.reserve(entries);
      for (int i = 0; i < entries; ++i) {
         LVITEM lvi{};
         lvi.iItem = i;
         lvi.mask = LVIF_PARAM;
         if (ListView_GetItem(hQuietHoursTimes, &lvi) && lvi.lParam != 0) {
            const QuietHoursEntry *qh_entry = reinterpret_cast<QuietHoursEntry *>(lvi.lParam);
            times.emplace_back(qh_entry->start, qh_entry->end);
         }
      }

      if (SaveQuietHours(qhdata->settings, qhEnabled, qhForceUnmute, qhNotifications, times)) {
         FreeListViewEntries(hQuietHoursTimes);
         delete qhdata;
         EndDialog(hDlg, 0);
      }
      return 0;
   }
   case WM_COMMAND:
   {
      HWND hEnable = GetDlgItem(hDlg, IDC_ENABLEQUIETHOURS);
      HWND hForce = GetDlgItem(hDlg, IDC_FORCEUNMUTE);
      HWND hNotify = GetDlgItem(hDlg, IDC_SHOWNOTIFICATIONS);
      HWND hQuietHoursTimes = GetDlgItem(hDlg, IDC_QUIET_HOURS_TIMES);

      if (LOWORD(wParam) == IDC_ENABLEQUIETHOURS) {
         const bool qhEnabled = Button_GetCheck(hEnable) == BST_CHECKED;
         EnableWindow(hForce, qhEnabled);
         EnableWindow(hNotify, qhEnabled);
         EnableWindow(hQuietHoursTimes, qhEnabled);
         UpdateButtonStates(hDlg, hQuietHoursTimes, qhEnabled);
      } else if (LOWORD(wParam) == IDC_QUIET_HOURS_ADD) {
         auto *qh_entry = new QuietHoursEntry();
         if (DialogBoxParam(
                 nullptr,
                 MAKEINTRESOURCE(IDD_SETTINGS_QUIETHOURS_ADD),
                 hDlg,
                 Settings_QuietHoursAddDlgProc,
                 reinterpret_cast<LPARAM>(qh_entry)) == 0) {
            if (HasOverlapWithExistingEntries(hQuietHoursTimes, qh_entry, -1)) {
               ShowOverlapError(hDlg);
               delete qh_entry;
            } else {
               AddQuietHoursEntryToListView(hQuietHoursTimes, qh_entry);
               UpdateButtonStates(hDlg, hQuietHoursTimes, true);
            }
         } else {
            delete qh_entry;
         }
      } else if (LOWORD(wParam) == IDC_QUIET_HOURS_EDIT) {
         const int sel = ListView_GetNextItem(hQuietHoursTimes, -1, LVNI_SELECTED);
         if (sel != -1) {
            LVITEM lvi{};
            lvi.mask = LVIF_PARAM;
            lvi.iItem = sel;
            if (ListView_GetItem(hQuietHoursTimes, &lvi) && lvi.lParam != 0) {
               QuietHoursEntry *entry = reinterpret_cast<QuietHoursEntry *>(lvi.lParam);
               QuietHoursEntry editCopy = *entry;
               if (DialogBoxParam(
                       nullptr,
                       MAKEINTRESOURCE(IDD_SETTINGS_QUIETHOURS_ADD),
                       hDlg,
                       Settings_QuietHoursAddDlgProc,
                       reinterpret_cast<LPARAM>(&editCopy)) == 0) {
                  if (HasOverlapWithExistingEntries(hQuietHoursTimes, &editCopy, sel)) {
                     ShowOverlapError(hDlg);
                  } else {
                     entry->start = editCopy.start;
                     entry->end = editCopy.end;
                     UpdateListViewItem(hQuietHoursTimes, sel, entry);
                  }
               }
            }
         }
      } else if (LOWORD(wParam) == IDC_QUIET_HOURS_REMOVE) {
         const int sel = ListView_GetNextItem(hQuietHoursTimes, -1, LVNI_SELECTED);
         if (sel != -1)
         {
            LVITEM lvi{};
            lvi.mask = LVIF_PARAM;
            lvi.iItem = sel;
            if (ListView_GetItem(hQuietHoursTimes, &lvi) && lvi.lParam != 0)
            {
               delete reinterpret_cast<QuietHoursEntry *>(lvi.lParam);
            }
            ListView_DeleteItem(hQuietHoursTimes, sel);
            UpdateButtonStates(hDlg, hQuietHoursTimes, true);
         }
      } else if (LOWORD(wParam) == IDC_QUIET_HOURS_REMOVEALL) {
         FreeListViewEntries(hQuietHoursTimes);
         ListView_DeleteAllItems(hQuietHoursTimes);
         UpdateButtonStates(hDlg, hQuietHoursTimes, true);
      }
      return 0;
   }
   case WM_CLOSE:
   {
      HWND hQuietHoursTimes = GetDlgItem(hDlg, IDC_QUIET_HOURS_TIMES);
      FreeListViewEntries(hQuietHoursTimes);
      auto qhdata = reinterpret_cast<QuietHoursDlgData *>(GetWindowLongPtr(hDlg, DWLP_USER));
      delete qhdata;
      EndDialog(hDlg, 0);
      return TRUE;
   }
   default:
      break;
   }
   return FALSE;
}
