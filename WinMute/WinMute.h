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

#pragma once

#include "common.h"

class WinAudio;

class WinMute {
public:
   explicit WinMute(WMSettings& settings);
   WinMute(const WinMute&) = delete;
   WinMute(WinMute&&) = delete;
   WinMute& operator=(const WinMute &) = delete;
   WinMute& operator=(WinMute&&) = delete;
   ~WinMute() noexcept;

   bool Init();
   void Close();

   // for internal use
   LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
   void CheckForUpdatesAsync(std::unique_ptr<UpdateChecker> updateChecker);
private:
   HWND hWnd_;
   HMENU hTrayMenu_;
   HICON hAppIcon_;
   HICON hTrayIcon_;
   HICON hUpdateIcon_;

   struct MuteConfig {
      MuteConfig();
      bool showNotifications;
      bool muteOnWlan;
      bool muteOnBluetooth;
   } muteConfig_;

   TrayIcon wmTray_;
   TrayIcon updateTray_;
   WifiDetector wifiDetector_;
   WMSettings& settings_;
   WMi18n &i18n_;
   MuteControl muteCtrl_;
   QuietHoursTimer quietHours_;
   BluetoothDetector btDetector_;
   UpdateInfo updateInfo_;

   void CheckForUpdates();
   bool RegisterWindowClass();
   bool InitWindow();
   bool InitAudio();
   bool InitTrayMenu();
   bool LoadSettings();

   void Unload() noexcept;

   void ToggleMenuCheck(UINT item, bool* setting) noexcept;

   void LoadMainMenuText();

   // Windows Callback
   LRESULT OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
   LRESULT OnTrayIcon(HWND hWnd, WPARAM wParam, LPARAM lParam);
   LRESULT OnSettingChange(HWND hWnd, WPARAM wParam, LPARAM lParam);
   LRESULT OnPowerBroadcast(HWND hWnd, WPARAM wParam, LPARAM lParam);
   LRESULT OnWifiStatusChange(HWND hWnd, WPARAM wParam, LPARAM lParam);
   LRESULT OnUpdatePopup(HWND hWnd, WPARAM wParam, LPARAM lParam);
   
   LRESULT OnDeviceChange(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
   LRESULT OnQuietHours(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
