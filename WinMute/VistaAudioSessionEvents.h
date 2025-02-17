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
#include <audiopolicy.h>

class WinAudio;

class VistaAudioSessionEvents : public IAudioSessionEvents {
public:
   explicit VistaAudioSessionEvents(WinAudio* notifyParent);
   ~VistaAudioSessionEvents();

   // IUnknown methods -- AddRef, Release, and QueryInterface
   ULONG STDMETHODCALLTYPE AddRef();
   ULONG STDMETHODCALLTYPE Release();

   HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID** ppvInterface);

   // Notification methods for audio session events
   HRESULT STDMETHODCALLTYPE OnDisplayNameChanged(
      LPCWSTR newDisplayName,
      LPCGUID eventContext) noexcept override;

   HRESULT STDMETHODCALLTYPE OnIconPathChanged(
      LPCWSTR newIconPath,
      LPCGUID eventContext) noexcept override;

   HRESULT STDMETHODCALLTYPE OnSimpleVolumeChanged(
      float newVolume,
      BOOL newMute,
      LPCGUID eventContext) noexcept override;

   HRESULT STDMETHODCALLTYPE OnChannelVolumeChanged(
      DWORD channelCount,
      float newChannelVolumeArray[],
      DWORD changedChannel,
      LPCGUID eventContext) noexcept override;

   HRESULT STDMETHODCALLTYPE OnGroupingParamChanged(
      LPCGUID newGroupingParam,
      LPCGUID eventContext) noexcept override;

   HRESULT STDMETHODCALLTYPE OnStateChanged(
      AudioSessionState newState) noexcept override;

   HRESULT STDMETHODCALLTYPE OnSessionDisconnected(
      AudioSessionDisconnectReason disconnectReason) noexcept override;

private:
   LONG ref_;
   WinAudio* notifyParent_;
};
