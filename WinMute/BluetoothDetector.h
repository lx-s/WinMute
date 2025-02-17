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

/* wParam = Connected = 1 | Disconnected = 0. lParam = Pointer to Device Name */
constexpr int WM_BTSTATUSCHANGED = WM_USER + 500;

class BluetoothDetector {
public:
   enum class BluetoothStatus { Unknown, Connected, Disconnected };
   BluetoothDetector();
   ~BluetoothDetector();
   BluetoothDetector(const WifiDetector&) = delete;
   BluetoothDetector(WifiDetector&&) = delete;
   BluetoothDetector& operator=(const WifiDetector&) = delete;
   BluetoothDetector& operator=(WifiDetector&&) = delete;

   void SetDeviceList(const std::vector<std::string>& devices, bool useDeviceList);

   bool Init(HWND hNotifyWnd);
   void Unload();

   BluetoothStatus GetBluetoothStatus(
      const UINT message, const WPARAM wParam, const LPARAM lParam);

private:
   HWND hNotifyWnd_;
   HDEVNOTIFY hBluetoothNotify_;

   std::vector<HDEVNOTIFY> notificationHandles_;

   bool LoadRadioNotifications();
   void UnloadRadioNotifications() noexcept;

   bool initialized_;
   bool useDeviceList_;
   std::vector<std::string> deviceNames_;
};
