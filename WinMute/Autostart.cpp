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

// Autostart for the packaged (MSIX / Microsoft Store) build.
//
// A packaged WinMute must not use the "Run" registry key: it is installed into
// a versioned directory below WindowsApps that changes with every update, so a
// stored path goes stale as soon as the app is updated, and the entry survives
// uninstall. The supported mechanism is the <uap5:StartupTask> declared in the
// package manifest, which Windows resolves by TaskId and surfaces in the Task
// Manager "Startup apps" tab.

#include <appmodel.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.h>

#include "common.h"

using namespace winrt::Windows::ApplicationModel;

namespace {

AutostartState ToAutostartState(StartupTaskState state) noexcept
{
    switch (state) {
        case StartupTaskState::Enabled:
            return AutostartState::Enabled;
        case StartupTaskState::EnabledByPolicy:
            return AutostartState::EnabledByPolicy;
        case StartupTaskState::Disabled:
            return AutostartState::Disabled;
        case StartupTaskState::DisabledByUser:
            return AutostartState::DisabledByUser;
        case StartupTaskState::DisabledByPolicy:
            return AutostartState::DisabledByPolicy;
        default:
            return AutostartState::Unknown;
    }
}

const wchar_t* ToString(AutostartState state) noexcept
{
    switch (state) {
        case AutostartState::Disabled:
            return L"Disabled";
        case AutostartState::Enabled:
            return L"Enabled";
        case AutostartState::DisabledByUser:
            return L"DisabledByUser";
        case AutostartState::DisabledByPolicy:
            return L"DisabledByPolicy";
        case AutostartState::EnabledByPolicy:
            return L"EnabledByPolicy";
        default:
            return L"Unknown";
    }
}

// The StartupTask calls block on an async operation. Doing that on the STA UI
// thread risks a deadlock (and trips an assert inside C++/WinRT), so the work
// joins the MTA on a worker thread.
template <typename Fn>
AutostartState RunOnMta(Fn fn)
{
    AutostartState result = AutostartState::Unknown;
    std::thread worker([&result, &fn]() {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        try {
            result = fn();
        } catch (const winrt::hresult_error& e) {
            WMLog::GetInstance().LogError(
                L"Autostart: StartupTask \"{}\" failed with 0x{:08x}: {}",
                STARTUP_TASK_ID, static_cast<uint32_t>(e.code().value),
                e.message());
        }
        winrt::uninit_apartment();
    });
    worker.join();
    return result;
}

}  // namespace

bool IsPackagedApp()
{
    static const bool isPackaged = []() {
        UINT32 fullNameLength = 0;
        // Querying with a zero-sized buffer only reports whether this process
        // has package identity; the name itself is of no interest here.
        const LONG rc = GetCurrentPackageFullName(&fullNameLength, nullptr);
        return rc != APPMODEL_ERROR_NO_PACKAGE;
    }();
    return isPackaged;
}

AutostartState GetPackagedAutostartState()
{
    return RunOnMta([]() {
        const auto task = StartupTask::GetAsync(STARTUP_TASK_ID).get();
        return ToAutostartState(task.State());
    });
}

AutostartState SetPackagedAutostart(bool enable)
{
    const AutostartState newState = RunOnMta([enable]() {
        const auto task = StartupTask::GetAsync(STARTUP_TASK_ID).get();
        const StartupTaskState state = task.State();
        if (state == StartupTaskState::EnabledByPolicy ||
            state == StartupTaskState::DisabledByPolicy)
        {
            // Group policy owns the setting; enabling or disabling would throw.
            return ToAutostartState(state);
        }
        if (enable) {
            if (state == StartupTaskState::Enabled) {
                return AutostartState::Enabled;
            }
            // Unlike a UWP app, a packaged desktop app gets no consent dialog
            // here, so this does not need to run on a UI thread. The request is
            // silently refused with DisabledByUser if the user turned the entry
            // off in Task Manager.
            return ToAutostartState(task.RequestEnableAsync().get());
        }
        if (state == StartupTaskState::Enabled) {
            task.Disable();
        }
        return ToAutostartState(task.State());
    });

    WMLog::GetInstance().LogInfo(
        L"Autostart: requested {} for StartupTask \"{}\", resulting state: {}",
        enable ? L"enable" : L"disable", STARTUP_TASK_ID, ToString(newState));

    return newState;
}
