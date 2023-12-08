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

static constexpr int BT_DEV_NAME_MAX_LEN = 100;

struct BtDeviceData {
   std::wstring devName;
};

static bool GetPairedBtAudioDevices(std::vector<std::wstring>& devices)
{
   auto& log = WMLog::GetInstance();

   BLUETOOTH_DEVICE_SEARCH_PARAMS bfrp;
   ZeroMemory(&bfrp, sizeof(bfrp));
   bfrp.dwSize = sizeof(bfrp);
   bfrp.fReturnRemembered = 1;
   BLUETOOTH_DEVICE_INFO btdi;
   ZeroMemory(&btdi, sizeof(btdi));
   btdi.dwSize = sizeof(btdi);
   HBLUETOOTH_DEVICE_FIND hBtDevFind = BluetoothFindFirstDevice(&bfrp, &btdi);
   if (hBtDevFind == nullptr) {
      log.WriteWindowsError(L"BluetoothFindFirstDevice", GetLastError());
   } else {
      do {
         if (GET_COD_MAJOR(btdi.ulClassofDevice) == COD_MAJOR_AUDIO) {
            devices.push_back(btdi.szName);
         }
      } while (BluetoothFindNextDevice(hBtDevFind, &btdi));
      BluetoothFindDeviceClose(hBtDevFind);
      return true;
   }
   return false;
}

static void LoadBluetoothAddDlgTranslation(HWND hDlg, bool isEdit)
{
   WMi18n &i18n = WMi18n::GetInstance();
   if (isEdit) {
      i18n.SetItemText(hDlg, IDS_SETTINGS_BLUETOOTH_EDIT_DLG_TITLE);
   } else {
      i18n.SetItemText(hDlg, IDS_SETTINGS_BLUETOOTH_ADD_DLG_TITLE);
   }
   i18n.SetItemText(hDlg, IDC_LABEL_BT_DEVICE_NAME, IDS_SETTINGS_BLUETOOTH_ADD_DLG_DEVICE_NAME_LABEL);
   i18n.SetItemText(hDlg, IDOK, IDS_SETTINGS_BTN_SAVE);
   i18n.SetItemText(hDlg, IDCANCEL, IDS_SETTINGS_BTN_CANCEL);

   ComboBox_SetCueBannerText(
      GetDlgItem(hDlg, IDC_BT_DEVICE_NAME),
      i18n.GetTextW(IDS_SETTINGS_BLUETOOTH_ADD_DLG_ENTER_DEVICE_NAME_TEXT).c_str());
}

static INT_PTR CALLBACK Settings_BluetoothAddDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) {
   case WM_INITDIALOG: {
      HWND hBtDevName = GetDlgItem(hDlg, IDC_BT_DEVICE_NAME);
      BtDeviceData* btDeviceData = reinterpret_cast<BtDeviceData*>(lParam);
      if (btDeviceData == nullptr) {
         return FALSE;
      }
      SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(btDeviceData));
      LoadBluetoothAddDlgTranslation(hDlg, btDeviceData->devName.length() != 0);
      if (btDeviceData->devName.length() != 0) {
         SetWindowTextW(GetDlgItem(hDlg, IDC_BT_DEVICE_NAME),
                        btDeviceData->devName.c_str());
      }

      // Disable save button until at least one string change is made
      EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);

      // Fill Combobox
      std::vector<std::wstring> registeredBtDevices;
      if (GetPairedBtAudioDevices(registeredBtDevices)) {
         for (const auto& devName : registeredBtDevices) {
            ComboBox_AddString(hBtDevName, devName.c_str());
         }
      }
      ComboBox_SetExtendedUI(hBtDevName, TRUE);

      Edit_LimitText(hBtDevName, BT_DEV_NAME_MAX_LEN);
      if (GetDlgCtrlID(reinterpret_cast<HWND>(wParam)) != IDC_BT_DEVICE_NAME) {
         SetFocus(hBtDevName);
         return FALSE;
      }
      return TRUE;
   }
   case WM_COMMAND: {
      
      if (LOWORD(wParam) == IDC_BT_DEVICE_NAME && HIWORD(wParam) == CBN_EDITUPDATE) {
         HWND hDevName = GetDlgItem(hDlg, IDC_BT_DEVICE_NAME);
         const int textLen = Edit_GetTextLength(hDevName);
         EnableWindow(GetDlgItem(hDlg, IDOK), textLen > 0);
      } else if (LOWORD(wParam) == IDOK) {
         HWND hDevName = GetDlgItem(hDlg, IDC_BT_DEVICE_NAME);
         const int textLen = Edit_GetTextLength(hDevName);
         if (textLen != 0) {
            wchar_t devNameBuf[BT_DEV_NAME_MAX_LEN + 1] = { 'L\0' };
            BtDeviceData* btDevName = reinterpret_cast<BtDeviceData*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
            if (btDevName != nullptr) {
               Edit_GetText(hDevName, devNameBuf, ARRAY_SIZE(devNameBuf));
               btDevName->devName = devNameBuf;
               EndDialog(hDlg, 0);
            } else { 
               EndDialog(hDlg, 1);
            }
         }
      }
      else if (LOWORD(wParam) == IDCANCEL) {
         EndDialog(hDlg, 1);
      }
      return FALSE;
   }
   case WM_CLOSE:
      EndDialog(hDlg, 1);
      return TRUE;
   default:
      break;
   }
   return FALSE;
}

static std::vector<std::wstring> ExportBluetoothDeviceList(HWND hList)
{
   std::vector<std::wstring> items;
   DWORD itemCount = ListBox_GetCount(hList);
   for (DWORD i = 0; i < itemCount; ++i) {
      wchar_t textBuf[BT_DEV_NAME_MAX_LEN + 1] = { 0 };
      DWORD textLen = ListBox_GetTextLen(hList, i);
      if (textLen < ARRAY_SIZE(textBuf)) {
         ListBox_GetText(hList, i, textBuf);
         items.push_back(textBuf);
      }
   }
   return items;
}

static bool IsBluetoothAvailable() noexcept
{
   HANDLE btHandle;
   BLUETOOTH_FIND_RADIO_PARAMS bfrp = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };
   HBLUETOOTH_RADIO_FIND hRadiosFind = BluetoothFindFirstRadio(&bfrp, &btHandle);
   if (hRadiosFind == nullptr) {
      return false;
   }
   BluetoothFindRadioClose(hRadiosFind);
   return true;
}

static BOOL CALLBACK ShowChildWindow(HWND hWnd, LPARAM lParam) noexcept
{
   ShowWindow(hWnd, static_cast<int>(lParam));
   return TRUE;
}

static void LoadBluetoothDlgTranslation(HWND hDlg)
{
   WMi18n &i18n = WMi18n::GetInstance();

   i18n.SetItemText(hDlg, IDC_BLUETOOTH_DESCRIPTION_LABEL, IDS_SETTINGS_BLUETOOTH_DESCRIPTION);
   i18n.SetItemText(hDlg, IDC_ENABLE_BLUETOOTH_MUTE, IDS_SETTINGS_BLUETOOTH_ENABLE_MUTING);
   i18n.SetItemText(hDlg, IDC_ENABLE_BLUETOOTH_MUTE_DEVICE_LIST, IDS_SETTINGS_BLUETOOTH_ENABLE_MUTING_FILTER);
   i18n.SetItemText(hDlg, IDC_BLUETOOTH_ADD, IDS_SETTINGS_BTN_ADD);
   i18n.SetItemText(hDlg, IDC_BLUETOOTH_EDIT, IDS_SETTINGS_BTN_EDIT);
   i18n.SetItemText(hDlg, IDC_BLUETOOTH_REMOVE, IDS_SETTINGS_BTN_REMOVE);
   i18n.SetItemText(hDlg, IDC_BLUETOOTH_REMOVEALL, IDS_SETTINGS_BTN_REMOVE_ALL);
   i18n.SetItemText(hDlg, IDC_STATIC_BLUETOOTH_NOT_AVAILABLE, IDS_SETTINGS_BLUETOOTH_DISABLED_INFO);
}

INT_PTR CALLBACK Settings_BluetoothDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) {
   case WM_INITDIALOG:
   {
      if (IsAppThemed()) {
         EnableThemeDialogTexture(hDlg, ETDT_ENABLETAB);
      }
      LoadBluetoothDlgTranslation(hDlg);

      WMSettings* settings = reinterpret_cast<WMSettings*>(lParam);
      assert(settings != nullptr);
      SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(settings));

      DWORD enabled = settings->QueryValue(SettingsKey::MUTE_ON_BLUETOOTH);

      Button_SetCheck(GetDlgItem(hDlg, IDC_ENABLE_BLUETOOTH_MUTE),
         enabled ? BST_CHECKED : BST_UNCHECKED);
      Button_Enable(GetDlgItem(hDlg, IDC_ENABLE_BLUETOOTH_MUTE_DEVICE_LIST), enabled);

      enabled = settings->QueryValue(SettingsKey::MUTE_ON_BLUETOOTH_DEVICELIST);
      Button_SetCheck(GetDlgItem(hDlg, IDC_ENABLE_BLUETOOTH_MUTE_DEVICE_LIST),
         enabled ? BST_CHECKED : BST_UNCHECKED);


      Button_Enable(GetDlgItem(hDlg, IDC_BLUETOOTH_EDIT), FALSE);
      Button_Enable(GetDlgItem(hDlg, IDC_BLUETOOTH_REMOVE), FALSE);

      HWND hList = GetDlgItem(hDlg, IDC_BLUETOOTH_LIST);
      const auto devices = settings->GetBluetoothDevicesW();
      for (const auto& dev : devices) {
         ListBox_AddString(hList, dev.c_str());
      }
      Button_Enable(
         GetDlgItem(hDlg, IDC_BLUETOOTH_REMOVEALL),
         ListBox_GetCount(hList) > 0);

      if (!IsBluetoothAvailable()) {
         // Set defaults
         Button_SetCheck(GetDlgItem(hDlg, IDC_ENABLE_BLUETOOTH_MUTE), BST_UNCHECKED);
         Button_Enable(GetDlgItem(hDlg, IDC_ENABLE_BLUETOOTH_MUTE), FALSE);
         Button_SetCheck(GetDlgItem(hDlg, IDC_ENABLE_BLUETOOTH_MUTE_DEVICE_LIST), BST_UNCHECKED);
         Button_Enable(GetDlgItem(hDlg, IDC_ENABLE_BLUETOOTH_MUTE_DEVICE_LIST), FALSE);

         // Hide all child windows, except the notice
         EnumChildWindows(hDlg, ShowChildWindow, SW_HIDE);

         HWND hBtNotAvail = GetDlgItem(hDlg, IDC_STATIC_BLUETOOTH_NOT_AVAILABLE);
         RECT r;
         GetClientRect(hBtNotAvail, &r);
         constexpr int margin = 20;
         SetWindowPos(
            hBtNotAvail, HWND_TOP,
            margin, margin, r.right, r.bottom,
            SWP_SHOWWINDOW);
      } else {
         ShowWindow(GetDlgItem(hDlg, IDC_STATIC_BLUETOOTH_NOT_AVAILABLE), SW_HIDE);
      }
      return TRUE;
   }
   case WM_COMMAND:
   {
      if (LOWORD(wParam) == IDC_ENABLE_BLUETOOTH_MUTE) {
         const DWORD checked = Button_GetCheck(GetDlgItem(hDlg, IDC_ENABLE_BLUETOOTH_MUTE));
         Button_Enable(GetDlgItem(hDlg, IDC_ENABLE_BLUETOOTH_MUTE_DEVICE_LIST), checked == BST_CHECKED);
      } else if (LOWORD(wParam) == IDC_BLUETOOTH_LIST) {
         HWND hList = GetDlgItem(hDlg, IDC_BLUETOOTH_LIST);
         if (HIWORD(wParam) == LBN_SELCHANGE || HIWORD(wParam) == LBN_SELCANCEL) {
            const bool entrySelected = (ListBox_GetCurSel(hList) != LB_ERR);
            Button_Enable(GetDlgItem(hDlg, IDC_BLUETOOTH_EDIT), entrySelected);
            Button_Enable(GetDlgItem(hDlg, IDC_BLUETOOTH_REMOVE), entrySelected);
         }
         else if (HIWORD(wParam) == LBN_KILLFOCUS) {
            const bool entrySelected = (ListBox_GetCurSel(hList) != LB_ERR);
            Button_Enable(GetDlgItem(hDlg, IDC_BLUETOOTH_EDIT), entrySelected);
            Button_Enable(GetDlgItem(hDlg, IDC_BLUETOOTH_REMOVE), entrySelected);
         }
      } else if (LOWORD(wParam) == IDC_BLUETOOTH_ADD) {
         BtDeviceData btDeviceName;
         if (DialogBoxParam(
            nullptr,
               MAKEINTRESOURCE(IDD_SETTINGS_BLUETOOTH_ADD),
               hDlg,
               Settings_BluetoothAddDlgProc,
               reinterpret_cast<LPARAM>(&btDeviceName)) == 0) {
            std::vector<std::wstring> devices = ExportBluetoothDeviceList(GetDlgItem(hDlg, IDC_BLUETOOTH_LIST));
            if (std::find(begin(devices), end(devices), btDeviceName.devName) == end(devices)) {
               ListBox_AddString(
                  GetDlgItem(hDlg, IDC_BLUETOOTH_LIST),
                  btDeviceName.devName.c_str());
               HWND hRemoveAll = GetDlgItem(hDlg, IDC_BLUETOOTH_REMOVEALL);
               if (!IsWindowEnabled(hRemoveAll)) {
                  Button_Enable(hRemoveAll, TRUE);
               }
            }
         }
      } else if (LOWORD(wParam) == IDC_BLUETOOTH_EDIT) {
         HWND hList = GetDlgItem(hDlg, IDC_BLUETOOTH_LIST);
         const WPARAM sel = ListBox_GetCurSel(hList);
         if (sel != LB_ERR) {
            const int len = ListBox_GetTextLen(hList, sel);
            wchar_t* textBuf = nullptr;
            if (len != LB_ERR) {
               if ((textBuf = new wchar_t[static_cast<size_t>(len) + 1]) != nullptr) {
                  ListBox_GetText(hList, sel, textBuf);

                  BtDeviceData btDevName;
                  btDevName.devName = textBuf;
                  delete[] textBuf;

                  if (DialogBoxParam(
                        nullptr,
                        MAKEINTRESOURCE(IDD_SETTINGS_BLUETOOTH_ADD),
                        hDlg,
                        Settings_BluetoothAddDlgProc,
                        reinterpret_cast<LPARAM>(&btDevName)) == 0) {
                     std::vector<std::wstring> networks = ExportBluetoothDeviceList(GetDlgItem(hDlg, IDC_WIFI_LIST));
                     if (std::find(begin(networks), end(networks), btDevName.devName) == end(networks)) {
                        ListBox_InsertString(hList, sel, btDevName.devName.c_str());
                        ListBox_DeleteString(hList, sel + 1);
                     }
                     else {
                        ListBox_DeleteString(hList, sel);
                     }
                  }
               }
            }
         }
      } else if (LOWORD(wParam) == IDC_BLUETOOTH_REMOVE) {
         HWND hList = GetDlgItem(hDlg, IDC_BLUETOOTH_LIST);
         const WPARAM sel = ListBox_GetCurSel(hList);
         if (sel != LB_ERR) {
            ListBox_DeleteString(hList, sel);
            if (ListBox_GetCount(hList) == 0) {
               Button_Enable(GetDlgItem(hDlg, IDC_BLUETOOTH_EDIT), FALSE);
               Button_Enable(GetDlgItem(hDlg, IDC_BLUETOOTH_REMOVE), FALSE);
               Button_Enable(GetDlgItem(hDlg, IDC_BLUETOOTH_REMOVEALL), FALSE);
            }
         }
      } else if (LOWORD(wParam) == IDC_BLUETOOTH_REMOVEALL) {
         ListBox_ResetContent(GetDlgItem(hDlg, IDC_BLUETOOTH_LIST));
         Button_Enable(GetDlgItem(hDlg, IDC_BLUETOOTH_EDIT), FALSE);
         Button_Enable(GetDlgItem(hDlg, IDC_BLUETOOTH_REMOVE), FALSE);
         Button_Enable(GetDlgItem(hDlg, IDC_BLUETOOTH_REMOVEALL), FALSE);
      }
      return 0;
   }
   case WM_SAVESETTINGS: {
      WMSettings* settings = reinterpret_cast<WMSettings*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
      std::vector<std::wstring> devices = ExportBluetoothDeviceList(GetDlgItem(hDlg, IDC_BLUETOOTH_LIST));
      settings->StoreBluetoothDevices(devices);

      DWORD checked = Button_GetCheck(GetDlgItem(hDlg, IDC_ENABLE_BLUETOOTH_MUTE));
      settings->SetValue(SettingsKey::MUTE_ON_BLUETOOTH, checked == BST_CHECKED);

      checked = Button_GetCheck(GetDlgItem(hDlg, IDC_ENABLE_BLUETOOTH_MUTE_DEVICE_LIST));
      settings->SetValue(SettingsKey::MUTE_ON_BLUETOOTH_DEVICELIST, checked == BST_CHECKED);

      return 0;
   }
   default:
      break;
   }
   return FALSE;
}
