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

#include <format>
#include <string>
#include <string_view>
#include <optional>

#include <atlbase.h>
#include <windows.h>
#include <mmdeviceapi.h>

// =============================================================================
// Utility

// Formats a runtime format string (e.g. from a translation file) without
// letting a malformed placeholder terminate the process: on std::format_error
// the unformatted string is returned instead.
template<typename... Args>
std::wstring SafeVFormat(std::wstring_view fmt, Args&&... args)
{
   try {
      return std::vformat(fmt, std::make_wformat_args(args...));
   } catch (const std::format_error &) {
      return std::wstring{ fmt };
   }
}

void ShowWindowsError(const wchar_t *functionName, DWORD lastError = -1);
bool GetWinMuteVersion(std::wstring &versNumber);

std::wstring ConvertStringToWideString(const std::string &ansiString);
std::string ConvertWideStringToString(const std::wstring &wideString);

bool LaunchBrowser(HWND hParent, const std::wstring &url);

std::optional<std::wstring> GetAudioDeviceName(const CComPtr<IMMDevice> &devicePtr);

// =============================================================================
// COM Helper
template <class Interface>
inline void SafeRelease(Interface **ppInterfaceToRelease)
{
   if (*ppInterfaceToRelease)
   {
      (*ppInterfaceToRelease)->Release();
      (*ppInterfaceToRelease) = nullptr;
   }
}
