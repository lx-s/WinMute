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

#pragma once

#include "MMNotificationClient.h"
#include "VistaAudioSessionEvents.h"
#include "common.h"

class WinAudio {
   public:
    virtual bool Init(HWND hParent) = 0;
    virtual void ShouldReInit() = 0;
    virtual void OnAudioServiceShutdown() = 0;
    virtual void OnDeviceArrived() = 0;
    virtual bool AllEndpointsMuted() = 0;
    virtual bool SaveMuteStatus() = 0;
    virtual bool RestoreMuteStatus() = 0;
    virtual void RestoreArrivedEndpoints() = 0;
    virtual void SetMute(bool mute) = 0;
    virtual void MuteSpecificEndpoints(bool muteSpecific) = 0;
    virtual void SetManagedEndpoints(
        const std::vector<ManagedEndpoint>& endpoints, bool isAllowList) = 0;
    virtual ~WinAudio() noexcept {};
};

struct Endpoint {
    // Stable identity of the endpoint. The friendly name is for logging and
    // for the user-facing allow/block list only -- it is neither unique nor
    // guaranteed to survive a driver update.
    std::wstring deviceId;
    std::wstring deviceName;
    CComPtr<IAudioEndpointVolume> endpointVolume;
    CComPtr<IAudioSessionControl> sessionCtrl;
    // VistaAudioSessionEvents is a ref-counted COM object; holding it in a
    // CComPtr keeps its lifetime tied to the COM refcount instead of fighting
    // it with a second owner.
    CComPtr<VistaAudioSessionEvents> wasapiAudioEvents;

    Endpoint() = default;
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
    void OnAudioServiceShutdown() override;
    void OnDeviceArrived() override;
    bool AllEndpointsMuted() override;
    bool SaveMuteStatus() override;
    bool RestoreMuteStatus() override;
    void RestoreArrivedEndpoints() override;
    void SetMute(bool mute) override;

    void MuteSpecificEndpoints(bool muteSpecific) override;
    void SetManagedEndpoints(const std::vector<ManagedEndpoint>& endpoints,
                             bool isAllowList) override;

   private:
    void Uninit();
    bool CheckForReInit();

    bool LoadAllEndpoints();
    bool IsEndpointManaged(const Endpoint& ep) const;
    bool RestoreEndpoint(const Endpoint& ep, bool wasMuted);

    std::vector<std::unique_ptr<Endpoint>> endpoints_;
    CComPtr<MMNotificationClient> mmnAudioEvents_;
    CComPtr<IMMDeviceEnumerator> deviceEnumerator_;

    // Saved mute state lives here, not on Endpoint: the endpoint objects are
    // torn down and rebuilt on every re-init, which would otherwise discard
    // the state saved just before a mute event (or between save and mute).
    std::map<std::wstring, bool> savedMuteState_;

    // Endpoints that were saved but absent when the restore came in, plus the
    // deadline until which their late arrival still triggers a restore.
    std::set<std::wstring> pendingRestoreIds_;
    std::chrono::steady_clock::time_point restoreDeadline_{};

    std::atomic<bool> reInit_;
    bool muteSpecificEndpoints_;
    bool muteSpecificEndpointsAllowList_;
    HWND hParent_;

    std::vector<ManagedEndpoint> managedEndpoints_;

    // non copy-able
    VistaAudio(const VistaAudio& other) = delete;
    VistaAudio& operator=(const VistaAudio& other) = delete;
};
