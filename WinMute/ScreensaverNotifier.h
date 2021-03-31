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

#pragma once

#include "StdAfx.h"

#define WM_SCRNSAVE_CHANGE (WM_APP + 100)

#define SCRNSAVE_START 1
#define SCRNSAVE_STOP  2

class ScreensaverNotifier {
public:
   ScreensaverNotifier();
   ~ScreensaverNotifier();

   ScreensaverNotifier(const ScreensaverNotifier&) = delete;
   ScreensaverNotifier& operator= (const ScreensaverNotifier&) = delete;

   bool Init();
   void Unload();

   bool ActivateNotifications(HWND hNotifyWnd);
   void ClearNotifications();


   LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
private:
   typedef int(*RegisterHook)(HWND, UINT);
   typedef void(*UnregisterHook)(void);

   bool alreadyNotified_;
   bool isRegistered_;

   HINSTANCE hookDll_;
   HWND hWnd_, hNotifyWnd_;
   HANDLE hJob_;
   UINT hookWndMsg_;

   RegisterHook regHook_;
   UnregisterHook unregHook_;

   void StartScreensaverPollTimer(bool start = true);

   bool IsScreensaverRunning();

   bool InitHook32();
   bool InitHookDll();
   bool RegisterWindowClass();
   bool InitWindow();
   bool InitWindowMessage();
};
