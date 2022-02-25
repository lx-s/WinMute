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

#pragma once

#include "common.h"

class WinAudio;

static const int WM_WINMUTE_QUIETHOURS_CHANGE = WM_APP + 200;
static const int WM_WINMUTE_MUTE = WM_APP + 201;
static const int WM_WINMUTE_QUIETHOURS_START = WM_APP + 202;
static const int WM_WINMUTE_QUIETHOURS_END = WM_APP + 203;
static const int QUIETHOURS_TIMER_START_ID = 271020;
static const int QUIETHOURS_TIMER_END_ID = 271021;

class WinMute {
public:
   WinMute();
   ~WinMute();

   bool Init();
   void Close();

   // for internal use
   LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
private:
   struct MuteConfigItem {
      bool shouldMute;
      bool isActive;
   };

   std::unordered_map<std::string, MuteConfigItem> muteCfg_;

   HWND hWnd_;
   HMENU hTrayMenu_;
   HICON hAppIcon_;
   HICON hTrayIcon_;

   struct MuteConfig {
      MuteConfig();
      struct {
         bool onLock;
         bool onScreensaver;
      } withRestore;
      bool restoreAudio;
      struct {
         bool onLogoff;
         bool onShutdown;
         bool onSuspend;
      } noRestore;
      struct {
         bool enabled;
         bool forceUnmute;
         bool notifications;
         time_t start;
         time_t end;
      } quietHours;
   } muteConfig_;

   bool wasAlreadyMuted_;
   int muteCounter_;

   TrayIcon trayIcon_;
   std::unique_ptr<WinAudio> audio_;
   ScreensaverNotifier scrnSaverNoti_;
   Settings settings_;

   bool RegisterWindowClass();
   bool InitWindow();
   bool InitAudio();
   bool InitTrayMenu();
   bool LoadDefaults();

   void ResetQuietHours();
   void SetQuietHoursStart();
   void SetQuietHoursEnd();

   void Unload();

   void ToggleMenuCheck(UINT item, bool* setting);

   // Disable copying
   WinMute(const WinMute&) = delete;
   WinMute& operator=(const WinMute&) = delete;
};
