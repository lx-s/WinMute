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
extern INT_PTR CALLBACK QuietHoursDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR CALLBACK SettingsDlgProc(HWND, UINT, WPARAM, LPARAM);

static LPCTSTR WINMUTE_CLASS_NAME = _T("WinMute");

static LRESULT CALLBACK WinMuteWndProc(HWND hWnd, UINT msg, WPARAM wParam,
   LPARAM lParam)
{
   auto wm = reinterpret_cast<WinMute*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
   switch (msg) {
   case WM_NCCREATE: {
      LPCREATESTRUCTW cs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
      SetWindowLongPtr(hWnd, GWLP_USERDATA,
         reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
      return TRUE;
   }
   default:
      break;
   }
   return (wm) ? wm->WindowProc(hWnd, msg, wParam, lParam)
      : DefWindowProc(hWnd, msg, wParam, lParam);
}

static int IsDarkMode(bool& isDarkMode)
{
   HKEY hKey;
   int rc = 1;
   //RegOpenCurrentUser
   LSTATUS error = RegOpenKey(
      HKEY_CURRENT_USER,
      _T("Software\\Microsoft\\Windows\\CurrentVersion\\")
      _T("Themes\\Personalize"),
      &hKey);
   if (error != ERROR_SUCCESS) {
      PrintWindowsError(_T("RegOpenKey"), error);
   } else {
      DWORD isLightTheme = 0;
      DWORD bufSize = sizeof(isLightTheme);
      DWORD valType = 0;
      error = RegQueryValueEx(
         hKey,
         _T("SystemUsesLightTheme"),
         NULL,
         &valType,
         reinterpret_cast<LPBYTE>(&isLightTheme),
         &bufSize);
      if (error != ERROR_SUCCESS) {
         PrintWindowsError(_T("RegQueryValueEx"), error);
      } else {
         isDarkMode = !isLightTheme;
         rc = 0;
      }
      RegCloseKey(hKey);
   }
   return rc;
}

WinMute::MuteConfig::MuteConfig()
{
   this->quietHours.enabled = false;
   this->quietHours.forceUnmute = false;
   this->quietHours.notifications = false;
   this->quietHours.start = 0;
   this->quietHours.end = 0;
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
      PrintWindowsError(_T("RegisterClass"));
      return false;
   }
   return true;
}

bool WinMute::InitWindow()
{
   hWnd_ = CreateWindowEx(WS_EX_TOOLWINDOW, WINMUTE_CLASS_NAME, PROGRAM_NAME,
      WS_POPUP, 0, 0, 0, 0, NULL, 0, hglobInstance, this);
   if (hWnd_ == nullptr) {
      PrintWindowsError(_T("CreateWindowEx"));
      return false;
   }
   return true;
}

bool WinMute::InitAudio()
{
   if (IsWindowsVistaOrGreater()) {
      // Nothing to do
   } else if (IsWindowsXPOrGreater()) {
      TaskDialog(nullptr, nullptr, PROGRAM_NAME,
         _T("Only Windows Vista and newer is supported"),
         _T("For Windows XP support, please download WinMute ")
         _T(" version 1.4.2 or older"),
         TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
      return false;
   }

   if (!muteCtrl_.Init(hWnd_)) {
      return false;
   }
   

   return true;
}

#define CHECK_MENU_ITEM(id, cond) \
   (CheckMenuItem(hTrayMenu_, ID_TRAYMENU_ ## id, \
                  (cond) ? MF_CHECKED : MF_UNCHECKED) != -1)
bool WinMute::InitTrayMenu()
{
   hTrayMenu_ = LoadMenu(hglobInstance, MAKEINTRESOURCE(IDR_TRAYMENU));
   if (!hTrayMenu_) {
      PrintWindowsError(_T("LoadMenu"));
      return false;
   } else if (
      !CHECK_MENU_ITEM(MUTEONLOCK, muteCtrl_.GetMuteOnWorkstationLock()) ||
      !CHECK_MENU_ITEM(MUTEONSCREENSAVER, muteCtrl_.GetMuteOnScreensaverActivation()) ||
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

   hAppIcon_ = LoadIcon(hglobInstance, MAKEINTRESOURCE(IDI_APP));

   if (!settings_.Init() ||
       !LoadDefaults()) {
      return false;
   }

#ifdef _DEBUG
   WMLog::GetInstance().SetEnabled(true);
#else
   WMLog::GetInstance().SetEnabled(settings_.QueryValue(LOGGING_ENABLED, 0));
#endif
   log.Write(_T("Starting new session..."));

   if (!RegisterWindowClass() ||
       !InitWindow() ||
       !InitTrayMenu()) {
      return false;
   }

   if (!InitAudio()) {
      return false;
   }

   if (!scrnSaverNoti_.Init()) {
      return false;
   }

   if (muteCtrl_.GetMuteOnScreensaverActivation()) {
      if (!scrnSaverNoti_.ActivateNotifications(hWnd_)) {
         return false;
      }
   }

   if (!WTSRegisterSessionNotification(hWnd_, NOTIFY_FOR_THIS_SESSION)) {
      PrintWindowsError(_T("WTSRegisterSessionNotification"));
      return false;
   }

   bool isDarkMode = true;
   IsDarkMode(isDarkMode);
   hTrayIcon_ = LoadIcon(
      hglobInstance,
      isDarkMode ? MAKEINTRESOURCE(IDI_TRAY_DARK)
                 : MAKEINTRESOURCE(IDI_TRAY_BRIGHT));
   if (hTrayIcon_ == NULL) {
      PrintWindowsError(_T("LoadIcon"));
      return false;
   }
   trayIcon_.Init(hWnd_, 0, hTrayIcon_, _T("WinMute"), true);

   ResetQuietHours();

   log.Write(_T("WinMute initialized"));

   return true;
}

bool WinMute::LoadDefaults()
{
   muteCtrl_.SetRestoreVolume(!!settings_.QueryValue(SettingsKey::RESTORE_AUDIO, 1));
   muteCtrl_.SetMuteOnWorkstationLock(!!settings_.QueryValue(SettingsKey::MUTE_ON_LOCK, 1));
   muteCtrl_.SetMuteOnScreensaverActivation(!!settings_.QueryValue(SettingsKey::MUTE_ON_SCREENSAVER, 1));
   muteCtrl_.SetMuteOnDisplayStandby(!!settings_.QueryValue(SettingsKey::MUTE_ON_DISPLAYSTANDBY, 1));
   muteCtrl_.SetMuteOnLogout(!!settings_.QueryValue(SettingsKey::MUTE_ON_LOGOUT, 0));
   muteCtrl_.SetMuteOnSuspend(!!settings_.QueryValue(SettingsKey::MUTE_ON_SUSPEND, 0));
   muteCtrl_.SetMuteOnShutdown(!!settings_.QueryValue(SettingsKey::MUTE_ON_SHUTDOWN, 0));

   muteConfig_.quietHours.enabled =
      settings_.QueryValue(SettingsKey::QUIETHOURS_ENABLE, 0) != 0;
   muteConfig_.quietHours.forceUnmute =
      settings_.QueryValue(SettingsKey::QUIETHOURS_FORCEUNMUTE, 0) != 0;
   muteConfig_.quietHours.notifications =
      settings_.QueryValue(SettingsKey::QUIETHOURS_NOTIFICATIONS, 0) != 0;
   muteConfig_.quietHours.start =
      settings_.QueryValue(SettingsKey::QUIETHOURS_START, 0);
   muteConfig_.quietHours.end =
      settings_.QueryValue(SettingsKey::QUIETHOURS_END, 0);

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
      uTaskbarRestart = RegisterWindowMessage(_T("TaskbarCreated"));
      return TRUE;
   }
   case WM_COMMAND: {
      switch (LOWORD(wParam)) {
      case ID_TRAYMENU_INFO:
         DialogBox(
            hglobInstance,
            MAKEINTRESOURCE(IDD_ABOUT),
            hWnd_,
            AboutDlgProc);
         break;
      case ID_TRAYMENU_EXIT:
         SendMessage(hWnd, WM_CLOSE, 0, 0);
         break;
      case ID_TRAYMENU_SETTINGS:
         DialogBoxParam(
            hglobInstance,
            MAKEINTRESOURCE(IDD_SETTINGS),
            hWnd_,
            SettingsDlgProc,
            reinterpret_cast<LPARAM>(&settings_));
         break;
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
      case ID_TRAYMENU_MUTEONSCREENSAVER: {
         bool checked = false;
         ToggleMenuCheck(ID_TRAYMENU_MUTEONSCREENSAVER, &checked);
         muteCtrl_.SetMuteOnScreensaverActivation(checked);
         settings_.SetValue(SettingsKey::MUTE_ON_SCREENSAVER, checked);
         if (checked) {
            scrnSaverNoti_.ActivateNotifications(hWnd_);
         } else {
            scrnSaverNoti_.ClearNotifications();
         }
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
         ToggleMenuCheck(ID_TRAYMENU_MUTEONLOGOUT, &checked);
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
         TrackPopupMenuEx(GetSubMenu(hTrayMenu_, 0), TPM_NONOTIFY | TPM_TOPALIGN
            | TPM_LEFTALIGN, p.x, p.y, hWnd_, nullptr);
         PostMessage(hWnd, WM_NULL, 0, 0);
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
      break;
   case WM_QUERYENDSESSION:
      if (lParam == 0) { // Shutdown
         muteCtrl_.NotifyShutdown();
      } else if ((lParam & ENDSESSION_LOGOFF)) {
         muteCtrl_.NotifyLogout();
      }
      break;
   case WM_WINMUTE_QUIETHOURS_START:
      muteCtrl_.NotifyQuietHours(true);
      if (muteConfig_.quietHours.notifications) {
         trayIcon_.ShowPopup(
            _T("WinMute: Quiet hours started"),
            _T("Your workstation audio will now be muted."));
      }
      SetQuietHoursEnd();
      return 0;
   case WM_WINMUTE_QUIETHOURS_END:
      muteCtrl_.NotifyQuietHours(false);
      trayIcon_.ShowPopup(
         _T("WinMute: Quiet Hours ended"),
         _T("Your workstation audio has been restored."));
      if (muteConfig_.quietHours.forceUnmute) {
         muteCtrl_.SetMute(false);
      }
      SetQuietHoursStart();
      return 0;
   case WM_WINMUTE_QUIETHOURS_CHANGE: {
      muteConfig_.quietHours.enabled = settings_.QueryValue(
         SettingsKey::QUIETHOURS_ENABLE,
         0);
      muteConfig_.quietHours.forceUnmute = settings_.QueryValue(
         SettingsKey::QUIETHOURS_FORCEUNMUTE,
         0);
      muteConfig_.quietHours.notifications = settings_.QueryValue(
         SettingsKey::QUIETHOURS_NOTIFICATIONS,
         0);
      CheckMenuItem(
         hTrayMenu_,
         ID_TRAYMENU_CONFIGUREQUIETHOURS,
         (muteConfig_.quietHours.enabled) ? MF_CHECKED : MF_UNCHECKED);

      if (muteConfig_.quietHours.enabled) {
         muteConfig_.quietHours.start = settings_.QueryValue(
            SettingsKey::QUIETHOURS_START,
            0);
         muteConfig_.quietHours.end = settings_.QueryValue(
            SettingsKey::QUIETHOURS_END,
            0);
      }
      ResetQuietHours();

      return 0;
   }
   case WM_SCRNSAVE_CHANGE: {
      if (wParam == SCRNSAVE_START) {
         muteCtrl_.NotifyScreensaver(true);
      } else if (wParam == SCRNSAVE_STOP) {
         muteCtrl_.NotifyScreensaver(false);
      }
      return 0;
   }
   case WM_SETTINGCHANGE: {
      if (lstrcmp(LPCTSTR(lParam), _T("ImmersiveColorSet")) == 0) {
         bool isDarkMode = true;
         IsDarkMode(isDarkMode);
         hTrayIcon_ = LoadIcon(
            hglobInstance,
            isDarkMode ? MAKEINTRESOURCE(IDI_TRAY_DARK)
            : MAKEINTRESOURCE(IDI_TRAY_BRIGHT));
         if (hTrayIcon_ == NULL) {
            PrintWindowsError(_T("LoadIcon"));
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

static __int64 ConvertSystemTimeTo100NS(const LPSYSTEMTIME sysTime)
{
   FILETIME ft;
   SystemTimeToFileTime(sysTime, &ft);
   ULARGE_INTEGER ulInt;
   ulInt.HighPart = ft.dwHighDateTime;
   ulInt.LowPart = ft.dwLowDateTime;
   return static_cast<__int64>(ulInt.QuadPart);
}

static bool QuietHoursShouldAlreadyHaveStarted(
   const LPSYSTEMTIME now,
   const LPSYSTEMTIME qhStart,
   const LPSYSTEMTIME qhEnd)
{
   __int64 iStart = ConvertSystemTimeTo100NS(qhStart);
   __int64 iEnd = ConvertSystemTimeTo100NS(qhEnd);
   __int64 iNow = ConvertSystemTimeTo100NS(now);

   if (iEnd < iStart) {
      // Add one day
      iEnd += 864000000000ll;
   }

   if (iEnd - iNow > 0 && iStart - iNow < 0) {
      return true;
   }

   return false;
}

/**
 * Gets the number of milliseconds that t2 would have to add, to reach t1.
 * Takes day wrap around into consideration.
 */
static int GetDiffMillseconds(const LPSYSTEMTIME t1, const LPSYSTEMTIME t2)
{
   __int64 it1 = ConvertSystemTimeTo100NS(t1);
   __int64 it2 = ConvertSystemTimeTo100NS(t2);

   // 100NS to Milliseconds;
   it1 /= 10000;
   it2 /= 10000;
   if (it1 < it2) { // add one day wrap around
      it1 += 24 * 60 * 60 * 1000;
   }
   __int64 res = it1 - it2;

   return static_cast<int>(res);
}

VOID CALLBACK QuietHoursTimer(HWND hWnd, UINT msg, UINT_PTR id, DWORD msSinceSysStart)
{
   UNREFERENCED_PARAMETER(msg);
   UNREFERENCED_PARAMETER(msSinceSysStart);
   if (id == QUIETHOURS_TIMER_START_ID) {
      KillTimer(hWnd, id);
      SendMessage(hWnd, WM_WINMUTE_QUIETHOURS_START, 0, 0);
   } else if (id == QUIETHOURS_TIMER_END_ID) {
      KillTimer(hWnd, id);
      SendMessage(hWnd, WM_WINMUTE_QUIETHOURS_END, 0, 0);
   }
}

void WinMute::ResetQuietHours()
{
   if (!muteConfig_.quietHours.enabled) {
      KillTimer(hWnd_, QUIETHOURS_TIMER_START_ID);
      KillTimer(hWnd_, QUIETHOURS_TIMER_END_ID);
   } else {
      SYSTEMTIME now;
      SYSTEMTIME start;
      SYSTEMTIME end;
      GetLocalTime(&now);
      GetLocalTime(&start);
      GetLocalTime(&end);

      start.wSecond = static_cast<WORD>(muteConfig_.quietHours.start % 60);
      start.wMinute = static_cast<WORD>(((muteConfig_.quietHours.start - start.wSecond) / 60) % 60);
      start.wHour = static_cast<WORD>((muteConfig_.quietHours.start - start.wMinute - start.wSecond) / 3600);

      end.wSecond = static_cast<WORD>(muteConfig_.quietHours.end % 60);
      end.wMinute = static_cast<WORD>(((muteConfig_.quietHours.end - end.wSecond) / 60) % 60);
      end.wHour = static_cast<WORD>((muteConfig_.quietHours.end - end.wMinute - end.wSecond) / 3600);

      if (QuietHoursShouldAlreadyHaveStarted(&now, &start, &end)) {
         int timerQhEnd = GetDiffMillseconds(&end, &now);
         WMLog::GetInstance().Write(L"Mute: On | Quiet hours have already started");
         muteCtrl_.NotifyQuietHours(true);
         if (SetTimer(hWnd_, QUIETHOURS_TIMER_END_ID, timerQhEnd, QuietHoursTimer) == 0) {
            MessageBox(hWnd_, L"Failed to create Timer", PROGRAM_NAME, MB_OK);
         }
      } else {
         int timerQhStart = GetDiffMillseconds(&start, &now);
         if (SetTimer(hWnd_, QUIETHOURS_TIMER_START_ID, timerQhStart, QuietHoursTimer) == 0) {
            MessageBox(hWnd_, L"Failed to create Timer", PROGRAM_NAME, MB_OK);
         }
      }
   }
}

void WinMute::SetQuietHoursStart()
{
   SYSTEMTIME now;
   SYSTEMTIME start;

   GetLocalTime(&now);
   GetLocalTime(&start);

   start.wSecond = static_cast<WORD>(muteConfig_.quietHours.start % 60);
   start.wMinute = static_cast<WORD>(((muteConfig_.quietHours.start - start.wSecond) / 60) % 60);
   start.wHour = static_cast<WORD>((muteConfig_.quietHours.start - start.wMinute - start.wSecond) / 3600);

   int timerQhStart = GetDiffMillseconds(&start, &now);
   if (SetTimer(hWnd_, QUIETHOURS_TIMER_START_ID, timerQhStart, QuietHoursTimer) == 0) {
      MessageBox(hWnd_, L"Failed to create Timer", PROGRAM_NAME, MB_OK);
   }
}

void WinMute::SetQuietHoursEnd()
{
   SYSTEMTIME now;
   SYSTEMTIME end;

   GetLocalTime(&now);
   GetLocalTime(&end);

   end.wSecond = static_cast<WORD>(muteConfig_.quietHours.end % 60);
   end.wMinute = static_cast<WORD>(((muteConfig_.quietHours.end - end.wSecond) / 60) % 60);
   end.wHour = static_cast<WORD>((muteConfig_.quietHours.end - end.wMinute - end.wSecond) / 3600);

   int timerQhEnd = GetDiffMillseconds(&end, &now);
   if (timerQhEnd <= 0) {
      SendMessage(hWnd_, WM_WINMUTE_QUIETHOURS_END, 0, 0);
   } else if (SetTimer(hWnd_, QUIETHOURS_TIMER_END_ID, timerQhEnd, QuietHoursTimer) == 0) {
      MessageBox(hWnd_, L"Failed to create Timer", PROGRAM_NAME, MB_OK);
   }
}

void WinMute::Unload()
{
   scrnSaverNoti_.Unload();
   settings_.Unload();
}

void WinMute::Close()
{
   Unload();
   PostQuitMessage(0);
}
