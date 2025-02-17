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

#include "Common.h"

// =========================================================================
//    Types
// =========================================================================

struct WINDOWPROCESSINFO {
   DWORD pid;
   HWND hWnd;
};

// =========================================================================
//    ShowWindowsError
// =========================================================================

void ShowWindowsError(const wchar_t *functionName, DWORD lastError)
{
   // Retrieve the system error message for the last-error code
   if (lastError == -1) {
      lastError = GetLastError();
   }

   LPVOID lpMsgBuf;
   if (FormatMessageW(
         FORMAT_MESSAGE_ALLOCATE_BUFFER |
         FORMAT_MESSAGE_FROM_SYSTEM |
         FORMAT_MESSAGE_IGNORE_INSERTS,
         nullptr,
         lastError,
         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
         reinterpret_cast<wchar_t*>(&lpMsgBuf),
         0,
         nullptr) == 0) {
      return;
   }
   // Display the error message and exit the process
   const std::wstring errorMsgText{ reinterpret_cast<wchar_t *>(lpMsgBuf) };
   const std::wstring errorMsg = std::vformat(
      WMi18n::GetInstance().GetTranslationW("general.error.winapi.text"),
      std::make_wformat_args(
         functionName,
         lastError,
         errorMsgText));
   TaskDialog(
      nullptr, nullptr, PROGRAM_NAME, errorMsg.c_str(),
      nullptr, TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
   LocalFree(lpMsgBuf);
}

// =========================================================================
//    GetWinMuteversion
// =========================================================================

bool GetWinMuteVersion(std::wstring &versNumber)
{
   DWORD unused;
   wchar_t fileName[MAX_PATH];
   if (GetModuleFileNameW(nullptr, fileName, sizeof(fileName) / sizeof(fileName[0])) == 0) {
      return false;
   }
   const DWORD versSize = GetFileVersionInfoSizeEx(FILE_VER_GET_NEUTRAL, fileName, &unused);
   std::vector<std::byte> versData(versSize);
   if (!GetFileVersionInfoExW(FILE_VER_GET_NEUTRAL, fileName, 0, versSize, versData.data())) {
      return false;
   }
   VS_FIXEDFILEINFO *pvi = nullptr;
   UINT pviLen = sizeof(*pvi);
   if (!VerQueryValueW(versData.data(), L"\\", reinterpret_cast<LPVOID *>(&pvi), &pviLen)) {
      return false;
   }
   versNumber = std::format(
      L"{}.{}.{}.{}",
      pvi->dwProductVersionMS >> 16,
      pvi->dwFileVersionMS & 0xFFFF,
      pvi->dwFileVersionLS >> 16,
      pvi->dwFileVersionLS & 0xFFFF);
   return true;
}

// =========================================================================
//    String Conversion
// =========================================================================

std::wstring ConvertStringToWideString(const std::string &ansiString)
{
   std::wstring unicodeString;
   auto wideCharSize = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, ansiString.c_str(), -1, nullptr, 0);
   if (wideCharSize == 0) {
      return L"";
   }
   unicodeString.reserve(wideCharSize);
   unicodeString.resize(wideCharSize - 1);
   wideCharSize = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, ansiString.c_str(), -1, &unicodeString[0], wideCharSize);
   return unicodeString;
}

std::string ConvertWideStringToString(const std::wstring &wideString)
{
   std::string ansiString;
   auto ansiStringSize = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, wideString.c_str(), -1, nullptr, 0, "?", nullptr);
   if (ansiStringSize == 0) {
      return "";
   }
   ansiString.reserve(ansiStringSize);
   ansiString.resize(ansiStringSize - 1);
   ansiStringSize = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, wideString.c_str(), -1, &ansiString[0], ansiStringSize, "?", nullptr);
   return ansiString;
}

// =========================================================================
//    Launch Browser
// =========================================================================

bool LaunchBrowser(HWND hParent, const std::wstring &url)
{
   SHELLEXECUTEINFOW sxi = { 0 };
   sxi.cbSize = sizeof(sxi);
   sxi.nShow = SW_SHOWNORMAL;
   sxi.hwnd = hParent;
   sxi.lpVerb = L"open";
   sxi.lpFile = url.c_str();
   if (!ShellExecuteExW(&sxi)) {
      WMLog::GetInstance().LogWinError(L"ShellExecuteEx", GetLastError());
      return false;
   }
   return true;
}
