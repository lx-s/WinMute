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

/* wParam = Connected = 1 | Disconnected = 0. lParam = Pointer to Wifi Name */
constexpr int WM_WIFISTATUSCHANGED = WM_USER + 400;

class WifiDetector {
public:
   WifiDetector() noexcept;
   ~WifiDetector();
   WifiDetector(const WifiDetector&) = delete;
   WifiDetector(WifiDetector&&) = delete;
   WifiDetector& operator=(const WifiDetector&) = delete;
   WifiDetector &operator=(WifiDetector&&) = delete;

   bool Init(HWND hNotifyWnd);
   void Unload() noexcept;
   void SetNetworkList(const std::vector<std::wstring>& networks, bool isMuteList);

   void CheckNetwork();
   void WlanNotificationCallback(PWLAN_NOTIFICATION_DATA notifyData);

private:
   HWND hNotifyWnd_;
   HANDLE wlanHandle_;
   // If "true" then networks_ contains all networks where the workstation should
   // be muted. If false, then networks_ contains all networks where the ws should
   // not be muted.
   bool isMuteList_;
   bool initialized_;
   std::vector<std::wstring> networks_;
};
