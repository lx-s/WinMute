/*
 WinMute
           Copyright (c) 2025, Alexander Steinhoefer

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

static constexpr int WM_WINMUTE_QUIETHOURS_START = WM_APP + 202;
static constexpr int WM_WINMUTE_QUIETHOURS_END = WM_APP + 203;

class QuietHoursTimer {
public:
   QuietHoursTimer();
   ~QuietHoursTimer();
   QuietHoursTimer(const QuietHoursTimer&) = delete;
   QuietHoursTimer& operator=(const QuietHoursTimer&) = delete;

   bool Init(HWND hParent, const WMSettings& settings);

   bool IsQuietTime();

   bool SetStart();
   bool SetEnd();

   bool Reset(const WMSettings& settings);

private:
   HWND hParent_;
   bool initialized_;
   bool enabled_;
   time_t qhStart_;
   time_t qhEnd_;

   bool LoadFromSettings(const WMSettings& settings);
};
