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

enum MuteType {
   // With restore
   MuteTypeWorkstationLock = 0,
   MuteTypeScreensaverActive,
   MuteTypeRemoteSession,
   MuteTypeDisplayStandby,

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
   WMLog::GetInstance().Write(_T("Manual muting: %s"), mute ? _T("on") : _T("off"));
   winAudio_->SetMute(mute);
}

void MuteControl::SaveMuteStatus()
{
   bool alreadySaved = false;
   for (const auto& conf : muteConfig_) {
      if (conf.shouldMute && conf.active) {
         WMLog::GetInstance().Write(_T("Muting event already active. Skipping status save"));
         alreadySaved = true;
         break;
      }
   }
   if (!alreadySaved) {
      WMLog::GetInstance().Write(_T("Saving mute status"));
      winAudio_->SaveMuteStatus();
   }
}

void MuteControl::ShowNotification(const tstring& title, const tstring& text)
{
   if (notificationsEnabled_ && trayIcon_ != nullptr) {
      trayIcon_->ShowPopup(title, text);
   }
}

void MuteControl::SetNotifications(bool enable)
{
   notificationsEnabled_ = enable;
}

void MuteControl::RestoreVolume()
{
   WMLog& log = WMLog::GetInstance();
   if (!restoreVolume_) {
      log.Write(_T("Volume Restore has been disabled"));
      return;
   }
   bool restore = true;
   for (const auto& conf : muteConfig_) {
      if (conf.shouldMute && conf.active) {
         log.Write(_T("Skipping restore since other mute event is currently active"));
         restore = false;
         break;
      }
   }
   if (restore) {
      log.Write(_T("Restoring previous mute state"));
      ShowNotification(
         _T("Volume restored"),
         _T("All endpoints have been restored to their previous configuration"));
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

bool MuteControl::GetRestoreVolume()
{
   return restoreVolume_;
}

bool MuteControl::GetMuteOnWorkstationLock()
{
   return muteConfig_[MuteTypeWorkstationLock].shouldMute;
}

bool MuteControl::GetMuteOnScreensaverActivation()
{
   return muteConfig_[MuteTypeScreensaverActive].shouldMute;
}

bool MuteControl::GetMuteOnRemoteSession()
{
   return muteConfig_[MuteTypeRemoteSession].shouldMute;
}

bool MuteControl::GetMuteOnDisplayStandby()
{
   return muteConfig_[MuteTypeDisplayStandby].shouldMute;
}

bool MuteControl::GetMuteOnLogout()
{
   return muteConfig_[MuteTypeLogout].shouldMute;
}

bool MuteControl::GetMuteOnSuspend()
{
   return muteConfig_[MuteTypeSuspend].shouldMute;
}

bool MuteControl::GetMuteOnShutdown()
{
   return muteConfig_[MuteTypeShutdown].shouldMute;
}

void MuteControl::NotifyRestoreCondition(int type, bool active)
{
   if (active) {
      SaveMuteStatus();
      muteConfig_[type].active = active;
      if (muteConfig_[type].shouldMute) {
         WMLog::GetInstance().Write(_T("Muting workstation"));
         ShowNotification(
            _T("Muting workstation"),
            _T("All endpoints have been muted"));
         winAudio_->SetMute(true);
      }
   } else {
      if (muteConfig_[type].active) {
         muteConfig_[type].active = false;
         if (muteConfig_[type].shouldMute) {
            RestoreVolume();
         }
      }
   }
}

void MuteControl::NotifyWorkstationLock(bool active)
{
   WMLog::GetInstance().Write(_T("Mute Event: Workstation Lock %s"),
                              active ? _T("start") : _T("stop"));
   NotifyRestoreCondition(MuteTypeWorkstationLock, active);
}

void MuteControl::NotifyScreensaver(bool active)
{
   WMLog::GetInstance().Write(_T("Mute Event: Screensaver %s"),
                              active ? _T("start") : _T("stop"));
   NotifyRestoreCondition(MuteTypeScreensaverActive, active);
}

void MuteControl::NotifyRemoteSession(bool active)
{
   WMLog::GetInstance().Write(_T("Mute Event: Remote Session %s"),
                              active ? _T("start") : _T("stop"));
   NotifyRestoreCondition(MuteTypeRemoteSession, active);
}

void MuteControl::NotifyDisplayStandby(bool active)
{
   if (displayWasOffOnce_ || active) {
      WMLog::GetInstance().Write(_T("Mute Event: Display Standby %s"),
                                 active ? _T("start") : _T("stop"));
      NotifyRestoreCondition(MuteTypeDisplayStandby, active);
      displayWasOffOnce_ = true;
   }
}

void MuteControl::NotifyLogout()
{
   WMLog::GetInstance().Write(_T("Mute Event: Logout start"));
   if (muteConfig_[MuteTypeLogout].shouldMute) {
      winAudio_->SetMute(true);
   }
}

void MuteControl::NotifySuspend(bool /*active*/)
{
   WMLog::GetInstance().Write(_T("Mute Event: Suspend start"));
   if (muteConfig_[MuteTypeSuspend].shouldMute) {
      winAudio_->SetMute(true);
   }
}

void MuteControl::NotifyShutdown()
{
   WMLog::GetInstance().Write(_T("Mute Event: Shutdown start"));
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
