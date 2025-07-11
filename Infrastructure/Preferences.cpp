
/** $VER: Preferences.cpp (2025.07.11) P. Stuer **/

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

#include "Resource.h"

#include "Configuration.h"
#include "Preset.h"
#include "Encoding.h"

#include "BMPlayer.h"
#include "FSPlayer.h"
#include "CLAPPlayer.h"
#include "VSTiPlayer.h"

#include "VSTiHost.h"
#include "SecretSauce.h"
#include "CLAPHost.h"

#pragma hdrstop

//extern volatile int _IsRunning;

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

        // Output
        COMMAND_HANDLER_EX(IDC_PLAYER_TYPE, CBN_SELCHANGE, OnPlayerTypeChange)

        COMMAND_HANDLER_EX(IDC_CONFIGURE, BN_CLICKED, OnButtonConfig)

        DROPDOWN_HISTORY_HANDLER(IDC_SAMPLERATE, CfgSampleRateHistory)
        COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_EDITCHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_SELCHANGE, OnSelectionChange)

        // Looping
        COMMAND_HANDLER_EX(IDC_LOOP_PLAYBACK, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_LOOP_OTHER, CBN_SELCHANGE, OnSelectionChange)

        COMMAND_HANDLER_EX(IDC_DECAY_TIME, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_LOOP_COUNT, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_FADE_OUT_TIME, EN_CHANGE, OnEditChange)

        COMMAND_HANDLER_EX(IDC_RPGMAKER_LOOPS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_LEAPFROG_LOOPS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_XMI_LOOPS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_TOUHOU_LOOPS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_FF7_LOOPS, BN_CLICKED, OnButtonClick)

        // MIDI
        COMMAND_HANDLER_EX(IDC_MIDI_FLAVOR,                CBN_SELCHANGE, OnSelectionChange)

        COMMAND_HANDLER_EX(IDC_MIDI_EFFECTS,               BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_MIDI_USE_MT32EMU_WITH_MT32, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_MIDI_USE_VSTI_WITH_XG,      BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_MIDI_DETECT_EXTRA_DRUM,     BN_CLICKED, OnButtonClick)

        COMMAND_HANDLER_EX(IDC_EMIDI_EXCLUSION, BN_CLICKED, OnButtonClick)

        COMMAND_HANDLER_EX(IDC_FILTER_INSTRUMENTS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_FILTER_BANKS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_SKIP_TO_FIRST_NOTE, BN_CLICKED, OnButtonClick)
        #pragma endregion
    END_MSG_MAP()

    enum
    {
        IDD = IDD_PREFERENCES_ROOT
    };

private:
    BOOL OnInitDialog(CWindow, LPARAM);

    void OnPlayerTypeChange(UINT, int, CWindow w);
    void OnEditChange(UINT, int, CWindow);
    void OnSelectionChange(UINT, int, CWindow);
    void OnButtonClick(UINT, int, CWindow);
    void OnButtonConfig(UINT, int, CWindow);

    bool HasChanged();
    void OnChanged();

    void UpdateDialog() const noexcept;
    void UpdateConfigureButton() noexcept;

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
        std::string Name;

        // Unique key
        PlayerTypes Type;
        fs::path FilePath;  // Path name of the VSTi or CLAP plug-in file
        size_t Index;       // Index with a CLAP plug-in

        size_t PlugInIndex; // Index in the VSTi or CLAP plug-in list

        installed_player_t() : Type(PlayerTypes::Unknown), Index((size_t) -1), PlugInIndex((size_t) -1) { }
        installed_player_t(const std::string & name, PlayerTypes type, fs::path filePath, size_t index, size_t plugInIndex) : Name(name), Type(type), FilePath(filePath), Index(index), PlugInIndex(plugInIndex) { }

        bool operator ==(const installed_player_t & other) const noexcept
        {
            if (Type != other.Type)
                return false;

            if (((Type == PlayerTypes::VSTi) || (Type == PlayerTypes::CLAP)))
            {
                if (FilePath != other.FilePath)
                    return false;

                if (Type == PlayerTypes::CLAP)
                    return Index == other.Index;
            }

            return true;
        }

        bool SupportsMIDIFlavor() const noexcept
        {
            return ((Type == PlayerTypes::VSTi) || (Type == PlayerTypes::CLAP) || (Type == PlayerTypes::FluidSynth) || (Type == PlayerTypes::BASSMIDI) || (Type == PlayerTypes::SecretSauce));
        }
    };

    std::vector<installed_player_t> _InstalledPlayers;
    installed_player_t _SelectedPlayer;

    static const int _SampleRates[];

    #pragma endregion

    // Secret Sauce
    bool _HasSecretSauce;

    // VSTi
    VSTi::Host _VSTiHost;
    std::vector<VSTi::PlugIn> _VSTiPlugIns;

    // CLAP
    CLAP::Host _CLAPHost;
    std::vector<CLAP::PlugIn> _CLAPPlugIns;

    // FluidSynth
    bool _HasFluidSynth;

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
    { "LibMT32Emu",     PlayerTypes::MT32Emu,       PlayerIsAlwaysPresent },
    { "LibADLMIDI",     PlayerTypes::ADL,           PlayerIsAlwaysPresent },
    { "LibOPNMIDI",     PlayerTypes::OPN,           PlayerIsAlwaysPresent },
    { "OPL MIDI",       PlayerTypes::OPL,           PlayerIsNeverPresent },
    { "Nuked OPL3",     PlayerTypes::NukedOPL3,     PlayerIsAlwaysPresent },
    { "Nuked SC-55",    PlayerTypes::NukedSC55,     PlayerIsNeverPresent },
    { "Secret Sauce",   PlayerTypes::SecretSauce,   IsSecretSaucePresent },
    { "MCI",            PlayerTypes::MCI,           PlayerIsNeverPresent },
    { "FMMIDI",         PlayerTypes::FMMIDI,        PlayerIsAlwaysPresent },
};

const int PreferencesRootPage::_SampleRates[] = { 8'000, 11'025, 16'000, 22'050, 24'000, 32'000, 44'100, 48'000, 49'716, 64'000, 88'200, 96'000 };

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

            if (_SelectedPlayer.Type == PlayerTypes::CLAP)
            {
                const auto & PlugIn = _CLAPPlugIns[_SelectedPlayer.PlugInIndex];

                CfgPlugInFilePath = (const char *) PlugIn.FilePath.u8string().c_str();
                CfgCLAPIndex      = (int64_t) PlugIn.Index;
                CfgPlugInName     = PlugIn.Name.c_str();

                _CLAPHost.Load(CfgPlugInFilePath.get().c_str(), (uint32_t) CfgCLAPIndex);
            }
            else
            {
                _CLAPHost.UnLoad();

                if (_SelectedPlayer.Type == PlayerTypes::VSTi)
                {
                    const auto & PlugIn = _VSTiPlugIns[_SelectedPlayer.PlugInIndex];

                    CfgPlugInFilePath        = (const char *) PlugIn.FilePath.u8string().c_str();
                    CfgCLAPIndex             = (int64_t) -1;
                    CfgPlugInName            = PlugIn.Name.c_str();
                    CfgVSTiConfig[PlugIn.Id] = _VSTiHost.Config;
                }
                else
                {
                    CfgPlugInFilePath = "";
                    CfgCLAPIndex      = (int64_t) -1;
                    CfgPlugInName     = "";
                }
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
        CfgLoopCount            = (t_int32) GetDlgItemInt(IDC_LOOP_COUNT, NULL, FALSE);
        CfgFadeOutTime          = (t_int32) GetDlgItemInt(IDC_FADE_OUT_TIME, NULL, FALSE);
    }

    // MIDI
    {
        CfgMIDIFlavor         = (t_int32) SendDlgItemMessage(IDC_MIDI_FLAVOR, CB_GETCURSEL);

        CfgUseMIDIEffects     = (t_int32) SendDlgItemMessage(IDC_MIDI_EFFECTS, BM_GETCHECK) ? 0 : 1;
        CfgUseMT32EmuWithMT32 = (t_int32) SendDlgItemMessage(IDC_MIDI_USE_MT32EMU_WITH_MT32, BM_GETCHECK);
        CfgUseVSTiWithXG      = (t_int32) SendDlgItemMessage(IDC_MIDI_USE_VSTI_WITH_XG, BM_GETCHECK);
        CfgDetectExtraDrum    = (bool)    SendDlgItemMessage(IDC_MIDI_DETECT_EXTRA_DRUM, BM_GETCHECK);

        CfgExcludeEMIDITrackDesignation = (t_int32) SendDlgItemMessage(IDC_EMIDI_EXCLUSION, BM_GETCHECK);

        CfgFilterInstruments    = (t_int32) SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_GETCHECK);
        CfgFilterBanks          = (t_int32) SendDlgItemMessage(IDC_FILTER_BANKS, BM_GETCHECK);
        CfgSkipToFirstNote      = (bool) SendDlgItemMessage(IDC_SKIP_TO_FIRST_NOTE, BM_GETCHECK);

        CfgDetectTouhouLoops    = (t_int32) SendDlgItemMessage(IDC_TOUHOU_LOOPS, BM_GETCHECK);
        CfgDetectRPGMakerLoops  = (t_int32) SendDlgItemMessage(IDC_RPGMAKER_LOOPS, BM_GETCHECK);
        CfgDetectLeapFrogLoops  = (t_int32) SendDlgItemMessage(IDC_LEAPFROG_LOOPS, BM_GETCHECK);
        CfgDetectXMILoops       = (t_int32) SendDlgItemMessage(IDC_XMI_LOOPS, BM_GETCHECK);
        CfgDetectFF7Loops       = (t_int32) SendDlgItemMessage(IDC_FF7_LOOPS, BM_GETCHECK);

        pfc::string FilePath = CfgVSTiXGPlugInFilePath;

        GetDlgItem(IDC_MIDI_USE_VSTI_WITH_XG).EnableWindow(!FilePath.isEmpty());
    }

    OnChanged();
}

/// <summary>
/// Resets the dialog state to default.
/// </summary>
void PreferencesRootPage::reset()
{
    _SelectedPlayer = installed_player_t("", PlayerTypes::Default, "", (size_t) -1, (size_t) -1);

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

        UpdateConfigureButton();
    }

    // Configure
    {
    }

    // Sample Rate
    {
/*
        if ((_SelectedPlayer.Type == PlayerTypes::EmuDeMIDI) && _IsRunning)
            GetDlgItem(IDC_SAMPLERATE).EnableWindow(FALSE);
*/
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

        SendDlgItemMessage(IDC_MIDI_FLAVOR,                 CB_SETCURSEL, DefaultMIDIFlavor);

        SendDlgItemMessage(IDC_TOUHOU_LOOPS,                BM_SETCHECK, DefaultDetectTouhouLoops);
        SendDlgItemMessage(IDC_RPGMAKER_LOOPS,              BM_SETCHECK, DefaultDetectRPGMakerLoops);
        SendDlgItemMessage(IDC_LEAPFROG_LOOPS,              BM_SETCHECK, DefaultDetectLeapFrogLoops);
        SendDlgItemMessage(IDC_XMI_LOOPS,                   BM_SETCHECK, DefaultDetectXMILoops);
        SendDlgItemMessage(IDC_FF7_LOOPS,                   BM_SETCHECK, DefaultDetectFF7Loops);

        SendDlgItemMessage(IDC_MIDI_EFFECTS,                BM_SETCHECK, DefaultUseMIDIEffects ? 0 : 1);
        SendDlgItemMessage(IDC_MIDI_USE_MT32EMU_WITH_MT32,  BM_SETCHECK, DefaultUseMT32EmuWithMT32);
        SendDlgItemMessage(IDC_MIDI_USE_VSTI_WITH_XG,       BM_SETCHECK, DefaultUseVSTiWithXG);
        SendDlgItemMessage(IDC_MIDI_DETECT_EXTRA_DRUM,      BM_SETCHECK, DefaultDetectExtraDrum);
        SendDlgItemMessage(IDC_EMIDI_EXCLUSION,             BM_SETCHECK, DefaultEmuDeMIDIExclusion);
        SendDlgItemMessage(IDC_FILTER_INSTRUMENTS,          BM_SETCHECK, DefaultFilterInstruments);
        SendDlgItemMessage(IDC_FILTER_BANKS,                BM_SETCHECK, DefaultFilterBanks);
        SendDlgItemMessage(IDC_SKIP_TO_FIRST_NOTE,          BM_SETCHECK, DefaultSkipToFirstNote);
    }

    _VSTiHost.Config.resize(0);

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
    _HasFluidSynth = FluidSynth::API::Exists();
    _HasSecretSauce = SecretSauce::Exists();

    #pragma region Known Players

    // Add the available known players to the installed player list. FluidSynth and Secret Sauce are optional.
    {
        for (const auto & Player : _KnownPlayers)
        {
            if (Player.IsPresent(this))
            {
                installed_player_t ip(Player.Name, Player.Type, "", (size_t) -1, (size_t) -1);

                _InstalledPlayers.push_back(ip);
            }
        }
    }

    #pragma endregion

    _SelectedPlayer = installed_player_t("", (PlayerTypes) CfgPlayerType.get(), CfgPlugInFilePath.get().c_str(), (size_t) CfgCLAPIndex, (size_t) -1);

    #pragma region VSTi Players

    // Add the VSTi plug-ins to the installed player list.
    {
        console::print(STR_COMPONENT_BASENAME " is enumerating VSTi plug-ins...");

        _VSTiPlugIns = _VSTiHost.GetPlugIns(std::u8string((const char8_t *) (const char *) CfgVSTiPlugInDirectoryPath.get()));

        if (!_VSTiPlugIns.empty())
        {
            console::print(STR_COMPONENT_BASENAME " found ", _VSTiPlugIns.size(), " VSTi plug-insinstruments.");

            size_t i = 0;

            for (const auto & PlugIn : _VSTiPlugIns)
            {
                installed_player_t ip((std::string("VSTi ") + PlugIn.Name), PlayerTypes::VSTi, PlugIn.FilePath, (size_t) -1, i);

                _InstalledPlayers.push_back(ip);

                if (_SelectedPlayer.FilePath == PlugIn.FilePath.c_str())
                    _SelectedPlayer.PlugInIndex = i;

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
        console::print(STR_COMPONENT_BASENAME " is enumerating CLAP plug-ins...");

        fs::path BaseDirectory(std::u8string((const char8_t *) CfgCLAPPlugInDirectoryPath.get().c_str()));

        _CLAPPlugIns = _CLAPHost.GetPlugIns(BaseDirectory);

        if (!_CLAPPlugIns.empty())
        {
            console::print(STR_COMPONENT_BASENAME " found ", _CLAPPlugIns.size(), " CLAP plug-ins.");

            size_t i = 0;

            for (const auto & PlugIn : _CLAPPlugIns)
            {
                installed_player_t ip((std::string("CLAP ") + PlugIn.Name), PlayerTypes::CLAP, PlugIn.FilePath, PlugIn.Index, i);

                _InstalledPlayers.push_back(ip);

                if (_SelectedPlayer.FilePath == PlugIn.FilePath.c_str())
                    _SelectedPlayer.PlugInIndex = i;

                ++i;
            }
        }
        else
            console::print(STR_COMPONENT_BASENAME " found no compatible CLAP plug-ins.");
    }

    #pragma endregion

    // Player Type
    {
        auto w = (CComboBox) GetDlgItem(IDC_PLAYER_TYPE);

        int SelectedIndex = -1;
        int i = 0;

        for (const auto & Player : _InstalledPlayers)
        {
            w.AddString(::UTF8ToWide(Player.Name).c_str());

            if (Player == _SelectedPlayer)
                SelectedIndex = i;

            ++i;
        }

        if (SelectedIndex == -1)
        {
            _SelectedPlayer = installed_player_t("", PlayerTypes::Default, "", (size_t) -1, (size_t) -1);

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

    // Configure
    {
        UpdateConfigureButton();
    }

    // Sample Rate
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
/*
            if ((_SelectedPlayer.Type == PlayerTypes::EmuDeMIDI) && _IsRunning)
                w.EnableWindow(FALSE);
*/
        }
    }

    // Looping
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
        ::uSetDlgItemText(m_hWnd, IDC_LOOP_COUNT, pfc::format_int(CfgLoopCount));
        ::uSetDlgItemText(m_hWnd, IDC_FADE_OUT_TIME, pfc::format_int(CfgFadeOutTime));
    }

    // MIDI
    {
        auto w = GetDlgItem(IDC_MIDI_FLAVOR);

        static const char * Flavors[] =
        {
            "Default", "GM", "GM2", "GS SC-55", "GS SC-88", "GS SC-88 Pro", "GS SC-8820", "XG"
        };

        for (const auto & Flavor : Flavors)
            ::uSendMessageText(w, CB_ADDSTRING, 0, Flavor);

        ::SendMessage(w, CB_SETCURSEL, (WPARAM) CfgMIDIFlavor, 0);

        SendDlgItemMessage(IDC_MIDI_EFFECTS,               BM_SETCHECK, (WPARAM) (CfgUseMIDIEffects ? 0 : 1));
        SendDlgItemMessage(IDC_MIDI_USE_MT32EMU_WITH_MT32, BM_SETCHECK, (WPARAM) CfgUseMT32EmuWithMT32);
        SendDlgItemMessage(IDC_MIDI_USE_VSTI_WITH_XG,      BM_SETCHECK, (WPARAM) CfgUseVSTiWithXG);
        SendDlgItemMessage(IDC_MIDI_DETECT_EXTRA_DRUM,     BM_SETCHECK, (WPARAM) CfgDetectExtraDrum);

        const bool Enabled = _SelectedPlayer.SupportsMIDIFlavor();

        GetDlgItem(IDC_MIDI_FLAVOR_TEXT).EnableWindow(Enabled);
        GetDlgItem(IDC_MIDI_FLAVOR)     .EnableWindow(Enabled);
        GetDlgItem(IDC_MIDI_EFFECTS)    .EnableWindow(Enabled);

        SendDlgItemMessage(IDC_EMIDI_EXCLUSION,    BM_SETCHECK, (WPARAM) CfgExcludeEMIDITrackDesignation);

        SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_SETCHECK, (WPARAM) CfgFilterInstruments);
        SendDlgItemMessage(IDC_FILTER_BANKS,       BM_SETCHECK, (WPARAM) CfgFilterBanks);
        SendDlgItemMessage(IDC_SKIP_TO_FIRST_NOTE, BM_SETCHECK, (WPARAM) CfgSkipToFirstNote);

        SendDlgItemMessage(IDC_TOUHOU_LOOPS,       BM_SETCHECK, (WPARAM) CfgDetectTouhouLoops);
        SendDlgItemMessage(IDC_RPGMAKER_LOOPS,     BM_SETCHECK, (WPARAM) CfgDetectRPGMakerLoops);
        SendDlgItemMessage(IDC_LEAPFROG_LOOPS,     BM_SETCHECK, (WPARAM) CfgDetectLeapFrogLoops);
        SendDlgItemMessage(IDC_XMI_LOOPS,          BM_SETCHECK, (WPARAM) CfgDetectXMILoops);
        SendDlgItemMessage(IDC_FF7_LOOPS,          BM_SETCHECK, (WPARAM) CfgDetectFF7Loops);
    }
    #pragma endregion

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
        VSTi::Player Player;

        if (Player.LoadVST(_SelectedPlayer.FilePath))
        {
            if (_VSTiHost.Config.size() != 0)
                Player.SetChunk(_VSTiHost.Config.data(), _VSTiHost.Config.size());

            Player.DisplayEditorModal();

            Player.GetChunk(_VSTiHost.Config);
        }
    }
/*
    else
    if (_SelectedPlayer.Type == PlayerTypes::ADL)
        ui_control::get()->show_preferences(GUID_PREFS_FM);
*/
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
            _VSTiHost.Config = CfgVSTiConfig[_VSTiPlugIns[_SelectedPlayer.PlugInIndex].Id];
    }

    // Configure
    {
        UpdateConfigureButton();
    }

    // Sample Rate
    {
/*
        const BOOL Enable = (_SelectedPlayer.Type != PlayerTypes::EmuDeMIDI) || !_IsRunning;

        GetDlgItem(IDC_SAMPLERATE).EnableWindow(Enable);
*/
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
/*
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
*/
    OnChanged();
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
                if (CfgPlugInFilePath.get() != (const char *) _SelectedPlayer.FilePath.u8string().c_str())
                    return true;

                t_uint32 Id = _VSTiPlugIns[_SelectedPlayer.PlugInIndex].Id;

                if (_VSTiHost.Config.size() != CfgVSTiConfig[Id].size())
                    return true;

                if ((_VSTiHost.Config.size() != 0) && (::memcmp(_VSTiHost.Config.data(), CfgVSTiConfig[Id].data(), _VSTiHost.Config.size()) != 0))
                    return true;
            }
            else
            if (_SelectedPlayer.Type == PlayerTypes::CLAP)
            {
                if (CfgPlugInFilePath.get() != (const char *) _SelectedPlayer.FilePath.u8string().c_str())
                    return true;

                if (CfgCLAPIndex != (int64_t) _SelectedPlayer.Index)
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

        if (GetDlgItemInt(IDC_LOOP_COUNT, NULL, FALSE) != (UINT) CfgLoopCount)
            return true;

        if (GetDlgItemInt(IDC_FADE_OUT_TIME, NULL, FALSE) != (UINT) CfgFadeOutTime)
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

        if (SendDlgItemMessage(IDC_MIDI_USE_MT32EMU_WITH_MT32, BM_GETCHECK) != CfgUseMT32EmuWithMT32)
            return true;

        if (SendDlgItemMessage(IDC_MIDI_USE_VSTI_WITH_XG, BM_GETCHECK) != CfgUseVSTiWithXG)
            return true;

        if (SendDlgItemMessage(IDC_MIDI_DETECT_EXTRA_DRUM, BM_GETCHECK) != CfgDetectExtraDrum)
            return true;
    }
    #pragma endregion

    #pragma region Miscellaneous
    {
        if (SendDlgItemMessage(IDC_EMIDI_EXCLUSION, BM_GETCHECK) != CfgExcludeEMIDITrackDesignation)
            return true;

        if (SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_GETCHECK) != CfgFilterInstruments)
            return true;

        if (SendDlgItemMessage(IDC_FILTER_BANKS, BM_GETCHECK) != CfgFilterBanks)
            return true;

        if (SendDlgItemMessage(IDC_SKIP_TO_FIRST_NOTE, BM_GETCHECK) != CfgSkipToFirstNote)
            return true;
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
/// Updates the dialog according to the values of the settings.
/// </summary>
void PreferencesRootPage::UpdateDialog() const noexcept
{
    #pragma region MIDI Flavors

    pfc::string FilePath = CfgVSTiXGPlugInFilePath;

    GetDlgItem(IDC_MIDI_USE_VSTI_WITH_XG).EnableWindow(!FilePath.isEmpty());

    #pragma endregion
}

/// <summary>
/// Updates the Configure button.
/// </summary>
void PreferencesRootPage::UpdateConfigureButton() noexcept
{
    BOOL Enable = FALSE;

    if ((_SelectedPlayer.Type == PlayerTypes::VSTi) && (_SelectedPlayer.PlugInIndex != (size_t) -1))
    {
        const auto &  Plugin = _VSTiPlugIns[_SelectedPlayer.PlugInIndex];

        Enable = Plugin.HasEditor;

        _VSTiHost.Config = CfgVSTiConfig[Plugin.Id];
    }
/*
    else
    if (_SelectedPlayer.Type == PlayerTypes::ADL)
        Enable = TRUE;
*/
/*
    else
    if ((_SelectedPlayer.Type == PlayerTypes::CLAP) && (_SelectedPlayer.PlugInIndex != (size_t) -1))
    {
        const auto &  Plugin = _CLAPPlugIns[_SelectedPlayer.PlugInIndex];

        Enable = Plugin.HasGUI;
    }
*/
    GetDlgItem(IDC_CONFIGURE).EnableWindow(Enable);
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
