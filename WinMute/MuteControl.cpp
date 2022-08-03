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

const int BLUETOOTH_RECONNECT_UNMUTE_DELAY = 5000; // Milliseconds

enum MuteType {
   // With restore
   MuteTypeWorkstationLock = 0,
   MuteTypeScreensaverActive,
   MuteTypeRemoteSession,
   MuteTypeDisplayStandby,
   MuteTypeBluetoothDisconnect,

   // Without restore
   MuteTypeLogout,
   MuteTypeSuspend,
   MuteTypeShutdown,
   MuteTypeCount // Meta
};

MuteControl::MuteControl() :
   restoreVolume_(false),
   notificationsEnabled_(false),
   displayWasOffOnce_(false),
   trayIcon_(nullptr)
{
   MuteConfig initMuteConf;
   initMuteConf.active = false;
   initMuteConf.shouldMute = false;
   for (int i = 0; i < MuteTypeCount; ++i) {
      muteConfig_.push_back(initMuteConf);
   }
}

MuteControl::~MuteControl()
{
}

bool MuteControl::Init(HWND hParent, const TrayIcon *trayIcon)
{
   winAudio_ = std::make_unique<VistaAudio>();
   if (!winAudio_->Init(hParent)) {
      return false;
   }
   trayIcon_ = trayIcon;
   return true;
}

void MuteControl::SetMute(bool mute)
{
   WMLog::GetInstance().Write(L"Manual muting: %s", mute ? L"on" : L"off");
   winAudio_->SetMute(mute);
}

void MuteControl::SaveMuteStatus()
{
   bool alreadySaved = false;
   for (const auto& conf : muteConfig_) {
      if (conf.shouldMute && conf.active) {
         WMLog::GetInstance().Write(L"Muting event already active. Skipping status save");
         alreadySaved = true;
         break;
      }
   }
   if (!alreadySaved) {
      WMLog::GetInstance().Write(L"Saving mute status");
      winAudio_->SaveMuteStatus();
   }
}

void MuteControl::ShowNotification(const std::wstring& title, const std::wstring& text)
{
   if (notificationsEnabled_ && trayIcon_ != nullptr) {
      trayIcon_->ShowPopup(title, text);
   }
}

void MuteControl::SetNotifications(bool enable)
{
   notificationsEnabled_ = enable;
}

void MuteControl::RestoreVolume(bool withDelay)
{
   WMLog& log = WMLog::GetInstance();
   if (!restoreVolume_) {
      log.Write(L"Volume Restore has been disabled");
      return;
   }
   bool restore = true;
   for (const auto& conf : muteConfig_) {
      if (conf.shouldMute && conf.active) {
         log.Write(L"Skipping restore since other mute event is currently active");
         restore = false;
         break;
      }
   }
   if (restore) {
      log.Write(L"Restoring previous mute state");
      ShowNotification(
         L"Volume restored",
         L"All endpoints have been restored to their previous configuration");
      if (withDelay) {
         Sleep(BLUETOOTH_RECONNECT_UNMUTE_DELAY);
      }
      winAudio_->RestoreMuteStatus();
   }
}

void MuteControl::SetRestoreVolume(bool enable)
{
   restoreVolume_ = enable;
}

void MuteControl::SetMuteOnWorkstationLock(bool enable)
{
   muteConfig_[MuteTypeWorkstationLock].shouldMute = enable;
}

void MuteControl::SetMuteOnScreensaverActivation(bool enable)
{
   muteConfig_[MuteTypeScreensaverActive].shouldMute = enable;
}

void MuteControl::SetMuteOnLogout(bool enable)
{
   muteConfig_[MuteTypeLogout].shouldMute = enable;
}

void MuteControl::SetMuteOnSuspend(bool enable)
{
   muteConfig_[MuteTypeSuspend].shouldMute = enable;
}

void MuteControl::SetMuteOnShutdown(bool enable)
{
   muteConfig_[MuteTypeShutdown].shouldMute = enable;
}

void MuteControl::SetMuteOnRemoteSession(bool enable)
{
   muteConfig_[MuteTypeRemoteSession].shouldMute = enable;
}

void MuteControl::SetMuteOnDisplayStandby(bool enable)
{
   muteConfig_[MuteTypeDisplayStandby].shouldMute = enable;
}

void MuteControl::SetMuteOnBluetoothDisconnect(bool enable)
{
   muteConfig_[MuteTypeBluetoothDisconnect].shouldMute = enable;
}

bool MuteControl::GetRestoreVolume()
{
   return restoreVolume_;
}

bool MuteControl::GetMuteOnWorkstationLock() const
{
   return muteConfig_[MuteTypeWorkstationLock].shouldMute;
}

bool MuteControl::GetMuteOnScreensaverActivation() const
{
   return muteConfig_[MuteTypeScreensaverActive].shouldMute;
}

bool MuteControl::GetMuteOnRemoteSession() const
{
   return muteConfig_[MuteTypeRemoteSession].shouldMute;
}

bool MuteControl::GetMuteOnDisplayStandby() const
{
   return muteConfig_[MuteTypeDisplayStandby].shouldMute;
}

bool MuteControl::GetMuteOnBluetoothDisconnect() const
{
   return muteConfig_[MuteTypeBluetoothDisconnect].shouldMute;
}

bool MuteControl::GetMuteOnLogout() const
{
   return muteConfig_[MuteTypeLogout].shouldMute;
}

bool MuteControl::GetMuteOnSuspend() const
{
   return muteConfig_[MuteTypeSuspend].shouldMute;
}

bool MuteControl::GetMuteOnShutdown() const
{
   return muteConfig_[MuteTypeShutdown].shouldMute;
}

void MuteControl::NotifyRestoreCondition(int type, bool active, bool withDelay)
{
   if (active) {
      SaveMuteStatus();
      muteConfig_[type].active = active;
      if (muteConfig_[type].shouldMute) {
         WMLog::GetInstance().Write(L"Muting workstation");
         ShowNotification(
            L"Muting workstation",
            L"All endpoints have been muted");
         winAudio_->SetMute(true);
      }
   } else {
      if (muteConfig_[type].active) {
         muteConfig_[type].active = false;
         if (muteConfig_[type].shouldMute) {
            RestoreVolume(withDelay);
         }
      }
   }
}

void MuteControl::NotifyWorkstationLock(bool active)
{
   WMLog::GetInstance().Write(L"Mute Event: Workstation Lock %s",
                              active ? L"start" : L"stop");
   NotifyRestoreCondition(MuteTypeWorkstationLock, active);
}

void MuteControl::NotifyScreensaver(bool active)
{
   WMLog::GetInstance().Write(L"Mute Event: Screensaver %s",
                              active ? L"start" : L"stop");
   NotifyRestoreCondition(MuteTypeScreensaverActive, active);
}

void MuteControl::NotifyRemoteSession(bool active)
{
   WMLog::GetInstance().Write(L"Mute Event: Remote Session %s",
                              active ? L"start" : L"stop");
   NotifyRestoreCondition(MuteTypeRemoteSession, active);
}

void MuteControl::NotifyDisplayStandby(bool active)
{
   if (displayWasOffOnce_ || active) {
      WMLog::GetInstance().Write(L"Mute Event: Display Standby %s",
                                 active ? L"start" : L"stop");
      NotifyRestoreCondition(MuteTypeDisplayStandby, active);
      displayWasOffOnce_ = true;
   }
}

void MuteControl::NotifyBluetoothConnected(bool connected)
{
   WMLog::GetInstance().Write(
      L"Mute Event: Bluetooth audio device %s",
      connected ? L"connected" : L"disconnected");
   NotifyRestoreCondition(MuteTypeBluetoothDisconnect, !connected, true);
}

void MuteControl::NotifyLogout()
{
   WMLog::GetInstance().Write(L"Mute Event: Logout start");
   if (muteConfig_[MuteTypeLogout].shouldMute) {
      winAudio_->SetMute(true);
   }
}

void MuteControl::NotifySuspend(bool /*active*/)
{
   WMLog::GetInstance().Write(L"Mute Event: Suspend start");
   if (muteConfig_[MuteTypeSuspend].shouldMute) {
      winAudio_->SetMute(true);
   }
}

void MuteControl::NotifyShutdown()
{
   WMLog::GetInstance().Write(L"Mute Event: Shutdown start");
   if (muteConfig_[MuteTypeShutdown].shouldMute) {
      winAudio_->SetMute(true);
   }
}

void MuteControl::NotifyQuietHours(bool active)
{
   if (active) {
      SaveMuteStatus();
      WMLog::GetInstance().Write(L"Mute Event: Quiet Hours startet");
      winAudio_->SaveMuteStatus();
      winAudio_->SetMute(true);
   } else {
      WMLog::GetInstance().Write(L"Mute Event: Quiet Hours ended");
      RestoreVolume();
   }
}
