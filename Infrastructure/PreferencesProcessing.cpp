
/** $VER: PreferencesProcessing.cpp (2025.07.13) P. Stuer **/

#include "pch.h"

#include <atlbase.h>
#include <atlapp.h>
#include <atlcrack.h>
#include <atlctrls.h>
#include <atlmisc.h>

#include <map>
#include <algorithm>

#include <sdk/foobar2000-lite.h>
#include <sdk/console.h>
#include <sdk/preferences_page.h>
#include <sdk/coreDarkMode.h>
#include <sdk/hasher_md5.h>

#include <helpers/atl-misc.h>
#include <helpers/dropdown_helper.h>

#include <pfc/string-conv-lite.h>

#include "Resource.h"
#include "Log.h"

#include "Configuration.h"
#include "Channels.h"

#pragma hdrstop

#pragma warning(disable: 4820) // x bytes padding added after data member

#define ConfigVariable(name, type, type2, value, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
static const GUID GUIDCfg##name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }; \
cfg_var_modern::type Cfg##name(GUIDCfg##name, value); \
const type2 Def##name = value;

#define InitializeConfigVariable(name) _##name = Cfg##name;
#define ResetConfigVariable(name) _##name = Def##name;
#define ApplyConfigVariable(name) Cfg##name = _##name;

#define HasConfigVariableChanged(name) if (_##name != Cfg##name) return true;

// RCP
ConfigVariable(LoopExpansion,       cfg_int,  int,      0, 0x90c3d952,0xd07f,0x4d76,0x84,0x40,0x03,0x4a,0xb6,0x4e,0xfd,0x35);
ConfigVariable(WriteBarMarkers,     cfg_bool, bool, false, 0x532741c5,0xe1a3,0x4334,0xa5,0xb6,0xa5,0x99,0x6d,0x08,0xdc,0x9c);
ConfigVariable(WriteSysExNames,     cfg_bool, bool, false, 0xe00a19b1,0xe0dd,0x46dc,0xb0,0x6c,0xdb,0xb7,0x13,0x5c,0x07,0xc8);
ConfigVariable(ExtendLoops,         cfg_bool, bool, true,  0x0d8983e5,0x748e,0x456d,0xb3,0x20,0x74,0x33,0x81,0xb5,0xd0,0x11);
ConfigVariable(WolfteamLoopMode,    cfg_bool, bool, false, 0x373c9824,0x32a3,0x4ebe,0x87,0x3f,0xb2,0xda,0x7e,0xb8,0x50,0x29);
ConfigVariable(KeepMutedChannels,   cfg_bool, bool, false, 0x5ded0321,0xc53c,0x4581,0xb3,0x1e,0x3c,0x7b,0x3d,0xc0,0x90,0xb5);
ConfigVariable(IncludeControlData,  cfg_bool, bool, true,  0x55930500,0xb061,0x4974,0xaa,0x60,0x3c,0xdf,0xb6,0x07,0x25,0xbc);

// HMI / HMP
ConfigVariable(DefaultTempo,        cfg_int, int,    160,  0xf94e1919,0xd2ed,0x4a3c,0xb5,0x9a,0x9e,0x3a,0x03,0xbf,0x49,0xc4);

// Component
ConfigVariable(LogLevel,            cfg_int,  int,      4, 0x12be0a92,0x3794,0x4414,0x82,0x3e,0xd3,0x14,0x29,0x67,0xa4,0xee);

/// <summary>
/// Implements a preferences page.
/// </summary>
class ProcessingDialog : public CDialogImpl<ProcessingDialog>, public preferences_page_instance
{
public:
    ProcessingDialog(preferences_page_callback::ptr callback) noexcept : _Callback(callback) { }

    ProcessingDialog(const ProcessingDialog &) = delete;
    ProcessingDialog(const ProcessingDialog &&) = delete;
    ProcessingDialog & operator=(const ProcessingDialog &) = delete;
    ProcessingDialog & operator=(ProcessingDialog &&) = delete;

    virtual ~ProcessingDialog() { };

    #pragma region preferences_page_instance

    t_uint32 get_state() final;
    void apply() final;
    void reset() final;

    #pragma endregion

    // WTL message map
    BEGIN_MSG_MAP_EX(ProcessingDialog)
        MSG_WM_INITDIALOG(OnInitDialog)

        MSG_WM_CTLCOLORDLG(OnCtlColorDlg)

        COMMAND_CODE_HANDLER_EX(EN_CHANGE, OnEditChange)
        COMMAND_CODE_HANDLER_EX(BN_CLICKED, OnButtonClick)

        MESSAGE_HANDLER_EX(WM_HSCROLL, OnHScroll)

        COMMAND_HANDLER_EX(IDC_LOG_LEVEL, CBN_SELCHANGE, OnSelectionChange)
    END_MSG_MAP()

    enum
    {
        IDD = IDD_PREFERENCES_PROCESSING
    };

private:
    BOOL OnInitDialog(CWindow, LPARAM) noexcept;
    HBRUSH OnCtlColorDlg(CDCHandle dc, CWindow wnd) const noexcept;

    void OnEditChange(UINT, int, CWindow) noexcept;
    void OnSelectionChange(UINT, int, CWindow) noexcept;
    void OnButtonClick(UINT, int id, CWindow) noexcept;
    LRESULT OnHScroll(UINT, WPARAM, LPARAM) noexcept;

    void UpdateDialog() noexcept;
    void UpdateChannelButtons() noexcept;

    bool HasChanged() const noexcept;
    void OnChanged() const noexcept;

    void InitializePortControls() noexcept
    {
        _PortNumber = 0;

        auto w = (CTrackBarCtrl) GetDlgItem(IDC_PORT_SLIDER);

        w.SetBuddy(GetDlgItem(IDC_PORT), TRUE);
        w.SetRange(0, 127);
        w.SetPageSize(1);
        w.SetPos(_PortNumber);
        w.SetTicFreq(8);

        SetDlgItemInt(IDC_PORT, _PortNumber);
    }

private:
    const preferences_page_callback::ptr _Callback;

    fb2k::CCoreDarkModeHooks _DarkModeHooks;

    // RCP
    int64_t  _LoopExpansion;
    bool _WriteBarMarkers;
    bool _WriteSysExNames;
    bool _ExtendLoops;
    bool _WolfteamLoopMode;
    bool _KeepMutedChannels;
    bool _IncludeControlData;

    // HMI / HMP
    int64_t _DefaultTempo;

    uint16_t _ChannelMask[128];
    uint64_t _ChannelMaskVersion; // Version number of the channel configuration

    uint8_t _PortNumber;

    int64_t _LogLevel;
};

#pragma region preferences_page_instance

/// <summary>
/// Gets the state of the dialog.
/// </summary>
t_uint32 ProcessingDialog::get_state()
{
    t_uint32 State = preferences_state::resettable | preferences_state::dark_mode_supported;

    if (HasChanged())
        State |= preferences_state::changed;

    return State;
}

/// <summary>
/// Applies the changes to the preferences.
/// </summary>
void ProcessingDialog::apply()
{
    ApplyConfigVariable(LoopExpansion);
    ApplyConfigVariable(WriteBarMarkers);
    ApplyConfigVariable(WriteSysExNames);
    ApplyConfigVariable(ExtendLoops);
    ApplyConfigVariable(WolfteamLoopMode);
    ApplyConfigVariable(KeepMutedChannels);
    ApplyConfigVariable(IncludeControlData);

    ApplyConfigVariable(DefaultTempo);

    ApplyConfigVariable(LogLevel);

    CfgChannels.Apply();

    CfgChannels.Get(_ChannelMask, sizeof(_ChannelMask), _ChannelMaskVersion);

    ApplyConfigVariable(LogLevel);

    Log.SetLevel((LogLevel) CfgLogLevel.get());

    OnChanged();
}

/// <summary>
/// Resets this page's content to the default values. Does not apply any changes - lets user preview the changes before hitting "apply".
/// </summary>
void ProcessingDialog::reset()
{
    ResetConfigVariable(LoopExpansion);
    ResetConfigVariable(WriteBarMarkers);
    ResetConfigVariable(WriteSysExNames);
    ResetConfigVariable(ExtendLoops);
    ResetConfigVariable(WolfteamLoopMode);
    ResetConfigVariable(KeepMutedChannels);
    ResetConfigVariable(IncludeControlData);

    ResetConfigVariable(DefaultTempo);

    ResetConfigVariable(LogLevel);

    CfgChannels.Reset();

    InitializePortControls();

    UpdateDialog();

    OnChanged();
}

#pragma endregion

#pragma region CDialogImpl

/// <summary>
/// Initializes the dialog.
/// </summary>
BOOL ProcessingDialog::OnInitDialog(CWindow window, LPARAM) noexcept
{
    _DarkModeHooks.AddDialogWithControls(*this);

    InitializeConfigVariable(LoopExpansion);
    InitializeConfigVariable(WriteBarMarkers);
    InitializeConfigVariable(WriteSysExNames);
    InitializeConfigVariable(ExtendLoops);
    InitializeConfigVariable(WolfteamLoopMode);
    InitializeConfigVariable(KeepMutedChannels);
    InitializeConfigVariable(IncludeControlData);

    InitializeConfigVariable(DefaultTempo);

    CfgChannels.Initialize();

    CfgChannels.Get(_ChannelMask, sizeof(_ChannelMask), _ChannelMaskVersion);

    InitializePortControls();

    UpdateDialog();

    return FALSE;
}

/// <summary>
/// Sets the background color brush.
/// </summary>
HBRUSH ProcessingDialog::OnCtlColorDlg(CDCHandle dc, CWindow wnd) const noexcept
{
#ifdef _DEBUG
    return ::CreateSolidBrush(RGB(250, 250, 250));
#else
    return FALSE;
#endif
}
/// <summary>
/// Handles the notification when a control loses focus.
/// </summary>
void ProcessingDialog::OnEditChange(UINT code, int id, CWindow) noexcept
{
    if (code != EN_CHANGE)
        return;

    WCHAR Text[MAX_PATH];

    GetDlgItemText(id, Text, _countof(Text));

    switch (id)
    {
        case IDC_LOOP_EXPANSION:
            _LoopExpansion = (int) std::clamp((uint32_t) ::_wtoi(Text), 0u, ~0u);
            break;

        case IDC_DEFAULT_TEMPO:
            _DefaultTempo = (int) std::clamp((uint32_t) ::_wtoi(Text), 0u, ~0u);
            break;

        default:
            return;
    }

    OnChanged();
}

/// <summary>
/// Handles a selection change in a combo box.
/// </summary>
void ProcessingDialog::OnSelectionChange(UINT, int id, CWindow) noexcept
{
    if (id == IDC_LOG_LEVEL)
    {
        _LogLevel = SendDlgItemMessage(IDC_LOG_LEVEL, CB_GETCURSEL);

        OnChanged();
    }
}

/// <summary>
/// Handles the notification from the track bar.
/// </summary>
LRESULT ProcessingDialog::OnHScroll(UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
    if ((LOWORD(wParam) != TB_ENDTRACK) && (LOWORD(wParam) != TB_THUMBTRACK))
        return 1;

    auto w = (CTrackBarCtrl) GetDlgItem(IDC_PORT_SLIDER);

    _PortNumber = (uint8_t) w.GetPos();

    SetDlgItemInt(IDC_PORT, _PortNumber);

    UpdateChannelButtons();

    return 0;
}

/// <summary>
/// Handles a click on a button.
/// </summary>
void ProcessingDialog::OnButtonClick(UINT, int id, CWindow w) noexcept
{
    switch (id)
    {
        case IDC_WRITE_BAR_MARKERS:
            _WriteBarMarkers    = !_WriteBarMarkers;
            break;

        case IDC_WRITE_SYSEX_NAMES:
            _WriteSysExNames    = !_WriteSysExNames;
            break;

        case IDC_EXTEND_LOOPS:
            _ExtendLoops        = !_ExtendLoops;
            break;

        case IDC_WOLFTEAM_LOOPS:
            _WolfteamLoopMode   = !_WolfteamLoopMode;
            break;

        case IDC_KEEP_MUTED_CHANNELS:
            _KeepMutedChannels  = !_KeepMutedChannels;
            break;

        case IDC_INCLUDE_CONTROL_DATA:
            _IncludeControlData = !_IncludeControlData;
            break;

        case IDC_CHANNEL_01:
        case IDC_CHANNEL_02:
        case IDC_CHANNEL_03:
        case IDC_CHANNEL_04:
        case IDC_CHANNEL_05:
        case IDC_CHANNEL_06:
        case IDC_CHANNEL_07:
        case IDC_CHANNEL_08:
        case IDC_CHANNEL_09:
        case IDC_CHANNEL_10:
        case IDC_CHANNEL_11:
        case IDC_CHANNEL_12:
        case IDC_CHANNEL_13:
        case IDC_CHANNEL_14:
        case IDC_CHANNEL_15:
        case IDC_CHANNEL_16:
        {
            CfgChannels.Toggle(_PortNumber, (uint32_t) (id - IDC_CHANNEL_01));
            break;
        }

        case IDC_CHANNEL_ALL:
        {
            CfgChannels.All();

            UpdateChannelButtons();
            break;
        }

        case IDC_CHANNEL_NONE:
        {
            CfgChannels.None();

            UpdateChannelButtons();
            break;
        }

        case IDC_CHANNEL_1_10:
        {
            CfgChannels.OnlyLow();

            UpdateChannelButtons();
            break;
        }

        case IDC_CHANNEL_11_16:
        {
            CfgChannels.OnlyHigh();

            UpdateChannelButtons();
            break;
        }

        default:
            return;
    }

    OnChanged();
}

/// <summary>
/// Returns whether our dialog content is different from the current configuration (whether the Apply button should be enabled or not)
/// </summary>
bool ProcessingDialog::HasChanged() const noexcept
{
    HasConfigVariableChanged(LoopExpansion);
    HasConfigVariableChanged(WriteBarMarkers);
    HasConfigVariableChanged(WriteSysExNames);
    HasConfigVariableChanged(ExtendLoops);
    HasConfigVariableChanged(KeepMutedChannels);
    HasConfigVariableChanged(IncludeControlData);

    HasConfigVariableChanged(DefaultTempo);

    if (CfgChannels.HasChanged(_ChannelMask, sizeof(_ChannelMask)))
        return true;

    HasConfigVariableChanged(LogLevel);

    return false;
}

/// <summary>
/// Tells the host that our state has changed to enable/disable the Apply button appropriately.
/// </summary>
void ProcessingDialog::OnChanged() const noexcept
{
    _Callback->on_state_changed();
}

/// <summary>
/// Updates the appearance of the dialog according to the values of the settings.
/// </summary>
void ProcessingDialog::UpdateDialog() noexcept
{
    ::uSetDlgItemText(m_hWnd, IDC_LOOP_EXPANSION, pfc::format_int(_LoopExpansion));

    SendDlgItemMessageW(IDC_WRITE_BAR_MARKERS,    BM_SETCHECK, (WPARAM) _WriteBarMarkers);
    SendDlgItemMessageW(IDC_WRITE_SYSEX_NAMES,    BM_SETCHECK, (WPARAM) _WriteSysExNames);
    SendDlgItemMessageW(IDC_EXTEND_LOOPS,         BM_SETCHECK, (WPARAM) _ExtendLoops);
    SendDlgItemMessageW(IDC_WOLFTEAM_LOOPS,       BM_SETCHECK, (WPARAM) _WolfteamLoopMode);
    SendDlgItemMessageW(IDC_KEEP_MUTED_CHANNELS,  BM_SETCHECK, (WPARAM) _KeepMutedChannels);
    SendDlgItemMessageW(IDC_INCLUDE_CONTROL_DATA, BM_SETCHECK, (WPARAM) _IncludeControlData);

    ::uSetDlgItemText(m_hWnd, IDC_DEFAULT_TEMPO, pfc::format_int(_DefaultTempo));

    UpdateChannelButtons();

    // Log Level
    {
        InitializeConfigVariable(LogLevel);

        auto w = (CComboBox) GetDlgItem(IDC_LOG_LEVEL);

        w.Clear();

        int i = -1;

        for (const auto & Text : { L"Never", L"Fatal", L"Error", L"Warn", L"Info", L"Debug", L"Trace", L"Always", })
        {
            w.AddString(Text);

            if (++i ==  CfgLogLevel)
                w.SetCurSel((int) i);
        }
    }
}

void ProcessingDialog::UpdateChannelButtons() noexcept
{
    for (int i = IDC_CHANNEL_01; i <= IDC_CHANNEL_16; ++i)
        SendDlgItemMessageW(i, BM_SETCHECK, (WPARAM) (CfgChannels.IsEnabled(_PortNumber, (uint32_t) i - IDC_CHANNEL_01) ? BST_CHECKED: BST_UNCHECKED));
}

#pragma endregion

class ProcessingPage : public preferences_page_impl<ProcessingDialog>
{
public:
    ProcessingPage() noexcept { };

    ProcessingPage(const ProcessingPage &) = delete;
    ProcessingPage(const ProcessingPage &&) = delete;
    ProcessingPage & operator=(const ProcessingPage &) = delete;
    ProcessingPage & operator=(ProcessingPage &&) = delete;

    virtual ~ProcessingPage() noexcept { };

    const char * get_name() noexcept
    {
        return IDD_PREFERENCES_PROCESSING_NAME;
    }

    GUID get_guid() noexcept
    {
        return { 0x19bb1820, 0x3b64, 0x403c, { 0xab, 0xf0, 0x3d, 0xd0, 0x06, 0x37, 0xf2, 0x7a } };
    }

    GUID get_parent_guid() noexcept
    {
        return PreferencesPageGUID;
    }
};

static preferences_page_factory_t<ProcessingPage> _Factory;
