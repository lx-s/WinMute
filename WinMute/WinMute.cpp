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
   return (wm)
      ? wm->WindowProc(hWnd, msg, wParam, lParam)
      : DefWindowProcW(hWnd, msg, wParam, lParam);
}

static bool IsCurrentSessionRemoteable() noexcept
{
   bool isRemoteable = false;

   if (GetSystemMetrics(SM_REMOTESESSION)) {
      isRemoteable = true;
   } else {
      HKEY hRegKey = nullptr;
      LONG lResult;

      lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, TERMINAL_SERVER_KEY, 0, KEY_READ, &hRegKey);
      if (lResult == ERROR_SUCCESS) {
         DWORD dwGlassSessionId = 0;
         DWORD cbGlassSessionId = sizeof(dwGlassSessionId);
         DWORD dwType;

         lResult = RegQueryValueExW(hRegKey, GLASS_SESSION_ID, nullptr, &dwType,
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

WinMute::WinMute(WMSettings& settings) :
   hWnd_(nullptr),
   hTrayMenu_(nullptr),
   hAppIcon_(nullptr),
   hTrayIcon_(nullptr),
   hUpdateIcon_(nullptr),
   settings_(settings),
   i18n_(WMi18n::GetInstance())
{
}

WinMute::~WinMute() noexcept
{
   Unload();
}

bool WinMute::RegisterWindowClass()
{
   WNDCLASSEX wc = { 0 };
   wc.cbSize = sizeof(wc);
   wc.hIcon = LoadIcon(hglobInstance, MAKEINTRESOURCE(IDI_APP));
   wc.hbrBackground = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
   wc.hInstance = hglobInstance;
   wc.lpfnWndProc = WinMuteWndProc;
   wc.lpszClassName = WINMUTE_CLASS_NAME;

   if (!RegisterClassExW(&wc)) {
      ShowWindowsError(L"RegisterClass");
      return false;
   }
   return true;
}

bool WinMute::InitWindow()
{
   hWnd_ = CreateWindowExW(WS_EX_TOOLWINDOW, WINMUTE_CLASS_NAME, PROGRAM_NAME,
      WS_POPUP, 0, 0, 0, 0, nullptr, 0, hglobInstance, this);
   if (hWnd_ == nullptr) {
      ShowWindowsError(L"CreateWindowEx");
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
         nullptr, nullptr,
         PROGRAM_NAME,
         i18n_.GetTranslationW("init.error.winmute.platform-support.title").c_str(),
         i18n_.GetTranslationW("init.error.winmute.platform-support.text").c_str(),
         TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
      return false;
   }

   if (!muteCtrl_.Init(hWnd_, &wmTray_)) {
      return false;
   }

   return true;
}

#define CHECK_MENU_ITEM(id, cond) \
   (CheckMenuItem(hTrayMenu_, ID_TRAYMENU_ ## id, \
                  (cond) ? MF_CHECKED : MF_UNCHECKED) != -1)
bool WinMute::InitTrayMenu()
{
   if (hTrayMenu_ == nullptr) {
      hTrayMenu_ = LoadMenuW(hglobInstance, MAKEINTRESOURCE(IDR_TRAYMENU));
      if (hTrayMenu_ == nullptr) {
         ShowWindowsError(L"LoadMenu");
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

   LoadMainMenuText();
   return true;
}
#undef CHECK_MENU_ITEM

bool WinMute::Init()
{
   WMLog& log = WMLog::GetInstance();

   hAppIcon_ = LoadIconW(hglobInstance, MAKEINTRESOURCE(IDI_APP));

#ifdef _DEBUG
   WMLog::GetInstance().SetEnabled(true);
#else
   WMLog::GetInstance().SetEnabled(settings_.QueryValue(SettingsKey::LOGGING_ENABLED));
#endif
   log.LogInfo(L"Starting new session...");

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
      const DWORD lastError = GetLastError();
      ShowWindowsError(L"WTSRegisterSessionNotification");
      log.LogWinError(L"WTSRegisterSessionNotification", lastError);
      return false;
   }

   if (!RegisterPowerSettingNotification(hWnd_, &GUID_CONSOLE_DISPLAY_STATE, DEVICE_NOTIFY_WINDOW_HANDLE)) {
      const DWORD lastError = GetLastError();
      ShowWindowsError(L"RegisterPowerSettingNotification", lastError);
      log.LogWinError(L"RegisterPowerSettingNotification", lastError);
      return false;
   }

   hTrayIcon_ = LoadIconW(hglobInstance, MAKEINTRESOURCE(IDI_APP));
   if (hTrayIcon_ == nullptr) {
      ShowWindowsError(_T("LoadIcon"));
      return false;
   }
   hUpdateIcon_ = LoadIconW(hglobInstance, MAKEINTRESOURCE(IDI_APPUPDATE));
   if (hUpdateIcon_ == nullptr) {
      ShowWindowsError(_T("LoadIcon"));
      return false;
   }
   wmTray_.Init(hWnd_, 0, hTrayIcon_, L"WinMute", true);
   updateTray_.Init(hWnd_, 1, hUpdateIcon_, L"WinMute Update", false, WM_WINMUTE_UPDATE_POPUP);

   quietHours_.Init(hWnd_, settings_);

   log.LogInfo(L"WinMute initialized");

   if (settings_.QueryValue(SettingsKey::MUTE_ON_RDP)
       && IsCurrentSessionRemoteable()) {
      wmTray_.ShowPopup(
         i18n_.GetTranslationW("popup.remote-session-detected.title"),
         i18n_.GetTranslationW("popup.remote-session-detected.text"));
      muteCtrl_.SetMute(true);
   }

   CheckForUpdates();

   return true;
}

bool WinMute::LoadSettings()
{
   WMLog& log = WMLog::GetInstance();

   if (log.IsEnabled()) {
      std::wstring versionNumber;
      GetWinMuteVersion(versionNumber);
      log.LogInfo(L"Starting WinMute %s", versionNumber.c_str());
      log.LogInfo(L"Loading settings:");
      log.LogInfo(L"Check for updates: %s", settings_.QueryValue(SettingsKey::CHECK_FOR_UPDATE) ? L"Yes" : L"No");
      log.LogInfo(L"\tCheck for beta updates: %s", settings_.QueryValue(SettingsKey::CHECK_FOR_BETA_UPDATE) ? L"Yes" : L"No");
      log.LogInfo(L"\tRestore volume: %s", settings_.QueryValue(SettingsKey::RESTORE_AUDIO) ? L"Yes" : L"No");
      log.LogInfo(L"\tMute delay: %d", settings_.QueryValue(SettingsKey::MUTE_DELAY));
      log.LogInfo(L"\tMute on lock: %s", settings_.QueryValue(SettingsKey::MUTE_ON_LOCK) ? L"Yes" : L"No");
      log.LogInfo(L"\tMute on display standby: %s", settings_.QueryValue(SettingsKey::MUTE_ON_DISPLAYSTANDBY) ? L"Yes" : L"No");
      log.LogInfo(L"\tMute on logout: %s", settings_.QueryValue(SettingsKey::MUTE_ON_LOGOUT) ? L"Yes" : L"No");
      log.LogInfo(L"\tMute on suspend: %s", settings_.QueryValue(SettingsKey::MUTE_ON_SUSPEND) ? L"Yes" : L"No");
      log.LogInfo(L"\tMute on shutdown: %s", settings_.QueryValue(SettingsKey::MUTE_ON_SHUTDOWN) ? L"Yes" : L"No");
      log.LogInfo(L"\tShow notifications: %s", settings_.QueryValue(SettingsKey::NOTIFICATIONS_ENABLED) ? L"Yes" : L"No");
      log.LogInfo(L"\tMute on bluetooth: %s", settings_.QueryValue(SettingsKey::MUTE_ON_BLUETOOTH) ? L"Yes" : L"No");
      log.LogInfo(L"\t\tUse devicelist: %s", settings_.QueryValue(SettingsKey::MUTE_ON_BLUETOOTH_DEVICELIST) ? L"Yes" : L"No");
      log.LogInfo(L"\tMute on WLAN: %s", settings_.QueryValue(SettingsKey::MUTE_ON_WLAN) ? L"Yes" : L"No");
      log.LogInfo(L"\t\tUse allowlist: %s", settings_.QueryValue(SettingsKey::MUTE_ON_WLAN_ALLOWLIST) ? L"Yes" : L"No");
      log.LogInfo(L"\tMute specific endpoints only: %s", settings_.QueryValue(SettingsKey::MUTE_INDIVIDUAL_ENDPOINTS) ? L"Yes" : L"No");
   }

   if (!settings_.QueryValue(SettingsKey::MUTE_INDIVIDUAL_ENDPOINTS)) {
      muteCtrl_.ClearManagedEndpoints();
   } else {
      const auto endpoints = settings_.GetManagedAudioEndpoints();
      const bool isAllowList = settings_.QueryValue(SettingsKey::MUTE_INDIVIDUAL_ENDPOINTS_MODE) == MUTE_ENDPOINT_MODE_INDIVIDUAL_ALLOW_LIST;
      muteCtrl_.SetManagedEndpoints(endpoints, isAllowList);
   }
   muteCtrl_.SetMuteDelay(settings_.QueryValue(SettingsKey::MUTE_DELAY));
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
         wmTray_.ShowPopup(
            i18n_.GetTranslationW("popup.bluetooth-muting-disabled.title"),
            i18n_.GetTranslationW("popup.bluetooth-muting-disabled.text"));
         settings_.SetValue(SettingsKey::MUTE_ON_BLUETOOTH, FALSE);
      } else {
         muteCtrl_.SetMuteOnBluetoothDisconnect(true);
         const bool muteOnWithDeviceList = settings_.QueryValue(SettingsKey::MUTE_ON_BLUETOOTH_DEVICELIST);
         btDetector_.SetDeviceList(settings_.GetBluetoothDevicesA(), muteOnWithDeviceList);
      }
   }

   muteConfig_.muteOnWlan = settings_.QueryValue(SettingsKey::MUTE_ON_WLAN);
   if (!muteConfig_.muteOnWlan) {
      wifiDetector_.Unload();
   } else {
      if (!wifiDetector_.Init(hWnd_)) {
         wmTray_.ShowPopup(
            i18n_.GetTranslationW("popup.wlan-muting-disabled.title"),
            i18n_.GetTranslationW("popup.wlan-muting-disabled.text"));
         settings_.SetValue(SettingsKey::MUTE_ON_WLAN, FALSE);
      } else {
         const bool isMuteList = !settings_.QueryValue(SettingsKey::MUTE_ON_WLAN_ALLOWLIST);
         wifiDetector_.SetNetworkList(settings_.GetWifiNetworks(), isMuteList);
         wifiDetector_.CheckNetwork();
      }
   }

   //GUID_MONITOR_POWER_ON
   return true;
}

void WinMute::ToggleMenuCheck(UINT item, bool* setting) noexcept
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

void WinMute::LoadMainMenuText()
{
   std::map<UINT, std::wstring> menuText;
   menuText[ID_TRAYMENU_INFO] = i18n_.GetTranslationW("traymenu.info");
   menuText[ID_TRAYMENU_LABEL_MUTEWHEN] = i18n_.GetTranslationW("traymenu.mute-when");
   menuText[ID_TRAYMENU_MUTEONLOCK] = i18n_.GetTranslationW("traymenu.mute-on-lock");
   menuText[ID_TRAYMENU_MUTEONSCREENSUSPEND] = i18n_.GetTranslationW("traymenu.mute-on-screen-suspend");
   menuText[ID_TRAYMENU_RESTOREAUDIO] = i18n_.GetTranslationW("traymenu.restore-volume");
   menuText[ID_TRAYMENU_LABEL_MUTEON_NO_RESTORE] = i18n_.GetTranslationW("traymenu.mute-no-restore");
   menuText[ID_TRAYMENU_MUTEONSHUTDOWN] = i18n_.GetTranslationW("traymenu.mute-on-shutdown");
   menuText[ID_TRAYMENU_MUTEONSUSPEND] = i18n_.GetTranslationW("traymenu.mute-on-sleep");
   menuText[ID_TRAYMENU_MUTEONLOGOUT] = i18n_.GetTranslationW("traymenu.mute-on-logout");
   menuText[ID_TRAYMENU_MUTE] = i18n_.GetTranslationW("traymenu.mute-all-devices");
   menuText[ID_TRAYMENU_SETTINGS] = i18n_.GetTranslationW("traymenu.settings");
   menuText[ID_TRAYMENU_EXIT] = i18n_.GetTranslationW("traymenu.exit");

   for (const auto& mt : menuText) {
      MENUITEMINFO mii{ sizeof(MENUITEMINFO) };
      if (!GetMenuItemInfo(hTrayMenu_, mt.first, false, &mii)) {
         continue;
      }
      mii.fMask = MIIM_TYPE;
      mii.fType = MFT_STRING;
      mii.dwTypeData = const_cast<LPWSTR>(mt.second.c_str());
      mii.cch = static_cast<UINT>(mt.second.length());
      if (!SetMenuItemInfo(hTrayMenu_, mt.first, false, &mii)) {
         continue;
      }
   }
}

void WinMute::CheckForUpdates()
{
   auto updateChecker = std::make_unique<UpdateChecker>();
   if (!updateChecker ||
       !updateChecker->IsUpdateCheckEnabled(settings_)) {
      return;
   }
   std::thread updateThread(&WinMute::CheckForUpdatesAsync, this, std::move(updateChecker));
   updateThread.detach();
}

void WinMute::CheckForUpdatesAsync(std::unique_ptr<UpdateChecker> updateChecker)
{
   const bool betaUpdates = settings_.QueryValue(SettingsKey::CHECK_FOR_BETA_UPDATE) != 0;
   if (!updateChecker) {
      return;
   }
   if (!updateChecker->GetUpdateInfo(updateInfo_)) {
      wmTray_.ShowPopup(
         i18n_.GetTranslationW("popup.error.update-check-failed.title"),
         i18n_.GetTranslationW("popup.error.update-check-failed.text"));
   } else if (updateInfo_.beta.shouldUpdate && betaUpdates) {
      const std::wstring popupTitle = std::vformat(
         i18n_.GetTranslationW("popup.update-available-beta.title"),
         std::make_wformat_args(updateInfo_.beta.version));
      const std::wstring popupText = std::vformat(
         i18n_.GetTranslationW("popup.update-available-beta.text"),
         std::make_wformat_args(updateInfo_.currentVersion));
      updateTray_.Show();
      updateTray_.ShowPopup(popupTitle, popupText);
   } else if (updateInfo_.stable.shouldUpdate) {
      const std::wstring popupTitle = std::vformat(
         i18n_.GetTranslationW("popup.update-available.title"),
         std::make_wformat_args(updateInfo_.stable.version));
      const std::wstring popupText = std::vformat(
         i18n_.GetTranslationW("popup.update-available.text"),
         std::make_wformat_args(updateInfo_.currentVersion));
      updateTray_.Show();
      updateTray_.ShowPopup(popupTitle, popupText);
   }
}

LRESULT WinMute::OnCommand(HWND hWnd, WPARAM wParam, LPARAM)
{
   switch (LOWORD(wParam)) {
   case ID_TRAYMENU_INFO:
   {
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
   case ID_TRAYMENU_RESTOREAUDIO: {
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

LRESULT WinMute::OnTrayIcon(HWND hWnd, WPARAM, LPARAM lParam)
{
   if (LOWORD(lParam) == WM_CONTEXTMENU ||
       lParam == WM_LBUTTONUP ||
       lParam == WM_RBUTTONUP) {
      POINT p = { 0 };
      GetCursorPos(&p);
      SetForegroundWindow(hWnd);
      TrackPopupMenuEx(
         GetSubMenu(hTrayMenu_, 0),
         TPM_NONOTIFY | TPM_TOPALIGN | TPM_LEFTALIGN,
         p.x, p.y, hWnd_, nullptr);
   }
   return TRUE;
}

LRESULT WinMute::OnUpdatePopup(HWND hWnd, WPARAM, LPARAM lParam)
{
   if (lParam == NIN_BALLOONUSERCLICK) {
      const bool betaUpdates = settings_.QueryValue(SettingsKey::CHECK_FOR_BETA_UPDATE) != 0;
      if (betaUpdates && updateInfo_.beta.shouldUpdate) {
         LaunchBrowser(hWnd, updateInfo_.beta.downloadUrl);
      } else if (updateInfo_.stable.shouldUpdate) {
         LaunchBrowser(hWnd, updateInfo_.stable.downloadUrl);
      }
   }
   if (lParam == NIN_BALLOONHIDE || lParam == NIN_BALLOONTIMEOUT || lParam == NIN_BALLOONUSERCLICK) {
      updateTray_.Hide();
   }
   return 0;
}


LRESULT WinMute::OnSettingChange(HWND, WPARAM, LPARAM)
{
   return 0;
}

LRESULT WinMute::OnPowerBroadcast(HWND, WPARAM wParam, LPARAM lParam)
{
   if (wParam == PBT_APMSUSPEND) {
      muteCtrl_.NotifySuspend(true);
   } else if (wParam == PBT_POWERSETTINGCHANGE) {
      const PPOWERBROADCAST_SETTING bs =
         reinterpret_cast<PPOWERBROADCAST_SETTING>(lParam);
      if (IsEqualGUID(bs->PowerSetting, GUID_CONSOLE_DISPLAY_STATE)) {
         const DWORD state = bs->Data[0];
         if (state == 0x0) { // Display standby
            muteCtrl_.NotifyDisplayStandby(true);
         } else if (state == 0x1) { // Display on
            muteCtrl_.NotifyDisplayStandby(false);
         } else if (state == 0x2) { // Display dimmed
         }
      }
   }
   return TRUE;
}

LRESULT WinMute::OnQuietHours(HWND, UINT msg, WPARAM, LPARAM)
{
   if (msg == WM_WINMUTE_QUIETHOURS_START) {
      muteCtrl_.NotifyQuietHours(true);
      if (settings_.QueryValue(SettingsKey::QUIETHOURS_NOTIFICATIONS)) {
         wmTray_.ShowPopup(
            i18n_.GetTranslationW("popup.quiet-hours-started.title"),
            i18n_.GetTranslationW("popup.quiet-hours-started.text"));
      }
      quietHours_.SetEnd();
      return 0;
   } else if (msg == WM_WINMUTE_QUIETHOURS_END) {
      muteCtrl_.NotifyQuietHours(false);
      if (settings_.QueryValue(SettingsKey::QUIETHOURS_NOTIFICATIONS)) {
         wmTray_.ShowPopup(
            i18n_.GetTranslationW("popup.quiet-hours-ended.title"),
            i18n_.GetTranslationW("popup.quiet-hours-ended.text"));
      }
      if (settings_.QueryValue(SettingsKey::QUIETHOURS_FORCEUNMUTE)) {
         muteCtrl_.SetMute(false);
      }
      quietHours_.SetStart();
   }
   return 0;
}

LRESULT WinMute::OnDeviceChange(HWND, UINT msg, WPARAM wParam, LPARAM lParam)
{
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

LRESULT WinMute::OnWifiStatusChange(HWND, WPARAM wParam, LPARAM lParam)
{
   if (!muteConfig_.muteOnWlan) {
      return 0;
   }
   if (wParam != 1) { // Not Connected
      return 0;
   }

   if (settings_.QueryValue(SettingsKey::NOTIFICATIONS_ENABLED)) {
      std::wstring popupMsg;
      wchar_t *wifiName = reinterpret_cast<wchar_t *>(lParam);
      if (settings_.QueryValue(SettingsKey::MUTE_ON_WLAN_ALLOWLIST)) {
         popupMsg = std::vformat(
            i18n_.GetTranslationW("popup.wlan-not-on-mute-list.text"),
            std::make_wformat_args(wifiName));
      } else {
         popupMsg = std::vformat(
            i18n_.GetTranslationW("popup.wlan-is-on-mute-list.text"),
            std::make_wformat_args(wifiName));
      }
      wmTray_.ShowPopup(
         i18n_.GetTranslationW("popup.workstation-muted.title"),
         popupMsg);
      delete[] wifiName;
   }
   muteCtrl_.SetMute(true);

   return 0;
}

LRESULT CALLBACK WinMute::WindowProc(
   HWND hWnd,
   UINT msg,
   WPARAM wParam,
   LPARAM lParam)
{
   static UINT uTaskbarRestart = 0;
   switch (msg) {
   case WM_CREATE:
      uTaskbarRestart = RegisterWindowMessageW(L"TaskbarCreated");
      return TRUE;
   case WM_COMMAND:
      return OnCommand(hWnd, wParam, lParam);
   case WM_WINMUTE_UPDATE_POPUP:
      return OnUpdatePopup(hWnd, wParam, lParam);
   case WM_TRAYICON:
      return OnTrayIcon(hWnd, wParam, lParam);
   case WM_CLOSE:
      Close();
      return 0;
   case WM_WTSSESSION_CHANGE: {
      if (wParam == WTS_SESSION_LOCK) {
         muteCtrl_.NotifyWorkstationLock(true);
      } else if (wParam == WTS_SESSION_UNLOCK) {
         muteCtrl_.NotifyWorkstationLock(false);
      }
      return 0;
   }
   case WM_POWERBROADCAST:
      return OnPowerBroadcast(hWnd, wParam, lParam);
   case WM_QUERYENDSESSION:
      return TRUE;
   case WM_ENDSESSION:
      if (wParam == TRUE) {
         if (lParam == 0) { // Shutdown
            muteCtrl_.NotifyShutdown();
         } else if ((lParam & ENDSESSION_LOGOFF)) {
            muteCtrl_.NotifyLogout();
         }
      }
      break;
   case WM_WINMUTE_QUIETHOURS_START: // fall through
   case WM_WINMUTE_QUIETHOURS_END:
      return OnQuietHours(hWnd, msg, wParam, lParam);
   case WM_DEVICECHANGE:
      return OnDeviceChange(hWnd, msg, wParam, lParam);
   case WM_WIFISTATUSCHANGED:
      return OnWifiStatusChange(hWnd, wParam, lParam);
   case WM_SETTINGCHANGE:
      return OnSettingChange(hWnd, wParam, lParam);
   default:
      if (msg == uTaskbarRestart) { // Restore trayicon if explorer.exe crashes
         wmTray_.Hide();
         wmTray_.Show();
      }
      break;
   }

   return DefWindowProc(hWnd, msg, wParam, lParam);
}

void WinMute::Unload() noexcept
{
   settings_.Unload();
}

void WinMute::Close()
{
   Unload();
   PostQuitMessage(0);
}
