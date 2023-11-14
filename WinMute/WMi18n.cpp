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

extern HINSTANCE hglobInstance;

WMi18n::WMi18n() noexcept
{
}

WMi18n::~WMi18n() noexcept
{
   UnloadLanguage();
}

WMi18n& WMi18n::GetInstance()
{
   static WMi18n inst;
   return inst;
}

std::optional<fs::path> WMi18n::GetLanguageModulesPath() const
{
   wchar_t wmPath[MAX_PATH + 1]{ 0 };
   if (GetModuleFileNameW(nullptr, wmPath, MAX_PATH) <= 0) {
      WMLog::GetInstance().WriteWindowsError(L"GetModuleFileNameW", GetLastError());
      return std::nullopt;
   }
   fs::path langPath = wmPath;
   return langPath.remove_filename() / L"lang";
}

std::vector<LanguageModule> WMi18n::GetAvailableLanguages() const
{
   std::vector<LanguageModule> langDlls{ { L"English", L"" } };
   const auto langPath = GetLanguageModulesPath();
   if (langPath.has_value()) {
      const fs::path searchPath = *langPath / L"*.dll";
      WIN32_FIND_DATAW wfd{ 0 };
      HANDLE hFindFile = FindFirstFileExW(
         searchPath.c_str(),
         FindExInfoBasic,
         &wfd,
         FindExSearchNameMatch,
         nullptr,
         FIND_FIRST_EX_CASE_SENSITIVE);
      if (hFindFile != INVALID_HANDLE_VALUE) {
         do {
            LanguageModule langMod;
            const fs::path langFilePath = *langPath / wfd.cFileName;
            langMod.fileName = wfd.cFileName;
            HMODULE hLangFile = LoadLibraryExW(
               langFilePath.c_str(),
               nullptr,
               LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE);
            if (hLangFile == nullptr) {
               continue;
            }
            wchar_t langName[255 + 1];
            if (LoadStringW(hLangFile, IDS_LANG_NAME, langName, ARRAY_SIZE(langName)) <= 0) {
               WMLog &log = WMLog::GetInstance();
               log.WriteWindowsError(L"LoadStringW", GetLastError());
               log.Write(L"Failed to load language %ls", wfd.cFileName);
            } else {
               langMod.langName = langName;
               langDlls.push_back(langMod);
            }
            FreeLibrary(hLangFile);
         } while (FindNextFileW(hFindFile, &wfd));
      }
   }
   return langDlls;
}

std::wstring WMi18n::GetCurrentLanguageModule() const
{
   return curModuleName_;
}

std::wstring WMi18n::GetCurrentLanguageName() const
{
   return GetTextW(IDS_LANG_NAME);
}

// https://learn.microsoft.com/en-us/windows/win32/intl/creating-a-multilingual-user-interface-application

bool WMi18n::LoadLanguage(const std::wstring &dllName)
{
   if (dllName.empty()) {
      UnloadLanguage();
      return true;
   } else {
      auto langModPath = GetLanguageModulesPath();
      if (!langModPath.has_value()) {
         return false;
      }
      const fs::path pathDllName{ dllName };
      *langModPath /= pathDllName.filename(); // Sanitize
      if (!fs::exists(*langModPath)) {
         WMLog::GetInstance().Write(L"Language module \"%s\" does not exist", langModPath->c_str());
      } else {
         auto newLangModule = LoadLibraryExW(
            langModPath->c_str(), nullptr,
            LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE);
         if (newLangModule == nullptr) {
            WMLog::GetInstance().WriteWindowsError(L"LoadLibraryW", GetLastError());
         } else {
            UnloadLanguage();
            langModule_ = newLangModule;
            curModuleName_ = dllName;
            return true;
         }
      }
   }
   return false;
}

void WMi18n::UnloadLanguage() noexcept
{
   if (langModule_ != nullptr) {
      FreeLibrary(langModule_);
      langModule_ = nullptr;
      curModuleName_.clear();
   }
}

std::wstring WMi18n::GetTextW(UINT textId) const
{
   HMODULE hMod = (langModule_ == nullptr) ? hglobInstance : langModule_;
   const wchar_t *strBuffer = nullptr;
   int strLength = LoadStringW(hMod, textId, reinterpret_cast<LPWSTR>(&strBuffer), 0);
   if (strLength == 0 && langModule_ != nullptr) {
      // Try to fall back to standard language
      strLength = LoadStringW(hglobInstance, textId, reinterpret_cast<LPWSTR>(&strBuffer), 0);
   }
   if (strLength == 0) {
      WMLog::GetInstance().WriteWindowsError(L"LoadStringW", GetLastError());
      std::wstring err = L"String resource " + std::to_wstring(textId) + L" not found";
      return err;
   }
   return std::wstring(strBuffer, strBuffer + strLength);
}

std::string WMi18n::GetTextA(UINT textId) const
{
   HMODULE hMod = (langModule_ == nullptr) ? hglobInstance : langModule_;
   const char *strBuffer = nullptr;
   int strLength = LoadStringA(hMod, textId, reinterpret_cast<LPSTR>(&strBuffer), 0);
   if (strLength == 0 && langModule_ != nullptr) {
      // Try to fall back to standard language
      strLength = LoadStringA(hglobInstance, textId, reinterpret_cast<LPSTR>(&strBuffer), 0);
   }
   if (strLength == 0) {
      WMLog::GetInstance().WriteWindowsError(L"LoadStringA", GetLastError());
      std::string err = "String resource " + std::to_string(textId) + " not found";
      return err;
   }
   return std::string(strBuffer, strBuffer + strLength);
}

bool WMi18n::SetItemText(HWND hWnd, int dlgItem, UINT textId)
{
   const auto text = GetTextW(textId);
   if (!SetDlgItemTextW(hWnd, dlgItem, text.c_str())) {
      WMLog::GetInstance().WriteWindowsError(L"SetDlgItemTextW", GetLastError());
      return false;
   }
   return true;
}

bool WMi18n::SetItemText(HWND hItem, UINT textId)
{
   const auto text = GetTextW(textId);
   if (!SetWindowTextW(hItem, text.c_str())) {
      WMLog::GetInstance().WriteWindowsError(L"SetDlgItemTextW", GetLastError());
      return false;
   }
   return true;
}
