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

#include "VistaAudioSessionEvents.h"
#include "MMNotificationClient.h"

_COM_SMARTPTR_TYPEDEF(IAudioEndpointVolume, __uuidof(IAudioEndpointVolume));
_COM_SMARTPTR_TYPEDEF(IMMDeviceEnumerator, __uuidof(IMMDeviceEnumerator));
_COM_SMARTPTR_TYPEDEF(IAudioSessionControl, __uuidof(IAudioSessionControl));

class WinAudio {
public:
   virtual bool Init(HWND hParent) = 0;
   virtual void ShouldReInit() = 0;
   virtual bool AllEndpointsMuted() = 0;
   virtual bool SaveMuteStatus() = 0;
   virtual bool RestoreMuteStatus() = 0;
   virtual void SetMute(bool mute) = 0;
   virtual void MuteSpecificEndpoints(bool muteSpecific) = 0;
   virtual void SetManagedEndpoints(
      const std::vector<std::wstring> &endpoints,
      bool isAllowList) = 0;
   virtual ~WinAudio() noexcept {};
};

struct Endpoint {
   wchar_t deviceName[100];
   IAudioEndpointVolumePtr endpointVolume;
   IAudioSessionControlPtr sessionCtrl;
   std::unique_ptr<VistaAudioSessionEvents> wasapiAudioEvents;

   bool wasMuted;

   Endpoint();
   ~Endpoint();
   Endpoint(const Endpoint&) = delete;
   Endpoint& operator=(const Endpoint&) = delete;
};

class VistaAudio : public WinAudio {
public:
   VistaAudio();
   ~VistaAudio() noexcept;

   bool Init(HWND hParent) override;
   void ShouldReInit() override;
   bool AllEndpointsMuted() override;
   bool SaveMuteStatus() override;
   bool RestoreMuteStatus() override;
   void SetMute(bool mute) override;

   void MuteSpecificEndpoints(bool muteSpecific) override;
   void SetManagedEndpoints(
      const std::vector<std::wstring> &endpoints,
      bool isAllowList) override;

private:
   void Uninit();
   bool CheckForReInit();

   bool LoadAllEndpoints();
   bool IsEndpointManaged(const std::wstring& endpointName) const;

   std::vector<std::unique_ptr<Endpoint>> endpoints_;
   MMNotificationClient* mmnAudioEvents_;
   IMMDeviceEnumeratorPtr deviceEnumerator_;

   bool reInit_;
   bool muteSpecificEndpoints_;
   bool muteSpecificEndpointsAllowList_;
   HWND hParent_;

   std::vector<std::wstring> managedEndpointNames_;

   // non copy-able
   VistaAudio(const VistaAudio& other) = delete;
   VistaAudio& operator=(const VistaAudio& other) = delete;
};
