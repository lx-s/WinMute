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

   void SetMuteOnWorkstationLock(bool enable);
   void SetMuteOnScreensaverActivation(bool enable);
   void SetMuteOnRemoteSession(bool enable);
   void SetMuteOnDisplayStandby(bool enable);
   
   void SetMuteOnLogout(bool enable);
   void SetMuteOnSuspend(bool enable);
   void SetMuteOnShutdown(bool enable);

   bool GetRestoreVolume();

   bool GetMuteOnWorkstationLock();
   bool GetMuteOnScreensaverActivation();
   bool GetMuteOnRemoteSession();
   bool GetMuteOnDisplayStandby();

   bool GetMuteOnLogout();
   bool GetMuteOnSuspend();
   bool GetMuteOnShutdown();

   void NotifyScreensaver(bool active);
   void NotifyWorkstationLock(bool active);
   void NotifyRemoteSession(bool active);
   void NotifyDisplayStandby(bool enable);

   void NotifyLogout();
   void NotifySuspend(bool active);
   void NotifyShutdown();

   void NotifyQuietHours(bool active);
private:
   struct MuteConfig {
      bool shouldMute;
      bool active;
   };
   std::vector<MuteConfig> muteConfig_;
   bool restoreVolume_;
   bool notificationsEnabled_;
   std::unique_ptr<WinAudio> winAudio_;

   bool displayWasOffOnce_; // Since a power message is sent right after registering
                            // We skip the first "was on" message

   const TrayIcon* trayIcon_;

   void NotifyRestoreCondition(int type, bool active);
   void SaveMuteStatus();
   void RestoreVolume();
   void ShowNotification(const tstring& title, const tstring& text);
};
