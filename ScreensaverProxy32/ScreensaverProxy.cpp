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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <shellapi.h>
#include <strsafe.h>
#include <commctrl.h>
#include <string>

#include "ScreensaverNotifierProxy.h"

static HINSTANCE hGlobInstance_ = nullptr;
static int notifyMsgId_ = 0;

void PrintWindowsError(const char* lpszFunction, DWORD lastError = -1);

void PrintWindowsError(const char* lpszFunction, DWORD lastError)
{
   // Retrieve the system error message for the last-error code
   if (lastError == -1) {
      lastError = GetLastError();
   }

   LPVOID lpMsgBuf;
   if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, lastError,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr) != 0) {
      size_t displayBufSize =
         ((size_t)lstrlen(static_cast<LPCTSTR>(lpMsgBuf)) +
            (size_t)lstrlen(static_cast<LPCTSTR>(lpszFunction))
            + 40) * sizeof(TCHAR);
      // Display the error message and exit the process
      LPVOID lpDisplayBuf = reinterpret_cast<LPVOID>(
         LocalAlloc(LMEM_ZEROINIT, displayBufSize));
      if (lpDisplayBuf) {
         StringCchPrintf((LPTSTR)lpDisplayBuf,
            LocalSize(lpDisplayBuf),
            _T("%s failed with error %u: %s"),
            lpszFunction,
            lastError,
            reinterpret_cast<TCHAR*>(lpMsgBuf));
         MessageBox(nullptr, static_cast<LPCTSTR>(lpDisplayBuf),
                   "WinMute - ScreensaverProxy", MB_OK | MB_ICONERROR);
         LocalFree(lpDisplayBuf);
      }
      LocalFree(lpMsgBuf);
   }
}

static bool ParseCommandLine(std::string cmdLine)
{
   auto pos = cmdLine.find("/msgId:");
   if (pos != std::string::npos) {
      auto msgId = cmdLine.substr(pos + strlen("/msgId:"));
      try {
         notifyMsgId_ = std::stoi(msgId);
         if (notifyMsgId_ != 0) {
            return true;
         }
      } catch (...) {
         return false;
      }
   }
   return false;
}

int WINAPI WinMain(
   _In_ HINSTANCE hInstance,
   _In_opt_ HINSTANCE,
   _In_ LPSTR,
   _In_ int)
{
   hGlobInstance_ = hInstance;

   auto cmdLine = GetCommandLineA();
   if (!ParseCommandLine(cmdLine)) {
      MessageBox(0, "Wrong parameters", "WinMute Screensaver Proxy", MB_ICONERROR);
      return FALSE;
   }
   HANDLE hMutex = CreateMutex(nullptr, TRUE, _T("LxSystemsWinMuteScreensaverProxy32"));
   if (hMutex == nullptr) {
      return FALSE;
   }
   if (GetLastError() == ERROR_ALREADY_EXISTS) {
      ReleaseMutex(hMutex);
      return FALSE;
   }

   ScreensaverNotifierProxy notifyProxy;
   if (!notifyProxy.Init(notifyMsgId_)) {
      MessageBox(0, "Failed to init Screensaver Notify Proxy", "WinMute", MB_ICONERROR);
      return FALSE;
   }

   MSG msg = { 0 };
   while (GetMessage(&msg, nullptr, 0, 0)) {
      if (msg.message == WM_QUIT) {
         break;
      }
   }
   ReleaseMutex(hMutex);
   notifyProxy.Unload();

   return static_cast<int>(msg.wParam);
}
