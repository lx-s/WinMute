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
   const LPSYSTEMTIME start,
   const LPSYSTEMTIME end)
{
   if (enabled) {
      if (!IsValidTimeRange(start, end)) {
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
      }
   }

   int setStart = start->wHour * 3600 + start->wMinute * 60 + start->wSecond;
   int setEnd = end->wHour * 3600 + end->wMinute * 60 + end->wSecond;

   if (!settings->SetValue(SettingsKey::QUIETHOURS_ENABLE, enabled)
       || !settings->SetValue(SettingsKey::QUIETHOURS_FORCEUNMUTE, forceUnmute)
       || !settings->SetValue(SettingsKey::QUIETHOURS_NOTIFICATIONS, showNotifications)
       || !settings->SetValue(SettingsKey::QUIETHOURS_START, setStart)
       || !settings->SetValue(SettingsKey::QUIETHOURS_END, setEnd)) {
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

static void LoadQuietHoursDlgTranslation(HWND hDlg)
{
   WMi18n &i18n = WMi18n::GetInstance();

   i18n.SetItemText(hDlg, IDC_QUIET_HOURS_DESCRIPTION, "settings.quiet-hours.intro");
   i18n.SetItemText(hDlg, IDC_ENABLEQUIETHOURS, "settings.quiet-hours.enable");
   i18n.SetItemText(hDlg, IDC_QUIET_HOURS_START_LABEL, "settings.quiet-hours.start-time-label");
   i18n.SetItemText(hDlg, IDC_QUIET_HOURS_END_LABEL, "settings.quiet-hours.end-time-label");
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
      HWND hStart = GetDlgItem(hDlg, IDC_QUIETHOURS_START);
      HWND hEnd = GetDlgItem(hDlg, IDC_QUIETHOURS_END);

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
      EnableWindow(hStart, qhEnabled);
      EnableWindow(hEnd, qhEnabled);

      int setStart = settings->QueryValue(SettingsKey::QUIETHOURS_START);
      int setEnd = settings->QueryValue(SettingsKey::QUIETHOURS_END);

      SYSTEMTIME start;
      GetLocalTime(&start);
      if (setStart > 0) {
         start.wSecond = static_cast<WORD>(setStart % 60);
         start.wMinute = static_cast<WORD>(((setStart - start.wSecond) / 60) % 60);
         start.wHour = static_cast<WORD>((setStart - start.wMinute - start.wSecond) / 3600);
      }
      DateTime_SetSystemtime(hStart, GDT_VALID, &start);

      SYSTEMTIME end;
      GetLocalTime(&end);
      if (setEnd > 0) {
         end.wSecond = static_cast<WORD>(setEnd % 60);
         end.wMinute = static_cast<WORD>(((setEnd - end.wSecond) / 60) % 60);
         end.wHour = static_cast<WORD>((setEnd - end.wMinute - end.wSecond) / 3600);
      }
      DateTime_SetSystemtime(hEnd, GDT_VALID, &end);

      return TRUE;
   }
   case WM_SAVESETTINGS:
   {
      SYSTEMTIME start;
      SYSTEMTIME end;
      HWND hEnable  = GetDlgItem(hDlg, IDC_ENABLEQUIETHOURS);
      HWND hForce  = GetDlgItem(hDlg, IDC_FORCEUNMUTE);
      HWND hNotify = GetDlgItem(hDlg, IDC_SHOWNOTIFICATIONS);
      HWND hStart  = GetDlgItem(hDlg, IDC_QUIETHOURS_START);
      HWND hEnd    = GetDlgItem(hDlg, IDC_QUIETHOURS_END);
      WMSettings* settings = reinterpret_cast<WMSettings*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
      int qhEnabled       = Button_GetCheck(hEnable) == BST_CHECKED;
      int qhForceUnmute   = Button_GetCheck(hForce)  == BST_CHECKED;
      int qhNotifications = Button_GetCheck(hNotify) == BST_CHECKED;
      DateTime_GetSystemtime(hStart, &start);
      DateTime_GetSystemtime(hEnd, &end);
      settings->SetValue(SettingsKey::QUIETHOURS_ENABLE, qhEnabled);
      if (SaveQuietHours(settings, qhEnabled, qhForceUnmute, qhNotifications, &start, &end)) {
         EndDialog(hDlg, 0);
      }
      return 0;
   }
   case WM_COMMAND:
   {
      HWND hEnable = GetDlgItem(hDlg, IDC_ENABLEQUIETHOURS);
      HWND hForce  = GetDlgItem(hDlg, IDC_FORCEUNMUTE);
      HWND hNotify = GetDlgItem(hDlg, IDC_SHOWNOTIFICATIONS);
      HWND hStart  = GetDlgItem(hDlg, IDC_QUIETHOURS_START);
      HWND hEnd    = GetDlgItem(hDlg, IDC_QUIETHOURS_END);
      if (LOWORD(wParam) == IDC_ENABLEQUIETHOURS) {
         int qhEnabled = Button_GetCheck(hEnable) == BST_CHECKED;
         EnableWindow(hStart, qhEnabled);
         EnableWindow(hEnd, qhEnabled);
         EnableWindow(hNotify, qhEnabled);
         EnableWindow(hForce, qhEnabled);
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
