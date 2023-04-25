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

INT_PTR CALLBACK Settings_GeneralDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) {
   case WM_INITDIALOG: {
      HWND hAutostart = GetDlgItem(hDlg, IDC_RUNONSTARTUP);
      HWND hLogging = GetDlgItem(hDlg, IDC_ENABLELOGGING);
      HWND hOpenLog = GetDlgItem(hDlg, IDC_OPENLOG);

      if (IsAppThemed()) {
         EnableThemeDialogTexture(hDlg, ETDT_ENABLETAB);
      }

      WMSettings* settings = reinterpret_cast<WMSettings*>(lParam);
      assert(settings != nullptr);
      SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(settings));

      DWORD enabled = settings->IsAutostartEnabled();
      Button_SetCheck(hAutostart, enabled ? BST_CHECKED : BST_UNCHECKED);

      enabled = !!settings->QueryValue(SettingsKey::LOGGING_ENABLED);
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
      } else if (LOWORD(wParam) == IDC_OPENLOG) {
         WMLog& log = WMLog::GetInstance();
         const std::wstring filePath = log.GetLogFilePath().c_str();
         ShellExecuteW(nullptr, L"open", filePath.c_str(), nullptr, nullptr, SW_SHOW);
      }
      return 0;
   }
   case WM_SAVESETTINGS: {
      WMSettings* settings = reinterpret_cast<WMSettings*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));

      HWND hAutostart = GetDlgItem(hDlg, IDC_RUNONSTARTUP);
      HWND hLogging = GetDlgItem(hDlg, IDC_ENABLELOGGING);

      int enableLog = Button_GetCheck(hLogging) == BST_CHECKED;
      settings->SetValue(SettingsKey::LOGGING_ENABLED, enableLog);
      WMLog::GetInstance().SetEnabled(enableLog);

      if (Button_GetCheck(hAutostart) == BST_CHECKED) {
         settings->EnableAutostart(true);
      } else {
         settings->EnableAutostart(false);
      }

      return 0;
   }
   default:
      break;
   }
   return FALSE;
}
