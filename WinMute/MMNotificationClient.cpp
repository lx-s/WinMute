/*
 WinMute
           Copyright (c) 2017, Alexander Steinhoefer

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
#include "WinAudio.h"
#include "MMNotificationClient.h"

class WinAudio;

MMNotificationClient::MMNotificationClient(WinAudio* notifyParent) :
   ref_(1), pEnumerator_(nullptr), notifyParent_(notifyParent)
{
}

MMNotificationClient::~MMNotificationClient()
{
   SafeRelease(&pEnumerator_);
}

ULONG STDMETHODCALLTYPE MMNotificationClient::AddRef()
{
   return InterlockedIncrement(&ref_);
}

ULONG STDMETHODCALLTYPE MMNotificationClient::Release()
{
   ULONG ref = InterlockedDecrement(&ref_);
   if (ref == 0) {
      delete this;
   }
   return ref;
}

HRESULT STDMETHODCALLTYPE MMNotificationClient::QueryInterface(
                                                            REFIID riid,
                                                            VOID **ppvInterface)
{
   if (IID_IUnknown == riid) {
      AddRef();
      *ppvInterface = (IUnknown*)this;
   } else if (__uuidof(IMMNotificationClient) == riid) {
      AddRef();
      *ppvInterface = (IMMNotificationClient*)this;
   } else {
      *ppvInterface = nullptr;
      return E_NOINTERFACE;
   }
   return S_OK;
}


HRESULT STDMETHODCALLTYPE MMNotificationClient::OnDefaultDeviceChanged(
                                                     EDataFlow,
                                                     ERole,
                                                     LPCWSTR)
{
   if (notifyParent_) {
      notifyParent_->ShouldReInit();
   }
   return S_OK;
}

HRESULT STDMETHODCALLTYPE MMNotificationClient::OnDeviceAdded(LPCWSTR)
{
   return S_OK;
}

HRESULT STDMETHODCALLTYPE MMNotificationClient::OnDeviceRemoved(LPCWSTR)
{
   if (notifyParent_) {
      notifyParent_->ShouldReInit();
   }
   return S_OK;
}

HRESULT STDMETHODCALLTYPE MMNotificationClient::OnDeviceStateChanged(LPCWSTR,
                                                                     DWORD)
{
   return S_OK;
}

HRESULT STDMETHODCALLTYPE MMNotificationClient::OnPropertyValueChanged(
                                                              LPCWSTR,
                                                              const PROPERTYKEY)
{
   return S_OK;
}

