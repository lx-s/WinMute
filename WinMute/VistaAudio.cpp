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

#include "common.h"
#include <Functiondiscoverykeys_devpkey.h>
#include <mmdeviceapi.h>

#include "WinAudio.h"

Endpoint::~Endpoint()
{
    if (sessionCtrl && wasapiAudioEvents != nullptr) {
        sessionCtrl->UnregisterAudioSessionNotification(wasapiAudioEvents);
    }
}

// Endpoints attached to a monitor (HDMI/DisplayPort, typically exposed by the
// GPU's audio driver) drop to DEVICE_STATE_UNPLUGGED while the display sleeps.
// They must stay under management across that transition, otherwise a mute
// applied before standby can never be undone afterwards.
static constexpr DWORD MANAGED_DEVICE_STATES =
    DEVICE_STATE_ACTIVE | DEVICE_STATE_UNPLUGGED;

static const wchar_t* DeviceStateToString(DWORD state)
{
    switch (state) {
        case DEVICE_STATE_ACTIVE:
            return L"active";
        case DEVICE_STATE_UNPLUGGED:
            return L"unplugged";
        case DEVICE_STATE_DISABLED:
            return L"disabled";
        case DEVICE_STATE_NOTPRESENT:
            return L"not present";
        default:
            return L"unknown";
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
        eRender, MANAGED_DEVICE_STATES, &audioEndpoints);
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

        const auto deviceId = GetAudioDeviceId(device);
        if (!deviceId) {
            log.LogError(L"Failed to get device id for audio endpoint #{}", i);
            continue;
        }
        ep->deviceId = *deviceId;

        const auto deviceName = GetAudioDeviceName(device);
        if (!deviceName) {
            log.LogError(L"Failed to get device name for audio endpoint #{}",
                         i);
            continue;
        } else {
            DWORD deviceState = 0;
            if (FAILED(device->GetState(&deviceState))) {
                deviceState = 0;
            }
            log.LogInfo(L"Found audio endpoint \"{}\" ({})", *deviceName,
                        DeviceStateToString(deviceState));
            ep->deviceName = *deviceName;
        }

        // Session notifications are a convenience (they trigger a re-init when
        // a session drops), not a requirement for muting. An unplugged endpoint
        // has no session manager, so a failure here must not disqualify it.
        CComPtr<IAudioSessionManager2> sessionManager2;
        if (FAILED(device->Activate(
                __uuidof(IAudioSessionManager2), CLSCTX_INPROC_SERVER, nullptr,
                reinterpret_cast<LPVOID*>(&sessionManager2))))
        {
            log.LogInfo(
                L"No audio session manager for \"{}\";"
                L" continuing without session notifications",
                ep->deviceName);
        } else if (FAILED(sessionManager2->GetAudioSessionControl(
                       nullptr, 0, &ep->sessionCtrl)))
        {
            log.LogInfo(
                L"No audio session control for \"{}\";"
                L" continuing without session notifications",
                ep->deviceName);
        } else {
            // Attach: the CComPtr takes over the initial reference from new,
            // so the refcount stays balanced.
            ep->wasapiAudioEvents.Attach(new VistaAudioSessionEvents(this));
            ep->sessionCtrl->RegisterAudioSessionNotification(
                ep->wasapiAudioEvents);
        }

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

void VistaAudio::OnDeviceArrived()
{
    // Called from a WASAPI notification thread. Only post here; all COM work
    // has to happen on the main thread.
    PostMessageW(hParent_, WM_WINMUTE_AUDIO_DEVICE_ARRIVED, 0, 0);
}

bool VistaAudio::CheckForReInit()
{
    if (reInit_.exchange(false)) {
        Uninit();
        if (!Init(hParent_)) {
            // Without re-arming, a single failed re-init would leave WinMute
            // with an empty endpoint list and no way back: the flag is already
            // consumed, so no later call would ever try again.
            WMLog::GetInstance().LogError(
                L"Audio re-initialization failed; will retry on next event");
            reInit_ = true;
            return false;
        }
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
        if (!IsEndpointManaged(*e)) {
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

// How long after a restore a late-arriving endpoint is still considered part
// of that restore. Monitor-attached endpoints usually reappear within a few
// seconds of the display waking; anything later is a genuinely new device that
// must not be touched.
static constexpr auto LATE_RESTORE_WINDOW = std::chrono::seconds(60);

bool VistaAudio::SaveMuteStatus()
{
    bool success = true;
    WMLog& log = WMLog::GetInstance();

    if (CheckForReInit()) {
        // A new mute cycle supersedes any restore still waiting for a device.
        pendingRestoreIds_.clear();
        savedMuteState_.clear();
        for (auto& e : endpoints_) {
            BOOL isMuted = FALSE;
            if (FAILED(e->endpointVolume->GetMute(&isMuted))) {
                log.LogError(L"Failed to get mute status for \"{}\"",
                             e->deviceName);
                success = false;
            } else {
                savedMuteState_[e->deviceId] = !!isMuted;
            }
        }
    }
    return success;
}

bool VistaAudio::RestoreEndpoint(const Endpoint& ep, bool wasMuted)
{
    WMLog& log = WMLog::GetInstance();

    log.LogInfo(L"Restoring: Mute {} for \"{}\"", wasMuted ? L"true" : L"false",
                ep.deviceName);
    if (wasMuted) {
        return true;
    }
    if (FAILED(ep.endpointVolume->SetMute(false, nullptr))) {
        log.LogError(L"Failed to restore mute status to false for \"{}\"",
                     ep.deviceName);
        return false;
    }
    return true;
}

bool VistaAudio::RestoreMuteStatus()
{
    bool success = true;
    WMLog& log = WMLog::GetInstance();

    if (!CheckForReInit()) {
        return false;
    }

    pendingRestoreIds_.clear();
    std::set<std::wstring> presentIds;
    for (auto& e : endpoints_) {
        presentIds.insert(e->deviceId);
        if (!IsEndpointManaged(*e)) {
            log.LogInfo(L"Skipping Endpoint {}", e->deviceName);
            continue;
        }
        const auto saved = savedMuteState_.find(e->deviceId);
        if (saved == savedMuteState_.end()) {
            // Appeared after the mute event; nothing was remembered for it.
            log.LogInfo(L"No saved mute state for \"{}\"; leaving it alone",
                        e->deviceName);
            continue;
        }
        if (!RestoreEndpoint(*e, saved->second)) {
            success = false;
        }
    }

    // Endpoints that were around when we muted but are gone now (a sleeping
    // monitor's HDMI/DisplayPort audio, for example) keep the mute flag
    // Windows persisted for them. Remember them so their reappearance within
    // the next minute still completes the restore.
    for (const auto& [deviceId, wasMuted] : savedMuteState_) {
        if (presentIds.count(deviceId) == 0 && !wasMuted) {
            pendingRestoreIds_.insert(deviceId);
        }
    }
    if (!pendingRestoreIds_.empty()) {
        restoreDeadline_ = std::chrono::steady_clock::now() +
                           LATE_RESTORE_WINDOW;
        log.LogInfo(
            L"{} endpoint(s) were not present at restore time."
            L" Waiting up to {}s for them to reappear",
            pendingRestoreIds_.size(), LATE_RESTORE_WINDOW.count());
    }

    return success;
}

void VistaAudio::RestoreArrivedEndpoints()
{
    WMLog& log = WMLog::GetInstance();

    if (pendingRestoreIds_.empty()) {
        return;
    }
    if (std::chrono::steady_clock::now() > restoreDeadline_) {
        log.LogInfo(
            L"Endpoint arrived, but the restore window has expired."
            L" Discarding {} pending restore(s)",
            pendingRestoreIds_.size());
        pendingRestoreIds_.clear();
        return;
    }
    if (!CheckForReInit()) {
        return;
    }
    for (auto& e : endpoints_) {
        if (pendingRestoreIds_.count(e->deviceId) == 0) {
            continue;
        }
        if (!IsEndpointManaged(*e)) {
            log.LogInfo(L"Skipping Endpoint {}", e->deviceName);
            pendingRestoreIds_.erase(e->deviceId);
            continue;
        }
        log.LogInfo(L"Endpoint \"{}\" reappeared after restore", e->deviceName);
        RestoreEndpoint(*e, false);
        pendingRestoreIds_.erase(e->deviceId);
    }
}

void VistaAudio::SetMute(bool mute)
{
    WMLog& log = WMLog::GetInstance();
    if (CheckForReInit()) {
        for (auto& e : endpoints_) {
            BOOL isMuted = !mute;
            if (!IsEndpointManaged(*e)) {
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
                                 mute ? L"true" : L"false", e->deviceName);
                }
            }
        }
    }
}

bool VistaAudio::IsEndpointManaged(const Endpoint& ep) const
{
    if (!muteSpecificEndpoints_) {
        return true;
    }

    // An entry that carries an id identifies exactly one device, so the
    // friendly name is ignored for it.
    const bool inManagedEndpoints =
        std::any_of(std::begin(managedEndpoints_), std::end(managedEndpoints_),
                    [&ep](const ManagedEndpoint& managed) {
                        return managed.id.empty()
                                   ? managed.name == ep.deviceName
                                   : managed.id == ep.deviceId;
                    });

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

void VistaAudio::SetManagedEndpoints(
    const std::vector<ManagedEndpoint>& endpoints, bool isAllowList)
{
    managedEndpoints_ = endpoints;
    muteSpecificEndpointsAllowList_ = isAllowList;
}
