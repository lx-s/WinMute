/*
 WinMute
           Copyright (c) 2020, Alexander Steinhoefer

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
#include "ScreensaverNotifier.h"

/* ========================================================================== */
/*    Globals                                                                 */
/* ========================================================================== */

extern HINSTANCE hglobInstance;

static LPCTSTR SCRSVER_NOTIFY_WND_CLASS = _T("LXS_WinMuteScreensaverNotifyClass");

#define SCRSV_TIMER_ID 1

/* ========================================================================== */
/*    Window Helper                                                           */
/* ========================================================================== */


static LRESULT CALLBACK ScrsvrWndProc(HWND hWnd, UINT msg, WPARAM wParam,
                                     LPARAM lParam)
{
   auto sn = reinterpret_cast<ScreensaverNotifier*>(
                                         GetWindowLongPtr(hWnd, GWLP_USERDATA));
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
   return (sn) ? sn->WindowProc(hWnd, msg, wParam, lParam)
               : DefWindowProc(hWnd, msg, wParam, lParam);
}

/* ========================================================================== */
/*    Construction, Destruction                                               */
/* ========================================================================== */

ScreensaverNotifier::ScreensaverNotifier() :
   alreadyNotified_(false),
   isRegistered_(false),
   hookDll_(nullptr),
   hWnd_(nullptr),
   hNotifyWnd_(nullptr),
   hJob_(nullptr),
   hookWndMsg_(0),
   regHook_(nullptr),
   unregHook_(nullptr)
{ }

ScreensaverNotifier::~ScreensaverNotifier()
{
   Unload();
}

/* ========================================================================== */
/*    Init, Unload                                                            */
/* ========================================================================== */

bool ScreensaverNotifier::Init()
{
   if (hookDll_ != nullptr) {
      return true;
   }
   return RegisterWindowClass() &&
      InitWindow() &&
      InitWindowMessage() &&
      InitHookDll() &&
      InitHook32();
}

bool ScreensaverNotifier::InitWindow()
{
   hWnd_ = CreateWindowEx(
      WS_EX_TOOLWINDOW,
      SCRSVER_NOTIFY_WND_CLASS,
      _T("LXS_WinMute_ScreensaverNotify"),
      WS_POPUP,
      0, 0, 0, 0,
      NULL,
      0,
      hglobInstance,
      this);
   if (hWnd_ == nullptr) {
      PrintWindowsError(_T("CreateWindow"));
      return false;
   }
   ShowWindow(hWnd_, SW_HIDE);
   return true;
}

bool ScreensaverNotifier::InitWindowMessage()
{
   hookWndMsg_ = RegisterWindowMessage(_T("WinMuteScreenSaveNotifyMsg"));
   if (hookWndMsg_ == 0) {
      PrintWindowsError(_T("RegisterWindowMessage"));
      return false;
   }
   return true;
}

bool ScreensaverNotifier::InitHook32()
{
   bool success = false;
#ifdef _UNICODE
   wchar_t commandLine[MAX_PATH + 1];
   swprintf_s(commandLine, L"/msgId:%d", hookWndMsg_);
#else
   char commandLine[MAX_PATH + 1];
   sprintf_s(commandLine, "/msgId:%d", hookWndMsg_));
#endif

   JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo;
   ZeroMemory(&jobInfo, sizeof(jobInfo));
   jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

   STARTUPINFO startupInfo;
   PROCESS_INFORMATION procInfo;
   ZeroMemory(&startupInfo, sizeof(startupInfo));
   ZeroMemory(&procInfo, sizeof(procInfo));

   hJob_ = CreateJobObject(NULL, NULL);
   if (hJob_ == nullptr) {
      PrintWindowsError(_T("CreateJobObject"), GetLastError());
   } else if (SetInformationJobObject(
         hJob_,
         JobObjectExtendedLimitInformation,
         &jobInfo,
         sizeof(jobInfo)) == FALSE) {
      PrintWindowsError(_T("SetInformationJobObject"), GetLastError());
      CloseHandle(hJob_);
   } else if (CreateProcess(
         _T(".\\ScreensaverProxy32.exe"),
         commandLine,
         NULL,
         NULL,
         FALSE,
         NORMAL_PRIORITY_CLASS,
         NULL,
         NULL,
         &startupInfo,
         &procInfo) == FALSE) {
      PrintWindowsError(_T("CreateProcess"));
      CloseHandle(hJob_);
      hJob_ = nullptr;
   } else {
      if (AssignProcessToJobObject(hJob_, procInfo.hProcess) == FALSE) {
         PrintWindowsError(_T("AssignProcessToJobObject"), GetLastError());
         CloseHandle(hJob_);
         hJob_ = nullptr;
      } else {
         success = true;
      }
      CloseHandle(procInfo.hProcess);
      CloseHandle(procInfo.hThread);
   }
   return success;
}

bool ScreensaverNotifier::InitHookDll()
{
   if (hookDll_ == nullptr) {
      hookDll_ = LoadLibrary(_T("ScreensaverNotify"));
      if (hookDll_ == nullptr) {
         PrintWindowsError(_T("LoadLibrary"));
      } else {
         regHook_ = reinterpret_cast<RegisterHook>(
            GetProcAddress(hookDll_, "RegisterScreensaverHook"));
         unregHook_ = reinterpret_cast<UnregisterHook>(
            GetProcAddress(hookDll_, "UnregisterScreensaverHook"));
         if (regHook_ == nullptr || unregHook_ == nullptr) {
            Unload();
         }
      }
   }
   return hookDll_ != nullptr;
}

bool ScreensaverNotifier::RegisterWindowClass()
{
   WNDCLASS wc = { 0 };
   wc.hIcon = LoadIcon(hglobInstance, MAKEINTRESOURCE(IDI_APP));
   wc.hbrBackground = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
   wc.hInstance = hglobInstance;
   wc.lpfnWndProc = ScrsvrWndProc;
   wc.lpszClassName = SCRSVER_NOTIFY_WND_CLASS;

   if (!RegisterClass(&wc)) {
      PrintWindowsError(_T("RegisterClass"));
      return false;
   }
   return true;
}

void ScreensaverNotifier::Unload()
{
   if (hookDll_) {
      if (isRegistered_ && unregHook_) {
         unregHook_();
      }
      FreeLibrary(hookDll_);
      regHook_ = nullptr;
      unregHook_ = nullptr;
      hookDll_ = nullptr;
   }
   if (hJob_ != nullptr) {
      CloseHandle(hJob_);
      hJob_ = nullptr;
   }
}

/* ========================================================================== */
/*    Notifications                                                           */
/* ========================================================================== */

bool ScreensaverNotifier::ActivateNotifications(HWND hNotifyWnd)
{
   hNotifyWnd_ = hNotifyWnd;
   if (regHook_ != nullptr) {
      if (regHook_(hWnd_, hookWndMsg_)) {
         isRegistered_ = true;
         return true;
      }
   }
   return false;
}

void ScreensaverNotifier::ClearNotifications()
{
   hNotifyWnd_ = nullptr;
   if (isRegistered_ && unregHook_ != nullptr) {
      unregHook_();
   }
}

void ScreensaverNotifier::StartScreensaverPollTimer(bool start)
{
   if (start) {
      if (SetTimer(hWnd_, SCRSV_TIMER_ID, 1000, nullptr) == 0) {
         PrintWindowsError(_T("SetTimer"));
      }
   } else {
      KillTimer(hWnd_, SCRSV_TIMER_ID);
   }
}

/* ========================================================================== */
/*    Other                                                                   */
/* ========================================================================== */

bool ScreensaverNotifier::IsScreensaverRunning()
{
   BOOL isRunning = FALSE;
   if (SystemParametersInfo(
         SPI_GETSCREENSAVERRUNNING,
         0,
         &isRunning,
         FALSE) == FALSE) {
      PrintWindowsError(_T("SystemParametersInfo"));
      return false;
   }
   return isRunning != 0;
}

/* ========================================================================== */
/*    Window Proc                                                             */
/* ========================================================================== */

LRESULT CALLBACK ScreensaverNotifier::WindowProc(
   HWND hWnd,
   UINT msg,
   WPARAM wParam,
   LPARAM lParam)
{
   static UINT uTaskbarRestart = 0;
   switch (msg) {
   case WM_CREATE: {
      return TRUE;
   }
   case WM_TIMER: {
      if (wParam == SCRSV_TIMER_ID) {
         if (!IsScreensaverRunning()) {
            StartScreensaverPollTimer(false);
            SendMessage(hNotifyWnd_, WM_SCRNSAVE_CHANGE, SCRNSAVE_STOP, 0);
            alreadyNotified_ = false;
         }
      }
      return 0;
   }
   default:
      if (hookWndMsg_ != 0 && msg == hookWndMsg_) {
         /* Windows sends SC_SCREENSAVE when it tries to start a screensaver.
            If screensaver is set to "none" or if another application cancels
            this event, we would falsely mute windows.
            To prevent that, we wait for a few milliseconds to and then check
            if the screensaver has actually started. If yes, then send the
            notification.
         */
         Sleep(500);
         if (IsScreensaverRunning()) {
            /* SC_SCREENSAVE might be fired multiple times */
            if (!alreadyNotified_) {
               alreadyNotified_ = true;
               SendMessage(hNotifyWnd_, WM_SCRNSAVE_CHANGE, SCRNSAVE_START, 0);
               StartScreensaverPollTimer();
            }
         }
      }
      break;
   }
   return DefWindowProc(hWnd, msg, wParam, lParam);
}
