/*
 WinMute
           Copyright (C) 2016, Alexander Steinhoefer

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
#include <mmdeviceapi.h>

#pragma warning(disable : 4201)
#  include <endpointvolume.h>
#pragma warning(default : 4201)

#include "WinAudio.h"
#include "VistaAudioSessionEvents.h"
#include "MMNotificationClient.h"

VistaAudio::VistaAudio() :
   endpointVolume_(nullptr), sessionControl_(nullptr),
   wasapiAudioEvents_(nullptr), mmnAudioEvents_(nullptr),
   deviceEnumerator_(nullptr), reInit_(false)
{
   if (FAILED(CoInitialize(nullptr))) {
      throw std::exception("Failed to initialize COM Library");
   }
}

VistaAudio::~VistaAudio()
{
   Uninit();
   CoUninitialize();
}

bool VistaAudio::Init(HWND hParent)
{
   hParent_ = hParent;
   reInit_ = false;

   if(FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator),
                       nullptr, CLSCTX_INPROC_SERVER,
                       IID_PPV_ARGS(&deviceEnumerator_)))) {
     return false;
   }

   IMMDevice *defaultDevice = nullptr;
   if(FAILED(deviceEnumerator_->GetDefaultAudioEndpoint(eRender, eConsole,
      &defaultDevice))) {
      TaskDialog(nullptr,
                 nullptr,
                 PROGRAM_NAME,
                 _T("Failed to get default audio endpoint device"),
                 _T("WinMute is not able to recover from that condition.\n")
                 _T("Please try restarting the program"),
                 TDCBF_OK_BUTTON,
                 TD_ERROR_ICON,
                 nullptr);
      return false;
   }


   IAudioSessionManager2* sessionManager2 = nullptr;
   if (FAILED(defaultDevice->Activate(__uuidof(IAudioSessionManager2),
                                CLSCTX_INPROC_SERVER, nullptr,
                                reinterpret_cast<LPVOID*>(&sessionManager2)))) {
      TaskDialog(nullptr,
                 nullptr,
                 PROGRAM_NAME,
                 _T("Failed to retrieve audio session manager."),
                 _T("Please try restarting the program"),
                 TDCBF_OK_BUTTON,
                 TD_ERROR_ICON,
                 nullptr);
      SafeRelease(&defaultDevice);
      return false;
   }

   if (FAILED(sessionManager2->GetAudioSessionControl(nullptr, 0,
                                                      &sessionControl_))) {
      TaskDialog(nullptr,
                 nullptr,
                 PROGRAM_NAME,
                 _T("Failed to retrieve session control."),
                 _T("Please try restarting the program"),
                 TDCBF_OK_BUTTON,
                 TD_ERROR_ICON,
                 nullptr);
      SafeRelease(&sessionManager2);
      return false;
   }
   SafeRelease(&sessionManager2);

   wasapiAudioEvents_ = new VistaAudioSessionEvents(this);
   if (wasapiAudioEvents_) {
      sessionControl_->RegisterAudioSessionNotification(wasapiAudioEvents_);
   }
   mmnAudioEvents_ = new MMNotificationClient(this);
   if (mmnAudioEvents_) {
      deviceEnumerator_->RegisterEndpointNotificationCallback(mmnAudioEvents_);
   }

   if (FAILED(defaultDevice->Activate(__uuidof(IAudioEndpointVolume),
                                CLSCTX_INPROC_SERVER, nullptr,
                                reinterpret_cast<LPVOID*>(&endpointVolume_)))) {
      TaskDialog(nullptr,
                 nullptr,
                 PROGRAM_NAME,
                 _T("Failed to activate default audio device."),
                 _T("Please try restarting the program"),
                 TDCBF_OK_BUTTON,
                 TD_ERROR_ICON,
                 nullptr);
      SafeRelease(&defaultDevice);
      return false;
   }
   defaultDevice->Release();

   return true;
}

void VistaAudio::Uninit()
{
   if (sessionControl_ && wasapiAudioEvents_) {
      sessionControl_->UnregisterAudioSessionNotification(wasapiAudioEvents_);
   }
   if (deviceEnumerator_ && mmnAudioEvents_) {
      deviceEnumerator_->UnregisterEndpointNotificationCallback(mmnAudioEvents_);
   }

   SafeRelease(&wasapiAudioEvents_);
   SafeRelease(&mmnAudioEvents_);
   SafeRelease(&sessionControl_);
   SafeRelease(&endpointVolume_);
   SafeRelease(&deviceEnumerator_);
}

void VistaAudio::ShouldReInit()
{
   reInit_ = true;
}

bool VistaAudio::CheckForReInit()
{
   bool ok = true;
   if (reInit_) {
      Uninit();
      ok = Init(hParent_);
   }
   return ok;
}

bool VistaAudio::IsMuted()
{
   if (CheckForReInit()) {
      BOOL isMuted = FALSE;
      endpointVolume_->GetMute(&isMuted);
      return isMuted == TRUE;
   }
   return false;
}

void VistaAudio::Mute()
{
   if (CheckForReInit()) {
      BOOL isMuted = FALSE;
      endpointVolume_->GetMute(&isMuted);
      if (!isMuted) {
         endpointVolume_->SetMute(TRUE, nullptr);
      }
   }
}

void VistaAudio::UnMute()
{
   if (CheckForReInit()) {
      BOOL isMuted = TRUE;
      endpointVolume_->GetMute(&isMuted);
      if (isMuted) {
         endpointVolume_->SetMute(FALSE, nullptr);
      }
   }
}
