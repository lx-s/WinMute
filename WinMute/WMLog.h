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

#include "common.h"

#include <format>

enum class LogLevel {
   Debug,
   Info,
   Warning,
   Error,
};

struct LogMessage {
   LogLevel level = LogLevel::Info;
   std::chrono::zoned_time<std::chrono::milliseconds> time;
   std::wstring message;
};

class WMLog {
public:
   static WMLog& GetInstance();

   // The format string is validated against the arguments at compile time.
   template<typename... Args>
   void LogDebug(std::wformat_string<Args...> fmt, Args&&... args)
   {
      StoreMessage(LogLevel::Debug,
                   std::format(fmt, std::forward<Args>(args)...).c_str());
   }
   template<typename... Args>
   void LogInfo(std::wformat_string<Args...> fmt, Args&&... args)
   {
      StoreMessage(LogLevel::Info,
                   std::format(fmt, std::forward<Args>(args)...).c_str());
   }
   template<typename... Args>
   void LogWarning(std::wformat_string<Args...> fmt, Args&&... args)
   {
      StoreMessage(LogLevel::Warning,
                   std::format(fmt, std::forward<Args>(args)...).c_str());
   }
   template<typename... Args>
   void LogError(std::wformat_string<Args...> fmt, Args&&... args)
   {
      StoreMessage(LogLevel::Error,
                   std::format(fmt, std::forward<Args>(args)...).c_str());
   }
   void LogWinError(const wchar_t *functionName, DWORD errorCode = -1);

   void EnableLogFile(bool enable);
   bool IsLogFileEnabled() const;
   std::wstring GetLogFilePath();

   std::vector<LogMessage> GetLogMessages() const;

   void RegisterForLogUpdates(HWND hWnd);
   void UnregisterForLogUpdates(HWND hWnd);

   std::wstring FormatLogMessage(const LogMessage &logMsg, bool new_line=false) const;

private:
   WMLog();
   ~WMLog();
   WMLog(const WMLog&) = delete;
   WMLog& operator=(const WMLog&) = delete;

   const size_t kMaxLogEntries_ = 500;

   void StoreMessage(LogLevel level, const wchar_t *msg);
   void DeleteLogFile();

   mutable std::mutex logMutex_;
   mutable std::mutex wndMutex_;

   bool enabled_;
   std::wofstream logFile_;
   std::vector<LogMessage> logMessages_;
   std::vector<HWND> registeredWindows_;
};
