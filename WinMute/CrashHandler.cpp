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

#include "common.h"

#include "CrashHandler.h"

#include <DbgHelp.h>

#include <csignal>
#include <exception>

// Everything in here runs while the process is already going down, so it
// deliberately avoids WMLog (it may be what threw, or another thread may hold
// its mutex), WMi18n, and std::chrono time zones (see issue #59).

namespace {

using MiniDumpWriteDumpFn = BOOL(WINAPI*)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE,
                                          PMINIDUMP_EXCEPTION_INFORMATION,
                                          PMINIDUMP_USER_STREAM_INFORMATION,
                                          PMINIDUMP_CALLBACK_INFORMATION);

// Resolved up front: loading a DLL while crashing is best avoided.
MiniDumpWriteDumpFn miniDumpWriteDump = nullptr;

// Set by whichever handler runs first. std::terminate ends in abort(), which
// raises SIGABRT, so without this the crash would be handled twice.
std::atomic_flag crashHandled;

std::wstring GetCurrentExceptionMessage()
{
    const auto ex = std::current_exception();
    if (!ex) {
        return L"The program was terminated (no active exception).";
    }
    try {
        std::rethrow_exception(ex);
    } catch (const std::exception& e) {
        return L"Unhandled exception: " + ConvertStringToWideString(e.what());
    } catch (...) {
        return L"Unhandled exception of unknown type.";
    }
}

std::wstring GetDumpFilePath()
{
    wchar_t tempPath[MAX_PATH + 1];
    if (GetTempPathW(ARRAY_SIZE(tempPath), tempPath) == 0) {
        return std::wstring();
    }
    SYSTEMTIME st;
    GetLocalTime(&st);
    return std::format(L"{}WinMute_{:04}-{:02}-{:02}_{:02}-{:02}-{:02}.dmp",
                       tempPath, st.wYear, st.wMonth, st.wDay, st.wHour,
                       st.wMinute, st.wSecond);
}

struct DumpJob {
    const std::wstring* path;
    const std::wstring* comment;
    bool success;
};

// MiniDumpWriteDump should not be called on the thread being dumped, as its
// stack would be captured in the middle of the dump code. A helper thread
// dumps the process while the crashing thread waits, so the dump shows the
// crashing thread's stack, including the frames below the throw.
DWORD WINAPI WriteDumpThread(LPVOID param)
{
    auto& job = *static_cast<DumpJob*>(param);

    const HANDLE hFile =
        CreateFileW(job.path->c_str(), GENERIC_WRITE, 0, nullptr,
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return 1;
    }

    // Shown by WinDbg when the dump is opened, so the exception message
    // travels with the dump.
    MINIDUMP_USER_STREAM commentStream;
    commentStream.Type = CommentStreamW;
    commentStream.BufferSize =
        static_cast<ULONG>((job.comment->size() + 1) * sizeof(wchar_t));
    commentStream.Buffer = const_cast<wchar_t*>(job.comment->c_str());
    MINIDUMP_USER_STREAM_INFORMATION userStreams;
    userStreams.UserStreamCount = 1;
    userStreams.UserStreamArray = &commentStream;

    // Small dumps with enough memory to inspect the exception object and
    // locals. Full memory dumps would also contain far more user data.
    const auto dumpType = static_cast<MINIDUMP_TYPE>(
        MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithThreadInfo |
        MiniDumpWithUnloadedModules);

    job.success = miniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                                    hFile, dumpType, nullptr, &userStreams,
                                    nullptr) != FALSE;
    CloseHandle(hFile);
    if (!job.success) {
        DeleteFileW(job.path->c_str());
    }
    return 0;
}

bool WriteMiniDump(const std::wstring& path, const std::wstring& comment)
{
    if (miniDumpWriteDump == nullptr || path.empty()) {
        return false;
    }
    DumpJob job{&path, &comment, false};
    const HANDLE hThread =
        CreateThread(nullptr, 0, WriteDumpThread, &job, 0, nullptr);
    if (hThread == nullptr) {
        return false;
    }
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    return job.success;
}

void HandleCrash() noexcept
{
    if (crashHandled.test_and_set()) {
        return;
    }
    try {
        const std::wstring message = GetCurrentExceptionMessage();
        const std::wstring dumpPath = GetDumpFilePath();

        std::wstring text = message;
        if (WriteMiniDump(dumpPath, message)) {
            text += L"\n\nA crash dump was saved to:\n" + dumpPath +
                    L"\n\nPlease attach it when reporting this problem.";
        }
        TaskDialog(nullptr, nullptr, PROGRAM_NAME, L"WinMute has crashed",
                   text.c_str(), TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
    } catch (...) {
        // Nothing sensible left to do; let the process die.
    }
}

[[noreturn]] void OnTerminate()
{
    HandleCrash();
    std::abort();
}

// The MSVC runtime keeps terminate handlers per thread, so std::set_terminate
// only covers the thread that calls it. An exception escaping a worker thread
// (e.g. a std::jthread) still ends in the default terminate -> abort(), and
// the SIGABRT handler is process-wide, so it catches those.
void OnAbortSignal(int)
{
    HandleCrash();
    // Returning lets abort() continue: it fast-fails into Windows Error
    // Reporting, so the crash still shows up in the Event Viewer.
}

}  // namespace

void CrashHandler::Install()
{
    const HMODULE hDbgHelp =
        LoadLibraryExW(L"dbghelp.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (hDbgHelp != nullptr) {
        miniDumpWriteDump = reinterpret_cast<MiniDumpWriteDumpFn>(
            GetProcAddress(hDbgHelp, "MiniDumpWriteDump"));
    }

    std::set_terminate(OnTerminate);
    std::signal(SIGABRT, OnAbortSignal);
}
