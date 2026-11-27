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

#include "WMLog.h"

#include "common.h"

static const wchar_t* LogLevelToString(LogLevel level)
{
    switch (level) {
        case LogLevel::Debug:
            return L"DEBUG";
        case LogLevel::Info:
            return L"INFO";
        case LogLevel::Warning:
            return L"WARNING";
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

WMLog::WMLog() : enabled_(false)
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
        std::wstring path{tempPath};
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

std::wstring WMLog::FormatLogMessage(const LogMessage& logMsg,
                                     bool new_line) const
{
    // Same output as "%Ez" on a zoned_time, e.g. "-05:00".
    const auto offsetMinutes = logMsg.utcOffset.count();
    const auto absOffset = offsetMinutes < 0 ? -offsetMinutes : offsetMinutes;
    auto msg = std::format(L"{:%F %T}{}{:02}:{:02}  {}:  {}{}", logMsg.time,
                           offsetMinutes < 0 ? L'-' : L'+', absOffset / 60,
                           absOffset % 60, LogLevelToString(logMsg.level),
                           logMsg.message, new_line ? L"\r\n" : L"");
    return msg;
}

// Determines the UTC offset via the Win32 API. Used when the C++ tzdb cannot
// resolve the system time zone.
static std::chrono::minutes GetWin32UtcOffset(
    std::chrono::sys_time<std::chrono::milliseconds> now)
{
    namespace ch = std::chrono;

    // FILETIME counts 100ns intervals since 1601-01-01.
    constexpr long long kUnixEpochAsFileTime = 116444736000000000LL;
    const ULONGLONG utcTicks =
        static_cast<ULONGLONG>(now.time_since_epoch().count()) * 10000ULL +
        kUnixEpochAsFileTime;

    FILETIME ftUtc;
    ftUtc.dwLowDateTime = static_cast<DWORD>(utcTicks & 0xFFFFFFFF);
    ftUtc.dwHighDateTime = static_cast<DWORD>(utcTicks >> 32);

    SYSTEMTIME stUtc;
    SYSTEMTIME stLocal;
    FILETIME ftLocal;
    if (!FileTimeToSystemTime(&ftUtc, &stUtc) ||
        !SystemTimeToTzSpecificLocalTime(nullptr, &stUtc, &stLocal) ||
        !SystemTimeToFileTime(&stLocal, &ftLocal))
    {
        return ch::minutes{0};
    }
    const ULONGLONG localTicks =
        (static_cast<ULONGLONG>(ftLocal.dwHighDateTime) << 32) |
        ftLocal.dwLowDateTime;
    const auto diffTicks = static_cast<long long>(localTicks) -
                           static_cast<long long>(utcTicks);
    // Round to whole minutes; the conversion drops sub-millisecond precision.
    return ch::round<ch::minutes>(ch::milliseconds{diffTicks / 10000});
}

static std::chrono::minutes GetUtcOffset(
    std::chrono::sys_time<std::chrono::milliseconds> now)
{
    namespace ch = std::chrono;

    // current_zone() throws if the system time zone reported by ICU is not
    // in the tzdb (issue #59). Once that happened, don't retry on every log
    // call but stick with the Win32 fallback.
    static std::atomic<bool> tzdbFailed{false};
    if (!tzdbFailed.load(std::memory_order_relaxed)) {
        try {
            const auto info = ch::current_zone()->get_info(now);
            return ch::duration_cast<ch::minutes>(info.offset);
        } catch (const std::exception&) {
            tzdbFailed.store(true, std::memory_order_relaxed);
        }
    }
    return GetWin32UtcOffset(now);
}

void WMLog::StoreMessage(LogLevel level, const wchar_t* msg)
{
    namespace ch = std::chrono;

    const auto now = ch::floor<ch::milliseconds>(ch::system_clock::now());

    LogMessage lm;
    lm.level = level;
    lm.message = msg;
    lm.utcOffset = GetUtcOffset(now);
    lm.time = ch::local_time<ch::milliseconds>{now.time_since_epoch() +
                                               lm.utcOffset};

    {
        // logMutex_ also guards logFile_, which EnableLogFile can close
        // concurrently from another thread.
        const std::scoped_lock<std::mutex> lock(logMutex_);
        if (enabled_ && logFile_.is_open()) {
            const auto logMsg = FormatLogMessage(lm, true);
            logFile_.write(logMsg.c_str(), logMsg.length());
            logFile_.flush();
        }
    }

    {
        // Copy the window list first: holding wndMutex_ across SendMessageW
        // deadlocks if the UI thread is simultaneously blocked on wndMutex_
        // in (Un)RegisterForLogUpdates.
        std::vector<HWND> logWindows;
        {
            const std::scoped_lock<std::mutex> lock(wndMutex_);
            logWindows = registeredWindows_;
        }
        for (const auto hWnd : logWindows) {
            if (IsWindow(hWnd)) {
                SendMessageW(hWnd, WM_LOG_UPDATED, 0,
                             reinterpret_cast<LPARAM>(&lm));
            }
        }
    }

    {
        const std::scoped_lock<std::mutex> lock(logMutex_);
        logMessages_.push_back(std::move(lm));
        if (logMessages_.size() >= kMaxLogEntries_) {
            logMessages_.erase(logMessages_.begin(),
                               logMessages_.begin() +
                                   (logMessages_.size() - kMaxLogEntries_ / 2));
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
    if (std::find(registeredWindows_.begin(), registeredWindows_.end(), hWnd) ==
        registeredWindows_.end())
    {
        registeredWindows_.push_back(hWnd);
    }
}

void WMLog::UnregisterForLogUpdates(HWND hWnd)
{
    const std::scoped_lock<std::mutex> lock(wndMutex_);
    if (const auto it = std::find(registeredWindows_.begin(),
                                  registeredWindows_.end(), hWnd);
        it != registeredWindows_.end())
    {
        registeredWindows_.erase(it);
    }
}

void WMLog::LogWinError(const wchar_t* functionName, DWORD lastError)
{
    // Retrieve the system error message for the last-error code
    if (lastError == -1) {
        lastError = GetLastError();
    }

    LPVOID lpMsgBuf;
    if (FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<wchar_t*>(&lpMsgBuf), 0, nullptr) == 0)
    {
        return;
    }
    const std::wstring errorMsgText{reinterpret_cast<wchar_t*>(lpMsgBuf)};
    const std::wstring errorMsg = SafeVFormat(
        WMi18n::GetInstance().GetTranslationW("general.error.winapi.text"),
        functionName, lastError, errorMsgText);
    StoreMessage(LogLevel::Error, errorMsg.c_str());
    LocalFree(lpMsgBuf);
}
