/*
 WinMute
           Copyright (c) 2011-2026 Alexander Steinhoefer

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


#include <string>

// One entry of the managed audio endpoint allow/block list.
//
// "id" is the WASAPI endpoint id and is the preferred way to identify a
// device: it survives the friendly name changing (driver updates rename
// endpoints fairly often) and it is unique, which the friendly name is not.
//
// "id" is empty for entries the user typed by hand instead of picking an
// existing device from the dropdown. Those stay pure name matches on purpose,
// so a name can be configured for a device that is not attached yet.
struct ManagedEndpoint {
    std::wstring id;
    std::wstring name;

    bool operator==(const ManagedEndpoint& other) const
    {
        return id == other.id && name == other.name;
    }
    bool operator<(const ManagedEndpoint& other) const
    {
        return (name != other.name) ? name < other.name : id < other.id;
    }
};
