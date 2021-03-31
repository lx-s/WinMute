/*
 WinMute
           Copyright (c) 2021, Alexander Steinhoefer

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

#include "ScreensaverNotifierProxy.h"

extern void PrintWindowsError(const char* lpszFunction, DWORD lastError = -1);

ScreensaverNotifierProxy::ScreensaverNotifierProxy() :
   isRegistered_(false),
   hookDll_(nullptr),
   hNotifyWnd_(nullptr),
   notifyWndMsg_(0),
   regHook_(nullptr),
   unregHook_(nullptr)
{ }

ScreensaverNotifierProxy::~ScreensaverNotifierProxy()
{
   Unload();
}

bool ScreensaverNotifierProxy::Init(int msgId)
{
   if (hookDll_ != nullptr) {
      return true;
   }
   notifyWndMsg_ = msgId;
   return InitNotifyWnd() && InitHookDll() && ActivateNotifications();
}

void ScreensaverNotifierProxy::Unload()
{
   if (hookDll_) {
      if (isRegistered_ && unregHook_) {
         unregHook_();
      }

      FreeLibrary(hookDll_);
      regHook_ = nullptr;
      unregHook_ = nullptr;
      hookDll_ = nullptr;
   }
}

bool ScreensaverNotifierProxy::InitHookDll()
{
   if (hookDll_ == nullptr) {
      hookDll_ = LoadLibrary("ScreensaverNotify32");
      if (hookDll_ == nullptr) {
         PrintWindowsError("LoadLibrary");
      } else {
         regHook_ = reinterpret_cast<RegisterHook>(
            GetProcAddress(hookDll_, "RegisterScreensaverHook"));
         unregHook_ = reinterpret_cast<UnregisterHook>(
            GetProcAddress(hookDll_, "UnregisterScreensaverHook"));
         if (regHook_ == nullptr || unregHook_ == nullptr) {
            MessageBox(0, "MISSING", "", 0);
            Unload();
         }
      }
   }
   return hookDll_ != nullptr;
}

bool ScreensaverNotifierProxy::InitNotifyWnd()
{
   hNotifyWnd_ = FindWindowEx(0, 0, "LXS_WinMuteScreensaverNotifyClass", nullptr);
   if (hNotifyWnd_ == nullptr) {
      auto lastError = GetLastError();
      PrintWindowsError("FindWindowEx", lastError);
   }
   return hNotifyWnd_ != nullptr;
}

bool ScreensaverNotifierProxy::ActivateNotifications()
{
   if (regHook_ != nullptr) {
      if (regHook_(hNotifyWnd_, notifyWndMsg_)) {
         isRegistered_ = true;
         return true;
      }
   }
   return false;
}
