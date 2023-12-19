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

#pragma once

#ifndef UNICODE
#  define UNICODE
#endif

#ifndef _UNICODE
#  define _UNICODE
#endif

#if defined _M_IX86
#  pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#  pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#  pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#  pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#define STRICT
#define _WIN32_WINNT 0x0601
#include <sdkddkver.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdarg>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <locale>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <WtsApi32.h>
#include <winhttp.h>
#include <dbt.h>
#include <Bthdef.h>
#include <Bthsdpdef.h>
#include <BluetoothAPIs.h>
#include <tchar.h>
#include <strsafe.h>
#include <time.h>
#include <uxtheme.h>
#include <comip.h>
#include <comdef.h>
#include <wlanapi.h>
#include <Mmdeviceapi.h>
#include <dwmapi.h>
#include <sal.h>
#pragma warning(disable : 4201)
#  include <endpointvolume.h>
#pragma warning(default : 4201)

#include "libs/json.hpp"

#include "resource.h"

#include "WMi18n.h"
#include "WMSettings.h"
#include "WMLog.h"
#include "UpdateChecker.h"
#include "TrayIcon.h"
#include "WinAudio.h"
#include "MuteControl.h"
#include "WiFiDetector.h"
#include "BluetoothDetector.h"
#include "QuietHoursTimer.h"
#include "WinMute.h"
#include "VersionHelper.h"

// Utility
void PrintWindowsError(const wchar_t *functionName, DWORD lastError = -1);
bool GetWinMuteVersion(std::wstring &versNumber);

static const wchar_t *PROGRAM_NAME = L"WinMute";
static const wchar_t *LOG_FILE_NAME = L"WinMute.log";

constexpr int WM_SAVESETTINGS = WM_USER + 300;

template<class Interface>
inline void SafeRelease(Interface * *ppInterfaceToRelease)
{
   if (*ppInterfaceToRelease) {
      (*ppInterfaceToRelease)->Release();
      (*ppInterfaceToRelease) = nullptr;
   }
}

#define ARRAY_SIZE(arr) ((sizeof(arr)) / (sizeof(arr[0])))
