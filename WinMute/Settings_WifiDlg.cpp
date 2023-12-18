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

static constexpr int SSID_MAX_LEN = 32;

struct WiFiData {
   std::wstring ssidName;
};

static void LoadWifiAddDlgTranslation(HWND hDlg, bool isEdit)
{
   WMi18n &i18n = WMi18n::GetInstance();
   if (isEdit) {
      i18n.SetItemText(hDlg, "settings.wifi.add-edit.edit-title");
   } else {
      i18n.SetItemText(hDlg, "settings.wifi.add-edit.add-title");
   }
   i18n.SetItemText(hDlg, IDC_WIFI_NAME_LABEL, "settings.wifi.add-edit.ssid-name-label");
   i18n.SetItemText(hDlg, IDOK, "settings.btn-save");
   i18n.SetItemText(hDlg, IDCANCEL, "settings.btn-cancel");

   const auto placeholder = i18n.GetTranslationW("settings.wifi.add-edit.enter-device-name-placeholder");
   Edit_SetCueBannerText(
      GetDlgItem(hDlg, IDC_WIFI_NAME),
      placeholder.c_str());
}

INT_PTR CALLBACK Settings_WifiAddDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) {
   case WM_INITDIALOG: {
      HWND hSsid = GetDlgItem(hDlg, IDC_WIFI_NAME);
      WiFiData* wifiData = reinterpret_cast<WiFiData*>(lParam);
      if (wifiData == nullptr) {
         return FALSE;
      }
      SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(wifiData));

      LoadWifiAddDlgTranslation(hDlg, wifiData->ssidName.length() != 0);

      if (wifiData->ssidName.length() != 0) {
         SetWindowTextW(GetDlgItem(hDlg, IDC_WIFI_NAME), wifiData->ssidName.c_str());
      }
      // Disable save button until at least one string change is made
      EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
      Edit_LimitText(hSsid, SSID_MAX_LEN);
      if (GetDlgCtrlID(reinterpret_cast<HWND>(wParam)) != IDC_WIFI_NAME) {
         SetFocus(hSsid);
         return FALSE;
      }
      return TRUE;
   }
   case WM_COMMAND:
      if (LOWORD(wParam) == IDC_WIFI_NAME && HIWORD(wParam) == CBN_EDITUPDATE) {
         HWND hSsid = GetDlgItem(hDlg, IDC_WIFI_NAME);
         const int textLen = Edit_GetTextLength(hSsid);
         EnableWindow(GetDlgItem(hDlg, IDOK), textLen > 0);
      } else if (LOWORD(wParam) == IDOK) {
         HWND hSsid = GetDlgItem(hDlg, IDC_WIFI_NAME);
         const int textLen = Edit_GetTextLength(hSsid);
         if (textLen != 0) {
            wchar_t ssidBuf[SSID_MAX_LEN + 1];
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

static std::vector<std::wstring> ExportSsidListItems(HWND hList)
{
   std::vector<std::wstring> items;
   DWORD itemCount = ListBox_GetCount(hList);
   for (DWORD i = 0; i < itemCount; ++i) {
      wchar_t textBuf[SSID_MAX_LEN + 1] = { 0 };
      DWORD textLen = ListBox_GetTextLen(hList, i);
      if (textLen < ARRAY_SIZE(textBuf)) {
         ListBox_GetText(hList, i, textBuf);
         items.push_back(textBuf);
      }
   }
   return items;
}

static bool IsWlanAvailable()
{
   HANDLE wlanHandle;
   DWORD vers = 2;
   if (WlanOpenHandle(vers, nullptr, &vers, &wlanHandle) != ERROR_SUCCESS) {
      return false;
   }
   WlanCloseHandle(wlanHandle, nullptr);
   return true;
}

static BOOL CALLBACK ShowChildWindow(HWND hWnd, LPARAM lParam)
{
   ShowWindow(hWnd, static_cast<int>(lParam));
   return TRUE;
}


static void LoadWifiDlgTranslation(HWND hDlg)
{
   WMi18n &i18n = WMi18n::GetInstance();

   i18n.SetItemText(hDlg, IDC_WIFI_INTRO, "settings.wifi.intro");
   i18n.SetItemText(hDlg, IDC_ENABLE_WIFI_MUTE, "settings.wifi.enable");
   i18n.SetItemText(hDlg, IDC_IS_PERMITLIST, "settings.wifi.mute-when-not-in-list");
   i18n.SetItemText(hDlg, IDC_WIFI_ADD, "settings.btn-add");
   i18n.SetItemText(hDlg, IDC_WIFI_EDIT, "settings.btn-edit");
   i18n.SetItemText(hDlg, IDC_WIFI_REMOVE, "settings.btn-remove");
   i18n.SetItemText(hDlg, IDC_WIFI_REMOVEALL, "settings.btn-remove-all");
   i18n.SetItemText(hDlg, IDC_WLAN_LIST_IS_ALLOWLIST, "settings.wifi.mute-when-in-list");
   i18n.SetItemText(hDlg, IDC_WLAN_LIST_IS_BLOCKLIST, "settings.wifi.mute-when-not-in-list");
}

INT_PTR CALLBACK Settings_WifiDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) {
   case WM_INITDIALOG:
   {
      if (IsAppThemed()) {
         EnableThemeDialogTexture(hDlg, ETDT_ENABLETAB);
      }
      LoadWifiDlgTranslation(hDlg);
      WMSettings* settings = reinterpret_cast<WMSettings*>(lParam);
      assert(settings != nullptr);
      SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(settings));

      DWORD enabled = settings->QueryValue(SettingsKey::MUTE_ON_WLAN);
      Button_SetCheck(GetDlgItem(hDlg, IDC_ENABLE_WIFI_MUTE),
                      enabled ? BST_CHECKED : BST_UNCHECKED);
      Button_Enable(GetDlgItem(hDlg, IDC_WLAN_LIST_IS_ALLOWLIST), enabled == BST_CHECKED);
      Button_Enable(GetDlgItem(hDlg, IDC_WLAN_LIST_IS_BLOCKLIST), enabled == BST_CHECKED);
      enabled = settings->QueryValue(SettingsKey::MUTE_ON_WLAN_ALLOWLIST);
      Button_SetCheck(GetDlgItem(hDlg, IDC_WLAN_LIST_IS_ALLOWLIST),
                      enabled ? BST_CHECKED : BST_UNCHECKED);
      Button_SetCheck(GetDlgItem(hDlg, IDC_WLAN_LIST_IS_BLOCKLIST),
                      !enabled ? BST_CHECKED : BST_UNCHECKED);

      Button_Enable(GetDlgItem(hDlg, IDC_WIFI_EDIT), FALSE);
      Button_Enable(GetDlgItem(hDlg, IDC_WIFI_REMOVE), FALSE);

      HWND hList = GetDlgItem(hDlg, IDC_WIFI_LIST);
      const auto networks = settings->GetWifiNetworks();
      for (const auto& n : networks) {
         ListBox_AddString(hList, n.c_str());
      }
      Button_Enable(GetDlgItem(hDlg, IDC_WIFI_REMOVEALL),
                    ListBox_GetCount(hList) > 0);

      if (!IsWlanAvailable()) {
         // Set defaults
         Button_SetCheck(GetDlgItem(hDlg, IDC_ENABLE_WIFI_MUTE), BST_UNCHECKED);
         Button_Enable(GetDlgItem(hDlg, IDC_ENABLE_WIFI_MUTE), FALSE);
         Button_SetCheck(GetDlgItem(hDlg, IDC_WLAN_LIST_IS_ALLOWLIST), BST_CHECKED);
         Button_Enable(GetDlgItem(hDlg, IDC_WLAN_LIST_IS_ALLOWLIST), FALSE);
         Button_SetCheck(GetDlgItem(hDlg, IDC_WLAN_LIST_IS_BLOCKLIST), BST_UNCHECKED);
         Button_Enable(GetDlgItem(hDlg, IDC_WLAN_LIST_IS_BLOCKLIST), FALSE);

         // Hide all child windows, except the notice
         EnumChildWindows(hDlg, ShowChildWindow, SW_HIDE);

         HWND hDescription = GetDlgItem(hDlg, IDC_WIFI_INTRO);
         WMi18n::GetInstance().SetItemText(hDescription, "settings.wifi.wifi-disabled-info");
         RECT r;
         GetClientRect(hDescription, &r);
         const int margin = 20;
         SetWindowPos(
            hDescription, HWND_TOP,
            margin, margin, r.right, r.bottom,
            SWP_SHOWWINDOW);
      }
      return TRUE;
   }
   case WM_COMMAND:
   {
      if (LOWORD(wParam) == IDC_ENABLE_WIFI_MUTE) {
         const DWORD wlanEnabled = Button_GetCheck(GetDlgItem(hDlg, IDC_ENABLE_WIFI_MUTE));
         Button_Enable(GetDlgItem(hDlg, IDC_WLAN_LIST_IS_ALLOWLIST), wlanEnabled == BST_CHECKED);
         Button_Enable(GetDlgItem(hDlg, IDC_WLAN_LIST_IS_BLOCKLIST), wlanEnabled == BST_CHECKED);
      } else if (LOWORD(wParam) == IDC_WIFI_LIST) {
         HWND hList = GetDlgItem(hDlg, IDC_WIFI_LIST);
         if (HIWORD(wParam) == LBN_SELCHANGE || HIWORD(wParam) == LBN_SELCANCEL) {
            const bool entrySelected = (ListBox_GetCurSel(hList) != LB_ERR);
            Button_Enable(GetDlgItem(hDlg, IDC_WIFI_EDIT), entrySelected);
            Button_Enable(GetDlgItem(hDlg, IDC_WIFI_REMOVE), entrySelected);
         } else if (HIWORD(wParam) == LBN_KILLFOCUS) {
            const bool entrySelected = (ListBox_GetCurSel(hList) != LB_ERR);
            Button_Enable(GetDlgItem(hDlg, IDC_WIFI_EDIT), entrySelected);
            Button_Enable(GetDlgItem(hDlg, IDC_WIFI_REMOVE), entrySelected);
         }
      } else if (LOWORD(wParam) == IDC_WIFI_ADD) {
         WiFiData wifiData;
         if (DialogBoxParam(
               nullptr,
               MAKEINTRESOURCE(IDD_SETTINGS_WIFI_ADD),
               hDlg,
               Settings_WifiAddDlgProc,
               reinterpret_cast<LPARAM>(&wifiData)) == 0) {
            std::vector<std::wstring> networks = ExportSsidListItems(GetDlgItem(hDlg, IDC_WIFI_LIST));
            if (std::find(begin(networks), end(networks), wifiData.ssidName) == end(networks)) {
               ListBox_AddString(
                  GetDlgItem(hDlg, IDC_WIFI_LIST),
                  wifiData.ssidName.c_str());
               HWND hRemoveAll = GetDlgItem(hDlg, IDC_WIFI_REMOVEALL);
               if (!IsWindowEnabled(hRemoveAll)) {
                  Button_Enable(hRemoveAll, TRUE);
               }
            }
         }
      } else if (LOWORD(wParam) == IDC_WIFI_EDIT) {
         HWND hList = GetDlgItem(hDlg, IDC_WIFI_LIST);
         const WPARAM sel = ListBox_GetCurSel(hList);
         if (sel != LB_ERR) {
            const int len = ListBox_GetTextLen(hList, sel);
            wchar_t *textBuf = nullptr;
            if (len != LB_ERR) {
               if ((textBuf = new wchar_t[static_cast<size_t>(len) + 1]) != nullptr) {
                  ListBox_GetText(hList, sel, textBuf);

                  WiFiData wifiData;
                  wifiData.ssidName = textBuf;
                  delete[] textBuf;

                  if (DialogBoxParam(
                        nullptr,
                        MAKEINTRESOURCE(IDD_SETTINGS_WIFI_ADD),
                        hDlg,
                        Settings_WifiAddDlgProc,
                        reinterpret_cast<LPARAM>(&wifiData)) == 0) {
                     std::vector<std::wstring> networks = ExportSsidListItems(GetDlgItem(hDlg, IDC_WIFI_LIST));
                     if (std::find(begin(networks), end(networks), wifiData.ssidName) == end(networks)) {
                        ListBox_InsertString(hList, sel, wifiData.ssidName.c_str());
                        ListBox_DeleteString(hList, sel + 1);
                     } else {
                        ListBox_DeleteString(hList, sel);
                     }
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
      WMSettings* settings = reinterpret_cast<WMSettings*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
      std::vector<std::wstring> networks = ExportSsidListItems(GetDlgItem(hDlg, IDC_WIFI_LIST));
      settings->StoreWifiNetworks(networks);

      DWORD checked = Button_GetCheck(GetDlgItem(hDlg, IDC_ENABLE_WIFI_MUTE));
      settings->SetValue(SettingsKey::MUTE_ON_WLAN, checked == BST_CHECKED);

      checked = Button_GetCheck(GetDlgItem(hDlg, IDC_WLAN_LIST_IS_ALLOWLIST));
      settings->SetValue(SettingsKey::MUTE_ON_WLAN_ALLOWLIST, checked == BST_CHECKED);
      
      return 0;
   }
   default:
      break;
   }
   return FALSE;
}
