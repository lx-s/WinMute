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

static constexpr int SSID_MAX_LEN = 32;

struct WiFiData {
   tstring ssidName;
};

INT_PTR CALLBACK Settings_GeneralWifiAddDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) {
   case WM_INITDIALOG: {
      HWND hSsid = GetDlgItem(hDlg, IDC_WIFI_NAME);
      WiFiData* wifiData = reinterpret_cast<WiFiData*>(lParam);
      if (wifiData == nullptr) {
         return FALSE;
      }
      SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(wifiData));

      if (wifiData->ssidName.length() == 0) {
         if (!SetWindowText(hDlg, _T("Add WiFi network"))) {
            PrintWindowsError(_T("SetWindowText"), GetLastError());
            return FALSE;
         }
      } else {
         if (!SetWindowText(hDlg, _T("Edit WiFi network"))
            || !SetWindowText(
               GetDlgItem(hDlg, IDC_WIFI_NAME),
               wifiData->ssidName.c_str())) {
            PrintWindowsError(_T("SetWindowText"), GetLastError());
            return FALSE;
         }
      }
      Edit_LimitText(hSsid, SSID_MAX_LEN);
      if (GetDlgCtrlID(reinterpret_cast<HWND>(wParam)) != IDC_WIFI_NAME) {
         SetFocus(hSsid);
         return FALSE;
      }
      return TRUE;
   }
   case WM_COMMAND:
      if (LOWORD(wParam) == IDOK) {
         HWND hSsid = GetDlgItem(hDlg, IDC_WIFI_NAME);
         int textLen = Edit_GetTextLength(hSsid);
         if (textLen == 0) {
            EDITBALLOONTIP ebt;
            ZeroMemory(&ebt, sizeof(ebt));
            ebt.cbStruct = sizeof(ebt);
            ebt.pszText = _T("Please enter a SSID/WIFI name");
            ebt.pszTitle = _T("SSID Name");
            ebt.ttiIcon = TTI_WARNING_LARGE;
            Edit_ShowBalloonTip(hSsid, &ebt);
         } else {
            TCHAR ssidBuf[SSID_MAX_LEN + 1];
            WiFiData* wifiData = reinterpret_cast<WiFiData*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
            if (wifiData != nullptr) {
               Edit_GetText(hSsid, ssidBuf, ARRAY_SIZE(ssidBuf));
               wifiData->ssidName = ssidBuf;
               EndDialog(hDlg, 0);
            } else {
               EndDialog(hDlg, 1);
            }  
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

INT_PTR CALLBACK Settings_GeneralWifiDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) {
   case WM_INITDIALOG:
   {
      if (IsAppThemed()) {
         EnableThemeDialogTexture(hDlg, ETDT_ENABLETAB);
      }
      WMSettings* settings = reinterpret_cast<WMSettings*>(lParam);
      assert(settings != nullptr);
      SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(settings));

      Button_Enable(GetDlgItem(hDlg, IDC_WIFI_EDIT), FALSE);
      Button_Enable(GetDlgItem(hDlg, IDC_WIFI_REMOVE), FALSE);

      HWND hList = GetDlgItem(hDlg, IDC_WIFI_LIST);
      const auto networks = settings->GetWifiNetworks();
      for (const auto& n : networks) {
         ListBox_AddString(hList, n.c_str());
      }
      Button_Enable(GetDlgItem(hDlg, IDC_WIFI_REMOVEALL),
                    ListBox_GetCount(hList) > 0);
      return TRUE;
   }
   case WM_COMMAND:
   {
      if (LOWORD(wParam) == IDC_WIFI_LIST) {
         HWND hList = GetDlgItem(hDlg, IDC_WIFI_LIST);
         if (HIWORD(wParam) == LBN_SELCHANGE || HIWORD(wParam) == LBN_SELCANCEL) {
            bool entrySelected = (ListBox_GetCurSel(hList) != LB_ERR);
            Button_Enable(GetDlgItem(hDlg, IDC_WIFI_EDIT), entrySelected);
            Button_Enable(GetDlgItem(hDlg, IDC_WIFI_REMOVE), entrySelected);
         } else if (HIWORD(wParam) == LBN_KILLFOCUS) {
            bool entrySelected = (ListBox_GetCurSel(hList) != LB_ERR);
            Button_Enable(GetDlgItem(hDlg, IDC_WIFI_EDIT), entrySelected);
            Button_Enable(GetDlgItem(hDlg, IDC_WIFI_REMOVE), entrySelected);
         }
      } else if (LOWORD(wParam) == IDC_WIFI_ADD) {
         WiFiData wifiData;
         if (DialogBoxParam(
               NULL,
               MAKEINTRESOURCE(IDD_SETTINGS_WIFI_ADD),
               hDlg,
               Settings_GeneralWifiAddDlgProc,
               reinterpret_cast<LPARAM>(&wifiData)) == 0) {
            ListBox_AddString(
               GetDlgItem(hDlg, IDC_WIFI_LIST),
               wifiData.ssidName.c_str());
            HWND hRemoveAll = GetDlgItem(hDlg, IDC_WIFI_REMOVEALL);
            if (!IsWindowEnabled(hRemoveAll)) {
               Button_Enable(hRemoveAll, TRUE);
            }
         }
      } else if (LOWORD(wParam) == IDC_WIFI_EDIT) {
         HWND hList = GetDlgItem(hDlg, IDC_WIFI_LIST);
         WPARAM sel = ListBox_GetCurSel(hList);
         if (sel != LB_ERR) {
            int len = ListBox_GetTextLen(hList, sel);
            TCHAR * textBuf = NULL;
            if (len != LB_ERR) {
               if ((textBuf = new TCHAR[len + 1]) != NULL) {
                  ListBox_GetText(hList, sel, textBuf);

                  WiFiData wifiData;
                  wifiData.ssidName = textBuf;
                  delete[] textBuf;

                  if (DialogBoxParam(
                        NULL,
                        MAKEINTRESOURCE(IDD_SETTINGS_WIFI_ADD),
                        hDlg,
                        Settings_GeneralWifiAddDlgProc,
                        reinterpret_cast<LPARAM>(&wifiData)) == 0) {
                     ListBox_InsertString(hList, sel, wifiData.ssidName.c_str());
                     ListBox_DeleteString(hList, sel + 1);
                  }
               }
            }
         }
      } else if (LOWORD(wParam) == IDC_WIFI_REMOVE) {
         HWND hList = GetDlgItem(hDlg, IDC_WIFI_LIST);
         WPARAM sel = ListBox_GetCurSel(hList);
         if (sel != LB_ERR) {
            ListBox_DeleteString(hList, sel);
            if (ListBox_GetCount(hList) == 0) {
               Button_Enable(GetDlgItem(hDlg, IDC_WIFI_EDIT), FALSE);
               Button_Enable(GetDlgItem(hDlg, IDC_WIFI_REMOVE), FALSE);
               Button_Enable(GetDlgItem(hDlg, IDC_WIFI_REMOVEALL), FALSE);
            }
         }
      } else if (LOWORD(wParam) == IDC_WIFI_REMOVEALL) {
         ListBox_ResetContent(GetDlgItem(hDlg, IDC_WIFI_LIST));
         Button_Enable(GetDlgItem(hDlg, IDC_WIFI_EDIT), FALSE);
         Button_Enable(GetDlgItem(hDlg, IDC_WIFI_REMOVE), FALSE);
         Button_Enable(GetDlgItem(hDlg, IDC_WIFI_REMOVEALL), FALSE);
      }
      return 0;
   }
   case WM_SAVESETTINGS: {
      HWND hList = GetDlgItem(hDlg, IDC_WIFI_LIST);
      WMSettings* settings = reinterpret_cast<WMSettings*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
      DWORD items = ListBox_GetCount(hList);
      std::vector<tstring> networks;
      for (DWORD i = 0; i < items; ++i) {
         TCHAR textBuf[SSID_MAX_LEN + 1];
         DWORD textLen = ListBox_GetTextLen(hDlg, i);
         if (textLen < ARRAY_SIZE(textBuf)) {
            ListBox_GetText(hList, i, textBuf);
            networks.push_back(textBuf);
         }
      }
      settings->StoreWifiNetworks(networks);
      return 0;
   }
   default:
      break;
   }
   return FALSE;
}
