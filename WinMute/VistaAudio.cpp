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
#include <mmdeviceapi.h>
#include <comip.h>
#include <comdef.h>
#include <Functiondiscoverykeys_devpkey.h>

#pragma warning(disable : 4201)
#  include <endpointvolume.h>
#pragma warning(default : 4201)

#include "WinAudio.h"

#define IF_FAILED_JUMP(hResult, ExitLabel) if (FAILED(hr)) { goto ExitLabel; }

_COM_SMARTPTR_TYPEDEF(IPropertyStore, __uuidof(IPropertyStore));
_COM_SMARTPTR_TYPEDEF(IMMDevice,      __uuidof(IMMDevice));
_COM_SMARTPTR_TYPEDEF(IMMDeviceCollection,   __uuidof(IMMDeviceCollection));
_COM_SMARTPTR_TYPEDEF(IMMDeviceEnumerator,   __uuidof(IMMDeviceEnumerator));
_COM_SMARTPTR_TYPEDEF(IAudioSessionManager2, __uuidof(IAudioSessionManager2));
_COM_SMARTPTR_TYPEDEF(IAudioSessionControl, __uuidof(IAudioSessionControl));

Endpoint::Endpoint()
   : endpointVolume(nullptr), wasapiAudioEvents(nullptr), wasMuted(false)
{
   deviceName[0] = _T('\0');
}

Endpoint::~Endpoint()
{
   if (endpointVolume != nullptr) {
      SafeRelease(&endpointVolume);
   }
}

VistaAudio::VistaAudio() :
   endpointVolume_(nullptr),
   sessionControl_(nullptr),
   deviceEnumerator_(nullptr),
   wasapiAudioEvents_(nullptr),
   mmnAudioEvents_(nullptr),
   reInit_(false),
   oldVolume_(0),
   hParent_(nullptr)
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

bool VistaAudio::LoadAllEndpoints()
{
   WMLog& log = WMLog::GetInstance();
   IMMDeviceEnumeratorPtr deviceEnumerator;
   if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator),
              nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&deviceEnumerator)))) {
      log.Write(_T("Failed to create instance of MMDeviceEnumerator"));
      return false;
   }

   IMMDeviceCollectionPtr audioEndpoints;
   HRESULT hr = deviceEnumerator->EnumAudioEndpoints(
      eRender,
      DEVICE_STATE_ACTIVE,
      &audioEndpoints);
   IF_FAILED_JUMP(hr, exit_error);

   UINT epCount;
   hr = audioEndpoints->GetCount(&epCount);
   IF_FAILED_JUMP(hr, exit_error);

   for (UINT i = 0; i < epCount; ++i) {
      std::unique_ptr<Endpoint> ep = std::make_unique<Endpoint>();
      IMMDevicePtr device = nullptr;
      IPropertyStorePtr propStore;


      hr = audioEndpoints->Item(i, &device);
      if (FAILED(hr)) {
         log.Write(_T("Failed to get audio endpoint #{}"), i);
         continue;
      }

      hr = device->OpenPropertyStore(STGM_READ, &propStore);
      if (FAILED(hr)) {
         log.Write(_T("Failed to open property store for audio endpoint #{}"), i);
         continue;
      }

      PROPVARIANT value;
      PropVariantInit(&value);
      hr = propStore->GetValue(PKEY_Device_FriendlyName, &value);
      if (FAILED(hr)) {
         log.Write(_T("Failed to get device name for audio endpoint #{}"), i);
         continue;
      }
      log.Write(_T("Found audio endpoint {}"), value.pwszVal);
      StringCchCopy(ep->deviceName, sizeof(ep->deviceName), value.pwszVal);
      PropVariantClear(&value);


      IAudioSessionManager2Ptr sessionManager2;
      if (FAILED(device->Activate(
            __uuidof(IAudioSessionManager2), CLSCTX_INPROC_SERVER, nullptr,
            reinterpret_cast<LPVOID*>(&sessionManager2)))) {
         log.Write(_T("Failed to retrieve audio session manager for {}"),
                   ep->deviceName);
         continue;
      }

      IAudioSessionControlPtr sessionControl;
      hr = sessionManager2->GetAudioSessionControl(nullptr, 0, &sessionControl);
      if (FAILED(hr)) {
         log.Write(_T("Failed to retrieve audio session manager for {}"),
            ep->deviceName);
         continue;
      }

      ep->wasapiAudioEvents = std::make_unique<VistaAudioSessionEvents>(this);
      if (wasapiAudioEvents_) {
         sessionControl->RegisterAudioSessionNotification(wasapiAudioEvents_);
      }

      hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER,
                            nullptr, reinterpret_cast<LPVOID*>(&ep->endpointVolume));
      if (FAILED(hr)) {
         log.Write(_T("Failed to active endpoint volume for device {}"),
                   ep->deviceName);
         continue;
      }
      endpoints_.push_back(std::move(ep));
   }

exit_error:

   return true;
}

bool VistaAudio::Init(HWND hParent)
{
   hParent_ = hParent;
   reInit_ = false;

   if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator),
      nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&deviceEnumerator_)))) {
      return false;
   }

   LoadAllEndpoints();

   IMMDevice* defaultDevice = nullptr;
   HRESULT hr = deviceEnumerator_->GetDefaultAudioEndpoint(eRender, eConsole,
      &defaultDevice);

   if (FAILED(hr)) {
      if (hr != E_NOTFOUND) {
         TaskDialog(nullptr,
            nullptr,
            PROGRAM_NAME,
            _T("Failed to get default audio endpoint device"),
            _T("WinMute is not able to recover from that condition.\n")
            _T("Please try restarting the program"),
            TDCBF_OK_BUTTON,
            TD_ERROR_ICON,
            nullptr);
      }
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

   endpoints_.clear();
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
   if (CheckForReInit() && endpointVolume_ != nullptr) {
      BOOL isMuted = FALSE;
      endpointVolume_->GetMute(&isMuted);
      return isMuted == TRUE;
   }
   return false;
}

void VistaAudio::SetMute(bool mute)
{
   if (CheckForReInit() && endpointVolume_ != nullptr) {
      BOOL isMuted = !mute;
      endpointVolume_->GetMute(&isMuted);
      if (!!isMuted != mute) {
         endpointVolume_->SetMute(mute, nullptr);
      }
   }
}
