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

#pragma once

#include "common.h"

class WMLog {
public:
   static WMLog& GetInstance();

   void Write(const tstring& wmsg);

   template<typename... Args>
   void Write(
      [[maybe_unused]] const tstring_view& fmt,
      [[maybe_unused]] Args&&... args)
   {
      if (!initialized_ || !enabled_) {
         return;
      }
#ifdef UNICODE
      const std::wstring str = std::vformat(
         fmt, std::make_wformat_args(args...));
#else
      const std::wstring str = std::vformat(
         fmt, std::make_format_args(args...));
#endif
      WriteMessage(str);
   }

   void SetEnabled(bool enable);
   std::string GetLogFilePath();

private:
   WMLog();
   ~WMLog();
   WMLog(const WMLog&) = delete;
   WMLog& operator=(const WMLog&) = delete;

   void WriteMessage(const tstring& msg);

   bool initialized_;
   bool enabled_;
#ifdef UNICODE
   std::wofstream logFile_;
#else
   std::ofstream logFile_;
#endif
};
