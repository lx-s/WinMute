/*
 WinMute
           Copyright (c) 2024, Alexander Steinhoefer

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

struct LogDlgData {
};

INT_PTR CALLBACK LogDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   auto *dlgData =
      reinterpret_cast<LogDlgData *>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
   UNREFERENCED_PARAMETER(wParam);
   UNREFERENCED_PARAMETER(lParam);
   switch (msg) {
   case WM_INITDIALOG:
   {
      dlgData = new LogDlgData();
      SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dlgData));

      WMi18n &i18n = WMi18n::GetInstance();
      SetWindowText(hDlg, i18n.GetTranslationW("log.title").c_str());

      HICON hIcon = LoadIcon(
         GetModuleHandle(nullptr),
         MAKEINTRESOURCE(IDI_TRAY_DARK));
      SendMessageW(hDlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));

      return TRUE;
   }
   case WM_COMMAND:
      return 0;
   case WM_DESTROY:
      delete dlgData;
      SetWindowLongPtrW(hDlg, GWLP_USERDATA, 0);
      return 0;
   case WM_CLOSE:
      EndDialog(hDlg, 0);
      return TRUE;
   default:
      break;
   }
   return FALSE;
}
