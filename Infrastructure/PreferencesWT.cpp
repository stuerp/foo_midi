
/** $VER: PreferencesWT.cpp (2025.07.11) P. Stuer **/

#include "pch.h"

#include <atlbase.h>
#include <atlapp.h>
#include <atlcrack.h>
#include <atlctrls.h>
#include <atlmisc.h>

#include <map>

#include <sdk/foobar2000-lite.h>
#include <sdk/console.h>
#include <sdk/preferences_page.h>
#include <sdk/coreDarkMode.h>
#include <sdk/hasher_md5.h>

#include <helpers/atl-misc.h>
#include <helpers/dropdown_helper.h>

#include <pfc/string-conv-lite.h>

#include "Resource.h"

#include "PreferencesFM.h"
#include "Configuration.h"

#include "BMPlayer.h"
#include "FSPlayer.h"

#include "Encoding.h"

#pragma hdrstop

#pragma warning(disable: 4820) // x bytes padding added after data member

/// <summary>
/// Implements a preferences page.
/// </summary>
class PreferencesWT : public CDialogImpl<PreferencesWT>, public preferences_page_instance
{
public:
    PreferencesWT(preferences_page_callback::ptr callback) noexcept : _Callback(callback)
    {
    }

    PreferencesWT(const PreferencesWT &) = delete;
    PreferencesWT(const PreferencesWT &&) = delete;
    PreferencesWT & operator=(const PreferencesWT &) = delete;
    PreferencesWT & operator=(PreferencesWT &&) = delete;

    virtual ~PreferencesWT() { };

    #pragma region preferences_page_instance

    t_uint32 get_state() final;
    void apply() final;
    void reset() final;

    #pragma endregion

    // WTL message map
    BEGIN_MSG_MAP_EX(PreferencesWT)
        MSG_WM_INITDIALOG(OnInitDialog)

        MSG_WM_TIMER(OnTimer)

        // BASS MIDI
        COMMAND_HANDLER_EX(IDC_BASSMIDI_GAIN, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_BASSMIDI_RESAMPLING, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_BASSMIDI_MAX_VOICES, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_BASSMIDI_EFFECTS, BN_CLICKED, OnButtonClicked)

        // FluidSynth
        COMMAND_HANDLER_EX(IDC_FLUIDSYNTH_INTERPOLATION, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_FLUIDSYNTH_MAX_VOICES, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_FLUIDSYNTH_EFFECTS, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_FLUIDSYNTH_DYN_LOADING, BN_CLICKED, OnButtonClicked)

        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

    enum
    {
        IDD = IDD_PREFERENCES_WT
    };

    const UINT_PTR ID_REFRESH = 1000;

private:
    BOOL OnInitDialog(CWindow, LPARAM) noexcept;
    void OnTimer(UINT_PTR);

    void OnEditChange(UINT, int, CWindow) noexcept;
    void OnSelectionChange(UINT, int, CWindow) noexcept;
    void OnButtonClicked(UINT, int, CWindow) noexcept;

    bool HasChanged() noexcept;
    void OnChanged() const noexcept;

private:
    pfc::string _CacheStatusText;
    pfc::string _CacheStatusTextCurrent;

    struct InterpolationMethod
    {
        std::string Name;
        int Id;
    };

    static const InterpolationMethod _InterpolationMethods[];

    const preferences_page_callback::ptr _Callback;

    fb2k::CCoreDarkModeHooks _DarkModeHooks;
};

const PreferencesWT::InterpolationMethod PreferencesWT::_InterpolationMethods[] =
{
    { "None", FLUID_INTERP_NONE },
    { "Linear", FLUID_INTERP_LINEAR },
    { "Cubic", FLUID_INTERP_4THORDER },
    { "7th Order Sinc", FLUID_INTERP_7THORDER }
};

#pragma region preferences_page_instance

/// <summary>
/// Gets the state of the Preference dialog.
/// </summary>
t_uint32 PreferencesWT::get_state()
{
    t_uint32 State = preferences_state::resettable | preferences_state::dark_mode_supported;

    if (HasChanged())
        State |= preferences_state::changed;

    return State;
}

/// <summary>
/// Applies the changes to the preferences.
/// </summary>
void PreferencesWT::apply()
{
    // BASS MIDI
    {
        wchar_t Text[32];

        GetDlgItemTextW(IDC_BASSMIDI_GAIN, Text, _countof(Text));

        CfgBASSMIDIVolume = std::clamp(::_wtof(Text), 0., 2.);

        ::uSetDlgItemText(m_hWnd, IDC_BASSMIDI_GAIN, pfc::format_float(CfgBASSMIDIVolume, 4, 2));

        CfgBASSMIDIResamplingMode = (t_int32) SendDlgItemMessage(IDC_BASSMIDI_RESAMPLING, CB_GETCURSEL);

        GetDlgItemTextW(IDC_BASSMIDI_MAX_VOICES, Text, _countof(Text));

        CfgBASSMIDIMaxVoices = std::clamp(::_wtoi(Text), 1, 65535);

        CfgBASSMIDIProcessEffects = (bool) SendDlgItemMessage(IDC_BASSMIDI_EFFECTS, BM_GETCHECK);
    }

    // FluidSynth
    {
        int SelectedIndex = (int) SendDlgItemMessage(IDC_FLUIDSYNTH_INTERPOLATION, CB_GETCURSEL);

        if (SelectedIndex != -1)
            CfgFluidSynthInterpolationMode = _InterpolationMethods[SelectedIndex].Id;

        wchar_t Text[32];

        GetDlgItemTextW(IDC_FLUIDSYNTH_MAX_VOICES, Text, _countof(Text));

        CfgFluidSynthMaxVoices = std::clamp(::_wtoi(Text), 1, 65535);

        CfgFluidSynthProcessEffects   = (bool) SendDlgItemMessage(IDC_FLUIDSYNTH_EFFECTS, BM_GETCHECK);
        CfgFluidSynthDynSampleLoading = (bool) SendDlgItemMessage(IDC_FLUIDSYNTH_DYN_LOADING, BM_GETCHECK);
    }

    OnChanged();
}

/// <summary>
/// Resets this page's content to the default values. Does not apply any changes - lets user preview the changes before hitting "apply".
/// </summary>
void PreferencesWT::reset()
{
    // BASS MIDI
    {
        ::uSetDlgItemText(m_hWnd, IDC_BASSMIDI_GAIN, pfc::format_float(DefaultBASSMIDIVolume, 4, 2));

        SendDlgItemMessage(IDC_BASSMIDI_RESAMPLING, CB_SETCURSEL, DefaultBASSMIDIResamplingMode);

        ::SetDlgItemTextA(m_hWnd, IDC_BASSMIDI_MAX_VOICES, pfc::format_int(DefaultBASSMIDIMaxVoices));

        SendDlgItemMessage(IDC_BASSMIDI_EFFECTS, BM_SETCHECK, DefaultBASSMIDIProcessEffects);

        const BOOL IsBASSMIDI = true; //(_SelectedPlayer.Type == PlayerTypes::BASSMIDI);

        const int ControlIds[] =
        {
            IDC_BASSMIDI_VOLUME_LBL, IDC_BASSMIDI_GAIN,
            IDC_RESAMPLING_LBL, IDC_BASSMIDI_RESAMPLING,
            IDC_CACHED_LBL, IDC_CACHED
        };

        for (const int & ControlId : ControlIds)
            GetDlgItem(ControlId).EnableWindow(IsBASSMIDI);
    }

    // FluidSynth
    {
        int SelectedIndex = -1;

        for (int i = 0; i < (int) _countof(_InterpolationMethods); ++i)
        {
            if (_InterpolationMethods[i].Id == DefaultFluidSynthInterpolationMethod)
            {
                SelectedIndex = i;
                break;
            }
        }

        SendDlgItemMessage(IDC_FLUIDSYNTH_INTERPOLATION, CB_SETCURSEL, (WPARAM) SelectedIndex);

        ::SetDlgItemTextA(m_hWnd, IDC_FLUIDSYNTH_MAX_VOICES, pfc::format_int(DefaultFluidSynthMaxVoices));

        SendDlgItemMessage(IDC_FLUIDSYNTH_EFFECTS, BM_SETCHECK, DefaultFluidSynthProcessEffects);
        SendDlgItemMessage(IDC_FLUIDSYNTH_DYN_LOADING, BM_SETCHECK, DefaultFluidSynthDynSampleLoading);

        const BOOL IsFluidSynth = true;//(_SelectedPlayer.Type == PlayerTypes::FluidSynth);

        GetDlgItem(IDC_FLUIDSYNTH_INTERPOLATION_TEXT).EnableWindow(IsFluidSynth);
        GetDlgItem(IDC_FLUIDSYNTH_INTERPOLATION).EnableWindow(IsFluidSynth);
    }

    OnChanged();
}

#pragma endregion

#pragma region CDialogImpl

/// <summary>
/// Initializes the dialog.
/// </summary>
BOOL PreferencesWT::OnInitDialog(CWindow window, LPARAM) noexcept
{
    // BASS MIDI
    {
        ::uSetDlgItemText(m_hWnd, IDC_BASSMIDI_GAIN, pfc::format_float(CfgBASSMIDIVolume, 4, 2));

        auto w = (CComboBox) GetDlgItem(IDC_BASSMIDI_RESAMPLING);

        w.AddString(L"Linear interpolation");
        w.AddString(L"8pt Sinc interpolation");
        w.AddString(L"16pt Sinc interpolation");

        w.SetCurSel((int) CfgBASSMIDIResamplingMode);

        ::uSetDlgItemText(m_hWnd, IDC_BASSMIDI_MAX_VOICES, pfc::format_int(CfgBASSMIDIMaxVoices));

        SendDlgItemMessage(IDC_BASSMIDI_EFFECTS, BM_SETCHECK, CfgBASSMIDIProcessEffects);

        const BOOL Enable = true;//(_SelectedPlayer.Type == PlayerTypes::BASSMIDI);

        const int ControlIds[] =
        {
            IDC_BASSMIDI_VOLUME_LBL, IDC_BASSMIDI_GAIN,
            IDC_RESAMPLING_LBL, IDC_BASSMIDI_RESAMPLING,
            IDC_CACHED_LBL, IDC_CACHED
        };

        for (const auto & ControlId : ControlIds)
            GetDlgItem(ControlId).EnableWindow(Enable);
    }

    // FluidSynth
    {
        auto w = (CComboBox) GetDlgItem(IDC_FLUIDSYNTH_INTERPOLATION);

        int SelectedIndex = -1;
        int i = 0;

        for (const auto & InterpolationMethod : _InterpolationMethods)
        {
            w.AddString(::UTF8ToWide(InterpolationMethod.Name).c_str());

            if (InterpolationMethod.Id == CfgFluidSynthInterpolationMode)
                SelectedIndex = (int) i;

            ++i;
        }

        w.SetCurSel(SelectedIndex);

        ::uSetDlgItemText(m_hWnd, IDC_FLUIDSYNTH_MAX_VOICES, pfc::format_int(CfgFluidSynthMaxVoices));

        SendDlgItemMessage(IDC_FLUIDSYNTH_EFFECTS, BM_SETCHECK, CfgFluidSynthProcessEffects);
        SendDlgItemMessage(IDC_FLUIDSYNTH_DYN_LOADING, BM_SETCHECK, CfgFluidSynthDynSampleLoading);

        const BOOL Enable = true;//(_SelectedPlayer.Type == PlayerTypes::FluidSynth);

        GetDlgItem(IDC_FLUIDSYNTH_INTERPOLATION_TEXT).EnableWindow(Enable);
        GetDlgItem(IDC_FLUIDSYNTH_INTERPOLATION).EnableWindow(Enable);
    }

    SetTimer(ID_REFRESH, 20);

    _DarkModeHooks.AddDialogWithControls(*this);

    return FALSE;
}

/// <summary>
/// Handles the notification when a control loses focus.
/// </summary>
void PreferencesWT::OnEditChange(UINT code, int id, CWindow) noexcept
{
    if (code != EN_CHANGE)
        return;

    OnChanged();
}

/// <summary>
/// Handles a selection change in a combo box.
/// </summary>
void PreferencesWT::OnSelectionChange(UINT, int, CWindow) noexcept
{
    OnChanged();
}

/// <summary>
/// Handles a click on a button.
/// </summary>
void PreferencesWT::OnButtonClicked(UINT, int id, CWindow) noexcept
{
}

/// <summary>
/// Returns whether our dialog content is different from the current configuration (whether the Apply button should be enabled or not)
/// </summary>
bool PreferencesWT::HasChanged() noexcept
{
    // BASS MIDI
    {
        wchar_t Text[32];

        GetDlgItemTextW(IDC_BASSMIDI_GAIN, Text, _countof(Text));

        if (std::abs(::_wtof(Text) - CfgBASSMIDIVolume) > 0.001)
            return true;

        if (SendDlgItemMessage(IDC_BASSMIDI_RESAMPLING, CB_GETCURSEL) != CfgBASSMIDIResamplingMode)
            return true;

        GetDlgItemTextW(IDC_BASSMIDI_MAX_VOICES, Text, _countof(Text));

        if (::_wtoi(Text) != CfgBASSMIDIMaxVoices)
            return true;

        if (SendDlgItemMessage(IDC_BASSMIDI_EFFECTS, BM_GETCHECK) != CfgBASSMIDIProcessEffects)
            return true;
    }

    // FluidSynth
    {
        int SelectedIndex = (int) SendDlgItemMessage(IDC_FLUIDSYNTH_INTERPOLATION, CB_GETCURSEL);

        if ((SelectedIndex != -1) && (_InterpolationMethods[SelectedIndex ].Id != CfgFluidSynthInterpolationMode))
            return true;

        wchar_t Text[32];

        GetDlgItemTextW(IDC_FLUIDSYNTH_MAX_VOICES, Text, _countof(Text));

        if (::_wtoi(Text) != CfgFluidSynthMaxVoices)
            return true;

        if (SendDlgItemMessage(IDC_FLUIDSYNTH_EFFECTS, BM_GETCHECK) != CfgFluidSynthProcessEffects)
            return true;

        if (SendDlgItemMessage(IDC_FLUIDSYNTH_DYN_LOADING, BM_GETCHECK) != CfgFluidSynthDynSampleLoading)
            return true;
    }

    return false;
}

/// <summary>
/// Tells the host that our state has changed to enable/disable the Apply button appropriately.
/// </summary>
void PreferencesWT::OnChanged() const noexcept
{
    _Callback->on_state_changed();
}


/// <summary>
/// Update the status of the SoundFont cache.
/// </summary>
void PreferencesWT::OnTimer(UINT_PTR eventId)
{
    if (eventId != ID_REFRESH)
        return;
 
//  GetDlgItem(IDC_SAMPLERATE).EnableWindow(CfgPlayerType || !_IsRunning);

    uint64_t SampleDataSize, SampleDataLoaded;

    if (::GetSoundFontStatistics(SampleDataSize, SampleDataLoaded))
    {
        _CacheStatusText.reset();
        _CacheStatusText << pfc::format_file_size_short(SampleDataLoaded) << " / " << pfc::format_file_size_short(SampleDataSize);
    }
    else
        _CacheStatusText = "BASS not loaded.";

    if (_CacheStatusText != _CacheStatusTextCurrent)
    {
        // Weird code to work around foobar2000 rendering bug in dark mode: Get the state of the control, enable it, set the text and restore the state.
        auto Control = GetDlgItem(IDC_CACHED);

        BOOL State = Control.IsWindowEnabled();

        Control.EnableWindow(TRUE);

        _CacheStatusTextCurrent = _CacheStatusText;

        ::uSetWindowText(Control, _CacheStatusText);

        Control.EnableWindow(State);
    }
}

#pragma endregion

class PreferencesWTPage : public preferences_page_impl<PreferencesWT>
{
public:
    PreferencesWTPage() noexcept { };

    PreferencesWTPage(const PreferencesWTPage &) = delete;
    PreferencesWTPage(const PreferencesWTPage &&) = delete;
    PreferencesWTPage & operator=(const PreferencesWTPage &) = delete;
    PreferencesWTPage & operator=(PreferencesWTPage &&) = delete;

    virtual ~PreferencesWTPage() noexcept { };

    const char * get_name() noexcept
    {
        return IDD_PREFERENCES_WT_NAME;
    }

    GUID get_guid() noexcept
    {
        return { 0xf2a7c780, 0x9dde, 0x4aff, { 0x9e, 0x7c, 0x8e, 0xa0, 0xf8, 0x4d, 0xcb, 0x2c } };
    }

    GUID get_parent_guid() noexcept
    {
        return PreferencesPageGUID;
    }
};

static preferences_page_factory_t<PreferencesWTPage> PreferencesPageFactory;
