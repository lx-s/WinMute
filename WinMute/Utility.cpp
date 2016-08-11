/*
 WinMute
           Copyright (C) 2016, Alexander Steinhoefer

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

#include "StdAfx.h"

void PrintWindowsError(LPTSTR lpszFunction, DWORD lastError)
{
   // Retrieve the system error message for the last-error code
   if (lastError == -1) {
      lastError = GetLastError();
   }

   LPVOID lpMsgBuf;
   if (FormatMessage(
       FORMAT_MESSAGE_ALLOCATE_BUFFER |
       FORMAT_MESSAGE_FROM_SYSTEM |
       FORMAT_MESSAGE_IGNORE_INSERTS,
       nullptr, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
       reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr) != 0) {

      // Display the error message and exit the process
      LPVOID lpDisplayBuf = reinterpret_cast<LPVOID>(LocalAlloc(LMEM_ZEROINIT,
                            (lstrlen(static_cast<LPCTSTR>(lpMsgBuf)) +
                             lstrlen(static_cast<LPCTSTR>(lpszFunction)) + 40)
                            * sizeof(TCHAR)));
      if (lpDisplayBuf) {
         StringCchPrintf((LPTSTR)lpDisplayBuf,
                         LocalSize(lpDisplayBuf),
                         _T("%s failed with error %ul: %s"),
                         lpszFunction,
                         lastError,
                         reinterpret_cast<TCHAR*>(lpMsgBuf));
         TaskDialog(nullptr,
                    nullptr,
                    PROGRAM_NAME,
                    static_cast<LPCTSTR>(lpDisplayBuf),
                    nullptr,
                    TDCBF_OK_BUTTON,
                    TD_ERROR_ICON,
                    nullptr);
         LocalFree(lpDisplayBuf);
      }
      LocalFree(lpMsgBuf);
   }
}
