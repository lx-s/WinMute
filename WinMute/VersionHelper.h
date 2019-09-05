/*
 WinMute
           Copyright (c) 2017, Alexander Steinhoefer

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

#ifndef _versionhelpers_H_INCLUDED_ // avoid conflict with MS versionhelpers

inline bool IsWindowsVersionOrGreater(WORD wMajorVersion,
   WORD wMinorVersion,
   WORD wServicePackMajor)
{
   OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, { 0 }, 0, 0 };
   DWORDLONG        const dwlConditionMask = VerSetConditionMask(
      VerSetConditionMask(
      VerSetConditionMask(
      0, VER_MAJORVERSION, VER_GREATER_EQUAL),
      VER_MINORVERSION, VER_GREATER_EQUAL),
      VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);

   osvi.dwMajorVersion = wMajorVersion;
   osvi.dwMinorVersion = wMinorVersion;
   osvi.wServicePackMajor = wServicePackMajor;

   return VerifyVersionInfoW(&osvi,
      VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR,
      dwlConditionMask) != FALSE;
}

inline bool IsWindowsXPOrGreater()
{
   return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINXP),
      LOBYTE(_WIN32_WINNT_WINXP),
      0);
}

inline bool IsWindowsVistaOrGreater()
{
   return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_VISTA),
      LOBYTE(_WIN32_WINNT_VISTA),
      0);
}

#endif