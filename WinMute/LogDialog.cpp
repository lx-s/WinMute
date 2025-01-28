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

static void TranslateLogDlgProc(HWND hDlg)
{
   WMi18n &i18n = WMi18n::GetInstance();
   SetWindowText(hDlg, i18n.GetTranslationW("log.title").c_str());
}

INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   AboutDlgData *dlgData =
      reinterpret_cast<AboutDlgData *>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
   switch (msg) {
   case WM_INITDIALOG:
   {
      dlgData = new AboutDlgData();
      SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dlgData));

      dlgData->hTabCtrl = GetDlgItem(hDlg, IDC_ABOUT_TAB);

      WMi18n &i18n = WMi18n::GetInstance();
      InsertTabItem(dlgData->hTabCtrl, ABOUT_TAB_GENERAL, i18n.GetTranslationW("about.tab.winmute"));
      InsertTabItem(dlgData->hTabCtrl, ABOUT_TAB_LICENSE, i18n.GetTranslationW("about.tab.license"));

      TranslateAboutDlgProc(hDlg);

      dlgData->hTabs[ABOUT_TAB_GENERAL] = CreateDialog(
         nullptr,
         MAKEINTRESOURCE(IDD_ABOUT_WINMUTE),
         hDlg,
         About_GeneralDlgProc);
      dlgData->hTabs[ABOUT_TAB_LICENSE] = CreateDialog(
         nullptr,
         MAKEINTRESOURCE(IDD_ABOUT_LICENSE),
         hDlg,
         About_LicenseDlgProc);


      // Set title font
      LOGFONT font;
      font.lfHeight = 32;
      font.lfWidth = 0;
      font.lfEscapement = 0;
      font.lfOrientation = 0;
      font.lfWeight = FW_BOLD;
      font.lfItalic = false;
      font.lfUnderline = false;
      font.lfStrikeOut = false;
      font.lfEscapement = 0;
      font.lfOrientation = 0;
      font.lfOutPrecision = OUT_DEFAULT_PRECIS;
      font.lfClipPrecision = CLIP_STROKE_PRECIS | CLIP_MASK | CLIP_TT_ALWAYS | CLIP_LH_ANGLES;
      font.lfQuality = CLEARTYPE_QUALITY;
      font.lfPitchAndFamily = VARIABLE_PITCH | FF_ROMAN;
      wcscpy_s(font.lfFaceName, L"Segoe UI");

      dlgData->hTitleFont = CreateFontIndirect(&font);
      SendMessage(
         hTitle,
         WM_SETFONT,
         reinterpret_cast<WPARAM>(dlgData->hTitleFont),
         TRUE);

      HICON hIcon = LoadIcon(
         GetModuleHandle(nullptr),
         MAKEINTRESOURCE(IDI_TRAY_DARK));
      SendMessageW(hDlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));

      return TRUE;
   }
   case WM_COMMAND:
      return 0;
   case WM_DESTROY:
      DeleteObject(dlgData->hTitleFont);
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
