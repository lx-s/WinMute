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

#include "WinMute.h"

#include "WinAudio.h"
#include "common.h"

extern HINSTANCE hglobInstance;
extern INT_PTR CALLBACK AboutDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR CALLBACK SettingsDlgProc(HWND, UINT, WPARAM, LPARAM);

static const wchar_t* WINMUTE_CLASS_NAME = L"WinMute";

static const wchar_t* TERMINAL_SERVER_KEY =
    L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\";
static const wchar_t* GLASS_SESSION_ID = L"GlassSessionId";

static constexpr UINT_PTR WTS_RETRY_TIMER_ID = 190503;
// When WinMute is started from autostart, the services Remote Desktop
// Services depends on can still be coming up, which makes
// WTSRegisterSessionNotification fail with RPC_S_INVALID_BINDING. Retry for
// roughly half a minute before giving up.
static constexpr UINT WTS_RETRY_INTERVAL_MS = 1000;
static constexpr int WTS_RETRY_MAX_ATTEMPTS = 30;

static constexpr int GLOBAL_HOTKEY_ID_MUTE = 1000;

extern void ShowLogDialog(HWND hParent);

static LRESULT CALLBACK WinMuteWndProc(HWND hWnd, UINT msg, WPARAM wParam,
                                       LPARAM lParam)
{
    auto wm = reinterpret_cast<WinMute*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    switch (msg) {
        case WM_NCCREATE: {
            LPCREATESTRUCTW cs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
            SetWindowLongPtrW(hWnd, GWLP_USERDATA,
                              reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return TRUE;
        }
        default:
            break;
    }
    return (wm) ? wm->WindowProc(hWnd, msg, wParam, lParam)
                : DefWindowProcW(hWnd, msg, wParam, lParam);
}

static void CALLBACK WtsRetryTimerProc(HWND hWnd, UINT, UINT_PTR, DWORD)
{
    auto wm =
        reinterpret_cast<WinMute*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    if (wm != nullptr) {
        wm->RetrySessionNotification();
    }
}

static bool IsCurrentSessionRemoteable() noexcept
{
    bool isRemoteable = false;

    if (GetSystemMetrics(SM_REMOTESESSION)) {
        isRemoteable = true;
    } else {
        HKEY hRegKey = nullptr;
        LONG lResult;

        lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, TERMINAL_SERVER_KEY, 0,
                                KEY_READ, &hRegKey);
        if (lResult == ERROR_SUCCESS) {
            DWORD dwGlassSessionId = 0;
            DWORD cbGlassSessionId = sizeof(dwGlassSessionId);
            DWORD dwType;

            lResult = RegQueryValueExW(
                hRegKey, GLASS_SESSION_ID, nullptr, &dwType,
                reinterpret_cast<BYTE*>(&dwGlassSessionId), &cbGlassSessionId);

            if (lResult == ERROR_SUCCESS && dwType == REG_DWORD &&
                cbGlassSessionId == sizeof(dwGlassSessionId))
            {
                DWORD dwCurrentSessionId;
                if (ProcessIdToSessionId(GetCurrentProcessId(),
                                         &dwCurrentSessionId))
                {
                    isRemoteable = (dwCurrentSessionId != dwGlassSessionId);
                }
            }
            RegCloseKey(hRegKey);
        }
    }

    return isRemoteable;
}

WinMute::MuteConfig::MuteConfig()
    : muteOnWlan(false), muteOnBluetooth(false), showNotifications(false)
{
}

WinMute::WinMute(WMSettings& settings)
    : hWnd_(nullptr),
      hTrayMenu_(nullptr),
      hAppIcon_(nullptr),
      hTrayIcon_(nullptr),
      hUpdateIcon_(nullptr),
      settings_(settings),
      i18n_(WMi18n::GetInstance())
{
}

WinMute::~WinMute() noexcept
{
    Unload();
}

bool WinMute::RegisterWindowClass()
{
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(wc);
    wc.hIcon = LoadIcon(hglobInstance, MAKEINTRESOURCE(IDI_APP));
    // System color index brush: no GDI object to leak, tracks theme changes
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.hInstance = hglobInstance;
    wc.lpfnWndProc = WinMuteWndProc;
    wc.lpszClassName = WINMUTE_CLASS_NAME;

    if (!RegisterClassExW(&wc)) {
        ShowWindowsError(L"RegisterClass");
        return false;
    }
    return true;
}

bool WinMute::InitWindow()
{
    hWnd_ =
        CreateWindowExW(WS_EX_TOOLWINDOW, WINMUTE_CLASS_NAME, PROGRAM_NAME,
                        WS_POPUP, 0, 0, 0, 0, nullptr, 0, hglobInstance, this);
    if (hWnd_ == nullptr) {
        ShowWindowsError(L"CreateWindowEx");
        return false;
    }
    return true;
}

bool WinMute::InitAudio()
{
    if (IsWindowsVistaOrGreater()) {
        // Nothing to do
    } else if (IsWindowsXPOrGreater()) {
        TaskDialog(
            nullptr, nullptr, PROGRAM_NAME,
            i18n_.GetTranslationW("init.error.winmute.platform-support.title")
                .c_str(),
            i18n_.GetTranslationW("init.error.winmute.platform-support.text")
                .c_str(),
            TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
        return false;
    }

    if (!muteCtrl_.Init(hWnd_, &wmTray_)) {
        return false;
    }

    return true;
}

#define CHECK_MENU_ITEM(id, cond)                \
    (CheckMenuItem(hTrayMenu_, ID_TRAYMENU_##id, \
                   (cond) ? MF_CHECKED : MF_UNCHECKED) != -1)
bool WinMute::InitTrayMenu()
{
    if (hTrayMenu_ == nullptr) {
        hTrayMenu_ = LoadMenuW(hglobInstance, MAKEINTRESOURCE(IDR_TRAYMENU));
        if (hTrayMenu_ == nullptr) {
            ShowWindowsError(L"LoadMenu");
            return false;
        }
    }

    if (!CHECK_MENU_ITEM(MUTE, muteCtrl_.IsMuted()) ||
        !CHECK_MENU_ITEM(MUTEONLOCK, muteCtrl_.GetMuteOnWorkstationLock()) ||
        !CHECK_MENU_ITEM(RESTOREAUDIO, muteCtrl_.GetRestoreVolume()) ||
        !CHECK_MENU_ITEM(MUTEONSUSPEND, muteCtrl_.GetMuteOnSuspend()) ||
        !CHECK_MENU_ITEM(MUTEONSHUTDOWN, muteCtrl_.GetMuteOnShutdown()) ||
        !CHECK_MENU_ITEM(MUTEONLOGOUT, muteCtrl_.GetMuteOnLogout()))
    {
        return false;
    }

    LoadMainMenuText();
    return true;
}
#undef CHECK_MENU_ITEM

bool WinMute::TryRegisterSessionNotification()
{
    if (wtsSessionNotificationRegistered_) {
        return true;
    }
    if (!WTSRegisterSessionNotification(hWnd_, NOTIFY_FOR_THIS_SESSION)) {
        WMLog::GetInstance().LogWinError(L"WTSRegisterSessionNotification",
                                         GetLastError());
        return false;
    }
    wtsSessionNotificationRegistered_ = true;
    return true;
}

void WinMute::StartSessionNotificationRetry()
{
    WMLog& log = WMLog::GetInstance();

    if (wtsRetryTimerId_ != 0) {
        return;
    }
    if (SetTimer(hWnd_, WTS_RETRY_TIMER_ID, WTS_RETRY_INTERVAL_MS,
                 WtsRetryTimerProc) == 0)
    {
        log.LogWinError(L"SetTimer (session notification retry)",
                        GetLastError());
        log.LogError(L"Muting on workstation lock is not available");
        return;
    }
    wtsRetryTimerId_ = WTS_RETRY_TIMER_ID;
    wtsRetryAttempts_ = 0;
    log.LogInfo(L"Retrying to register for session notifications every {} ms",
                WTS_RETRY_INTERVAL_MS);
}

void WinMute::StopSessionNotificationRetry() noexcept
{
    if (wtsRetryTimerId_ != 0) {
        KillTimer(hWnd_, wtsRetryTimerId_);
        wtsRetryTimerId_ = 0;
    }
}

void WinMute::RetrySessionNotification()
{
    WMLog& log = WMLog::GetInstance();

    ++wtsRetryAttempts_;
    if (TryRegisterSessionNotification()) {
        StopSessionNotificationRetry();
        log.LogInfo(L"Registered for session notifications after {} attempts",
                    wtsRetryAttempts_);
        return;
    }
    if (wtsRetryAttempts_ >= WTS_RETRY_MAX_ATTEMPTS) {
        StopSessionNotificationRetry();
        log.LogError(
            L"Giving up on registering for session notifications after {} "
            L"attempts. Muting on workstation lock is not available. "
            L"Please check if TermService is running.",
            wtsRetryAttempts_);
        wmTray_.ShowPopup(
            i18n_.GetTranslationW("popup.session-notification-failed.title"),
            i18n_.GetTranslationW("popup.session-notification-failed.text"));
    }
}

bool WinMute::Init()
{
    WMLog& log = WMLog::GetInstance();

    hAppIcon_ = LoadIconW(hglobInstance, MAKEINTRESOURCE(IDI_APP));

#ifdef _DEBUG
    WMLog::GetInstance().EnableLogFile(true);
#else
    WMLog::GetInstance().EnableLogFile(
        settings_.QueryValue(SettingsKey::LOGGING_ENABLED));
#endif
    log.LogInfo(L"Starting new session...");

    if (!RegisterWindowClass() || !InitWindow()) {
        return false;
    }

    if (!InitAudio()) {
        return false;
    }

    if (!LoadSettings()) {
        return false;
    }

    if (!InitTrayMenu()) {
        return false;
    }

    // Try to register session notification. This is needed for mute-on-lock
    // to work. If it fails, try again for a few times in the background,
    // but don't block startup on it.
    if (!TryRegisterSessionNotification()) {
        StartSessionNotificationRetry();
    }

    hPowerNotify_ = RegisterPowerSettingNotification(
        hWnd_, &GUID_CONSOLE_DISPLAY_STATE, DEVICE_NOTIFY_WINDOW_HANDLE);
    if (hPowerNotify_ == nullptr) {
        const DWORD lastError = GetLastError();
        ShowWindowsError(L"RegisterPowerSettingNotification", lastError);
        log.LogWinError(L"RegisterPowerSettingNotification", lastError);
        return false;
    }

    hLidCloseNotify_ = RegisterPowerSettingNotification(
        hWnd_, &GUID_LIDSWITCH_STATE_CHANGE, DEVICE_NOTIFY_WINDOW_HANDLE);
    if (hLidCloseNotify_ == nullptr) {
        const DWORD lastError = GetLastError();
        ShowWindowsError(L"RegisterPowerSettingNotification", lastError);
        log.LogWinError(L"RegisterPowerSettingNotification", lastError);
        UnregisterPowerSettingNotification(hPowerNotify_);
        return false;
    }

    hTrayIcon_ = LoadIconW(hglobInstance, MAKEINTRESOURCE(IDI_APP));
    if (hTrayIcon_ == nullptr) {
        ShowWindowsError(L"LoadIcon");
        return false;
    }
    hUpdateIcon_ = LoadIconW(hglobInstance, MAKEINTRESOURCE(IDI_APPUPDATE));
    if (hUpdateIcon_ == nullptr) {
        ShowWindowsError(L"LoadIcon");
        return false;
    }
    wmTray_.Init(hWnd_, 0, hTrayIcon_, L"WinMute", true);
    updateTray_.Init(hWnd_, 1, hUpdateIcon_, L"WinMute Update", false,
                     WM_WINMUTE_UPDATE_POPUP);

    quietHours_.Init(hWnd_, settings_);

    log.LogInfo(L"WinMute initialized");

    if (settings_.QueryValue(SettingsKey::MUTE_ON_RDP) &&
        IsCurrentSessionRemoteable())
    {
        wmTray_.ShowPopup(
            i18n_.GetTranslationW("popup.remote-session-detected.title"),
            i18n_.GetTranslationW("popup.remote-session-detected.text"));
        muteCtrl_.SetMute(true);
    }

    CheckForUpdates();

    return true;
}

bool WinMute::LoadSettings()
{
    WMLog& log = WMLog::GetInstance();

    std::wstring versionNumber;
    GetWinMuteVersion(versionNumber);
    log.LogInfo(L"Starting WinMute {}", versionNumber);
    log.LogInfo(L"Loading settings:");
    log.LogInfo(L"Enable global mute hotkey: {}",
                settings_.QueryValue(SettingsKey::ENABLE_GLOBAL_MUTE_HOTKEY)
                    ? L"Yes"
                    : L"No");

    std::wstring hotKeyStr;
    const auto hotkeyValue =
        settings_.QueryValue(SettingsKey::GLOBAL_MUTE_HOTKEY);
    UINT hkMods = LOWORD(HIBYTE(hotkeyValue));
    wchar_t vk = static_cast<wchar_t>(
        MapVirtualKeyW(LOWORD(LOBYTE(hotkeyValue)), MAPVK_VK_TO_CHAR));
    if (hkMods & HOTKEYF_ALT) {
        hotKeyStr += L"Alt + ";
    }
    if (hkMods & HOTKEYF_CONTROL) {
        hotKeyStr += L"Ctrl + ";
    }
    if (hkMods & HOTKEYF_SHIFT) {
        hotKeyStr += L"Shift + ";
    }
    hotKeyStr += vk;
    log.LogInfo(L"\tHotkey: {}", hotKeyStr);

    log.LogInfo(
        L"Check for updates: {}",
        settings_.QueryValue(SettingsKey::CHECK_FOR_UPDATE) ? L"Yes" : L"No");
    log.LogInfo(L"\tCheck for beta updates: {}",
                settings_.QueryValue(SettingsKey::CHECK_FOR_BETA_UPDATE)
                    ? L"Yes"
                    : L"No");
    log.LogInfo(
        L"\tRestore volume: {}",
        settings_.QueryValue(SettingsKey::RESTORE_AUDIO) ? L"Yes" : L"No");
    log.LogInfo(L"\tMute delay: {}",
                settings_.QueryValue(SettingsKey::MUTE_DELAY));
    log.LogInfo(
        L"\tMute on lock: {}",
        settings_.QueryValue(SettingsKey::MUTE_ON_LOCK) ? L"Yes" : L"No");
    log.LogInfo(L"\tMute on display standby: {}",
                settings_.QueryValue(SettingsKey::MUTE_ON_DISPLAYSTANDBY)
                    ? L"Yes"
                    : L"No");
    log.LogInfo(
        L"\tMute on display lid close: {}",
        settings_.QueryValue(SettingsKey::MUTE_ON_LIDCLOSE) ? L"Yes" : L"No");
    log.LogInfo(
        L"\tMute on logout: {}",
        settings_.QueryValue(SettingsKey::MUTE_ON_LOGOUT) ? L"Yes" : L"No");
    log.LogInfo(
        L"\tMute on suspend: {}",
        settings_.QueryValue(SettingsKey::MUTE_ON_SUSPEND) ? L"Yes" : L"No");
    log.LogInfo(
        L"\tMute on shutdown: {}",
        settings_.QueryValue(SettingsKey::MUTE_ON_SHUTDOWN) ? L"Yes" : L"No");
    log.LogInfo(L"\tShow notifications: {}",
                settings_.QueryValue(SettingsKey::NOTIFICATIONS_ENABLED)
                    ? L"Yes"
                    : L"No");
    log.LogInfo(
        L"\tMute on bluetooth: {}",
        settings_.QueryValue(SettingsKey::MUTE_ON_BLUETOOTH) ? L"Yes" : L"No");
    log.LogInfo(L"\tTry pausing media when muting: {}",
                settings_.QueryValue(SettingsKey::MUTE_TRY_PAUSE_MEDIA)
                    ? L"Yes"
                    : L"No");
    log.LogInfo(L"\tTry resuming media when unmuting: {}",
                settings_.QueryValue(SettingsKey::MUTE_TRY_RESUME_MEDIA)
                    ? L"Yes"
                    : L"No");
    log.LogInfo(L"\t\tUse devicelist: {}",
                settings_.QueryValue(SettingsKey::MUTE_ON_BLUETOOTH_DEVICELIST)
                    ? L"Yes"
                    : L"No");
    log.LogInfo(
        L"\tMute on WLAN: {}",
        settings_.QueryValue(SettingsKey::MUTE_ON_WLAN) ? L"Yes" : L"No");
    log.LogInfo(L"\t\tUse allowlist: {}",
                settings_.QueryValue(SettingsKey::MUTE_ON_WLAN_ALLOWLIST)
                    ? L"Yes"
                    : L"No");
    log.LogInfo(L"\tMute specific endpoints only: {}",
                settings_.QueryValue(SettingsKey::MUTE_INDIVIDUAL_ENDPOINTS)
                    ? L"Yes"
                    : L"No");

    if (!settings_.QueryValue(SettingsKey::MUTE_INDIVIDUAL_ENDPOINTS)) {
        muteCtrl_.ClearManagedEndpoints();
    } else {
        // Not done in WMSettings::Init: this needs COM, which is only
        // initialized once WinMute itself starts up.
        settings_.MigrateManagedEndpointIds();
        const auto endpoints = settings_.GetManagedAudioEndpoints();
        const bool isAllowList =
            settings_.QueryValue(SettingsKey::MUTE_INDIVIDUAL_ENDPOINTS_MODE) ==
            MUTE_ENDPOINT_MODE_INDIVIDUAL_ALLOW_LIST;
        muteCtrl_.SetManagedEndpoints(endpoints, isAllowList);
    }
    muteCtrl_.SetMuteDelay(settings_.QueryValue(SettingsKey::MUTE_DELAY));
    muteCtrl_.SetRestoreVolume(
        settings_.QueryValue(SettingsKey::RESTORE_AUDIO));
    muteCtrl_.SetMuteOnWorkstationLock(
        settings_.QueryValue(SettingsKey::MUTE_ON_LOCK));
    muteCtrl_.SetMuteOnDisplayStandby(
        settings_.QueryValue(SettingsKey::MUTE_ON_DISPLAYSTANDBY));
    muteCtrl_.SetMuteOnLidClose(
        settings_.QueryValue(SettingsKey::MUTE_ON_LIDCLOSE));
    muteCtrl_.SetMuteOnLogout(
        settings_.QueryValue(SettingsKey::MUTE_ON_LOGOUT));
    muteCtrl_.SetMuteOnSuspend(
        settings_.QueryValue(SettingsKey::MUTE_ON_SUSPEND));
    muteCtrl_.SetMuteOnShutdown(
        settings_.QueryValue(SettingsKey::MUTE_ON_SHUTDOWN));
    muteCtrl_.SetMuteTryPauseMedia(
        settings_.QueryValue(SettingsKey::MUTE_TRY_PAUSE_MEDIA));
    muteCtrl_.SetMuteTryResumeMedia(
        settings_.QueryValue(SettingsKey::MUTE_TRY_RESUME_MEDIA));

    muteCtrl_.SetNotifications(
        settings_.QueryValue(SettingsKey::NOTIFICATIONS_ENABLED));

    muteConfig_.showNotifications =
        settings_.QueryValue(SettingsKey::NOTIFICATIONS_ENABLED);

    muteConfig_.muteOnBluetooth =
        settings_.QueryValue(SettingsKey::MUTE_ON_BLUETOOTH);
    if (!muteConfig_.muteOnBluetooth) {
        muteCtrl_.SetMuteOnBluetoothDisconnect(false);
        btDetector_.Unload();
    } else {
        if (!btDetector_.Init(hWnd_)) {
            wmTray_.ShowPopup(
                i18n_.GetTranslationW("popup.bluetooth-muting-disabled.title"),
                i18n_.GetTranslationW("popup.bluetooth-muting-disabled.text"));
            settings_.SetValue(SettingsKey::MUTE_ON_BLUETOOTH, FALSE);
        } else {
            muteCtrl_.SetMuteOnBluetoothDisconnect(true);
            const bool muteOnWithDeviceList =
                settings_.QueryValue(SettingsKey::MUTE_ON_BLUETOOTH_DEVICELIST);
            btDetector_.SetDeviceList(settings_.GetBluetoothDevices(),
                                      muteOnWithDeviceList);
        }
    }

    muteConfig_.muteOnWlan = settings_.QueryValue(SettingsKey::MUTE_ON_WLAN);
    if (!muteConfig_.muteOnWlan) {
        wifiDetector_.Unload();
    } else {
        if (!wifiDetector_.Init(hWnd_)) {
            wmTray_.ShowPopup(
                i18n_.GetTranslationW("popup.wlan-muting-disabled.title"),
                i18n_.GetTranslationW("popup.wlan-muting-disabled.text"));
            settings_.SetValue(SettingsKey::MUTE_ON_WLAN, FALSE);
        } else {
            const bool isMuteList =
                !settings_.QueryValue(SettingsKey::MUTE_ON_WLAN_ALLOWLIST);
            wifiDetector_.SetNetworkList(settings_.GetWifiNetworks(),
                                         isMuteList);
            wifiDetector_.CheckNetwork();
        }
    }

    LoadGlobalHotkeys();

    return true;
}

void WinMute::ToggleMenuCheck(UINT item, bool* setting) noexcept
{
    UINT state = GetMenuState(hTrayMenu_, item, MF_BYCOMMAND);
    if (state & MF_CHECKED) {
        *setting = false;
        CheckMenuItem(hTrayMenu_, item, MF_UNCHECKED);
    } else {
        *setting = true;
        CheckMenuItem(hTrayMenu_, item, MF_CHECKED);
    }
}

void WinMute::UpdateMuteMenuCheck(bool muted) noexcept
{
    CheckMenuItem(hTrayMenu_, ID_TRAYMENU_MUTE,
                  muted ? MF_CHECKED : MF_UNCHECKED);
}

void WinMute::ToggleMute()
{
    UpdateMuteMenuCheck(muteCtrl_.ToggleMute());
}

void WinMute::LoadMainMenuText()
{
    std::map<UINT, std::wstring> menuText;
    menuText[ID_TRAYMENU_INFO] = i18n_.GetTranslationW("traymenu.info");
    menuText[ID_TRAYMENU_LABEL_MUTEWHEN] =
        i18n_.GetTranslationW("traymenu.mute-when");
    menuText[ID_TRAYMENU_MUTEONLOCK] =
        i18n_.GetTranslationW("traymenu.mute-on-lock");
    menuText[ID_TRAYMENU_MUTEONSCREENSUSPEND] =
        i18n_.GetTranslationW("traymenu.mute-on-screen-suspend");
    menuText[ID_TRAYMENU_RESTOREAUDIO] =
        i18n_.GetTranslationW("traymenu.restore-volume");
    menuText[ID_TRAYMENU_LABEL_MUTEON_NO_RESTORE] =
        i18n_.GetTranslationW("traymenu.mute-no-restore");
    menuText[ID_TRAYMENU_MUTEONSHUTDOWN] =
        i18n_.GetTranslationW("traymenu.mute-on-shutdown");
    menuText[ID_TRAYMENU_MUTEONSUSPEND] =
        i18n_.GetTranslationW("traymenu.mute-on-sleep");
    menuText[ID_TRAYMENU_MUTEONLOGOUT] =
        i18n_.GetTranslationW("traymenu.mute-on-logout");
    menuText[ID_TRAYMENU_MUTE] =
        i18n_.GetTranslationW("traymenu.mute-all-devices");
    menuText[ID_TRAYMENU_SHOWLOG] =
        i18n_.GetTranslationW("traymenu.show-log");
    menuText[ID_TRAYMENU_SETTINGS] = i18n_.GetTranslationW("traymenu.settings");
    menuText[ID_TRAYMENU_EXIT] = i18n_.GetTranslationW("traymenu.exit");

    for (const auto& mt : menuText) {
        MENUITEMINFO mii{sizeof(MENUITEMINFO)};
        if (!GetMenuItemInfo(hTrayMenu_, mt.first, false, &mii)) {
            continue;
        }
        mii.fMask = MIIM_TYPE;
        mii.fType = MFT_STRING;
        mii.dwTypeData = const_cast<LPWSTR>(mt.second.c_str());
        mii.cch = static_cast<UINT>(mt.second.length());
        if (!SetMenuItemInfo(hTrayMenu_, mt.first, false, &mii)) {
            continue;
        }
    }
}

void WinMute::LoadGlobalHotkeys()
{
    const auto hotKey = settings_.QueryValue(SettingsKey::GLOBAL_MUTE_HOTKEY);
    UINT modifiers = 0;
    UINT hkMods = LOWORD(HIBYTE(hotKey));
    UINT vk = LOWORD(LOBYTE(hotKey));
    if (hkMods & HOTKEYF_ALT) {
        modifiers |= MOD_ALT;
    }
    if (hkMods & HOTKEYF_CONTROL) {
        modifiers |= MOD_CONTROL;
    }
    if (hkMods & HOTKEYF_SHIFT) {
        modifiers |= MOD_SHIFT;
    }
    // The hotkey toggles, so auto-repeat while the key is held down would
    // make the mute state flap.
    modifiers |= MOD_NOREPEAT;

    // Ignore return value, we don't care if it was registered or not
    UnregisterHotKey(hWnd_, GLOBAL_HOTKEY_ID_MUTE);
    globalHotkeys_.erase(GLOBAL_HOTKEY_ID_MUTE);
    if (settings_.QueryValue(SettingsKey::ENABLE_GLOBAL_MUTE_HOTKEY)) {
        if (!RegisterHotKey(hWnd_, GLOBAL_HOTKEY_ID_MUTE, modifiers, vk)) {
            WMLog::GetInstance().LogWinError(L"RegisterHotKey", GetLastError());
            wmTray_.ShowPopup(
                i18n_.GetTranslationW(
                    "popup.error.global-mute-hotkey-register.title"),
                i18n_.GetTranslationW(
                    "popup.error.global-mute-hotkey-register.text"));
        } else {
            globalHotkeys_[GLOBAL_HOTKEY_ID_MUTE] = GlobalHotKey::Mute;
        }
    }
}

void WinMute::CheckForUpdates()
{
    const UpdateChecker updateChecker;
    if (!updateChecker.IsUpdateCheckEnabled(settings_)) {
        return;
    }
    if (updateThread_.joinable()) {  // Check still running
        return;
    }
    // The worker only uses its own locals and the raw window handle; the
    // result is marshalled back to the UI thread via
    // WM_WINMUTE_UPDATE_CHECK_DONE and handled in OnUpdateCheckDone. As a
    // std::jthread member, the thread is joined before this object dies.
    updateThread_ = std::jthread(&WinMute::CheckForUpdatesAsync, hWnd_);
}

void WinMute::CheckForUpdatesAsync(HWND hWnd)
{
    const UpdateChecker updateChecker;
    auto updateInfo = std::make_unique<UpdateInfo>();
    const bool success = updateChecker.GetUpdateInfo(*updateInfo);
    if (PostMessageW(hWnd, WM_WINMUTE_UPDATE_CHECK_DONE,
                     static_cast<WPARAM>(success),
                     reinterpret_cast<LPARAM>(updateInfo.get())))
    {
        // Ownership has passed to OnUpdateCheckDone
        updateInfo.release();
    }
}

LRESULT WinMute::OnUpdateCheckDone(HWND, WPARAM wParam, LPARAM lParam)
{
    const std::unique_ptr<UpdateInfo> updateInfo{
        reinterpret_cast<UpdateInfo*>(lParam)};
    if (updateThread_.joinable()) {
        updateThread_.join();
    }
    if (wParam == 0 || !updateInfo) {
        wmTray_.ShowPopup(
            i18n_.GetTranslationW("popup.error.update-check-failed.title"),
            i18n_.GetTranslationW("popup.error.update-check-failed.text"));
        return 0;
    }
    updateInfo_ = *updateInfo;
    const bool betaUpdates =
        settings_.QueryValue(SettingsKey::CHECK_FOR_BETA_UPDATE) != 0;
    if (updateInfo_.beta.shouldUpdate && betaUpdates) {
        const std::wstring popupTitle = SafeVFormat(
            i18n_.GetTranslationW("popup.update-available-beta.title"),
            updateInfo_.beta.version);
        const std::wstring popupText = SafeVFormat(
            i18n_.GetTranslationW("popup.update-available-beta.text"),
            updateInfo_.currentVersion);
        updateTray_.Show();
        updateTray_.ShowPopup(popupTitle, popupText);
    } else if (updateInfo_.stable.shouldUpdate) {
        const std::wstring popupTitle =
            SafeVFormat(i18n_.GetTranslationW("popup.update-available.title"),
                        updateInfo_.stable.version);
        const std::wstring popupText =
            SafeVFormat(i18n_.GetTranslationW("popup.update-available.text"),
                        updateInfo_.currentVersion);
        updateTray_.Show();
        updateTray_.ShowPopup(popupTitle, popupText);
    }
    return 0;
}

LRESULT WinMute::OnCommand(HWND hWnd, WPARAM wParam, LPARAM)
{
    switch (LOWORD(wParam)) {
        case ID_TRAYMENU_INFO: {
            static bool dialogOpen = false;
            if (!dialogOpen) {
                dialogOpen = true;
                DialogBox(hglobInstance, MAKEINTRESOURCE(IDD_ABOUT), hWnd_,
                          AboutDlgProc);
                dialogOpen = false;
            }
            break;
        }
        case ID_TRAYMENU_EXIT:
            SendMessage(hWnd, WM_CLOSE, 0, 0);
            break;
        case ID_TRAYMENU_SHOWLOG:
            ShowLogDialog(hWnd);
            break;
        case ID_TRAYMENU_SETTINGS: {
            static bool dialogOpen = false;
            if (!dialogOpen) {
                dialogOpen = true;
                if (DialogBoxParamW(hglobInstance,
                                    MAKEINTRESOURCE(IDD_SETTINGS), hWnd_,
                                    SettingsDlgProc,
                                    reinterpret_cast<LPARAM>(&settings_)) == 0)
                {
                    LoadSettings();
                    InitTrayMenu();
                    quietHours_.Reset(settings_);
                }
                dialogOpen = false;
            }
            break;
        }
        case ID_TRAYMENU_MUTE: {
            ToggleMute();
            break;
        }
        case ID_TRAYMENU_MUTEONLOCK: {
            bool checked = false;
            ToggleMenuCheck(ID_TRAYMENU_MUTEONLOCK, &checked);
            muteCtrl_.SetMuteOnWorkstationLock(checked);
            settings_.SetValue(SettingsKey::MUTE_ON_LOCK, checked);
            break;
        }
        case ID_TRAYMENU_RESTOREAUDIO: {
            bool checked = false;
            ToggleMenuCheck(ID_TRAYMENU_RESTOREAUDIO, &checked);
            muteCtrl_.SetRestoreVolume(checked);
            settings_.SetValue(SettingsKey::RESTORE_AUDIO, checked);
            break;
        }
        case ID_TRAYMENU_MUTEONSCREENSUSPEND: {
            bool checked = false;
            ToggleMenuCheck(ID_TRAYMENU_SCREENSUSPEND, &checked);
            muteCtrl_.SetMuteOnDisplayStandby(checked);
            settings_.SetValue(SettingsKey::MUTE_ON_DISPLAYSTANDBY, checked);
            break;
        }
        case ID_TRAYMENU_MUTEONSHUTDOWN: {
            bool checked = false;
            ToggleMenuCheck(ID_TRAYMENU_MUTEONSHUTDOWN, &checked);
            muteCtrl_.SetMuteOnShutdown(checked);
            settings_.SetValue(SettingsKey::MUTE_ON_SHUTDOWN, checked);
            break;
        }
        case ID_TRAYMENU_MUTEONSUSPEND: {
            bool checked = false;
            ToggleMenuCheck(ID_TRAYMENU_MUTEONSUSPEND, &checked);
            muteCtrl_.SetMuteOnSuspend(checked);
            settings_.SetValue(SettingsKey::MUTE_ON_SUSPEND, checked);
            break;
        }
        case ID_TRAYMENU_MUTEONLOGOUT: {
            bool checked = false;
            ToggleMenuCheck(ID_TRAYMENU_MUTEONLOGOUT, &checked);
            muteCtrl_.SetMuteOnLogout(checked);
            settings_.SetValue(SettingsKey::MUTE_ON_LOGOUT, checked);
            break;
        }
        default:
            break;
    }
    return 0;
}

LRESULT WinMute::OnTrayIcon(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    // NOTIFYICON_VERSION_4 semantics: LOWORD(lParam) carries the event,
    // HIWORD(lParam) the icon ID and wParam the anchor coordinates.
    const UINT event = LOWORD(lParam);
    if (event == WM_CONTEXTMENU || event == NIN_SELECT ||
        event == NIN_KEYSELECT)
    {
        const POINT p{GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam)};
        // Anything else can have changed the mute status in the meantime
        // (a mute event, quiet hours, or the Windows volume mixer), so
        // refresh the check right before the menu becomes visible.
        // TPM_NONOTIFY suppresses WM_INITMENUPOPUP, hence doing it here.
        UpdateMuteMenuCheck(muteCtrl_.IsMuted());
        SetForegroundWindow(hWnd);
        TrackPopupMenuEx(GetSubMenu(hTrayMenu_, 0),
                         TPM_NONOTIFY | TPM_TOPALIGN | TPM_LEFTALIGN, p.x, p.y,
                         hWnd_, nullptr);
    }
    return TRUE;
}

LRESULT WinMute::OnHotKey([[maybe_unused]] HWND hWnd, WPARAM wParam,
                          [[maybe_unused]] LPARAM lParam)
{
    // wParam is the id that was passed to RegisterHotKey.
    auto hotKeyIt = globalHotkeys_.find(static_cast<int>(wParam));
    if (hotKeyIt != globalHotkeys_.end()) {
        switch (hotKeyIt->second) {
            case GlobalHotKey::Mute:
                ToggleMute();
                break;
        }
    }
    return LRESULT();
}

LRESULT WinMute::OnUpdatePopup(HWND hWnd, WPARAM, LPARAM lParam)
{
    // NOTIFYICON_VERSION_4 semantics: LOWORD(lParam) carries the event.
    const UINT event = LOWORD(lParam);
    if (event == NIN_BALLOONUSERCLICK) {
        const bool betaUpdates =
            settings_.QueryValue(SettingsKey::CHECK_FOR_BETA_UPDATE) != 0;
        if (betaUpdates && updateInfo_.beta.shouldUpdate) {
            LaunchBrowser(hWnd, updateInfo_.beta.downloadUrl);
        } else if (updateInfo_.stable.shouldUpdate) {
            LaunchBrowser(hWnd, updateInfo_.stable.downloadUrl);
        }
    }
    if (event == NIN_BALLOONHIDE || event == NIN_BALLOONTIMEOUT ||
        event == NIN_BALLOONUSERCLICK)
    {
        updateTray_.Hide();
    }
    return 0;
}

LRESULT WinMute::OnSettingChange(HWND, WPARAM, LPARAM)
{
    return 0;
}

LRESULT WinMute::OnPowerBroadcast(HWND, WPARAM wParam, LPARAM lParam)
{
    if (wParam == PBT_APMSUSPEND) {
        muteCtrl_.NotifySuspend(true);
    } else if (wParam == PBT_POWERSETTINGCHANGE) {
        const PPOWERBROADCAST_SETTING bs =
            reinterpret_cast<PPOWERBROADCAST_SETTING>(lParam);
        if (IsEqualGUID(bs->PowerSetting, GUID_CONSOLE_DISPLAY_STATE)) {
            const DWORD state = bs->Data[0];
            if (state == 0x0) {  // Display standby
                muteCtrl_.NotifyDisplayStandby(true);
            } else if (state == 0x1) {  // Display on
                muteCtrl_.NotifyDisplayStandby(false);
            } else if (state == 0x2) {  // Display dimmed
            }
        } else if (IsEqualGUID(bs->PowerSetting, GUID_LIDSWITCH_STATE_CHANGE)) {
            const DWORD state = bs->Data[0];
            if (state == 0x0) {  // Lid closed
                muteCtrl_.NotifyLidClosed(true);
            } else if (state == 0x1) {  // Lid open
                muteCtrl_.NotifyLidClosed(false);
            }
        }
    }
    return TRUE;
}

LRESULT WinMute::OnQuietHours(HWND, UINT msg, WPARAM, LPARAM)
{
    if (msg == WM_WINMUTE_QUIETHOURS_START) {
        muteCtrl_.NotifyQuietHours(true);
        if (settings_.QueryValue(SettingsKey::QUIETHOURS_NOTIFICATIONS)) {
            wmTray_.ShowPopup(
                i18n_.GetTranslationW("popup.quiet-hours-started.title"),
                i18n_.GetTranslationW("popup.quiet-hours-started.text"));
        }
        quietHours_.SetEnd();
        return 0;
    } else if (msg == WM_WINMUTE_QUIETHOURS_END) {
        muteCtrl_.NotifyQuietHours(false);
        if (settings_.QueryValue(SettingsKey::QUIETHOURS_NOTIFICATIONS)) {
            wmTray_.ShowPopup(
                i18n_.GetTranslationW("popup.quiet-hours-ended.title"),
                i18n_.GetTranslationW("popup.quiet-hours-ended.text"));
        }
        if (settings_.QueryValue(SettingsKey::QUIETHOURS_FORCEUNMUTE)) {
            muteCtrl_.SetMute(false);
        }
        quietHours_.SetStart();
    }
    return 0;
}

LRESULT WinMute::OnAudioServiceShutdown(HWND hWnd, WPARAM, LPARAM)
{
    TaskDialog(
        hWnd, hglobInstance, PROGRAM_NAME,
        i18n_.GetTranslationW("general.error.audio-service-shutdown.title")
            .c_str(),
        i18n_.GetTranslationW("general.error.audio-service-shutdown.text")
            .c_str(),
        TDCBF_OK_BUTTON, TD_WARNING_ICON, nullptr);
    return 0;
}

LRESULT WinMute::OnDeviceChange(HWND, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (muteConfig_.muteOnBluetooth) {
        const auto btStatus =
            btDetector_.GetBluetoothStatus(msg, wParam, lParam);
        if (btStatus == BluetoothDetector::BluetoothStatus::Connected) {
            muteCtrl_.NotifyBluetoothConnected(true);
        } else if (btStatus == BluetoothDetector::BluetoothStatus::Disconnected)
        {
            muteCtrl_.NotifyBluetoothConnected(false);
        }
    }
    return TRUE;
}

LRESULT WinMute::OnWifiStatusChange(HWND, WPARAM wParam, LPARAM lParam)
{
    if (!muteConfig_.muteOnWlan) {
        return 0;
    }
    if (wParam != 1) {  // Not Connected
        return 0;
    }

    if (settings_.QueryValue(SettingsKey::NOTIFICATIONS_ENABLED)) {
        std::wstring popupMsg;
        // Non-owning: the name is only valid for the duration of this call.
        const wchar_t* wifiName = reinterpret_cast<const wchar_t*>(lParam);
        if (settings_.QueryValue(SettingsKey::MUTE_ON_WLAN_ALLOWLIST)) {
            popupMsg = SafeVFormat(
                i18n_.GetTranslationW("popup.wlan-not-on-mute-list.text"),
                wifiName);
        } else {
            popupMsg = SafeVFormat(
                i18n_.GetTranslationW("popup.wlan-is-on-mute-list.text"),
                wifiName);
        }
        wmTray_.ShowPopup(
            i18n_.GetTranslationW("popup.workstation-muted.title"), popupMsg);
    }
    muteCtrl_.SetMute(true);

    return 0;
}

LRESULT CALLBACK WinMute::WindowProc(HWND hWnd, UINT msg, WPARAM wParam,
                                     LPARAM lParam)
{
    static UINT uTaskbarRestart = 0;
    switch (msg) {
        case WM_CREATE:
            uTaskbarRestart = RegisterWindowMessageW(L"TaskbarCreated");
            return TRUE;
        case WM_COMMAND:
            return OnCommand(hWnd, wParam, lParam);
        case WM_WINMUTE_UPDATE_POPUP:
            return OnUpdatePopup(hWnd, wParam, lParam);
        case WM_TRAYICON:
            return OnTrayIcon(hWnd, wParam, lParam);
        case WM_CLOSE:
            Close();
            return 0;
        case WM_HOTKEY: {
            return OnHotKey(hWnd, wParam, lParam);
        }
        case WM_WTSSESSION_CHANGE: {
            if (wParam == WTS_SESSION_LOCK) {
                muteCtrl_.NotifyWorkstationLock(true);
            } else if (wParam == WTS_SESSION_UNLOCK) {
                muteCtrl_.NotifyWorkstationLock(false);
            }
            return 0;
        }
        case WM_POWERBROADCAST:
            return OnPowerBroadcast(hWnd, wParam, lParam);
        case WM_QUERYENDSESSION:
            return TRUE;
        case WM_ENDSESSION:
            if (wParam == TRUE) {
                if (lParam == 0) {  // Shutdown
                    muteCtrl_.NotifyShutdown();
                } else if ((lParam & ENDSESSION_LOGOFF)) {
                    muteCtrl_.NotifyLogout();
                }
            }
            break;
        case WM_WINMUTE_QUIETHOURS_START:  // fall through
        case WM_WINMUTE_QUIETHOURS_END:
            return OnQuietHours(hWnd, msg, wParam, lParam);
        case WM_WINMUTE_AUDIO_SERVICE_SHUTDOWN:
            return OnAudioServiceShutdown(hWnd, wParam, lParam);
        case WM_WINMUTE_AUDIO_DEVICE_ARRIVED:
            muteCtrl_.NotifyAudioDeviceArrived();
            return 0;
        case WM_DEVICECHANGE:
            return OnDeviceChange(hWnd, msg, wParam, lParam);
        case WM_WIFISTATUSCHANGED:
            return OnWifiStatusChange(hWnd, wParam, lParam);
        case WM_WINMUTE_UPDATE_CHECK_DONE:
            return OnUpdateCheckDone(hWnd, wParam, lParam);
        case WM_SETTINGCHANGE:
            return OnSettingChange(hWnd, wParam, lParam);
        default:
            if (msg ==
                uTaskbarRestart) {  // Restore trayicon if explorer.exe crashes
                wmTray_.Hide();
                wmTray_.Show();
            }
            break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void WinMute::Unload() noexcept
{
    if (hPowerNotify_ != nullptr) {
        UnregisterPowerSettingNotification(hPowerNotify_);
        hPowerNotify_ = nullptr;
    }
    if (hLidCloseNotify_ != nullptr) {
        UnregisterPowerSettingNotification(hLidCloseNotify_);
        hLidCloseNotify_ = nullptr;
    }
    StopSessionNotificationRetry();
    if (wtsSessionNotificationRegistered_) {
        WTSUnRegisterSessionNotification(hWnd_);
        wtsSessionNotificationRegistered_ = false;
    }
    settings_.Unload();
}

void WinMute::Close()
{
    Unload();
    PostQuitMessage(0);
}
