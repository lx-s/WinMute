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
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>

static constexpr int ENDPOINT_NAME_MAX_LEN = 200;

struct EndpointData {
   std::wstring name;
};

_COM_SMARTPTR_TYPEDEF(IPropertyStore, __uuidof(IPropertyStore));
_COM_SMARTPTR_TYPEDEF(IMMDevice, __uuidof(IMMDevice));
_COM_SMARTPTR_TYPEDEF(IMMDeviceEnumerator, __uuidof(IMMDeviceEnumerator));
_COM_SMARTPTR_TYPEDEF(IMMDeviceCollection, __uuidof(IMMDeviceCollection));

static bool GetAudioEndpoints(std::vector<std::wstring>& endpoints)
{
   WMLog &log = WMLog::GetInstance();

   IMMDeviceEnumeratorPtr deviceEnumerator;
   if (FAILED(deviceEnumerator.CreateInstance(
         __uuidof(MMDeviceEnumerator),
         nullptr,
         CLSCTX_INPROC_SERVER))) {
      return false;
   }
   IMMDeviceCollectionPtr audioEndpoints;
   if (FAILED(deviceEnumerator->EnumAudioEndpoints(
         eRender,
         DEVICE_STATE_ACTIVE,
         &audioEndpoints))) {
      return false;
   }

   UINT epCount = 0;
   if (FAILED(audioEndpoints->GetCount(&epCount))) {
      return false;
   }

   for (UINT i = 0; i < epCount; ++i) {
      IMMDevicePtr device = nullptr;
      if (FAILED(audioEndpoints->Item(i, &device))) {
         log.Write(L"Failed to get audio endpoint #%d", i);
         continue;
      }

      IPropertyStorePtr propStore = nullptr;
      if (FAILED(device->OpenPropertyStore(STGM_READ, &propStore))) {
         log.Write(L"Failed to open property store for audio endpoint #%d", i);
         continue;
      }

      PROPVARIANT value;
      PropVariantInit(&value);
      if (FAILED(propStore->GetValue(PKEY_Device_FriendlyName, &value))) {
         log.Write(L"Failed to get device name for audio endpoint #%d", i);
         continue;
      }

      wchar_t deviceName[100];
      StringCchCopy(deviceName,
                    sizeof(deviceName) / sizeof(deviceName[0]),
                    value.pwszVal);
      PropVariantClear(&value);
      endpoints.push_back(deviceName);
   }

   return true;
}

static INT_PTR CALLBACK Settings_EndpointAddDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) {
   case WM_INITDIALOG:
   {
      HWND hEndpointName = GetDlgItem(hDlg, IDC_ENDPOINT_NAME);
      EndpointData *endpointData = reinterpret_cast<EndpointData*>(lParam);
      if (endpointData == nullptr) {
         return FALSE;
      }
      SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(endpointData));

      if (endpointData->name.length() == 0) {
         if (!SetWindowTextW(hDlg, L"Add endpoint")) {
            PrintWindowsError(L"SetWindowText", GetLastError());
            return FALSE;
         }
      } else {
         if (!SetWindowTextW(hDlg, L"Edit endpoint") ||
             !SetWindowTextW(GetDlgItem(hDlg, IDC_BT_DEVICE_NAME),
                             endpointData->name.c_str())) {
            PrintWindowsError(L"SetWindowText", GetLastError());
            return FALSE;
         }
      }
      // Fill Combobox
      std::vector<std::wstring> audioEndpoints;
      if (GetAudioEndpoints(audioEndpoints)) {
         for (const auto &devName : audioEndpoints) {
            ComboBox_AddString(hEndpointName, devName.c_str());
         }
      }

      Edit_LimitText(hEndpointName, ENDPOINT_NAME_MAX_LEN);
      if (GetDlgCtrlID(reinterpret_cast<HWND>(wParam)) != IDC_BT_DEVICE_NAME) {
         SetFocus(hEndpointName);
         return FALSE;
      }
      return TRUE;
   }
   case WM_COMMAND:
      if (LOWORD(wParam) == IDOK) {
         HWND hEpName = GetDlgItem(hDlg, IDC_ENDPOINT_NAME);
         const int textLen = Edit_GetTextLength(hEpName);
         if (textLen == 0) {
            EDITBALLOONTIP ebt;
            ZeroMemory(&ebt, sizeof(ebt));
            ebt.cbStruct = sizeof(ebt);
            ebt.pszText = L"Please enter a endpoint name";
            ebt.pszTitle = L"Endpoint Name";
            ebt.ttiIcon = TTI_INFO;
            Edit_ShowBalloonTip(hEpName, &ebt);
         } else {
            wchar_t epNameBuf[ENDPOINT_NAME_MAX_LEN + 1] = { 'L\0' };
            EndpointData *epData = reinterpret_cast<EndpointData*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
            if (epData != nullptr) {
               Edit_GetText(hEpName, epNameBuf, ARRAY_SIZE(epNameBuf));
               epData->name = epNameBuf;
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

static std::vector<std::wstring> ExportEndpointNameList(HWND hList)
{
   std::vector<std::wstring> items;
   DWORD itemCount = ListBox_GetCount(hList);
   for (DWORD i = 0; i < itemCount; ++i) {
      wchar_t textBuf[ENDPOINT_NAME_MAX_LEN + 1] = { 0 };
      DWORD textLen = ListBox_GetTextLen(hList, i);
      if (textLen < ARRAY_SIZE(textBuf)) {
         ListBox_GetText(hList, i, textBuf);
         items.push_back(textBuf);
      }
   }
   return items;
}

INT_PTR CALLBACK Settings_ManageEndpointsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) {
   case WM_INITDIALOG:
   {
      if (IsAppThemed()) {
         EnableThemeDialogTexture(hDlg, ETDT_ENABLETAB);
      }
      WMSettings *settings = reinterpret_cast<WMSettings *>(lParam);
      assert(settings != nullptr);
      SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(settings));

      DWORD endpointMode = settings->QueryValue(SettingsKey::MUTE_INDIVIDUAL_ENDPOINTS_MODE);
      if (endpointMode == MUTE_ENDPOINT_MODE_INDIVIDUAL_ALLOW_LIST) {
         Button_SetCheck(GetDlgItem(hDlg, IDC_ENDPOINT_LIST_IS_ALLOWLIST), BST_CHECKED);
         Button_SetCheck(GetDlgItem(hDlg, IDC_ENDPOINT_LIST_IS_BLOCKLIST), BST_UNCHECKED);
      } else if (endpointMode == MUTE_ENDPOINT_MODE_INDIVIDUAL_BLOCK_LIST) {
         Button_SetCheck(GetDlgItem(hDlg, IDC_ENDPOINT_LIST_IS_ALLOWLIST), BST_UNCHECKED);
         Button_SetCheck(GetDlgItem(hDlg, IDC_ENDPOINT_LIST_IS_BLOCKLIST), BST_CHECKED);
      }

      HWND hList = GetDlgItem(hDlg, IDC_ENDPOINT_LIST);
      const auto devices = settings->GetManagedAudioEndpoints();
      for (const auto &dev : devices) {
         ListBox_AddString(hList, dev.c_str());
      }
      Button_Enable(
         GetDlgItem(hDlg, IDC_ENDPOINT_REMOVEALL),
         ListBox_GetCount(hList) > 0);

      return TRUE;
   }
   case WM_COMMAND:
   {
      if (LOWORD(wParam) == IDC_ENDPOINT_LIST) {
         HWND hList = GetDlgItem(hDlg, IDC_ENDPOINT_LIST);
         if (HIWORD(wParam) == LBN_SELCHANGE || HIWORD(wParam) == LBN_SELCANCEL) {
            const bool entrySelected = (ListBox_GetCurSel(hList) != LB_ERR);
            Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_EDIT), entrySelected);
            Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_REMOVE), entrySelected);
         } else if (HIWORD(wParam) == LBN_KILLFOCUS) {
            const bool entrySelected = (ListBox_GetCurSel(hList) != LB_ERR);
            Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_EDIT), entrySelected);
            Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_REMOVE), entrySelected);
         }
      } else if (LOWORD(wParam) == IDC_ENDPOINT_ADD) {
         EndpointData epData;
         if (DialogBoxParam(
               nullptr,
               MAKEINTRESOURCE(IDD_MANAGE_ENDPOINTS_ADD),
               hDlg,
               Settings_EndpointAddDlgProc,
               reinterpret_cast<LPARAM>(&epData)) == 0) {
            std::vector<std::wstring> devices = ExportEndpointNameList(GetDlgItem(hDlg, IDC_ENDPOINT_LIST));
            if (std::find(begin(devices), end(devices), epData.name) == end(devices)) {
               ListBox_AddString(
                  GetDlgItem(hDlg, IDC_ENDPOINT_LIST),
                  epData.name.c_str());
               HWND hRemoveAll = GetDlgItem(hDlg, IDC_ENDPOINT_REMOVEALL);
               if (!IsWindowEnabled(hRemoveAll)) {
                  Button_Enable(hRemoveAll, TRUE);
               }
            }
         }
      } else if (LOWORD(wParam) == IDC_ENDPOINT_EDIT) {
         HWND hList = GetDlgItem(hDlg, IDC_ENDPOINT_LIST);
         const WPARAM sel = ListBox_GetCurSel(hList);
         if (sel != LB_ERR) {
            const int len = ListBox_GetTextLen(hList, sel);
            wchar_t *textBuf = nullptr;
            if (len != LB_ERR) {
               if ((textBuf = new wchar_t[static_cast<size_t>(len) + 1]) != nullptr) {
                  ListBox_GetText(hList, sel, textBuf);

                  EndpointData epData;
                  epData.name = textBuf;
                  delete[] textBuf;

                  if (DialogBoxParam(
                        nullptr,
                        MAKEINTRESOURCE(IDD_MANAGE_ENDPOINTS_ADD),
                        hDlg,
                        Settings_EndpointAddDlgProc,
                        reinterpret_cast<LPARAM>(&epData)) == 0) {
                     std::vector<std::wstring> networks = ExportEndpointNameList(GetDlgItem(hDlg, IDC_WIFI_LIST));
                     if (std::find(begin(networks), end(networks), epData.name) == end(networks)) {
                        ListBox_InsertString(hList, sel, epData.name.c_str());
                        ListBox_DeleteString(hList, sel + 1);
                     } else {
                        ListBox_DeleteString(hList, sel);
                     }
                  }
               }
            }
         }
      } else if (LOWORD(wParam) == IDC_ENDPOINT_REMOVE) {
         HWND hList = GetDlgItem(hDlg, IDC_ENDPOINT_LIST);
         const WPARAM sel = ListBox_GetCurSel(hList);
         if (sel != LB_ERR) {
            ListBox_DeleteString(hList, sel);
            if (ListBox_GetCount(hList) == 0) {
               Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_EDIT), FALSE);
               Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_REMOVE), FALSE);
               Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_REMOVEALL), FALSE);
            }
         }
      } else if (LOWORD(wParam) == IDC_ENDPOINT_REMOVEALL) {
         ListBox_ResetContent(GetDlgItem(hDlg, IDC_ENDPOINT_LIST));
         Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_EDIT), FALSE);
         Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_REMOVE), FALSE);
         Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_REMOVEALL), FALSE);
      } else if (LOWORD(wParam) == IDOK) {
         WMSettings *settings = reinterpret_cast<WMSettings *>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
         std::vector<std::wstring> devices = ExportEndpointNameList(GetDlgItem(hDlg, IDC_ENDPOINT_LIST));
         settings->StoreManagedAudioEndpoints(devices);
         if (Button_GetCheck(GetDlgItem(hDlg, IDC_ENDPOINT_LIST_IS_ALLOWLIST))) {
            settings->SetValue(SettingsKey::MUTE_INDIVIDUAL_ENDPOINTS_MODE, MUTE_ENDPOINT_MODE_INDIVIDUAL_ALLOW_LIST);
         } else if (Button_GetCheck(GetDlgItem(hDlg, IDC_ENDPOINT_LIST_IS_BLOCKLIST))) {
            settings->SetValue(SettingsKey::MUTE_INDIVIDUAL_ENDPOINTS_MODE, MUTE_ENDPOINT_MODE_INDIVIDUAL_BLOCK_LIST);
         }
         EndDialog(hDlg, 0);
      } else if (LOWORD(wParam) == IDCANCEL) {
         EndDialog(hDlg, 0);
      }
      return 0;
   }
   default:
      break;
   }
   return FALSE;
}
