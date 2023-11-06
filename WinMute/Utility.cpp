/*
 WinMute
           Copyright (c) 2023, Alexander Steinhoefer

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

#include "Common.h"

void PrintWindowsError(const wchar_t *functionName, DWORD lastError)
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
         nullptr,
         lastError,
         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
         reinterpret_cast<wchar_t*>(&lpMsgBuf), 0, nullptr) != 0) {
      // Display the error message and exit the process
      const std::wstring errorMsg = std::vformat(
         WMi18n::GetInstance().GetTextW(IDS_MAIN_ERROR_WINAPI_FAILURE_TEXT),
         std::make_wformat_args(
            functionName,
            lastError,
            reinterpret_cast<wchar_t *>(lpMsgBuf)));
      TaskDialog(
         nullptr,
         nullptr,
         PROGRAM_NAME,
         errorMsg.c_str(),
         nullptr,
         TDCBF_OK_BUTTON,
         TD_ERROR_ICON,
         nullptr);
      LocalFree(lpMsgBuf);
   }
}

bool GetWinMuteVersion(std::wstring &versNumber)
{
   bool success = false;
   DWORD dummy;
   wchar_t szFileName[MAX_PATH];
   GetModuleFileName(nullptr, szFileName, sizeof(szFileName) / sizeof(szFileName[0]));
   const DWORD versSize = GetFileVersionInfoSizeEx(FILE_VER_GET_NEUTRAL, szFileName, &dummy);
   LPVOID versData = malloc(versSize);
   if (versData != nullptr) {
      if (GetFileVersionInfoExW(
         FILE_VER_GET_NEUTRAL,
         szFileName,
         0,
         versSize,
         versData)) {
         VS_FIXEDFILEINFO *pvi;
         UINT pviLen = sizeof(*pvi);
         if (VerQueryValueW(
            versData,
            L"\\",
            reinterpret_cast<LPVOID *>(&pvi),
            &pviLen)) {
            wchar_t buf[50];
            StringCchPrintfW(
               buf,
               ARRAY_SIZE(buf),
               L"%d.%d.%d.%d",
               pvi->dwProductVersionMS >> 16,
               pvi->dwFileVersionMS & 0xFFFF,
               pvi->dwFileVersionLS >> 16,
               pvi->dwFileVersionLS & 0xFFFF);
            versNumber = buf;
            success = true;
         }
      }
      free(versData);
   }
   return success;
}
