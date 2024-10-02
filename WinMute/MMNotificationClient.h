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

#pragma once

#include "Common.h"

class WinAudio;

class MMNotificationClient : public IMMNotificationClient {
public:
   explicit MMNotificationClient(WinAudio* notifyParent);
   ~MMNotificationClient();

   STDMETHODIMP_(ULONG) AddRef();
   STDMETHODIMP_(ULONG) Release();
   STDMETHODIMP_(HRESULT) QueryInterface(REFIID riid, VOID** ppvInterface);

   STDMETHODIMP_(HRESULT) OnDefaultDeviceChanged(EDataFlow flow, ERole role,
      LPCWSTR pwstrDeviceId);
   STDMETHODIMP_(HRESULT) OnDeviceAdded(LPCWSTR pwstrDeviceId);
   STDMETHODIMP_(HRESULT) OnDeviceRemoved(LPCWSTR pwstrDeviceId);
   STDMETHODIMP_(HRESULT) OnDeviceStateChanged(LPCWSTR pwstrDeviceId,
      DWORD dwNewState);
   STDMETHODIMP_(HRESULT) OnPropertyValueChanged(LPCWSTR pwstrDeviceId,
      const PROPERTYKEY key);

private:
   std::atomic<LONG> ref_count_;
   IMMDeviceEnumerator* pEnumerator_;
   WinAudio* notifyParent_;

   std::wstring GetFriendlyDeviceName(LPCWSTR pwstrDeviceId);
};

