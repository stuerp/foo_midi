
/** $VER: PreferencesRoot.cpp (2023.07.23) **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 5045 ALL_CPPCORECHECK_WARNINGS)

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

#include <pfc/pathUtils.h>

#include "resource.h"

#include "Configuration.h"
#include "MIDIPreset.h"

#include "ADLPlayer.h"
#include "BMPlayer.h"
#include "NukePlayer.h"
#include "VSTiPlayer.h"

#pragma hdrstop

extern char _DLLFileName[];
extern volatile int _IsRunning;

static const GUID GUIDCfgSampleRateHistory = { 0x408aa155, 0x4c42, 0x42b5, { 0x8c, 0x3e, 0xd1, 0xc, 0x35, 0xdd, 0x5e, 0xf1 } };

static const int SampleRates[] = { 8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 49716, 64000, 88200, 96000 };

static cfg_dropdown_history CfgSampleRateHistory(GUIDCfgSampleRateHistory, 16);

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
/// Implements a preferences page.
/// </summary>
#pragma warning(disable: 4820)
class PreferencesRootPage : public CDialogImpl<PreferencesRootPage>, public preferences_page_instance
{
public:
    PreferencesRootPage(preferences_page_callback::ptr callback) noexcept : _IsBusy(false), _Callback(callback) { }

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

        #pragma region("Output")
        COMMAND_HANDLER_EX(IDC_PLAYER_TYPE, CBN_SELCHANGE, OnPlayerTypeChange)

        COMMAND_HANDLER_EX(IDC_CONFIGURE, BN_CLICKED, OnButtonConfig)

        DROPDOWN_HISTORY_HANDLER(IDC_SAMPLERATE, CfgSampleRateHistory)
        COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_EDITCHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_SELCHANGE, OnSelectionChange)
        #pragma endregion

        #pragma region("Looping")
        COMMAND_HANDLER_EX(IDC_LOOP_PLAYBACK, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_LOOP_OTHER, CBN_SELCHANGE, OnSelectionChange)
        #pragma endregion

        #pragma region("MIDI")
        COMMAND_HANDLER_EX(IDC_MIDI_FLAVOR, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_MIDI_EFFECTS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_MIDI_USE_SUPER_MUNT, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_MIDI_USE_VSTI_WITH_XG, BN_CLICKED, OnButtonClick)
        #pragma endregion

        #pragma region("Miscellaneous")
        COMMAND_HANDLER_EX(IDC_EMIDI_EXCLUSION, BN_CLICKED, OnButtonClick)

        COMMAND_HANDLER_EX(IDC_FILTER_INSTRUMENTS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_FILTER_BANKS, BN_CLICKED, OnButtonClick)

        COMMAND_HANDLER_EX(IDC_RPGMAKER_LOOPS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_XMI_LOOPS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_TOUHOU_LOOPS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_FF7_LOOPS, BN_CLICKED, OnButtonClick)
        #pragma endregion

        #pragma region("SoundFont")
        COMMAND_HANDLER_EX(IDC_RESAMPLING_MODE, CBN_SELCHANGE, OnSelectionChange)
        #pragma endregion

        #pragma region("Munt")
        COMMAND_HANDLER_EX(IDC_MUNT_GM_SET, CBN_SELCHANGE, OnSelectionChange)
        #pragma endregion

        #pragma region("Nuke")
        COMMAND_HANDLER_EX(IDC_NUKE_PRESET, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_NUKE_PANNING, BN_CLICKED, OnButtonClick)
        #pragma endregion

        #pragma region("ADL")
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

    struct BuiltInPlayer
    {
        const char * Name;
        int Type;
        int PlayerTypeAlternate;
        bool (*IsPresent)(PreferencesRootPage *);
    };

#ifdef DXISUPPORT
    pfc::array_t<CLSID> dxi_plugins;
#endif

#pragma region("VSTi")
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

#pragma region("BASS MIDI")
    pfc::string8 _CacheStatusText;
    pfc::string8 _CacheStatusTextCurrent;
#pragma endregion

#pragma region("ADL")
    struct adl_bank
    {
        int number;
        const char * name;

        adl_bank() : number(-1), name("")
        {
        }

        adl_bank(const adl_bank & b) : number(b.number), name(b.name)
        {
        }

        adl_bank(int _number, const char * _name) : number(_number), name(_name)
        {
        }

        adl_bank & operator=(const adl_bank & b)
        {
            number = b.number;
            name = b.name;

            return *this;
        }

        bool operator==(const adl_bank & b) const
        {
            return number == b.number;
        }

        bool operator<(const adl_bank & b) const
        {
            int c = ::stricmp_utf8(name, b.name);

            if (c)
                return c < 0;

            return 0;
        }

        bool operator>(const adl_bank & b) const
        {
            int c = ::stricmp_utf8(name, b.name);

            if (c == 0)
                return 0;

            return c > 0;
        }

        bool operator!=(const adl_bank & b) const
        {
            return !operator==(b);
        }
    };

    pfc::list_t<adl_bank> _ADLBanks;
#pragma endregion

#pragma region("Secret Sauce")
    bool _HasSecretSauce;
    static bool HasSecretSauce();
#pragma endregion

    static bool IsPluginAlwaysPresent(PreferencesRootPage *)
    {
        return true;
    }

    static bool IsPluginNeverPresent(PreferencesRootPage *)
    {
        return false;
    }

    static bool IsSecretSaucePresent(PreferencesRootPage * p)
    {
        return p->_HasSecretSauce;
    }

    int _PlayerPresentCount;

    static const BuiltInPlayer BuiltInPlayers[];

    std::map<int, int> _PlayerTypeToPlayerIndex;
    std::map<int, int> _PlayerPresentMap;
    std::map<int, int> _PlayerIndexToPlayerType;

    const preferences_page_callback::ptr _Callback;

    fb2k::CCoreDarkModeHooks _DarkModeHooks;
};

/// <summary>
/// Contains all the plugins that are build into the component.
/// </summary>
const PreferencesRootPage::BuiltInPlayer PreferencesRootPage::BuiltInPlayers[] =
{
    { "Emu de MIDI",    PlayerTypeEmuDeMIDI,      -1, IsPluginAlwaysPresent },

#ifdef FLUIDSYNTHSUPPORT
    { "FluidSynth",     FluidSynthPluginId,       -1, IsPluginAlwaysPresent },
#endif

    { "BASSMIDI",       PlayerTypeBASSMIDI, PlayerTypeFluidSynth, IsPluginAlwaysPresent },

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
    #pragma region("Output")
    {
        int PlayerType = -1;

        int SelectedItem = (int) SendDlgItemMessage(IDC_PLAYER_TYPE, CB_GETCURSEL);

        {
            if ((SelectedItem >= _PlayerPresentCount) && (SelectedItem < (int)(_PlayerPresentCount + _VSTiPlugIns.get_count())))
                PlayerType = PlayerTypeVSTi;
        #ifdef DXISUPPORT
            else
            if (plugin_selected >= plugins_reported + _VSTiPlugins.get_count())
                PlayerType = PlayerTypeDirectX;
        #endif
            else
                PlayerType = _PlayerIndexToPlayerType[SelectedItem];

            CfgPlayerType = PlayerType;
        }

        {
            if (PlayerType == PlayerTypeVSTi)
            {
                CfgVSTiFilePath = _VSTiPlugIns[(size_t)(SelectedItem - _PlayerPresentCount)].PathName.c_str();

                CfgVSTiConfig[_VSTiPlugIns[(size_t)(SelectedItem - _PlayerPresentCount)].Id] = _VSTiConfig;
            }
            else
                CfgVSTiFilePath = "";
        }

        #ifdef DXISUPPORT
        {
            if (PlayerType == PlayerTypeDirectX)
            {
                cfg_dxi_plugin = dxi_plugins[plugin_selected - _VSTiPlugins.get_count() - plugins_reported];
            }
        }
        #endif
    }

    {
        int t = (int) GetDlgItemInt(IDC_SAMPLERATE, NULL, FALSE);

        if (t < 6000)
            t = 6000;
        else
        if (t > 192000)
            t = 192000;

        SetDlgItemInt(IDC_SAMPLERATE, (UINT)t, FALSE);

        {
            char Text[16];

            _itoa_s(t, Text, _countof(Text), 10);

            CfgSampleRateHistory.add_item(Text);
            CfgSampleRate = t;
        }
    }
    #pragma endregion

    #pragma region("Looping")
    {
        CfgLoopTypePlayback = (t_int32) SendDlgItemMessage(IDC_LOOP_PLAYBACK, CB_GETCURSEL);
        CfgLoopTypeOther = (t_int32) SendDlgItemMessage(IDC_LOOP_OTHER, CB_GETCURSEL);
    }
    #pragma endregion

    #pragma region("MIDI Flavor")
    CfgMIDIFlavor = (t_int32) SendDlgItemMessage(IDC_MIDI_FLAVOR, CB_GETCURSEL);
    CfgUseMIDIEffects = (t_int32) !SendDlgItemMessage(IDC_MIDI_EFFECTS, BM_GETCHECK);
    CfgUseSuperMuntWithMT32 = (t_int32) SendDlgItemMessage(IDC_MIDI_USE_SUPER_MUNT, BM_GETCHECK);
    CfgUseVSTiWithXG = (t_int32) SendDlgItemMessage(IDC_MIDI_USE_VSTI_WITH_XG, BM_GETCHECK);
    #pragma endregion

    #pragma region("Miscellaneous")
    {
        CfgEmuDeMIDIExclusion = (t_int32) SendDlgItemMessage(IDC_EMIDI_EXCLUSION, BM_GETCHECK);

        CfgFilterInstruments = (t_int32) SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_GETCHECK);
        CfgFilterBanks = (t_int32) SendDlgItemMessage(IDC_FILTER_BANKS, BM_GETCHECK);

        CfgDetectTouhouLoops = (t_int32) SendDlgItemMessage(IDC_TOUHOU_LOOPS, BM_GETCHECK);
        CfgDetectRPGMakerLoops = (t_int32) SendDlgItemMessage(IDC_RPGMAKER_LOOPS, BM_GETCHECK);
        CfgDetectXMILoops = (t_int32) SendDlgItemMessage(IDC_XMI_LOOPS, BM_GETCHECK);
        CfgDetectFF7Loops = (t_int32) SendDlgItemMessage(IDC_FF7_LOOPS, BM_GETCHECK);

    }
    #pragma endregion

    #pragma region("SoundFont")
    {
        CfgBASSMIDIInterpolationMode = (t_int32) SendDlgItemMessage(IDC_RESAMPLING_MODE, CB_GETCURSEL);
    }
    #pragma endregion

    #pragma region("Munt")
    {
        CfgMuntGMSet = (t_int32) SendDlgItemMessage(IDC_MUNT_GM_SET, CB_GETCURSEL);
    }
    #pragma endregion

    #pragma region("Nuke")
    {
        size_t PresetIndex = (size_t) SendDlgItemMessage(IDC_NUKE_PRESET, CB_GETCURSEL);

        uint32_t Synth;
        uint32_t Bank;

        NukePlayer::GetPreset(PresetIndex, Synth, Bank);

        CfgNukeSynthesizer = (t_int32) Synth;
        CfgNukeBank = (t_int32) Bank;
        CfgNukePanning = (t_int32) SendDlgItemMessage(IDC_NUKE_PANNING, BM_GETCHECK);
    }
    #pragma endregion

    #pragma region("ADL")
    {
        {
            int t = (int) SendDlgItemMessage(IDC_ADL_BANK, CB_GETCURSEL);

            if (t < 0 || t >= (int)_ADLBanks.get_count())
                t = 0;

            CfgADLBank = _ADLBanks[(size_t)t].number;
        }

        {
            int t = (int) GetDlgItemInt(IDC_ADL_CHIPS, NULL, FALSE);

            if (t < 1)
                t = 1;
            else
            if (t > 100)
                t = 100;

            SetDlgItemInt(IDC_ADL_CHIPS, (UINT)t, FALSE);

            CfgADLChipCount = t;
        }

        CfgADLPanning = (t_int32) SendDlgItemMessage(IDC_ADL_PANNING, BM_GETCHECK);
    }
    #pragma endregion

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

    OnChanged();
}

/// <summary>
/// Resets the dialog state to default.
/// </summary>
void PreferencesRootPage::reset()
{
    int PlayerType = DefaultPlayerType;

    int PlayerIndex = _PlayerTypeToPlayerIndex[PlayerType];
    int SelectedItem = _PlayerPresentMap[PlayerIndex];

    {
        bool Enable = (PlayerType == PlayerTypeFluidSynth) || (PlayerType == PlayerTypeBASSMIDI);

        const int ControlId[] =
        {
            IDC_RESAMPLING_TEXT, IDC_RESAMPLING_MODE,
            IDC_CACHED_TEXT, IDC_CACHED
        };

        for (size_t i = 0; i < _countof(ControlId); ++i)
            GetDlgItem(ControlId[i]).EnableWindow(Enable);
    }

    if (PlayerType != PlayerTypeADL)
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

    GetDlgItem(IDC_CONFIGURE).EnableWindow(FALSE);

    {
        BOOL Enable = (PlayerType == PlayerTypeSuperMunt);

        GetDlgItem(IDC_MUNT_GM_TEXT).EnableWindow(Enable);
        GetDlgItem(IDC_MUNT_GM_SET).EnableWindow(Enable);

        GetDlgItem(IDC_MUNT_WARNING).ShowWindow((PlayerType == PlayerTypeSuperMunt) ? SW_SHOW : SW_HIDE);
    }

    {
        BOOL Enable = (PlayerType == PlayerTypeNuke);

        GetDlgItem(IDC_NUKE_PRESET_TEXT).EnableWindow(Enable);
        GetDlgItem(IDC_NUKE_PRESET).EnableWindow(Enable);
        GetDlgItem(IDC_NUKE_PANNING).EnableWindow(Enable);
    }

    {
        bool Enable = (PlayerType == PlayerTypeVSTi) || (PlayerType == PlayerTypeFluidSynth) || (PlayerType == PlayerTypeBASSMIDI) || (PlayerType == PlayerTypeSecretSauce);

        GetDlgItem(IDC_MIDI_FLAVOR_TEXT).EnableWindow(Enable);
        GetDlgItem(IDC_MIDI_FLAVOR).EnableWindow(Enable);
        GetDlgItem(IDC_MIDI_EFFECTS).EnableWindow(Enable);
    }

    if ((PlayerType == PlayerTypeEmuDeMIDI) && _IsRunning)
        GetDlgItem(IDC_SAMPLERATE).EnableWindow(FALSE);

    #pragma region("Output")
    SendDlgItemMessage(IDC_PLAYER_TYPE, CB_SETCURSEL, (WPARAM)SelectedItem);
    SetDlgItemInt(IDC_SAMPLERATE, DefaultSampleRate, FALSE);
    #pragma endregion

    #pragma region("Looping")
    SendDlgItemMessage(IDC_LOOP_PLAYBACK, CB_SETCURSEL, DefaultPlaybackLoopType);
    SendDlgItemMessage(IDC_LOOP_OTHER, CB_SETCURSEL, DefaultOtherLoopType);
    #pragma endregion

    #pragma region("MIDI")
    SendDlgItemMessage(IDC_MIDI_FLAVOR, CB_SETCURSEL, DefaultMIDIFlavor);
    SendDlgItemMessage(IDC_MIDI_EFFECTS, BM_SETCHECK, !DefaultUseMIDIEffects);
    SendDlgItemMessage(IDC_MIDI_USE_SUPER_MUNT, BM_SETCHECK, DefaultUseSuperMuntWithMT32);
    SendDlgItemMessage(IDC_MIDI_USE_VSTI_WITH_XG, BM_SETCHECK, DefaultUseSecretSauceWithXG);
    #pragma endregion

    #pragma region("Miscellaneous")
    SendDlgItemMessage(IDC_EMIDI_EXCLUSION, BM_SETCHECK, DefaultEmuDeMIDIExclusion);
    SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_SETCHECK, DefaultFilterInstruments);
    SendDlgItemMessage(IDC_FILTER_BANKS, BM_SETCHECK, DefaultFilterBanks);
    SendDlgItemMessage(IDC_TOUHOU_LOOPS, BM_SETCHECK, default_cfg_thloopz);
    SendDlgItemMessage(IDC_RPGMAKER_LOOPS, BM_SETCHECK, default_cfg_rpgmloopz);
    SendDlgItemMessage(IDC_XMI_LOOPS, BM_SETCHECK, default_cfg_xmiloopz);
    SendDlgItemMessage(IDC_FF7_LOOPS, BM_SETCHECK, default_cfg_ff7loopz);
    #pragma endregion

    #pragma region("SoundFont")
#ifdef FLUIDSYNTHSUPPORT
    SendDlgItemMessage(IDC_RESAMPLING, CB_SETCURSEL, 2);
#else
    SendDlgItemMessage(IDC_RESAMPLING_MODE, CB_SETCURSEL, DefaultBASSMIDIInterpolationMode);
#endif
    #pragma endregion

    #pragma region("Munt")
    SendDlgItemMessage(IDC_NUKE_PRESET, CB_SETCURSEL, (WPARAM)DefaultGMSet);
    #pragma endregion

    #pragma region("Nuke")
    SendDlgItemMessage(IDC_NUKE_PRESET, CB_SETCURSEL, (WPARAM)NukePlayer::GetPresetIndex(DefaultNukeSynth, DefaultNukeBank));
    SendDlgItemMessage(IDC_NUKE_PANNING, BM_SETCHECK, DefaultNukePanning);
    #pragma endregion

    #pragma region("ADL")
    {
        size_t BankIndex = 0;

        for (size_t i = 0; i < _ADLBanks.get_count(); ++i)
        {
            if (_ADLBanks[i].number == DefaultADLBank)
            {
                BankIndex = i;
                break;
            }
        }

        SendDlgItemMessage(IDC_ADL_BANK, CB_SETCURSEL, (WPARAM)BankIndex);
        SetDlgItemInt(IDC_ADL_CHIPS, DefaultADLChipCount, 0);
        SendDlgItemMessage(IDC_ADL_PANNING, BM_SETCHECK, DefaultADLPanning);
    }
    #pragma endregion

    _VSTiConfig.resize(0);

    UpdateDialog();

    OnChanged();
}
#pragma endregion

#pragma region("CDialogImpl")
/// <summary>
/// Initializes the dialog.
/// </summary>
BOOL PreferencesRootPage::OnInitDialog(CWindow, LPARAM)
{
    int PlayerType = CfgPlayerType;

    _PlayerTypeToPlayerIndex[PlayerType] = -1;
    _PlayerIndexToPlayerType[-1] = -1;

    for (size_t i = 0; i < _countof(BuiltInPlayers); ++i)
    {
        _PlayerTypeToPlayerIndex[BuiltInPlayers[i].Type] = (int)i;

        if (BuiltInPlayers[i].PlayerTypeAlternate >= 0)
            _PlayerTypeToPlayerIndex[BuiltInPlayers[i].PlayerTypeAlternate] = (int)i;
    }

    int PlayerIndex = -1;

    if ((PlayerType != PlayerTypeVSTi) && (PlayerType != PlayerTypeDirectX))
    {
        PlayerIndex = _PlayerTypeToPlayerIndex[PlayerType];

        if ((PlayerIndex < 0) || !BuiltInPlayers[PlayerIndex].IsPresent(this))
        {
            PlayerType  = DefaultPlayerType;
            PlayerIndex = _PlayerTypeToPlayerIndex[DefaultPlayerType];
        }
        else
        if (BuiltInPlayers[PlayerIndex].PlayerTypeAlternate == PlayerType)
            PlayerType = BuiltInPlayers[PlayerIndex].Type;
    }

    size_t VSTiPluginIndex = (size_t)~0;

    {
        CWindow w = GetDlgItem(IDC_PLAYER_TYPE);

        {
            _PlayerPresentCount = 0;

            _PlayerPresentMap[PlayerType] = -1;

            for (size_t i = 0; i < _countof(BuiltInPlayers); ++i)
            {
                const BuiltInPlayer& bip = BuiltInPlayers[i];

                if (bip.IsPresent(this))
                {
                    _PlayerPresentMap[bip.Type] = _PlayerPresentCount;

                    if (bip.PlayerTypeAlternate >= 0)
                        _PlayerPresentMap[bip.PlayerTypeAlternate] = _PlayerPresentCount;

                    _PlayerIndexToPlayerType[_PlayerPresentCount] = bip.Type;
                    _PlayerPresentCount++;

                    ::uSendMessageText(w, CB_ADDSTRING, 0, bip.Name);
                }
            }
        }
    }

 #pragma region("VSTi")
    {
        GetVSTiPlugins();

        size_t VSTiCount = _VSTiPlugIns.get_size();

        if (VSTiCount > 0)
        {
            CWindow w = GetDlgItem(IDC_PLAYER_TYPE);

            console::print("Found ",pfc::format_int((t_int64)_VSTiPlugIns.get_size()), " VST instruments.");

            for (size_t i = 0, j = VSTiCount; i < j; ++i)
            {
                _PlayerIndexToPlayerType[_PlayerPresentCount + (int)i] = 1;

                ::uSendMessageText(w, CB_ADDSTRING, 0, _VSTiPlugIns[i].Name.c_str());

                if ((PlayerType == PlayerTypeVSTi) && (::stricmp_utf8(_VSTiPlugIns[i].PathName.c_str(), CfgVSTiFilePath) == 0))
                    VSTiPluginIndex = i;
            }
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

    if ((PlayerType == PlayerTypeEmuDeMIDI) && _IsRunning)
        GetDlgItem(IDC_SAMPLERATE).EnableWindow(FALSE);

    if ((PlayerType == PlayerTypeVSTi) && (VSTiPluginIndex == ~0))
    {
        PlayerType = DefaultPlayerType;
        PlayerIndex = _PlayerTypeToPlayerIndex[DefaultPlayerType];
    }

    {
        bool Enable = (PlayerType == PlayerTypeFluidSynth) || (PlayerType == PlayerTypeBASSMIDI);

        const int ControlId[] =
        {
            IDC_RESAMPLING_TEXT, IDC_RESAMPLING_MODE,
            IDC_CACHED_TEXT, IDC_CACHED
        };

        for (size_t i = 0; i < _countof(ControlId); ++i)
            GetDlgItem(ControlId[i]).EnableWindow(Enable);
    }

    {
        BOOL Enable = (PlayerType == PlayerTypeSuperMunt);

        GetDlgItem(IDC_MUNT_GM_TEXT).EnableWindow(Enable);
        GetDlgItem(IDC_MUNT_GM_SET).EnableWindow(Enable);

        GetDlgItem(IDC_MUNT_WARNING).ShowWindow((PlayerType == PlayerTypeSuperMunt) ? SW_SHOW : SW_HIDE);
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
        GetDlgItem(IDC_CONFIGURE).EnableWindow(FALSE);
    }
    else
    {
        GetDlgItem(IDC_CONFIGURE).EnableWindow(_VSTiPlugIns[VSTiPluginIndex].HasEditor);
        _VSTiConfig = CfgVSTiConfig[_VSTiPlugIns[VSTiPluginIndex].Id];
    }

    if (PlayerType != PlayerTypeNuke)
    {
        GetDlgItem(IDC_NUKE_PRESET_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_NUKE_PRESET).EnableWindow(FALSE);
        GetDlgItem(IDC_NUKE_PANNING).EnableWindow(FALSE);
    }

    {
        bool Enabled = ((PlayerType == PlayerTypeVSTi) || (PlayerType == PlayerTypeFluidSynth) || (PlayerType == PlayerTypeBASSMIDI) || (PlayerType == PlayerTypeSecretSauce));

        GetDlgItem(IDC_MIDI_FLAVOR_TEXT).EnableWindow(Enabled);
        GetDlgItem(IDC_MIDI_FLAVOR).EnableWindow(Enabled);
        GetDlgItem(IDC_MIDI_EFFECTS).EnableWindow(Enabled);
   }

    // Set the selected player.
    {
        CWindow w = GetDlgItem(IDC_PLAYER_TYPE);

        int SelectedIndex = -1;

        if (PlayerType == PlayerTypeVSTi)
            SelectedIndex = _PlayerPresentCount + (int)VSTiPluginIndex;
    #ifdef DXISUPPORT
        else
        if (PlayerType == PlayerTypeDirectX)
        {
            if (dxi_selected != ~0)
                SelectedIndex = _PlayerPresentCount + vsti_count + dxi_selected;
            else
                SelectedIndex = 0;
        }
    #endif
        else
            SelectedIndex = _PlayerPresentMap[PlayerType];

        ::SendMessage(w, CB_SETCURSEL, (WPARAM)SelectedIndex, (LPARAM)0);
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
            "Never loop, 1s decay time",
            "Loop and fade when detected",
            "Loop and fade always",
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
 
            ::SendMessage(w1, CB_SETCURSEL, (WPARAM) CfgLoopTypePlayback, 0);
            ::SendMessage(w2, CB_SETCURSEL, (WPARAM) CfgLoopTypeOther, 0);
        }
    }

    SendDlgItemMessage(IDC_EMIDI_EXCLUSION, BM_SETCHECK, (WPARAM) CfgEmuDeMIDIExclusion);

    SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_SETCHECK, (WPARAM) CfgFilterInstruments);
    SendDlgItemMessage(IDC_FILTER_BANKS, BM_SETCHECK, (WPARAM) CfgFilterBanks);

    SendDlgItemMessage(IDC_TOUHOU_LOOPS, BM_SETCHECK, (WPARAM) CfgDetectTouhouLoops);
    SendDlgItemMessage(IDC_RPGMAKER_LOOPS, BM_SETCHECK, (WPARAM) CfgDetectRPGMakerLoops);
    SendDlgItemMessage(IDC_XMI_LOOPS, BM_SETCHECK, (WPARAM) CfgDetectXMILoops);
    SendDlgItemMessage(IDC_FF7_LOOPS, BM_SETCHECK, (WPARAM) CfgDetectFF7Loops);

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

    {
        auto w = GetDlgItem(IDC_RESAMPLING_MODE);

        ::uSendMessageText(w, CB_ADDSTRING, 0, "Linear interpolation");
        ::uSendMessageText(w, CB_ADDSTRING, 0, "8pt Sinc interpolation");
        ::uSendMessageText(w, CB_ADDSTRING, 0, "16pt Sinc interpolation");

        ::SendMessage(w, CB_SETCURSEL, (WPARAM)CfgBASSMIDIInterpolationMode, 0);
    }

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

    #pragma region("Munt")
    {
        auto w = GetDlgItem(IDC_MUNT_GM_SET);

        for (size_t i = 0; i < _countof(_MuntGMSets); ++i)
            ::uSendMessageText(w, CB_ADDSTRING, 0, _MuntGMSets[i]);

        ::SendMessage(w, CB_SETCURSEL, (WPARAM)CfgMuntGMSet, 0);
    }
    #pragma endregion

    #pragma region("Nuke")
    {
        auto w = GetDlgItem(IDC_NUKE_PRESET);

        size_t PresetNumber = 0;

        NukePlayer::EnumeratePresets([w, PresetNumber] (const pfc::string8 name, unsigned int synth, unsigned int bank) mutable noexcept
        {
            ::uSendMessageText(w, CB_ADDSTRING, 0, name.c_str());

            if ((synth == (unsigned int)CfgNukeSynthesizer) && (bank == (unsigned int)CfgNukeBank))
                ::SendMessage(w, CB_SETCURSEL, PresetNumber, 0);

            PresetNumber++;
        });

        SendDlgItemMessage(IDC_NUKE_PANNING, BM_SETCHECK, (WPARAM)CfgNukePanning);
    }
    #pragma endregion

    #pragma region("MIDI")
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

        SendDlgItemMessage(IDC_MIDI_EFFECTS, BM_SETCHECK, !CfgUseMIDIEffects);
        SendDlgItemMessage(IDC_MIDI_USE_SUPER_MUNT, BM_SETCHECK, (WPARAM) CfgUseSuperMuntWithMT32);
        SendDlgItemMessage(IDC_MIDI_USE_VSTI_WITH_XG, BM_SETCHECK, (WPARAM) CfgUseVSTiWithXG);
    }
    #pragma endregion

    _HasSecretSauce = HasSecretSauce();

    SetTimer(ID_REFRESH, 20);

    _IsBusy = false;

    _DarkModeHooks.AddDialogWithControls(*this);

    UpdateDialog();

    return FALSE;
}

void PreferencesRootPage::OnEditChange(UINT, int, CWindow)
{
    OnChanged();
}

void PreferencesRootPage::OnSelectionChange(UINT, int, CWindow)
{
    OnChanged();
}

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

    if ((SelectedIndex >= _PlayerPresentCount) && SelectedIndex < (int) (_PlayerPresentCount + _VSTiPlugIns.get_count()))
    {
        _IsBusy = true;
        OnChanged();

        VSTiPlayer Player;

        if (Player.LoadVST(_VSTiPlugIns[(size_t)(SelectedIndex - _PlayerPresentCount)].PathName.c_str()))
        {
            if (_VSTiConfig.size() != 0)
                Player.SetChunk(&_VSTiConfig[0], _VSTiConfig.size());

            Player.DisplayEditorModal();
            Player.GetChunk(_VSTiConfig);
        }

        _IsBusy = false;
        OnChanged();
    }
}

/// <summary>
/// Updates the dialog controls when the player type changes.
/// </summary>
void PreferencesRootPage::OnPlayerTypeChange(UINT, int, CWindow w)
{
    int PlayerType = -1;

    // Player Type
    int SelectedIndex = (int) ::SendMessage(w, CB_GETCURSEL, 0, 0);

    {
        if ((SelectedIndex >= _PlayerPresentCount) && (SelectedIndex < (int)(_PlayerPresentCount + _VSTiPlugIns.get_count())))
        {
            PlayerType = PlayerTypeVSTi;
            _VSTiConfig = CfgVSTiConfig[_VSTiPlugIns[(size_t)(SelectedIndex - _PlayerPresentCount)].Id];
        }
    #ifdef DXISUPPORT
        else
        if (SelectedIndex >= plugins_reported + _VSTiPlugins.get_count())
            PlayerType = PlayerTypeDirectX;
    #endif
        else
            PlayerType = _PlayerIndexToPlayerType[SelectedIndex];
    }

    // Configure
    {
        bool Enable = (SelectedIndex >= _PlayerPresentCount) && (PlayerType < _PlayerPresentCount + (int)_VSTiPlugIns.get_count()) && _VSTiPlugIns[(size_t)(SelectedIndex - _PlayerPresentCount)].HasEditor;

        GetDlgItem(IDC_CONFIGURE).EnableWindow(Enable);
    }

    // Sample Rate
    {
        GetDlgItem(IDC_SAMPLERATE).EnableWindow(PlayerType || !_IsRunning);
    }

    // Looping

    // MIDI Flavor and Effects
    {
        BOOL Enable = (PlayerType == PlayerTypeVSTi) || (PlayerType == PlayerTypeFluidSynth) || (PlayerType == PlayerTypeBASSMIDI) || (PlayerType == PlayerTypeSecretSauce);

        GetDlgItem(IDC_MIDI_FLAVOR_TEXT).EnableWindow(Enable);
        GetDlgItem(IDC_MIDI_FLAVOR).EnableWindow(Enable);

        GetDlgItem(IDC_MIDI_EFFECTS).EnableWindow(Enable);
    }

    // SoundFont resampling mode and cache status
    {
        BOOL Enable = (PlayerType == PlayerTypeFluidSynth) || (PlayerType == PlayerTypeBASSMIDI);

        const int ControlId[] =
        {
            IDC_RESAMPLING_TEXT, IDC_RESAMPLING_MODE,
            IDC_CACHED_TEXT, IDC_CACHED
        };

        for (size_t i = 0; i < _countof(ControlId); ++i)
            GetDlgItem(ControlId[i]).EnableWindow(Enable);
    }

    // Munt
    {
        BOOL Enable = (PlayerType == PlayerTypeSuperMunt);

        GetDlgItem(IDC_MUNT_GM_TEXT).EnableWindow(Enable);
        GetDlgItem(IDC_MUNT_GM_SET).EnableWindow(Enable);

        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(Enable ? SW_SHOW : SW_HIDE);
    }

    // Nuke
    {
        GetDlgItem(IDC_NUKE_PRESET_TEXT).EnableWindow(PlayerType == PlayerTypeNuke);
        GetDlgItem(IDC_NUKE_PRESET).EnableWindow(PlayerType == PlayerTypeNuke);
        GetDlgItem(IDC_NUKE_PANNING).EnableWindow(PlayerType == PlayerTypeNuke);
    }

    // ADL
    {
        GetDlgItem(IDC_ADL_BANK_TEXT).EnableWindow(PlayerType == PlayerTypeADL);
        GetDlgItem(IDC_ADL_BANK).EnableWindow(PlayerType == PlayerTypeADL);
    }

    // Nuke and ADL
    {
        GetDlgItem(IDC_ADL_CHIPS_TEXT).EnableWindow(PlayerType == PlayerTypeADL || PlayerType == PlayerTypeNuke);
        GetDlgItem(IDC_ADL_CHIPS).EnableWindow(PlayerType == PlayerTypeADL || PlayerType == PlayerTypeNuke);
        GetDlgItem(IDC_ADL_PANNING).EnableWindow(PlayerType == PlayerTypeADL || PlayerType == PlayerTypeNuke);
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
    #pragma region("Output")
    if (GetDlgItemInt(IDC_SAMPLERATE, NULL, FALSE) != (UINT)CfgSampleRate)
        return true;
    #pragma endregion

    #pragma region("Looping")
    if (SendDlgItemMessage(IDC_LOOP_PLAYBACK, CB_GETCURSEL) != CfgLoopTypePlayback)
        return true;

    if (SendDlgItemMessage(IDC_LOOP_OTHER, CB_GETCURSEL) != CfgLoopTypeOther)
        return true;

    if (SendDlgItemMessage(IDC_TOUHOU_LOOPS, BM_GETCHECK) != CfgDetectTouhouLoops)
        return true;

    if (SendDlgItemMessage(IDC_RPGMAKER_LOOPS, BM_GETCHECK) != CfgDetectRPGMakerLoops)
        return true;

    if (SendDlgItemMessage(IDC_XMI_LOOPS, BM_GETCHECK) != CfgDetectXMILoops)
        return true;

    if (SendDlgItemMessage(IDC_FF7_LOOPS, BM_GETCHECK) != CfgDetectFF7Loops)
        return true;
    #pragma endregion

    #pragma region("MIDI")
    if (SendDlgItemMessage(IDC_MIDI_FLAVOR, CB_GETCURSEL) != CfgMIDIFlavor)
        return true;

    if (SendDlgItemMessage(IDC_MIDI_EFFECTS, BM_GETCHECK) != !CfgUseMIDIEffects)
        return true;

    if (SendDlgItemMessage(IDC_MIDI_USE_SUPER_MUNT, BM_GETCHECK) != CfgUseSuperMuntWithMT32)
        return true;

    if (SendDlgItemMessage(IDC_MIDI_USE_VSTI_WITH_XG, BM_GETCHECK) != CfgUseVSTiWithXG)
        return true;
    #pragma endregion

    #pragma region("Miscellaneous")
    if (SendDlgItemMessage(IDC_EMIDI_EXCLUSION, BM_GETCHECK) != CfgEmuDeMIDIExclusion)
        return true;

    if (SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_GETCHECK) != CfgFilterInstruments)
        return true;

    if (SendDlgItemMessage(IDC_FILTER_BANKS, BM_GETCHECK) != CfgFilterBanks)
        return true;
    #pragma endregion

    #pragma region("SoundFont")
    if (SendDlgItemMessage(IDC_RESAMPLING_MODE, CB_GETCURSEL) != CfgBASSMIDIInterpolationMode)
        return true;

#ifdef FLUIDSYNTHSUPPORT
    {
        int interp_method = SendDlgItemMessage(IDC_RESAMPLING, CB_GETCURSEL);

        if (interp_method == 2)
            interp_method = 4;
        else
        if (interp_method == 3)
            interp_method = 7;

        if (interp_method != Cfg_FluidSynthInterpolationMethod)
            return true;
    }
#endif
    #pragma endregion

    #pragma region("Munt")
    if (SendDlgItemMessage(IDC_MUNT_GM_SET, CB_GETCURSEL) != CfgMuntGMSet)
        return true;
    #pragma endregion

    #pragma region("Nuke")
    if (SendDlgItemMessage(IDC_NUKE_PANNING, BM_GETCHECK) != CfgNukePanning)
        return true;

    {
        size_t PresetNumber = (size_t)SendDlgItemMessage(IDC_NUKE_PRESET, CB_GETCURSEL);

        unsigned int Synth;
        unsigned int Bank;

        NukePlayer::GetPreset(PresetNumber, Synth, Bank);

        if (!(Synth == (unsigned int)CfgNukeSynthesizer && Bank == (unsigned int)CfgNukeBank))
            return true;
    }
    #pragma endregion

    #pragma region("ADL")
    {
        int SelectedIndex = (int)SendDlgItemMessage(IDC_ADL_BANK, CB_GETCURSEL);

        if ((SelectedIndex < 0) || (SelectedIndex >= (int)_ADLBanks.get_count()))
            SelectedIndex = 0;

        if (_ADLBanks[(t_size)SelectedIndex].number != (int)CfgADLBank)
            return true;
    }

    if (GetDlgItemInt(IDC_ADL_CHIPS, NULL, FALSE) != (UINT)CfgADLChipCount)
        return true;

    if (SendDlgItemMessage(IDC_ADL_PANNING, BM_GETCHECK) != CfgADLPanning)
        return true;
    #pragma endregion

    {
        int PlayerIndex = (int)SendDlgItemMessage(IDC_PLAYER_TYPE, CB_GETCURSEL);
        int PlayerType = -1;

        if ((PlayerIndex >= _PlayerPresentCount) && (PlayerIndex < _PlayerPresentCount + (int)_VSTiPlugIns.get_count()))
            PlayerType = PlayerTypeVSTi;
    #ifdef DXISUPPORT
        else
        if (PlayerIndex >= plugins_reported + _VSTiPlugins.get_count())
            PlayerType = PlayerTypeDirectX;
    #endif
        else
            PlayerType = _PlayerIndexToPlayerType[PlayerIndex];

        if (PlayerType != CfgPlayerType)
            return true;

        if (PlayerType == PlayerTypeVSTi)
        {
            if (CfgVSTiFilePath != _VSTiPlugIns[(size_t)(PlayerIndex - _PlayerPresentCount)].PathName)
                return true;

            t_uint32 Id = _VSTiPlugIns[(size_t)(PlayerIndex - _PlayerPresentCount)].Id;

            if (_VSTiConfig.size() != CfgVSTiConfig[Id].size())
                return true;

            if ((_VSTiConfig.size() != 0) && (::memcmp(&_VSTiConfig[0], &CfgVSTiConfig[Id][0], _VSTiConfig.size()) != 0))
                return true;
        }
    #ifdef DXISUPPORT
        else
        if (PlayerType == PlayerTypeDirectX)
        {
            if (dxi_plugins[PlayerIndex - _VSTiPlugins.get_count() - plugins_reported] != cfg_dxi_plugin.get_value())
                return true;
        }
    #endif
    }

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
    #pragma region("MIDI")
    pfc::string8 FilePath; AdvCfgVSTiXGPlugin.get(FilePath);

    GetDlgItem(IDC_MIDI_USE_VSTI_WITH_XG).EnableWindow(!FilePath.isEmpty());
    #pragma endregion
}
#pragma endregion

#pragma region("VSTi")
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

                    Plugin.Name = "VST ";
                    Plugin.PathName = PathName;

                    pfc::string8 VendorName;

                    Player.GetVendorName(VendorName);

                    pfc::string8 ProductName;

                    Player.GetProductName(ProductName);

                    // Fix the plugin name if necessary.
                    {
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
};

/// <summary>
/// Is a compatible SecretSauce DLL available?
/// </summary>
bool PreferencesRootPage::HasSecretSauce()
{
    FILE * fp = nullptr;

    {
        pfc::string8 PathName;

        AdvCfgSecretSauceDirectoryPath.get(PathName);

        if (PathName.is_empty())
            return false;

        pfc::string8 FilePath = pfc::io::path::combine(PathName, _DLLFileName);

        pfc::stringcvt::string_os_from_utf8 FilePathW(PathName);

        _tfopen_s(&fp, FilePathW, _T("rb"));
    }

    bool rc = false;

    if (fp)
    {
        ::fseek(fp, 0, SEEK_END);

        size_t FileSize = (size_t)::ftell(fp);

        for (size_t i = 0; i < _countof(SecretSauceInfos); ++i)
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

                    if (SecretSauceInfos[i].Hash == Hash)
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
