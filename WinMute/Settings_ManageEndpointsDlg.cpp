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

#include <Functiondiscoverykeys_devpkey.h>
#include <atlbase.h>
#include <mmdeviceapi.h>

#include "common.h"

// Passed as lParam to the add/edit dialog; owned by the calling dialog.
//
// The combo box is pick-only, so every entry created here is bound to a real
// endpoint id. The one exception is an entry carried over from an older
// version (or created before the device list included it): that name is added
// to the list as-is so editing does not silently discard it.
struct EndpointAddCtx {
    ManagedEndpoint* data = nullptr;
    std::vector<ManagedEndpoint> candidates;
};

// Item data of the "keep this legacy name" entry. Every other item carries an
// index into EndpointAddCtx::candidates -- the combo box is sorted, so the
// selection index on its own says nothing about which device was picked.
static constexpr LONG_PTR ENDPOINT_ITEM_LEGACY_NAME = -1;

// State of the endpoint list dialog. The list box only renders friendly
// names, so the endpoint ids have to be tracked alongside it.
struct ManageEndpointsDlgState {
    WMSettings* settings = nullptr;
    std::vector<ManagedEndpoint> entries;
};

static void RebuildEndpointList(HWND hList,
                                const std::vector<ManagedEndpoint>& entries)
{
    ListBox_ResetContent(hList);
    for (const auto& entry : entries) {
        ListBox_AddString(hList, entry.name.c_str());
    }
}

static void LoadManageEndpointsAddDlgTranslation(HWND hDlg, bool isEdit)
{
    WMi18n& i18n = WMi18n::GetInstance();
    if (isEdit) {
        i18n.SetItemText(hDlg,
                         "settings.mute.manage-endpoints.add-edit.edit-title");
    } else {
        i18n.SetItemText(hDlg,
                         "settings.mute.manage-endpoints.add-edit.add-title");
    }
    i18n.SetItemText(hDlg, IDC_ENDPOINT_NAME_LABEL,
                     "settings.bluetooth.add-edit.device-name-label");
    i18n.SetItemText(hDlg, IDOK, "settings.btn-save");
    i18n.SetItemText(hDlg, IDCANCEL, "settings.btn-cancel");
}

static INT_PTR CALLBACK Settings_EndpointAddDlgProc(HWND hDlg, UINT msg,
                                                    WPARAM wParam,
                                                    LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG: {
            HWND hEndpointName = GetDlgItem(hDlg, IDC_ENDPOINT_NAME);
            EndpointAddCtx* ctx = reinterpret_cast<EndpointAddCtx*>(lParam);
            if (ctx == nullptr || ctx->data == nullptr) {
                return FALSE;
            }
            SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(ctx));

            const bool isEdit = ctx->data->name.length() != 0;
            LoadManageEndpointsAddDlgTranslation(hDlg, isEdit);

            // Disable save button until a device has been picked
            EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);

            // The combo box sorts, so remember which device each item is via
            // its item data rather than relying on the insertion order.
            if (EnumerateAudioEndpoints(ctx->candidates)) {
                for (size_t i = 0; i < ctx->candidates.size(); ++i) {
                    const int pos = ComboBox_AddString(
                        hEndpointName, ctx->candidates[i].name.c_str());
                    if (pos >= 0) {
                        ComboBox_SetItemData(hEndpointName, pos,
                                             static_cast<LONG_PTR>(i));
                    }
                }
            }

            if (isEdit) {
                int pos = ComboBox_FindStringExact(hEndpointName, -1,
                                                   ctx->data->name.c_str());
                if (pos == CB_ERR) {
                    // The entry refers to a device that is not around (or was
                    // stored by name only). Offer it so it can be kept.
                    pos = ComboBox_AddString(hEndpointName,
                                             ctx->data->name.c_str());
                    if (pos >= 0) {
                        ComboBox_SetItemData(hEndpointName, pos,
                                             ENDPOINT_ITEM_LEGACY_NAME);
                    }
                }
                if (pos >= 0) {
                    ComboBox_SetCurSel(hEndpointName, pos);
                }
            }

            SetFocus(hEndpointName);
            return FALSE;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_ENDPOINT_NAME) {
                if (HIWORD(wParam) == CBN_SELCHANGE) {
                    const int sel =
                        ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_ENDPOINT_NAME));
                    EnableWindow(GetDlgItem(hDlg, IDOK), sel != CB_ERR);
                }
            } else if (LOWORD(wParam) == IDOK) {
                HWND hEpName = GetDlgItem(hDlg, IDC_ENDPOINT_NAME);
                EndpointAddCtx* ctx = reinterpret_cast<EndpointAddCtx*>(
                    GetWindowLongPtr(hDlg, DWLP_USER));
                const int sel = ComboBox_GetCurSel(hEpName);
                if (ctx == nullptr || ctx->data == nullptr || sel == CB_ERR) {
                    EndDialog(hDlg, 1);
                    return FALSE;
                }
                const LONG_PTR item =
                    static_cast<LONG_PTR>(ComboBox_GetItemData(hEpName, sel));
                if (item != ENDPOINT_ITEM_LEGACY_NAME &&
                    static_cast<size_t>(item) < ctx->candidates.size())
                {
                    ctx->data->name = ctx->candidates[item].name;
                    ctx->data->id = ctx->candidates[item].id;
                } else {
                    // Kept an entry whose device is not currently around, so
                    // there is no id to bind; it stays matched by name.
                    const int len = ComboBox_GetLBTextLen(hEpName, sel);
                    if (len > 0) {
                        std::wstring name(static_cast<size_t>(len), L'\0');
                        ComboBox_GetLBText(hEpName, sel, name.data());
                        ctx->data->name = name;
                    }
                    ctx->data->id.clear();
                }
                EndDialog(hDlg, 0);
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, 1);
            }
            return FALSE;
        case WM_CLOSE:
            EndDialog(hDlg, 1);
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

static void LoadManageEndpointsDlgTranslation(HWND hDlg)
{
    WMi18n& i18n = WMi18n::GetInstance();

    SetWindowText(
        hDlg,
        i18n.GetTranslationW("settings.mute.manage-endpoints.title").c_str());
    i18n.SetItemText(hDlg, IDC_GROUP_LIST_BEHAVIOUR,
                     "settings.mute.manage-endpoints.list-behaviour.title");
    i18n.SetItemText(
        hDlg, IDC_ENDPOINT_LIST_IS_ALLOWLIST,
        "settings.mute.manage-endpoints.list-behaviour.mute-only-listed");
    i18n.SetItemText(
        hDlg, IDC_ENDPOINT_LIST_IS_BLOCKLIST,
        "settings.mute.manage-endpoints.list-behaviour.mute-all-but-listed");
    i18n.SetItemText(hDlg, IDC_GROUP_ENDPOINTS,
                     "settings.mute.manage-endpoints.endpoints.title");

    i18n.SetItemText(hDlg, IDC_ENDPOINT_ADD, "settings.btn-add");
    i18n.SetItemText(hDlg, IDC_ENDPOINT_EDIT, "settings.btn-edit");
    i18n.SetItemText(hDlg, IDC_ENDPOINT_REMOVE, "settings.btn-remove");
    i18n.SetItemText(hDlg, IDC_ENDPOINT_REMOVEALL, "settings.btn-remove-all");
    i18n.SetItemText(hDlg, IDOK, "settings.btn-save");
    i18n.SetItemText(hDlg, IDCANCEL, "settings.btn-cancel");
}

INT_PTR CALLBACK Settings_ManageEndpointsDlgProc(HWND hDlg, UINT msg,
                                                 WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG: {
            if (IsAppThemed()) {
                EnableThemeDialogTexture(hDlg, ETDT_ENABLETAB);
            }
            LoadManageEndpointsDlgTranslation(hDlg);

            WMSettings* settings = reinterpret_cast<WMSettings*>(lParam);
            assert(settings != nullptr);
            auto* state = new ManageEndpointsDlgState();
            state->settings = settings;
            SetWindowLongPtr(hDlg, DWLP_USER,
                             reinterpret_cast<LONG_PTR>(state));

            DWORD endpointMode = settings->QueryValue(
                SettingsKey::MUTE_INDIVIDUAL_ENDPOINTS_MODE);
            if (endpointMode == MUTE_ENDPOINT_MODE_INDIVIDUAL_ALLOW_LIST) {
                Button_SetCheck(
                    GetDlgItem(hDlg, IDC_ENDPOINT_LIST_IS_ALLOWLIST),
                    BST_CHECKED);
                Button_SetCheck(
                    GetDlgItem(hDlg, IDC_ENDPOINT_LIST_IS_BLOCKLIST),
                    BST_UNCHECKED);
            } else if (endpointMode == MUTE_ENDPOINT_MODE_INDIVIDUAL_BLOCK_LIST)
            {
                Button_SetCheck(
                    GetDlgItem(hDlg, IDC_ENDPOINT_LIST_IS_ALLOWLIST),
                    BST_UNCHECKED);
                Button_SetCheck(
                    GetDlgItem(hDlg, IDC_ENDPOINT_LIST_IS_BLOCKLIST),
                    BST_CHECKED);
            }

            HWND hList = GetDlgItem(hDlg, IDC_ENDPOINT_LIST);
            state->entries = settings->GetManagedAudioEndpoints();
            RebuildEndpointList(hList, state->entries);
            Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_REMOVEALL),
                          ListBox_GetCount(hList) > 0);

            return TRUE;
        }
        case WM_NCDESTROY: {
            delete reinterpret_cast<ManageEndpointsDlgState*>(
                GetWindowLongPtr(hDlg, DWLP_USER));
            SetWindowLongPtr(hDlg, DWLP_USER, 0);
            break;
        }
        case WM_COMMAND: {
            auto* state = reinterpret_cast<ManageEndpointsDlgState*>(
                GetWindowLongPtr(hDlg, DWLP_USER));
            if (state == nullptr) {
                return FALSE;
            }
            if (LOWORD(wParam) == IDC_ENDPOINT_LIST) {
                HWND hList = GetDlgItem(hDlg, IDC_ENDPOINT_LIST);
                if (HIWORD(wParam) == LBN_SELCHANGE ||
                    HIWORD(wParam) == LBN_SELCANCEL)
                {
                    const bool entrySelected =
                        (ListBox_GetCurSel(hList) != LB_ERR);
                    Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_EDIT),
                                  entrySelected);
                    Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_REMOVE),
                                  entrySelected);
                } else if (HIWORD(wParam) == LBN_KILLFOCUS) {
                    const bool entrySelected =
                        (ListBox_GetCurSel(hList) != LB_ERR);
                    Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_EDIT),
                                  entrySelected);
                    Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_REMOVE),
                                  entrySelected);
                }
            } else if (LOWORD(wParam) == IDC_ENDPOINT_ADD) {
                ManagedEndpoint epData;
                EndpointAddCtx ctx;
                ctx.data = &epData;
                if (DialogBoxParam(nullptr,
                                   MAKEINTRESOURCE(IDD_MANAGE_ENDPOINTS_ADD),
                                   hDlg, Settings_EndpointAddDlgProc,
                                   reinterpret_cast<LPARAM>(&ctx)) == 0)
                {
                    const bool duplicate = std::any_of(
                        state->entries.begin(), state->entries.end(),
                        [&epData](const ManagedEndpoint& e) {
                            return e.name == epData.name;
                        });
                    if (!duplicate) {
                        state->entries.push_back(epData);
                        RebuildEndpointList(GetDlgItem(hDlg, IDC_ENDPOINT_LIST),
                                            state->entries);
                        HWND hRemoveAll =
                            GetDlgItem(hDlg, IDC_ENDPOINT_REMOVEALL);
                        if (!IsWindowEnabled(hRemoveAll)) {
                            Button_Enable(hRemoveAll, TRUE);
                        }
                    }
                }
            } else if (LOWORD(wParam) == IDC_ENDPOINT_EDIT) {
                HWND hList = GetDlgItem(hDlg, IDC_ENDPOINT_LIST);
                const int sel = ListBox_GetCurSel(hList);
                if (sel != LB_ERR &&
                    static_cast<size_t>(sel) < state->entries.size())
                {
                    ManagedEndpoint epData = state->entries[sel];

                    EndpointAddCtx ctx;
                    ctx.data = &epData;
                    if (DialogBoxParam(
                            nullptr, MAKEINTRESOURCE(IDD_MANAGE_ENDPOINTS_ADD),
                            hDlg, Settings_EndpointAddDlgProc,
                            reinterpret_cast<LPARAM>(&ctx)) == 0)
                    {
                        // If the edit turned this entry into a duplicate of
                        // another one, drop it instead of keeping both.
                        bool duplicate = false;
                        for (size_t i = 0; i < state->entries.size(); ++i) {
                            if (static_cast<int>(i) != sel &&
                                state->entries[i].name == epData.name)
                            {
                                duplicate = true;
                                break;
                            }
                        }
                        if (duplicate) {
                            state->entries.erase(state->entries.begin() + sel);
                        } else {
                            state->entries[sel] = epData;
                        }
                        // Rebuilding drops the selection, so restore it.
                        RebuildEndpointList(hList, state->entries);
                        if (static_cast<size_t>(sel) < state->entries.size()) {
                            ListBox_SetCurSel(hList, sel);
                        } else {
                            Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_EDIT),
                                          FALSE);
                            Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_REMOVE),
                                          FALSE);
                        }
                    }
                }
            } else if (LOWORD(wParam) == IDC_ENDPOINT_REMOVE) {
                HWND hList = GetDlgItem(hDlg, IDC_ENDPOINT_LIST);
                const int sel = ListBox_GetCurSel(hList);
                if (sel != LB_ERR &&
                    static_cast<size_t>(sel) < state->entries.size())
                {
                    state->entries.erase(state->entries.begin() + sel);
                    // Nothing is selected after the rebuild.
                    RebuildEndpointList(hList, state->entries);
                    Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_EDIT), FALSE);
                    Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_REMOVE), FALSE);
                    if (ListBox_GetCount(hList) == 0) {
                        Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_REMOVEALL),
                                      FALSE);
                    }
                }
            } else if (LOWORD(wParam) == IDC_ENDPOINT_REMOVEALL) {
                state->entries.clear();
                ListBox_ResetContent(GetDlgItem(hDlg, IDC_ENDPOINT_LIST));
                Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_EDIT), FALSE);
                Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_REMOVE), FALSE);
                Button_Enable(GetDlgItem(hDlg, IDC_ENDPOINT_REMOVEALL), FALSE);
            } else if (LOWORD(wParam) == IDOK) {
                WMSettings* settings = state->settings;
                settings->StoreManagedAudioEndpoints(state->entries);
                if (Button_GetCheck(
                        GetDlgItem(hDlg, IDC_ENDPOINT_LIST_IS_ALLOWLIST)))
                {
                    settings->SetValue(
                        SettingsKey::MUTE_INDIVIDUAL_ENDPOINTS_MODE,
                        MUTE_ENDPOINT_MODE_INDIVIDUAL_ALLOW_LIST);
                } else if (Button_GetCheck(GetDlgItem(
                               hDlg, IDC_ENDPOINT_LIST_IS_BLOCKLIST)))
                {
                    settings->SetValue(
                        SettingsKey::MUTE_INDIVIDUAL_ENDPOINTS_MODE,
                        MUTE_ENDPOINT_MODE_INDIVIDUAL_BLOCK_LIST);
                }
                EndDialog(hDlg, 0);
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, 0);
            }
            return 0;
        }
        default:
            break;
    }
    return FALSE;
}
