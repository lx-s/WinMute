/*
 WinMute
           Copyright (c) 2022, Alexander Steinhoefer

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

#ifdef UNICODE
static bool OpenLogFile(std::wofstream& logFile)
#else
static bool OpenLogFile(std::ofstream& logFile)
#endif
{
   auto path = std::filesystem::temp_directory_path();
   path /= L"WinMute.log";
   logFile.open(path.string(), std::ios::out | std::ios::app | std::ios::binary);
   return logFile.is_open();
}

WMLog& WMLog::GetInstance()
{
   static WMLog log;
   return log;
}

WMLog::WMLog() :
   initialized_(false)
{
   if (OpenLogFile(logFile_)) {
      initialized_ = true;
   }
}

WMLog::~WMLog()
{
   if (initialized_) {
      logFile_.close();
   }
}

void WMLog::SetEnabled(bool enable)
{
   enabled_ = enable;
}

void WMLog::WriteMessage(const tstring& msg)
{
   struct tm tm;
   auto now = std::chrono::system_clock::now();
   auto in_time_t = std::chrono::system_clock::to_time_t(now);

#ifdef UNICODE
   std::wstringstream ss;
#else
   std::stringstream ss;
#endif
   localtime_s(&tm, &in_time_t);
   ss << _T("[") << std::put_time(&tm, _T("%Y-%m-%d %X")) << _T("] ")
      << msg << _T("\n");
   const auto& logStr = ss.str();
   logFile_.write(logStr.c_str(), logStr.length());
   logFile_.flush();
}

void WMLog::Write(const tstring& msg)
{
   if (!initialized_ || !enabled_) {
      return;
   }
   WriteMessage(msg);
}
