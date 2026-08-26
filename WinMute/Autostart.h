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

#include "common.h"

// TaskId of the <uap5:StartupTask> element in
// Dist/msix-package/AppxManifest.xml. Both must stay in sync; a mismatch makes
// StartupTask::GetAsync fail with "element not found".
inline constexpr wchar_t STARTUP_TASK_ID[] = L"WinMuteStartup";

enum class AutostartState {
    Disabled,
    Enabled,
    // Switched off by the user in the Task Manager "Startup apps" tab. Windows
    // does not let an app override that choice; only the user can undo it.
    DisabledByUser,
    // Pinned by group policy. WinMute cannot change it in either direction.
    DisabledByPolicy,
    EnabledByPolicy,
    // The state could not be determined.
    Unknown
};

// True when WinMute runs from an MSIX package (Store or sideloaded). The answer
// cannot change while the process lives, so it is determined once and cached.
bool IsPackagedApp();

// Autostart for the packaged build, backed by the manifest's StartupTask
// instead of the "Run" registry key. Only meaningful when IsPackagedApp() is
// true. Both calls are safe to make from the (STA) UI thread; they do their
// blocking WinRT work on a worker thread and return the resulting state.
AutostartState GetPackagedAutostartState();
AutostartState SetPackagedAutostart(bool enable);
