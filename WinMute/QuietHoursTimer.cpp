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

extern HINSTANCE hglobInstance;

static constexpr int QUIETHOURS_TIMER_START_ID = 271020;
static constexpr int QUIETHOURS_TIMER_END_ID = 271021;

static std::int64_t ConvertSystemTimeTo100NS(const LPSYSTEMTIME sysTime) noexcept
{
   FILETIME ft;
   SystemTimeToFileTime(sysTime, &ft);
   ULARGE_INTEGER ulInt;
   ulInt.HighPart = ft.dwHighDateTime;
   ulInt.LowPart = ft.dwLowDateTime;
   return static_cast<std::int64_t>(ulInt.QuadPart);
}

static bool IsQuietHours(
   const LPSYSTEMTIME now,
   const LPSYSTEMTIME qhStart,
   const LPSYSTEMTIME qhEnd)
{
   const std::int64_t iStart = ConvertSystemTimeTo100NS(qhStart);
   std::int64_t iEnd = ConvertSystemTimeTo100NS(qhEnd);
   const std::int64_t iNow = ConvertSystemTimeTo100NS(now);

   if (iEnd < iStart) {
      // Add one day
      iEnd += 864000000000ll;
   }

   if (iEnd - iNow > 0 && iStart - iNow < 0) {
      return true;
   }

   return false;
}

/**
 * Gets the number of milliseconds that t2 would have to add, to reach t1.
 * Takes day wrap around into consideration.
 */
static int GetDiffMillseconds(const LPSYSTEMTIME t1, const LPSYSTEMTIME t2)
{
   __int64 it1 = ConvertSystemTimeTo100NS(t1);
   __int64 it2 = ConvertSystemTimeTo100NS(t2);

   // 100NS to Milliseconds;
   it1 /= 10000;
   it2 /= 10000;
   if (it1 < it2) { // add one day wrap around
      it1 += 24 * 60 * 60 * 1000;
   }
   __int64 res = it1 - it2;

   return static_cast<int>(res);
}

static VOID CALLBACK QuietHoursTimerProc(HWND hWnd, UINT msg, UINT_PTR id, DWORD msSinceSysStart) noexcept
{
   UNREFERENCED_PARAMETER(msg);
   UNREFERENCED_PARAMETER(msSinceSysStart);
   if (id == QUIETHOURS_TIMER_START_ID) {
      KillTimer(hWnd, id);
      SendMessageW(hWnd, WM_WINMUTE_QUIETHOURS_START, 0, 0);
   }
   else if (id == QUIETHOURS_TIMER_END_ID) {
      KillTimer(hWnd, id);
      SendMessageW(hWnd, WM_WINMUTE_QUIETHOURS_END, 0, 0);
   }
}

QuietHoursTimer::QuietHoursTimer() :
   initialized_{ false },
   enabled_{ false },
   hParent_{ nullptr },
   qhStart_{0},
   qhEnd_{ 0 }
{
}

QuietHoursTimer::~QuietHoursTimer()
{
   if (initialized_ && enabled_) {
      KillTimer(hParent_, QUIETHOURS_TIMER_START_ID);
      KillTimer(hParent_, QUIETHOURS_TIMER_END_ID);
   }
}

bool QuietHoursTimer::LoadFromSettings(const WMSettings& settings)
{
   KillTimer(hParent_, QUIETHOURS_TIMER_START_ID);
   KillTimer(hParent_, QUIETHOURS_TIMER_END_ID);

   if (!settings.QueryValue(SettingsKey::QUIETHOURS_ENABLE)) {
      enabled_ = false;
      return true;
   }

   qhStart_ = settings.QueryValue(SettingsKey::QUIETHOURS_START);
   qhEnd_ = settings.QueryValue(SettingsKey::QUIETHOURS_END);

   SYSTEMTIME now;
   SYSTEMTIME start;
   SYSTEMTIME end;
   GetLocalTime(&now);
   GetLocalTime(&start);
   GetLocalTime(&end);

   start.wSecond = static_cast<WORD>(qhStart_ % 60); 
   start.wMinute = static_cast<WORD>(((qhStart_ - start.wSecond) / 60) % 60);
   start.wHour = static_cast<WORD>((qhStart_ - start.wMinute - start.wSecond) / 3600);

   end.wSecond = static_cast<WORD>(qhEnd_ % 60);
   end.wMinute = static_cast<WORD>(((qhEnd_ - end.wSecond) / 60) % 60);
   end.wHour = static_cast<WORD>((qhEnd_ - end.wMinute - end.wSecond) / 3600);

   if (IsQuietHours(&now, &start, &end)) {
      WMLog::GetInstance().Write(L"Mute: On | Quiet hours have already started");
      SendMessage(hParent_, WM_WINMUTE_QUIETHOURS_START, 0, 0);
   } else {
      int timerQhStart = GetDiffMillseconds(&start, &now);
      if (SetTimer(
            hParent_,
            QUIETHOURS_TIMER_START_ID,
            timerQhStart,
            QuietHoursTimerProc) == 0) {
         TaskDialog(
            hParent_,
            hglobInstance,
            PROGRAM_NAME,
            WMi18n::GetInstance().GetTextW(IDS_POPUP_QUIET_HOURS_START_ERROR_TITLE).c_str(),
            WMi18n::GetInstance().GetTextW(IDS_POPUP_QUIET_HOURS_START_ERROR_TEXT).c_str(),
            TDCBF_OK_BUTTON,
            TD_ERROR_ICON,
            nullptr);
         return false;
      }
   }
   return true;
}

bool QuietHoursTimer::SetStart()
{
   SYSTEMTIME now;
   SYSTEMTIME start;

   GetLocalTime(&now);
   GetLocalTime(&start);

   start.wSecond = static_cast<WORD>(qhStart_ % 60);
   start.wMinute = static_cast<WORD>(((qhStart_ - start.wSecond) / 60) % 60);
   start.wHour = static_cast<WORD>((qhStart_ - start.wMinute - start.wSecond) / 3600);

   const int timerQhStart = GetDiffMillseconds(&start, &now);
   if (SetTimer(
         hParent_,
         QUIETHOURS_TIMER_START_ID,
         timerQhStart,
         QuietHoursTimerProc) == 0) {
      TaskDialog(
         hParent_,
         hglobInstance,
         PROGRAM_NAME,
         WMi18n::GetInstance().GetTextW(IDS_POPUP_QUIET_HOURS_START_ERROR_TITLE).c_str(),
         WMi18n::GetInstance().GetTextW(IDS_POPUP_QUIET_HOURS_START_ERROR_TEXT).c_str(),
         TDCBF_OK_BUTTON,
         TD_ERROR_ICON,
         nullptr);
      return false;
   }
   return true;
}

bool QuietHoursTimer::SetEnd()
{
   SYSTEMTIME now;
   SYSTEMTIME end;

   GetLocalTime(&now);
   GetLocalTime(&end);

   end.wSecond = static_cast<WORD>(qhEnd_ % 60);
   end.wMinute = static_cast<WORD>(((qhEnd_ - end.wSecond) / 60) % 60);
   end.wHour = static_cast<WORD>((qhEnd_ - end.wMinute - end.wSecond) / 3600);

   int timerQhEnd = GetDiffMillseconds(&end, &now);
   if (timerQhEnd <= 0) {
      SendMessage(hParent_, WM_WINMUTE_QUIETHOURS_END, 0, 0);
   } else if (SetTimer(
         hParent_,
         QUIETHOURS_TIMER_END_ID,
         timerQhEnd,
         QuietHoursTimerProc) == 0) {
      TaskDialog(
         hParent_,
         hglobInstance,
         PROGRAM_NAME,
         WMi18n::GetInstance().GetTextW(IDS_POPUP_QUIET_HOURS_STOP_ERROR_TITLE).c_str(),
         WMi18n::GetInstance().GetTextW(IDS_POPUP_QUIET_HOURS_STOP_ERROR_TEXT).c_str(),
         TDCBF_OK_BUTTON,
         TD_ERROR_ICON,
         nullptr);
      return false;
   }
   return true;
}

bool QuietHoursTimer::Init(HWND hParent, const WMSettings& settings)
{
   if (initialized_) {
      return true;
   }
   hParent_ = hParent;
   initialized_ = true;
   return LoadFromSettings(settings);
}

bool QuietHoursTimer::IsQuietTime()
{
   SYSTEMTIME now;
   SYSTEMTIME start;
   SYSTEMTIME end;
   GetLocalTime(&now);
   GetLocalTime(&start);
   GetLocalTime(&end);
   return IsQuietHours(&now, &start, &end);
}

bool QuietHoursTimer::Reset(const WMSettings& settings)
{
   return LoadFromSettings(settings);
}


