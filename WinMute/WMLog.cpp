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

#include "common.h"
#include "WMLog.h"

static const wchar_t* LogLevelToString(LogLevel level)
{
   switch (level) {
   case LogLevel::Debug:
      return L"DEBUG";
   case LogLevel::Info:
      return L"INFO";
   case LogLevel::Error:
      return L"ERROR";
   default:
      return L"UNKNOWN";
   }
}

static bool OpenLogFile(const std::wstring& filePath, std::wofstream& logFile)
{
   logFile.open(filePath, std::ios::out | std::ios::app | std::ios::binary);
   return logFile.is_open();
}

WMLog& WMLog::GetInstance()
{
   static WMLog log;
   return log;
}

WMLog::WMLog():
   enabled_(false)
{
}

WMLog::~WMLog()
{
   if (logFile_.is_open()) {
      logFile_.close();
   }
}

std::wstring WMLog::GetLogFilePath()
{
   wchar_t tempPath[MAX_PATH + 1];
   if (GetTempPathW(ARRAY_SIZE(tempPath), tempPath)) {
      std::wstring path{ tempPath };
      path += LOG_FILE_NAME;
      return path;
   }
   return std::wstring();
}

void WMLog::DeleteLogFile()
{
   const auto logFilePath = GetLogFilePath();
   if (logFile_.is_open()) {
      logFile_.close();
   }
   DeleteFileW(logFilePath.c_str());
}

void WMLog::EnableLogFile(bool enable)
{
   const std::lock_guard lock(logMutex_);
   if (enable == enabled_) {
      if (!enable) {
         DeleteLogFile();
      }
      return;
   }
   
   if (enable) {
      if (OpenLogFile(GetLogFilePath(), logFile_)) {
         enabled_ = true;
      }
   } else {
      if (logFile_.is_open()) {
         logFile_.close();
      }
      DeleteLogFile();
      enabled_ = false;
   }
}

bool WMLog::IsLogFileEnabled() const
{
   return enabled_;
}

std::wstring WMLog::FormatLogMessage(const LogMessage &logMsg, bool new_line) const
{
   auto msg = std::format(
      L"{:%F %T%Ez}  {}:  {}{}",
      logMsg.time,
      LogLevelToString(logMsg.level),
      logMsg.message,
      new_line ? L"\r\n" : L"");
   return msg;
}

void WMLog::StoreMessage(LogLevel level, const wchar_t* msg)
{
   namespace ch = std::chrono;

   LogMessage lm;
   lm.level = level;
   lm.message = msg;
   lm.time = ch::zoned_time<ch::milliseconds>{
      ch::current_zone(), ch::floor<ch::milliseconds>(ch::system_clock::now())
   };

   if (enabled_ && logFile_.is_open()) {
      const auto logMsg = FormatLogMessage(lm, true);
      logFile_.write(logMsg.c_str(), logMsg.length());
      logFile_.flush();
   }

   {
      const std::scoped_lock<std::mutex> lock(wndMutex_);
      for (const auto hWnd : registeredWindows_) {
         if (IsWindow(hWnd)) {
            SendMessageW(hWnd, WM_LOG_UPDATED, 0, reinterpret_cast<LPARAM>(&lm));
         }
      }
   }

   {
      const std::scoped_lock<std::mutex> lock(logMutex_);
      logMessages_.push_back(std::move(lm));
      if (logMessages_.size() >= kMaxLogEntries_) {
         logMessages_.erase(
            logMessages_.begin(),
            logMessages_.begin() + (logMessages_.size() - kMaxLogEntries_ / 2));
      }
   }
}

std::vector<LogMessage> WMLog::GetLogMessages() const
{
   const std::scoped_lock<std::mutex> lock(logMutex_);
   return logMessages_;
}

void WMLog::RegisterForLogUpdates(HWND hWnd)
{
   const std::scoped_lock<std::mutex> lock(wndMutex_);
   if (std::find(registeredWindows_.begin(), registeredWindows_.end(), hWnd) == registeredWindows_.end()) {
      registeredWindows_.push_back(hWnd);
   }
}

void WMLog::UnregisterForLogUpdates(HWND hWnd)
{
   const std::scoped_lock<std::mutex> lock(wndMutex_);
   if (const auto it = std::find(registeredWindows_.begin(), registeredWindows_.end(), hWnd);
       it != registeredWindows_.end()) {
      registeredWindows_.erase(it);
   }
}

void WMLog::LogDebug(_In_z_ _Printf_format_string_ const wchar_t *fmt, ...)
{
   wchar_t buf[1024];
   va_list ap;
   va_start(ap, fmt);

   vswprintf_s(buf, fmt, ap);
   StoreMessage(LogLevel::Debug, buf);

   va_end(ap);
}

void WMLog::LogInfo(_In_z_ _Printf_format_string_ const wchar_t *fmt, ...)
{
   wchar_t buf[1024];
   va_list ap;
   va_start(ap, fmt);

   vswprintf_s(buf, fmt, ap);
   StoreMessage(LogLevel::Info, buf);

   va_end(ap);
}

void WMLog::LogError(_In_z_ _Printf_format_string_ const wchar_t *fmt, ...)
{
   wchar_t buf[1024];
   va_list ap;
   va_start(ap, fmt);

   vswprintf_s(buf, fmt, ap);
   StoreMessage(LogLevel::Error, buf);

   va_end(ap);
}

void WMLog::LogWinError(const wchar_t *functionName, DWORD lastError)
{
   // Retrieve the system error message for the last-error code
   if (lastError == -1) {
      lastError = GetLastError();
   }

   LPVOID lpMsgBuf;
   if (FormatMessage(
         FORMAT_MESSAGE_ALLOCATE_BUFFER
         | FORMAT_MESSAGE_FROM_SYSTEM
         | FORMAT_MESSAGE_IGNORE_INSERTS,
         nullptr, lastError,
         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
         reinterpret_cast<wchar_t*>(&lpMsgBuf), 0, nullptr) == 0) {
      return;
   }
   const std::wstring errorMsgText{ reinterpret_cast<wchar_t *>(lpMsgBuf) };
   const std::wstring errorMsg = std::vformat(
      WMi18n::GetInstance().GetTranslationW("general.error.winapi.text"),
      std::make_wformat_args(
         functionName,
         lastError,
         errorMsgText));
   StoreMessage(LogLevel::Error, static_cast<const wchar_t *>(lpMsgBuf));
   LocalFree(lpMsgBuf);
}
