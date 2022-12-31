
/** $VER: Preferences.cpp (2022.12.31) **/

#pragma warning(disable: 5045)

#include <atlbase.h>
#include <atlapp.h>
#include <atlcrack.h>
#include <atlctrls.h>
#include <atlmisc.h>

#include <map>

#include <sdk/foobar2000.h>
#include <sdk/preferences_page.h>
#include <sdk/coreDarkMode.h>

#include <helpers/atl-misc.h>
#include <helpers/dropdown_helper.h>

#include "resource.h"

#include "Configuration.h"
#include "MIDIPreset.h"

#include "ADLPlayer.h"
#include "BMPlayer.h"
#include "VSTiPlayer.h"

#pragma warning(disable: 4820)

//#define DEBUG_DIALOG

#pragma hdrstop

extern char _DLLFileName[];
extern volatile int g_running;

static const GUID GUIDCfgSampleRateHistory = { 0x408aa155, 0x4c42, 0x42b5, { 0x8c, 0x3e, 0xd1, 0xc, 0x35, 0xdd, 0x5e, 0xf1 } };
static cfg_dropdown_history CfgSampleRateHistory(GUIDCfgSampleRateHistory, 16);

static const char * DefaultPathMessage = "Click to set.";

static const int SampleRates[] = { 8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 49716, 64000, 88200, 96000 };

#pragma region("MUNT")
const char * _MUNTGMSets[] =
{
    "Roland",
    "Sierra / King's Quest 6",
};
const size_t _MUNTGMSetCount = _countof(_MUNTGMSets);
#pragma endregion

#ifdef FLUIDSYNTHSUPPORT
static const char * interp_txt[] = { "None", "Linear", "Cubic", "7th Order Sinc" };
static int interp_method[] = { FLUID_INTERP_NONE, FLUID_INTERP_LINEAR, FLUID_INTERP_4THORDER, FLUID_INTERP_7THORDER };
enum
{
    interp_method_default = 2
};
#endif

class Preferences : public CDialogImpl<Preferences>, public preferences_page_instance
{
public:
    Preferences(preferences_page_callback::ptr callback) noexcept : m_callback(callback), _IsBusy(false) { }
    Preferences(const Preferences&) = delete;
    Preferences(const Preferences&&) = delete;
    Preferences& operator=(const Preferences&) = delete;
    Preferences& operator=(Preferences&&) = delete;
    virtual ~Preferences() { };

    t_uint32 get_state() override;
    void apply() override;
    void reset() override;

    // WTL message map
    BEGIN_MSG_MAP(Preferences)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_TIMER(OnTimer)

        COMMAND_HANDLER_EX(IDC_PLUGIN, CBN_SELCHANGE, OnPluginChange)

        DROPDOWN_HISTORY_HANDLER(IDC_SAMPLERATE, CfgSampleRateHistory)
        COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_EDITCHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_SELCHANGE, OnSelectionChange)

        COMMAND_HANDLER_EX(IDC_SOUNDFONT, EN_SETFOCUS, OnSetFocus)
        COMMAND_HANDLER_EX(IDC_RESAMPLING_MODE, CBN_SELCHANGE, OnSelectionChange)

        COMMAND_HANDLER_EX(IDC_MUNT, EN_SETFOCUS, OnSetFocus)

        COMMAND_HANDLER_EX(IDC_LOOP_PLAYBACK, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_LOOP_OTHER, CBN_SELCHANGE, OnSelectionChange)

        COMMAND_HANDLER_EX(IDC_RPGMLOOPZ, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_XMILOOPZ, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_FF7LOOPZ, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_EMIDI_EX, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_FILTER_INSTRUMENTS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_FILTER_BANKS, BN_CLICKED, OnButtonClick)

        COMMAND_HANDLER_EX(IDC_VST_PATH, EN_SETFOCUS, OnSetFocus)
        COMMAND_HANDLER_EX(IDC_VST_CONFIGURE, BN_CLICKED, OnButtonConfig)

        COMMAND_HANDLER_EX(IDC_ADL_BANK, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_ADL_CHIPS, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_ADL_CHIPS, CBN_EDITCHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_ADL_PANNING, BN_CLICKED, OnButtonClick)

        COMMAND_HANDLER_EX(IDC_MUNT_GM, CBN_SELCHANGE, OnSelectionChange)

        COMMAND_HANDLER_EX(IDC_MS_PRESET, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_MS_PANNING, BN_CLICKED, OnButtonClick)

        COMMAND_HANDLER_EX(IDC_MIDI_FLAVOR, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_MIDI_EFFECTS, BN_CLICKED, OnButtonClick)

#ifdef DEBUG_DIALOG
        MSG_WM_CTLCOLORDLG(OnCtlColorDlg)
#endif
    END_MSG_MAP()

    enum
    {
        IDD = IDD_CONFIG
    };

    const UINT_PTR ID_REFRESH = 1000;

private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnTimer(UINT_PTR);

    void OnEditChange(UINT, int, CWindow);
    void OnSelectionChange(UINT, int, CWindow);
    void OnPluginChange(UINT, int, CWindow);
    void OnButtonClick(UINT, int, CWindow);
    void OnButtonConfig(UINT, int, CWindow);
    void OnSetFocus(UINT, int, CWindow);
    bool HasChanged();
    void OnChanged();

#ifdef DEBUG_DIALOG
    /// <summary>
    /// Returns a brush that the system uses to draw the dialog background.
    /// </summary>
    HBRUSH OnCtlColorDlg(HDC, HWND)
    {
        return (HBRUSH)::GetStockObject(DKGRAY_BRUSH);
    }
#endif


private:
    const preferences_page_callback::ptr m_callback;

    bool _IsBusy;

    struct BuiltInPlugin
    {
        const char * Name;
        int Id;
        int plugin_number_alt;
        bool (*IsPresent)(Preferences *);
    };

#ifdef DXISUPPORT
    pfc::array_t<CLSID> dxi_plugins;
#endif

#pragma region("VSTi")
    void GetVSTiPlugins(const char * pathName = nullptr, puFindFile findFile = nullptr);

    struct VSTiPlugin
    {
        std::string Name;
        std::string PathName;
        uint32_t Id;
        bool HasEditor;
    };

    pfc::array_t<VSTiPlugin> _VSTiPlugIns;
    std::vector<uint8_t> _VSTiConfig;
#pragma endregion

#pragma region("SoundFont")
    pfc::string8 _SoundFontPath;
#pragma endregion

#pragma region("MUNT")
    pfc::string8 _MUNTPath;
#pragma endregion

#pragma region("Secret Sauce")
    bool _HasSecretSauce;
    static bool HasSecretSauce();
#pragma endregion

#ifdef BASSMIDISUPPORT
    pfc::string8_fast m_cached, m_cached_current;
#endif

    struct adl_bank
    {
        int number;
        const char * name;

        adl_bank()
            : number(-1), name("")
        {
        }
        adl_bank(const adl_bank & b)
            : number(b.number), name(b.name)
        {
        }
        adl_bank(int _number, const char * _name)
            : number(_number), name(_name)
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
            int c = stricmp_utf8(name, b.name);
            if (c) return c < 0;
            return 0;
        }
        bool operator>(const adl_bank & b) const
        {
            int c = stricmp_utf8(name, b.name);
            if (c) return c > 0;
            return 0;
        }
        bool operator!=(const adl_bank & b) const
        {
            return !operator==(b);
        }
    };

    pfc::list_t<adl_bank> _ADLBanks;

    static bool IsPluginAlwaysPresent(Preferences *)
    {
        return true;
    }

    static bool IsPluginNeverPresent(Preferences *)
    {
        return false;
    }

    static bool IsSecretSaucePresent(Preferences * p)
    {
        return p->_HasSecretSauce;
    }

    int _ReportedPlugInCount;

    static const BuiltInPlugin BuiltInPlugins[];

    std::map<int, int> _PluginIdToIndex;
    std::map<int, int> _PluginPresentMap;
    std::map<int, int> _PluginPresentReverseMap;

    fb2k::CCoreDarkModeHooks _DarkModeHooks;
};

/// <summary>
/// Contains all the plugins that are build into the component.
/// </summary>
const Preferences::BuiltInPlugin Preferences::BuiltInPlugins[] =
{
    { "Emu de MIDI",    EmuDeMIDIPlugInId,      -1, IsPluginAlwaysPresent },

#ifdef FLUIDSYNTHSUPPORT
    { "FluidSynth",
    #ifdef BASSMIDISUPPORT
          FluidSynthPluginId, -1,
    #else
          BASSMIDIPlugInId, FluidSynthPluginId,
    #endif
      IsPluginAlwaysPresent },
#endif

#ifdef BASSMIDISUPPORT
    { "BASSMIDI",
    #ifdef FLUIDSYNTHSUPPORT
          BASSMIDIPlugInId, -1,
    #else
          BASSMIDIPlugInId, FluidSynthPlugInId,
    #endif
      IsPluginAlwaysPresent },
#endif

    { "Super MUNT GM",  SuperMUNTPlugInId,      -1, IsPluginAlwaysPresent },

    { "libADLMIDI",     ADLPlugInId,            -1, IsPluginAlwaysPresent },
    { "libOPNMIDI",     OPNPlugInId,            -1, IsPluginAlwaysPresent },
    { "oplmidi",        OPLPlugInId,            -1, IsPluginNeverPresent },
    { "Nuclear Option", NuclearOptionPlugInId,  -1, IsPluginAlwaysPresent },
    { "Secret Sauce",   SecretSaucePlugInId,    -1, IsSecretSaucePresent }
};

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

    _HasSecretSauce = HasSecretSauce();

    int PlugInId = CfgPlugInId;

    _PluginIdToIndex[PlugInId] = -1;
    _PluginPresentReverseMap[-1] = -1;

    for (size_t i = 0; i < _countof(BuiltInPlugins); ++i)
    {
        _PluginIdToIndex[BuiltInPlugins[i].Id] = i;

        if (BuiltInPlugins[i].plugin_number_alt >= 0)
            _PluginIdToIndex[BuiltInPlugins[i].plugin_number_alt] = i;
    }

    int PlugInIndex = -1;

    if (PlugInId != VSTiPlugInId && PlugInId != UnknownPlugInId)
    {
        PlugInIndex = _PluginIdToIndex[PlugInId];

        if (PlugInIndex < 0 || !BuiltInPlugins[PlugInIndex].IsPresent(this))
        {
            PlugInId = DefaultPlugInId;
            PlugInIndex = _PluginIdToIndex[DefaultPlugInId];
        }
        else
        if (BuiltInPlugins[PlugInIndex].plugin_number_alt == PlugInId)
            PlugInId = BuiltInPlugins[PlugInIndex].Id;
    }

    CWindow w = GetDlgItem(IDC_PLUGIN);

    {
        _ReportedPlugInCount = 0;

        _PluginPresentMap[PlugInId] = -1;

        for (size_t i = 0; i < _countof(BuiltInPlugins); ++i)
        {
            const BuiltInPlugin& bip = BuiltInPlugins[i];

            if (bip.IsPresent(this))
            {
                _PluginPresentMap[bip.Id] = _ReportedPlugInCount;

                if (bip.plugin_number_alt >= 0)
                    _PluginPresentMap[bip.plugin_number_alt] = _ReportedPlugInCount;

                _PluginPresentReverseMap[_ReportedPlugInCount] = bip.Id;
                ++_ReportedPlugInCount;

                ::uSendMessageText(w, CB_ADDSTRING, 0, bip.Name);
            }
        }
    }

 #pragma region("VSTi")
    size_t SelectedVSTiPluginIndex = ~0;

    GetVSTiPlugins();

    size_t VSTiCount = _VSTiPlugIns.get_size();

    if (VSTiCount > 0)
    {
        console::formatter() << "Found " << pfc::format_int(_VSTiPlugIns.get_size()) << " VSTi plug-ins.";

        for (size_t i = 0, j = VSTiCount; i < j; ++i)
        {
            _PluginPresentReverseMap[_ReportedPlugInCount + i] = 1;

            ::uSendMessageText(w, CB_ADDSTRING, 0, _VSTiPlugIns[i].Name.c_str());

            if ((PlugInId == VSTiPlugInId) && (::stricmp_utf8(_VSTiPlugIns[i].PathName.c_str(), CfgVSTiPath) == 0))
                SelectedVSTiPluginIndex = i;
        }
    }
#pragma endregion

#pragma region("SoundFont")
    {
        _SoundFontPath = CfgSoundFontPath;

        const char * FileName;

        if (_SoundFontPath.is_empty())
            FileName = DefaultPathMessage;
        else
            FileName = _SoundFontPath.get_ptr() + _SoundFontPath.scan_filename();

        ::uSetDlgItemText(m_hWnd, IDC_SOUNDFONT, FileName);
    }
#pragma endregion

#pragma region("MUNT")
    {
        _MUNTPath = CfgMUNTPath;

        const char * FileName;

        if (_MUNTPath.is_empty())
            FileName = DefaultPathMessage;
        else
            FileName = _MUNTPath;

        ::uSetDlgItemText(m_hWnd, IDC_MUNT, FileName);
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

    if ((PlugInId == VSTiPlugInId) && (SelectedVSTiPluginIndex == ~0))
    {
        PlugInId = DefaultPlugInId;
        PlugInIndex = _PluginIdToIndex[DefaultPlugInId];
    }

    if ((PlugInId == EmuDeMIDIPlugInId) && g_running)
        GetDlgItem(IDC_SAMPLERATE).EnableWindow(FALSE);

    if ((PlugInId != FluidSynthPlugInId) && (PlugInId != BASSMIDIPlugInId))
    {
        GetDlgItem(IDC_SOUNDFONT_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_SOUNDFONT).EnableWindow(FALSE);
        GetDlgItem(IDC_RESAMPLING_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_RESAMPLING_MODE).EnableWindow(FALSE);
        GetDlgItem(IDC_CACHED_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_CACHED).EnableWindow(FALSE);
    }

    if (PlugInId != SuperMUNTPlugInId)
    {
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_SHOW);
    }

    if (PlugInId != ADLPlugInId)
    {
        GetDlgItem(IDC_ADL_BANK_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_BANK).EnableWindow(FALSE);
    }

    if (PlugInId != ADLPlugInId && PlugInId != OPNPlugInId)
    {
        GetDlgItem(IDC_ADL_CHIPS_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_CHIPS).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_PANNING).EnableWindow(FALSE);
    }

    if (PlugInId != VSTiPlugInId)
    {
        GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(FALSE);
    }
    else
    {
        GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(_VSTiPlugIns[SelectedVSTiPluginIndex].HasEditor);
        _VSTiConfig = CfgVSTiConfig[_VSTiPlugIns[SelectedVSTiPluginIndex].Id];
    }

    if (PlugInId != NuclearOptionPlugInId)
    {
        GetDlgItem(IDC_MS_PRESET_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_MS_PRESET).EnableWindow(FALSE);
        GetDlgItem(IDC_MS_PANNING).EnableWindow(FALSE);
    }

    if ((PlugInId != VSTiPlugInId) && (PlugInId != FluidSynthPlugInId) && (PlugInId != BASSMIDIPlugInId) && (PlugInId != SecretSaucePlugInId))
    {
        GetDlgItem(IDC_FILTER_GROUP).EnableWindow(FALSE);

        GetDlgItem(IDC_MIDI_FLAVOR_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_MIDI_FLAVOR).EnableWindow(FALSE);
        GetDlgItem(IDC_MIDI_EFFECTS).EnableWindow(FALSE);
    }

    // Set the selected plug-in.
    {
        int SelectedPlugInIndex = -1;

        if (PlugInId == VSTiPlugInId)
            SelectedPlugInIndex = _ReportedPlugInCount + SelectedVSTiPluginIndex;
    #ifdef DXISUPPORT
        else
        if (PlugInId == DirectXPlugInId)
        {
            if (dxi_selected != ~0)
                SelectedPlugInIndex = _ReportedPlugInCount + vsti_count + dxi_selected;
            else
                plugin = 0;
        }
    #endif
        else
            SelectedPlugInIndex = _PluginPresentMap[PlugInId];

        ::SendMessage(w, CB_SETCURSEL, SelectedPlugInIndex, (WPARAM)0);
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
 
            ::SendMessage(w1, CB_SETCURSEL, (WPARAM)cfg_loop_type, 0);
            ::SendMessage(w2, CB_SETCURSEL, (WPARAM)cfg_loop_type_other, 0);
        }
    }

    SendDlgItemMessage(IDC_THLOOPZ, BM_SETCHECK, (WPARAM)cfg_thloopz);
    SendDlgItemMessage(IDC_RPGMLOOPZ, BM_SETCHECK, (WPARAM)cfg_rpgmloopz);
    SendDlgItemMessage(IDC_XMILOOPZ, BM_SETCHECK, (WPARAM)cfg_xmiloopz);
    SendDlgItemMessage(IDC_FF7LOOPZ, BM_SETCHECK, (WPARAM)cfg_ff7loopz);

    SendDlgItemMessage(IDC_EMIDI_EX, BM_SETCHECK, (WPARAM)cfg_emidi_exclusion);

    SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_SETCHECK, (WPARAM)cfg_filter_instruments);
    SendDlgItemMessage(IDC_FILTER_BANKS, BM_SETCHECK, (WPARAM)cfg_filter_banks);

    SendDlgItemMessage(IDC_MS_PANNING, BM_SETCHECK, (WPARAM)CfgMSPanning);

#pragma region("ADL")
    {
        const char * const * BankNames = adl_getBankNames();
        const size_t BankCount = (size_t)adl_getBanksCount();

        if (BankNames && (BankCount > 0))
        {
            for (unsigned i = 0; i < BankCount; ++i)
                _ADLBanks += adl_bank(i, BankNames[i]);

            _ADLBanks.sort();
        }

        {
            size_t SelectedBank = 0;

            auto w = GetDlgItem(IDC_ADL_BANK);

            for (unsigned i = 0; i < _ADLBanks.get_count(); ++i)
            {
                ::uSendMessageText(w, CB_ADDSTRING, 0, _ADLBanks[i].name);

                if (_ADLBanks[i].number == CfgADLBank)
                    SelectedBank = i;
            }

            w.SendMessage(CB_SETCURSEL, SelectedBank);
        }

        {
            static const char * ChipCounts[] = { "1", "2", "5", "10", "25", "50", "100" };

            w = GetDlgItem(IDC_ADL_CHIPS);

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

        if (_HASSSE2)
            ::uSendMessageText(w, CB_ADDSTRING, 0, "16pt Sinc interpolation");

        if (!_HASSSE2 && CfgResamplingMode > 1)
            ::SendMessage(w, CB_SETCURSEL, 1, 0);
        else
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
        auto w = GetDlgItem(IDC_MUNT_GM);

        for (size_t i = 0; i < _countof(_MUNTGMSets); ++i)
            ::uSendMessageText(w, CB_ADDSTRING, 0, _MUNTGMSets[i]);

        ::SendMessage(w, CB_SETCURSEL, (WPARAM)CfgMUNTGMSet, 0);
    }

    {
        size_t PresetNumber = 0;

        auto w = GetDlgItem(IDC_MS_PRESET);

        for (size_t i = 0; i < _MSPresets.get_count(); ++i)
        {
            const MSPreset & Preset = _MSPresets[i];

            ::uSendMessageText(w, CB_ADDSTRING, 0, Preset.name);

            if (Preset.synth == (unsigned int)CfgMSSynthesizer && Preset.bank == (unsigned int)CfgMSBank)
                PresetNumber = i;
        }

        ::SendMessage(w, CB_SETCURSEL, PresetNumber, 0);
    }

    {
        w = GetDlgItem(IDC_MIDI_FLAVOR);

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
    int plugin_selected = (int)GetDlgItem(IDC_PLUGIN).SendMessage(CB_GETCURSEL, 0, 0);

    if ((plugin_selected >= _ReportedPlugInCount) && plugin_selected < (int)(_ReportedPlugInCount + _VSTiPlugIns.get_count()))
    {
        _IsBusy = true;
        OnChanged();

        VSTiPlayer vstPlayer;

        if (vstPlayer.LoadVST(_VSTiPlugIns[(size_t)(plugin_selected - _ReportedPlugInCount)].PathName.c_str()))
        {
            if (_VSTiConfig.size())
                vstPlayer.setChunk(&_VSTiConfig[0], _VSTiConfig.size());

            vstPlayer.displayEditorModal();
            vstPlayer.getChunk(_VSTiConfig);
        }

        _IsBusy = false;

        OnChanged();
    }
}

void Preferences::OnPluginChange(UINT, int, CWindow w)
{
    // t_size vsti_count = _VSTiPlugins.get_size();
    int plugin_selected = (int)::SendMessage(w, CB_GETCURSEL, 0, 0);
//  int plugin_index = -1;
    int plugin = 0;

    if ((plugin_selected >= _ReportedPlugInCount) && (plugin_selected < (int)(_ReportedPlugInCount + _VSTiPlugIns.get_count())))
        plugin = 1;
#ifdef DXISUPPORT
    else if (plugin_selected >= plugins_reported + _VSTiPlugins.get_count())
        plugin = 5;
#endif
    else
        plugin = _PluginPresentReverseMap[plugin_selected];

    GetDlgItem(IDC_SAMPLERATE).EnableWindow(plugin || !g_running);

    GetDlgItem(IDC_SOUNDFONT_TEXT).EnableWindow(plugin == 2 || plugin == 4);
    GetDlgItem(IDC_SOUNDFONT).EnableWindow(plugin == 2 || plugin == 4);
    GetDlgItem(IDC_RESAMPLING_TEXT).EnableWindow(plugin == 2 || plugin == 4);
    GetDlgItem(IDC_RESAMPLING_MODE).EnableWindow(plugin == 2 || plugin == 4);
    GetDlgItem(IDC_CACHED_TEXT).EnableWindow(plugin == 2 || plugin == 4);
    GetDlgItem(IDC_CACHED).EnableWindow(plugin == 2 || plugin == 4);
    GetDlgItem(IDC_ADL_BANK_TEXT).EnableWindow(plugin == 6);
    GetDlgItem(IDC_ADL_BANK).EnableWindow(plugin == 6);
    GetDlgItem(IDC_ADL_CHIPS_TEXT).EnableWindow(plugin == 6 || plugin == 9);
    GetDlgItem(IDC_ADL_CHIPS).EnableWindow(plugin == 6 || plugin == 9);
    GetDlgItem(IDC_ADL_PANNING).EnableWindow(plugin == 6 || plugin == 9);
    GetDlgItem(IDC_MS_PRESET_TEXT).EnableWindow(plugin == 9);
    GetDlgItem(IDC_MS_PRESET).EnableWindow(plugin == 9);
    GetDlgItem(IDC_MS_PANNING).EnableWindow(plugin == 9);
    {
        bool enable = (plugin == 1) || (plugin == 2) || (plugin == 4) || (plugin == 10);
        GetDlgItem(IDC_FILTER_GROUP).EnableWindow(enable);
        GetDlgItem(IDC_MIDI_FLAVOR_TEXT).EnableWindow(enable);
        GetDlgItem(IDC_MIDI_FLAVOR).EnableWindow(enable);
        GetDlgItem(IDC_MIDI_EFFECTS).EnableWindow(enable);
    }

    if (plugin == 3)
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

    GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(plugin_selected >= _ReportedPlugInCount && plugin < _ReportedPlugInCount + _VSTiPlugIns.get_count() && _VSTiPlugIns[(size_t)(plugin_selected - _ReportedPlugInCount)].HasEditor);

    if ((plugin_selected >= _ReportedPlugInCount) && (plugin_selected < (int)(_ReportedPlugInCount + _VSTiPlugIns.get_count())))
    {
        _VSTiConfig = CfgVSTiConfig[_VSTiPlugIns[(size_t)(plugin_selected - _ReportedPlugInCount)].Id];
    }

    OnChanged();
}

void Preferences::OnSetFocus(UINT, int, CWindow w)
{
    SetFocus();

    if (w == GetDlgItem(IDC_SOUNDFONT))
    {
        pfc::string8 directory, filename;

        directory = _SoundFontPath;
        filename = _SoundFontPath;

        directory.truncate(directory.scan_filename());

        if (uGetOpenFileName(m_hWnd,
            "SoundFont and list files|*.sf2;*.sf3;*.sflist"
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
            0, "sf2", "Choose a SoundFont bank or list...", directory, filename, FALSE))
        {
            _SoundFontPath = filename;

            uSetWindowText(w, filename.get_ptr() + filename.scan_filename());
            OnChanged();
        }
    }
    else
    if (w == GetDlgItem(IDC_MUNT))
    {
        pfc::string8 path;
        if (uBrowseForFolder(m_hWnd, "Locate MT-32 or CM-32L ROM set...", path))
        {
            _MUNTPath = path;

            t_size length = _MUNTPath.length();

            if (length >= 1 && !pfc::is_path_separator((unsigned int)*(_MUNTPath.get_ptr() + length - 1)))
                _MUNTPath.add_byte('\\');

            const char * display_path;

            if (length)
                display_path = _MUNTPath;
            else
                display_path = DefaultPathMessage;

            uSetWindowText(w, display_path);
            OnChanged();
        }
    }
}

void Preferences::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == ID_REFRESH)
    {
        GetDlgItem(IDC_SAMPLERATE).EnableWindow(CfgPlugInId || !g_running);

    #ifdef BASSMIDISUPPORT
        m_cached.reset();

        uint64_t total_sample_size, samples_loaded_size;

        if (GetSoundFontStatistics(total_sample_size, samples_loaded_size))
        {
            m_cached = pfc::format_file_size_short(samples_loaded_size);
            m_cached += " / ";
            m_cached += pfc::format_file_size_short(total_sample_size);
        }
        else
        {
            m_cached = "BASS not loaded.";
        }

        if (strcmp(m_cached, m_cached_current))
        {
            m_cached_current = m_cached;
            uSetWindowText(GetDlgItem(IDC_CACHED), m_cached);
        }
    #endif
    }
}

t_uint32 Preferences::get_state()
{
    t_uint32 State = preferences_state::resettable | preferences_state::dark_mode_supported;

    if (HasChanged())
        State |= preferences_state::changed;

    if (_IsBusy)
        State |= preferences_state::busy;

    return State;
}

void Preferences::reset()
{
    int default_plugin_entry = _PluginIdToIndex[DefaultPlugInId];
    int plugin_selected = _PluginPresentMap[default_plugin_entry];

    SendDlgItemMessage(IDC_PLUGIN, CB_SETCURSEL, (WPARAM)plugin_selected);

    if (DefaultPlugInId != 2 && DefaultPlugInId != 4)
    {
        GetDlgItem(IDC_SOUNDFONT_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_SOUNDFONT).EnableWindow(FALSE);
        GetDlgItem(IDC_RESAMPLING_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_RESAMPLING_MODE).EnableWindow(FALSE);
        GetDlgItem(IDC_CACHED_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_CACHED).EnableWindow(FALSE);
    }
    if (DefaultPlugInId != 6)
    {
        GetDlgItem(IDC_ADL_BANK_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_BANK).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_CHIPS_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_CHIPS).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_PANNING).EnableWindow(FALSE);
    }
    if (DefaultPlugInId == 3)
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
    if (DefaultPlugInId != 9)
    {
        GetDlgItem(IDC_MS_PRESET_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_MS_PRESET).EnableWindow(FALSE);
        GetDlgItem(IDC_MS_PANNING).EnableWindow(FALSE);
    }
    {
        bool enable = (DefaultPlugInId == 1) || (DefaultPlugInId == 2) || (DefaultPlugInId == 4) || (DefaultPlugInId == 10);
        GetDlgItem(IDC_FILTER_GROUP).EnableWindow(enable);
        GetDlgItem(IDC_MIDI_FLAVOR_TEXT).EnableWindow(enable);
        GetDlgItem(IDC_MIDI_FLAVOR).EnableWindow(enable);
        GetDlgItem(IDC_MIDI_EFFECTS).EnableWindow(enable);
    }

    uSetDlgItemText(m_hWnd, IDC_SOUNDFONT, DefaultPathMessage);
    uSetDlgItemText(m_hWnd, IDC_MUNT, DefaultPathMessage);

    _SoundFontPath.reset();
    _MUNTPath.reset();

    SetDlgItemInt(IDC_SAMPLERATE, DefaultSampleRate, FALSE);

    if (!DefaultPlugInId)
    {
        if (g_running)
            GetDlgItem(IDC_SAMPLERATE).EnableWindow(FALSE);
    }

    SendDlgItemMessage(IDC_LOOP_PLAYBACK, CB_SETCURSEL, DefaultPlaybackLoopType);
    SendDlgItemMessage(IDC_LOOP_OTHER, CB_SETCURSEL, DefaultOtherLoopType);
    SendDlgItemMessage(IDC_THLOOPZ, BM_SETCHECK, default_cfg_thloopz);
    SendDlgItemMessage(IDC_RPGMLOOPZ, BM_SETCHECK, default_cfg_rpgmloopz);
    SendDlgItemMessage(IDC_XMILOOPZ, BM_SETCHECK, default_cfg_xmiloopz);
    SendDlgItemMessage(IDC_FF7LOOPZ, BM_SETCHECK, default_cfg_ff7loopz);
    SendDlgItemMessage(IDC_EMIDI_EX, BM_SETCHECK, default_cfg_emidi_exclusion);
    SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_SETCHECK, default_cfg_filter_instruments);
    SendDlgItemMessage(IDC_FILTER_BANKS, BM_SETCHECK, default_cfg_filter_banks);
    SendDlgItemMessage(IDC_MS_PANNING, BM_SETCHECK, DefaultMSPanning);
    SendDlgItemMessage(IDC_MIDI_FLAVOR, CB_SETCURSEL, default_cfg_midi_flavor);
    SendDlgItemMessage(IDC_MIDI_EFFECTS, BM_SETCHECK, !default_cfg_midi_reverb);

    {
        unsigned bank_selected = 0;

        for (unsigned i = 0; i < _ADLBanks.get_count(); ++i)
        {
            if (_ADLBanks[i].number == DefaultADLBank)
            {
                bank_selected = i;
                break;
            }
        }

        SendDlgItemMessage(IDC_ADL_BANK, CB_SETCURSEL, bank_selected);
    }

    SendDlgItemMessage(IDC_ADL_PANNING, BM_SETCHECK, DefaultADLPanning);
    SetDlgItemInt(IDC_ADL_CHIPS, DefaultADLChipCount, 0);
//  SendDlgItemMessage( IDC_RECOVER, BM_SETCHECK, default_cfg_recover_tracks );

#ifdef FLUIDSYNTHSUPPORT
    SendDlgItemMessage(IDC_RESAMPLING, CB_SETCURSEL, 2 /* 4 */);
#else
    SendDlgItemMessage(IDC_RESAMPLING_MODE, CB_SETCURSEL, DefaultResamplingMode);
#endif
    {
        size_t preset_number = 0;

        for (size_t i = 0, j = _MSPresets.get_count(); i < j; ++i)
        {
            const MSPreset & preset = _MSPresets[i];

            if (preset.synth == DefaultMSSynth && preset.bank == DefaultMSBank)
            {
                preset_number = i;
                break;
            }
        }

        SendDlgItemMessage(IDC_MS_PRESET, CB_SETCURSEL, preset_number);
    }

    SendDlgItemMessage(IDC_MIDI_FLAVOR, CB_SETCURSEL, (WPARAM)CfgMIDIFlavor);

    _VSTiConfig.resize(0);

    OnChanged();
}

void Preferences::apply()
{
    int t = (int)GetDlgItemInt(IDC_SAMPLERATE, NULL, FALSE);

    if (t < 6000)
        t = 6000;
    else
    if (t > 192000)
        t = 192000;

    SetDlgItemInt(IDC_SAMPLERATE, (UINT)t, FALSE);

    {
        char temp[16];

        _itoa_s(t, temp, _countof(temp), 10);

        CfgSampleRateHistory.add_item(temp);
        CfgSampleRate = t;
    }

    {
        t = (int)SendDlgItemMessage(IDC_ADL_BANK, CB_GETCURSEL);

        if (t < 0 || t >= (int)_ADLBanks.get_count())
            t = 0;

        CfgADLBank = _ADLBanks[(t_size)t].number;
    }

    {
        t = (int)GetDlgItemInt(IDC_ADL_CHIPS, NULL, FALSE);

        if (t < 1)
            t = 1;
        else
        if (t > 100)
            t = 100;

        SetDlgItemInt(IDC_ADL_CHIPS, (UINT)t, FALSE);

        CfgADLChipCount = t;
    }

    CfgADLPanning = (t_int32)SendDlgItemMessage(IDC_ADL_PANNING, BM_GETCHECK);
    CfgMUNTGMSet = (t_int32)SendDlgItemMessage(IDC_MUNT_GM, CB_GETCURSEL);

    {
        unsigned int preset_number = (unsigned int)SendDlgItemMessage(IDC_MS_PRESET, CB_GETCURSEL);

        if (preset_number >= _MSPresets.get_count())
            preset_number = 0;

        const MSPreset & preset = _MSPresets[preset_number];

        CfgMSSynthesizer = (t_int32)preset.synth;
        CfgMSBank = (t_int32)preset.bank;
    }

    {
        int plugin_selected = (int)SendDlgItemMessage(IDC_PLUGIN, CB_GETCURSEL);
        int plugin = -1;

        if (plugin_selected >= _ReportedPlugInCount && plugin_selected < (int)(_ReportedPlugInCount + _VSTiPlugIns.get_count()))
            plugin = 1;
    #ifdef DXISUPPORT
        else if (plugin_selected >= plugins_reported + _VSTiPlugins.get_count())
            plugin = 5;
    #endif
        else
            plugin = _PluginPresentReverseMap[plugin_selected];

        CfgVSTiPath = "";

        CfgPlugInId = plugin;

        if (plugin == 1)
        {
            CfgVSTiPath = _VSTiPlugIns[(size_t)(plugin_selected - _ReportedPlugInCount)].PathName.c_str();

            CfgVSTiConfig[_VSTiPlugIns[(size_t)(plugin_selected - _ReportedPlugInCount)].Id] = _VSTiConfig;
        }
    #ifdef DXISUPPORT
        else if (plugin == 5)
        {
            cfg_dxi_plugin = dxi_plugins[plugin_selected - _VSTiPlugins.get_count() - plugins_reported];
        }
    #endif
    }

    CfgSoundFontPath = _SoundFontPath;
    CfgMUNTPath = _MUNTPath;

    cfg_loop_type = (t_int32)SendDlgItemMessage(IDC_LOOP_PLAYBACK, CB_GETCURSEL);
    cfg_loop_type_other = (t_int32)SendDlgItemMessage(IDC_LOOP_OTHER, CB_GETCURSEL);
    cfg_thloopz = (t_int32)SendDlgItemMessage(IDC_THLOOPZ, BM_GETCHECK);
    cfg_rpgmloopz = (t_int32)SendDlgItemMessage(IDC_RPGMLOOPZ, BM_GETCHECK);
    cfg_xmiloopz = (t_int32)SendDlgItemMessage(IDC_XMILOOPZ, BM_GETCHECK);
    cfg_ff7loopz = (t_int32)SendDlgItemMessage(IDC_FF7LOOPZ, BM_GETCHECK);
    cfg_emidi_exclusion = (t_int32)SendDlgItemMessage(IDC_EMIDI_EX, BM_GETCHECK);
    cfg_filter_instruments = (t_int32)SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_GETCHECK);
    cfg_filter_banks = (t_int32)SendDlgItemMessage(IDC_FILTER_BANKS, BM_GETCHECK);
    CfgMSPanning = (t_int32)SendDlgItemMessage(IDC_MS_PANNING, BM_GETCHECK);
//  cfg_recover_tracks = SendDlgItemMessage( IDC_RECOVER, BM_GETCHECK );

#ifdef FLUIDSYNTHSUPPORT
    {
        int interp_method = SendDlgItemMessage(IDC_RESAMPLING, CB_GETCURSEL);
        if (interp_method == 2)
            interp_method = 4;
        else if (interp_method == 3)
            interp_method = 7;
        Cfg_FluidSynthInterpolationMethod = interp_method;
    }
#endif

#ifdef BASSMIDISUPPORT
    CfgResamplingMode = (t_int32)SendDlgItemMessage(IDC_RESAMPLING_MODE, CB_GETCURSEL);
#endif
    CfgMIDIFlavor = (t_int32)SendDlgItemMessage(IDC_MIDI_FLAVOR, CB_GETCURSEL);
    CfgAllowMIDIEffects = !SendDlgItemMessage(IDC_MIDI_EFFECTS, BM_GETCHECK);

    OnChanged(); // our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool Preferences::HasChanged()
{
    // returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
    bool changed = false;

    if (!changed && GetDlgItemInt(IDC_SAMPLERATE, NULL, FALSE) != (UINT)CfgSampleRate) changed = true;
    if (!changed && SendDlgItemMessage(IDC_LOOP_PLAYBACK, CB_GETCURSEL) != cfg_loop_type) changed = true;
    if (!changed && SendDlgItemMessage(IDC_LOOP_OTHER, CB_GETCURSEL) != cfg_loop_type_other) changed = true;
    if (!changed && SendDlgItemMessage(IDC_THLOOPZ, BM_GETCHECK) != cfg_thloopz) changed = true;
    if (!changed && SendDlgItemMessage(IDC_RPGMLOOPZ, BM_GETCHECK) != cfg_rpgmloopz) changed = true;
    if (!changed && SendDlgItemMessage(IDC_XMILOOPZ, BM_GETCHECK) != cfg_xmiloopz) changed = true;
    if (!changed && SendDlgItemMessage(IDC_FF7LOOPZ, BM_GETCHECK) != cfg_ff7loopz) changed = true;
    if (!changed && SendDlgItemMessage(IDC_EMIDI_EX, BM_GETCHECK) != cfg_emidi_exclusion) changed = true;
    if (!changed && SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_GETCHECK) != cfg_filter_instruments) changed = true;
    if (!changed && SendDlgItemMessage(IDC_FILTER_BANKS, BM_GETCHECK) != cfg_filter_banks) changed = true;
    if (!changed && SendDlgItemMessage(IDC_MS_PANNING, BM_GETCHECK) != CfgMSPanning) changed = true;

#ifdef FLUIDSYNTHSUPPORT
    if (!changed)
    {
        int interp_method = SendDlgItemMessage(IDC_RESAMPLING, CB_GETCURSEL);
        if (interp_method == 2)
            interp_method = 4;
        else if (interp_method == 3)
            interp_method = 7;
        if (interp_method != Cfg_FluidSynthInterpolationMethod) changed = true;
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
        unsigned int preset_number = (unsigned int)SendDlgItemMessage(IDC_MS_PRESET, CB_GETCURSEL);

        if (preset_number >= _MSPresets.get_count())
            preset_number = 0;

        const MSPreset & preset = _MSPresets[preset_number];

        if (!(preset.synth == (unsigned int)CfgMSSynthesizer && preset.bank == (unsigned int)CfgMSBank))
            changed = true;
    }

    if (!changed)
    {
        int t = (int)SendDlgItemMessage(IDC_ADL_BANK, CB_GETCURSEL);

        if (t < 0 || t >= (int)_ADLBanks.get_count())
            t = 0;

        if (_ADLBanks[(t_size)t].number != (int)CfgADLBank)
            changed = true;
    }

    if (!changed && GetDlgItemInt(IDC_ADL_CHIPS, NULL, FALSE) != (UINT)CfgADLChipCount)
        changed = true;

    if (!changed && SendDlgItemMessage(IDC_ADL_PANNING, BM_GETCHECK) != CfgADLPanning)
        changed = true;

    if (!changed && SendDlgItemMessage(IDC_MUNT_GM, CB_GETCURSEL) != CfgMUNTGMSet)
        changed = true;

    if (!changed)
    {
        int plugin_selected = (int)SendDlgItemMessage(IDC_PLUGIN, CB_GETCURSEL);
        int plugin = -1;

        if ((plugin_selected >= _ReportedPlugInCount) && (plugin_selected < _ReportedPlugInCount + (int)_VSTiPlugIns.get_count()))
            plugin = 1;
    #ifdef DXISUPPORT
        else if (plugin_selected >= plugins_reported + _VSTiPlugins.get_count())
            plugin = 5;
    #endif
        else
            plugin = _PluginPresentReverseMap[plugin_selected];

        if (plugin != CfgPlugInId)
            changed = true;

        if (!changed && plugin == 1)
        {
            if (stricmp_utf8(CfgVSTiPath, _VSTiPlugIns[(size_t)(plugin_selected - _ReportedPlugInCount)].PathName.c_str()))
                changed = true;

            if (!changed)
            {
                t_uint32 unique_id = _VSTiPlugIns[(size_t)(plugin_selected - _ReportedPlugInCount)].Id;

                if (_VSTiConfig.size() != CfgVSTiConfig[unique_id].size() || (_VSTiConfig.size() && memcmp(&_VSTiConfig[0], &CfgVSTiConfig[unique_id][0], _VSTiConfig.size())))
                    changed = true;
            }
        }
    #ifdef DXISUPPORT
        else if (!changed && plugin == 5)
        {
            if (dxi_plugins[plugin_selected - _VSTiPlugins.get_count() - plugins_reported] != cfg_dxi_plugin.get_value()) changed = true;
        }
    #endif
    }

    if (!changed)
    {
        if (stricmp_utf8(_SoundFontPath, CfgSoundFontPath))
            changed = true;
    }

    if (!changed)
    {
        if (stricmp_utf8(_MUNTPath, CfgMUNTPath))
            changed = true;
    }

    return changed;
}

void Preferences::OnChanged()
{
    // tell the host that our state has changed to enable/disable the apply button appropriately.
    m_callback->on_state_changed();
}

#pragma region("VSTi")
/// <summary>
/// Gets all the VTSi plugins if a root directory has been specified.
/// </summary>
void Preferences::GetVSTiPlugins(const char * pathName, puFindFile findFile)
{
    pfc::string8 VSTiSearchPath;

    if (findFile == nullptr)
    {
        _VSTiPlugIns.set_size(0);

        CfgVSTiSearchPath.get(VSTiSearchPath);

        if (VSTiSearchPath.is_empty())
        {
            GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(FALSE);

            return;
        }

        console::print("Enumerating VSTi plug-ins...");

        if (VSTiSearchPath[VSTiSearchPath.length() - 1] != '\\')
            VSTiSearchPath.add_byte('\\');

        VSTiSearchPath += "*.*";

        pathName = VSTiSearchPath;

        findFile = ::uFindFirstFile(VSTiSearchPath);
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
                console::formatter() << "Examining \"" << PathName << "\"...";

                VSTiPlayer Player;

                if (Player.LoadVST(PathName))
                {
                    VSTiPlugin Plugin;

                    Plugin.Name = "VST ";
                    Plugin.PathName = PathName;

                    std::string VendorName;

                    Player.getVendorString(VendorName);

                    std::string ProductName;

                    Player.getProductString(ProductName);

                    if (VendorName.length() || ProductName.length())
                    {
                        if ((VendorName.length() == 0) || ((ProductName.length() >= VendorName.length()) && (::strncmp(VendorName.c_str(), ProductName.c_str(), VendorName.length()) == 0)))
                        {
                            Plugin.Name += ProductName;
                        }
                        else
                        {
                            Plugin.Name += VendorName;

                            if (ProductName.length())
                            {
                                Plugin.Name += ' ';
                                Plugin.Name += ProductName;
                            }
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

        bool HasFileSizeMatch = false;

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

#ifdef BASSMIDISUPPORT
#if defined(_M_IX86) || defined(__i386__) // x86, either x86_64 or no SSE2 compiled in?
#if 1 /* SSE2 is my current minimum */
const bool _HASSSE2 = true;
#else
#ifdef _MSC_VER
#include <intrin.h>
#elif defined(__clang__) || defined(__GNUC__)
static inline void
__cpuid(int * data, int selector)
{
    asm("cpuid"
        : "=a"(data[0]),
        "=b"(data[1]),
        "=c"(data[2]),
        "=d"(data[3])
        : "a"(selector));
}
#else
#define __cpuid(a, b) memset((a), 0, sizeof(int) * 4)
#endif

static bool query_cpu_feature_sse2()
{
    int buffer[4];

    __cpuid(buffer, 1);
 
   return ((buffer[3] & (1 << 26)) == 0) ? false : true;
}

bool _HASSSE2 = query_cpu_feature_sse2();
#endif
#else
const bool _HASSSE2 = true;
#endif
#endif

class PreferencesPage : public preferences_page_impl<Preferences>
{
public:
    const char * get_name()
    {
        return COMPONENT_NAME;
    }

    GUID get_guid()
    {
        return COMPONENT_GUID;
    }

    GUID get_parent_guid()
    {
        return guid_input;
    }
};

static preferences_page_factory_t<PreferencesPage> PreferencesPageFactory;
