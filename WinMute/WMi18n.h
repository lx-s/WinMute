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

#include "common.h"

struct LanguageModule {
   std::wstring langName;
   std::wstring fileName;
};

using TranslationMap = std::map<std::string, std::wstring>;

class WMi18n {
public:
   static WMi18n& GetInstance();

   bool Init();

   bool LoadLanguage(const std::wstring &fileName);
   std::vector<LanguageModule> GetAvailableLanguages() const;

   std::optional<fs::path> GetLanguageFilesPath() const;
   std::wstring GetCurrentLanguageName() const;

   std::wstring GetTranslationW(const std::string& textId) const;
   std::string GetTranslationA(const std::string& textId) const;

   bool SetItemText(HWND hWnd, int dlgItem, const std::string &textId);
   bool SetItemText(HWND hItem, const std::string &textId);

private:
   WMi18n() noexcept;
   ~WMi18n() noexcept;
   WMi18n(const WMi18n &) = delete;
   WMi18n &operator=(const WMi18n &) = delete;

   mutable std::mutex langMutex_;
   const std::wstring defaultLangName_ = L"lang-en.json";
   std::wstring curModuleName_;
   TranslationMap loadedLang_;
   TranslationMap defaultLang_;

   void UnloadLanguage();
   bool LoadDefaultLanguage();
   bool LoadLanguage(const std::wstring &fileName, TranslationMap &strings);
};

