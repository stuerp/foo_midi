
/** $VER: PreferencesRoot.cpp (2025.06.25) P. Stuer **/

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

#include <pfc/string-conv-lite.h>
#include <pfc/pathUtils.h>

#include <helpers/atl-misc.h>
#include <helpers/dropdown_helper.h>

#include "resource.h"

#include "Configuration.h"
#include "Preset.h"

#include "BMPlayer.h"
#include "FSPlayer.h"
#include "CLAPPlayer.h"
#include "NukePlayer.h"
#include "VSTiPlayer.h"

#include "VSTi.h"
#include "SecretSauce.h"
#include "CLAPHost.h"

#pragma hdrstop

extern volatile int _IsRunning;

static cfg_dropdown_history CfgSampleRateHistory({ 0x408aa155, 0x4c42, 0x42b5, { 0x8c, 0x3e, 0xd1, 0xc, 0x35, 0xdd, 0x5e, 0xf1 } }, 16);

#pragma warning(disable: 4820)

/// <summary>
/// Implements the main preferences page.
/// </summary>
class PreferencesRootPage : public CDialogImpl<PreferencesRootPage>, public preferences_page_instance
{
public:
    PreferencesRootPage(preferences_page_callback::ptr callback) noexcept : _IsBusy(false), _HasSecretSauce(), _HasFluidSynth(), _Callback(callback) { }

    PreferencesRootPage(const PreferencesRootPage&) = delete;
    PreferencesRootPage(const PreferencesRootPage&&) = delete;
    PreferencesRootPage& operator=(const PreferencesRootPage&) = delete;
    PreferencesRootPage& operator=(PreferencesRootPage&&) = delete;

    virtual ~PreferencesRootPage() { };

    #pragma region preferences_page_instance

    t_uint32 get_state() final;
    void apply() final;
    void reset() final;

    #pragma endregion

    // WTL message map
    BEGIN_MSG_MAP_EX(PreferencesRootPage)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_TIMER(OnTimer)

        #pragma region Output

        COMMAND_HANDLER_EX(IDC_PLAYER_TYPE, CBN_SELCHANGE, OnPlayerTypeChange)

        COMMAND_HANDLER_EX(IDC_CONFIGURE, BN_CLICKED, OnButtonConfig)

        DROPDOWN_HISTORY_HANDLER(IDC_SAMPLERATE, CfgSampleRateHistory)
        COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_EDITCHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_SELCHANGE, OnSelectionChange)

        #pragma endregion

        #pragma region Looping

        COMMAND_HANDLER_EX(IDC_LOOP_PLAYBACK, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_LOOP_OTHER, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_DECAY_TIME, EN_CHANGE, OnEditChange)

        #pragma endregion

        #pragma region MIDI

        COMMAND_HANDLER_EX(IDC_MIDI_FLAVOR, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_MIDI_EFFECTS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_MIDI_USE_SUPER_MUNT, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_MIDI_USE_VSTI_WITH_XG, BN_CLICKED, OnButtonClick)

        #pragma endregion

        #pragma region Miscellaneous

        COMMAND_HANDLER_EX(IDC_EMIDI_EXCLUSION, BN_CLICKED, OnButtonClick)

        COMMAND_HANDLER_EX(IDC_FILTER_INSTRUMENTS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_FILTER_BANKS, BN_CLICKED, OnButtonClick)

        COMMAND_HANDLER_EX(IDC_RPGMAKER_LOOPS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_LEAPFROG_LOOPS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_XMI_LOOPS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_TOUHOU_LOOPS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_FF7_LOOPS, BN_CLICKED, OnButtonClick)

        #pragma endregion

        #pragma region FluidSynth

        COMMAND_HANDLER_EX(IDC_FLUIDSYNTH_INTERPOLATION, CBN_SELCHANGE, OnSelectionChange)

        #pragma endregion

        #pragma region BASS MIDI

        COMMAND_HANDLER_EX(IDC_BASSMIDI_VOLUME, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_RESAMPLING_MODE, CBN_SELCHANGE, OnSelectionChange)

        #pragma endregion

        #pragma region Munt

        COMMAND_HANDLER_EX(IDC_MUNT_GM_SET, CBN_SELCHANGE, OnSelectionChange)

        #pragma endregion

        #pragma region Nuked OPL3

        COMMAND_HANDLER_EX(IDC_NUKE_PRESET, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_NUKE_PANNING, BN_CLICKED, OnButtonClick)

        #pragma endregion
    END_MSG_MAP()

    enum
    {
        IDD = IDD_PREFERENCES_ROOT
    };

    const UINT_PTR ID_REFRESH = 1000;

private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnTimer(UINT_PTR);

    void OnPlayerTypeChange(UINT, int, CWindow w);
    void OnEditChange(UINT, int, CWindow);
    void OnSelectionChange(UINT, int, CWindow);
    void OnButtonClick(UINT, int, CWindow);
    void OnButtonConfig(UINT, int, CWindow);

    bool HasChanged();
    void OnChanged();

    void UpdateDialog() const noexcept;

private:
    bool _IsBusy;

    #pragma region Player Type

    static bool PlayerIsAlwaysPresent(PreferencesRootPage *)
    {
        return true;
    }

    static bool PlayerIsNeverPresent(PreferencesRootPage *)
    {
        return false;
    }

    static bool IsFluidSynthPresent(PreferencesRootPage * p)
    {
        return p->_HasFluidSynth;
    }

    static bool IsSecretSaucePresent(PreferencesRootPage * p)
    {
        return p->_HasSecretSauce;
    }

    struct known_player_t
    {
        const char * Name;
        PlayerTypes Type;
        bool (*IsPresent)(PreferencesRootPage *);
    };

    static const known_player_t _KnownPlayers[];

    struct installed_player_t
    {
        PlayerTypes Type;
        pfc::string Name;
        fs::path PathName;  // Path name of the VSTi or CLAP plug-in
        size_t Index;       // Index with a CLAP plug-in

        size_t PlugInIndex; // Index in the VSTi or CLAP plug-in list

        installed_player_t() : Type(PlayerTypes::Unknown), Index(~0u), PlugInIndex(~0u) { }
        installed_player_t(PlayerTypes type, pfc::string name, fs::path pathName, size_t index, size_t plugInIndex) : Type(type), Name(name), PathName(pathName), Index(index), PlugInIndex(plugInIndex) { }

        bool operator ==(const installed_player_t & other) const noexcept
        {
            if (Type != other.Type)
                return false;

            if (((Type == PlayerTypes::VSTi) || (Type == PlayerTypes::CLAP)))
            {
                if (PathName != other.PathName)
                    return false;

                if (Type == PlayerTypes::CLAP)
                    return Index == other.Index;
            }

            return true;
        }

        bool SupportsMIDIFlavor() const noexcept
        {
            return ((Type == PlayerTypes::VSTi) || (Type == PlayerTypes::FluidSynth) || (Type == PlayerTypes::BASSMIDI) || (Type == PlayerTypes::SecretSauce));
        }
    };

    std::vector<installed_player_t> _InstalledPlayers;
    installed_player_t _SelectedPlayer;

    static const int _SampleRates[];

    struct InterpolationMethod
    {
        const wchar_t * Name;
        int Id;
    };

    static const InterpolationMethod _InterpolationMethods[];

    #pragma endregion

    #pragma region Secret Sauce

    bool _HasSecretSauce;

    #pragma endregion

    #pragma region FluidSynth

    bool _HasFluidSynth;

    #pragma endregion

    #pragma region BASS MIDI

    pfc::string _CacheStatusText;
    pfc::string _CacheStatusTextCurrent;

    #pragma endregion

#ifdef DXISUPPORT
    pfc::array_t<CLSID> dxi_plugins;
#endif

    const preferences_page_callback::ptr _Callback;

    fb2k::CCoreDarkModeHooks _DarkModeHooks;
};

/// <summary>
/// Contains all the plug-ins that are build into the component.
/// </summary>
const PreferencesRootPage::known_player_t PreferencesRootPage::_KnownPlayers[] =
{
    { "LibEDMIDI",      PlayerTypes::EmuDeMIDI,     PlayerIsAlwaysPresent },
    { "FluidSynth",     PlayerTypes::FluidSynth,    IsFluidSynthPresent },
    { "BASSMIDI",       PlayerTypes::BASSMIDI,      PlayerIsAlwaysPresent },
    { "DirectX",        PlayerTypes::DirectX,       PlayerIsNeverPresent },
    { "Super Munt GM",  PlayerTypes::SuperMunt,     PlayerIsAlwaysPresent },
    { "LibADLMIDI",     PlayerTypes::ADL,           PlayerIsAlwaysPresent },
    { "LibOPNMIDI",     PlayerTypes::OPN,           PlayerIsAlwaysPresent },
    { "OPL MIDI",       PlayerTypes::OPL,           PlayerIsNeverPresent },
    { "Nuked OPL3",     PlayerTypes::NukedOPL3,     PlayerIsAlwaysPresent },
    { "Nuked SC-55",    PlayerTypes::NukedSC55,     PlayerIsNeverPresent },
    { "Secret Sauce",   PlayerTypes::SecretSauce,   IsSecretSaucePresent },
    { "MCI",            PlayerTypes::MCI,           PlayerIsNeverPresent },
    { "FMMIDI",         PlayerTypes::FMMIDI,        PlayerIsAlwaysPresent },
};

const int PreferencesRootPage::_SampleRates[] = { 8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 49716, 64000, 88200, 96000 };

const PreferencesRootPage::InterpolationMethod PreferencesRootPage::_InterpolationMethods[] =
{
    { L"None", FLUID_INTERP_NONE },
    { L"Linear", FLUID_INTERP_LINEAR },
    { L"Cubic", FLUID_INTERP_4THORDER },
    { L"7th Order Sinc", FLUID_INTERP_7THORDER }
};

const char * _MuntSets[] =
{
    "Roland",
    "Sierra / King's Quest 6",
};

const size_t _MuntSetCount = _countof(_MuntSets);

#pragma region preferences_page_instance

/// <summary>
/// Gets the state of the Preference dialog.
/// </summary>
t_uint32 PreferencesRootPage::get_state()
{
    t_uint32 State = preferences_state::resettable | preferences_state::dark_mode_supported;

    if (HasChanged())
        State |= preferences_state::changed;

    if (_IsBusy)
        State |= preferences_state::busy;

    return State;
}

/// <summary>
/// Applies the dialog state to the configuration variables.
/// </summary>
void PreferencesRootPage::apply()
{
    // Player Type
    {
        int SelectedIndex = (int) SendDlgItemMessageW(IDC_PLAYER_TYPE, CB_GETCURSEL);

        if (SelectedIndex != -1)
        {
            _SelectedPlayer = _InstalledPlayers[(size_t) SelectedIndex];

            CfgPlayerType = (int) _SelectedPlayer.Type;

            if (_SelectedPlayer.Type == PlayerTypes::VSTi)
            {
                const auto & PlugIn = VSTi::PlugIns[_SelectedPlayer.PlugInIndex];

                CfgPlugInFilePath = PlugIn.PathName.c_str();
                CfgPlugInIndex    = ~0;

                CfgVSTiConfig[PlugIn.Id] = VSTi::Config;
            }
            else
            if (_SelectedPlayer.Type == PlayerTypes::CLAP)
            {
                const auto & PlugIn = foo_midi::clap_host_t::PlugIns[_SelectedPlayer.PlugInIndex];

                CfgPlugInFilePath = PlugIn.PathName.c_str();
                CfgPlugInIndex    = PlugIn.Index;
            }
            else
            {
                CfgPlugInFilePath = "";
                CfgPlugInIndex    = ~0;
            }
        }
    }

    // Configure
    {
    }

    // Sample Rate
    {
        UINT t = std::clamp(GetDlgItemInt(IDC_SAMPLERATE, NULL, FALSE), 6000u, 192000u);

        SetDlgItemInt(IDC_SAMPLERATE, t, FALSE);

        CfgSampleRateHistory.add_item(pfc::format_int(t));
        CfgSampleRate = (t_int32) t;
    }

    // Looping
    {
        CfgLoopTypePlayback     = (t_int32) SendDlgItemMessage(IDC_LOOP_PLAYBACK, CB_GETCURSEL);
        CfgLoopTypeOther        = (t_int32) SendDlgItemMessage(IDC_LOOP_OTHER, CB_GETCURSEL);

        CfgDecayTime            = (t_int32) GetDlgItemInt(IDC_DECAY_TIME, NULL, FALSE);
    }

    // MIDI Flavor and Effects
    {
        CfgMIDIFlavor         = (t_int32) SendDlgItemMessage(IDC_MIDI_FLAVOR, CB_GETCURSEL);

        CfgUseMIDIEffects       = (t_int32) SendDlgItemMessage(IDC_MIDI_EFFECTS, BM_GETCHECK) ? 0 : 1;
        CfgUseSuperMuntWithMT32 = (t_int32) SendDlgItemMessage(IDC_MIDI_USE_SUPER_MUNT, BM_GETCHECK);
        CfgUseVSTiWithXG        = (t_int32) SendDlgItemMessage(IDC_MIDI_USE_VSTI_WITH_XG, BM_GETCHECK);
    }

    // Miscellaneous
    {
        CfgEmuDeMIDIExclusion   = (t_int32) SendDlgItemMessage(IDC_EMIDI_EXCLUSION, BM_GETCHECK);

        CfgFilterInstruments    = (t_int32) SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_GETCHECK);
        CfgFilterBanks          = (t_int32) SendDlgItemMessage(IDC_FILTER_BANKS, BM_GETCHECK);

        CfgDetectTouhouLoops    = (t_int32) SendDlgItemMessage(IDC_TOUHOU_LOOPS, BM_GETCHECK);
        CfgDetectRPGMakerLoops  = (t_int32) SendDlgItemMessage(IDC_RPGMAKER_LOOPS, BM_GETCHECK);
        CfgDetectLeapFrogLoops  = (t_int32) SendDlgItemMessage(IDC_LEAPFROG_LOOPS, BM_GETCHECK);
        CfgDetectXMILoops       = (t_int32) SendDlgItemMessage(IDC_XMI_LOOPS, BM_GETCHECK);
        CfgDetectFF7Loops       = (t_int32) SendDlgItemMessage(IDC_FF7_LOOPS, BM_GETCHECK);

    }

    // FluidSynth
    {
        int SelectedIndex = (int) SendDlgItemMessage(IDC_FLUIDSYNTH_INTERPOLATION, CB_GETCURSEL);

        if (SelectedIndex != -1)
            CfgFluidSynthInterpolationMode = _InterpolationMethods[SelectedIndex].Id;
    }

    // BASS MIDI resampling mode and cache status
    {
        wchar_t Text[32];

        GetDlgItemTextW(IDC_BASSMIDI_VOLUME, Text, _countof(Text));

        CfgBASSMIDIVolume = std::clamp(::_wtof(Text), 0., 2.);

        ::uSetDlgItemText(m_hWnd, IDC_BASSMIDI_VOLUME, pfc::format_float(CfgBASSMIDIVolume, 4, 2));
    }
    {
        CfgBASSMIDIResamplingMode = (t_int32) SendDlgItemMessage(IDC_RESAMPLING_MODE, CB_GETCURSEL);
    }

    // Munt
    {
        CfgMuntGMSet = (t_int32) SendDlgItemMessage(IDC_MUNT_GM_SET, CB_GETCURSEL);
    }

    // Nuked OPL3
    {
        size_t SelectedIndex = (size_t) SendDlgItemMessage(IDC_NUKE_PRESET, CB_GETCURSEL);

        uint32_t Synth;
        uint32_t Bank;

        NukePlayer::GetPreset(SelectedIndex, Synth, Bank);

        CfgNukeSynthesizer = (t_int32) Synth;
        CfgNukeBank        = (t_int32) Bank;
        CfgNukePanning     = (t_int32) SendDlgItemMessage(IDC_NUKE_PANNING, BM_GETCHECK);
    }

    OnChanged();
}

/// <summary>
/// Resets the dialog state to default.
/// </summary>
void PreferencesRootPage::reset()
{
    _SelectedPlayer = installed_player_t(PlayerTypes::Default, "", "", ~0u, ~0u);

    // Player Type
    {
        int SelectedIndex = -1;

        int i = 0;

        for (const auto & Player : _InstalledPlayers)
        {
            if (Player == _SelectedPlayer)
            {
                SelectedIndex = i;
                break;
            }

            ++i;
        }

        SendDlgItemMessage(IDC_PLAYER_TYPE, CB_SETCURSEL, (WPARAM) SelectedIndex);
        GetDlgItem(IDC_CONFIGURE).EnableWindow(FALSE);
    }

    // Configure
    {
    }

    // Sample Rate
    {
        if ((_SelectedPlayer.Type == PlayerTypes::EmuDeMIDI) && _IsRunning)
            GetDlgItem(IDC_SAMPLERATE).EnableWindow(FALSE);

        SetDlgItemInt(IDC_SAMPLERATE, DefaultSampleRate, FALSE);
    }

    // Looping
    {
        SendDlgItemMessage(IDC_LOOP_PLAYBACK, CB_SETCURSEL, DefaultPlaybackLoopType);
        SendDlgItemMessage(IDC_LOOP_OTHER,    CB_SETCURSEL, DefaultOtherLoopType);
    }

    // MIDI Flavor and Effects
    {
        const BOOL SupportsFlavors = _SelectedPlayer.SupportsMIDIFlavor();

        GetDlgItem(IDC_MIDI_FLAVOR_TEXT).EnableWindow(SupportsFlavors);
        GetDlgItem(IDC_MIDI_FLAVOR).EnableWindow(SupportsFlavors);
        GetDlgItem(IDC_MIDI_EFFECTS).EnableWindow(SupportsFlavors);

        SendDlgItemMessage(IDC_MIDI_FLAVOR,           CB_SETCURSEL, DefaultMIDIFlavor);
        SendDlgItemMessage(IDC_MIDI_EFFECTS,          BM_SETCHECK,  DefaultUseMIDIEffects ? 0 : 1);
        SendDlgItemMessage(IDC_MIDI_USE_SUPER_MUNT,   BM_SETCHECK,  DefaultUseSuperMuntWithMT32);
        SendDlgItemMessage(IDC_MIDI_USE_VSTI_WITH_XG, BM_SETCHECK,  DefaultUseVSTiWithXG);
    }

    // Miscellaneous
    {
        SendDlgItemMessage(IDC_EMIDI_EXCLUSION,    BM_SETCHECK, DefaultEmuDeMIDIExclusion);
        SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_SETCHECK, DefaultFilterInstruments);
        SendDlgItemMessage(IDC_FILTER_BANKS,       BM_SETCHECK, DefaultFilterBanks);

        SendDlgItemMessage(IDC_TOUHOU_LOOPS,       BM_SETCHECK, DefaultDetectTouhouLoops);
        SendDlgItemMessage(IDC_RPGMAKER_LOOPS,     BM_SETCHECK, DefaultDetectRPGMakerLoops);
        SendDlgItemMessage(IDC_LEAPFROG_LOOPS,     BM_SETCHECK, DefaultDetectLeapFrogLoops);
        SendDlgItemMessage(IDC_XMI_LOOPS,          BM_SETCHECK, DefaultDetectXMILoops);
        SendDlgItemMessage(IDC_FF7_LOOPS,          BM_SETCHECK, DefaultDetectFF7Loops);
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

        const BOOL IsFluidSynth = (_SelectedPlayer.Type == PlayerTypes::FluidSynth);

        GetDlgItem(IDC_FLUIDSYNTH_INTERPOLATION_TEXT).EnableWindow(IsFluidSynth);
        GetDlgItem(IDC_FLUIDSYNTH_INTERPOLATION).EnableWindow(IsFluidSynth);
    }

    // BASS MIDI resampling mode and cache status
    {
        ::uSetDlgItemText(m_hWnd, IDC_BASSMIDI_VOLUME, pfc::format_float(DefaultBASSMIDIVolume, 4, 2));

        SendDlgItemMessage(IDC_RESAMPLING_MODE, CB_SETCURSEL, DefaultBASSMIDIResamplingMode);

        const BOOL IsBASSMIDI = (_SelectedPlayer.Type == PlayerTypes::BASSMIDI);

        const int ControlIds[] =
        {
            IDC_BASSMIDI_VOLUME_LBL, IDC_BASSMIDI_VOLUME,
            IDC_RESAMPLING_LBL, IDC_RESAMPLING_MODE,
            IDC_CACHED_LBL, IDC_CACHED
        };

        for (const int & ControlId : ControlIds)
            GetDlgItem(ControlId).EnableWindow(IsBASSMIDI);
    }

    // Munt
    {
        const BOOL IsSuperMunt = (_SelectedPlayer.Type == PlayerTypes::SuperMunt);

        GetDlgItem(IDC_MUNT_GM_TEXT).EnableWindow(IsSuperMunt);
        GetDlgItem(IDC_MUNT_GM_SET).EnableWindow(IsSuperMunt);

        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(IsSuperMunt ? SW_SHOW : SW_HIDE);

        SendDlgItemMessage(IDC_MUNT_GM_SET, CB_SETCURSEL, (WPARAM) DefaultGMSet);
    }

    // Nuked OPL3
    {
        const BOOL IsNuke = (_SelectedPlayer.Type == PlayerTypes::NukedOPL3);

        const int ControlIds[] =
        {
            IDC_NUKE_PRESET_TEXT, IDC_NUKE_PRESET,
            IDC_NUKE_PANNING
        };

        for (const int & ControlId : ControlIds)
            GetDlgItem(ControlId).EnableWindow(IsNuke);

        SendDlgItemMessage(IDC_NUKE_PRESET, CB_SETCURSEL, (WPARAM)NukePlayer::GetPresetIndex(DefaultNukeSynth, DefaultNukeBank));
        SendDlgItemMessage(IDC_NUKE_PANNING, BM_SETCHECK, DefaultNukePanning);
    }

    VSTi::Config.resize(0);

    UpdateDialog();

    OnChanged();
}

#pragma endregion

#pragma region CDialogImpl

/// <summary>
/// Initializes the dialog.
/// </summary>
BOOL PreferencesRootPage::OnInitDialog(CWindow, LPARAM)
{
    #pragma region Player Type

    _HasFluidSynth = FluidSynth::Exists();
    _HasSecretSauce = SecretSauce::Exists();

    #pragma region Known Players

    // Add the available known players to the installed player list. FluidSynth and Secret Sauce are optional.
    {
        for (const auto & Player : _KnownPlayers)
        {
            if (Player.IsPresent(this))
            {
                installed_player_t ip(Player.Type, Player.Name, "", ~0u, ~0u);

                _InstalledPlayers.push_back(ip);
            }
        }
    }

    #pragma endregion

    #pragma region VSTi Players

    // Add the VSTi plug-ins to the installed player list.
    {
        VSTi::Enumerate();

        size_t VSTiCount = VSTi::PlugIns.size();

        if (VSTiCount > 0)
        {
            console::print(STR_COMPONENT_BASENAME " found ", VSTiCount, " VSTi plug-insinstruments.");

            size_t i = 0;

            for (const auto & PlugIn : VSTi::PlugIns)
            {
                installed_player_t ip(PlayerTypes::VSTi, PlugIn.Name.c_str(), PlugIn.PathName, ~0u, i);

                _InstalledPlayers.push_back(ip);

                ++i;
            }
        }
        else
            console::print(STR_COMPONENT_BASENAME " found no compatible VSTi plug-ins.");
    }

    #pragma endregion

    #pragma region CLAP Players

    // Add the CLAP plug-ins to the installed player list.
    {
        foo_midi::clap_host_t::PlugIns.clear();

        console::print(STR_COMPONENT_BASENAME " is enumerating CLAP plug-ins...");

        fs::path BaseDirectory(CfgCLAPPlugInDirectoryPath.get().c_str());

        foo_midi::clap_host_t::GetPlugIns(BaseDirectory);

        size_t CLAPCount = foo_midi::clap_host_t::PlugIns.size();

        if (CLAPCount > 0)
        {
            console::print(STR_COMPONENT_BASENAME " found ", CLAPCount, " CLAP plug-ins.");

            size_t i = 0;

            for (const auto & PlugIn : foo_midi::clap_host_t::PlugIns)
            {
                installed_player_t ip(PlayerTypes::CLAP, PlugIn.Name.c_str(), PlugIn.PathName, PlugIn.Index, i);

                _InstalledPlayers.push_back(ip);

                ++i;
            }
        }
        else
            console::print(STR_COMPONENT_BASENAME " found no compatible CLAP plug-ins.");
    }

    #pragma endregion

//  std::sort(_InstalledPlayers.begin(), _InstalledPlayers.end(), [](InstalledPlayer a, InstalledPlayer b) { return a.Name < b.Name; });

    // Determine the selected player.
    _SelectedPlayer = installed_player_t((PlayerTypes) CfgPlayerType.get(), "", CfgPlugInFilePath.get().c_str(), (size_t) CfgPlugInIndex, ~0u);

    #pragma endregion

    #pragma region Player Type
    {
        auto w = (CComboBox) GetDlgItem(IDC_PLAYER_TYPE);

        int SelectedIndex = -1;
        int i = 0;

        for (const auto & Player : _InstalledPlayers)
        {
            w.AddString(pfc::wideFromUTF8(Player.Name));

            if (Player == _SelectedPlayer)
                SelectedIndex = i;

            ++i;
        }

        if (SelectedIndex == -1)
        {
            _SelectedPlayer = installed_player_t(PlayerTypes::Default, "", "", ~0u, ~0u);

            i = 0;

            for (const auto & Player : _InstalledPlayers)
            {
                if (Player == _SelectedPlayer)
                {
                    SelectedIndex = i;
                    break;
                }

                ++i;
            }
        }

        w.SetCurSel(SelectedIndex);
    }
    #pragma endregion

    #pragma region Configure
    {
        auto w = (CComboBox) GetDlgItem(IDC_CONFIGURE);

        if (_SelectedPlayer.Type != PlayerTypes::VSTi)
        {
            w.EnableWindow(FALSE);
        }
        else
        {
            if (_SelectedPlayer.PlugInIndex != ~0u)
            {
                const VSTi::plugin_t & Plugin = VSTi::PlugIns[_SelectedPlayer.PlugInIndex];

                w.EnableWindow(Plugin.HasEditor);

                VSTi::Config = CfgVSTiConfig[Plugin.Id];
            }
        }
    }
    #pragma endregion

    #pragma region Sample Rate
    {
        for (size_t i = _countof(_SampleRates); i--;)
        {
            if (_SampleRates[i] != CfgSampleRate)
                CfgSampleRateHistory.add_item(pfc::format_int(_SampleRates[i]));
        }

        CfgSampleRateHistory.add_item(pfc::format_int(CfgSampleRate));

        {
            auto w = (CComboBox) GetDlgItem(IDC_SAMPLERATE);

            CfgSampleRateHistory.setup_dropdown(w);

             w.SetCurSel(0);

            if ((_SelectedPlayer.Type == PlayerTypes::EmuDeMIDI) && _IsRunning)
                w.EnableWindow(FALSE);
        }
    }
    #pragma endregion

    #pragma region Looping
    {
        static const wchar_t * LoopTypeDescriptions[] =
        {
            L"Never loop",
            L"Never loop. Use decay time",
            L"Loop and fade when detected",
            L"Loop and fade always",
            L"Play indefinitely when detected",
            L"Play indefinitely"
        };

        {
            auto w1 = (CComboBox) GetDlgItem(IDC_LOOP_PLAYBACK);
            auto w2 = (CComboBox) GetDlgItem(IDC_LOOP_OTHER);

            for (const auto & Description : LoopTypeDescriptions)
            {
                w1.AddString(Description);
                w2.AddString(Description);
            }
 
             w1.SetCurSel((int) CfgLoopTypePlayback);
             w2.SetCurSel((int) CfgLoopTypeOther);

            ::uSetDlgItemText(m_hWnd, IDC_DECAY_TIME, pfc::format_int(CfgDecayTime));
        }
    }
    #pragma endregion

    #pragma region MIDI Flavor
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

        ::SendMessage(w, CB_SETCURSEL, (WPARAM) CfgMIDIFlavor, 0);

        SendDlgItemMessage(IDC_MIDI_EFFECTS,          BM_SETCHECK, (WPARAM) (CfgUseMIDIEffects ? 0 : 1));
        SendDlgItemMessage(IDC_MIDI_USE_SUPER_MUNT,   BM_SETCHECK, (WPARAM) CfgUseSuperMuntWithMT32);
        SendDlgItemMessage(IDC_MIDI_USE_VSTI_WITH_XG, BM_SETCHECK, (WPARAM) CfgUseVSTiWithXG);

        const bool Enabled = _SelectedPlayer.SupportsMIDIFlavor();

        GetDlgItem(IDC_MIDI_FLAVOR_TEXT).EnableWindow(Enabled);
        GetDlgItem(IDC_MIDI_FLAVOR)     .EnableWindow(Enabled);
        GetDlgItem(IDC_MIDI_EFFECTS)    .EnableWindow(Enabled);
    }
    #pragma endregion

    #pragma region Miscellaneous
    {
        SendDlgItemMessage(IDC_EMIDI_EXCLUSION,    BM_SETCHECK, (WPARAM) CfgEmuDeMIDIExclusion);

        SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_SETCHECK, (WPARAM) CfgFilterInstruments);
        SendDlgItemMessage(IDC_FILTER_BANKS,       BM_SETCHECK, (WPARAM) CfgFilterBanks);

        SendDlgItemMessage(IDC_TOUHOU_LOOPS,       BM_SETCHECK, (WPARAM) CfgDetectTouhouLoops);
        SendDlgItemMessage(IDC_RPGMAKER_LOOPS,     BM_SETCHECK, (WPARAM) CfgDetectRPGMakerLoops);
        SendDlgItemMessage(IDC_LEAPFROG_LOOPS,     BM_SETCHECK, (WPARAM) CfgDetectLeapFrogLoops);
        SendDlgItemMessage(IDC_XMI_LOOPS,          BM_SETCHECK, (WPARAM) CfgDetectXMILoops);
        SendDlgItemMessage(IDC_FF7_LOOPS,          BM_SETCHECK, (WPARAM) CfgDetectFF7Loops);
    }
    #pragma endregion

    #pragma region FluidSynth
    {
        auto w = (CComboBox) GetDlgItem(IDC_FLUIDSYNTH_INTERPOLATION);

        int SelectedIndex = -1;
        int i = 0;

        for (const auto & InterpolationMethod : _InterpolationMethods)
        {
            w.AddString(InterpolationMethod.Name);

            if (InterpolationMethod.Id == CfgFluidSynthInterpolationMode)
                SelectedIndex = (int) i;

            ++i;
        }

        w.SetCurSel(SelectedIndex);

        const BOOL Enable = (_SelectedPlayer.Type == PlayerTypes::FluidSynth);

        GetDlgItem(IDC_FLUIDSYNTH_INTERPOLATION_TEXT).EnableWindow(Enable);
        GetDlgItem(IDC_FLUIDSYNTH_INTERPOLATION).EnableWindow(Enable);
    }
    #pragma endregion

    #pragma region BASS MIDI
    {
        ::uSetDlgItemText(m_hWnd, IDC_BASSMIDI_VOLUME, pfc::format_float(CfgBASSMIDIVolume, 4, 2));
    }

    {
        auto w = (CComboBox) GetDlgItem(IDC_RESAMPLING_MODE);

        w.AddString(L"Linear interpolation");
        w.AddString(L"8pt Sinc interpolation");
        w.AddString(L"16pt Sinc interpolation");

        w.SetCurSel((int) CfgBASSMIDIResamplingMode);

        const BOOL Enable = (_SelectedPlayer.Type == PlayerTypes::BASSMIDI);

        const int ControlIds[] =
        {
            IDC_BASSMIDI_VOLUME_LBL, IDC_BASSMIDI_VOLUME,
            IDC_RESAMPLING_LBL, IDC_RESAMPLING_MODE,
            IDC_CACHED_LBL, IDC_CACHED
        };

        for (const auto & ControlId : ControlIds)
            GetDlgItem(ControlId).EnableWindow(Enable);
    }
    #pragma endregion

    #pragma region Munt
    {
        auto w = (CComboBox) GetDlgItem(IDC_MUNT_GM_SET);

        for (const auto & Set : _MuntSets)
            ::uSendMessageText(w, CB_ADDSTRING, 0, Set);

        w.SetCurSel((int) CfgMuntGMSet);

        const BOOL IsSuperMunt = (_SelectedPlayer.Type == PlayerTypes::SuperMunt);

        GetDlgItem(IDC_MUNT_GM_TEXT).EnableWindow(IsSuperMunt);
        GetDlgItem(IDC_MUNT_GM_SET).EnableWindow(IsSuperMunt);

        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(IsSuperMunt ? SW_SHOW : SW_HIDE);
    }
    #pragma endregion

    #pragma region Nuked OPL3
    {
        auto w = GetDlgItem(IDC_NUKE_PRESET);

        size_t PresetNumber = 0;

        NukePlayer::EnumeratePresets([w, PresetNumber] (const pfc::string name, unsigned int synth, unsigned int bank) mutable noexcept
        {
            ::uSendMessageText(w, CB_ADDSTRING, 0, name.c_str());

            if ((synth == (unsigned int) CfgNukeSynthesizer) && (bank == (unsigned int) CfgNukeBank))
                ::SendMessage(w, CB_SETCURSEL, PresetNumber, 0);

            PresetNumber++;
        });

        SendDlgItemMessage(IDC_NUKE_PANNING, BM_SETCHECK, (WPARAM) CfgNukePanning);

        if (_SelectedPlayer.Type != PlayerTypes::NukedOPL3)
        {
            GetDlgItem(IDC_NUKE_PRESET_TEXT).EnableWindow(FALSE);
            GetDlgItem(IDC_NUKE_PRESET).EnableWindow(FALSE);
            GetDlgItem(IDC_NUKE_PANNING).EnableWindow(FALSE);
        }
    }
    #pragma endregion

    SetTimer(ID_REFRESH, 20);

    _IsBusy = false;

    _DarkModeHooks.AddDialogWithControls(*this);

    UpdateDialog();

    return FALSE;
}

/// <summary>
/// Handles a change in an edit box.
/// </summary>
void PreferencesRootPage::OnEditChange(UINT, int, CWindow)
{
    OnChanged();
}

/// <summary>
/// Handles a selection change in a combo box.
/// </summary>
void PreferencesRootPage::OnSelectionChange(UINT, int, CWindow)
{
    OnChanged();
}

/// <summary>
/// Handles a click on a button
/// </summary>
void PreferencesRootPage::OnButtonClick(UINT, int, CWindow)
{
    OnChanged();
}

/// <summary>
/// Handles a click on the Configure button.
/// </summary>
void PreferencesRootPage::OnButtonConfig(UINT, int, CWindow)
{
    const int SelectedIndex = (int) GetDlgItem(IDC_PLAYER_TYPE).SendMessage(CB_GETCURSEL, 0, 0);

    if (SelectedIndex == -1)
        return;

    _IsBusy = true;
    OnChanged();

    if (_SelectedPlayer.Type == PlayerTypes::VSTi)
    {
        VSTiPlayer Player;

        if (Player.LoadVST(_SelectedPlayer.PathName.string().c_str()))
        {
            if (VSTi::Config.size() != 0)
                Player.SetChunk(VSTi::Config.data(), VSTi::Config.size());

            Player.DisplayEditorModal();

            Player.GetChunk(VSTi::Config);
        }
    }
    else
    if (_SelectedPlayer.Type == PlayerTypes::CLAP)
    {
        CLAPPlayer Player;

        /* Configure CLAP */
    }

    _IsBusy = false;
    OnChanged();
}

/// <summary>
/// Updates the dialog controls when the player type changes.
/// </summary>
void PreferencesRootPage::OnPlayerTypeChange(UINT, int, CWindow w)
{
    int SelectedIndex = (int) ::SendMessage(w, CB_GETCURSEL, 0, 0);

    // Player Type
    if (SelectedIndex != -1)
    {
        _SelectedPlayer = _InstalledPlayers[(size_t) SelectedIndex];

        if (_SelectedPlayer.Type == PlayerTypes::VSTi)
            VSTi::Config = CfgVSTiConfig[VSTi::PlugIns[_SelectedPlayer.PlugInIndex].Id];
    }

    // Configure
    {
        const BOOL Enable = (_SelectedPlayer.Type == PlayerTypes::VSTi) ? VSTi::PlugIns[_SelectedPlayer.PlugInIndex].HasEditor : FALSE;

        GetDlgItem(IDC_CONFIGURE).EnableWindow(Enable);
    }

    // Sample Rate
    {
        const BOOL Enable = (_SelectedPlayer.Type != PlayerTypes::EmuDeMIDI) || !_IsRunning;

        GetDlgItem(IDC_SAMPLERATE).EnableWindow(Enable);
    }

    // Looping
    {
    }

    // MIDI Flavor and Effects
    {
        const BOOL SupportsFlavors = _SelectedPlayer.SupportsMIDIFlavor();

        const int ControlIds[] =
        {
            IDC_MIDI_FLAVOR_TEXT, IDC_MIDI_FLAVOR,
            IDC_MIDI_EFFECTS,
        };

        for (const int & ControlId : ControlIds)
            GetDlgItem(ControlId).EnableWindow(SupportsFlavors);
    }

    // Miscellaneous
    {
    }

    // FluidSynth
    {
        const BOOL IsFluidSynth = (_SelectedPlayer.Type == PlayerTypes::FluidSynth);

        GetDlgItem(IDC_FLUIDSYNTH_INTERPOLATION_TEXT).EnableWindow(IsFluidSynth);
        GetDlgItem(IDC_FLUIDSYNTH_INTERPOLATION).EnableWindow(IsFluidSynth);
    }

    // BASS MIDI resampling mode and cache status
    {
        const BOOL IsBASSMIDI = (_SelectedPlayer.Type == PlayerTypes::BASSMIDI);

        const int ControlIds[] =
        {
            IDC_BASSMIDI_VOLUME_LBL, IDC_BASSMIDI_VOLUME,
            IDC_RESAMPLING_LBL, IDC_RESAMPLING_MODE,
            IDC_CACHED_LBL, IDC_CACHED
        };

        for (const int & ControlId : ControlIds)
            GetDlgItem(ControlId).EnableWindow(IsBASSMIDI);
    }

    // Munt
    {
        const BOOL IsSuperMunt = (_SelectedPlayer.Type == PlayerTypes::SuperMunt);

        GetDlgItem(IDC_MUNT_GM_TEXT).EnableWindow(IsSuperMunt);
        GetDlgItem(IDC_MUNT_GM_SET) .EnableWindow(IsSuperMunt);

        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(IsSuperMunt ? SW_SHOW : SW_HIDE);
    }

    // Nuked OPL3
    {
        const BOOL IsNukeOPL3 = (_SelectedPlayer.Type == PlayerTypes::NukedOPL3);

        const int ControlIds[] =
        {
            IDC_NUKE_PRESET_TEXT, IDC_NUKE_PRESET,
            IDC_NUKE_PANNING
        };

        for (const int & ControlId : ControlIds)
            GetDlgItem(ControlId).EnableWindow(IsNukeOPL3);
    }

    OnChanged();
}

/// <summary>
/// Update the status of the SoundFont cache.
/// </summary>
void PreferencesRootPage::OnTimer(UINT_PTR eventId)
{
    if (eventId != ID_REFRESH)
        return;
 
    GetDlgItem(IDC_SAMPLERATE).EnableWindow(CfgPlayerType || !_IsRunning);

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

/// <summary>
/// Returns true if the dialog state has changed.
/// </summary>
bool PreferencesRootPage::HasChanged()
{
    #pragma region Player Type
    {
        int SelectedIndex = (int) SendDlgItemMessage(IDC_PLAYER_TYPE, CB_GETCURSEL);

        if (SelectedIndex != -1)
        {
            _SelectedPlayer = _InstalledPlayers[(size_t) SelectedIndex];

            if (_SelectedPlayer.Type != (PlayerTypes) (int) CfgPlayerType)
                return true;

            if (_SelectedPlayer.Type == PlayerTypes::VSTi)
            {
                if (CfgPlugInFilePath.get() != _SelectedPlayer.PathName.string().c_str())
                    return true;

                t_uint32 Id = VSTi::PlugIns[_SelectedPlayer.PlugInIndex].Id;

                if (VSTi::Config.size() != CfgVSTiConfig[Id].size())
                    return true;

                if ((VSTi::Config.size() != 0) && (::memcmp(VSTi::Config.data(), CfgVSTiConfig[Id].data(), VSTi::Config.size()) != 0))
                    return true;
            }
            else
            if (_SelectedPlayer.Type == PlayerTypes::CLAP)
            {
                if (CfgPlugInFilePath.get() != _SelectedPlayer.PathName.string().c_str())
                    return true;

                if (CfgPlugInIndex != (int64_t) _SelectedPlayer.Index)
                    return true;
            }
        }
    }
    #pragma endregion

    #pragma region Sample Rate
    {
        if (GetDlgItemInt(IDC_SAMPLERATE, NULL, FALSE) != (UINT) CfgSampleRate)
            return true;
    }
    #pragma endregion

    #pragma region Looping
    {
        if (SendDlgItemMessage(IDC_LOOP_PLAYBACK, CB_GETCURSEL) != CfgLoopTypePlayback)
            return true;

        if (SendDlgItemMessage(IDC_LOOP_OTHER, CB_GETCURSEL) != CfgLoopTypeOther)
            return true;

        if (GetDlgItemInt(IDC_DECAY_TIME, NULL, FALSE) != (UINT) CfgDecayTime)
            return true;

        if (SendDlgItemMessage(IDC_TOUHOU_LOOPS, BM_GETCHECK) != CfgDetectTouhouLoops)
            return true;

        if (SendDlgItemMessage(IDC_RPGMAKER_LOOPS, BM_GETCHECK) != CfgDetectRPGMakerLoops)
            return true;

        if (SendDlgItemMessage(IDC_LEAPFROG_LOOPS, BM_GETCHECK) != CfgDetectLeapFrogLoops)
            return true;

        if (SendDlgItemMessage(IDC_XMI_LOOPS, BM_GETCHECK) != CfgDetectXMILoops)
            return true;

        if (SendDlgItemMessage(IDC_FF7_LOOPS, BM_GETCHECK) != CfgDetectFF7Loops)
            return true;
    }
    #pragma endregion

    #pragma region MIDI
    {
        if (SendDlgItemMessage(IDC_MIDI_FLAVOR, CB_GETCURSEL) != CfgMIDIFlavor)
            return true;

        if (SendDlgItemMessage(IDC_MIDI_EFFECTS, BM_GETCHECK) != (CfgUseMIDIEffects ? 0 : 1))
            return true;

        if (SendDlgItemMessage(IDC_MIDI_USE_SUPER_MUNT, BM_GETCHECK) != CfgUseSuperMuntWithMT32)
            return true;

        if (SendDlgItemMessage(IDC_MIDI_USE_VSTI_WITH_XG, BM_GETCHECK) != CfgUseVSTiWithXG)
            return true;
    }
    #pragma endregion

    #pragma region Miscellaneous
    {
        if (SendDlgItemMessage(IDC_EMIDI_EXCLUSION, BM_GETCHECK) != CfgEmuDeMIDIExclusion)
            return true;

        if (SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_GETCHECK) != CfgFilterInstruments)
            return true;

        if (SendDlgItemMessage(IDC_FILTER_BANKS, BM_GETCHECK) != CfgFilterBanks)
            return true;
    }
    #pragma endregion

    #pragma region FluidSynth
    {
        int SelectedIndex = (int) SendDlgItemMessage(IDC_FLUIDSYNTH_INTERPOLATION, CB_GETCURSEL);

        if ((SelectedIndex != -1) && (_InterpolationMethods[SelectedIndex ].Id != CfgFluidSynthInterpolationMode))
            return true;
    }
    #pragma endregion

    #pragma region BASS MIDI
    {
        wchar_t Text[32];

        GetDlgItemTextW(IDC_BASSMIDI_VOLUME, Text, _countof(Text));

        if (std::abs(::_wtof(Text) - CfgBASSMIDIVolume) > 0.001)
            return true;

        if (SendDlgItemMessage(IDC_RESAMPLING_MODE, CB_GETCURSEL) != CfgBASSMIDIResamplingMode)
            return true;
    }
    #pragma endregion

    #pragma region Munt
    {
        if (SendDlgItemMessage(IDC_MUNT_GM_SET, CB_GETCURSEL) != CfgMuntGMSet)
            return true;
    }
    #pragma endregion

    #pragma region Nuked OPL3
    {
        if (SendDlgItemMessage(IDC_NUKE_PANNING, BM_GETCHECK) != CfgNukePanning)
            return true;

        {
            size_t PresetNumber = (size_t) SendDlgItemMessage(IDC_NUKE_PRESET, CB_GETCURSEL);

            uint32_t Synth;
            uint32_t Bank;

            NukePlayer::GetPreset(PresetNumber, Synth, Bank);

            if (!(Synth == (uint32_t) CfgNukeSynthesizer && Bank == (uint32_t) CfgNukeBank))
                return true;
        }
    }
    #pragma endregion

    return false;
}

/// <summary>
/// Notifies the parent that the dialog state has changed.
/// </summary>
void PreferencesRootPage::OnChanged()
{
    _Callback->on_state_changed();
}

/// <summary>
/// Updates the appearance of the dialog according to the values of the settings.
/// </summary>
void PreferencesRootPage::UpdateDialog() const noexcept
{
    #pragma region MIDI Flavors

    pfc::string FilePath; AdvCfgVSTiXGPlugin.get(FilePath);

    GetDlgItem(IDC_MIDI_USE_VSTI_WITH_XG).EnableWindow(!FilePath.isEmpty());

    #pragma endregion
}

#pragma endregion

class PreferencesRootPageImpl : public preferences_page_impl<PreferencesRootPage>
{
public:
    PreferencesRootPageImpl() noexcept { };

    PreferencesRootPageImpl(const PreferencesRootPageImpl &) = delete;
    PreferencesRootPageImpl(const PreferencesRootPageImpl &&) = delete;
    PreferencesRootPageImpl & operator=(const PreferencesRootPageImpl &) = delete;
    PreferencesRootPageImpl & operator=(PreferencesRootPageImpl &&) = delete;

    virtual ~PreferencesRootPageImpl() noexcept { };

    const char * get_name() noexcept
    {
        return IDD_PREFERENCES_ROOT_NAME;
    }

    GUID get_guid() noexcept
    {
        return PreferencesPageGUID;
    }

    GUID get_parent_guid() noexcept
    {
        return guid_input;
    }
};

static preferences_page_factory_t<PreferencesRootPageImpl> PreferencesPageFactory;
