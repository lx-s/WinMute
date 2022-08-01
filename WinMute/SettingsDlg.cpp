/*
 WinMute
           Copyright (c) 2022, Alexander Steinhoefer

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

enum SettingsTabsIDs {
   SETTINGS_TAB_GENERAL = 0,
   SETTINGS_TAB_MUTE,
   SETTINGS_TAB_QUIETHOURS,
   SETTINGS_TAB_WIFI,
   SETTINGS_TAB_BLUETOOTH,
   SETTINGS_TAB_COUNT
};

struct SettingsDlgData {
   HWND hTabCtrl;
   HWND hTabs[SETTINGS_TAB_COUNT];

   HWND hActiveTab;
   WMSettings* settings;

   SettingsDlgData(WMSettings* settings)
      : settings(settings), hTabCtrl(NULL), hActiveTab(NULL)
   {
      ZeroMemory(hTabs, sizeof(hTabs));
   }
};

extern INT_PTR CALLBACK Settings_QuietHoursDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR CALLBACK Settings_GeneralDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR CALLBACK Settings_MuteDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR CALLBACK Settings_BluetoothDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR CALLBACK Settings_WifiDlgProc(HWND, UINT, WPARAM, LPARAM);

extern HINSTANCE hglobInstance;

static void InsertTabItem(HWND hTabCtrl, UINT id, const TCHAR* itemName)
{
   constexpr int bufSize = 50;
   TCHAR buf[bufSize];
   TC_ITEM tcItem;
   ZeroMemory(&tcItem, sizeof(tcItem));
   tcItem.mask |= TCIF_TEXT;
   StringCchCopy(buf, bufSize, itemName);
   tcItem.pszText = buf;
   tcItem.cchTextMax = bufSize;

   TabCtrl_InsertItem(hTabCtrl, id, &tcItem);
}

static void SwitchTab(SettingsDlgData* dlgData, HWND hNewTab)
{
   if (dlgData->hActiveTab != NULL) {
      ShowWindow(dlgData->hActiveTab, SW_HIDE);
   }
   dlgData->hActiveTab = hNewTab;
   ShowWindow(dlgData->hActiveTab, SW_SHOW);
   //SetFocus(dlgData->hActiveTab);
}

static void ResizeTabs(HWND hTabCtrl, HWND* hTabs, int tabCount)
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

   HDWP hdwp = BeginDeferWindowPos(tabCount);
   if (hdwp == NULL) {
      PrintWindowsError(_T("BeginDeferWindowPos"), GetLastError());
   } else {
      for (int i = 0; i < tabCount; ++i) {
         HDWP newHdwp = DeferWindowPos(
            hdwp,
            hTabs[i],
            HWND_TOP,
            tabCtrlRect.left,
            tabCtrlRect.top,
            tabCtrlRect.right - tabCtrlRect.left,
            tabCtrlRect.bottom - tabCtrlRect.top,
            0);
         if (newHdwp == NULL) {
            PrintWindowsError(_T("DeferWindowPos"), GetLastError());
            break;
         } else {
            hdwp = newHdwp;
         }
      }
      EndDeferWindowPos(hdwp);
   }
}

INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   SettingsDlgData* dlgData =
      reinterpret_cast<SettingsDlgData*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
   switch (msg) {
   case WM_INITDIALOG: {
      assert(dlgData == nullptr);
      WMSettings* settings = reinterpret_cast<WMSettings*>(lParam);
      dlgData = new SettingsDlgData(settings);
      SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dlgData));
      
      dlgData->hTabCtrl = GetDlgItem(hDlg, IDC_SETTINGS_TAB);

      InsertTabItem(dlgData->hTabCtrl, SETTINGS_TAB_GENERAL, _T("General"));
      InsertTabItem(dlgData->hTabCtrl, SETTINGS_TAB_MUTE, _T("Mute"));
      InsertTabItem(dlgData->hTabCtrl, SETTINGS_TAB_QUIETHOURS, _T("Quiet Hours"));
      InsertTabItem(dlgData->hTabCtrl, SETTINGS_TAB_WIFI, _T("WLAN"));
      InsertTabItem(dlgData->hTabCtrl, SETTINGS_TAB_BLUETOOTH, _T("Bluetooth"));

      dlgData->hTabs[SETTINGS_TAB_GENERAL] = CreateDialogParam(
         hglobInstance,
         MAKEINTRESOURCE(IDD_SETTINGS_GENERAL),
         hDlg,
         Settings_GeneralDlgProc,
         reinterpret_cast<LPARAM>(settings));
      dlgData->hTabs[SETTINGS_TAB_MUTE] = CreateDialogParam(
         hglobInstance,
         MAKEINTRESOURCE(IDD_SETTINGS_MUTE),
         hDlg,
         Settings_MuteDlgProc,
         reinterpret_cast<LPARAM>(settings));
      dlgData->hTabs[SETTINGS_TAB_QUIETHOURS] = CreateDialogParam(
         hglobInstance,
         MAKEINTRESOURCE(IDD_SETTINGS_QUIETHOURS),
         hDlg,
         Settings_QuietHoursDlgProc,
         reinterpret_cast<LPARAM>(settings));
      dlgData->hTabs[SETTINGS_TAB_WIFI] = CreateDialogParam(
         hglobInstance,
         MAKEINTRESOURCE(IDD_SETTINGS_WIFI),
         hDlg,
         Settings_WifiDlgProc,
         reinterpret_cast<LPARAM>(settings));
      dlgData->hTabs[SETTINGS_TAB_BLUETOOTH] = CreateDialogParam(
         hglobInstance,
         MAKEINTRESOURCE(IDD_SETTINGS_BLUETOOTH),
         hDlg,
         Settings_BluetoothDlgProc,
         reinterpret_cast<LPARAM>(settings));

      // Init tab pages
      ResizeTabs(dlgData->hTabCtrl, dlgData->hTabs, SETTINGS_TAB_COUNT);
      for (int i = 0; i < SETTINGS_TAB_COUNT; ++i) {
         HWND hCurTab = dlgData->hTabs[i];
         ShowWindow(hCurTab, SW_HIDE);
      }

      HICON hIcon = LoadIcon(
         GetModuleHandle(NULL),
         MAKEINTRESOURCE(IDI_SETTINGS));
      SendMessageW(hDlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));

      SwitchTab(dlgData, dlgData->hTabs[SETTINGS_TAB_GENERAL]);
      return TRUE;
   }
   case WM_COMMAND:
      if (LOWORD(wParam) == IDOK) {
         for (int i = 0; i < SETTINGS_TAB_COUNT; ++i) {
            SendMessage(dlgData->hTabs[i], WM_SAVESETTINGS, 0, 0);
            EndDialog(dlgData->hTabs[i], 0);
         }
         EndDialog(hDlg, 0); 
      } else if (LOWORD(wParam) == IDCANCEL) {
         for (int i = 0; i < SETTINGS_TAB_COUNT; ++i) {
            EndDialog(dlgData->hTabs[i], 0);
         }
         EndDialog(hDlg, 1);
      }
      return 0;
   case WM_NOTIFY: {
      const LPNMHDR lpnmhdr = reinterpret_cast<LPNMHDR>(lParam);
      if (lpnmhdr->code == TCN_SELCHANGE
          && lpnmhdr->hwndFrom == dlgData->hTabCtrl) {
         const int curSel = TabCtrl_GetCurSel(dlgData->hTabCtrl);
         if (curSel >= 0 && curSel <= SETTINGS_TAB_COUNT) {
            SwitchTab(dlgData, dlgData->hTabs[curSel]);
         }
      }
      return 0;
   }
   case WM_CLOSE:
      EndDialog(hDlg, 1);
      return TRUE;
   case WM_DESTROY:
      delete dlgData;
      SetWindowLongPtr(hDlg, GWLP_USERDATA, NULL);
      return 0;
   default:
      break;
   }
   return FALSE;
}
