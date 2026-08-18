/*
 WinMute
           Copyright (c) 2011-2026 Alexander Steinhoefer

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

#include <Functiondiscoverykeys_devpkey.h>
#include <mmdeviceapi.h>

#include "WinAudio.h"
#include "common.h"

Endpoint::~Endpoint()
{
    if (sessionCtrl && wasapiAudioEvents != nullptr) {
        sessionCtrl->UnregisterAudioSessionNotification(wasapiAudioEvents);
    }
}

VistaAudio::VistaAudio()
    : deviceEnumerator_(nullptr),
      mmnAudioEvents_(nullptr),
      reInit_(false),
      muteSpecificEndpoints_(false),
      muteSpecificEndpointsAllowList_(false),
      hParent_(nullptr)
{
}

VistaAudio::~VistaAudio()
{
    Uninit();
}

bool VistaAudio::LoadAllEndpoints()
{
    WMLog& log = WMLog::GetInstance();

    assert(deviceEnumerator_ != nullptr);

    CComPtr<IMMDeviceCollection> audioEndpoints;
    HRESULT hr = deviceEnumerator_->EnumAudioEndpoints(
        eRender, DEVICE_STATE_ACTIVE, &audioEndpoints);
    if (FAILED(hr)) {
        return false;
    }

    UINT epCount;
    hr = audioEndpoints->GetCount(&epCount);
    if (FAILED(hr)) {
        return false;
    }

    for (UINT i = 0; i < epCount; ++i) {
        std::unique_ptr<Endpoint> ep = std::make_unique<Endpoint>();
        CComPtr<IMMDevice> device = nullptr;

        hr = audioEndpoints->Item(i, &device);
        if (FAILED(hr)) {
            log.LogError(L"Failed to get audio endpoint #{}", i);
            continue;
        }

        const auto deviceName = GetAudioDeviceName(device);
        if (!deviceName) {
            log.LogError(L"Failed to get device name for audio endpoint #{}",
                         i);
            continue;
        } else {
            log.LogInfo(L"Found audio endpoint \"{}\"", *deviceName);
            ep->deviceName = *deviceName;
        }

        CComPtr<IAudioSessionManager2> sessionManager2;
        if (FAILED(device->Activate(
                __uuidof(IAudioSessionManager2), CLSCTX_INPROC_SERVER, nullptr,
                reinterpret_cast<LPVOID*>(&sessionManager2))))
        {
            log.LogError(L"Failed to retrieve audio session manager for \"{}\"",
                         ep->deviceName);
            continue;
        }

        hr = sessionManager2->GetAudioSessionControl(nullptr, 0,
                                                     &ep->sessionCtrl);
        if (FAILED(hr)) {
            log.LogError(L"Failed to retrieve audio session manager for \"{}\"",
                         ep->deviceName);
            continue;
        }

        // Attach: the CComPtr takes over the initial reference from new,
        // so the refcount stays balanced.
        ep->wasapiAudioEvents.Attach(new VistaAudioSessionEvents(this));
        ep->sessionCtrl->RegisterAudioSessionNotification(
            ep->wasapiAudioEvents);

        hr = device->Activate(__uuidof(IAudioEndpointVolume),
                              CLSCTX_INPROC_SERVER, nullptr,
                              reinterpret_cast<LPVOID*>(&ep->endpointVolume));
        if (FAILED(hr)) {
            log.LogError(L"Failed to active endpoint volume for device \"{}\"",
                         ep->deviceName);
            continue;
        }
        endpoints_.push_back(std::move(ep));
    }

    return true;
}

bool VistaAudio::Init(HWND hParent)
{
    WMLog& log = WMLog::GetInstance();

    hParent_ = hParent;

    if (FAILED(deviceEnumerator_.CoCreateInstance(
            __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER)))
    {
        log.LogError(L"Failed to create instance of MMDeviceEnumerator");
        return false;
    }

    LoadAllEndpoints();

    // Attach: the client is created with a refcount of 1; a plain assignment
    // would AddRef it to 2 and Uninit's single Release would leak it.
    mmnAudioEvents_.Attach(new MMNotificationClient(this));
    deviceEnumerator_->RegisterEndpointNotificationCallback(mmnAudioEvents_);

    return true;
}

void VistaAudio::Uninit()
{
    if (deviceEnumerator_ && mmnAudioEvents_) {
        deviceEnumerator_->UnregisterEndpointNotificationCallback(
            mmnAudioEvents_);
    }
    deviceEnumerator_.Release();
    mmnAudioEvents_.Release();

    endpoints_.clear();
}

void VistaAudio::ShouldReInit()
{
    reInit_ = true;
}

void VistaAudio::OnAudioServiceShutdown()
{
    PostMessageW(hParent_, WM_WINMUTE_AUDIO_SERVICE_SHUTDOWN, 0, 0);
}

bool VistaAudio::CheckForReInit()
{
    if (reInit_.exchange(false)) {
        Uninit();
        return Init(hParent_);
    }
    return true;
}

bool VistaAudio::AllEndpointsMuted()
{
    WMLog& log = WMLog::GetInstance();

    if (!CheckForReInit()) {
        return false;
    }
    bool anyManaged = false;
    for (auto& e : endpoints_) {
        if (!IsEndpointManaged(e->deviceName)) {
            continue;
        }
        anyManaged = true;
        BOOL isMuted = FALSE;
        if (FAILED(e->endpointVolume->GetMute(&isMuted))) {
            log.LogError(L"Failed to get mute status for \"{}\"",
                         e->deviceName);
            return false;
        } else if (isMuted == FALSE) {
            return false;
        }
    }
    // Without a single managed endpoint there is nothing that could be muted,
    // so don't report "everything is muted" for an empty selection.
    return anyManaged;
}

bool VistaAudio::SaveMuteStatus()
{
    bool success = true;
    WMLog& log = WMLog::GetInstance();

    if (CheckForReInit()) {
        for (auto& e : endpoints_) {
            BOOL isMuted = FALSE;
            if (FAILED(e->endpointVolume->GetMute(&isMuted))) {
                log.LogError(L"Failed to get mute status for \"{}\"",
                             e->deviceName);
                success = false;
            } else {
                e->wasMuted = isMuted;
            }
        }
    }
    return success;
}

bool VistaAudio::RestoreMuteStatus()
{
    bool success = true;
    WMLog& log = WMLog::GetInstance();

    if (!CheckForReInit()) {
        return false;
    }
    for (auto& e : endpoints_) {
        if (!IsEndpointManaged(e->deviceName)) {
            log.LogInfo(L"Skipping Endpoint {}", e->deviceName);
            continue;
        }
        log.LogInfo(L"Restoring: Mute {} for \"{}\"",
                    (e->wasMuted) ? L"true" : L"false", e->deviceName);
        if (e->wasMuted != true) {
            if (FAILED(e->endpointVolume->SetMute(false, nullptr))) {
                log.LogError(L"Failed to restore mute status to {} for \"{}\"",
                             (e->wasMuted) ? L"true" : L"false", e->deviceName);
                success = false;
            }
        }
    }

    return success;
}

void VistaAudio::SetMute(bool mute)
{
    WMLog& log = WMLog::GetInstance();
    if (CheckForReInit()) {
        for (auto& e : endpoints_) {
            BOOL isMuted = !mute;
            if (!IsEndpointManaged(e->deviceName)) {
                log.LogInfo(L"Skipping Endpoint {}", e->deviceName);
                continue;
            }
            if (FAILED(e->endpointVolume->GetMute(&isMuted))) {
                log.LogError(L"Failed to get mute status for \"{}\"",
                             e->deviceName);
            }
            if (!!isMuted != mute) {
                if (FAILED(e->endpointVolume->SetMute(mute, nullptr))) {
                    log.LogError(L"Failed to set mute status to {} for \"{}\"",
                                 (e->wasMuted) ? L"true" : L"false",
                                 e->deviceName);
                }
            }
        }
    }
}

bool VistaAudio::IsEndpointManaged(const std::wstring& endpointName) const
{
    if (!muteSpecificEndpoints_) {
        return true;
    }

    const bool inManagedEndpoints =
        std::find(std::begin(managedEndpointNames_),
                  std::end(managedEndpointNames_),
                  endpointName) != std::end(managedEndpointNames_);

    // ------------+----------+-------------+
    //             | In List  | Not in List |
    // ------------+----------+-------------+
    //  Allow List | Mute     |  Not mute   |
    //  Block List | Not mute |  Mute       |

    return inManagedEndpoints ? muteSpecificEndpointsAllowList_
                              : !muteSpecificEndpointsAllowList_;
}

void VistaAudio::MuteSpecificEndpoints(bool muteSpecific)
{
    muteSpecificEndpoints_ = muteSpecific;
}

void VistaAudio::SetManagedEndpoints(const std::vector<std::wstring>& endpoints,
                                     bool isAllowList)
{
    managedEndpointNames_ = endpoints;
    muteSpecificEndpointsAllowList_ = isAllowList;
}
