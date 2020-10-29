/*
 WinMute
           Copyright (c) 2020, Alexander Steinhoefer

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

#include "StdAfx.h"

class WinAudio {
public:
   virtual bool Init(HWND hParent) = 0;
   virtual void ShouldReInit() = 0;
   virtual bool IsMuted() = 0;
   virtual void Mute() = 0;
   virtual void UnMute() = 0;
   virtual ~WinAudio() noexcept { };
};

// Forwards
struct IAudioEndpointVolume;
struct IAudioSessionControl;
struct IMMDeviceEnumerator;
class VistaAudioSessionEvents;
class MMNotificationClient;

class VistaAudio : public WinAudio {
public:
   VistaAudio();
   ~VistaAudio() noexcept;

   bool Init(HWND hParent) override;
   void ShouldReInit() override;
   bool IsMuted() override;
   void Mute() override;
   void UnMute() override;
private:
   void Uninit();
   bool CheckForReInit();

   IAudioEndpointVolume* endpointVolume_;
   IAudioSessionControl* sessionControl_;
   IMMDeviceEnumerator *deviceEnumerator_;
   VistaAudioSessionEvents* wasapiAudioEvents_;
   MMNotificationClient*    mmnAudioEvents_;

   bool reInit_;
   float oldVolume_;
   HWND hParent_;

   // non copy-able
   VistaAudio(const VistaAudio& other) = delete;
   VistaAudio& operator=(const VistaAudio& other) = delete;
};
