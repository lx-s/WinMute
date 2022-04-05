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
   wasAlreadyMuted_(false)
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

bool MuteControl::Init(HWND hParent)
{
   winAudio_ = std::make_unique<VistaAudio>();
   if (!winAudio_->Init(hParent)) {
      return false;
   }
   return true;
}

void MuteControl::SetMute(bool mute)
{
   WMLog::GetInstance().Write(_T("Manual muting: {}"), mute ? _T("on") : _T("off"));
   winAudio_->SetMute(mute);
}

void MuteControl::ConfigureWasAlreadyMuted()
{
   bool muteAlreadyActive = false;
   for (const auto& conf : muteConfig_) {
      if (conf.shouldMute && conf.active) {
         muteAlreadyActive = true;
         break;
      }
   }
   if (!muteAlreadyActive && winAudio_->IsMuted()) {
      wasAlreadyMuted_ = true;
   }
}

void MuteControl::RestoreVolume()
{
   if (!restoreVolume_) {
      WMLog::GetInstance().Write(_T("Restore is FALSE"));
      return;
   }
   bool restore = true;
   int i = 0;
   for (const auto& conf : muteConfig_) {
      if (conf.shouldMute && conf.active) {
         WMLog::GetInstance().Write(_T("Entry {} found with ShouldMute: Yes and Active: Yes"), i);
         restore = false;
         //break;
      }
      ++i;
   }
   if (restore) {
      if (winAudio_->IsMuted() && !wasAlreadyMuted_) {
         WMLog::GetInstance().Write(_T("Mute: Off"));
         winAudio_->SetMute(false);
      } else {
         WMLog::GetInstance().Write(
            _T("Skipping unmute since workstation was already muted."));
      }
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
      ConfigureWasAlreadyMuted();
      muteConfig_[type].active = active;
      WMLog::GetInstance().Write(_T("Condition {}: ACTIVE"), type);
      if (muteConfig_[type].shouldMute &&
         !winAudio_->IsMuted()) {
         WMLog::GetInstance().Write(_T("Mute: On"));
         winAudio_->SetMute(true);
      }
   } else {
      WMLog::GetInstance().Write(_T("Condition {}: INACTIVE"), type);
      muteConfig_[type].active = active;
      RestoreVolume();
   }
}

void MuteControl::NotifyWorkstationLock(bool active)
{
   NotifyRestoreCondition(MuteTypeWorkstationLock, active);
}

void MuteControl::NotifyScreensaver(bool active)
{
   NotifyRestoreCondition(MuteTypeScreensaverActive, active);
}

void MuteControl::NotifyRemoteSession(bool active)
{
   NotifyRestoreCondition(MuteTypeRemoteSession, active);
}

void MuteControl::NotifyDisplayStandby(bool active)
{
   NotifyRestoreCondition(MuteTypeDisplayStandby, active);
}

void MuteControl::NotifyLogout()
{
   if (muteConfig_[MuteTypeLogout].shouldMute) {
      winAudio_->SetMute(true);
   }
}

void MuteControl::NotifySuspend(bool /*active*/)
{
   WMLog::GetInstance().Write(_T("Suspend: START"));
   if (muteConfig_[MuteTypeSuspend].shouldMute) {
      winAudio_->SetMute(true);
   }
}

void MuteControl::NotifyShutdown()
{
   if (muteConfig_[MuteTypeShutdown].shouldMute) {
      winAudio_->SetMute(true);
   }
}

void MuteControl::NotifyQuietHours(bool active)
{
   if (active) {
      ConfigureWasAlreadyMuted();
      WMLog::GetInstance().Write(L"Quiet Hours startet");
      if (!winAudio_->IsMuted()) {
         winAudio_->SetMute(true);
      }
   } else {
      WMLog::GetInstance().Write(L"Quiet Hours ended");
      RestoreVolume();
   }
}
