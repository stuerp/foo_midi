
/** $VER: PreferencesRoot.cpp (2024.06.09) P. Stuer **/

#include "framework.h"

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
#include "MIDIPreset.h"

#include "ADLPlayer.h"
#include "BMPlayer.h"
#include "NukePlayer.h"
#include "VSTiPlayer.h"
#include "FSPlayer.h"

#include "VSTi.h"
#include "SecretSauce.h"

#pragma hdrstop

extern volatile int _IsRunning;

#pragma region Sample Rate

static const GUID GUIDCfgSampleRateHistory = { 0x408aa155, 0x4c42, 0x42b5, { 0x8c, 0x3e, 0xd1, 0xc, 0x35, 0xdd, 0x5e, 0xf1 } };
static cfg_dropdown_history CfgSampleRateHistory(GUIDCfgSampleRateHistory, 16);

static const int _SampleRates[] = { 8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 49716, 64000, 88200, 96000 };

#pragma endregion

#pragma region FluidSynth

#pragma warning(disable: 4820)
struct InterpolationMethod
{
    const wchar_t * Name;
    int Id;
};
#pragma warning(default: 4820)

static const InterpolationMethod _InterpolationMethods[] =
{
    { L"None", FLUID_INTERP_NONE },
    { L"Linear", FLUID_INTERP_LINEAR },
    { L"Cubic", FLUID_INTERP_4THORDER },
    { L"7th Order Sinc", FLUID_INTERP_7THORDER }
};

#pragma endregion

#pragma region Munt

const char * _MuntGMSets[] =
{
    "Roland",
    "Sierra / King's Quest 6",
};

const size_t _MuntGMSetCount = _countof(_MuntGMSets);

#pragma endregion

#pragma warning(disable: 4820)

/// <summary>
/// Implements the main preferences page.
/// </summary>
class PreferencesRootPage : public CDialogImpl<PreferencesRootPage>, public preferences_page_instance
{
public:
    PreferencesRootPage(preferences_page_callback::ptr callback) noexcept : _IsBusy(false), _InstalledKnownPlayerCount(), _HasSecretSauce(), _HasFluidSynth(), _Callback(callback) { }

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

        #pragma region Nuke

        COMMAND_HANDLER_EX(IDC_NUKE_PRESET, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_NUKE_PANNING, BN_CLICKED, OnButtonClick)

        #pragma endregion

        #pragma region ADL

        COMMAND_HANDLER_EX(IDC_ADL_BANK, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_ADL_CHIPS, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_ADL_CHIPS, CBN_EDITCHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_ADL_PANNING, BN_CLICKED, OnButtonClick)

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

    void OnPlayerTypeChange(UINT, int, CWindow);
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

    static bool IsPluginAlwaysPresent(PreferencesRootPage *)
    {
        return true;
    }

    static bool IsPluginNeverPresent(PreferencesRootPage *)
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

    struct KnownPlayer
    {
        const char * Name;
        PlayerType Type;
        bool (*IsPresent)(PreferencesRootPage *);
    };

    static const KnownPlayer _KnownPlayers[];

    struct InstalledPlayer
    {
        pfc::string8 Name;
        PlayerType Type;
    };

    std::vector<InstalledPlayer> _InstalledPlayers;

    size_t _InstalledKnownPlayerCount; // Number of installed known players.

    #pragma endregion

    #pragma region Secret Sauce

    bool _HasSecretSauce;

    #pragma endregion

    #pragma region FluidSynth

    bool _HasFluidSynth;

    #pragma endregion

    #pragma region BASS MIDI

    pfc::string8 _CacheStatusText;
    pfc::string8 _CacheStatusTextCurrent;

    #pragma endregion

    #pragma region ADL

    struct ADLBank
    {
        int Number;
        pfc::string8 Name;

        ADLBank() : Number(-1), Name() { }
        ADLBank(const ADLBank & b) : Number(b.Number), Name(b.Name) { }
        ADLBank(int number, const char * name) : Number(number), Name(name) { }

        ADLBank & operator =(const ADLBank & b)
        {
            Number = b.Number;
            Name = b.Name;

            return *this;
        }

        bool operator ==(const ADLBank & b) const { return Number == b.Number; }
        bool operator !=(const ADLBank & b) const { return !operator ==(b); }
        bool operator < (const ADLBank & b) const { return Name < b.Name; }
        bool operator > (const ADLBank & b) const { return Name > b.Name; }
    };

    pfc::list_t<ADLBank> _ADLBanks;

    #pragma endregion

#ifdef DXISUPPORT
    pfc::array_t<CLSID> dxi_plugins;
#endif

    const preferences_page_callback::ptr _Callback;

    fb2k::CCoreDarkModeHooks _DarkModeHooks;
};

/// <summary>
/// Contains all the plugins that are build into the component.
/// </summary>
const PreferencesRootPage::KnownPlayer PreferencesRootPage::_KnownPlayers[] =
{
    { "Emu de MIDI",    PlayerType::EmuDeMIDI,    IsPluginAlwaysPresent },
    { "FluidSynth",     PlayerType::FluidSynth,   IsFluidSynthPresent },
    { "BASSMIDI",       PlayerType::BASSMIDI,     IsPluginAlwaysPresent },
    { "DirectX",        PlayerType::DirectX,      IsPluginNeverPresent },
    { "Super Munt GM",  PlayerType::SuperMunt,    IsPluginAlwaysPresent },
    { "LibADLMIDI",     PlayerType::ADL,          IsPluginAlwaysPresent },
    { "LibOPNMIDI",     PlayerType::OPN,          IsPluginAlwaysPresent },
    { "OPL MIDI",       PlayerType::OPL,          IsPluginNeverPresent },
    { "Nuke",           PlayerType::Nuke,         IsPluginAlwaysPresent },
    { "Secret Sauce",   PlayerType::SecretSauce,  IsSecretSaucePresent },
    { "MCI",            PlayerType::MCI,          IsPluginNeverPresent }
};

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
    #pragma region PlayerType
    {
        int SelectedIndex = (int) SendDlgItemMessageW(IDC_PLAYER_TYPE, CB_GETCURSEL);

        if (SelectedIndex != -1)
        {
            if ((size_t) SelectedIndex < _InstalledKnownPlayerCount)
            {
                CfgPlayerType = (int) _InstalledPlayers[(size_t) SelectedIndex].Type;
                CfgVSTiFilePath = "";
            }
            else
            {
                CfgPlayerType = (int) PlayerType::VSTi;

                size_t VSTiIndex = (size_t) (SelectedIndex - _InstalledKnownPlayerCount);

                VSTi::plugin_t & Plugin = VSTi::PlugIns[VSTiIndex];

                CfgVSTiFilePath = Plugin.PathName.c_str();
                CfgVSTiConfig[Plugin.Id] = VSTi::Config;
            }
        }
    }
    #pragma endregion

    #pragma region Sample Rate
    {
        UINT t = std::clamp(GetDlgItemInt(IDC_SAMPLERATE, NULL, FALSE), 6000u, 192000u);

        SetDlgItemInt(IDC_SAMPLERATE, t, FALSE);

        CfgSampleRateHistory.add_item(pfc::format_int(t));
        CfgSampleRate = (t_int32) t;
    }
    #pragma endregion

    #pragma region Looping
    {
        CfgLoopTypePlayback     = (t_int32) SendDlgItemMessage(IDC_LOOP_PLAYBACK, CB_GETCURSEL);
        CfgLoopTypeOther        = (t_int32) SendDlgItemMessage(IDC_LOOP_OTHER, CB_GETCURSEL);

        CfgDecayTime            = (t_int32) GetDlgItemInt(IDC_DECAY_TIME, NULL, FALSE);
    }
    #pragma endregion

    #pragma region MIDI Flavor
    {
        CfgMIDIStandard         = (t_int32) SendDlgItemMessage(IDC_MIDI_FLAVOR, CB_GETCURSEL);

        CfgUseMIDIEffects       = (t_int32) SendDlgItemMessage(IDC_MIDI_EFFECTS, BM_GETCHECK) ? 0 : 1;
        CfgUseSuperMuntWithMT32 = (t_int32) SendDlgItemMessage(IDC_MIDI_USE_SUPER_MUNT, BM_GETCHECK);
        CfgUseVSTiWithXG        = (t_int32) SendDlgItemMessage(IDC_MIDI_USE_VSTI_WITH_XG, BM_GETCHECK);
    }
    #pragma endregion

    #pragma region("Miscellaneous")
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
    #pragma endregion

    #pragma region("FluidSynth")
    {
        int SelectedIndex = (int) SendDlgItemMessage(IDC_FLUIDSYNTH_INTERPOLATION, CB_GETCURSEL);

        if (SelectedIndex != -1)
            CfgFluidSynthInterpolationMode = _InterpolationMethods[SelectedIndex].Id;
    }
    #pragma endregion

    #pragma region BASS MIDI
    {
        wchar_t Text[32];

        GetDlgItemTextW(IDC_BASSMIDI_VOLUME, Text, _countof(Text));

        CfgBASSMIDIVolume = std::clamp(::_wtof(Text), 0., 2.);

        ::uSetDlgItemText(m_hWnd, IDC_BASSMIDI_VOLUME, pfc::format_float(CfgBASSMIDIVolume, 4, 2));
    }
    {
        CfgBASSMIDIInterpolationMode = (t_int32) SendDlgItemMessage(IDC_RESAMPLING_MODE, CB_GETCURSEL);
    }
    #pragma endregion

    #pragma region Munt
    {
        CfgMuntGMSet = (t_int32) SendDlgItemMessage(IDC_MUNT_GM_SET, CB_GETCURSEL);
    }
    #pragma endregion

    #pragma region ADL
    {
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_ADL_BANK, CB_GETCURSEL);

            if (SelectedIndex < 0 || SelectedIndex >= (int)_ADLBanks.get_count())
                SelectedIndex = 0;

            CfgADLBank = _ADLBanks[(size_t) SelectedIndex].Number;
        }

        {
            int Value = std::clamp((int) GetDlgItemInt(IDC_ADL_CHIPS, NULL, FALSE), 1, 100);

            SetDlgItemInt(IDC_ADL_CHIPS, (UINT) Value, FALSE);

            CfgADLChipCount = Value;
        }

        CfgADLPanning = (t_int32) SendDlgItemMessage(IDC_ADL_PANNING, BM_GETCHECK);
    }
    #pragma endregion

    #pragma region Nuke
    {
        size_t SelectedIndex = (size_t) SendDlgItemMessage(IDC_NUKE_PRESET, CB_GETCURSEL);

        uint32_t Synth;
        uint32_t Bank;

        NukePlayer::GetPreset(SelectedIndex, Synth, Bank);

        CfgNukeSynthesizer = (t_int32) Synth;
        CfgNukeBank        = (t_int32) Bank;
        CfgNukePanning     = (t_int32) SendDlgItemMessage(IDC_NUKE_PANNING, BM_GETCHECK);
    }
    #pragma endregion

    OnChanged();
}

/// <summary>
/// Resets the dialog state to default.
/// </summary>
void PreferencesRootPage::reset()
{
    PlayerType PlayerType = PlayerType::Default;

    #pragma region Player Type
    {
        int SelectedIndex = -1;

        int i = 0;

        for (const auto & Iter : _InstalledPlayers)
        {
            if (Iter.Type == PlayerType)
            {
                SelectedIndex = i;
                break;
            }

            ++i;
        }

        SendDlgItemMessage(IDC_PLAYER_TYPE, CB_SETCURSEL, (WPARAM) SelectedIndex);
        GetDlgItem(IDC_CONFIGURE).EnableWindow(FALSE);
    }
    #pragma endregion

    #pragma region Sample Rate
    {
        if ((PlayerType == PlayerType::EmuDeMIDI) && _IsRunning)
            GetDlgItem(IDC_SAMPLERATE).EnableWindow(FALSE);

        SetDlgItemInt(IDC_SAMPLERATE, DefaultSampleRate, FALSE);
    }
    #pragma endregion

    #pragma region Looping
    {
        SendDlgItemMessage(IDC_LOOP_PLAYBACK, CB_SETCURSEL, DefaultPlaybackLoopType);
        SendDlgItemMessage(IDC_LOOP_OTHER,    CB_SETCURSEL, DefaultOtherLoopType);
    }
    #pragma endregion

    #pragma region MIDI Flavor
    {
        bool Enable = (PlayerType == PlayerType::VSTi) || (PlayerType == PlayerType::FluidSynth) || (PlayerType == PlayerType::BASSMIDI) || (PlayerType == PlayerType::SecretSauce);

        GetDlgItem(IDC_MIDI_FLAVOR_TEXT).EnableWindow(Enable);
        GetDlgItem(IDC_MIDI_FLAVOR).EnableWindow(Enable);
        GetDlgItem(IDC_MIDI_EFFECTS).EnableWindow(Enable);

        SendDlgItemMessage(IDC_MIDI_FLAVOR,           CB_SETCURSEL, DefaultMIDIFlavor);
        SendDlgItemMessage(IDC_MIDI_EFFECTS,          BM_SETCHECK,  DefaultUseMIDIEffects ? 0 : 1);
        SendDlgItemMessage(IDC_MIDI_USE_SUPER_MUNT,   BM_SETCHECK, DefaultUseSuperMuntWithMT32);
        SendDlgItemMessage(IDC_MIDI_USE_VSTI_WITH_XG, BM_SETCHECK, DefaultUseVSTiWithXG);
    }
    #pragma endregion

    #pragma region Miscellaneous
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
    #pragma endregion

    #pragma region FluidSynth
    {
        int SelectedIndex = -1;

        for (int i = 0; i < _countof(_InterpolationMethods); ++i)
        {
            if (_InterpolationMethods[i].Id == DefaultFluidSynthInterpolationMethod)
            {
                SelectedIndex = i;
                break;
            }
        }

        SendDlgItemMessage(IDC_FLUIDSYNTH_INTERPOLATION, CB_SETCURSEL, (WPARAM) SelectedIndex);

        bool Enable = (PlayerType == PlayerType::FluidSynth);

        GetDlgItem(IDC_FLUIDSYNTH_INTERPOLATION_TEXT).EnableWindow(Enable);
        GetDlgItem(IDC_FLUIDSYNTH_INTERPOLATION).EnableWindow(Enable);
    }
    #pragma endregion

    #pragma region BASS MIDI
    {
        SendDlgItemMessage(IDC_RESAMPLING_MODE, CB_SETCURSEL, DefaultBASSMIDIInterpolationMode);

        bool Enable = (PlayerType == PlayerType::BASSMIDI);

        const int ControlId[] =
        {
            IDC_BASSMIDI_VOLUME_LBL, IDC_BASSMIDI_VOLUME,
            IDC_RESAMPLING_LBL, IDC_RESAMPLING_MODE,
            IDC_CACHED_LBL, IDC_CACHED
        };

        for (size_t i = 0; i < _countof(ControlId); ++i)
            GetDlgItem(ControlId[i]).EnableWindow(Enable);
    }
    #pragma endregion

    #pragma region Munt
    {
        BOOL Enable = (PlayerType == PlayerType::SuperMunt);

        GetDlgItem(IDC_MUNT_GM_TEXT).EnableWindow(Enable);
        GetDlgItem(IDC_MUNT_GM_SET).EnableWindow(Enable);

        GetDlgItem(IDC_MUNT_WARNING).ShowWindow((PlayerType == PlayerType::SuperMunt) ? SW_SHOW : SW_HIDE);

        SendDlgItemMessage(IDC_NUKE_PRESET, CB_SETCURSEL, (WPARAM) DefaultGMSet);
    }
    #pragma endregion

    #pragma region ADL
    {
        int SelectedIndex = -1;
        int i = 0;

        for (const auto & Iter : _ADLBanks)
        {
            if (Iter.Number == DefaultADLBank)
            {
                SelectedIndex = i;
                break;
            }
        }

        SendDlgItemMessage(IDC_ADL_BANK, CB_SETCURSEL, (WPARAM) SelectedIndex);
        SetDlgItemInt(IDC_ADL_CHIPS, DefaultADLChipCount, 0);
        SendDlgItemMessage(IDC_ADL_PANNING, BM_SETCHECK, DefaultADLPanning);

        if (PlayerType != PlayerType::ADL)
        {
            const int ControlId[] =
            {
                IDC_ADL_BANK_TEXT, IDC_ADL_BANK,
                IDC_ADL_CHIPS_TEXT, IDC_ADL_CHIPS,
                IDC_RESAMPLING_LBL, IDC_RESAMPLING_MODE,
                IDC_ADL_PANNING
            };

            for (const auto & Iter : ControlId)
                GetDlgItem(Iter).EnableWindow(FALSE);
        }
    }
    #pragma endregion

    #pragma region Nuke
    {
        BOOL Enable = (PlayerType == PlayerType::Nuke);

        GetDlgItem(IDC_NUKE_PRESET_TEXT).EnableWindow(Enable);
        GetDlgItem(IDC_NUKE_PRESET).EnableWindow(Enable);
        GetDlgItem(IDC_NUKE_PANNING).EnableWindow(Enable);

        SendDlgItemMessage(IDC_NUKE_PRESET, CB_SETCURSEL, (WPARAM)NukePlayer::GetPresetIndex(DefaultNukeSynth, DefaultNukeBank));
        SendDlgItemMessage(IDC_NUKE_PANNING, BM_SETCHECK, DefaultNukePanning);
    }
    #pragma endregion

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
    {
        // Gets the installed players. FluidSynth and Secret Sauce are optional.
        for (const auto & Iter : _KnownPlayers)
        {
            if (Iter.IsPresent(this))
            {
                struct InstalledPlayer ip;

                ip.Name = Iter.Name;
                ip.Type = Iter.Type;

                _InstalledPlayers.push_back(ip);
            }
        }

        _InstalledKnownPlayerCount = _InstalledPlayers.size();
    }
    #pragma endregion

    #pragma region VSTi Players

    size_t VSTiIndex = ~0u;

    // Add the VSTi to the installed player list.
    {
        VSTi::Enumerate();

        size_t VSTiCount = VSTi::PlugIns.size();

        if (VSTiCount > 0)
        {
            console::print("Found ", pfc::format_int((t_int64) VSTiCount), " VST instruments.");

            size_t i = 0;

            for (const auto & it : VSTi::PlugIns)
            {
                struct InstalledPlayer ip;

                ip.Name = it.Name.c_str();
                ip.Type = PlayerType::VSTi;

                _InstalledPlayers.push_back(ip);

                if (CfgVSTiFilePath.get() == it.PathName.c_str())
                    VSTiIndex = i;

                ++i;
            }
        }
        else
            console::print("Found no VST instruments.");
    }

    #pragma endregion

    // Determine the selected player.
    PlayerType PlayerType = (enum PlayerType) (uint8_t) CfgPlayerType;

    if ((PlayerType == PlayerType::VSTi) && (VSTiIndex == ~0u))
        PlayerType = PlayerType::Default; // In case the VSTi is no longer available.
    else
    if ((PlayerType == PlayerType::FluidSynth) && !_HasFluidSynth)
        PlayerType = PlayerType::BASSMIDI; // In case FluidSynth is no longer available.
    else
    if ((PlayerType == PlayerType::SecretSauce) && !_HasSecretSauce)
        PlayerType = PlayerType::BASSMIDI; // In case Secret Sauce is no longer available.

    #pragma endregion

    #pragma region Configure
    {
        auto w = (CComboBox) GetDlgItem(IDC_CONFIGURE);

        if (PlayerType != PlayerType::VSTi)
        {
            w.EnableWindow(FALSE);
        }
        else
        if (VSTiIndex != ~0u)
        {
            const VSTi::plugin_t & Plugin = VSTi::PlugIns[VSTiIndex];

            w.EnableWindow(Plugin.HasEditor);

            VSTi::Config = CfgVSTiConfig[Plugin.Id];
        }
    }
    #pragma endregion

    #pragma region Player Type
    {
        auto w = (CComboBox) GetDlgItem(IDC_PLAYER_TYPE);

        int SelectedIndex = -1;
        int i = 0;

        for (const auto & Player : _InstalledPlayers)
        {
            w.AddString(pfc::wideFromUTF8(Player.Name));

            if (Player.Type == PlayerType)
                SelectedIndex = i;

            i++;
        }

        if ((PlayerType == PlayerType::VSTi) && (VSTiIndex != ~0))
            SelectedIndex = (int) (_InstalledKnownPlayerCount + VSTiIndex);

         w.SetCurSel(SelectedIndex);
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

            if ((PlayerType == PlayerType::EmuDeMIDI) && _IsRunning)
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

            for (const auto & Iter : LoopTypeDescriptions)
            {
                w1.AddString(Iter);
                w2.AddString(Iter);
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

        ::SendMessage(w, CB_SETCURSEL, (WPARAM) CfgMIDIStandard, 0);

        SendDlgItemMessage(IDC_MIDI_EFFECTS,          BM_SETCHECK, (WPARAM) (CfgUseMIDIEffects ? 0 : 1));
        SendDlgItemMessage(IDC_MIDI_USE_SUPER_MUNT,   BM_SETCHECK, (WPARAM) CfgUseSuperMuntWithMT32);
        SendDlgItemMessage(IDC_MIDI_USE_VSTI_WITH_XG, BM_SETCHECK, (WPARAM) CfgUseVSTiWithXG);

        bool Enabled = ((PlayerType == PlayerType::VSTi) || (PlayerType == PlayerType::FluidSynth) || (PlayerType == PlayerType::BASSMIDI) || (PlayerType == PlayerType::SecretSauce));

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

        for (const auto & Iter : _InterpolationMethods)
        {
            w.AddString(Iter.Name);

            if (Iter.Id == CfgFluidSynthInterpolationMode)
                SelectedIndex = (int) i;

            ++i;
        }

        w.SetCurSel(SelectedIndex);

        bool Enable = (PlayerType == PlayerType::FluidSynth);

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

        w.SetCurSel((int) CfgBASSMIDIInterpolationMode);

        bool Enable = (PlayerType == PlayerType::BASSMIDI);

        const int ControlIds[] =
        {
            IDC_BASSMIDI_VOLUME_LBL, IDC_BASSMIDI_VOLUME,
            IDC_RESAMPLING_LBL, IDC_RESAMPLING_MODE,
            IDC_CACHED_LBL, IDC_CACHED
        };

        for (const auto & Iter : ControlIds)
            GetDlgItem(Iter).EnableWindow(Enable);
    }
    #pragma endregion

    #pragma region Super Munt
    {
        auto w = (CComboBox) GetDlgItem(IDC_MUNT_GM_SET);

        for (const auto & Iter : _MuntGMSets)
            ::uSendMessageText(w, CB_ADDSTRING, 0, Iter);

        w.SetCurSel((int) CfgMuntGMSet);

        BOOL Enable = (PlayerType == PlayerType::SuperMunt);

        GetDlgItem(IDC_MUNT_GM_TEXT).EnableWindow(Enable);
        GetDlgItem(IDC_MUNT_GM_SET).EnableWindow(Enable);

        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(Enable ? SW_SHOW : SW_HIDE);
    }
    #pragma endregion

    #pragma region ADL
    {
        const char * const * BankNames = ::adl_getBankNames();
        const size_t BankCount = (size_t) ::adl_getBanksCount();

        if (BankNames && (BankCount > 0))
        {
            for (size_t i = 0; i < BankCount; ++i)
                _ADLBanks += ADLBank((int) i, BankNames[i]);

            _ADLBanks.sort();
        }

        {
            auto w = (CComboBox) GetDlgItem(IDC_ADL_BANK);

            int SelectedBank = 0;

            for (size_t i = 0; i < _ADLBanks.get_count(); ++i)
            {
                w.AddString(pfc::wideFromUTF8(_ADLBanks[i].Name));

                if (_ADLBanks[i].Number == CfgADLBank)
                    SelectedBank = (int) i;
            }

            w.SetCurSel(SelectedBank);
        }

        {
            static const wchar_t * ChipCounts[] = { L"1", L"2", L"5", L"10", L"25", L"50", L"100" };

            auto w = (CComboBox) GetDlgItem(IDC_ADL_CHIPS);

            for (const auto & Iter : ChipCounts)
                w.AddString(Iter);

            SetDlgItemInt(IDC_ADL_CHIPS, (UINT) CfgADLChipCount, 0);
        }

        SendDlgItemMessage(IDC_ADL_PANNING, BM_SETCHECK, (WPARAM) CfgADLPanning);

        if (PlayerType != PlayerType::ADL)
        {
            GetDlgItem(IDC_ADL_BANK_TEXT).EnableWindow(FALSE);
            GetDlgItem(IDC_ADL_BANK).EnableWindow(FALSE);
        }

        if (PlayerType != PlayerType::ADL && PlayerType != PlayerType::OPN)
        {
            GetDlgItem(IDC_ADL_CHIPS_TEXT).EnableWindow(FALSE);
            GetDlgItem(IDC_ADL_CHIPS).EnableWindow(FALSE);
            GetDlgItem(IDC_ADL_PANNING).EnableWindow(FALSE);
        }
    }
    #pragma endregion

    #pragma region Nuke
    {
        auto w = GetDlgItem(IDC_NUKE_PRESET);

        size_t PresetNumber = 0;

        NukePlayer::EnumeratePresets([w, PresetNumber] (const pfc::string8 name, unsigned int synth, unsigned int bank) mutable noexcept
        {
            ::uSendMessageText(w, CB_ADDSTRING, 0, name.c_str());

            if ((synth == (unsigned int) CfgNukeSynthesizer) && (bank == (unsigned int) CfgNukeBank))
                ::SendMessage(w, CB_SETCURSEL, PresetNumber, 0);

            PresetNumber++;
        });

        SendDlgItemMessage(IDC_NUKE_PANNING, BM_SETCHECK, (WPARAM) CfgNukePanning);

        if (PlayerType != PlayerType::Nuke)
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
    int SelectedIndex = (int) GetDlgItem(IDC_PLAYER_TYPE).SendMessage(CB_GETCURSEL, 0, 0);

    if ((SelectedIndex != -1) && ((size_t) SelectedIndex < _InstalledKnownPlayerCount))
        return;

    _IsBusy = true;
    OnChanged();

    VSTiPlayer Player;

    size_t VSTiIndex = SelectedIndex - _InstalledKnownPlayerCount;

    if (Player.LoadVST(VSTi::PlugIns[VSTiIndex].PathName.c_str()))
    {
        if (VSTi::Config.size() != 0)
            Player.SetChunk(VSTi::Config.data(), VSTi::Config.size());

        Player.DisplayEditorModal();

        Player.GetChunk(VSTi::Config);
    }

    _IsBusy = false;
    OnChanged();
}

/// <summary>
/// Updates the dialog controls when the player type changes.
/// </summary>
void PreferencesRootPage::OnPlayerTypeChange(UINT, int, CWindow w)
{
    PlayerType PlayerType = PlayerType::Unknown;

    int SelectedIndex = (int) ::SendMessage(w, CB_GETCURSEL, 0, 0);
    size_t VSTiIndex = ~0u;

    // Player Type
    if (SelectedIndex != -1)
    {
        PlayerType = _InstalledPlayers[(size_t) SelectedIndex].Type;

        if ((size_t) SelectedIndex >= _InstalledKnownPlayerCount)
        {
            VSTiIndex = SelectedIndex - _InstalledKnownPlayerCount;
            VSTi::Config = CfgVSTiConfig[VSTi::PlugIns[VSTiIndex].Id];
        }
    }

    // Configure
    {
        const BOOL Enable = (PlayerType == PlayerType::VSTi) ? VSTi::PlugIns[VSTiIndex].HasEditor : FALSE;

        GetDlgItem(IDC_CONFIGURE).EnableWindow(Enable);
    }

    // Sample Rate
    {
        const BOOL Enable = (PlayerType != PlayerType::EmuDeMIDI) || !_IsRunning;

        GetDlgItem(IDC_SAMPLERATE).EnableWindow(Enable);
    }

    // Looping
    {
    }

    // MIDI Flavor and Effects
    {
        const BOOL Enable = (PlayerType == PlayerType::VSTi) || (PlayerType == PlayerType::FluidSynth) || (PlayerType == PlayerType::BASSMIDI) || (PlayerType == PlayerType::SecretSauce);

        const int ControlIds[] =
        {
            IDC_MIDI_FLAVOR_TEXT, IDC_MIDI_FLAVOR,
            IDC_MIDI_EFFECTS,
        };

        for (const auto & Iter : ControlIds)
            GetDlgItem(Iter).EnableWindow(Enable);
    }

    // Miscellaneous
    {
    }

    // FluidSynth
    {
        const BOOL Enable = (PlayerType == PlayerType::FluidSynth);

        GetDlgItem(IDC_FLUIDSYNTH_INTERPOLATION_TEXT).EnableWindow(Enable);
        GetDlgItem(IDC_FLUIDSYNTH_INTERPOLATION).EnableWindow(Enable);
    }

    // BASS MIDI resampling mode and cache status
    {
        const BOOL Enable = (PlayerType == PlayerType::BASSMIDI);

        const int ControlIds[] =
        {
            IDC_BASSMIDI_VOLUME_LBL, IDC_BASSMIDI_VOLUME,
            IDC_RESAMPLING_LBL, IDC_RESAMPLING_MODE,
            IDC_CACHED_LBL, IDC_CACHED
        };

        for (const auto & Iter : ControlIds)
            GetDlgItem(Iter).EnableWindow(Enable);
    }

    // Munt
    {
        const BOOL Enable = (PlayerType == PlayerType::SuperMunt);

        GetDlgItem(IDC_MUNT_GM_TEXT).EnableWindow(Enable);
        GetDlgItem(IDC_MUNT_GM_SET) .EnableWindow(Enable);

        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(Enable ? SW_SHOW : SW_HIDE);
    }

    // ADL
    {
        const BOOL Enable = (PlayerType == PlayerType::ADL);

        const int ControlIds[] =
        {
            IDC_ADL_BANK_TEXT, IDC_ADL_BANK,
            IDC_ADL_CHIPS_TEXT, IDC_ADL_CHIPS,
            IDC_ADL_PANNING
        };

        for (const auto & Iter : ControlIds)
            GetDlgItem(Iter).EnableWindow(Enable);
    }

    // Nuke
    {
        const BOOL Enable = (PlayerType == PlayerType::Nuke);

        const int ControlIds[] =
        {
            IDC_NUKE_PRESET_TEXT, IDC_NUKE_PRESET,
            IDC_NUKE_PANNING
        };

        for (const auto & Iter : ControlIds)
            GetDlgItem(Iter).EnableWindow(Enable);
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
        PlayerType PlayerType = PlayerType::Unknown;

        int SelectedIndex = (int) SendDlgItemMessage(IDC_PLAYER_TYPE, CB_GETCURSEL);

        if (SelectedIndex != -1)
        {
            if ((size_t) SelectedIndex < _InstalledKnownPlayerCount)
                PlayerType = _InstalledPlayers[(size_t) SelectedIndex].Type;
            else
                PlayerType = PlayerType::VSTi;

            if (PlayerType != (enum PlayerType) (int) CfgPlayerType)
                return true;

            if (PlayerType == PlayerType::VSTi)
            {
                size_t VSTiIndex = (size_t) (SelectedIndex - _InstalledKnownPlayerCount);

                if (CfgVSTiFilePath.get() != VSTi::PlugIns[VSTiIndex].PathName.c_str())
                    return true;

                t_uint32 Id = VSTi::PlugIns[VSTiIndex].Id;

                if (VSTi::Config.size() != CfgVSTiConfig[Id].size())
                    return true;

                if ((VSTi::Config.size() != 0) && (::memcmp(VSTi::Config.data(), CfgVSTiConfig[Id].data(), VSTi::Config.size()) != 0))
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
        if (SendDlgItemMessage(IDC_MIDI_FLAVOR, CB_GETCURSEL) != CfgMIDIStandard)
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

        if (SendDlgItemMessage(IDC_RESAMPLING_MODE, CB_GETCURSEL) != CfgBASSMIDIInterpolationMode)
            return true;
    }
    #pragma endregion

    #pragma region Munt
    {
        if (SendDlgItemMessage(IDC_MUNT_GM_SET, CB_GETCURSEL) != CfgMuntGMSet)
            return true;
    }
    #pragma endregion

    #pragma region ADL
    {
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_ADL_BANK, CB_GETCURSEL);

            if ((SelectedIndex < 0) || (SelectedIndex >= (int) _ADLBanks.get_count()))
                SelectedIndex = 0;

            if (_ADLBanks[(t_size) SelectedIndex].Number != CfgADLBank)
                return true;
        }

        if (GetDlgItemInt(IDC_ADL_CHIPS, NULL, FALSE) != (UINT) CfgADLChipCount)
            return true;

        if (SendDlgItemMessage(IDC_ADL_PANNING, BM_GETCHECK) != CfgADLPanning)
            return true;
    }
    #pragma endregion

    #pragma region Nuke
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

    pfc::string8 FilePath; AdvCfgVSTiXGPlugin.get(FilePath);

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
