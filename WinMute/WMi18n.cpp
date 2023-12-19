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

bool WMi18n::Init()
{
   return LoadDefaultLanguage();
}

std::wstring WMi18n::ConvertStringToWideString(const std::string& ansiString) const
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

std::string WMi18n::ConvertWideStringToString(const std::wstring &wideString) const
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

std::optional<fs::path> WMi18n::GetLanguageFilesPath() const
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
   std::vector<LanguageModule> langDlls;
   const auto langPath = GetLanguageFilesPath();
   if (langPath.has_value()) {
      const fs::path searchPath = *langPath / L"*.json";
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
            try {
               const fs::path langFilePath = *langPath / wfd.cFileName;
               std::ifstream json_file(langFilePath);
               nlohmann::json file_info = nlohmann::json::parse(json_file);
               if (file_info.contains("meta.lang.name")) {
                  LanguageModule langMod;
                  langMod.fileName = wfd.cFileName;
                  langMod.langName = ConvertStringToWideString(file_info["meta.lang.name"]);
                  langDlls.push_back(langMod);
               }
            } catch (const nlohmann::json::parse_error &pe) {
               WMLog::GetInstance().Write(L"Failed to parse language file \"%ls\": %S", wfd.cFileName, pe.what());
            }
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
   return GetTranslationW("meta.lang.name");
}

bool WMi18n::LoadDefaultLanguage()
{
   if (!LoadLanguage(defaultLangName_, defaultLang_)) {
      const std::wstring error = std::format(L"Failed to load default language. Please make sure the langs-Folder exists and contains {}.", defaultLangName_);;
      TaskDialog(
         nullptr,
         nullptr,
         PROGRAM_NAME,
         L"Failed to initialize translation framework",
         error.c_str(),
         TDCBF_OK_BUTTON,
         TD_ERROR_ICON,
         nullptr);
      return false;
   }
   return true;
}

bool WMi18n::LoadLanguage(const std::wstring &fileName, TranslationMap &strings)
{
   WMLog &log = WMLog::GetInstance();
   auto langFilePath = GetLanguageFilesPath();
   if (!langFilePath) {
      return false;
   }
   const fs::path loadFilePath{ fileName};
   *langFilePath /= loadFilePath.filename(); // Sanitize
   if (!fs::exists(*langFilePath)) {
      log.Write(L"Language module \"%ls\" does not exist", langFilePath->c_str());
      return false;
   }
   try {
      std::ifstream json_file(*langFilePath);
      nlohmann::json json_data = nlohmann::json::parse(json_file);
      TranslationMap translations_temp;
      for (auto it = json_data.begin(); it != json_data.end(); ++it) {
         if (it->is_structured()) {
            log.Write(L"Language module \"%ls\" has nested elements", langFilePath->c_str());
            return false;
         }
         const auto value = ConvertStringToWideString(it.value());
         if (value == L"") {
            log.Write(L"Unable to convert language element \"%S\"", it.key().c_str());
            return false;
         }
         if (translations_temp.contains(it.key())) {
            log.Write(L"Double entry for language key \"%S\" found.", it.key().c_str());
            return false;
         }
         translations_temp[it.key()] = value;
      }
      strings = std::move(translations_temp);
   } catch (const nlohmann::json::parse_error &pe) {
      WMLog::GetInstance().Write(
         L"Failed to parse language file \"%ls\": %S",
         langFilePath->filename().c_str(),
         pe.what());
      return false;
   }
   return true;
}

bool WMi18n::LoadLanguage(const std::wstring &fileName)
{
   if (fileName.empty() || fileName == defaultLangName_) {
      UnloadLanguage();
      return true;
   }

   TranslationMap new_lang;
   if (!LoadLanguage(fileName, new_lang)) {
      WMLog::GetInstance().Write(L"Failed to load language \"%ls\"", fileName.c_str());
      return false;
   } else {
      UnloadLanguage();
      loadedLang_ = std::move(new_lang);
   }

   return true;
}

void WMi18n::UnloadLanguage() noexcept
{
   loadedLang_.clear();
}

std::wstring WMi18n::GetTranslationW(const std::string& textId) const
{
   std::wstring text;
   if (!loadedLang_.empty() && loadedLang_.contains(textId)) {
      auto it = loadedLang_.find(textId);
      if (it != loadedLang_.end()) {
         text = it->second;
      }
   }
   if (text.empty() && defaultLang_.contains(textId)) {
      auto it = defaultLang_.find(textId);
      if (it != defaultLang_.cend()) {
         text = it->second;
      }
   }
   if (text.empty()) {
      std::wstring err = std::format(L"Translation for {} not found", ConvertStringToWideString(textId));
      return text;
   }
   return text;
}

std::string WMi18n::GetTranslationA(const std::string &textId) const
{
   const std::wstring wtext = GetTranslationW(textId);
   return ConvertWideStringToString(wtext);
}

bool WMi18n::SetItemText(HWND hWnd, int dlgItem, const std::string& textId)
{
   const auto text = GetTranslationW(textId);
   if (!SetDlgItemTextW(hWnd, dlgItem, text.c_str())) {
      WMLog::GetInstance().WriteWindowsError(L"SetDlgItemTextW", GetLastError());
      return false;
   }
   return true;
}

bool WMi18n::SetItemText(HWND hItem, const std::string &textId)
{
   const auto text = GetTranslationW(textId);
   if (!SetWindowTextW(hItem, text.c_str())) {
      WMLog::GetInstance().WriteWindowsError(L"SetDlgItemTextW", GetLastError());
      return false;
   }
   return true;
}
