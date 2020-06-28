/*
 WinMute
           Copyright (c) 2017, Alexander Steinhoefer

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
   this->withRestore.muteOnLock = true;
   this->withRestore.muteOnScreensaver = true;
   this->withRestore.muteOnRemoteSession = true;
   this->restoreAudio = true;

   this->noRestore.muteOnLogoff = false;
   this->noRestore.muteOnSuspend = false;
   this->noRestore.muteOnShutdown = false;
}

WinMute::WinMute() :
   hWnd_(nullptr),
   hTrayMenu_(nullptr),
   hAppIcon_(nullptr),
   hTrayIcon_(nullptr)
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
      WS_POPUP, 0, 0, 0, 0, HWND_MESSAGE, 0, hglobInstance, this);
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

#define CHECK_MENU_ITEM(cond) (cond) ? MF_CHECKED : MF_UNCHECKED
bool WinMute::InitTrayMenu()
{
   hTrayMenu_ = LoadMenu(hglobInstance, MAKEINTRESOURCE(IDR_TRAYMENU));
   if (!hTrayMenu_) {
      PrintWindowsError(_T("LoadMenu"));
      return false;
   } else if (
      CheckMenuItem(hTrayMenu_, ID_TRAYMENU_MUTEONLOCK,
         CHECK_MENU_ITEM(muteConfig_.withRestore.muteOnLock)) == -1 ||
      CheckMenuItem(hTrayMenu_, ID_TRAYMENU_MUTEONSCREENSAVER,
         CHECK_MENU_ITEM(muteConfig_.withRestore.muteOnScreensaver)) == -1 ||
      CheckMenuItem(hTrayMenu_, ID_TRAYMENU_RESTOREAUDIO,
         CHECK_MENU_ITEM(muteConfig_.restoreAudio)) == -1) {
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

   if (muteConfig_.withRestore.muteOnScreensaver) {
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
   return true;
}

bool WinMute::LoadDefaults()
{
   muteConfig_.withRestore.muteOnLock =
      settings_.QueryValue(SettingsKey::MUTE_ON_LOCK, 1) != 0;
   muteConfig_.withRestore.muteOnScreensaver =
      settings_.QueryValue(SettingsKey::MUTE_ON_SCREENSAVER, 1) != 0;
   muteConfig_.restoreAudio =
      settings_.QueryValue(SettingsKey::RESTORE_AUDIO, 1) != 0;
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

LRESULT CALLBACK WinMute::WindowProc(HWND hWnd, UINT msg,
   WPARAM wParam, LPARAM lParam)
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
         DialogBox(hglobInstance,
                   MAKEINTRESOURCE(IDD_ABOUT),
                   hWnd_,
                   AboutDlgProc);
         break;
      case ID_TRAYMENU_EXIT:
         SendMessage(hWnd, WM_CLOSE, 0, 0);
         break;
      case ID_TRAYMENU_MUTEONLOCK:
         ToggleMenuCheck(ID_TRAYMENU_MUTEONLOCK,
                         &muteConfig_.withRestore.muteOnLock);
         settings_.SetValue(SettingsKey::MUTE_ON_LOCK,
                            muteConfig_.withRestore.muteOnLock);
         break;
      case ID_TRAYMENU_RESTOREAUDIO:
         ToggleMenuCheck(ID_TRAYMENU_RESTOREAUDIO, &muteConfig_.restoreAudio);
         settings_.SetValue(SettingsKey::RESTORE_AUDIO,
                            muteConfig_.restoreAudio);
         break;
      case ID_TRAYMENU_MUTEONSCREENSAVER:
         ToggleMenuCheck(ID_TRAYMENU_MUTEONSCREENSAVER,
                         &muteConfig_.withRestore.muteOnScreensaver);
         settings_.SetValue(SettingsKey::MUTE_ON_SCREENSAVER,
                            muteConfig_.withRestore.muteOnScreensaver);
         if (muteConfig_.withRestore.muteOnScreensaver) {
            scrnSaverNoti_.ActivateNotifications(hWnd_);
         } else {
            scrnSaverNoti_.ClearNotifications();
         }
         break;
      case ID_TRAYMENU_MUTEONSHUTDOWN:
          break;
      case ID_TRAYMENU_MUTEONSUSPEND:
          break;
      case ID_TRAYMENU_MUTEONLOGOFF:
          break;
      case ID_TRAYMENU_MUTE: {
         bool state = false;
         ToggleMenuCheck(ID_TRAYMENU_MUTE, &state);
         if (!state) {
            audio_->UnMute();
         } else {
            audio_->Mute();
         }
         break;
      } default:
         __assume(0);
      }
      return 0;
   }
   case WM_TRAYICON: {
      switch (lParam) {
      case WM_LBUTTONUP:
      case WM_RBUTTONUP: {
         POINT p = {0};
         GetCursorPos(&p);
         SetForegroundWindow(hWnd);
         TrackPopupMenuEx(GetSubMenu(hTrayMenu_, 0), TPM_NONOTIFY | TPM_TOPALIGN
            | TPM_LEFTALIGN , p.x, p.y, hWnd_, nullptr);
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
      static bool wasAlreadyMuted = false;
      if (wParam == WTS_SESSION_LOCK) {
         wasAlreadyMuted = audio_->IsMuted();
         if (!wasAlreadyMuted && muteConfig_.withRestore.muteOnLock) {
            audio_->Mute();
         }
      } else if (wParam == WTS_SESSION_UNLOCK) {
         if (!wasAlreadyMuted &&
             muteConfig_.restoreAudio &&
             muteConfig_.withRestore.muteOnLock) {
            audio_->UnMute();
         }
      } else if (wParam == WTS_SESSION_LOGOFF) {
          if (muteConfig_.noRestore.muteOnLogoff) {
              audio_->Mute();
          }
      } else if (wParam == WTS_REMOTE_DISCONNECT) {
          wasAlreadyMuted = audio_->IsMuted();
          if (!wasAlreadyMuted &&
              muteConfig_.withRestore.muteOnRemoteSession) {
              audio_->Mute();
          }
      } else if (wParam == WTS_REMOTE_DISCONNECT) {
          if (!wasAlreadyMuted &&
              muteConfig_.restoreAudio &&
              muteConfig_.withRestore.muteOnRemoteSession) {
              audio_->UnMute();
          }
      }
      return 0;
   }
   case WM_SCRNSAVE_CHANGE: {
      static bool wasAlreadyMuted = false;
      if (wParam == SCRNSAVE_START) {
         wasAlreadyMuted = audio_->IsMuted();
         if (!wasAlreadyMuted && muteConfig_.withRestore.muteOnScreensaver) {
            audio_->Mute();
         }
      } else if (wParam == SCRNSAVE_STOP) {
         if (!wasAlreadyMuted && 
             muteConfig_.restoreAudio &&
             muteConfig_.withRestore.muteOnScreensaver) {
            audio_->UnMute();
         }
      }
      return 0;
   }
   default:
      if (msg == uTaskbarRestart) { // restore trayicon if explorer.exe crashes
         trayIcon_.Hide();
         trayIcon_.Show();
      }
      break;
   }

   return DefWindowProc(hWnd, msg, wParam, lParam);
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
