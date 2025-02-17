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

#pragma once

#include "common.h"

class MuteControl {
public:
   MuteControl();
   ~MuteControl();
   MuteControl(const MuteControl&) = delete;
   MuteControl& operator=(const MuteControl&) = delete;

   bool Init(HWND hParent, const TrayIcon* trayIcon);

   void SetNotifications(bool enable);

   void SetMute(bool mute);

   void SetRestoreVolume(bool enable);

   void SetMuteDelay(int delaySeconds);

   void SetMuteOnWorkstationLock(bool enable);
   void SetMuteOnRemoteSession(bool enable);
   void SetMuteOnDisplayStandby(bool enable);
   void SetMuteOnBluetoothDisconnect(bool enable);
   
   void SetMuteOnLogout(bool enable);
   void SetMuteOnSuspend(bool enable);
   void SetMuteOnShutdown(bool enable);

   bool GetRestoreVolume();

   bool GetMuteOnWorkstationLock()  const;
   bool GetMuteOnRemoteSession()  const;
   bool GetMuteOnDisplayStandby()  const;
   bool GetMuteOnBluetoothDisconnect() const;

   bool GetMuteOnLogout() const;
   bool GetMuteOnSuspend() const;
   bool GetMuteOnShutdown() const;

   void NotifyWorkstationLock(bool active);
   void NotifyRemoteSession(bool active);
   void NotifyDisplayStandby(bool active);
   void NotifyBluetoothConnected(bool connected);

   void NotifyLogout();
   void NotifySuspend(bool active);
   void NotifyShutdown();

   void NotifyQuietHours(bool active);

   void SetManagedEndpoints(
      const std::vector<std::wstring>& endpoints,
      bool isAllowList);
   void ClearManagedEndpoints();

   LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

   // Should only be called internally
   void MuteDelayed(int magic);
private:
   struct MuteConfig {
      bool shouldMute;
      bool active;
   };
   std::vector<MuteConfig> muteConfig_;
   bool restoreVolume_ = false;
   bool notificationsEnabled_ = false;
   int muteDelaySeconds_ = 0;
   UINT_PTR delayedMuteTimerId_ = 0;
   std::unique_ptr<WinAudio> winAudio_;
   HWND hMuteCtrlWnd_ = nullptr;

   // Since a power message is sent right after registering
   // We skip the first "was on" message
   bool displayWasOffOnce_ = false;

   const TrayIcon* trayIcon_ = nullptr;

   void NotifyRestoreCondition(int type, bool active, bool withDelay = false);
   void SaveMuteStatus();
   void RestoreVolume(bool withDelay = false);
   void ShowNotification(const std::wstring& title, const std::wstring& text);
   bool StartDelayedMute();
};
