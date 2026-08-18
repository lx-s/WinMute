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

extern HINSTANCE hglobInstance;

static constexpr UINT_PTR QUIETHOURS_TIMER_START_ID = 271020;
static constexpr UINT_PTR QUIETHOURS_TIMER_END_ID = 271021;
static constexpr DWORD SECONDS_PER_DAY = 86400;

// Returns seconds since midnight for the current local time
static DWORD GetNowSeconds() noexcept
{
    SYSTEMTIME now;
    GetLocalTime(&now);
    return static_cast<DWORD>(now.wHour) * 3600 +
           static_cast<DWORD>(now.wMinute) * 60 +
           static_cast<DWORD>(now.wSecond);
}

// Returns true if t falls within [wStart, wEnd) on the circular 24-hour clock
static bool TimeInWindow(DWORD t, DWORD wStart, DWORD wEnd) noexcept
{
    if (wStart < wEnd) {
        return t >= wStart && t < wEnd;
    } else {  // wraps midnight
        return t >= wStart || t < wEnd;
    }
}

static VOID CALLBACK QuietHoursTimerProc(HWND hWnd, UINT /*msg*/, UINT_PTR id,
                                         DWORD /*msSinceSysStart*/) noexcept
{
    KillTimer(hWnd, id);
    if (id == QUIETHOURS_TIMER_START_ID) {
        SendMessageW(hWnd, WM_WINMUTE_QUIETHOURS_START, 0, 0);
    } else if (id == QUIETHOURS_TIMER_END_ID) {
        SendMessageW(hWnd, WM_WINMUTE_QUIETHOURS_END, 0, 0);
    }
}

// ---------------------------------------------------------------------------

QuietHoursTimer::QuietHoursTimer()
    : hParent_(nullptr),
      initialized_(false),
      enabled_(false),
      activeWindowIdx_(-1)
{
}

QuietHoursTimer::~QuietHoursTimer()
{
    if (initialized_ && enabled_) {
        KillTimer(hParent_, QUIETHOURS_TIMER_START_ID);
        KillTimer(hParent_, QUIETHOURS_TIMER_END_ID);
    }
}

// Returns the index of the window that currently contains the local time,
// or -1 if none is active.
int QuietHoursTimer::FindActiveWindowIdx() const
{
    const DWORD now = GetNowSeconds();
    for (int i = 0; i < static_cast<int>(windows_.size()); ++i) {
        if (TimeInWindow(now, windows_[i].first, windows_[i].second)) {
            return i;
        }
    }
    return -1;
}

// Returns the index of the window whose start time is soonest in the future
// (wrapping around midnight if necessary).
int QuietHoursTimer::FindNextWindowIdx() const
{
    if (windows_.empty())
        return -1;

    const DWORD now = GetNowSeconds();
    int bestIdx = -1;
    DWORD bestDiff = MAXDWORD;

    for (int i = 0; i < static_cast<int>(windows_.size()); ++i) {
        const DWORD s = windows_[i].first;
        const DWORD diff = (s >= now) ? (s - now) : (SECONDS_PER_DAY - now + s);
        if (diff < bestDiff) {
            bestDiff = diff;
            bestIdx = i;
        }
    }
    return bestIdx;
}

bool QuietHoursTimer::SetStart()
{
    KillTimer(hParent_, QUIETHOURS_TIMER_START_ID);

    if (!enabled_ || windows_.empty())
        return true;

    const int idx = FindNextWindowIdx();
    if (idx == -1)
        return true;

    activeWindowIdx_ = idx;

    const DWORD now = GetNowSeconds();
    const DWORD startSec = windows_[idx].first;
    const DWORD diffSec = (startSec >= now)
                              ? (startSec - now)
                              : (SECONDS_PER_DAY - now + startSec);

    // diffSec of 0 shouldn't happen here (caller checked FindActiveWindowIdx
    // first), but guard against it so the timer fires without an infinite-delay
    // edge case.
    const UINT msDelay = (diffSec > 0) ? (diffSec * 1000u) : 1u;

    if (SetTimer(hParent_, QUIETHOURS_TIMER_START_ID, msDelay,
                 QuietHoursTimerProc) == 0)
    {
        TaskDialog(hParent_, hglobInstance, PROGRAM_NAME,
                   WMi18n::GetInstance()
                       .GetTranslationW("popup.error.quiet-hours-start.title")
                       .c_str(),
                   WMi18n::GetInstance()
                       .GetTranslationW("popup.error.quiet-hours-start.text")
                       .c_str(),
                   TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
        return false;
    }

    WMLog::GetInstance().LogInfo(
        L"QuietHours: Next start scheduled in {} s (window {}, "
        L"{:02}:{:02}-{:02}:{:02})",
        diffSec, idx, windows_[idx].first / 3600,
        (windows_[idx].first % 3600) / 60, windows_[idx].second / 3600,
        (windows_[idx].second % 3600) / 60);

    return true;
}

bool QuietHoursTimer::SetEnd()
{
    KillTimer(hParent_, QUIETHOURS_TIMER_END_ID);

    if (!enabled_ || windows_.empty() || activeWindowIdx_ < 0)
        return true;

    const DWORD now = GetNowSeconds();
    const DWORD endSec = windows_[activeWindowIdx_].second;
    const DWORD diffSec =
        (endSec > now) ? (endSec - now) : (SECONDS_PER_DAY - now + endSec);

    if (diffSec == 0) {
        // End is right now — fire the message immediately instead of a 0-ms
        // timer
        SendMessageW(hParent_, WM_WINMUTE_QUIETHOURS_END, 0, 0);
        return true;
    }

    if (SetTimer(hParent_, QUIETHOURS_TIMER_END_ID, diffSec * 1000u,
                 QuietHoursTimerProc) == 0)
    {
        TaskDialog(hParent_, hglobInstance, PROGRAM_NAME,
                   WMi18n::GetInstance()
                       .GetTranslationW("popup.error.quiet-hours-stop.title")
                       .c_str(),
                   WMi18n::GetInstance()
                       .GetTranslationW("popup.error.quiet-hours-stop.text")
                       .c_str(),
                   TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
        return false;
    }

    WMLog::GetInstance().LogInfo(
        L"QuietHours: End scheduled in {} s (window {})", diffSec,
        activeWindowIdx_);

    return true;
}

bool QuietHoursTimer::IsQuietTime() const
{
    if (!enabled_ || windows_.empty())
        return false;
    return FindActiveWindowIdx() != -1;
}

bool QuietHoursTimer::LoadFromSettings(WMSettings& settings)
{
    KillTimer(hParent_, QUIETHOURS_TIMER_START_ID);
    KillTimer(hParent_, QUIETHOURS_TIMER_END_ID);
    windows_.clear();
    activeWindowIdx_ = -1;

    enabled_ = settings.QueryValue(SettingsKey::QUIETHOURS_ENABLE) != 0;
    if (!enabled_)
        return true;

    // Load all defined time windows and sort them by start time
    auto loaded = settings.GetQuietHoursTimes();
    for (const auto& t : loaded) {
        if (t.first != t.second) {  // skip degenerate entries
            windows_.push_back(t);
        }
    }
    if (windows_.empty())
        return true;

    std::sort(windows_.begin(), windows_.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    // Check if we are already inside a quiet-hours window at startup
    const int activeIdx = FindActiveWindowIdx();
    if (activeIdx != -1) {
        activeWindowIdx_ = activeIdx;
        WMLog::GetInstance().LogInfo(
            L"QuietHours: Currently inside window {} — muting now", activeIdx);
        // Sending START will cause WinMute to mute and call SetEnd()
        SendMessageW(hParent_, WM_WINMUTE_QUIETHOURS_START, 0, 0);
    } else {
        SetStart();
    }

    return true;
}

bool QuietHoursTimer::Init(HWND hParent, WMSettings& settings)
{
    if (initialized_)
        return true;
    hParent_ = hParent;
    initialized_ = true;
    return LoadFromSettings(settings);
}

bool QuietHoursTimer::Reset(WMSettings& settings)
{
    return LoadFromSettings(settings);
}
