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
      path += L"WinMute.log";
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

void WMLog::SetEnabled(bool enable)
{
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

bool WMLog::IsEnabled() const
{
   return enabled_;
}

void WMLog::WriteMessage(const wchar_t* msg)
{
   struct tm tm = { 0 };
   auto now = std::chrono::system_clock::now();
   auto in_time_t = std::chrono::system_clock::to_time_t(now);

   std::wstringstream ss;
   localtime_s(&tm, &in_time_t);
   ss << L"[" << std::put_time(&tm, L"%Y-%m-%d %X") << L"] "
      << msg << L"\n";
   const auto& logStr = ss.str();
   logFile_.write(logStr.c_str(), logStr.length());
   logFile_.flush();
}

void WMLog::Write(const wchar_t *fmt, ...)
{
   if (!enabled_) {
      return;
   }

   wchar_t buf[200];
   va_list ap;
   va_start(ap, fmt);

   vswprintf_s(buf, fmt, ap);
   WriteMessage(buf);

   va_end(ap);
}

void WMLog::WriteWindowsError(const wchar_t *functionName, DWORD lastError)
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
         reinterpret_cast<wchar_t*>(&lpMsgBuf), 0, nullptr) != 0) {
      size_t displayBufSize =
         (wcslen(static_cast<const wchar_t*>(lpMsgBuf))
          + wcslen(static_cast<const wchar_t*>(functionName))
          + 40) * sizeof(wchar_t);
      // Display the error message and exit the process
      LPVOID lpDisplayBuf = reinterpret_cast<LPVOID>(
         LocalAlloc(LMEM_ZEROINIT, displayBufSize));
      if (lpDisplayBuf) {
         StringCchPrintfW(
            reinterpret_cast<wchar_t*>(lpDisplayBuf),
            LocalSize(lpDisplayBuf),
            L"%s failed with error %u: %s",
            functionName,
            lastError,
            reinterpret_cast<wchar_t*>(lpMsgBuf));
         WriteMessage(static_cast<const wchar_t*>(lpMsgBuf));
         LocalFree(lpDisplayBuf);
      }
      LocalFree(lpMsgBuf);
   }
}
