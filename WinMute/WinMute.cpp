/*
 WinMute
           Copyright (c) 2021, Alexander Steinhoefer

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

#include "StdAfx.h"
#include "WinMute.h"
#include "WinAudio.h"

extern HINSTANCE hglobInstance;
extern INT_PTR CALLBACK AboutDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR CALLBACK QuietHoursDlgProc(HWND, UINT, WPARAM, LPARAM);

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

WinMute::MuteConfig::MuteConfig()
{
   this->withRestore.onLock = true;
   this->withRestore.onScreensaver = true;
   this->restoreAudio = true;

   this->noRestore.onLogoff = false;
   this->noRestore.onSuspend = false;
   this->noRestore.onShutdown = false;

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
   hTrayIcon_(nullptr),
   wasAlreadyMuted_(false),
   muteCounter_(0)
{ }

WinMute::~WinMute()
{
   Unload();
}

bool WinMute::RegisterWindowClass()
{
   WNDCLASS wc={0};
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
      audio_ = std::unique_ptr<WinAudio>(new VistaAudio);
   } else if (IsWindowsXPOrGreater()) {
         TaskDialog(nullptr, nullptr, PROGRAM_NAME,
           _T("Only Windows Vista and newer is supported"),
           _T("For Windows XP support, please download WinMute ")
           _T(" version 1.4.2 or older"),
           TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
      return false;
   }

   if (!audio_->Init(hWnd_)) {
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
      !CHECK_MENU_ITEM(MUTEONLOCK, muteConfig_.withRestore.onLock) ||
      !CHECK_MENU_ITEM(MUTEONSCREENSAVER, muteConfig_.withRestore.onScreensaver) ||
      !CHECK_MENU_ITEM(RESTOREAUDIO, muteConfig_.restoreAudio) ||
      !CHECK_MENU_ITEM(MUTEONSUSPEND, muteConfig_.noRestore.onSuspend) ||
      !CHECK_MENU_ITEM(MUTEONSHUTDOWN, muteConfig_.noRestore.onShutdown) ||
      !CHECK_MENU_ITEM(MUTEONLOGOUT, muteConfig_.noRestore.onLogoff) ||
      !CHECK_MENU_ITEM(CONFIGUREQUIETHOURS, muteConfig_.quietHours.enabled)) {
      return false;
   }
   return true;
}
#undef CHECK_MENU_ITEM

bool WinMute::Init()
{
   hAppIcon_ = LoadIcon(hglobInstance, MAKEINTRESOURCE(IDI_APP));

   if (!settings_.Init() ||
       !LoadDefaults()) {
      return false;
   }

   if (!RegisterWindowClass() ||
       !InitWindow() ||
       !InitTrayMenu()) {
      return false;
   }

   if (!scrnSaverNoti_.Init()) {
      return false;
   }

   if (muteConfig_.withRestore.onScreensaver) {
      if (!scrnSaverNoti_.ActivateNotifications(hWnd_)) {
         return false;
      }
   }

   if (!WTSRegisterSessionNotification(hWnd_, NOTIFY_FOR_THIS_SESSION)) {
      PrintWindowsError(_T("WTSRegisterSessionNotification"));
      return false;
   }

   if (!InitAudio()) {
      return false;
   }

   hTrayIcon_ = LoadIcon(hglobInstance, MAKEINTRESOURCE(IDI_TRAY));
   if (hTrayIcon_ == NULL) {
      PrintWindowsError(_T("LoadIcon"));
      return false;
   }
   trayIcon_.Init(hWnd_, 0, hTrayIcon_, _T("WinMute"), true);

   ResetQuietHours();

   return true;
}

bool WinMute::LoadDefaults()
{
   muteConfig_.withRestore.onLock =
      settings_.QueryValue(SettingsKey::MUTE_ON_LOCK, 1) != 0;
   muteConfig_.withRestore.onScreensaver =
      settings_.QueryValue(SettingsKey::MUTE_ON_SCREENSAVER, 1) != 0;
   muteConfig_.restoreAudio =
      settings_.QueryValue(SettingsKey::RESTORE_AUDIO, 1) != 0;

   muteConfig_.noRestore.onLogoff =
      settings_.QueryValue(SettingsKey::MUTE_ON_LOGOUT, 0) != 0;
   muteConfig_.noRestore.onSuspend =
      settings_.QueryValue(SettingsKey::MUTE_ON_SUSPEND, 0) != 0;
   muteConfig_.noRestore.onShutdown =
      settings_.QueryValue(SettingsKey::MUTE_ON_SHUTDOWN, 0) != 0;

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
      case ID_TRAYMENU_CONFIGUREQUIETHOURS:
         DialogBoxParam(
            hglobInstance,
            MAKEINTRESOURCE(IDD_QUIETHOURS),
            hWnd_,
            QuietHoursDlgProc,
            reinterpret_cast<LPARAM>(&settings_));
         break;
      case ID_TRAYMENU_EXIT:
         SendMessage(hWnd, WM_CLOSE, 0, 0);
         break;
      case ID_TRAYMENU_MUTE: {
         bool state = false;
         ToggleMenuCheck(ID_TRAYMENU_MUTE, &state);
         if (!state) {
            Log::GetInstance().Write("Mute: Off | Manual per Menu");
            audio_->UnMute();
         }
         else {
            Log::GetInstance().Write("Mute: On | Manual per Menu");
            audio_->Mute();
         }
         break;
      }
      case ID_TRAYMENU_MUTEONLOCK:
         ToggleMenuCheck(ID_TRAYMENU_MUTEONLOCK,
            &muteConfig_.withRestore.onLock);
         settings_.SetValue(SettingsKey::MUTE_ON_LOCK,
            muteConfig_.withRestore.onLock);
         break;
      case ID_TRAYMENU_RESTOREAUDIO:
         ToggleMenuCheck(ID_TRAYMENU_RESTOREAUDIO, &muteConfig_.restoreAudio);
         settings_.SetValue(SettingsKey::RESTORE_AUDIO,
            muteConfig_.restoreAudio);
         break;
      case ID_TRAYMENU_MUTEONSCREENSAVER:
         ToggleMenuCheck(ID_TRAYMENU_MUTEONSCREENSAVER,
            &muteConfig_.withRestore.onScreensaver);
         settings_.SetValue(SettingsKey::MUTE_ON_SCREENSAVER,
            muteConfig_.withRestore.onScreensaver);
         if (muteConfig_.withRestore.onScreensaver) {
            scrnSaverNoti_.ActivateNotifications(hWnd_);
         }
         else {
            scrnSaverNoti_.ClearNotifications();
         }
         break;
      case ID_TRAYMENU_MUTEONSHUTDOWN:
         ToggleMenuCheck(ID_TRAYMENU_MUTEONSHUTDOWN,
            &muteConfig_.noRestore.onShutdown);
         settings_.SetValue(SettingsKey::MUTE_ON_SHUTDOWN,
            muteConfig_.noRestore.onShutdown);
         break;
      case ID_TRAYMENU_MUTEONSUSPEND:
         ToggleMenuCheck(ID_TRAYMENU_MUTEONSUSPEND,
            &muteConfig_.noRestore.onSuspend);
         settings_.SetValue(SettingsKey::MUTE_ON_SUSPEND,
            muteConfig_.noRestore.onSuspend);
         break;
      case ID_TRAYMENU_MUTEONLOGOUT:
         ToggleMenuCheck(ID_TRAYMENU_MUTEONLOGOUT,
            &muteConfig_.noRestore.onLogoff);
         settings_.SetValue(SettingsKey::MUTE_ON_LOGOUT,
            muteConfig_.noRestore.onLogoff);
         break;
      }
      return 0;
   }
   case WM_TRAYICON: {
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
         if (muteCounter_ == 0 && audio_->IsMuted()) {
            muteCounter_ = 1;
         }
         if (muteConfig_.withRestore.onLock) {
            if (muteCounter_ == 0) {
               Log::GetInstance().Write("Mute: Off | Workstation Lock");
               audio_->Mute();
            }
            muteCounter_ += 1;
         }
      }
      else if (wParam == WTS_SESSION_UNLOCK) {
         if (muteConfig_.restoreAudio &&
             muteConfig_.withRestore.onLock) {
            if (--muteCounter_ == 0) {
               Log::GetInstance().Write("Mute: Off | Workstation Unlock");
               audio_->UnMute();
            }
         }
      }
      return 0;
   }
   case WM_POWERBROADCAST:
      if (wParam == PBT_APMSUSPEND) {
         if (muteConfig_.noRestore.onSuspend) {
            Log::GetInstance().Write("Mute: On | Suspend/Hibernate");
            audio_->Mute();
         }
      }
      break;
   case WM_QUERYENDSESSION:
      if (lParam == 0) { // Shutdown
         if (muteConfig_.noRestore.onShutdown) {
            Log::GetInstance().Write("Mute: On | Shutdown");
            audio_->Mute();
         }
      }
      else if ((lParam & ENDSESSION_LOGOFF)) {
         if (muteConfig_.noRestore.onLogoff) {
            Log::GetInstance().Write("Mute: On | Logoff");
            audio_->Mute();
         }
      }
      break;
   case WM_WINMUTE_QUIETHOURS_START:
      muteCounter_ = !!audio_->IsMuted();
      Log::GetInstance().Write("Mute: On | Quiet Hours started");
      audio_->Mute();
      muteCounter_ += 1;
      if (muteConfig_.quietHours.notifications) {
         trayIcon_.ShowPopup(
            _T("WinMute: Quiet hours started"),
            _T("Your workstation audio is now muted"));
      }
      SetQuietHoursEnd();
      return 0;
   case WM_WINMUTE_QUIETHOURS_END:
      muteCounter_ -= 1;
      if (muteCounter_ > 0 && muteConfig_.quietHours.forceUnmute ||
          muteCounter_ == 0) {
         Log::GetInstance().Write("Mute: Off | Quiet Hours ended");
         audio_->UnMute();
         if (muteConfig_.quietHours.notifications) {
            trayIcon_.ShowPopup(
               _T("WinMute: Quiet Hours ended"),
               _T("Your workstation audio has been restored."));
         }
      } else {
         if (muteConfig_.quietHours.notifications) {
            trayIcon_.ShowPopup(
               _T("WinMute: Quiet Hours ended"),
               _T("Quiet hours ended, since your your audio was already muted ")
               _T("and force unmute is off, your workstation will remain muted."));
         }
      }
      SetQuietHoursStart();
      return 0;
   case WM_WINMUTE_MUTE: {
      bool mute = !!static_cast<int>(wParam);
      if (mute) {
         Log::GetInstance().Write("Mute: On | Per Windows-Message");
         audio_->Mute();
      } else {
         Log::GetInstance().Write("Mute: Off | Per Windows-Message");
         audio_->UnMute();
      }
      return 0;
   }
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
         muteCounter_ = !!audio_->IsMuted();
         if (muteConfig_.withRestore.onScreensaver) {
            if (muteCounter_ == 0) {
               Log::GetInstance().Write("Mute: On | Screensaver start");
               audio_->Mute();
            }
            muteCounter_ += 1;
         }
      } else if (wParam == SCRNSAVE_STOP) {
         if (muteConfig_.restoreAudio &&
             muteConfig_.withRestore.onScreensaver) {
            if (--muteCounter_ == 0) {
               Log::GetInstance().Write("Mute: Off | Screensaver start");
               audio_->UnMute();
            }
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
   UNREFERENCED_PARAMETER( msg );
   UNREFERENCED_PARAMETER( msSinceSysStart );
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
         Log::GetInstance().Write("Mute: On | Quiet hours have already started");
         audio_->Mute();
         if (SetTimer(hWnd_, QUIETHOURS_TIMER_END_ID, timerQhEnd, QuietHoursTimer) == 0) {
            MessageBox(hWnd_, _T("Failed to create Timer"), PROGRAM_NAME, MB_OK);
         }
      } else {
         int timerQhStart = GetDiffMillseconds(&start, &now);
         if (SetTimer(hWnd_, QUIETHOURS_TIMER_START_ID, timerQhStart, QuietHoursTimer) == 0) {
            MessageBox(hWnd_, _T("Failed to create Timer"), PROGRAM_NAME, MB_OK);
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
      MessageBox(hWnd_, _T("Failed to create Timer"), PROGRAM_NAME, MB_OK);
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
      MessageBox(hWnd_, _T("Failed to create Timer"), PROGRAM_NAME, MB_OK);
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
