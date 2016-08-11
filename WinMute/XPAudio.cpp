/*
 WinMute
           Copyright (c) 2014 Alexander Steinhoefer

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
#include <sstream>
#include "WinAudio.h"

static const TCHAR* MIXER_NOTIFY_WND_CLASS = _T("WinMuteMixerNotifyWnd");
extern HINSTANCE hglobInstance;

LRESULT CALLBACK MixerWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   XPAudio* xpa =
              reinterpret_cast<XPAudio*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
   switch (msg) {
   case WM_NCCREATE: {
      LPCREATESTRUCTW cs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
      SetWindowLongPtr(hWnd, GWLP_USERDATA,
                       reinterpret_cast<LONG>(cs->lpCreateParams));
      return TRUE;
   }
   default:
      break;
   }
   return (xpa) ? xpa->WindowProc(hWnd, msg, wParam, lParam)
                  : DefWindowProc(hWnd, msg, wParam, lParam);
}

static void PrintMixerError(const tstring& funcName, MMRESULT err)
{
#ifdef _DEBUG
   std::basic_string<TCHAR> error = funcName + L" failed with: ";
   switch (err) {
   case MIXERR_INVALCONTROL:
      error += _T("The control reference is invalid.");
      break;
   case MIXERR_INVALLINE:
      error += _T("The audio line reference is invalid.");
      break;
   case MMSYSERR_BADDEVICEID:
      error += _T("The hmxobj parameter specifies an invalid device")
               _T(" identifier.");
      break;
   case MMSYSERR_INVALFLAG:
      error += _T("One or more flags are invalid.");
      break;
   case MMSYSERR_INVALHANDLE:
      error += _T("The hmxobj parameter specifies an invalid handle.");
      break;
   case MMSYSERR_INVALPARAM:
      error += _T("One or more parameters are invalid.");
      break;
   case MMSYSERR_NODRIVER:
      error += _T("No mixer device is available for the object specified")
               _T(" by hmxobj.");
      break;
   default: {
         error += _T("An unknown error: ");
         std::wstringstream s;
         s << err;
         error += s.str();

         break;
      }
   }
   MessageBox(0, error.c_str(), 0, MB_ICONERROR);
#else
   UNREFERENCED_PARAMETER(funcName);
   UNREFERENCED_PARAMETER(err);
#endif
}

XPAudio::XPAudio() :
   hNotifyWnd_(nullptr), currentlyMuted_(false), muteTime_(0)
{
}

XPAudio::~XPAudio()
{
   std::for_each(mixers_.begin(), mixers_.end(), [](XPAudio::MixerDevice& md) {
      mixerClose(md.hMixer);
   });
   mixers_.clear();
   DestroyWindow(hNotifyWnd_);
}

/* -----------------------------------------------------------------------------
    Init
   -------------------------------------------------------------------------- */

bool XPAudio::Init(HWND hParent)
{
   if (!InitNotifyWindow(hParent))
      return false;

   UINT numMixers = mixerGetNumDevs();
   for (UINT i = 0; i < numMixers; ++i) {
      MixerDevice md;
      if (mixerOpen(&md.hMixer, i, reinterpret_cast<DWORD_PTR>(hNotifyWnd_),
                    0, CALLBACK_WINDOW) == MMSYSERR_NOERROR) {
         MIXERCAPS mixcaps;
         mixerGetDevCaps(i, &mixcaps, sizeof(MIXERCAPS));
         if (GetMuteControl(md)) {
            mixers_.push_back(md);
         } else {
            mixerClose(md.hMixer);
         }
      }
   }
   return !mixers_.empty();
}

bool XPAudio::InitNotifyWindow(HWND hParent)
{
   WNDCLASS wc={0};
   wc.hInstance = hglobInstance;
   wc.lpfnWndProc = MixerWndProc;
   wc.lpszClassName = MIXER_NOTIFY_WND_CLASS;

   if (!RegisterClass(&wc)) {
      PrintWindowsError(_T("RegisterClass"));
      return false;
   }
   hNotifyWnd_ = CreateWindow(MIXER_NOTIFY_WND_CLASS, 0, 0, 0, 0, 0, 0,
                              hParent, 0, hglobInstance, this);
   if (hNotifyWnd_ == nullptr) {
      PrintWindowsError(_T("CreateWindow"));
      return false;
   }
   return true;
}

void XPAudio::ShouldReInit()
{
}

bool XPAudio::GetMuteControl(XPAudio::MixerDevice& md)
{
   MIXERLINE mxl = {0};
   mxl.cbStruct = sizeof(MIXERLINE);
   mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

   MMRESULT mmrErr = mixerGetLineInfo(reinterpret_cast<HMIXEROBJ>(md.hMixer),
                                      &mxl, MIXER_GETLINEINFOF_COMPONENTTYPE);
   if (mmrErr != MMSYSERR_NOERROR) {
      return false;
   }

   // At first: Try to optain the Mute-Control.
   MIXERCONTROL mxCtrl = {0};
   MIXERLINECONTROLS mxLineCtrls = {0};
   mxLineCtrls.cbStruct = sizeof(MIXERLINECONTROLS);
   mxLineCtrls.cControls = 1;
   mxLineCtrls.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
   mxLineCtrls.dwLineID = mxl.dwLineID;
   mxLineCtrls.pamxctrl = &mxCtrl;
   mxLineCtrls.cbmxctrl = sizeof(MIXERCONTROL);

   mmrErr = mixerGetLineControls(reinterpret_cast<HMIXEROBJ>(md.hMixer),
                                &mxLineCtrls, MIXER_GETLINECONTROLSF_ONEBYTYPE);

   if (mmrErr != MMSYSERR_NOERROR) {
      // If this doesn't work, get the volume slider
      mxLineCtrls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
      mmrErr = mixerGetLineControls(reinterpret_cast<HMIXEROBJ>(md.hMixer),
                                &mxLineCtrls, MIXER_GETLINECONTROLSF_ONEBYTYPE);
      if (mmrErr != MMSYSERR_NOERROR) {
         return false;
      }
      md.volCtrlID = mxCtrl.dwControlID;
      md.useMute = false;
   } else {
      md.muteCtrlID = mxCtrl.dwControlID;
      md.useMute = true;
   }

   return true;
}

/* -----------------------------------------------------------------------------
    Window Proc
   -------------------------------------------------------------------------- */

LRESULT CALLBACK XPAudio::WindowProc(HWND hWnd, UINT msg, WPARAM wParam,
                                                          LPARAM lParam)
{
   static bool muteLock = false;
   if (msg == MM_MIXM_CONTROL_CHANGE) {
      if (!muteLock && currentlyMuted_ && time(0) - muteTime_ > 1) {
         muteLock = true;
         for (auto md = mixers_.begin(); md != mixers_.end(); ++md) {
            if (reinterpret_cast<HMIXER>(wParam) == md->hMixer) {
               DWORD notifyID = static_cast<DWORD>(lParam);
               if (notifyID == md->muteCtrlID || notifyID == md->volCtrlID) {
                  MuteDevice(*md, false);
               }
            }
         }
         SetTimer(hWnd, 5, 200, NULL);
      }
      return 0;
   } else if (msg == WM_TIMER) {
      if (wParam == 5 && muteLock) {
         KillTimer(hWnd, 5);
         muteLock = false;
      }
   }
   return DefWindowProc(hWnd, msg, wParam, lParam);
}


/* -----------------------------------------------------------------------------
    Mute Logic
   -------------------------------------------------------------------------- */

bool XPAudio::IsMuted()
{
   return currentlyMuted_;
}

DWORD XPAudio::GetOldControlValue(const MixerDevice& md)
{
   MMRESULT err;
   MIXERCONTROLDETAILS mxcd = {0};
   mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
   mxcd.cChannels = 1;
   DWORD oldVal;
   if (md.useMute) { // Mute control
      MIXERCONTROLDETAILS_BOOLEAN mxCtrlDetMuted = { 0 };
      mxcd.dwControlID = md.muteCtrlID;
      mxcd.paDetails = &mxCtrlDetMuted;
      mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);

      err = mixerGetControlDetails(reinterpret_cast<HMIXEROBJ>(md.hMixer),
                                   &mxcd, MIXER_GETCONTROLDETAILSF_VALUE);
      oldVal = mxCtrlDetMuted.fValue;
   } else { // Volume Slider control
      MIXERCONTROLDETAILS_UNSIGNED mxCtrlVolume = { 0 };

      mxcd.dwControlID = md.volCtrlID;
      mxcd.paDetails = &mxCtrlVolume;
      mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);

      err = mixerGetControlDetails(reinterpret_cast<HMIXEROBJ>(md.hMixer),
                                   &mxcd, MIXER_GETCONTROLDETAILSF_VALUE);
      oldVal = mxCtrlVolume.dwValue;
   }

   if (err != MMSYSERR_NOERROR) {
      PrintMixerError(L"mixerGetControlDetails", err);
      return 0;
   }

   return oldVal;
}

void XPAudio::SetDeviceValue(XPAudio::MixerDevice& md, DWORD val)
{
   MIXERCONTROLDETAILS mxcd = {0};
   mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
   mxcd.cChannels = 1;

   MIXERCONTROLDETAILS_UNSIGNED mxCtrlDetVolume = { val };
   MIXERCONTROLDETAILS_BOOLEAN mxCtrlDetMuted   = { static_cast<LONG>(val) };

   if (md.useMute) {
      mxcd.dwControlID = md.muteCtrlID;
      mxcd.paDetails = &mxCtrlDetMuted;
      mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
   } else { // Volume Slider
      mxcd.dwControlID = md.volCtrlID;
      mxcd.paDetails = &mxCtrlDetVolume;
      mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
   }

   MMRESULT mmrErr = mixerSetControlDetails(
                          reinterpret_cast<HMIXEROBJ>(md.hMixer), &mxcd,
                          MIXER_OBJECTF_MIXER | MIXER_SETCONTROLDETAILSF_VALUE);

   if (mmrErr != MMSYSERR_NOERROR) {
      PrintMixerError(_T("mixerSetControlDetails"), mmrErr);
   }
}

void XPAudio::MuteDevice(XPAudio::MixerDevice& md, bool saveOldValue)
{
   if (saveOldValue) {
      md.oldValue = GetOldControlValue(md);
   }
   SetDeviceValue(md, md.useMute ? 1 : 0);
}

void XPAudio::Mute()
{
   for (auto md = mixers_.begin(); md != mixers_.end(); ++md) {
      MuteDevice(*md, true);
   }
   currentlyMuted_ = true;
   muteTime_ = time(0);
}

void XPAudio::UnMute()
{
   muteTime_ = 0;
   currentlyMuted_ = false; // Order important (s. WindowProc notification)
   for (auto md = mixers_.begin(); md != mixers_.end(); ++md) {
      SetDeviceValue(*md, md->oldValue);
   }
}
