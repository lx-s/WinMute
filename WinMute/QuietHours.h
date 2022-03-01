#pragma once

#include "common.h"

class QuietHours {
   HWND hTimer;
public:
   QuietHours();
   ~QuietHours();
   QuietHours(const QuietHours&) = delete;
   QuietHours& operator=(const QuietHours&) = delete;

   bool IsQuietTime();

   void HandleTimerProc(HWND hWnd, UINT msg, UINT_PTR id, DWORD msSinceSysStart);

   bool SetStartTime();
   bool SetEndTime();
   bool ShowNotifications(bool show);
   bool ForceUnmute(bool force);
   bool Enable();

private:
   SYSTEMTIME start;
   SYSTEMTIME end;
};
