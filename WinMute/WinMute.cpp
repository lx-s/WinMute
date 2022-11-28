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
#include "WinMute.h"
#include "WinAudio.h"

extern HINSTANCE hglobInstance;
extern INT_PTR CALLBACK AboutDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR CALLBACK SettingsDlgProc(HWND, UINT, WPARAM, LPARAM);

static const wchar_t *WINMUTE_CLASS_NAME = L"WinMute";

static const wchar_t *TERMINAL_SERVER_KEY
   = L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\";
static const wchar_t* GLASS_SESSION_ID
   = L"GlassSessionId";

static LRESULT CALLBACK WinMuteWndProc(
   HWND hWnd,
   UINT msg,
   WPARAM wParam,
   LPARAM lParam)
{
   auto wm = reinterpret_cast<WinMute*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
   switch (msg) {
   case WM_NCCREATE: {
      LPCREATESTRUCTW cs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
      SetWindowLongPtrW(hWnd, GWLP_USERDATA,
         reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
      return TRUE;
   }
   default:
      break;
   }
   return (wm) ? wm->WindowProc(hWnd, msg, wParam, lParam)
      : DefWindowProcW(hWnd, msg, wParam, lParam);
}

static int IsDarkMode(bool& isDarkMode)
{
   HKEY hKey;
   int rc = 1;
   //RegOpenCurrentUser
   LSTATUS error = RegOpenKeyW(
      HKEY_CURRENT_USER,
      L"Software\\Microsoft\\Windows\\CurrentVersion\\"
      L"Themes\\Personalize",
      &hKey);
   if (error != ERROR_SUCCESS) {
      PrintWindowsError(L"RegOpenKey", error);
   } else {
      DWORD isLightTheme = 0;
      DWORD bufSize = sizeof(isLightTheme);
      DWORD valType = 0;
      error = RegQueryValueExW(
         hKey,
         L"SystemUsesLightTheme",
         NULL,
         &valType,
         reinterpret_cast<LPBYTE>(&isLightTheme),
         &bufSize);
      if (error != ERROR_SUCCESS) {
         PrintWindowsError(L"RegQueryValueEx", error);
      } else {
         isDarkMode = !isLightTheme;
         rc = 0;
      }
      RegCloseKey(hKey);
   }
   return rc;
}

static bool IsCurrentSessionRemoteable()
{
   bool isRemoteable = false;

   if (GetSystemMetrics(SM_REMOTESESSION)) {
      isRemoteable = true;
   } else {
      HKEY hRegKey = NULL;
      LONG lResult;

      lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, TERMINAL_SERVER_KEY, 0, KEY_READ, &hRegKey);
      if (lResult == ERROR_SUCCESS) {
         DWORD dwGlassSessionId;
         DWORD cbGlassSessionId = sizeof(dwGlassSessionId);
         DWORD dwType;

         lResult = RegQueryValueExW(hRegKey, GLASS_SESSION_ID, NULL, &dwType,
            reinterpret_cast<BYTE*>(&dwGlassSessionId),
            &cbGlassSessionId);

         if (lResult == ERROR_SUCCESS) {
            DWORD dwCurrentSessionId;
            if (ProcessIdToSessionId(GetCurrentProcessId(), &dwCurrentSessionId)) {
               isRemoteable = (dwCurrentSessionId != dwGlassSessionId);
            }
         }
         RegCloseKey(hRegKey);
      }
   }

   return isRemoteable;
}

WinMute::MuteConfig::MuteConfig()
   : muteOnWlan(false), muteOnBluetooth(false), showNotifications(false)
{
}

WinMute::WinMute() :
   hWnd_(nullptr),
   hTrayMenu_(nullptr),
   hAppIcon_(nullptr),
   hTrayIcon_(nullptr)
{
}

WinMute::~WinMute()
{
   Unload();
}

bool WinMute::RegisterWindowClass()
{
   WNDCLASS wc = { 0 };
   wc.hIcon = LoadIcon(hglobInstance, MAKEINTRESOURCE(IDI_APP));
   wc.hbrBackground = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
   wc.hInstance = hglobInstance;
   wc.lpfnWndProc = WinMuteWndProc;
   wc.lpszClassName = WINMUTE_CLASS_NAME;

   if (!RegisterClass(&wc)) {
      PrintWindowsError(L"RegisterClass");
      return false;
   }
   return true;
}

bool WinMute::InitWindow()
{
   hWnd_ = CreateWindowExW(WS_EX_TOOLWINDOW, WINMUTE_CLASS_NAME, PROGRAM_NAME,
      WS_POPUP, 0, 0, 0, 0, NULL, 0, hglobInstance, this);
   if (hWnd_ == nullptr) {
      PrintWindowsError(L"CreateWindowEx");
      return false;
   }
   return true;
}

bool WinMute::InitAudio()
{
   if (IsWindowsVistaOrGreater()) {
      // Nothing to do
   } else if (IsWindowsXPOrGreater()) {
      TaskDialog(
         nullptr, nullptr, PROGRAM_NAME,
         L"Only Windows Vista and newer is supported",
         L"For Windows XP support, please download WinMute "
         L" version 1.4.2 or older",
         TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
      return false;
   }

   if (!muteCtrl_.Init(hWnd_, &trayIcon_)) {
      return false;
   }

   return true;
}

#define CHECK_MENU_ITEM(id, cond) \
   (CheckMenuItem(hTrayMenu_, ID_TRAYMENU_ ## id, \
                  (cond) ? MF_CHECKED : MF_UNCHECKED) != -1)
bool WinMute::InitTrayMenu()
{
   if (hTrayMenu_ == NULL) {
      hTrayMenu_ = LoadMenuW(hglobInstance, MAKEINTRESOURCE(IDR_TRAYMENU));
      if (hTrayMenu_ == NULL) {
         PrintWindowsError(L"LoadMenu");
         return false;
      }
   }

   if (!CHECK_MENU_ITEM(MUTEONLOCK, muteCtrl_.GetMuteOnWorkstationLock()) ||
       !CHECK_MENU_ITEM(RESTOREAUDIO, muteCtrl_.GetRestoreVolume()) ||
       !CHECK_MENU_ITEM(MUTEONSUSPEND, muteCtrl_.GetMuteOnSuspend()) ||
       !CHECK_MENU_ITEM(MUTEONSHUTDOWN, muteCtrl_.GetMuteOnShutdown()) ||
       !CHECK_MENU_ITEM(MUTEONLOGOUT, muteCtrl_.GetMuteOnLogout())) {
      return false;
   }
   return true;
}
#undef CHECK_MENU_ITEM

bool WinMute::Init()
{
   WMLog& log = WMLog::GetInstance();

   hAppIcon_ = LoadIconW(hglobInstance, MAKEINTRESOURCE(IDI_APP));

   if (!settings_.Init()) {
      return false;
   }

#ifdef _DEBUG
   WMLog::GetInstance().SetEnabled(true);
#else
   WMLog::GetInstance().SetEnabled(settings_.QueryValue(SettingsKey::LOGGING_ENABLED));
#endif
   log.Write(L"Starting new session...");

   if (!RegisterWindowClass() || !InitWindow()) {
      return false;
   }

   if (!InitAudio()) {
      return false;
   }

   if (!LoadSettings()) {
      return false;
   }

   if (!InitTrayMenu()) {
      return false;
   }

   if (!WTSRegisterSessionNotification(hWnd_, NOTIFY_FOR_THIS_SESSION)) {
      PrintWindowsError(L"WTSRegisterSessionNotification");
      return false;
   }

   if (!RegisterPowerSettingNotification(hWnd_, &GUID_CONSOLE_DISPLAY_STATE, 0)) {
      PrintWindowsError(L"RegisterPowerSettingNotification");
      return false;
   }

   bool isDarkMode = true;
   IsDarkMode(isDarkMode);
   hTrayIcon_ = LoadIconW(
      hglobInstance,
      isDarkMode ? MAKEINTRESOURCE(IDI_TRAY_DARK)
                 : MAKEINTRESOURCE(IDI_TRAY_BRIGHT));
   if (hTrayIcon_ == NULL) {
      PrintWindowsError(_T("LoadIcon"));
      return false;
   }
   trayIcon_.Init(hWnd_, 0, hTrayIcon_, L"WinMute", true);

   quietHours_.Init(hWnd_, settings_);

   log.Write(L"WinMute initialized");

   if (settings_.QueryValue(SettingsKey::MUTE_ON_RDP)
       && IsCurrentSessionRemoteable()) {
      trayIcon_.ShowPopup(
         L"Remote Session detected",
         L"All audio endpoints have been muted");
      muteCtrl_.SetMute(true);
   }

   return true;
}

bool WinMute::LoadSettings()
{
   WMLog& log = WMLog::GetInstance();

   if (log.IsEnabled()) {
      log.Write(L"Loading settings:");
      log.Write(L"\tRestore volume: %s", settings_.QueryValue(SettingsKey::RESTORE_AUDIO) ? L"Yes" : L"No");
      log.Write(L"\tMute on lock: %s", settings_.QueryValue(SettingsKey::MUTE_ON_LOCK) ? L"Yes" : L"No");
      log.Write(L"\tMute on display standby: %s", settings_.QueryValue(SettingsKey::MUTE_ON_DISPLAYSTANDBY) ? L"Yes" : L"No");
      log.Write(L"\tMute on logout: %s", settings_.QueryValue(SettingsKey::MUTE_ON_LOGOUT) ? L"Yes" : L"No");
      log.Write(L"\tMute on suspend: %s", settings_.QueryValue(SettingsKey::MUTE_ON_SUSPEND) ? L"Yes" : L"No");
      log.Write(L"\tMute on shutdown: %s", settings_.QueryValue(SettingsKey::MUTE_ON_SHUTDOWN) ? L"Yes" : L"No");
      log.Write(L"\tShow notifications: %s", settings_.QueryValue(SettingsKey::NOTIFICATIONS_ENABLED) ? L"Yes" : L"No");
      log.Write(L"\tMute on bluetooth: %s", settings_.QueryValue(SettingsKey::MUTE_ON_BLUETOOTH) ? L"Yes" : L"No");
      log.Write(L"\t\tUse devicelist: %s", settings_.QueryValue(SettingsKey::MUTE_ON_BLUETOOTH_DEVICELIST) ? L"Yes" : L"No");
      log.Write(L"\tMute on WLAN: %s", settings_.QueryValue(SettingsKey::MUTE_ON_WLAN) ? L"Yes" : L"No");
      log.Write(L"\t\tUse allowlist: %s", settings_.QueryValue(SettingsKey::MUTE_ON_WLAN_ALLOWLIST) ? L"Yes" : L"No");
   }

   muteCtrl_.SetRestoreVolume(settings_.QueryValue(SettingsKey::RESTORE_AUDIO));
   muteCtrl_.SetMuteOnWorkstationLock(settings_.QueryValue(SettingsKey::MUTE_ON_LOCK));
   muteCtrl_.SetMuteOnDisplayStandby(settings_.QueryValue(SettingsKey::MUTE_ON_DISPLAYSTANDBY));
   muteCtrl_.SetMuteOnLogout(settings_.QueryValue(SettingsKey::MUTE_ON_LOGOUT));
   muteCtrl_.SetMuteOnSuspend(settings_.QueryValue(SettingsKey::MUTE_ON_SUSPEND));
   muteCtrl_.SetMuteOnShutdown(settings_.QueryValue(SettingsKey::MUTE_ON_SHUTDOWN));

   muteCtrl_.SetNotifications(settings_.QueryValue(SettingsKey::NOTIFICATIONS_ENABLED));

   muteConfig_.showNotifications = settings_.QueryValue(SettingsKey::NOTIFICATIONS_ENABLED);

   muteConfig_.muteOnBluetooth = settings_.QueryValue(SettingsKey::MUTE_ON_BLUETOOTH);
   if (!muteConfig_.muteOnBluetooth) {
      muteCtrl_.SetMuteOnBluetoothDisconnect(false);
      btDetector_.Unload();
   } else {
      if (!btDetector_.Init(hWnd_)) {
         trayIcon_.ShowPopup(
            L"Bluetooth muting disabled",
            L"Bluetooth is not available or disabled. "
            L"Bluetooth muting will be disabled on this computer");
         settings_.SetValue(SettingsKey::MUTE_ON_BLUETOOTH, FALSE);
      } else {
         muteCtrl_.SetMuteOnBluetoothDisconnect(true);
         bool muteOnWithDeviceList = settings_.QueryValue(SettingsKey::MUTE_ON_BLUETOOTH_DEVICELIST);
         btDetector_.SetDeviceList(settings_.GetBluetoothDevicesA(), muteOnWithDeviceList);
      }
   }

   muteConfig_.muteOnWlan = settings_.QueryValue(SettingsKey::MUTE_ON_WLAN);
   if (!muteConfig_.muteOnWlan) {
      wifiDetector_.Unload();
   } else {
      if (!wifiDetector_.Init(hWnd_)) {
         trayIcon_.ShowPopup(
            L"WLAN muting disabled",
            L"WLAN is not available or disabled. "
            L"WLAN muting will be disabled on this computer");
         settings_.SetValue(SettingsKey::MUTE_ON_WLAN, FALSE);
      } else {
         bool isMuteList = !settings_.QueryValue(SettingsKey::MUTE_ON_WLAN_ALLOWLIST);
         wifiDetector_.SetNetworkList(settings_.GetWifiNetworks(), isMuteList);
         wifiDetector_.CheckNetwork();
      }
   }

   return true;
}

void WinMute::ToggleMenuCheck(UINT item, bool* setting)
{
   UINT state = GetMenuState(hTrayMenu_, item, MF_BYCOMMAND);
   if (state & MF_CHECKED) {
      *setting = false;
      CheckMenuItem(hTrayMenu_, item, MF_UNCHECKED);
   } else {
      *setting = true;
      CheckMenuItem(hTrayMenu_, item, MF_CHECKED);
   }
}

LRESULT CALLBACK WinMute::WindowProc(
   HWND hWnd,
   UINT msg,
   WPARAM wParam,
   LPARAM lParam)
{
   static UINT uTaskbarRestart = 0;
   switch (msg) {
   case WM_CREATE: {
      uTaskbarRestart = RegisterWindowMessageW(L"TaskbarCreated");
      return TRUE;
   }
   case WM_COMMAND: {
      switch (LOWORD(wParam)) {
      case ID_TRAYMENU_INFO: {
         static bool dialogOpen = false;
         if (!dialogOpen) {
            dialogOpen = true;
            DialogBox(
               hglobInstance,
               MAKEINTRESOURCE(IDD_ABOUT),
               hWnd_,
               AboutDlgProc);
            dialogOpen = false;
         }
         break;
      }
      case ID_TRAYMENU_EXIT:
         SendMessage(hWnd, WM_CLOSE, 0, 0);
         break;
      case ID_TRAYMENU_SETTINGS: {
         static bool dialogOpen = false;
         if (!dialogOpen) {
            dialogOpen = true;
            if (DialogBoxParamW(
                  hglobInstance,
                  MAKEINTRESOURCE(IDD_SETTINGS),
                  hWnd_,
                  SettingsDlgProc,
                  reinterpret_cast<LPARAM>(&settings_)) == 0) {
               LoadSettings();
               InitTrayMenu();
               quietHours_.Reset(settings_);
            }
            dialogOpen = false;
         }
         break;
      }
      case ID_TRAYMENU_MUTE: {
         bool state = false;
         ToggleMenuCheck(ID_TRAYMENU_MUTE, &state);
         muteCtrl_.SetMute(state);
         break;
      }
      case ID_TRAYMENU_MUTEONLOCK: {
         bool checked = false;
         ToggleMenuCheck(ID_TRAYMENU_MUTEONLOCK, &checked);
         muteCtrl_.SetMuteOnWorkstationLock(checked);
         settings_.SetValue(SettingsKey::MUTE_ON_LOCK, checked);
         break;
      }
      case ID_TRAYMENU_RESTOREAUDIO:{
         bool checked = false;
         ToggleMenuCheck(ID_TRAYMENU_RESTOREAUDIO, &checked);
         muteCtrl_.SetRestoreVolume(checked);
         settings_.SetValue(SettingsKey::RESTORE_AUDIO, checked);
         break;
      }
      case ID_TRAYMENU_MUTEONSCREENSUSPEND: {
         bool checked = false;
         ToggleMenuCheck(ID_TRAYMENU_SCREENSUSPEND, &checked);
         muteCtrl_.SetMuteOnDisplayStandby(checked);
         settings_.SetValue(SettingsKey::MUTE_ON_DISPLAYSTANDBY, checked);
         break;
      }
      case ID_TRAYMENU_MUTEONSHUTDOWN: {
         bool checked = false;
         ToggleMenuCheck(ID_TRAYMENU_MUTEONSHUTDOWN, &checked);
         muteCtrl_.SetMuteOnShutdown(checked);
         settings_.SetValue(SettingsKey::MUTE_ON_SHUTDOWN, checked);
         break;
      }
      case ID_TRAYMENU_MUTEONSUSPEND: {
         bool checked = false;
         ToggleMenuCheck(ID_TRAYMENU_MUTEONSUSPEND, &checked);
         muteCtrl_.SetMuteOnSuspend(checked);
         settings_.SetValue(SettingsKey::MUTE_ON_SUSPEND, checked);
         break;
      }
      case ID_TRAYMENU_MUTEONLOGOUT: {
         bool checked = false;
         ToggleMenuCheck(ID_TRAYMENU_MUTEONBLUETOOTH, &checked);
         muteCtrl_.SetMuteOnLogout(checked);
         settings_.SetValue(SettingsKey::MUTE_ON_LOGOUT, checked);
         break;
      }
      default:
         break;
      }
      return 0;
   }
   case WM_TRAYICON:
   {
      switch (lParam) {
      case WM_LBUTTONUP:
      case WM_RBUTTONUP: {
         POINT p = { 0 };
         GetCursorPos(&p);
         SetForegroundWindow(hWnd);
         TrackPopupMenuEx(
            GetSubMenu(hTrayMenu_, 0),
            TPM_NONOTIFY | TPM_TOPALIGN | TPM_LEFTALIGN,
            p.x, p.y, hWnd_, nullptr);
         break;
      }
      default:
         break;
      }
      return TRUE;
   }
   case WM_CLOSE: {
      Close();
      return 0;
   }
   case WM_WTSSESSION_CHANGE: {
      if (wParam == WTS_SESSION_LOCK) {
         muteCtrl_.NotifyWorkstationLock(true);
      } else if (wParam == WTS_SESSION_UNLOCK) {
         muteCtrl_.NotifyWorkstationLock(false);
      }
      return 0;
   }
   case WM_POWERBROADCAST:
      if (wParam == PBT_APMSUSPEND) {
         muteCtrl_.NotifySuspend(true);
      } else if (wParam == PBT_POWERSETTINGCHANGE) {
         const PPOWERBROADCAST_SETTING bs =
            reinterpret_cast<PPOWERBROADCAST_SETTING>(lParam);
         if (IsEqualGUID(bs->PowerSetting, GUID_CONSOLE_DISPLAY_STATE)) {
            const DWORD state = bs->Data[0];
            if (state == 0x0) { // Display off
               muteCtrl_.NotifyDisplayStandby(true);
            } else if (state == 0x1) { // Display on
               muteCtrl_.NotifyDisplayStandby(false);
            } else if (state == 0x2) { // Display dimmed
            }
         }
      }
      return TRUE;
   case WM_QUERYENDSESSION:
      if (lParam == 0) { // Shutdown
         muteCtrl_.NotifyShutdown();
      } else if ((lParam & ENDSESSION_LOGOFF)) {
         muteCtrl_.NotifyLogout();
      }
      break;
   case WM_WINMUTE_QUIETHOURS_START:
      muteCtrl_.NotifyQuietHours(true);
      if (settings_.QueryValue(SettingsKey::QUIETHOURS_NOTIFICATIONS)) {
         trayIcon_.ShowPopup(
            L"WinMute: Quiet hours started",
            L"Your workstation audio will now be muted.");
      }
      quietHours_.SetEnd();
      return 0;
   case WM_WINMUTE_QUIETHOURS_END:
      muteCtrl_.NotifyQuietHours(false);
      if (settings_.QueryValue(SettingsKey::QUIETHOURS_NOTIFICATIONS)) {
         trayIcon_.ShowPopup(
            L"WinMute: Quiet Hours ended",
            L"Your workstation audio has been restored.");
      }
      if (settings_.QueryValue(SettingsKey::QUIETHOURS_FORCEUNMUTE)) {
         muteCtrl_.SetMute(false);
      }
      quietHours_.SetStart();
      return 0;
   case WM_DEVICECHANGE: {
      if (muteConfig_.muteOnBluetooth) {
         const auto btStatus = btDetector_.GetBluetoothStatus(msg, wParam, lParam);
         if (btStatus == BluetoothDetector::BluetoothStatus::Connected) {
            muteCtrl_.NotifyBluetoothConnected(true);
         } else if (btStatus == BluetoothDetector::BluetoothStatus::Disconnected) {
            muteCtrl_.NotifyBluetoothConnected(false);
         }
      }
      return TRUE;
   }
   case WM_WIFISTATUSCHANGED:
      if (muteConfig_.muteOnWlan) {
         if (wParam == 1) { // Connected
            if (settings_.QueryValue(SettingsKey::NOTIFICATIONS_ENABLED)) {
               wchar_t msgBuf[260] = { 0 };
               wchar_t *wifiName = reinterpret_cast<wchar_t*>(lParam);
               if (settings_.QueryValue(SettingsKey::MUTE_ON_WLAN_ALLOWLIST)) {
                  StringCchPrintfW(
                     msgBuf, ARRAY_SIZE(msgBuf),
                     L"WLAN \"%s\"network is not on allowed list.\n",
                     wifiName);
               }  else {
                  StringCchPrintfW(
                     msgBuf, ARRAY_SIZE(msgBuf),
                     L"WLAN network \"%s\" is configured for AutoMute.",
                     wifiName);
               }
               trayIcon_.ShowPopup(L"Workstation muted", msgBuf);
               delete [] wifiName;
            }
            muteCtrl_.SetMute(true);
         }
      }
      return 0;
   case WM_SETTINGCHANGE: {
      const wchar_t *changeParam = reinterpret_cast<const wchar_t *>(lParam);
      if (changeParam != nullptr && wcscmp(changeParam, L"ImmersiveColorSet") == 0) {
         bool isDarkMode = true;
         IsDarkMode(isDarkMode);
         hTrayIcon_ = LoadIconW(
            hglobInstance,
            isDarkMode ? MAKEINTRESOURCE(IDI_TRAY_DARK)
            : MAKEINTRESOURCE(IDI_TRAY_BRIGHT));
         if (hTrayIcon_ == NULL) {
            PrintWindowsError(L"LoadIcon");
         } else {
            trayIcon_.ChangeIcon(hTrayIcon_);
         }
      }
      return 0;
   }
   default:
      if (msg == uTaskbarRestart) { // Restore trayicon if explorer.exe crashes
         trayIcon_.Hide();
         trayIcon_.Show();
      }
      break;
   }

   return DefWindowProc(hWnd, msg, wParam, lParam);
}

void WinMute::Unload()
{
   settings_.Unload();
}

void WinMute::Close()
{
   Unload();
   PostQuitMessage(0);
}
