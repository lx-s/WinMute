/*
 WinMute
           Copyright (c) 2021, Alexander Steinhoefer

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

#include "StdAfx.h"

static bool OpenLogFile(std::ofstream &logFile)
{
   auto path = std::filesystem::temp_directory_path();
   path /= "WinMute.log";
   logFile.open(path.string(), std::ios::out|std::ios::app|std::ios::binary);
   return logFile.is_open();
}

Log& Log::GetInstance()
{
   static Log log;
   return log;
}

Log::Log():
   initialized_(false)
{
   if (OpenLogFile(logFile_)) {
      initialized_ = true;
   }
}

Log::~Log()
{
   if (initialized_) {
      logFile_.close();
   }
}

void Log::Write(std::string msg)
{
   if (!initialized_) {
      return;
   }
   auto now = std::chrono::system_clock::now();
   auto in_time_t = std::chrono::system_clock::to_time_t(now);
   struct tm tm;

   std::stringstream ss;
   localtime_s(&tm, &in_time_t);
   ss << "[" << std::put_time(&tm, "%Y-%m-%d %X") << "] ";
   ss << msg;
   ss << "\n";

   logFile_.write(ss.str().c_str(), ss.str().length());
   logFile_.flush();
}
