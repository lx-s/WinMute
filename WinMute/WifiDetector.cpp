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

#pragma once

#include "common.h"

static void WlanNotificationCallback(
   PWLAN_NOTIFICATION_DATA unnamedParam1,
   PVOID unnamedParam2)
{
   WifiDetector* wifiDetector = reinterpret_cast<WifiDetector*>(unnamedParam2);
   if (wifiDetector != nullptr) {
      wifiDetector->WlanNotificationCallback(unnamedParam1);
   }
}


WifiDetector::WifiDetector()
   : hNotifyWnd_(NULL), wlanHandle_(NULL), isMuteList_(true), initialized_(false)
{
}

WifiDetector::~WifiDetector()
{
}

void WifiDetector::Unload()
{
   if (initialized_) {
      WlanRegisterNotification(wlanHandle_, WLAN_NOTIFICATION_SOURCE_NONE,
         TRUE, NULL, NULL, NULL, NULL);
      WlanCloseHandle(wlanHandle_, NULL);
      initialized_ = false;
      hNotifyWnd_ = NULL;
   }
}

bool WifiDetector::Init(HWND hNotifyWnd)
{
   if (!initialized_) {
      DWORD vers = 2;
      hNotifyWnd_ = hNotifyWnd;
      DWORD wlErr = WlanOpenHandle(vers, NULL, &vers, &wlanHandle_);
      if (wlErr != ERROR_SUCCESS) {
         PrintWindowsError(_T("WlanOpenHandle"), wlErr);
      } else {
         wlErr = WlanRegisterNotification(
            wlanHandle_, WLAN_NOTIFICATION_SOURCE_ACM, TRUE,
            ::WlanNotificationCallback, this, NULL, NULL);
         if (wlErr != ERROR_SUCCESS) {
            PrintWindowsError(_T("WlanOpenHandle"), wlErr);
            WlanCloseHandle(wlanHandle_, NULL);
         } else {
            initialized_ = true;
         }
      }
   }
   return initialized_;
}

void WifiDetector::SetNetworkList(const std::vector<tstring> networks, bool isMuteList)
{
   networks_ = networks;
   isMuteList_ = isMuteList;
}

void WifiDetector::WlanNotificationCallback(PWLAN_NOTIFICATION_DATA notifyData)
{
   if (notifyData->NotificationSource != WLAN_NOTIFICATION_SOURCE_ACM) {
      return;
   }
   if (notifyData->NotificationCode == wlan_notification_acm_connection_complete
       || notifyData->NotificationCode == wlan_notification_acm_disconnected) {
      bool connected = notifyData->NotificationCode == wlan_notification_acm_connection_complete;
      WLAN_CONNECTION_NOTIFICATION_DATA *wcnd =
         reinterpret_cast<WLAN_CONNECTION_NOTIFICATION_DATA*>(notifyData->pData);
      const auto it = std::find(std::begin(networks_), std::end(networks_),
                                wcnd->strProfileName);
      if (isMuteList_ && it != std::end(networks_)) {
         SendMessage(hNotifyWnd_, WM_WIFISTATUSCHANGED, connected,
                      reinterpret_cast<LPARAM>((*it).c_str()));
      } else if (!isMuteList_ && it == std::end(networks_)) {
         SendMessage(hNotifyWnd_, WM_WIFISTATUSCHANGED, connected, NULL);
      }
   }
}
