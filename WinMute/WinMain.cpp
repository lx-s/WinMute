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

#include "common.h"

HINSTANCE hglobInstance;

static bool SetWorkingDirectory()
{
   wchar_t wmFileName[MAX_PATH + 1];
   if (GetModuleFileNameW(nullptr, wmFileName, ARRAY_SIZE(wmFileName)) > 0) {
      wchar_t* p = wcsrchr(wmFileName, L'\\');
      if (p != nullptr) {
         *(p + 1) = L'\0';
         SetCurrentDirectoryW(wmFileName);
         return true;
      }
   }
   return false;
}

static void LoadLanguage(WMSettings &settings, WMi18n &i18n)
{
   const auto langModule = settings.QueryStrValue(SettingsKey::APP_LANGUAGE);
   if (langModule.has_value()) {
      i18n.LoadLanguage(*langModule);
   }
}

static bool InitWindowsComponents()
{
   INITCOMMONCONTROLSEX initComCtrl;
   initComCtrl.dwSize = sizeof(INITCOMMONCONTROLSEX);
   initComCtrl.dwICC = ICC_LINK_CLASS;
   if (InitCommonControlsEx(&initComCtrl) == FALSE) {
      WMLog::GetInstance().WriteWindowsError(L"InitCommonControlsEx", GetLastError());
      return FALSE;
   } else if (CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED) != S_OK) {
      WMLog::GetInstance().WriteWindowsError(L"CoInitializeEx", GetLastError());
      return FALSE;
   }
   return TRUE;
}

static DWORD CheckForUpdates_Thread(LPVOID param)
{
   auto updateChecker = reinterpret_cast<UpdateChecker *>(param);
   if (updateChecker == nullptr) {
      return 1;
   }
   UpdateInfo updateInfo;
   if (!updateChecker->GetUpdateInfo(updateInfo)) {
      // Print Error
   } else {
      // Print Update Message
   }

   delete updateChecker;
   return 0;
}

static void CheckForUpdates(const WMSettings& settings)
{
   UpdateChecker *updateChecker = new UpdateChecker();
   if (updateChecker == nullptr) {
      return;
   }
   if (!updateChecker->IsUpdateCheckEnabled(settings)) {
      delete updateChecker;
      return;
   }
   auto updateThread = CreateThread(
      nullptr, 0, CheckForUpdates_Thread,
      reinterpret_cast<LPVOID>(updateChecker),
      0, nullptr);
   if (updateThread == nullptr) {
      delete updateChecker;
      return;
   }

}

int WINAPI wWinMain(
   _In_ HINSTANCE hInstance,
   _In_opt_ HINSTANCE,
   _In_ PWSTR,
   _In_ int)
{
   hglobInstance = hInstance;
   WMSettings settings;
   WMi18n& i18n = WMi18n::GetInstance();
   if (!i18n.Init()) {
      return FALSE;
   }
   if (!settings.Init()) {
      TaskDialog(
         nullptr,
         nullptr,
         PROGRAM_NAME,
         i18n.GetTranslationW("init.error.settings.title").c_str(),
         i18n.GetTranslationW("init.error.settings.text").c_str(),
         TDCBF_OK_BUTTON,
         TD_ERROR_ICON,
         nullptr);
      return FALSE;
   }

   LoadLanguage(settings, i18n);

   HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"LxSystemsWinMuteRunOnce");
   if (hMutex == nullptr) {
      return FALSE;
   } else if (GetLastError() == ERROR_ALREADY_EXISTS) {
      ReleaseMutex(hMutex);
      TaskDialog(
         nullptr,
         nullptr,
         PROGRAM_NAME,
         i18n.GetTranslationW("init.error.already-running.title").c_str(),
         i18n.GetTranslationW("init.error.already-running.text").c_str(),
         TDCBF_OK_BUTTON,
         TD_INFORMATION_ICON,
         nullptr);
      return FALSE;
   }

   SetWorkingDirectory();

   HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);

   if (!InitWindowsComponents()) {
      TaskDialog(
         nullptr,
         nullptr,
         PROGRAM_NAME,
         i18n.GetTranslationW("init.error.winmute.title").c_str(),
         i18n.GetTranslationW("init.error.winmute.text").c_str(),
         TDCBF_OK_BUTTON,
         TD_ERROR_ICON,
         nullptr);
      ReleaseMutex(hMutex);
      return FALSE;
   }

   CheckForUpdates(settings);

   MSG msg = { nullptr };
   WinMute program(settings);
   if (program.Init()) {
      while (GetMessage(&msg, nullptr, 0, 0)) {
         HWND hwnd = GetForegroundWindow();
         if (!IsWindow(hwnd) || !IsDialogMessage(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
         }
      }
   }

   CoUninitialize();
   ReleaseMutex(hMutex);
   return static_cast<int>(msg.wParam);
}
