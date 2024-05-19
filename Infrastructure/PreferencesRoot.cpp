
/** $VER: PreferencesRoot.cpp (2024.05.19) P. Stuer **/

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

#include <helpers/atl-misc.h>
#include <helpers/dropdown_helper.h>

#include <pfc/string-conv-lite.h>
#include <pfc/pathUtils.h>

#include "resource.h"

#include "Configuration.h"
#include "MIDIPreset.h"

#include "ADLPlayer.h"
#include "BMPlayer.h"
#include "NukePlayer.h"
#include "VSTiPlayer.h"
#include "FSPlayer.h"

#pragma hdrstop

extern char _DLLFileName[];
extern volatile int _IsRunning;

#pragma region("Sample Rate")

static const GUID GUIDCfgSampleRateHistory = { 0x408aa155, 0x4c42, 0x42b5, { 0x8c, 0x3e, 0xd1, 0xc, 0x35, 0xdd, 0x5e, 0xf1 } };
static cfg_dropdown_history CfgSampleRateHistory(GUIDCfgSampleRateHistory, 16);

static const int _SampleRates[] = { 8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 49716, 64000, 88200, 96000 };

#pragma endregion

#pragma region("FluidSynth")

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

#pragma region("Munt")

const char * _MuntGMSets[] =
{
    "Roland",
    "Sierra / King's Quest 6",
};

const size_t _MuntGMSetCount = _countof(_MuntGMSets);

#pragma endregion

/// <summary>
/// Implements the main preferences page.
/// </summary>
#pragma warning(disable: 4820)

class PreferencesRootPage : public CDialogImpl<PreferencesRootPage>, public preferences_page_instance
{
public:
    PreferencesRootPage(preferences_page_callback::ptr callback) noexcept : _IsBusy(false), _InstalledKnownPlayerCount(), _HasFluidSynth(), _HasSecretSauce(), _Callback(callback) { }

    PreferencesRootPage(const PreferencesRootPage&) = delete;
    PreferencesRootPage(const PreferencesRootPage&&) = delete;
    PreferencesRootPage& operator=(const PreferencesRootPage&) = delete;
    PreferencesRootPage& operator=(PreferencesRootPage&&) = delete;

    virtual ~PreferencesRootPage() { };

    #pragma region("preferences_page_instance")
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
        COMMAND_HANDLER_EX(IDC_XMI_LOOPS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_TOUHOU_LOOPS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_FF7_LOOPS, BN_CLICKED, OnButtonClick)

        #pragma endregion

        #pragma region FluidSynth

        COMMAND_HANDLER_EX(IDC_FLUIDSYNTH_INTERPOLATION, CBN_SELCHANGE, OnSelectionChange)

        #pragma endregion

        #pragma region BASS MIDI

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

    static bool IsFluidSynthPresent(PreferencesRootPage *)
    {
        return HasFluidSynth();
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

    #pragma region VSTi

    pfc::string8 _VSTiPath;

    void GetVSTiPlugins(const char * pathName = nullptr, puFindFile findFile = nullptr);

    struct VSTiPlugin
    {
        pfc::string8 Name;
        pfc::string8 PathName;
        uint32_t Id;
        bool HasEditor;
    };

    pfc::array_t<VSTiPlugin> _VSTiPlugIns;
    std::vector<uint8_t> _VSTiConfig;

    #pragma endregion

    #pragma region FluidSynth

    bool _HasFluidSynth;

    static bool HasFluidSynth() noexcept
    {
        return FluidSynth().Initialize(pfc::wideFromUTF8(CfgFluidSynthDirectoryPath.get()));
    }

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

    #pragma region("Secret Sauce")

    bool _HasSecretSauce;
    static bool HasSecretSauce() noexcept;

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

                VSTiPlugin & Plugin = _VSTiPlugIns[VSTiIndex];

                CfgVSTiFilePath = Plugin.PathName.c_str();
                CfgVSTiConfig[Plugin.Id] = _VSTiConfig;
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

    #pragma region("BASS MIDI")
    {
        wchar_t Text[32];

        GetDlgItemTextW(IDC_BASSMIDI_VOLUME, Text, _countof(Text));

        CfgBASSMIDIVolume = std::clamp((float) ::_wtof(Text), 0.f, 2.f);
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
            int ChipCount = std::clamp((int) GetDlgItemInt(IDC_ADL_CHIPS, NULL, FALSE), 1, 100);

            SetDlgItemInt(IDC_ADL_CHIPS, (UINT) ChipCount, FALSE);

            CfgADLChipCount = ChipCount;
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
        SendDlgItemMessage(IDC_MIDI_USE_VSTI_WITH_XG, BM_SETCHECK, DefaultUseSecretSauceWithXG);
    }
    #pragma endregion

    #pragma region Miscellaneous
    {
        SendDlgItemMessage(IDC_EMIDI_EXCLUSION,    BM_SETCHECK, DefaultEmuDeMIDIExclusion);
        SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_SETCHECK, DefaultFilterInstruments);
        SendDlgItemMessage(IDC_FILTER_BANKS,       BM_SETCHECK, DefaultFilterBanks);
        SendDlgItemMessage(IDC_TOUHOU_LOOPS,       BM_SETCHECK, default_cfg_thloopz);
        SendDlgItemMessage(IDC_RPGMAKER_LOOPS,     BM_SETCHECK, default_cfg_rpgmloopz);
        SendDlgItemMessage(IDC_XMI_LOOPS,          BM_SETCHECK, default_cfg_xmiloopz);
        SendDlgItemMessage(IDC_FF7_LOOPS,          BM_SETCHECK, default_cfg_ff7loopz);
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

    _VSTiConfig.resize(0);

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

    _HasFluidSynth = HasFluidSynth();
    _HasSecretSauce = HasSecretSauce();

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
        {
            GetVSTiPlugins();

            size_t VSTiCount = _VSTiPlugIns.get_size();

            if (VSTiCount > 0)
            {
                console::print("Found ",pfc::format_int((t_int64) VSTiCount), " VST instruments.");

                for (size_t i = 0; i < VSTiCount; ++i)
                {
                    struct InstalledPlayer ip;

                    ip.Name = _VSTiPlugIns[i].Name.c_str();
                    ip.Type = PlayerType::VSTi;

                    _InstalledPlayers.push_back(ip);

                    if (CfgVSTiFilePath.get() == _VSTiPlugIns[i].PathName)
                        VSTiIndex = i;
                }
            }
        }
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
            const VSTiPlugin & Plugin = _VSTiPlugIns[VSTiIndex];

            w.EnableWindow(Plugin.HasEditor);
            _VSTiConfig = CfgVSTiConfig[Plugin.Id];
        }
    }
    #pragma endregion

    #pragma region Player Type
    {
        auto w = (CComboBox) GetDlgItem(IDC_PLAYER_TYPE);

        int SelectedIndex = -1;
        int i = 0;

        for (auto Player : _InstalledPlayers)
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

    if (Player.LoadVST(_VSTiPlugIns[VSTiIndex].PathName.c_str()))
    {
        if (_VSTiConfig.size() != 0)
            Player.SetChunk(_VSTiConfig.data(), _VSTiConfig.size());

        Player.DisplayEditorModal();

        Player.GetChunk(_VSTiConfig);
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
            _VSTiConfig = CfgVSTiConfig[_VSTiPlugIns[VSTiIndex].Id];
        }
    }

    // Configure
    {
        if (PlayerType != PlayerType::VSTi)
            GetDlgItem(IDC_CONFIGURE).EnableWindow(FALSE);
        else
            GetDlgItem(IDC_CONFIGURE).EnableWindow(_VSTiPlugIns[VSTiIndex].HasEditor);
    }

    // Sample Rate
    {
        GetDlgItem(IDC_SAMPLERATE).EnableWindow((PlayerType != PlayerType::EmuDeMIDI) || !_IsRunning);
    }

    // Looping

    // MIDI Flavor and Effects
    {
        BOOL Enable = (PlayerType == PlayerType::VSTi) || (PlayerType == PlayerType::FluidSynth) || (PlayerType == PlayerType::BASSMIDI) || (PlayerType == PlayerType::SecretSauce);

        GetDlgItem(IDC_MIDI_FLAVOR_TEXT).EnableWindow(Enable);
        GetDlgItem(IDC_MIDI_FLAVOR).EnableWindow(Enable);

        GetDlgItem(IDC_MIDI_EFFECTS).EnableWindow(Enable);
    }

    // FluidSynth
    {
        BOOL Enable = (PlayerType == PlayerType::FluidSynth);

        GetDlgItem(IDC_FLUIDSYNTH_INTERPOLATION_TEXT).EnableWindow(Enable);
        GetDlgItem(IDC_FLUIDSYNTH_INTERPOLATION).EnableWindow(Enable);
    }

    // BASS MIDI resampling mode and cache status
    {
        BOOL Enable = (PlayerType == PlayerType::BASSMIDI);

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
        BOOL Enable = (PlayerType == PlayerType::SuperMunt);

        GetDlgItem(IDC_MUNT_GM_TEXT).EnableWindow(Enable);
        GetDlgItem(IDC_MUNT_GM_SET) .EnableWindow(Enable);

        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(Enable ? SW_SHOW : SW_HIDE);
    }

    // Nuke
    {
        GetDlgItem(IDC_NUKE_PRESET_TEXT).EnableWindow(PlayerType == PlayerType::Nuke);
        GetDlgItem(IDC_NUKE_PRESET)     .EnableWindow(PlayerType == PlayerType::Nuke);
        GetDlgItem(IDC_NUKE_PANNING)    .EnableWindow(PlayerType == PlayerType::Nuke);
    }

    // ADL
    {
        GetDlgItem(IDC_ADL_BANK_TEXT).EnableWindow(PlayerType == PlayerType::ADL);
        GetDlgItem(IDC_ADL_BANK)     .EnableWindow(PlayerType == PlayerType::ADL);
    }

    // Nuke and ADL
    {
        GetDlgItem(IDC_ADL_CHIPS_TEXT).EnableWindow(PlayerType == PlayerType::ADL || PlayerType == PlayerType::Nuke);
        GetDlgItem(IDC_ADL_CHIPS)     .EnableWindow(PlayerType == PlayerType::ADL || PlayerType == PlayerType::Nuke);
        GetDlgItem(IDC_ADL_PANNING)   .EnableWindow(PlayerType == PlayerType::ADL || PlayerType == PlayerType::Nuke);
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

#pragma region VSTi

/// <summary>
/// Gets all the VTSi plugins if a root directory has been specified.
/// </summary>
void PreferencesRootPage::GetVSTiPlugins(const char * pathName, puFindFile findFile)
{
    pfc::string8 DirectoryPath;

    if (findFile == nullptr)
    {
        _VSTiPlugIns.set_size(0);

        AdvCfgVSTiPluginDirectoryPath.get(DirectoryPath);

        if (DirectoryPath.is_empty())
            return;

        console::print("Enumerating VST instruments...");

        DirectoryPath = pfc::io::path::combine(DirectoryPath, "*.*");

        pathName = DirectoryPath;

        findFile = ::uFindFirstFile(DirectoryPath);
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
            PathName = pfc::io::path::combine(PathName, "*.*");

            puFindFile FindFile = ::uFindFirstFile(PathName);

            if (FindFile)
                GetVSTiPlugins(PathName, FindFile);
        }
        else
        {
            if ((PathName.length() < 5) || (pfc::stricmp_ascii(PathName.get_ptr() + PathName.length() - 4, ".dll") != 0))
                continue;

            // Examine all DLL files.
            if (findFile->GetFileSize() != 0)
            {
                console::print("Examining \"", PathName, "\"...");

                VSTiPlayer Player;

                if (Player.LoadVST(PathName))
                {
                    VSTiPlugin Plugin;

                    Plugin.PathName = PathName;

                    pfc::string8 VendorName;

                    Player.GetVendorName(VendorName);

                    pfc::string8 ProductName;

                    Player.GetProductName(ProductName);

                    // Get the plugin name.
                    {
                        Plugin.Name = "VSTi ";

                        if (VendorName.length() || ProductName.length())
                        {
                            if ((VendorName.length() == 0) || ((ProductName.length() >= VendorName.length()) && (::strncmp(VendorName.c_str(), ProductName.c_str(), VendorName.length()) == 0)))
                            {
                                Plugin.Name.add_string(ProductName);
                            }
                            else
                            {
                                Plugin.Name.add_string(VendorName);

                                if (ProductName.length())
                                    Plugin.Name.add_string(' ' + ProductName);
                            }
                        }
                        else
                            Plugin.Name = findFile->GetFileName();
                    }

                    Plugin.Id = Player.GetUniqueID();
                    Plugin.HasEditor = Player.HasEditor();

                    _VSTiPlugIns.append_single(Plugin);
                }
            }
        }
    }
    while (findFile->FindNext());

    delete findFile;
}

#pragma endregion

#pragma region Secret Sauce

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
};

/// <summary>
/// Is a compatible SecretSauce DLL available?
/// </summary>
bool PreferencesRootPage::HasSecretSauce() noexcept
{
    FILE * fp = nullptr;

    {
        pfc::string8 PathName;

        AdvCfgSecretSauceDirectoryPath.get(PathName);

        if (PathName.is_empty())
            return false;

        pfc::string8 FilePath = pfc::io::path::combine(PathName, _DLLFileName);

        pfc::stringcvt::string_os_from_utf8 FilePathW(FilePath);

        ::_wfopen_s(&fp, FilePathW, L"rb");
    }

    bool rc = false;

    if (fp)
    {
        ::fseek(fp, 0, SEEK_END);

        size_t FileSize = (size_t) ::ftell(fp);

        for (const auto & Iter : SecretSauceInfos)
        {
            if (Iter.FileSize == FileSize)
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

                    if (Iter.Hash == Hash)
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

                if (CfgVSTiFilePath.get() != _VSTiPlugIns[VSTiIndex].PathName)
                    return true;

                t_uint32 Id = _VSTiPlugIns[VSTiIndex].Id;

                if (_VSTiConfig.size() != CfgVSTiConfig[Id].size())
                    return true;

                if ((_VSTiConfig.size() != 0) && (::memcmp(_VSTiConfig.data(), &CfgVSTiConfig[Id][0], _VSTiConfig.size()) != 0))
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
