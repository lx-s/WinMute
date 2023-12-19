/*
 WinMute
           Copyright (c) 2023, Alexander Steinhoefer

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
#include "WinAudio.h"
#include "MMNotificationClient.h"

#include <Functiondiscoverykeys_devpkey.h>

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
   REFIID riid, VOID** ppvInterface)
{
   if (IID_IUnknown == riid) {
      AddRef();
      *ppvInterface = reinterpret_cast<IUnknown*>(this);
   } else if (__uuidof(IMMNotificationClient) == riid) {
      AddRef();
      *ppvInterface = reinterpret_cast<IMMNotificationClient*>(this);
   } else {
      *ppvInterface = nullptr;
      return E_NOINTERFACE;
   }
   return S_OK;
}


HRESULT STDMETHODCALLTYPE MMNotificationClient::OnDefaultDeviceChanged(
   EDataFlow, ERole, LPCWSTR)
{
   return S_OK;
}

HRESULT STDMETHODCALLTYPE MMNotificationClient::OnDeviceAdded(LPCWSTR pwstrDeviceId)
{
   if (notifyParent_ && pwstrDeviceId != nullptr) {
      const auto deviceName = GetFriendlyDeviceName(pwstrDeviceId);
      WMLog::GetInstance().GetInstance().LogInfo(L"Device \"%s\" added", deviceName.c_str());
      notifyParent_->ShouldReInit();
   }
   return S_OK;
}

HRESULT STDMETHODCALLTYPE MMNotificationClient::OnDeviceRemoved(LPCWSTR pwstrDeviceId)
{
   if (notifyParent_ && pwstrDeviceId != nullptr) {
      const auto deviceName = GetFriendlyDeviceName(pwstrDeviceId);
      WMLog::GetInstance().GetInstance().LogInfo(L"Device \"%s\" removed", deviceName.c_str());
      notifyParent_->ShouldReInit();
   }
   return S_OK;
}

HRESULT STDMETHODCALLTYPE MMNotificationClient::OnDeviceStateChanged(
   LPCWSTR pwstrDeviceId, DWORD dwNewState)
{
   bool notify = true;
   std::wstring what;
   if (pwstrDeviceId == nullptr) {
      return S_OK;
   }
   if (dwNewState == DEVICE_STATE_NOTPRESENT) {
      what = L"Not present";
   } else if (dwNewState == DEVICE_STATE_UNPLUGGED) {
      what = L"Unplugged";
   } else if (dwNewState == DEVICE_STATE_ACTIVE) {
      what = L"Active";
   } else {
      notify = false;
   }

   if (notify && notifyParent_) {
      // TODO: Check if output device
      const auto deviceName = GetFriendlyDeviceName(pwstrDeviceId);
      WMLog::GetInstance().GetInstance().LogInfo(
         L"Device \"%s\" status changed to %s",
         deviceName.c_str(),
         what.c_str());
      notifyParent_->ShouldReInit();
   } 
   return S_OK;
}

HRESULT STDMETHODCALLTYPE MMNotificationClient::OnPropertyValueChanged(
   LPCWSTR, const PROPERTYKEY)
{
   return S_OK;
}

std::wstring MMNotificationClient:: GetFriendlyDeviceName(LPCWSTR pwstrDeviceId)
{
   HRESULT hr = S_OK;
   IMMDevice *pDevice = nullptr;
   IPropertyStore *pProps = nullptr;
   PROPVARIANT varString;

   PropVariantInit(&varString);

   if (pEnumerator_ == nullptr) {
      // Get enumerator for audio endpoint devices.
      hr = CoCreateInstance(
         __uuidof(MMDeviceEnumerator),
         nullptr, CLSCTX_INPROC_SERVER,
         __uuidof(IMMDeviceEnumerator),
         reinterpret_cast<void **>(&pEnumerator_));
   }
   if (hr == S_OK) {
      hr = pEnumerator_->GetDevice(pwstrDeviceId, &pDevice);
   }
   if (hr == S_OK) {
      hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
   }
   if (hr == S_OK){
      // Get the endpoint device's friendly-name property.
      hr = pProps->GetValue(PKEY_Device_FriendlyName, &varString);
   }
   std::wstring deviceName = L"Unknown device";
   if (hr == S_OK) {
      deviceName = varString.pwszVal;
   }

   PropVariantClear(&varString);

   SafeRelease(&pProps);
   SafeRelease(&pDevice);
   return deviceName;
}
