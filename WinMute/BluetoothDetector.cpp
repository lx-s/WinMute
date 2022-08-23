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

BluetoothDetector::BluetoothDetector() :
   hNotifyWnd_(NULL),  hBluetoothNotify_(NULL),
   initialized_(false), useDeviceList_(false)
{

}

BluetoothDetector::~BluetoothDetector()
{
   if (initialized_) {
      UnloadRadioNotifications();
   }
}

void BluetoothDetector::UnloadRadioNotifications()
{
   for (auto hDevNotify : notificationHandles_) {
      UnregisterDeviceNotification(hDevNotify);
   }
   notificationHandles_.clear();
}

bool BluetoothDetector::LoadRadioNotifications()
{
   bool success = false;
   auto& log = WMLog::GetInstance();
   notificationHandles_.clear();

   HANDLE btHandle;
   BLUETOOTH_FIND_RADIO_PARAMS bfrp = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };
   HBLUETOOTH_RADIO_FIND hRadiosFind = BluetoothFindFirstRadio(&bfrp, &btHandle);
   if (hRadiosFind == nullptr) {
      log.WriteWindowsError(L"BluetoothFindFirstRadio", GetLastError());
      return false;
   }

   DEV_BROADCAST_HANDLE dbh = { 0 };
   dbh.dbch_devicetype = DBT_DEVTYP_HANDLE;
   dbh.dbch_size = sizeof(dbh);
   dbh.dbch_eventguid = GUID_BLUETOOTH_RADIO_IN_RANGE;

   do {
      dbh.dbch_handle = btHandle;
      HDEVNOTIFY devNotify = RegisterDeviceNotification(
         hNotifyWnd_,
         &dbh,
         DEVICE_NOTIFY_WINDOW_HANDLE);
      CloseHandle(btHandle);

      if (devNotify == NULL) {
         log.WriteWindowsError(L"RegisterDeviceNotification", GetLastError());
      } else {
         notificationHandles_.push_back(devNotify);
      }
   } while (BluetoothFindNextRadio(hRadiosFind, &btHandle));
   DWORD dwLastError = GetLastError();
   if (dwLastError != ERROR_NO_MORE_ITEMS) {
      log.WriteWindowsError(L"BluetoothFindNextRadio", GetLastError());
      UnloadRadioNotifications();
   } else {
      success = true;
   }

   BluetoothFindRadioClose(hRadiosFind);
   return success;
}

bool BluetoothDetector::Init(HWND hNotifyWnd)
{
   if (!initialized_) {
      hNotifyWnd_ = hNotifyWnd;
      if (LoadRadioNotifications()) {
         initialized_ = true;
      }
   }
   return initialized_;
}

void BluetoothDetector::Unload()
{
   UnloadRadioNotifications();
   hNotifyWnd_ = NULL;
   initialized_ = false;
}

void BluetoothDetector::SetDeviceList(
   const std::vector<std::string>& devices,
   bool useDeviceList)
{
   deviceNames_ = devices;
   useDeviceList_ = useDeviceList;
}

BluetoothDetector::BluetoothStatus BluetoothDetector::GetBluetoothStatus(
   const UINT message, const WPARAM wParam, const LPARAM lParam)
{
   if (message != WM_DEVICECHANGE || wParam != DBT_CUSTOMEVENT || lParam == 0) {
      return BluetoothStatus::Unknown;
   }

   const DEV_BROADCAST_HDR *header = reinterpret_cast<DEV_BROADCAST_HDR*>(lParam);
   if (header->dbch_devicetype != DBT_DEVTYP_HANDLE) {
      return BluetoothStatus::Unknown;
   }

   const DEV_BROADCAST_HANDLE* handle = reinterpret_cast<const DEV_BROADCAST_HANDLE*>(header);
   if (!IsEqualGUID(handle->dbch_eventguid, GUID_BLUETOOTH_RADIO_IN_RANGE)) {
      return BluetoothStatus::Unknown;
   }

   const BTH_RADIO_IN_RANGE *inRangeInfo = reinterpret_cast<const BTH_RADIO_IN_RANGE*>(handle->dbch_data);
   if (inRangeInfo == nullptr) {
      return BluetoothStatus::Unknown;
   }

   // only react to audio devices and not (e. g.) game controllers
   if ((inRangeInfo->deviceInfo.flags & BDIF_COD) &&
       GET_COD_MAJOR(inRangeInfo->deviceInfo.classOfDevice) != COD_MAJOR_AUDIO) {
      return BluetoothStatus::Unknown;
   }

   auto& log = WMLog::GetInstance();
   if (useDeviceList_) {
      auto pos = std::find(begin(deviceNames_), end(deviceNames_), inRangeInfo->deviceInfo.name);
      if (pos == end(deviceNames_)) {
         log.Write(L"Bluetooth device \"S\" not in list.", inRangeInfo->deviceInfo.name);
         return BluetoothStatus::Unknown;
      }
   }
   if ((inRangeInfo->deviceInfo.flags & BDIF_CONNECTED) && !(inRangeInfo->previousDeviceFlags & BDIF_CONNECTED)) {
      log.Write(L"Bluetooth Audio device \"%S\" connected.", inRangeInfo->deviceInfo.name);
      return BluetoothStatus::Connected;
   } else if (!(inRangeInfo->deviceInfo.flags & BDIF_CONNECTED) && (inRangeInfo->previousDeviceFlags & BDIF_CONNECTED)) {
      log.Write(L"Bluetooth Audio device \"%S\" disconnected.", inRangeInfo->deviceInfo.name);
      return BluetoothStatus::Disconnected;
   }
   return BluetoothStatus::Unknown;
}
