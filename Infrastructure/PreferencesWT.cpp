
/** $VER: PreferencesWT.cpp (2025.07.14) P. Stuer **/

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

const std::vector<std::string> _MT32EmuSets =
{
    "Roland",
    "Sierra / King's Quest 6",
};

#pragma warning(disable: 4820) // x bytes padding added after data member

/// <summary>
/// Implements a preferences page.
/// </summary>
class WTDialog : public CDialogImpl<WTDialog>, public preferences_page_instance
{
public:
    WTDialog(preferences_page_callback::ptr callback) noexcept : _Callback(callback)
    {
    }

    WTDialog(const WTDialog &) = delete;
    WTDialog(const WTDialog &&) = delete;
    WTDialog & operator=(const WTDialog &) = delete;
    WTDialog & operator=(WTDialog &&) = delete;

    virtual ~WTDialog() { };

    #pragma region preferences_page_instance

    t_uint32 get_state() final;
    void apply() final;
    void reset() final;

    #pragma endregion

    // WTL message map
    BEGIN_MSG_MAP_EX(WTDialog)
        MSG_WM_INITDIALOG(OnInitDialog)

        MSG_WM_TIMER(OnTimer)
        MSG_WM_CTLCOLORDLG(OnCtlColorDlg)

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

        // MT-32 Player
        COMMAND_HANDLER_EX(IDC_MT32_CONVERSION_QUALITY, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_MT32_MAX_PARTIALS, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_MT32_ANALOG_OUTPUT_MODE, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_MT32_GM_SET, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_MT32_DAC_INPUT_MODE, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_MT32_REVERB, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_MT32_NICE_AMP_RAMP, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_MT32_NICE_PANNING, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_MT32_NICE_PARTIAL_MIXING, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_MT32_REVERSE_STEREO, BN_CLICKED, OnButtonClicked)

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
    HBRUSH OnCtlColorDlg(CDCHandle, CWindow) const noexcept;

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

const WTDialog::InterpolationMethod WTDialog::_InterpolationMethods[] =
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
t_uint32 WTDialog::get_state()
{
    t_uint32 State = preferences_state::resettable | preferences_state::dark_mode_supported;

    if (HasChanged())
        State |= preferences_state::changed;

    return State;
}

/// <summary>
/// Applies the changes to the preferences.
/// </summary>
void WTDialog::apply()
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

    // MT32
    {
        // MT32 Conversion Quality
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_MT32_CONVERSION_QUALITY, CB_GETCURSEL);

            if (SelectedIndex < 0 || SelectedIndex > 3)
                SelectedIndex = 0;

            CfgMT32EmuConversionQuality = SelectedIndex;
        }

        // MT32 Max. Partials
        {
            int Value = std::clamp((int) GetDlgItemInt(IDC_MT32_MAX_PARTIALS, NULL, FALSE), 8, 256);

            SetDlgItemInt(IDC_MT32_MAX_PARTIALS, (UINT) Value, FALSE);

            CfgMT32EmuMaxPartials = Value;
        }

        // MT32 Analog Output Mode
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_MT32_ANALOG_OUTPUT_MODE, CB_GETCURSEL);

            if (SelectedIndex < 0 || SelectedIndex > 3)
                SelectedIndex = 0;

            CfgMT32EmuAnalogOutputMode = SelectedIndex;
        }

        // MT32 GM Set
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_MT32_GM_SET, CB_GETCURSEL);

            if (SelectedIndex < 0 || SelectedIndex >= (int) _MT32EmuSets.size())
                SelectedIndex = 0;

            CfgMT32EmuGMSet = SelectedIndex;
        }

        // MT32 DAC Input Mode
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_MT32_DAC_INPUT_MODE, CB_GETCURSEL);

            if (SelectedIndex < 0 || SelectedIndex > 3)
                SelectedIndex = 0;

            CfgMT32EmuDACInputMode = SelectedIndex;
        }

        CfgMT32EmuReverb            = (bool) SendDlgItemMessage(IDC_MT32_REVERB, BM_GETCHECK);

        // MT32 Nice Amp Ramp / Nice Panning / Nice Partial Mixing / Reverse Stereo
        CfgMT32EmuNiceAmpRamp       = (bool) SendDlgItemMessage(IDC_MT32_NICE_AMP_RAMP, BM_GETCHECK);
        CfgMT32EmuNicePanning       = (bool) SendDlgItemMessage(IDC_MT32_NICE_PANNING, BM_GETCHECK);
        CfgMT32EmuNicePartialMixing = (bool) SendDlgItemMessage(IDC_MT32_NICE_PARTIAL_MIXING, BM_GETCHECK);
        CfgMT32EmuReverseStereo     = (bool) SendDlgItemMessage(IDC_MT32_REVERSE_STEREO, BM_GETCHECK);
    }

    OnChanged();
}

/// <summary>
/// Resets this page's content to the default values. Does not apply any changes - lets user preview the changes before hitting "apply".
/// </summary>
void WTDialog::reset()
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

    // MT32
    {
        // MT32 Conversion Quality
        SendDlgItemMessage(IDC_MT32_CONVERSION_QUALITY, CB_SETCURSEL, DefaultMT32EmuConversionQuality);

        // MT32 Max. Partials
        SetDlgItemInt(IDC_MT32_MAX_PARTIALS, DefaultMT32EmuMaxPartials, 0);

        // MT32 Analog Output Mode
        SendDlgItemMessage(IDC_MT32_ANALOG_OUTPUT_MODE, CB_SETCURSEL, DefaultMT32EmuAnalogOutputMode);

        // MT32 GM Set
        SendDlgItemMessage(IDC_MT32_GM_SET, CB_SETCURSEL, DefaultMT32EmuGMSet);

        // MT32 DAC Input Mode
        SendDlgItemMessage(IDC_MT32_DAC_INPUT_MODE, CB_SETCURSEL, DefaultMT32EmuDACInputMode);

        // MT32 Reverb
        SendDlgItemMessage(IDC_MT32_REVERB, BM_SETCHECK, (WPARAM) DefaultMT32EmuReverb);

        // Nice Amp Ramp / Nice Panning / Nice Partial Mixing
        SendDlgItemMessage(IDC_MT32_NICE_AMP_RAMP,       BM_SETCHECK, (WPARAM) DefaultMT32EmuNiceAmpRamp);
        SendDlgItemMessage(IDC_MT32_NICE_PANNING,        BM_SETCHECK, (WPARAM) DefaultMT32EmuNicePanning);
        SendDlgItemMessage(IDC_MT32_NICE_PARTIAL_MIXING, BM_SETCHECK, (WPARAM) DefaultMT32EmuNicePartialMixing);
        SendDlgItemMessage(IDC_MT32_REVERSE_STEREO,      BM_SETCHECK, (WPARAM) DefaultMT32EmuReverseStereo);
    }

    OnChanged();
}

#pragma endregion

#pragma region CDialogImpl

/// <summary>
/// Initializes the dialog.
/// </summary>
BOOL WTDialog::OnInitDialog(CWindow window, LPARAM) noexcept
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

    // MT-32
    {
        // MT32Emu Conversion Quality
        {
            static const wchar_t * ConversionQualities[] = { L"Fastest", L"Fast", L"Good", L"Best" };

            auto w = (CComboBox) GetDlgItem(IDC_MT32_CONVERSION_QUALITY);

            for (const auto & Iter : ConversionQualities)
                w.AddString(Iter);

            w.SetCurSel((int) CfgMT32EmuConversionQuality);
        }

        // MT32Emu Max. Partials
        {
            SetDlgItemInt(IDC_MT32_MAX_PARTIALS, (UINT) CfgMT32EmuMaxPartials, 0);
        }

        // MT32Emu Analog Output Mode
        {
            static const wchar_t * AnalogOutputModes[] = { L"Digital Only", L"Coarse", L"Accurate", L"Oversampled" };

            auto w = (CComboBox) GetDlgItem(IDC_MT32_ANALOG_OUTPUT_MODE);

            for (const auto & Iter : AnalogOutputModes)
                w.AddString(Iter);

            w.SetCurSel((int) CfgMT32EmuAnalogOutputMode);
        }

        // MT32Emu GM Set
        {
            auto w = (CComboBox) GetDlgItem(IDC_MT32_GM_SET);

            for (const auto & Set : _MT32EmuSets)
                ::uSendMessageText(w, CB_ADDSTRING, 0, Set.c_str());

            w.SetCurSel((int) CfgMT32EmuGMSet);
        }

        // MT32Emu DAC Input Mode
        {
            static const wchar_t * DACInputModes[] = { L"Nice", L"Pure", L"Generation 1", L"Generation 2" };

            auto w = (CComboBox) GetDlgItem(IDC_MT32_DAC_INPUT_MODE);

            for (const auto & Iter : DACInputModes)
                w.AddString(Iter);

            w.SetCurSel((int) CfgMT32EmuDACInputMode);
        }

        // MT32Emu Reverb
        {
            SendDlgItemMessage(IDC_MT32_REVERB, BM_SETCHECK, (WPARAM) CfgMT32EmuReverb);
        }

        // MT32Emu Nice Amp Ramp / Nice Panning / Nice Partial Mixing
        {
            SendDlgItemMessage(IDC_MT32_NICE_AMP_RAMP,       BM_SETCHECK, (WPARAM) CfgMT32EmuNiceAmpRamp);
            SendDlgItemMessage(IDC_MT32_NICE_PANNING,        BM_SETCHECK, (WPARAM) CfgMT32EmuNicePanning);
            SendDlgItemMessage(IDC_MT32_NICE_PARTIAL_MIXING, BM_SETCHECK, (WPARAM) CfgMT32EmuNicePartialMixing);
            SendDlgItemMessage(IDC_MT32_REVERSE_STEREO,      BM_SETCHECK, (WPARAM) CfgMT32EmuReverseStereo);
        }
    }

    SetTimer(ID_REFRESH, 20);

    _DarkModeHooks.AddDialogWithControls(*this);

    return FALSE;
}

/// <summary>
/// Sets the background color brush.
/// </summary>
HBRUSH WTDialog::OnCtlColorDlg(CDCHandle dc, CWindow wnd) const noexcept
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
void WTDialog::OnEditChange(UINT code, int id, CWindow) noexcept
{
    if (code != EN_CHANGE)
        return;

    OnChanged();
}

/// <summary>
/// Handles a selection change in a combo box.
/// </summary>
void WTDialog::OnSelectionChange(UINT, int, CWindow) noexcept
{
    OnChanged();
}

/// <summary>
/// Handles a click on a button.
/// </summary>
void WTDialog::OnButtonClicked(UINT, int id, CWindow) noexcept
{
    switch (id)
    {
        case IDC_BASSMIDI_EFFECTS:
        case IDC_FLUIDSYNTH_EFFECTS:
        case IDC_FLUIDSYNTH_DYN_LOADING:

        case IDC_MT32_REVERB:
        case IDC_MT32_NICE_AMP_RAMP:
        case IDC_MT32_NICE_PANNING:
        case IDC_MT32_NICE_PARTIAL_MIXING:
        case IDC_MT32_REVERSE_STEREO:
        {
            OnChanged();
            break;
        }
    }
}

/// <summary>
/// Returns whether our dialog content is different from the current configuration (whether the Apply button should be enabled or not)
/// </summary>
bool WTDialog::HasChanged() noexcept
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

    // MT32
    {
        // MT32 Conversion Quality
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_MT32_CONVERSION_QUALITY, CB_GETCURSEL);

            if (SelectedIndex != CfgMT32EmuConversionQuality)
                return true;
        }

        // MT32 Max. Partials
        {
            if (GetDlgItemInt(IDC_MT32_MAX_PARTIALS, NULL, FALSE) != (UINT) CfgMT32EmuMaxPartials)
                return true;
        }

        // MT32 Analog Output Mode
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_MT32_ANALOG_OUTPUT_MODE, CB_GETCURSEL);

            if (SelectedIndex != CfgMT32EmuAnalogOutputMode)
                return true;
        }

        // MT32 GM Set
        {
            if (SendDlgItemMessage(IDC_MT32_GM_SET, CB_GETCURSEL) != (UINT) CfgMT32EmuGMSet)
                return true;
        }

        // MT32 DAC Input Mode
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_MT32_DAC_INPUT_MODE, CB_GETCURSEL);

            if (SelectedIndex != CfgMT32EmuDACInputMode)
                return true;
        }

        // MT32 Reverb
        if (SendDlgItemMessage(IDC_MT32_REVERB, BM_GETCHECK) != CfgMT32EmuReverb)
            return true;

        // MT32 Nice Amp Ramp
        if (SendDlgItemMessage(IDC_MT32_NICE_AMP_RAMP, BM_GETCHECK) != CfgMT32EmuNiceAmpRamp)
            return true;

        // MT32 Nice Panning
        if (SendDlgItemMessage(IDC_MT32_NICE_PANNING, BM_GETCHECK) != CfgMT32EmuNicePanning)
            return true;

        // MT32 Nice Partial Mixing
        if (SendDlgItemMessage(IDC_MT32_NICE_PARTIAL_MIXING, BM_GETCHECK) != CfgMT32EmuNicePartialMixing)
            return true;

        // MT32 Reverse Stereo
        if (SendDlgItemMessage(IDC_MT32_REVERSE_STEREO, BM_GETCHECK) != CfgMT32EmuReverseStereo)
            return true;
    }

    return false;
}

/// <summary>
/// Tells the host that our state has changed to enable/disable the Apply button appropriately.
/// </summary>
void WTDialog::OnChanged() const noexcept
{
    _Callback->on_state_changed();
}

/// <summary>
/// Update the status of the SoundFont cache.
/// </summary>
void WTDialog::OnTimer(UINT_PTR eventId)
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

class WTPage : public preferences_page_impl<WTDialog>
{
public:
    WTPage() noexcept { };

    WTPage(const WTPage &) = delete;
    WTPage(const WTPage &&) = delete;
    WTPage & operator=(const WTPage &) = delete;
    WTPage & operator=(WTPage &&) = delete;

    virtual ~WTPage() noexcept { };

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

static preferences_page_factory_t<WTPage> _Factory;
