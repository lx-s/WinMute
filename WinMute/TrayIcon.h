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

constexpr int WM_TRAYICON = WM_APP + 700;

struct TrayIconPopup {
   std::wstring title;
   std::wstring text;
};

class TrayIcon {
public:
   TrayIcon();
   TrayIcon(HWND hWnd, UINT trayID, HICON hIcon, const std::wstring& tooltip,
      bool show, int callbackId = WM_TRAYICON);
   ~TrayIcon();
   void Init(HWND hWnd, UINT trayID, HICON hIcon, const std::wstring& tooltip,
      bool show, int callbackId = WM_TRAYICON);
   void Hide();
   void Show();
   bool IsShown() const noexcept { return iconVisible_; }
   void ChangeIcon(HICON hNewIcon);
   void ChangeText(const std::wstring& tooltip);
   void ShowPopup(
      const std::wstring& title,
      const std::wstring& text) const;

private:
   void DestroyTrayIcon();
   bool AddNotifyIcon();
   bool RemoveNotifyIcon();
   bool ChangeText();


   mutable std::vector<TrayIconPopup> popupQueue_;

   int callbackId_ = WM_TRAYICON;
   bool initialized_ = false;
   bool iconVisible_ = false;
   UINT trayID_ = 0;
   HICON hIcon_ = 0;
   HWND hWnd_ = 0;
   std::wstring tooltip_;
};
