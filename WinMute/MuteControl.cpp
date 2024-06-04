/*
 WinMute
           Copyright (c) 2024, Alexander Steinhoefer

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

extern HINSTANCE hglobInstance;

static const int BLUETOOTH_RECONNECT_UNMUTE_DELAY = 5000; // Milliseconds
static const int MUTE_DELAY_MAGIC_VALUE = 0x198604;

static const wchar_t* MUTECONTROL_CLASS_NAME = L"WinMuteMuteControl";

enum MuteType {
   // With restore
   MuteTypeWorkstationLock = 0,
   MuteTypeRemoteSession,
   MuteTypeDisplayStandby,
   MuteTypeBluetoothDisconnect,

   // Without restore
   MuteTypeLogout,
   MuteTypeSuspend,
   MuteTypeShutdown,

   // Quiet Hours
   MuteTypeQuietHours,

   MuteTypeCount // Meta
};

void DelayedMuteTimerProc(HWND hWnd, UINT, UINT_PTR, DWORD)
{
   MuteControl *muteCtrl = reinterpret_cast<MuteControl*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
   if (muteCtrl != nullptr) {
      muteCtrl->MuteDelayed(MUTE_DELAY_MAGIC_VALUE);
   }
}

static LRESULT CALLBACK MuteControlWndProc(
   HWND hWnd,
   UINT msg,
   WPARAM wParam,
   LPARAM lParam)
{
   auto wm = reinterpret_cast<MuteControl*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
   switch (msg) {
   case WM_NCCREATE:
   {
      LPCREATESTRUCTW cs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
      SetWindowLongPtrW(hWnd, GWLP_USERDATA,
                        reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
      return TRUE;
   }
   default:
      break;
   }
   return (wm)
      ? wm->WindowProc(hWnd, msg, wParam, lParam)
      : DefWindowProcW(hWnd, msg, wParam, lParam);
}

MuteControl::MuteControl()
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
   UnregisterClassW(MUTECONTROL_CLASS_NAME, hglobInstance);
   DestroyWindow(hMuteCtrlWnd_);
}

bool MuteControl::Init(HWND hParent, const TrayIcon *trayIcon)
{
   WNDCLASSEXW wndClass{ 0 };
   wndClass.cbSize = sizeof(wndClass);
   wndClass.lpfnWndProc = MuteControlWndProc;
   wndClass.hInstance = hglobInstance;
   wndClass.lpszClassName = MUTECONTROL_CLASS_NAME;
   if (!RegisterClassExW(&wndClass)) {
      WMLog::GetInstance().LogWinError(L"RegisterClassEx");
      return false;
   }
   hMuteCtrlWnd_ = CreateWindowEx(
      WS_EX_TOOLWINDOW,
      MUTECONTROL_CLASS_NAME,
      L"",
      0,
      0, 0, 0, 0,
      nullptr,
      nullptr,
      hglobInstance,
      this);
   if (hMuteCtrlWnd_ == nullptr) {
      WMLog::GetInstance().LogWinError(L"CreateWindowEx");
      UnregisterClassW(MUTECONTROL_CLASS_NAME, hglobInstance);
      return false;
   }
   winAudio_ = std::make_unique<VistaAudio>();
   if (!winAudio_->Init(hParent)) {
      DestroyWindow(hMuteCtrlWnd_);
      UnregisterClassW(MUTECONTROL_CLASS_NAME, hglobInstance);
      return false;
   }

   trayIcon_ = trayIcon;
   return true;
}

LRESULT CALLBACK MuteControl::WindowProc(
   HWND hWnd,
   UINT msg,
   WPARAM wParam,
   LPARAM lParam)
{
   return DefWindowProc(hWnd, msg, wParam, lParam);
}

void MuteControl::SetMute(bool mute)
{
   WMLog::GetInstance().LogInfo(L"Manual muting: %s", mute ? L"on" : L"off");
   winAudio_->SetMute(mute);
}

void MuteControl::SaveMuteStatus()
{
   const bool alreadySaved = std::any_of(
      muteConfig_.begin(),
      muteConfig_.end(),
      [](const MuteConfig &conf) {
         return conf.shouldMute && conf.active;
      }
   );

   if (alreadySaved) {
      WMLog::GetInstance().LogInfo(L"Muting event already active. Skipping status save");
   } else {
      WMLog::GetInstance().LogInfo(L"Saving mute status");
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

void MuteControl::MuteDelayed(int magic)
{
   if (magic != MUTE_DELAY_MAGIC_VALUE || delayedMuteTimerId_ == 0) {
      return;
   }
   WMLog::GetInstance().LogInfo(L"Muting workstation after delay");
   ShowNotification(
      WMi18n::GetInstance().GetTranslationW("popup.muting-workstation-after-delay.title"),
      WMi18n::GetInstance().GetTranslationW("popup.muting-workstation-after-delay.text"));
   winAudio_->SetMute(true);
   KillTimer(hMuteCtrlWnd_, delayedMuteTimerId_);
   delayedMuteTimerId_ = 0;
}

bool MuteControl::StartDelayedMute()
{
   delayedMuteTimerId_ = SetTimer(
      hMuteCtrlWnd_,
      delayedMuteTimerId_,
      muteDelaySeconds_ * 1000,
      DelayedMuteTimerProc);
   if (delayedMuteTimerId_ == 0) {
      WMLog::GetInstance().LogWinError(L"SetTimer", GetLastError());
   }
   return delayedMuteTimerId_ != 0;
}

void MuteControl::RestoreVolume(bool withDelay)
{
   WMLog& log = WMLog::GetInstance();
   if (!restoreVolume_) {
      log.LogInfo(L"Volume Restore has been disabled");
      return;
   }
   const bool restore = !std::any_of(
      muteConfig_.begin(),
      muteConfig_.end(),
      [](const MuteConfig &conf) {
         return conf.shouldMute && conf.active;
      }
   );
   if (!restore) {
      log.LogInfo(L"Skipping restore since other mute event is currently active");
   } else if (delayedMuteTimerId_ != 0) {
      KillTimer(hMuteCtrlWnd_, delayedMuteTimerId_);
      delayedMuteTimerId_ = 0;
      log.LogInfo(L"Skipping restore, since delayed mute was not triggered yet");
   } else {
      log.LogInfo(L"Restoring previous mute state");
      ShowNotification(
         WMi18n::GetInstance().GetTranslationW("popup.volume-restored.title"),
         WMi18n::GetInstance().GetTranslationW("popup.volume-restored.text"));
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

void MuteControl::SetMuteDelay(int delaySeconds)
{
   muteDelaySeconds_ = delaySeconds;
}

void MuteControl::SetMuteOnWorkstationLock(bool enable)
{
   muteConfig_[MuteTypeWorkstationLock].shouldMute = enable;
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
         if (muteDelaySeconds_ == 0) {
            WMLog::GetInstance().LogInfo(L"Muting workstation");
            ShowNotification(
               WMi18n::GetInstance().GetTranslationW("popup.muting-workstation.title"),
               WMi18n::GetInstance().GetTranslationW("popup.muting-workstation.text"));
            winAudio_->SetMute(true);
         } else {
            WMLog::GetInstance().LogInfo(L"Starting delayed mute timer...");
            StartDelayedMute();
         }
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
   WMLog::GetInstance().LogInfo(L"Mute Event: Workstation Lock %s",
                              active ? L"start" : L"stop");
   NotifyRestoreCondition(MuteTypeWorkstationLock, active);
}

void MuteControl::NotifyRemoteSession(bool active)
{
   WMLog::GetInstance().LogInfo(L"Mute Event: Remote Session %s",
                              active ? L"start" : L"stop");
   NotifyRestoreCondition(MuteTypeRemoteSession, active);
}

void MuteControl::NotifyDisplayStandby(bool active)
{
   if (displayWasOffOnce_ || active) {
      WMLog::GetInstance().LogInfo(L"Mute Event: Display Standby %s",
                                 active ? L"start" : L"stop");
      NotifyRestoreCondition(MuteTypeDisplayStandby, active);
      displayWasOffOnce_ = true;
   }
}

void MuteControl::NotifyBluetoothConnected(bool connected)
{
   WMLog::GetInstance().LogInfo(
      L"Mute Event: Bluetooth audio device %s",
      connected ? L"connected" : L"disconnected");
   NotifyRestoreCondition(MuteTypeBluetoothDisconnect, !connected, true);
}

void MuteControl::NotifyLogout()
{
   WMLog::GetInstance().LogInfo(L"Mute Event: Logout start");
   if (muteConfig_[MuteTypeLogout].shouldMute) {
      winAudio_->SetMute(true);
   }
}

void MuteControl::NotifySuspend(bool /*active*/)
{
   WMLog::GetInstance().LogInfo(L"Mute Event: Suspend start");
   if (muteConfig_[MuteTypeSuspend].shouldMute) {
      winAudio_->SetMute(true);
   }
}

void MuteControl::NotifyShutdown()
{
   WMLog::GetInstance().LogInfo(L"Mute Event: Shutdown start");
   if (muteConfig_[MuteTypeShutdown].shouldMute) {
      winAudio_->SetMute(true);
   }
}

void MuteControl::NotifyQuietHours(bool active)
{
   if (active) {
      SaveMuteStatus();
      WMLog::GetInstance().LogInfo(L"Mute Event: Quiet Hours startet");
      muteConfig_[MuteTypeQuietHours].active = true;
      winAudio_->SaveMuteStatus();
      winAudio_->SetMute(true);
   } else {
      WMLog::GetInstance().LogInfo(L"Mute Event: Quiet Hours ended");
      muteConfig_[MuteTypeQuietHours].active = false;
      RestoreVolume();
   }
}

void MuteControl::SetManagedEndpoints(
   const std::vector<std::wstring>& endpoints,
   bool isAllowList)
{
   winAudio_->MuteSpecificEndpoints(true);
   winAudio_->SetManagedEndpoints(endpoints, isAllowList);
}

void MuteControl::ClearManagedEndpoints()
{
   winAudio_->MuteSpecificEndpoints(false);
}
