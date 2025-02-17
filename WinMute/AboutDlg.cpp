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

static const std::wstring licenseText = L"\
WinMute\r\n\
Copyright(c) 2025, Alexander Steinhoefer\r\n\
\r\n\
-----------------------------------------------------------------------------\r\n\
Redistribution and use in source and binary forms, with or without\r\n\
modification, are permitted provided that the following conditions are met:\r\n\
\r\n\
 * Redistributions of source code must retain the above copyright notice,\r\n\
   this list of conditions and the following disclaimer.\r\n\
\r\n\
 * Redistributions in binary form must reproduce the above copyright\r\n\
   notice, this list of conditions and the following disclaimer in the\r\n\
   documentation and /or other materials provided with the distribution.\r\n\
\r\n\
 * Neither the name of the author nor the names of its contributors may\r\n\
   be used to endorse or promote products derived from this software\r\n\
   without specific prior written permission.\r\n\
\r\n\
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\r\n\
\"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\r\n\
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS\r\n\
FOR A PARTICULAR PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL THE\r\n\
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,\r\n\
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING,\r\n\
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\r\n\
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\r\n\
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT\r\n\
LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY\r\n\
WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE\r\n\
POSSIBILITY OF SUCH DAMAGE.\r\n\
";

enum AboutTabsIDs {
   ABOUT_TAB_GENERAL = 0,
   ABOUT_TAB_LICENSE,
   ABOUT_TAB_COUNT
};

struct AboutDlgData {
   HWND hTabCtrl;
   std::array<HWND, ABOUT_TAB_COUNT> hTabs;
   HWND hActiveTab;
   HFONT hTitleFont;

   AboutDlgData() noexcept
      : hTabCtrl{nullptr}, hTabs{nullptr}, hActiveTab{nullptr}, hTitleFont{nullptr}
   {
   }
};

static void InsertTabItem(HWND hTabCtrl, UINT id, const std::wstring& itemName)
{
   constexpr int bufSize = 50;
   wchar_t buf[bufSize] = { L'\0' };
   TC_ITEM tcItem;
   ZeroMemory(&tcItem, sizeof(tcItem));
   tcItem.mask |= TCIF_TEXT;
   StringCchCopyW(buf, bufSize, itemName.c_str());
   tcItem.pszText = buf;
   tcItem.cchTextMax = bufSize;

   TabCtrl_InsertItem(hTabCtrl, id, &tcItem);
}

static void SwitchTab(AboutDlgData* dlgData, HWND hNewTab) noexcept
{
   if (dlgData->hActiveTab != nullptr) {
      ShowWindow(dlgData->hActiveTab, SW_HIDE);
   }
   dlgData->hActiveTab = hNewTab;
   ShowWindow(dlgData->hActiveTab, SW_SHOW);
}

static void ResizeTabs(HWND hTabCtrl, std::span<HWND> tabs)
{
   RECT tabCtrlRect = { 0 };
   GetWindowRect(hTabCtrl, &tabCtrlRect);
   POINT tabCtrlPos = { 0 };
   tabCtrlPos.x = tabCtrlRect.left;
   tabCtrlPos.y = tabCtrlRect.top;
   ScreenToClient(GetParent(hTabCtrl), &tabCtrlPos);

   GetClientRect(hTabCtrl, &tabCtrlRect);
   TabCtrl_AdjustRect(hTabCtrl, FALSE, &tabCtrlRect);
   tabCtrlRect.left += tabCtrlPos.x;
   tabCtrlRect.top += tabCtrlPos.y;

   HDWP hdwp = BeginDeferWindowPos(static_cast<int>(tabs.size()));
   if (hdwp == nullptr) {
      ShowWindowsError(L"BeginDeferWindowPos", GetLastError());
   } else {
      for (auto hTab : tabs) {
         HDWP newHdwp = DeferWindowPos(
            hdwp,
            hTab,
            HWND_TOP,
            tabCtrlRect.left,
            tabCtrlRect.top,
            tabCtrlRect.right - tabCtrlRect.left,
            tabCtrlRect.bottom - tabCtrlRect.top,
            0);
         if (newHdwp == nullptr) {
            ShowWindowsError(L"DeferWindowPos", GetLastError());
            break;
         } else {
            hdwp = newHdwp;
         }
      }
      EndDeferWindowPos(hdwp);
   }
}

static void TranslateAboutGeneralDlgProc(HWND hDlg)
{
   WMi18n &i18n = WMi18n::GetInstance(); 

   const auto hpLink = std::vformat(
      std::wstring_view(L"<a href=\"https://www.lx-s.de\">{}</a>"),
      std::make_wformat_args(i18n.GetTranslationW("about.general.author-site-label")));

   const auto projLink = std::vformat(
      L"<a href=\"https://github.com/lx-s/WinMute/\">{}</a>",
      std::make_wformat_args(i18n.GetTranslationW("about.general.project-site-label")));

   const auto supportLink = std::vformat(
      L"<a href=\"https://github.com/lx-s/WinMute/issues/\">{}</a>",
      std::make_wformat_args(i18n.GetTranslationW("about.general.support-label")));

   SetDlgItemTextW(hDlg, IDC_LINK_HOMEPAGE, hpLink.c_str());
   SetDlgItemTextW(hDlg, IDC_LINK_PROJECT, projLink.c_str());
   SetDlgItemTextW(hDlg, IDC_LINK_TICKETS, supportLink.c_str());

   i18n.SetItemText(hDlg, IDC_ABOUTTEXT, "about.general.description");
}

static INT_PTR CALLBACK About_GeneralDlgProc(HWND hDlg, UINT msg, WPARAM, LPARAM lParam) noexcept
{
   switch (msg) {
   case WM_INITDIALOG:
      if (IsAppThemed()) {
         EnableThemeDialogTexture(hDlg, ETDT_ENABLETAB);
      }
      TranslateAboutGeneralDlgProc(hDlg);
      return TRUE;
   case WM_NOTIFY: {
      const PNMLINK pNmLink = reinterpret_cast<PNMLINK>(lParam);
#pragma warning(push)
#pragma warning(disable : 26454) // Disable arithmetic overflow warning for NM_CLICK and NM_RETURN
      if (pNmLink->hdr.code == NM_CLICK ||
          pNmLink->hdr.code == NM_RETURN) {
#pragma warning(pop)
         const UINT_PTR ctrlId = pNmLink->hdr.idFrom;
         const LITEM item = pNmLink->item;
         if ((ctrlId == IDC_LINK_HOMEPAGE || ctrlId == IDC_LINK_TICKETS || ctrlId == IDC_LINK_PROJECT)
            && item.iLink == 0) {
            LaunchBrowser(hDlg, item.szUrl);
         }
      }
      return TRUE;
   }
   default:
      break;
   }
   return FALSE;
}

static INT_PTR CALLBACK About_LicenseDlgProc(HWND hDlg, UINT msg, WPARAM, LPARAM) noexcept
{
   switch (msg) {
   case WM_INITDIALOG:
      if (IsAppThemed()) {
         EnableThemeDialogTexture(hDlg, ETDT_ENABLETAB);
      }
      Edit_SetText(GetDlgItem(hDlg, IDC_LICENSETEXT), licenseText.c_str());
      return TRUE;
   default:
      break;
   }
   return FALSE;
}

static void TranslateAboutDlgProc(HWND hDlg)
{
   WMi18n &i18n = WMi18n::GetInstance();
   SetWindowText(hDlg, i18n.GetTranslationW("about.title").c_str());
   i18n.SetItemText(hDlg, IDOK, "about.btn-close");
}

INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   AboutDlgData* dlgData =
      reinterpret_cast<AboutDlgData*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
   switch (msg) {
   case WM_INITDIALOG: {
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

      // Init tab pages
      ResizeTabs(dlgData->hTabCtrl, std::span{dlgData->hTabs});
      for (auto hTab : dlgData->hTabs) {
         ShowWindow(hTab, SW_HIDE);
      }
      SwitchTab(dlgData, dlgData->hTabs[ABOUT_TAB_GENERAL]);

      HWND hTitle = GetDlgItem(hDlg, IDC_ABOUT_TITLE);
      std::wstring progVers;
      if (GetWinMuteVersion(progVers)) {
         std::wstring progName = std::wstring{ L"WinMute " } + progVers;
         Static_SetText(hTitle, progName.c_str());
      }

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
   case WM_NOTIFY: {
      const LPNMHDR lpnmhdr = reinterpret_cast<LPNMHDR>(lParam);
#pragma warning(push)
#pragma warning(disable : 26454) // Disable arithmetic overflow for TCN_SELCHANGE
      if (lpnmhdr->code == TCN_SELCHANGE && lpnmhdr->hwndFrom == dlgData->hTabCtrl) {
#pragma warning(pop)
         const int curSel = TabCtrl_GetCurSel(dlgData->hTabCtrl);
         if (curSel >= 0 && curSel < ABOUT_TAB_COUNT) {
            SwitchTab(dlgData, dlgData->hTabs[curSel]);
         }
      }
      return 0;
   }
   case WM_COMMAND:
      if (LOWORD(wParam) == IDOK) {
         EndDialog(hDlg, 0);
      }
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
