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

extern INT_PTR CALLBACK Settings_LanguageDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR CALLBACK Settings_UpdatesDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR CALLBACK Settings_HotkeysDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR CALLBACK Settings_LoggingDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR CALLBACK Settings_MuteDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR CALLBACK Settings_ManageEndpointsDlgProc(HWND, UINT, WPARAM,
                                                        LPARAM);
extern INT_PTR CALLBACK Settings_MediaDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR CALLBACK Settings_QuietHoursDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR CALLBACK Settings_WifiDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR CALLBACK Settings_BluetoothDlgProc(HWND, UINT, WPARAM, LPARAM);

extern HINSTANCE hglobInstance;

// The settings pages, in the order the navigation tree lists them. Every
// group below covers a contiguous run of this enum, so a group only has to
// name its first page and how many pages follow.
enum SettingsPageId {
    SETTINGS_PAGE_LANGUAGE = 0,
    SETTINGS_PAGE_UPDATES,
    SETTINGS_PAGE_HOTKEYS,
    SETTINGS_PAGE_LOGGING,
    SETTINGS_PAGE_MUTE,
    SETTINGS_PAGE_ENDPOINTS,
    SETTINGS_PAGE_MEDIA,
    SETTINGS_PAGE_QUIETHOURS,
    SETTINGS_PAGE_WIFI,
    SETTINGS_PAGE_BLUETOOTH,
    SETTINGS_PAGE_COUNT
};

struct SettingsPageDesc {
    int templateId;
    DLGPROC dlgProc;
    const char* titleId;
};

struct SettingsGroupDesc {
    const char* titleId;
    int firstPage;
    int pageCount;
};

static const SettingsPageDesc SETTINGS_PAGES[SETTINGS_PAGE_COUNT] = {
    {IDD_SETTINGS_LANGUAGE, Settings_LanguageDlgProc,
     "settings.nav.general.language"},
    {IDD_SETTINGS_UPDATES, Settings_UpdatesDlgProc,
     "settings.nav.general.startup"},
    {IDD_SETTINGS_HOTKEYS, Settings_HotkeysDlgProc,
     "settings.nav.general.hotkeys"},
    {IDD_SETTINGS_LOGGING, Settings_LoggingDlgProc,
     "settings.nav.general.logging"},
    {IDD_SETTINGS_MUTE, Settings_MuteDlgProc, "settings.nav.muting.events"},
    {IDD_SETTINGS_ENDPOINTS, Settings_ManageEndpointsDlgProc,
     "settings.nav.muting.endpoints"},
    {IDD_SETTINGS_MEDIA, Settings_MediaDlgProc, "settings.nav.muting.media"},
    {IDD_SETTINGS_QUIETHOURS, Settings_QuietHoursDlgProc,
     "settings.nav.triggers.quiet-hours"},
    {IDD_SETTINGS_WIFI, Settings_WifiDlgProc, "settings.nav.triggers.wifi"},
    {IDD_SETTINGS_BLUETOOTH, Settings_BluetoothDlgProc,
     "settings.nav.triggers.bluetooth"},
};

static const SettingsGroupDesc SETTINGS_GROUPS[] = {
    {"settings.nav.general", SETTINGS_PAGE_LANGUAGE, 4},
    {"settings.nav.muting", SETTINGS_PAGE_MUTE, 3},
    {"settings.nav.triggers", SETTINGS_PAGE_QUIETHOURS, 3},
};

// lParam of a tree item that only groups other items and has no page of its
// own.
static constexpr LPARAM SETTINGS_NAV_GROUP = -1;

// =============================================================================
// Page host
//
// A plain scrolling viewport: it holds exactly one visible page dialog at a
// time and moves it up and down behind its own client area. The pages keep
// the fixed layout of their dialog template, so only their height matters
// here -- a page taller than the viewport gets a scroll bar, everything else
// is shown as-is.

static const wchar_t* SETTINGS_HOST_CLASS = L"WinMuteSettingsPageHost";

struct PageHostData {
    HWND hPage = nullptr;
    int pageHeight = 0;  // natural height of hPage in pixels
    int scrollPos = 0;   // pixels of hPage scrolled off the top
    int lineHeight = 0;  // scroll step of one wheel/arrow line
};

static PageHostData* GetPageHostData(HWND hHost)
{
    return reinterpret_cast<PageHostData*>(
        GetWindowLongPtr(hHost, GWLP_USERDATA));
}

// Re-applies the current scroll position and stretches the page to the width
// of the viewport. Call whenever the page or the viewport size changes.
static void PageHostUpdateLayout(HWND hHost)
{
    PageHostData* host = GetPageHostData(hHost);
    if (host == nullptr) {
        return;
    }

    RECT clientRect = {0};
    GetClientRect(hHost, &clientRect);
    const int clientHeight = clientRect.bottom - clientRect.top;

    const int maxScroll = (std::max)(0, host->pageHeight - clientHeight);
    host->scrollPos = std::clamp(host->scrollPos, 0, maxScroll);

    // Windows hides the scroll bar on its own once the page covers the whole
    // range, which also gives the width used below.
    SCROLLINFO si = {0};
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = (host->pageHeight > 0) ? host->pageHeight - 1 : 0;
    si.nPage = clientHeight;
    si.nPos = host->scrollPos;
    SetScrollInfo(hHost, SB_VERT, &si, TRUE);

    if (host->hPage != nullptr) {
        GetClientRect(hHost, &clientRect);
        SetWindowPos(host->hPage, nullptr, 0, -host->scrollPos,
                     clientRect.right - clientRect.left, host->pageHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

static void PageHostScrollTo(HWND hHost, int newPos)
{
    PageHostData* host = GetPageHostData(hHost);
    if (host == nullptr || host->hPage == nullptr) {
        return;
    }

    RECT clientRect = {0};
    GetClientRect(hHost, &clientRect);
    const int clientHeight = clientRect.bottom - clientRect.top;
    const int maxScroll = (std::max)(0, host->pageHeight - clientHeight);
    newPos = std::clamp(newPos, 0, maxScroll);
    if (newPos == host->scrollPos) {
        return;
    }

    const int delta = host->scrollPos - newPos;
    host->scrollPos = newPos;
    SetScrollPos(hHost, SB_VERT, host->scrollPos, TRUE);
    ScrollWindowEx(hHost, 0, delta, nullptr, nullptr, nullptr, nullptr,
                   SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE);
    UpdateWindow(hHost);
}

static void PageHostSetPage(HWND hHost, HWND hPage, int pageHeight)
{
    PageHostData* host = GetPageHostData(hHost);
    if (host == nullptr || host->hPage == hPage) {
        return;
    }

    if (host->hPage != nullptr) {
        ShowWindow(host->hPage, SW_HIDE);
    }
    host->hPage = hPage;
    host->pageHeight = pageHeight;
    host->scrollPos = 0;
    PageHostUpdateLayout(hHost);
    if (host->hPage != nullptr) {
        ShowWindow(host->hPage, SW_SHOW);
    }
}

static LRESULT CALLBACK PageHostWndProc(HWND hWnd, UINT msg, WPARAM wParam,
                                        LPARAM lParam)
{
    PageHostData* host = GetPageHostData(hWnd);
    switch (msg) {
        case WM_CREATE: {
            PageHostData* newHost = new PageHostData;
            const CREATESTRUCT* cs =
                reinterpret_cast<const CREATESTRUCT*>(lParam);
            newHost->lineHeight =
                static_cast<int>(reinterpret_cast<INT_PTR>(cs->lpCreateParams));
            SetWindowLongPtr(hWnd, GWLP_USERDATA,
                             reinterpret_cast<LONG_PTR>(newHost));
            return 0;
        }
        case WM_NCDESTROY:
            delete host;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
            break;
        case WM_SIZE:
            PageHostUpdateLayout(hWnd);
            return 0;
        case WM_VSCROLL: {
            if (host == nullptr) {
                break;
            }
            RECT clientRect = {0};
            GetClientRect(hWnd, &clientRect);
            const int page = clientRect.bottom - clientRect.top;
            int newPos = host->scrollPos;
            switch (LOWORD(wParam)) {
                case SB_LINEUP:
                    newPos -= host->lineHeight;
                    break;
                case SB_LINEDOWN:
                    newPos += host->lineHeight;
                    break;
                case SB_PAGEUP:
                    newPos -= page;
                    break;
                case SB_PAGEDOWN:
                    newPos += page;
                    break;
                case SB_TOP:
                    newPos = 0;
                    break;
                case SB_BOTTOM:
                    newPos = host->pageHeight;
                    break;
                case SB_THUMBTRACK:
                case SB_THUMBPOSITION: {
                    // The 16 bit wParam position tops out at 65535, so ask for
                    // the full one.
                    SCROLLINFO si = {0};
                    si.cbSize = sizeof(si);
                    si.fMask = SIF_TRACKPOS;
                    if (GetScrollInfo(hWnd, SB_VERT, &si)) {
                        newPos = si.nTrackPos;
                    }
                    break;
                }
                default:
                    break;
            }
            PageHostScrollTo(hWnd, newPos);
            return 0;
        }
        case WM_MOUSEWHEEL: {
            if (host == nullptr) {
                break;
            }
            UINT scrollLines = 3;
            SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &scrollLines, 0);
            RECT clientRect = {0};
            GetClientRect(hWnd, &clientRect);
            const int step =
                (scrollLines == WHEEL_PAGESCROLL)
                    ? (clientRect.bottom - clientRect.top)
                    : static_cast<int>(scrollLines) * host->lineHeight;
            const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            PageHostScrollTo(
                hWnd, host->scrollPos - MulDiv(delta, step, WHEEL_DELTA));
            return 0;
        }
        default:
            break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

static bool RegisterPageHostClass()
{
    static bool registered = false;
    if (registered) {
        return true;
    }
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = PageHostWndProc;
    wc.hInstance = hglobInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = SETTINGS_HOST_CLASS;
    if (RegisterClassEx(&wc) == 0) {
        ShowWindowsError(L"RegisterClassEx", GetLastError());
        return false;
    }
    registered = true;
    return true;
}

// =============================================================================
// Settings dialog

struct SettingsDlgData {
    HWND hTree = nullptr;
    HWND hHost = nullptr;
    HWND hPages[SETTINGS_PAGE_COUNT] = {nullptr};
    int pageHeights[SETTINGS_PAGE_COUNT] = {0};
    WMSettings* settings = nullptr;

    explicit SettingsDlgData(WMSettings* settings) : settings(settings)
    {
    }
};

static HTREEITEM InsertNavItem(HWND hTree, HTREEITEM hParent,
                               const std::wstring& text, LPARAM data)
{
    TVINSERTSTRUCTW tvis = {0};
    tvis.hParent = (hParent == nullptr) ? TVI_ROOT : hParent;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvis.item.pszText = const_cast<LPWSTR>(text.c_str());
    tvis.item.lParam = data;
    return TreeView_InsertItem(hTree, &tvis);
}

// Returns the tree item of the given page, so the initial selection can be
// made once the whole tree exists.
static HTREEITEM BuildNavTree(HWND hTree, int selectPage)
{
    WMi18n& i18n = WMi18n::GetInstance();
    HTREEITEM hSelect = nullptr;

    for (const auto& group : SETTINGS_GROUPS) {
        const HTREEITEM hGroup =
            InsertNavItem(hTree, nullptr, i18n.GetTranslationW(group.titleId),
                          SETTINGS_NAV_GROUP);
        for (int i = 0; i < group.pageCount; ++i) {
            const int page = group.firstPage + i;
            const HTREEITEM hItem = InsertNavItem(
                hTree, hGroup,
                i18n.GetTranslationW(SETTINGS_PAGES[page].titleId), page);
            if (page == selectPage) {
                hSelect = hItem;
            }
        }
        TreeView_Expand(hTree, hGroup, TVE_EXPAND);
    }
    return hSelect;
}

// Places the page viewport in the gap the dialog template leaves to the right
// of the navigation tree.
static HWND CreatePageHost(HWND hDlg, HWND hTree)
{
    if (!RegisterPageHostClass()) {
        return nullptr;
    }

    RECT dlgRect = {0};
    GetClientRect(hDlg, &dlgRect);

    RECT treeRect = {0};
    GetWindowRect(hTree, &treeRect);
    MapWindowPoints(nullptr, hDlg, reinterpret_cast<LPPOINT>(&treeRect), 2);

    // Same gap between tree and pages as between tree and dialog border.
    RECT unit = {0, 0, 6, 8};
    MapDialogRect(hDlg, &unit);
    const int gap = unit.right;
    const int left = treeRect.right + gap;

    const HWND hHost = CreateWindowEx(
        WS_EX_CONTROLPARENT, SETTINGS_HOST_CLASS, nullptr,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_VSCROLL, left,
        treeRect.top, dlgRect.right - treeRect.left - left,
        treeRect.bottom - treeRect.top, hDlg, nullptr, hglobInstance,
        reinterpret_cast<LPVOID>(static_cast<INT_PTR>(unit.bottom)));
    if (hHost == nullptr) {
        ShowWindowsError(L"CreateWindowEx", GetLastError());
    }
    return hHost;
}

static void SwitchPage(SettingsDlgData* dlgData, int page)
{
    if (page < 0 || page >= SETTINGS_PAGE_COUNT) {
        return;
    }
    PageHostSetPage(dlgData->hHost, dlgData->hPages[page],
                    dlgData->pageHeights[page]);
}

INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                 LPARAM lParam)
{
    SettingsDlgData* dlgData =
        reinterpret_cast<SettingsDlgData*>(GetWindowLongPtr(hDlg, DWLP_USER));
    switch (msg) {
        case WM_INITDIALOG: {
            WMi18n& i18n = WMi18n::GetInstance();

            assert(dlgData == nullptr);
            WMSettings* settings = reinterpret_cast<WMSettings*>(lParam);
            dlgData = new SettingsDlgData(settings);
            SetWindowLongPtr(hDlg, DWLP_USER,
                             reinterpret_cast<LONG_PTR>(dlgData));

            SetWindowText(hDlg, i18n.GetTranslationW("settings.title").c_str());
            i18n.SetItemText(hDlg, IDOK, "settings.btn-save");
            i18n.SetItemText(hDlg, IDCANCEL, "settings.btn-cancel");

            dlgData->hTree = GetDlgItem(hDlg, IDC_SETTINGS_TREE);
            SetWindowTheme(dlgData->hTree, L"Explorer", nullptr);
            TreeView_SetExtendedStyle(dlgData->hTree, TVS_EX_DOUBLEBUFFER,
                                      TVS_EX_DOUBLEBUFFER);

            dlgData->hHost = CreatePageHost(hDlg, dlgData->hTree);
            if (dlgData->hHost == nullptr) {
                EndDialog(hDlg, 1);
                return TRUE;
            }

            for (int i = 0; i < SETTINGS_PAGE_COUNT; ++i) {
                const SettingsPageDesc& desc = SETTINGS_PAGES[i];
                HWND hPage = CreateDialogParam(
                    hglobInstance, MAKEINTRESOURCE(desc.templateId),
                    dlgData->hHost, desc.dlgProc,
                    reinterpret_cast<LPARAM>(settings));
                if (hPage == nullptr) {
                    ShowWindowsError(L"CreateDialogParam", GetLastError());
                    continue;
                }
                RECT pageRect = {0};
                GetWindowRect(hPage, &pageRect);
                dlgData->hPages[i] = hPage;
                dlgData->pageHeights[i] = pageRect.bottom - pageRect.top;
                ShowWindow(hPage, SW_HIDE);
            }

            const HTREEITEM hSelect =
                BuildNavTree(dlgData->hTree, SETTINGS_PAGE_LANGUAGE);
            SwitchPage(dlgData, SETTINGS_PAGE_LANGUAGE);
            if (hSelect != nullptr) {
                TreeView_SelectItem(dlgData->hTree, hSelect);
            }

            HICON hIcon = LoadIcon(GetModuleHandle(nullptr),
                                   MAKEINTRESOURCE(IDI_SETTINGS));
            SendMessageW(hDlg, WM_SETICON, ICON_BIG,
                         reinterpret_cast<LPARAM>(hIcon));

            return TRUE;
        }
        case WM_COMMAND:
            if (dlgData == nullptr) {
                return FALSE;
            }
            if (LOWORD(wParam) == IDOK) {
                for (int i = 0; i < SETTINGS_PAGE_COUNT; ++i) {
                    if (dlgData->hPages[i] != nullptr) {
                        SendMessage(dlgData->hPages[i], WM_SAVESETTINGS, 0, 0);
                    }
                }
                EndDialog(hDlg, 0);
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, 1);
            }
            return 0;
        case WM_NOTIFY: {
            // The tree already notifies while the dialog manager is building
            // the dialog, so this runs before WM_INITDIALOG.
            if (dlgData == nullptr) {
                return FALSE;
            }
            const LPNMHDR lpnmhdr = reinterpret_cast<LPNMHDR>(lParam);
            if (lpnmhdr->hwndFrom != dlgData->hTree) {
                return 0;
            }
#pragma warning(push)
#pragma warning( \
    disable : 26454)  // Disable arithmetic overflow warning for TVN_SELCHANGED
            if (lpnmhdr->code == TVN_SELCHANGED) {
#pragma warning(pop)
                const LPNMTREEVIEW nmtv =
                    reinterpret_cast<LPNMTREEVIEW>(lParam);
                if (nmtv->itemNew.lParam == SETTINGS_NAV_GROUP) {
                    // Group items carry no page of their own; show the first
                    // page below them instead.
                    const HTREEITEM hChild =
                        TreeView_GetChild(dlgData->hTree, nmtv->itemNew.hItem);
                    if (hChild != nullptr) {
                        TreeView_SelectItem(dlgData->hTree, hChild);
                    }
                } else {
                    SwitchPage(dlgData, static_cast<int>(nmtv->itemNew.lParam));
                }
            }
            return 0;
        }
        case WM_CLOSE:
            EndDialog(hDlg, 1);
            return TRUE;
        case WM_DESTROY:
            delete dlgData;
            SetWindowLongPtrW(hDlg, DWLP_USER, 0);
            return 0;
        default:
            break;
    }
    return FALSE;
}
