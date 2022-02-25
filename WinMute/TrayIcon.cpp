/*
 WinMute
           Copyright (c) 2022, Alexander Steinhoefer

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

TrayIcon::TrayIcon() :
   iconVisible_(false), trayID_(0), hIcon_(0), hWnd_(0)
{
}

TrayIcon::TrayIcon(HWND hWnd, UINT trayID, HICON hIcon,
   const std::wstring& tooltip, bool show) :
   iconVisible_(false)
{
   Init(hWnd, trayID, hIcon, tooltip, show);
}

void TrayIcon::Init(HWND hWnd, UINT trayID, HICON hIcon,
   const std::wstring& tooltip, bool show)
{
   if (!hIcon) {
      hIcon_ = LoadIcon(0, IDI_APPLICATION);
   } else {
      hIcon_ = hIcon;
   }

   tooltip_ = tooltip;
   trayID_ = trayID;
   hWnd_ = hWnd;

   if (show) {
      Show();
   }
}

TrayIcon::~TrayIcon()
{
   Hide();
   DestroyTrayIcon();
}

void TrayIcon::Hide()
{
   if (iconVisible_) {
      if (RemoveNotifyIcon()) {
         iconVisible_ = false;
      }
   }
}

void TrayIcon::Show()
{
   if (!iconVisible_) {
      if (AddNotifyIcon()) {
         iconVisible_ = true;
      }
   }
}

void TrayIcon::ChangeIcon(HICON hNewIcon)
{
   if (!hNewIcon) {
      return;
   }
   bool oldIconVisible = iconVisible_;

   DestroyTrayIcon();
   hIcon_ = hNewIcon;

   if (oldIconVisible) {
      NOTIFYICONDATA tnid;

      tnid.cbSize = sizeof(NOTIFYICONDATA);
      tnid.hWnd = hWnd_;
      tnid.uID = trayID_;
      tnid.hIcon = hIcon_;
      tnid.uFlags = NIF_ICON;

      if (!Shell_NotifyIcon(NIM_MODIFY, &tnid))
         return;

      iconVisible_ = true;
   }

   return;
}

void TrayIcon::ChangeText(const std::wstring& tooltip)
{
   tooltip_ = tooltip;

   if (iconVisible_)
      ChangeText();
}

void TrayIcon::ShowPopup(
   const std::wstring& title,
   const std::wstring& text)
{
   NOTIFYICONDATA tnid;
   tnid.cbSize = sizeof(NOTIFYICONDATA);
   tnid.hWnd = hWnd_;
   tnid.uID = trayID_;
   tnid.uFlags = NIF_INFO;
   tnid.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND | NIIF_LARGE_ICON
      | NIIF_RESPECT_QUIET_TIME;
   tnid.uTimeout = 10 * 1000;
   wcscpy_s(tnid.szInfoTitle, sizeof(tnid.szInfoTitle) / sizeof(TCHAR), title.c_str());
   wcscpy_s(tnid.szInfo, sizeof(tnid.szInfo) / sizeof(TCHAR), text.c_str());

   Shell_NotifyIcon(NIM_MODIFY, &tnid);
}

void TrayIcon::DestroyTrayIcon()
{
   if (hIcon_) {
      DestroyIcon(hIcon_);
      hIcon_ = 0;
   }
}

bool TrayIcon::AddNotifyIcon()
{
   NOTIFYICONDATA tnid;

   tnid.cbSize = sizeof(NOTIFYICONDATA);
   tnid.hWnd = hWnd_;
   tnid.uID = trayID_;
   tnid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
   tnid.hIcon = hIcon_;
   tnid.uCallbackMessage = WM_TRAYICON;

   if (SUCCEEDED(StringCchCopy(tnid.szTip,
      sizeof(tnid.szTip) / sizeof(TCHAR),
      tooltip_.c_str()))) {
      return Shell_NotifyIcon(NIM_ADD, &tnid) != 0;
   }
   return false;
}

bool TrayIcon::RemoveNotifyIcon()
{
   NOTIFYICONDATA tnid;

   tnid.cbSize = sizeof(NOTIFYICONDATA);
   tnid.hWnd = hWnd_;
   tnid.uID = trayID_;

   return Shell_NotifyIcon(NIM_DELETE, &tnid) != 0;
}

bool TrayIcon::ChangeText()
{
   NOTIFYICONDATA tnid;

   tnid.cbSize = sizeof(NOTIFYICONDATA);
   tnid.hWnd = hWnd_;
   tnid.uID = trayID_;
   tnid.uFlags = NIF_TIP;

   if (SUCCEEDED(StringCchCopy(tnid.szTip,
      sizeof(tnid.szTip) / sizeof(TCHAR),
      tooltip_.c_str()))) {
      return Shell_NotifyIcon(NIM_MODIFY, &tnid) != 0;
   }
   return false;
}


