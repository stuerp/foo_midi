
/** $VER: Preferences.cpp (2023.01.05) **/

#pragma warning(disable: 5045 26481 26485)

#include "Preferences.h"

#include "NukePlayer.h"

//#define DEBUG_DIALOG

#pragma hdrstop

extern char _DLLFileName[];
extern volatile int _IsRunning;

static const GUID GUIDCfgSampleRateHistory = { 0x408aa155, 0x4c42, 0x42b5, { 0x8c, 0x3e, 0xd1, 0xc, 0x35, 0xdd, 0x5e, 0xf1 } };

static const char * DefaultPathMessage = "Click to set.";

static const int SampleRates[] = { 8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 49716, 64000, 88200, 96000 };

cfg_dropdown_history CfgSampleRateHistory(GUIDCfgSampleRateHistory, 16);

#pragma region("FluidSynth")
#ifdef FLUIDSYNTHSUPPORT
static const char * interp_txt[] = { "None", "Linear", "Cubic", "7th Order Sinc" };
static int interp_method[] = { FLUID_INTERP_NONE, FLUID_INTERP_LINEAR, FLUID_INTERP_4THORDER, FLUID_INTERP_7THORDER };
enum
{
    interp_method_default = 2
};
#endif
#pragma endregion

#pragma region("Munt")
const char * _MuntGMSets[] =
{
    "Roland",
    "Sierra / King's Quest 6",
};
const size_t _MuntGMSetCount = _countof(_MuntGMSets);
#pragma endregion

/// <summary>
/// Contains all the plugins that are build into the component.
/// </summary>
const Preferences::BuiltInPlugin Preferences::BuiltInPlugins[] =
{
    { "Emu de MIDI",    PlayerTypeEmuDeMIDI,      -1, IsPluginAlwaysPresent },

#ifdef FLUIDSYNTHSUPPORT
    { "FluidSynth",
    #ifdef BASSMIDISUPPORT
          FluidSynthPluginId, -1,
    #else
          PlayerTypeBASSMIDI, FluidSynthPluginId,
    #endif
      IsPluginAlwaysPresent },
#endif

#ifdef BASSMIDISUPPORT
    { "BASSMIDI",
    #ifdef FLUIDSYNTHSUPPORT
          PlayerTypeBASSMIDI, -1,
    #else
          PlayerTypeBASSMIDI, PlayerTypeFluidSynth,
    #endif
      IsPluginAlwaysPresent },
#endif

    { "Super Munt GM",  PlayerTypeSuperMunt,      -1, IsPluginAlwaysPresent },

    { "LibADLMIDI",     PlayerTypeADL,            -1, IsPluginAlwaysPresent },
    { "LibOPNMIDI",     PlayerTypeOPN,            -1, IsPluginAlwaysPresent },
    { "OPL MIDI",       PlayerTypeOPL,            -1, IsPluginNeverPresent },
    { "Nuke",           PlayerTypeNuke,           -1, IsPluginAlwaysPresent },
    { "Secret Sauce",   PlayerTypeSecretSauce,    -1, IsSecretSaucePresent }
};

#pragma region("preferences_page_instance")
/// <summary>
/// Gets the state of the Preference dialog.
/// </summary>
t_uint32 Preferences::get_state()
{
    t_uint32 State = preferences_state::resettable | preferences_state::dark_mode_supported;

    if (HasChanged())
        State |= preferences_state::changed;

    if (_IsBusy)
        State |= preferences_state::busy;

    return State;
}

void Preferences::apply()
{
    {
        int PlayerType = -1;

        int SelectedItem = (int)SendDlgItemMessage(IDC_PLUGIN, CB_GETCURSEL);

        {
            if (SelectedItem >= _ReportedPlugInCount && SelectedItem < (int)(_ReportedPlugInCount + _VSTiPlugIns.get_count()))
                PlayerType = PlayerTypeVSTi;
        #ifdef DXISUPPORT
            else
            if (plugin_selected >= plugins_reported + _VSTiPlugins.get_count())
                PlayerType = PlayerTypeDirectX;
        #endif
            else
                PlayerType = _IndexToPlugInId[SelectedItem];

            CfgPlayerType = PlayerType;
        }

        {
            CfgVSTiFilePath = "";

            if (PlayerType == PlayerTypeVSTi)
            {
                CfgVSTiFilePath = _VSTiPlugIns[(size_t)(SelectedItem - _ReportedPlugInCount)].PathName.c_str();

                CfgVSTiConfig[_VSTiPlugIns[(size_t)(SelectedItem - _ReportedPlugInCount)].Id] = _VSTiConfig;
            }
        }

        #ifdef DXISUPPORT
            else
            if (PlayerType == PlayerTypeDirectX)
            {
                cfg_dxi_plugin = dxi_plugins[plugin_selected - _VSTiPlugins.get_count() - plugins_reported];
            }
        #endif
    }

    {
        int SampleRate = (int)GetDlgItemInt(IDC_SAMPLERATE, NULL, FALSE);

        if (SampleRate < 6000)
            SampleRate = 6000;
        else
        if (SampleRate > 192000)
            SampleRate = 192000;

        SetDlgItemInt(IDC_SAMPLERATE, (UINT)SampleRate, FALSE);

        {
            char Text[16];

            _itoa_s(SampleRate, Text, _countof(Text), 10);

            CfgSampleRateHistory.add_item(Text);
            CfgSampleRate = SampleRate;
        }
    }

    {
        int t = (int)SendDlgItemMessage(IDC_ADL_BANK, CB_GETCURSEL);

        if (t < 0 || t >= (int)_ADLBanks.get_count())
            t = 0;

        CfgADLBank = _ADLBanks[(t_size)t].number;
    }

    {
        int t = (int)GetDlgItemInt(IDC_ADL_CHIPS, NULL, FALSE);

        if (t < 1)
            t = 1;
        else
        if (t > 100)
            t = 100;

        SetDlgItemInt(IDC_ADL_CHIPS, (UINT)t, FALSE);

        CfgADLChipCount = t;
    }

    CfgADLPanning = (t_int32)SendDlgItemMessage(IDC_ADL_PANNING, BM_GETCHECK);
    CfgMuntGMSet = (t_int32)SendDlgItemMessage(IDC_MUNT_GM_SET, CB_GETCURSEL);

    {
        size_t PresetIndex = (size_t)SendDlgItemMessage(IDC_NUKE_PRESET, CB_GETCURSEL);

        unsigned int Synth;
        unsigned int Bank;

        NukePlayer::GetPreset(PresetIndex, Synth, Bank);

        CfgMSSynthesizer = (t_int32)Synth;
        CfgMSBank = (t_int32)Bank;
    }

    CfgSoundFontFilePath = _SoundFontPath;
    CfgMuntFilePath = _MuntPath;

    CfgLoopTypePlayback = (t_int32)SendDlgItemMessage(IDC_LOOP_PLAYBACK, CB_GETCURSEL);
    CfgLoopTypeOther = (t_int32)SendDlgItemMessage(IDC_LOOP_OTHER, CB_GETCURSEL);

    cfg_thloopz = (t_int32)SendDlgItemMessage(IDC_THLOOPZ, BM_GETCHECK);
    cfg_rpgmloopz = (t_int32)SendDlgItemMessage(IDC_RPGMLOOPZ, BM_GETCHECK);
    cfg_xmiloopz = (t_int32)SendDlgItemMessage(IDC_XMILOOPZ, BM_GETCHECK);
    cfg_ff7loopz = (t_int32)SendDlgItemMessage(IDC_FF7LOOPZ, BM_GETCHECK);

    cfg_emidi_exclusion = (t_int32)SendDlgItemMessage(IDC_EMIDI_EX, BM_GETCHECK);

    cfg_filter_instruments = (t_int32)SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_GETCHECK);
    cfg_filter_banks = (t_int32)SendDlgItemMessage(IDC_FILTER_BANKS, BM_GETCHECK);

    CfgMSPanning = (t_int32)SendDlgItemMessage(IDC_NUKE_PANNING, BM_GETCHECK);

#ifdef FLUIDSYNTHSUPPORT
    {
        int interp_method = SendDlgItemMessage(IDC_RESAMPLING, CB_GETCURSEL);

        if (interp_method == 2)
            interp_method = 4;
        else
        if (interp_method == 3)
            interp_method = 7;
        
Cfg_FluidSynthInterpolationMethod = interp_method;
    }
#endif

#ifdef BASSMIDISUPPORT
    CfgResamplingMode = (t_int32)SendDlgItemMessage(IDC_RESAMPLING_MODE, CB_GETCURSEL);
#endif

    CfgMIDIFlavor = (t_int32)SendDlgItemMessage(IDC_MIDI_FLAVOR, CB_GETCURSEL);
    CfgAllowMIDIEffects = !SendDlgItemMessage(IDC_MIDI_EFFECTS, BM_GETCHECK);

    OnChanged();
}

void Preferences::reset()
{
    int PlugInIndex = _PlugInIdToIndex[DefaultPlayerType];
    int SelectedItem = _PluginPresentMap[PlugInIndex];

    SendDlgItemMessage(IDC_PLUGIN, CB_SETCURSEL, (WPARAM)SelectedItem);

    if ((DefaultPlayerType != PlayerTypeFluidSynth) && (DefaultPlayerType != PlayerTypeBASSMIDI))
    {
        const int ControlId[] =
        {
            IDC_VST_PATH,
            IDC_SOUNDFONT_TEXT, IDC_SOUNDFONT_FILE_PATH,
            IDC_RESAMPLING_TEXT, IDC_RESAMPLING_MODE,
            IDC_CACHED_TEXT, IDC_CACHED
        };

        for (size_t i = 0; i < _countof(ControlId); ++i)
            GetDlgItem(ControlId[i]).EnableWindow(FALSE);
    }

    if (DefaultPlayerType != PlayerTypeADL)
    {
        const int ControlId[] =
        {
            IDC_ADL_BANK_TEXT, IDC_ADL_BANK,
            IDC_ADL_CHIPS_TEXT, IDC_ADL_CHIPS,
            IDC_RESAMPLING_TEXT, IDC_RESAMPLING_MODE,
            IDC_ADL_PANNING
        };

        for (size_t i = 0; i < _countof(ControlId); ++i)
            GetDlgItem(ControlId[i]).EnableWindow(FALSE);
    }

    if (DefaultPlayerType == PlayerTypeSuperMunt)
    {
        GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(FALSE);
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_SHOW);
    }
    else
    if (_VSTiPlugIns.get_count() == 0)
    {
        GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(FALSE);
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_HIDE);
    }
    else
    {
        GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(FALSE);
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_HIDE);
    }

    if (DefaultPlayerType != PlayerTypeNuke)
    {
        GetDlgItem(IDC_NUKE_PRESET_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_NUKE_PRESET).EnableWindow(FALSE);
        GetDlgItem(IDC_NUKE_PANNING).EnableWindow(FALSE);
    }

    {
        bool enable = (DefaultPlayerType == PlayerTypeVSTi) || (DefaultPlayerType == PlayerTypeFluidSynth) || (DefaultPlayerType == PlayerTypeBASSMIDI) || (DefaultPlayerType == PlayerTypeSecretSauce);

        GetDlgItem(IDC_FILTER_GROUP).EnableWindow(enable);
        GetDlgItem(IDC_MIDI_FLAVOR_TEXT).EnableWindow(enable);
        GetDlgItem(IDC_MIDI_FLAVOR).EnableWindow(enable);
        GetDlgItem(IDC_MIDI_EFFECTS).EnableWindow(enable);
    }

    ::uSetDlgItemText(m_hWnd, IDC_VST_PATH, DefaultPathMessage);
    ::uSetDlgItemText(m_hWnd, IDC_SOUNDFONT_FILE_PATH, DefaultPathMessage);
    ::uSetDlgItemText(m_hWnd, IDC_MUNT_FILE_PATH, DefaultPathMessage);

    _VSTiPath.reset();
    _SoundFontPath.reset();
    _MuntPath.reset();

    SetDlgItemInt(IDC_SAMPLERATE, DefaultSampleRate, FALSE);

    if ((DefaultPlayerType == PlayerTypeEmuDeMIDI) && _IsRunning)
        GetDlgItem(IDC_SAMPLERATE).EnableWindow(FALSE);

    SendDlgItemMessage(IDC_LOOP_PLAYBACK, CB_SETCURSEL, DefaultPlaybackLoopType);
    SendDlgItemMessage(IDC_LOOP_OTHER, CB_SETCURSEL, DefaultOtherLoopType);
    SendDlgItemMessage(IDC_THLOOPZ, BM_SETCHECK, default_cfg_thloopz);
    SendDlgItemMessage(IDC_RPGMLOOPZ, BM_SETCHECK, default_cfg_rpgmloopz);
    SendDlgItemMessage(IDC_XMILOOPZ, BM_SETCHECK, default_cfg_xmiloopz);
    SendDlgItemMessage(IDC_FF7LOOPZ, BM_SETCHECK, default_cfg_ff7loopz);
    SendDlgItemMessage(IDC_EMIDI_EX, BM_SETCHECK, default_cfg_emidi_exclusion);
    SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_SETCHECK, default_cfg_filter_instruments);
    SendDlgItemMessage(IDC_FILTER_BANKS, BM_SETCHECK, default_cfg_filter_banks);
    SendDlgItemMessage(IDC_NUKE_PANNING, BM_SETCHECK, DefaultMSPanning);
    SendDlgItemMessage(IDC_MIDI_FLAVOR, CB_SETCURSEL, DefaultMIDIFlavor);
    SendDlgItemMessage(IDC_MIDI_EFFECTS, BM_SETCHECK, !DefaultMIDIEffects);

    {
        SelectedItem = 0;

        for (size_t i = 0; i < _ADLBanks.get_count(); ++i)
        {
            if (_ADLBanks[i].number == DefaultADLBank)
            {
                SelectedItem = (int)i;
                break;
            }
        }

        SendDlgItemMessage(IDC_ADL_BANK, CB_SETCURSEL, (WPARAM)SelectedItem);
    }

    SendDlgItemMessage(IDC_ADL_PANNING, BM_SETCHECK, DefaultADLPanning);
    SetDlgItemInt(IDC_ADL_CHIPS, DefaultADLChipCount, 0);

#ifdef FLUIDSYNTHSUPPORT
    SendDlgItemMessage(IDC_RESAMPLING, CB_SETCURSEL, 2);
#else
    SendDlgItemMessage(IDC_RESAMPLING_MODE, CB_SETCURSEL, DefaultResamplingMode);
#endif

    SendDlgItemMessage(IDC_NUKE_PRESET, CB_SETCURSEL, (WPARAM)NukePlayer::GetPresetIndex(DefaultMSSynth, DefaultMSBank));

    SendDlgItemMessage(IDC_MIDI_FLAVOR, CB_SETCURSEL, (WPARAM)CfgMIDIFlavor);

    _VSTiConfig.resize(0);

    OnChanged();
}
#pragma endregion

#pragma region("CDialogImpl")
/// <summary>
/// Initializes the dialog.
/// </summary>
BOOL Preferences::OnInitDialog(CWindow, LPARAM)
{
#ifdef DEBUG_DIALOG
    long BaseUnits = ::GetDialogBaseUnits();

    int TemplateUnitX = ::MulDiv(498, 4, LOWORD(BaseUnits));
    int TemplateUnitY = ::MulDiv(468, 8, HIWORD(BaseUnits));
#endif

    int PlayerType = CfgPlayerType;

    _PlugInIdToIndex[PlayerType] = -1;
    _IndexToPlugInId[-1] = -1;

    for (size_t i = 0; i < _countof(BuiltInPlugins); ++i)
    {
        _PlugInIdToIndex[BuiltInPlugins[i].Id] = (int)i;

        if (BuiltInPlugins[i].plugin_number_alt >= 0)
            _PlugInIdToIndex[BuiltInPlugins[i].plugin_number_alt] = (int)i;
    }

    int PlugInIndex = -1;

    if (PlayerType != PlayerTypeVSTi && PlayerType != PlayerTypeDirectX)
    {
        PlugInIndex = _PlugInIdToIndex[PlayerType];

        if (PlugInIndex < 0 || !BuiltInPlugins[PlugInIndex].IsPresent(this))
        {
            PlayerType = DefaultPlayerType;
            PlugInIndex = _PlugInIdToIndex[DefaultPlayerType];
        }
        else
        if (BuiltInPlugins[PlugInIndex].plugin_number_alt == PlayerType)
            PlayerType = BuiltInPlugins[PlugInIndex].Id;
    }

    size_t VSTiPluginIndex = (size_t)~0;

    {
        CWindow w = GetDlgItem(IDC_PLUGIN);

        {
            _ReportedPlugInCount = 0;

            _PluginPresentMap[PlayerType] = -1;

            for (size_t i = 0; i < _countof(BuiltInPlugins); ++i)
            {
                const BuiltInPlugin& bip = BuiltInPlugins[i];

                if (bip.IsPresent(this))
                {
                    _PluginPresentMap[bip.Id] = _ReportedPlugInCount;

                    if (bip.plugin_number_alt >= 0)
                        _PluginPresentMap[bip.plugin_number_alt] = _ReportedPlugInCount;

                    _IndexToPlugInId[_ReportedPlugInCount] = bip.Id;
                    ++_ReportedPlugInCount;

                    ::uSendMessageText(w, CB_ADDSTRING, 0, bip.Name);
                }
            }
        }
    }

 #pragma region("VSTi")
    {
        {
            CfgVSTiSearchPath.get(_VSTiSearchPath);

            ::uSetDlgItemText(m_hWnd, IDC_VST_PATH, !_VSTiSearchPath.is_empty() ? _VSTiSearchPath : DefaultPathMessage);
        }

        GetVSTiPlugins();

        size_t VSTiCount = _VSTiPlugIns.get_size();

        if (VSTiCount > 0)
        {
            CWindow w = GetDlgItem(IDC_PLUGIN);

            pfc::string8 Text; Text << "Found " << pfc::format_int((t_int64)_VSTiPlugIns.get_size()).c_str() << " VSTi plug-ins."; console::print(Text);

            for (size_t i = 0, j = VSTiCount; i < j; ++i)
            {
                _IndexToPlugInId[_ReportedPlugInCount + (int)i] = 1;

                ::uSendMessageText(w, CB_ADDSTRING, 0, _VSTiPlugIns[i].Name.c_str());

                if ((PlayerType == PlayerTypeVSTi) && (::stricmp_utf8(_VSTiPlugIns[i].PathName.c_str(), CfgVSTiFilePath) == 0))
                    VSTiPluginIndex = i;
            }
        }
#pragma endregion

#pragma region("SoundFont")
        {
            _SoundFontPath = CfgSoundFontFilePath;

            const char * FileName;

            if (_SoundFontPath.is_empty())
                FileName = DefaultPathMessage;
            else
                FileName = _SoundFontPath.get_ptr() + _SoundFontPath.scan_filename();

            ::uSetDlgItemText(m_hWnd, IDC_SOUNDFONT_FILE_PATH, FileName);
        }
#pragma endregion

#pragma region("Munt")
        {
            _MuntPath = CfgMuntFilePath;

            const char * FileName;

            if (_MuntPath.is_empty())
                FileName = DefaultPathMessage;
            else
                FileName = _MuntPath;

            ::uSetDlgItemText(m_hWnd, IDC_MUNT_FILE_PATH, FileName);
        }
#pragma endregion

#ifdef DXISUPPORT
        unsigned dxi_selected = ~0;

        dxi_plugins.set_count(0);

        ::CoInitialize(NULL);

        {
            CPlugInInventory theInventory;

            if (SUCCEEDED(theInventory.EnumPlugIns()))
            {
                unsigned count = theInventory.GetCount();
                pfc::string8_fastalloc name;
                CLSID theClsid;

                for (unsigned i = 0; i < count; ++i)
                {
                    if (SUCCEEDED(theInventory.GetInfo(i, &theClsid, name)))
                    {
                        dxi_plugins.append_single(theClsid);
                        uSendMessageText(w, CB_ADDSTRING, 0, name);

                        if (theClsid == cfg_dxi_plugin.get_value())
                            dxi_selected = i;

                        plugins_present_reverse_map[plugins_reported + vsti_count + i] = 5;
                    }
                }
            }
        }

        ::CoUninitialize();
#endif
    }

    if ((PlayerType == PlayerTypeVSTi) && (VSTiPluginIndex == ~0))
    {
        PlayerType = DefaultPlayerType;
        PlugInIndex = _PlugInIdToIndex[DefaultPlayerType];
    }

    if ((PlayerType == PlayerTypeEmuDeMIDI) && _IsRunning)
        GetDlgItem(IDC_SAMPLERATE).EnableWindow(FALSE);

    if ((PlayerType != PlayerTypeFluidSynth) && (PlayerType != PlayerTypeBASSMIDI))
    {
        GetDlgItem(IDC_SOUNDFONT_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_SOUNDFONT_FILE_PATH).EnableWindow(FALSE);
        GetDlgItem(IDC_RESAMPLING_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_RESAMPLING_MODE).EnableWindow(FALSE);
        GetDlgItem(IDC_CACHED_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_CACHED).EnableWindow(FALSE);
    }

    if (PlayerType != PlayerTypeSuperMunt)
    {
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_SHOW);
    }

    if (PlayerType != PlayerTypeADL)
    {
        GetDlgItem(IDC_ADL_BANK_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_BANK).EnableWindow(FALSE);
    }

    if (PlayerType != PlayerTypeADL && PlayerType != PlayerTypeOPN)
    {
        GetDlgItem(IDC_ADL_CHIPS_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_CHIPS).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_PANNING).EnableWindow(FALSE);
    }

    if (PlayerType != PlayerTypeVSTi)
    {
        GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(FALSE);
    }
    else
    {
        GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(_VSTiPlugIns[VSTiPluginIndex].HasEditor);
        _VSTiConfig = CfgVSTiConfig[_VSTiPlugIns[VSTiPluginIndex].Id];
    }

    if (PlayerType != PlayerTypeNuke)
    {
        GetDlgItem(IDC_NUKE_PRESET_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_NUKE_PRESET).EnableWindow(FALSE);
        GetDlgItem(IDC_NUKE_PANNING).EnableWindow(FALSE);
    }

    if ((PlayerType != PlayerTypeVSTi) && (PlayerType != PlayerTypeFluidSynth) && (PlayerType != PlayerTypeBASSMIDI) && (PlayerType != PlayerTypeSecretSauce))
    {
        GetDlgItem(IDC_FILTER_GROUP).EnableWindow(FALSE);

        GetDlgItem(IDC_MIDI_FLAVOR_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_MIDI_FLAVOR).EnableWindow(FALSE);
        GetDlgItem(IDC_MIDI_EFFECTS).EnableWindow(FALSE);
    }

    // Set the selected plug-in.
    {
        CWindow w = GetDlgItem(IDC_PLUGIN);

        int SelectedPlugInIndex = -1;

        if (PlayerType == PlayerTypeVSTi)
            SelectedPlugInIndex = _ReportedPlugInCount + (int)VSTiPluginIndex;
    #ifdef DXISUPPORT
        else
        if (PlugInId == PlayerTypeDirectX)
        {
            if (dxi_selected != ~0)
                SelectedPlugInIndex = _ReportedPlugInCount + vsti_count + dxi_selected;
            else
                plugin = 0;
        }
    #endif
        else
            SelectedPlugInIndex = _PluginPresentMap[PlayerType];

        ::SendMessage(w, CB_SETCURSEL, (WPARAM)SelectedPlugInIndex, (LPARAM)0);
    }

    // Set the selected sample rate.
    {
        char Text[16];

        for (size_t i = _countof(SampleRates); i--;)
        {
            if (SampleRates[i] != CfgSampleRate)
            {
                _itoa_s(SampleRates[i], Text, _countof(Text), 10);
                CfgSampleRateHistory.add_item(Text);
            }
        }

        _itoa_s(CfgSampleRate, Text, _countof(Text), 10);
        CfgSampleRateHistory.add_item(Text);

        auto w = GetDlgItem(IDC_SAMPLERATE);

        CfgSampleRateHistory.setup_dropdown(w);

        ::SendMessage(w, CB_SETCURSEL, 0, 0);
    }

    // Set the selected loop types.
    {
        static const char * LoopTypeDescription[] =
        {
            "Never loop",
            "Never, add 1s decay time",
            "Loop and fade when detected",
            "Always loop and fade",
            "Play indefinitely when detected",
            "Play indefinitely"
        };

        {
            auto w1 = GetDlgItem(IDC_LOOP_PLAYBACK);
            auto w2 = GetDlgItem(IDC_LOOP_OTHER);

            for (size_t i = 0; i < _countof(LoopTypeDescription); ++i)
            {
                ::uSendMessageText(w1, CB_ADDSTRING, 0, LoopTypeDescription[i]);
                ::uSendMessageText(w2, CB_ADDSTRING, 0, LoopTypeDescription[i]);
            }
 
            ::SendMessage(w1, CB_SETCURSEL, (WPARAM)CfgLoopTypePlayback, 0);
            ::SendMessage(w2, CB_SETCURSEL, (WPARAM)CfgLoopTypeOther, 0);
        }
    }

    SendDlgItemMessage(IDC_THLOOPZ, BM_SETCHECK, (WPARAM)cfg_thloopz);
    SendDlgItemMessage(IDC_RPGMLOOPZ, BM_SETCHECK, (WPARAM)cfg_rpgmloopz);
    SendDlgItemMessage(IDC_XMILOOPZ, BM_SETCHECK, (WPARAM)cfg_xmiloopz);
    SendDlgItemMessage(IDC_FF7LOOPZ, BM_SETCHECK, (WPARAM)cfg_ff7loopz);

    SendDlgItemMessage(IDC_EMIDI_EX, BM_SETCHECK, (WPARAM)cfg_emidi_exclusion);

    SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_SETCHECK, (WPARAM)cfg_filter_instruments);
    SendDlgItemMessage(IDC_FILTER_BANKS, BM_SETCHECK, (WPARAM)cfg_filter_banks);

    SendDlgItemMessage(IDC_NUKE_PANNING, BM_SETCHECK, (WPARAM)CfgMSPanning);

#pragma region("ADL")
    {
        const char * const * BankNames = adl_getBankNames();
        const size_t BankCount = (size_t)adl_getBanksCount();

        if (BankNames && (BankCount > 0))
        {
            for (size_t i = 0; i < BankCount; ++i)
                _ADLBanks += adl_bank((int)i, BankNames[i]);

            _ADLBanks.sort();
        }

        {
            size_t SelectedBank = 0;

            auto w = GetDlgItem(IDC_ADL_BANK);

            for (size_t i = 0; i < _ADLBanks.get_count(); ++i)
            {
                ::uSendMessageText(w, CB_ADDSTRING, 0, _ADLBanks[i].name);

                if (_ADLBanks[i].number == CfgADLBank)
                    SelectedBank = i;
            }

            w.SendMessage(CB_SETCURSEL, SelectedBank);
        }

        {
            static const char * ChipCounts[] = { "1", "2", "5", "10", "25", "50", "100" };

            auto w = GetDlgItem(IDC_ADL_CHIPS);

            for (size_t i = 0; i < _countof(ChipCounts); ++i)
                ::uSendMessageText(w, CB_ADDSTRING, 0, ChipCounts[i]);

            SetDlgItemInt(IDC_ADL_CHIPS, (UINT)CfgADLChipCount, 0);
        }

        SendDlgItemMessage(IDC_ADL_PANNING, BM_SETCHECK, (WPARAM)CfgADLPanning);
    }
#pragma endregion

#ifdef BASSMIDISUPPORT
    {
        auto w = GetDlgItem(IDC_RESAMPLING_MODE);

        ::uSendMessageText(w, CB_ADDSTRING, 0, "Linear interpolation");
        ::uSendMessageText(w, CB_ADDSTRING, 0, "8pt Sinc interpolation");
        ::uSendMessageText(w, CB_ADDSTRING, 0, "16pt Sinc interpolation");

        ::SendMessage(w, CB_SETCURSEL, (WPARAM)CfgResamplingMode, 0);
    }
#endif

#ifdef FLUIDSYNTHSUPPORT
    {
        auto w = GetDlgItem(IDC_RESAMPLING);

        ::uSendMessageText(w, CB_ADDSTRING, 0, "No interpolation");
        ::uSendMessageText(w, CB_ADDSTRING, 0, "Linear interpolation");
        ::uSendMessageText(w, CB_ADDSTRING, 0, "Cubic interpolation");
        ::uSendMessageText(w, CB_ADDSTRING, 0, "7pt Sinc interpolation");

        if (Cfg_FluidSynthInterpolationMethod == 0)
            ::SendMessage(w, CB_SETCURSEL, 0, 0);
        else
        if (Cfg_FluidSynthInterpolationMethod == 1)
            ::SendMessage(w, CB_SETCURSEL, 1, 0);
        else
        if (Cfg_FluidSynthInterpolationMethod == 4)
            ::SendMessage(w, CB_SETCURSEL, 2, 0);
        else
        if (Cfg_FluidSynthInterpolationMethod == 7)
            ::SendMessage(w, CB_SETCURSEL, 3, 0);
        else
            ::SendMessage(w, CB_SETCURSEL, 3, 0);
    }
#endif

    {
        auto w = GetDlgItem(IDC_MUNT_GM_SET);

        for (size_t i = 0; i < _countof(_MuntGMSets); ++i)
            ::uSendMessageText(w, CB_ADDSTRING, 0, _MuntGMSets[i]);

        ::SendMessage(w, CB_SETCURSEL, (WPARAM)CfgMuntGMSet, 0);
    }

    {
        auto w = GetDlgItem(IDC_NUKE_PRESET);

        size_t PresetNumber = 0;

        NukePlayer::EnumeratePresets([w, PresetNumber] (const pfc::string8 name, unsigned int synth, unsigned int bank) mutable noexcept
        {
            ::uSendMessageText(w, CB_ADDSTRING, 0, name.c_str());

            if ((synth == (unsigned int)CfgMSSynthesizer) && (bank == (unsigned int)CfgMSBank))
                ::SendMessage(w, CB_SETCURSEL, PresetNumber, 0);

            PresetNumber++;
        });
    }

    {
        auto w = GetDlgItem(IDC_MIDI_FLAVOR);

        ::uSendMessageText(w, CB_ADDSTRING, 0, "Default");
        ::uSendMessageText(w, CB_ADDSTRING, 0, "GM");
        ::uSendMessageText(w, CB_ADDSTRING, 0, "GM2");
        ::uSendMessageText(w, CB_ADDSTRING, 0, "GS SC-55");
        ::uSendMessageText(w, CB_ADDSTRING, 0, "GS SC-88");
        ::uSendMessageText(w, CB_ADDSTRING, 0, "GS SC-88 Pro");
        ::uSendMessageText(w, CB_ADDSTRING, 0, "GS SC-8820");
        ::uSendMessageText(w, CB_ADDSTRING, 0, "XG");

        ::SendMessage(w, CB_SETCURSEL, (WPARAM)CfgMIDIFlavor, 0);

        SendDlgItemMessage(IDC_MIDI_EFFECTS, BM_SETCHECK, !CfgAllowMIDIEffects);
    }

#ifndef BASSMIDISUPPORT
    uSetWindowText(GetDlgItem(IDC_CACHED), "No info.");
#endif

    _HasSecretSauce = HasSecretSauce();

    SetTimer(ID_REFRESH, 20);

    _IsBusy = false;

    _DarkModeHooks.AddDialogWithControls(*this);

    return FALSE;
}

void Preferences::OnEditChange(UINT, int, CWindow)
{
    OnChanged();
}

void Preferences::OnSelectionChange(UINT, int, CWindow)
{
    OnChanged();
}

void Preferences::OnButtonClick(UINT, int, CWindow)
{
    OnChanged();
}

void Preferences::OnButtonConfig(UINT, int, CWindow)
{
    int SelectedIndex = (int)GetDlgItem(IDC_PLUGIN).SendMessage(CB_GETCURSEL, 0, 0);

    if ((SelectedIndex >= _ReportedPlugInCount) && SelectedIndex < (int)(_ReportedPlugInCount + _VSTiPlugIns.get_count()))
    {
        _IsBusy = true;
        OnChanged();

        VSTiPlayer Player;

        if (Player.LoadVST(_VSTiPlugIns[(size_t)(SelectedIndex - _ReportedPlugInCount)].PathName.c_str()))
        {
            if (_VSTiConfig.size())
                Player.setChunk(&_VSTiConfig[0], (unsigned long)_VSTiConfig.size());

            Player.displayEditorModal();
            Player.getChunk(_VSTiConfig);
        }

        _IsBusy = false;
        OnChanged();
    }
}

void Preferences::OnPlugInChange(UINT, int, CWindow w)
{
    int PlugInId = 0;

    int SelectedIndex = (int)::SendMessage(w, CB_GETCURSEL, 0, 0);

    {
        if ((SelectedIndex >= _ReportedPlugInCount) && (SelectedIndex < (int)(_ReportedPlugInCount + _VSTiPlugIns.get_count())))
            PlugInId = PlayerTypeVSTi;
    #ifdef DXISUPPORT
        else
        if (plugin_selected >= plugins_reported + _VSTiPlugins.get_count())
            PlugInId = PlayerTypeDirectX;
    #endif
        else
            PlugInId = _IndexToPlugInId[SelectedIndex];
    }

    GetDlgItem(IDC_SAMPLERATE).EnableWindow(PlugInId || !_IsRunning);

    GetDlgItem(IDC_SOUNDFONT_TEXT).EnableWindow(PlugInId == 2 || PlugInId == 4);
    GetDlgItem(IDC_SOUNDFONT_FILE_PATH).EnableWindow(PlugInId == 2 || PlugInId == 4);

    GetDlgItem(IDC_RESAMPLING_TEXT).EnableWindow(PlugInId == 2 || PlugInId == 4);
    GetDlgItem(IDC_RESAMPLING_MODE).EnableWindow(PlugInId == 2 || PlugInId == 4);

    GetDlgItem(IDC_CACHED_TEXT).EnableWindow(PlugInId == 2 || PlugInId == 4);
    GetDlgItem(IDC_CACHED).EnableWindow(PlugInId == 2 || PlugInId == 4);

    GetDlgItem(IDC_ADL_BANK_TEXT).EnableWindow(PlugInId == 6);
    GetDlgItem(IDC_ADL_BANK).EnableWindow(PlugInId == 6);
    GetDlgItem(IDC_ADL_CHIPS_TEXT).EnableWindow(PlugInId == 6 || PlugInId == 9);
    GetDlgItem(IDC_ADL_CHIPS).EnableWindow(PlugInId == 6 || PlugInId == 9);
    GetDlgItem(IDC_ADL_PANNING).EnableWindow(PlugInId == 6 || PlugInId == 9);

    GetDlgItem(IDC_NUKE_PRESET_TEXT).EnableWindow(PlugInId == 9);
    GetDlgItem(IDC_NUKE_PRESET).EnableWindow(PlugInId == 9);
    GetDlgItem(IDC_NUKE_PANNING).EnableWindow(PlugInId == 9);

    {
        bool enable = (PlugInId == 1) || (PlugInId == 2) || (PlugInId == 4) || (PlugInId == 10);

        GetDlgItem(IDC_FILTER_GROUP).EnableWindow(enable);
        GetDlgItem(IDC_MIDI_FLAVOR_TEXT).EnableWindow(enable);
        GetDlgItem(IDC_MIDI_FLAVOR).EnableWindow(enable);
        GetDlgItem(IDC_MIDI_EFFECTS).EnableWindow(enable);
    }

    if (PlugInId == 3)
    {
        GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(FALSE);
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_SHOW);
    }
    else
    if (_VSTiPlugIns.get_count() == 0)
    {
        GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(FALSE);
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_HIDE);
    }
    else
    {
        GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(FALSE);
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_HIDE);
    }

    {
        bool Enable = (SelectedIndex >= _ReportedPlugInCount) && (PlugInId < _ReportedPlugInCount + (int)_VSTiPlugIns.get_count()) && _VSTiPlugIns[(size_t)(SelectedIndex - _ReportedPlugInCount)].HasEditor;

        GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(Enable);
    }

    if ((SelectedIndex >= _ReportedPlugInCount) && (SelectedIndex < (int)(_ReportedPlugInCount + _VSTiPlugIns.get_count())))
        _VSTiConfig = CfgVSTiConfig[_VSTiPlugIns[(size_t)(SelectedIndex - _ReportedPlugInCount)].Id];

    OnChanged();
}

void Preferences::OnSetFocus(UINT, int, CWindow w)
{
    SetFocus();

    if (w == GetDlgItem(IDC_VST_PATH))
    {
        pfc::string8 DirectoryPath = _VSTiSearchPath;

        DirectoryPath.truncate_to_parent_path();

        if (::uBrowseForFolder(m_hWnd, "Locate VSTi plug-ins...", DirectoryPath))
        {
            _VSTiSearchPath = DirectoryPath;

            ::uSetWindowText(w, !_VSTiSearchPath.is_empty() ? _VSTiSearchPath : DefaultPathMessage);
        }
    }
    else
    if (w == GetDlgItem(IDC_SOUNDFONT_FILE_PATH))
    {
        pfc::string8 DirectoryPath = _SoundFontPath;

        DirectoryPath.truncate_to_parent_path();

        pfc::string8 FilePath = _SoundFontPath;

        if (::uGetOpenFileName(m_hWnd, "SoundFont and list files|*.sf2;*.sf3;*.sflist"
        #ifdef SF2PACK
            "*.sf2pack;*.sfogg;"
        #endif
        #ifdef BASSMIDISUPPORT
            ";*.json"
        #endif

            "*.sflist|SoundFont files|*.sf2;*.sf3"
        #ifdef SF2PACK
            ";*.sf2pack;*.sfogg;"
        #endif

            "|SoundFont list files|*.sflist"
        #ifdef BASSMIDISUPPORT
            ";*.json"
        #endif
            ,
            0, "sf2", "Choose a SoundFont bank or list...", DirectoryPath, FilePath, FALSE))
        {
            _SoundFontPath = FilePath;

            ::uSetWindowText(w, !_SoundFontPath.isEmpty() ? _SoundFontPath : DefaultPathMessage);

            OnChanged();
        }
    }
    else
    if (w == GetDlgItem(IDC_MUNT_FILE_PATH))
    {
        pfc::string8 DirectoryPath;

        DirectoryPath.truncate_to_parent_path();

        if (::uBrowseForFolder(m_hWnd, "Locate MT-32 or CM-32L ROM sets...", DirectoryPath))
        {
            _MuntPath = DirectoryPath;

            ::uSetWindowText(w, !_MuntPath.isEmpty() ? _MuntPath : DefaultPathMessage);

            OnChanged();
        }
    }
}

void Preferences::OnTimer(UINT_PTR eventId)
{
    if (eventId != ID_REFRESH)
        return;
 
    GetDlgItem(IDC_SAMPLERATE).EnableWindow(CfgPlayerType || !_IsRunning);

#ifdef BASSMIDISUPPORT
    _CacheStatusText.reset();

    uint64_t SamplesMax, SamplesLoaded;

    if (::GetSoundFontStatistics(SamplesMax, SamplesLoaded))
        _CacheStatusText << pfc::format_file_size_short(SamplesLoaded) << " / " << pfc::format_file_size_short(SamplesMax);
    else
        _CacheStatusText = "BASS not loaded.";

    if (::strcmp(_CacheStatusText, _CacheStatusTextCurrent) != 0)
    {
        _CacheStatusTextCurrent = _CacheStatusText;

        uSetWindowText(GetDlgItem(IDC_CACHED), _CacheStatusText);
    }
#endif
}

bool Preferences::HasChanged()
{
    // returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
    bool changed = false;

    if (!changed && GetDlgItemInt(IDC_SAMPLERATE, NULL, FALSE) != (UINT)CfgSampleRate) changed = true;
    if (!changed && SendDlgItemMessage(IDC_LOOP_PLAYBACK, CB_GETCURSEL) != CfgLoopTypePlayback) changed = true;
    if (!changed && SendDlgItemMessage(IDC_LOOP_OTHER, CB_GETCURSEL) != CfgLoopTypeOther) changed = true;
    if (!changed && SendDlgItemMessage(IDC_THLOOPZ, BM_GETCHECK) != cfg_thloopz) changed = true;
    if (!changed && SendDlgItemMessage(IDC_RPGMLOOPZ, BM_GETCHECK) != cfg_rpgmloopz) changed = true;
    if (!changed && SendDlgItemMessage(IDC_XMILOOPZ, BM_GETCHECK) != cfg_xmiloopz) changed = true;
    if (!changed && SendDlgItemMessage(IDC_FF7LOOPZ, BM_GETCHECK) != cfg_ff7loopz) changed = true;
    if (!changed && SendDlgItemMessage(IDC_EMIDI_EX, BM_GETCHECK) != cfg_emidi_exclusion) changed = true;
    if (!changed && SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_GETCHECK) != cfg_filter_instruments) changed = true;
    if (!changed && SendDlgItemMessage(IDC_FILTER_BANKS, BM_GETCHECK) != cfg_filter_banks) changed = true;
    if (!changed && SendDlgItemMessage(IDC_NUKE_PANNING, BM_GETCHECK) != CfgMSPanning) changed = true;

#ifdef FLUIDSYNTHSUPPORT
    if (!changed)
    {
        int interp_method = SendDlgItemMessage(IDC_RESAMPLING, CB_GETCURSEL);

        if (interp_method == 2)
            interp_method = 4;
        else
        if (interp_method == 3)
            interp_method = 7;

        if (interp_method != Cfg_FluidSynthInterpolationMethod)
            changed = true;
    }
#endif

#ifdef BASSMIDISUPPORT
    if (!changed && SendDlgItemMessage(IDC_RESAMPLING_MODE, CB_GETCURSEL) != CfgResamplingMode)
        changed = true;
#endif
    if (!changed && SendDlgItemMessage(IDC_MIDI_FLAVOR, CB_GETCURSEL) != CfgMIDIFlavor)
        changed = true;

    if (!changed && !SendDlgItemMessage(IDC_MIDI_EFFECTS, BM_GETCHECK) != CfgAllowMIDIEffects)
        changed = true;

    if (!changed)
    {
        size_t PresetNumber = (size_t)SendDlgItemMessage(IDC_NUKE_PRESET, CB_GETCURSEL);

        unsigned int Synth;
        unsigned int Bank;

        NukePlayer::GetPreset(PresetNumber, Synth, Bank);

        changed = (!(Synth == (unsigned int)CfgMSSynthesizer && Bank == (unsigned int)CfgMSBank));
    }

    if (!changed)
    {
        int SelectedIndex = (int)SendDlgItemMessage(IDC_ADL_BANK, CB_GETCURSEL);

        if ((SelectedIndex < 0) || (SelectedIndex >= (int)_ADLBanks.get_count()))
            SelectedIndex = 0;

        if (_ADLBanks[(t_size)SelectedIndex].number != (int)CfgADLBank)
            changed = true;
    }

    if (!changed && GetDlgItemInt(IDC_ADL_CHIPS, NULL, FALSE) != (UINT)CfgADLChipCount)
        changed = true;

    if (!changed && SendDlgItemMessage(IDC_ADL_PANNING, BM_GETCHECK) != CfgADLPanning)
        changed = true;

    if (!changed && SendDlgItemMessage(IDC_MUNT_GM_SET, CB_GETCURSEL) != CfgMuntGMSet)
        changed = true;

    if (!changed)
    {
        int plugin_selected = (int)SendDlgItemMessage(IDC_PLUGIN, CB_GETCURSEL);
        int plugin = -1;

        if ((plugin_selected >= _ReportedPlugInCount) && (plugin_selected < _ReportedPlugInCount + (int)_VSTiPlugIns.get_count()))
            plugin = 1;
    #ifdef DXISUPPORT
        else
        if (plugin_selected >= plugins_reported + _VSTiPlugins.get_count())
            plugin = 5;
    #endif
        else
            plugin = _IndexToPlugInId[plugin_selected];

        if (plugin != CfgPlayerType)
            changed = true;

        if (!changed && plugin == 1)
        {
            if (::stricmp_utf8(CfgVSTiFilePath, _VSTiPlugIns[(size_t)(plugin_selected - _ReportedPlugInCount)].PathName.c_str()))
                changed = true;

            if (!changed)
            {
                t_uint32 unique_id = _VSTiPlugIns[(size_t)(plugin_selected - _ReportedPlugInCount)].Id;

                if (_VSTiConfig.size() != CfgVSTiConfig[unique_id].size() || (_VSTiConfig.size() && memcmp(&_VSTiConfig[0], &CfgVSTiConfig[unique_id][0], _VSTiConfig.size())))
                    changed = true;
            }
        }
    #ifdef DXISUPPORT
        else
        if (!changed && plugin == 5)
        {
            if (dxi_plugins[plugin_selected - _VSTiPlugins.get_count() - plugins_reported] != cfg_dxi_plugin.get_value()) changed = true;
        }
    #endif
    }

    if (!changed && (::stricmp_utf8(_SoundFontPath, CfgSoundFontFilePath) != 0))
        changed = true;

    if (!changed && (::stricmp_utf8(_MuntPath, CfgMuntFilePath) != 0))
        changed = true;

    return changed;
}

void Preferences::OnChanged()
{
    _Callback->on_state_changed();
}
#pragma endregion

#pragma region("VSTi")
/// <summary>
/// Gets all the VTSi plugins if a root directory has been specified.
/// </summary>
void Preferences::GetVSTiPlugins(const char * pathName, puFindFile findFile)
{
    if (findFile == nullptr)
    {
        _VSTiPlugIns.set_size(0);

        CfgVSTiSearchPath.get(_VSTiSearchPath);

        if (_VSTiSearchPath.is_empty())
        {
            GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(FALSE);

            return;
        }

        console::print("Enumerating VSTi plug-ins...");

        if (_VSTiSearchPath[_VSTiSearchPath.length() - 1] != '\\')
            _VSTiSearchPath.add_byte('\\');

        _VSTiSearchPath += "*.*";

        pathName = _VSTiSearchPath;

        findFile = ::uFindFirstFile(_VSTiSearchPath);
    }

    if (findFile == nullptr)
        return;

    do
    {
        pfc::string8 PathName(pathName);

        PathName.truncate(PathName.length() - 3);
        PathName += findFile->GetFileName();

        // Enter all subdirectories to look voor plug-ins.
        if (findFile->IsDirectory() && ::strcmp(findFile->GetFileName(), ".") && ::strcmp(findFile->GetFileName(), ".."))
        {
            PathName.add_byte('\\');
            PathName += "*.*";

            puFindFile FindFile = ::uFindFirstFile(PathName);

            if (FindFile)
                GetVSTiPlugins(PathName, FindFile);
        }
        else
        {
            if ((PathName.length() < 5) || (pfc::stricmp_ascii(PathName.get_ptr() + PathName.length() - 4, ".dll") != 0))
                continue;

            // Examine all files.
            if (findFile->GetFileSize())
            {
                pfc::string8 Text; Text << "Examining \"" << PathName << "\"..."; console::print(Text);

                VSTiPlayer Player;

                if (Player.LoadVST(PathName))
                {
                    VSTiPlugin Plugin;

                    Plugin.Name = "VST ";
                    Plugin.PathName = PathName;

                    pfc::string8 VendorName;

                    Player.getVendorString(VendorName);

                    pfc::string8 ProductName;

                    Player.getProductString(ProductName);

                    if (VendorName.length() || ProductName.length())
                    {
                        if ((VendorName.length() == 0) || ((ProductName.length() >= VendorName.length()) && (::strncmp(VendorName.c_str(), ProductName.c_str(), VendorName.length()) == 0)))
                        {
                            Plugin.Name.add_string(ProductName);
                        }
                        else
                        {
                            Plugin.Name += VendorName;

                            if (ProductName.length())
                                Plugin.Name.add_string(' ' + ProductName);
                        }
                    }
                    else
                        Plugin.Name = findFile->GetFileName();

                    Plugin.Id = (uint32_t)Player.getUniqueID();
                    Plugin.HasEditor = Player.hasEditor();

                    _VSTiPlugIns.append_single(Plugin);
                }
            }
        }
    }
    while (findFile->FindNext());

    delete findFile;
}
#pragma endregion

#pragma region("Secret Sauce")
struct SecretSauceInfo
{
    size_t FileSize;
    hasher_md5_result Hash;
};

static const SecretSauceInfo SecretSauceInfos[] =
{
    #pragma warning(disable: 4310)
    // 1.0.3 - 32 bit - 27,472,384 - d44d1b8c9a6f956ca2324f2f5d348c44
    { 27472384, { (char) 0xd4, 0x4d, 0x1b, (char) 0x8c, (char) 0x9a, 0x6f, (char) 0x95, 0x6c, (char) 0xa2, 0x32, 0x4f, 0x2f, 0x5d, 0x34, (char) 0x8c, 0x44 } },

    // 1.0.3 - 64 bit - 27,440,128 - f16b5eb9c7e204de7f9b3a829d2d5500
    { 27440128, { (char) 0xf1, 0x6b, 0x5e, (char) 0xb9, (char) 0xc7, (char) 0xe2, 0x04, (char) 0xde, 0x7f, (char) 0x9b, 0x3a, (char) 0x82, (char) 0x9d, 0x2d, 0x55, 0x00 } },

    // 1.0.6 - 32 bit - 27,319,296 - 6588e6aa17a57ba874e8b675114214f0
    { 27319296, { 0x65, (char) 0x88, (char) 0xe6, (char) 0xaa, 0x17, (char) 0xa5, 0x7b, (char) 0xa8, 0x74, (char) 0xe8, (char) 0xb6, 0x75, 0x11, 0x42, 0x14, (char) 0xf0 } },

    // 1.0.6 - 64 bit - 27,358,208 - 6abfbf61869fc436d76c93d1bc7e2735
    { 27358208, { 0x6a, (char) 0xbf, (char) 0xbf, 0x61, (char) 0x86, (char) 0x9f, (char) 0xc4, 0x36, (char) 0xd7, 0x6c, (char) 0x93, (char) 0xd1, (char) 0xbc, 0x7e, 0x27, 0x35 } },

    // 1.0.7 - 32 bit - 27,319,296 - 25830a6c2ff5751f3a55915fb60702f4
    { 27319296, { 0x25, (char) 0x83, 0x0a, 0x6c, 0x2f, (char) 0xf5, 0x75, 0x1f, 0x3a, 0x55, (char) 0x91, 0x5f, (char) 0xb6, 0x07, 0x02, (char) 0xf4 } },

    // 1.1.3 - 64 bit - 27,358,208 - 80f1e673d249d1cda67a2936326f866b
    { 27358208, { (char) 0x80, (char) 0xf1, (char) 0xe6, 0x73, (char) 0xd2, 0x49, (char) 0xd1, (char) 0xcd, (char) 0xa6, 0x7a, 0x29, 0x36, 0x32, 0x6f, (char) 0x86, 0x6b } },

    // 1.1.0 (S) - 64 bit - 27,358,208 - 3703e0dc7bd93abd4c29e1a03f1f6c0a
    { 27358208, { 0x37, 0x03, (char) 0xe0, (char) 0xdc, 0x7b, (char) 0xd9, 0x3a, (char) 0xbd, 0x4c, 0x29, (char) 0xe1, (char) 0xa0, 0x3f, 0x1f, 0x6c, 0x0a } },

    // 1.1.6 (S) - 64 bit - 27,347,456 - dbd9a30c168efef577d40a28d9adf37d
    { 27347456, { (char) 0xdb, (char) 0xd9, (char) 0xa3, 0x0c, 0x16, (char) 0x8e, (char) 0xfe, (char) 0xf5, 0x77, (char) 0xd4, 0x0a, 0x28, (char) 0xd9, (char) 0xad, (char) 0xf3, 0x7d } },
    #pragma warning(default: 4310)

    { 0, { 0 } }
};

bool Preferences::HasSecretSauce()
{
    FILE * fp = nullptr;

    {
        pfc::string8 PathName;

        CfgSecretSaucePath.get(PathName);

        if (PathName.is_empty())
            return false;

        PathName += "\\";
        PathName += _DLLFileName;

        pfc::stringcvt::string_os_from_utf8 FilePath(PathName);

        _tfopen_s(&fp, FilePath, _T("rb"));
    }

    bool rc = false;

    if (fp)
    {
        ::fseek(fp, 0, SEEK_END);

        size_t FileSize = (size_t)::ftell(fp);

        for (int i = 0; SecretSauceInfos[i].FileSize; ++i)
        {
            if (SecretSauceInfos[i].FileSize == FileSize)
            {
                ::fseek(fp, 0, SEEK_SET);

                static_api_ptr_t<hasher_md5> Hasher;
                hasher_md5_state HasherState;

                Hasher->initialize(HasherState);

                uint8_t Data[1024];
                size_t BytesReadTotal = 0;

                while (!::feof(fp))
                {
                    size_t BytesRead = ::fread(Data, 1, 1024, fp);

                    BytesReadTotal += BytesRead;

                    if (BytesRead != 0)
                        Hasher->process(HasherState, Data, BytesRead);

                    if (BytesRead < 1024)
                        break;
                }

                if (BytesReadTotal == FileSize)
                {
                    hasher_md5_result Hash = Hasher->get_result(HasherState);

                    if (Hash == SecretSauceInfos[i].Hash)
                    {
                        rc = true;
                        break;
                    }
                }
            }
        }

        ::fclose(fp);
    }

    return rc;
}
#pragma endregion

static preferences_page_factory_t<PreferencesPage> PreferencesPageFactory;
