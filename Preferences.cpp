
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
#include "Fields.h"
#include "MIDIPreset.h"

#include "ADLPlayer.h"
#include "BMPlayer.h"
#include "VSTiPlayer.h"

#pragma warning(disable: 4820)

#define DEBUG_DIALOG

#pragma hdrstop

extern char _DLLFileName[];
extern volatile int g_running;

static const GUID GUIDCfgSampleRateHistory = { 0x408aa155, 0x4c42, 0x42b5, { 0x8c, 0x3e, 0xd1, 0xc, 0x35, 0xdd, 0x5e, 0xf1 } };
static cfg_dropdown_history CfgSampleRateHistory(GUIDCfgSampleRateHistory, 16);

static const char * click_to_set = "Click to set.";

static const int SampleRates[] = { 8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 49716, 64000, 88200, 96000 };

static const char * loop_txt[] =
{
    "Never loop",
    "Never, add 1s decay time",
    "Loop and fade when detected",
    "Always loop and fade",
    "Play indefinitely when detected",
    "Play indefinitely"
};

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
    Preferences(preferences_page_callback::ptr callback) noexcept : m_callback(callback), busy(false) { }
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

        COMMAND_HANDLER_EX(IDC_MUNT, EN_SETFOCUS, OnSetFocus)

        COMMAND_HANDLER_EX(IDC_LOOP, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_CLOOP, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_RESAMPLING, CBN_SELCHANGE, OnSelectionChange)
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

        COMMAND_HANDLER_EX(IDC_FILTER_FLAVOR, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_FILTER_EFFECTS, BN_CLICKED, OnButtonClick)

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

    bool busy;

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

    pfc::array_t<VSTiPlugin> _VSTiPlugins;
    std::vector<uint8_t> vsti_config;
#pragma endregion

#pragma region("SoundFont")
    pfc::string8 m_soundfont;
#pragma endregion

#pragma region("MUNT")
    pfc::string8 m_munt_path;
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

    pfc::list_t<adl_bank> m_bank_list;

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

    int _ReportedPluginCount;

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
    { "Emu de MIDI",    EmuDeMIDIPluginId,      -1, IsPluginAlwaysPresent },

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
          BASSMIDIPlugInId, FluidSynthPluginId,
    #endif
      IsPluginAlwaysPresent },
#endif

    { "Super MUNT GM",  SuperMUNTPluginId,      -1, IsPluginAlwaysPresent },

    { "libADLMIDI",     ADLPluginId,            -1, IsPluginAlwaysPresent },
    { "libOPNMIDI",     OPNPluginId,            -1, IsPluginAlwaysPresent },
    { "oplmidi",        OPLPluginId,            -1, IsPluginNeverPresent },
    { "Nuclear Option", NuclearOptionPluginId,  -1, IsPluginAlwaysPresent },
    { "Secret Sauce",   SecretSaucePluginId,    -1, IsSecretSaucePresent }
};

static const char * chip_counts[] = { "1", "2", "5", "10", "25", "50", "100" };

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

    int Plugin = CfgPluginId;
    int plugin_selected = -1;

    _PluginIdToIndex[Plugin] = -1;
    _PluginPresentReverseMap[-1] = -1;

    for (size_t i = 0; i < _countof(BuiltInPlugins); ++i)
    {
        _PluginIdToIndex[BuiltInPlugins[i].Id] = i;

        if (BuiltInPlugins[i].plugin_number_alt >= 0)
            _PluginIdToIndex[BuiltInPlugins[i].plugin_number_alt] = i;
    }

    int PluginIndex = -1;

    if (Plugin != 1 && Plugin != 5)
    {
        PluginIndex = _PluginIdToIndex[Plugin];

        if (PluginIndex < 0 || !BuiltInPlugins[PluginIndex].IsPresent(this))
        {
            Plugin = DefaultPluginId;
            PluginIndex = _PluginIdToIndex[DefaultPluginId];
        }
        else
        if (BuiltInPlugins[PluginIndex].plugin_number_alt == Plugin)
            Plugin = BuiltInPlugins[PluginIndex].Id;
    }

    CWindow w = GetDlgItem(IDC_PLUGIN);

    {
        _ReportedPluginCount = 0;

        _PluginPresentMap[Plugin] = -1;

        for (size_t i = 0; i < _countof(BuiltInPlugins); ++i)
        {
            const BuiltInPlugin& Plugin = BuiltInPlugins[i];

            if (Plugin.IsPresent(this))
            {
                _PluginPresentMap[Plugin.Id] = _ReportedPluginCount;

                if (Plugin.plugin_number_alt >= 0)
                    _PluginPresentMap[Plugin.plugin_number_alt] = _ReportedPluginCount;

                _PluginPresentReverseMap[_ReportedPluginCount] = Plugin.Id;
                ++_ReportedPluginCount;

                uSendMessageText(w, CB_ADDSTRING, 0, Plugin.Name);
            }
        }
    }

 #pragma region("VSTi")
    size_t vsti_selected = (unsigned int)~0;

    GetVSTiPlugins();

    size_t VSTiCount = _VSTiPlugins.get_size();

    if (VSTiCount > 0)
    {
        console::formatter() << "Found " << pfc::format_int(_VSTiPlugins.get_size()) << " plug-ins";

        for (unsigned i = 0, j = VSTiCount; i < j; ++i)
            _PluginPresentReverseMap[_ReportedPluginCount + i] = 1;

        for (unsigned i = 0; i < VSTiCount; ++i)
        {
            uSendMessageText(w, CB_ADDSTRING, 0, _VSTiPlugins[i].Name.c_str());

            if (Plugin == 1 && !stricmp_utf8(_VSTiPlugins[i].PathName.c_str(), cfg_vst_path))
                vsti_selected = i;
        }
    }
#pragma endregion

    if (Plugin == 1 && vsti_selected == ~0)
    {
        Plugin = DefaultPluginId;
        PluginIndex = _PluginIdToIndex[DefaultPluginId];
    }

    if (Plugin != 2 && Plugin != 4)
    {
        GetDlgItem(IDC_SOUNDFONT_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_SOUNDFONT).EnableWindow(FALSE);
        GetDlgItem(IDC_RESAMPLING_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_RESAMPLING).EnableWindow(FALSE);
        GetDlgItem(IDC_CACHED_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_CACHED).EnableWindow(FALSE);
    }

    if (Plugin == 3)
    {
        GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(FALSE);
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_SHOW);
    }

    if (Plugin != 6)
    {
        GetDlgItem(IDC_ADL_BANK_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_BANK).EnableWindow(FALSE);
    }

    if (Plugin != 6 && Plugin != 7)
    {
        GetDlgItem(IDC_ADL_CHIPS_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_CHIPS).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_PANNING).EnableWindow(FALSE);
    }

    if (Plugin != 1)
    {
        GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(FALSE);
    }
    else
    {
        GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(_VSTiPlugins[vsti_selected].HasEditor);
        vsti_config = cfg_vst_config[_VSTiPlugins[vsti_selected].Id];
    }

    if (Plugin != 9)
    {
        GetDlgItem(IDC_MS_PRESET_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_MS_PRESET).EnableWindow(FALSE);
        GetDlgItem(IDC_MS_PANNING).EnableWindow(FALSE);
    }

    if (Plugin != 1 && Plugin != 2 && Plugin != 4 && Plugin != 10)
    {
        GetDlgItem(IDC_FILTER_GROUP).EnableWindow(FALSE);
        GetDlgItem(IDC_FILTER_FLAVOR_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_FILTER_FLAVOR).EnableWindow(FALSE);
        GetDlgItem(IDC_FILTER_EFFECTS).EnableWindow(FALSE);
    }

    {
        m_soundfont = cfg_soundfont_path;
        const char * filename;
        if (m_soundfont.is_empty())
            filename = click_to_set;
        else
            filename = m_soundfont.get_ptr() + m_soundfont.scan_filename();
        uSetDlgItemText(m_hWnd, IDC_SOUNDFONT, filename);
    }

    {
        m_munt_path = cfg_munt_base_path;
        const char * path;
        if (m_munt_path.is_empty())
            path = click_to_set;
        else
            path = m_munt_path;
        uSetDlgItemText(m_hWnd, IDC_MUNT, path);
    }

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
    if (Plugin == 1) plugin_selected = _ReportedPluginCount + vsti_selected;
#ifdef DXISUPPORT
    else if (plugin == 5)
    {
        if (dxi_selected != ~0)
            plugin_selected = plugins_reported + vsti_count + dxi_selected;
        else
            plugin = 0;
    }
#endif
    else
        plugin_selected = _PluginPresentMap[Plugin];

    ::SendMessage(w, CB_SETCURSEL, plugin_selected, (WPARAM)0);

    {
        char temp[16];

        for (size_t n = _countof(SampleRates); n--;)
        {
            if (SampleRates[n] != cfg_srate)
            {
                _itoa_s(SampleRates[n], temp, _countof(temp), 10);
                CfgSampleRateHistory.add_item(temp);
            }
        }

        _itoa_s(cfg_srate, temp, _countof(temp), 10);
        CfgSampleRateHistory.add_item(temp);

        w = GetDlgItem(IDC_SAMPLERATE);

        CfgSampleRateHistory.setup_dropdown(w);

        ::SendMessage(w, CB_SETCURSEL, 0, 0);
    }

    if (!Plugin)
    {
        if (g_running)
            GetDlgItem(IDC_SAMPLERATE).EnableWindow(FALSE);
    }

    w = GetDlgItem(IDC_LOOP);

    for (unsigned i = 0; i < _countof(loop_txt); ++i)
    {
        uSendMessageText(w, CB_ADDSTRING, 0, loop_txt[i]);
    }
    ::SendMessage(w, CB_SETCURSEL, (WPARAM)cfg_loop_type, 0);

    w = GetDlgItem(IDC_CLOOP);

    for (unsigned i = 0; i < _countof(loop_txt) - 2; ++i)
    {
        uSendMessageText(w, CB_ADDSTRING, 0, loop_txt[i]);
    }
    ::SendMessage(w, CB_SETCURSEL, (WPARAM)cfg_loop_type_other, 0);

    SendDlgItemMessage(IDC_THLOOPZ, BM_SETCHECK, (WPARAM)cfg_thloopz);
    SendDlgItemMessage(IDC_RPGMLOOPZ, BM_SETCHECK, (WPARAM)cfg_rpgmloopz);
    SendDlgItemMessage(IDC_XMILOOPZ, BM_SETCHECK, (WPARAM)cfg_xmiloopz);
    SendDlgItemMessage(IDC_FF7LOOPZ, BM_SETCHECK, (WPARAM)cfg_ff7loopz);

    SendDlgItemMessage(IDC_EMIDI_EX, BM_SETCHECK, (WPARAM)cfg_emidi_exclusion);
    SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_SETCHECK, (WPARAM)cfg_filter_instruments);
    SendDlgItemMessage(IDC_FILTER_BANKS, BM_SETCHECK, (WPARAM)cfg_filter_banks);

    SendDlgItemMessage(IDC_MS_PANNING, BM_SETCHECK, (WPARAM)cfg_ms_panning);
    SendDlgItemMessage(IDC_FILTER_EFFECTS, BM_SETCHECK, !cfg_midi_reverb);

    const char * const * banknames = adl_getBankNames();
    const unsigned bank_count = adl_getBanksCount();

    for (unsigned i = 0; i < bank_count; ++i)
    {
        m_bank_list += adl_bank(i, banknames[i]);
    }

    m_bank_list.sort();

    unsigned bank_selected = 0;

    w = GetDlgItem(IDC_ADL_BANK);

    for (unsigned i = 0; i < m_bank_list.get_count(); ++i)
    {
        uSendMessageText(w, CB_ADDSTRING, 0, m_bank_list[i].name);

        if (m_bank_list[i].number == cfg_adl_bank)
            bank_selected = i;
    }

    w.SendMessage(CB_SETCURSEL, bank_selected);

    w = GetDlgItem(IDC_ADL_CHIPS);

    for (unsigned i = 0; i < _countof(chip_counts); ++i)
    {
        uSendMessageText(w, CB_ADDSTRING, 0, chip_counts[i]);
    }

    SetDlgItemInt(IDC_ADL_CHIPS, (UINT)cfg_adl_chips, 0);

    SendDlgItemMessage(IDC_ADL_PANNING, BM_SETCHECK, (WPARAM)cfg_adl_panning);

#ifdef BASSMIDISUPPORT
    w = GetDlgItem(IDC_RESAMPLING);
    uSendMessageText(w, CB_ADDSTRING, 0, "Linear interpolation");
    uSendMessageText(w, CB_ADDSTRING, 0, "8pt Sinc interpolation");
    if (_bassmidi_src2_avail)
        uSendMessageText(w, CB_ADDSTRING, 0, "16pt Sinc interpolation");
    if (!_bassmidi_src2_avail && cfg_resampling > 1)
        ::SendMessage(w, CB_SETCURSEL, 1, 0);
    else
        ::SendMessage(w, CB_SETCURSEL, (WPARAM)cfg_resampling, 0);
#endif
#ifdef FLUIDSYNTHSUPPORT
    w = GetDlgItem(IDC_RESAMPLING);
    uSendMessageText(w, CB_ADDSTRING, 0, "No interpolation");
    uSendMessageText(w, CB_ADDSTRING, 0, "Linear interpolation");
    uSendMessageText(w, CB_ADDSTRING, 0, "Cubic interpolation");
    uSendMessageText(w, CB_ADDSTRING, 0, "7pt Sinc interpolation");
    if (cfg_fluid_interp_method == 0)
        ::SendMessage(w, CB_SETCURSEL, 0, 0);
    else if (cfg_fluid_interp_method == 1)
        ::SendMessage(w, CB_SETCURSEL, 1, 0);
    else if (cfg_fluid_interp_method == 4)
        ::SendMessage(w, CB_SETCURSEL, 2, 0);
    else if (cfg_fluid_interp_method == 7)
        ::SendMessage(w, CB_SETCURSEL, 3, 0);
    else
        ::SendMessage(w, CB_SETCURSEL, 3, 0);
#endif
    {
        w = GetDlgItem(IDC_MUNT_GM);

        for (unsigned i = 0, j = _countof(MuntBankNames); i < j; ++i)
        {
            uSendMessageText(w, CB_ADDSTRING, 0, MuntBankNames[i]);
        }

        ::SendMessage(w, CB_SETCURSEL, (WPARAM)cfg_munt_gm, 0);
    }

    {
        size_t PresetNumber = 0;

        w = GetDlgItem(IDC_MS_PRESET);

        for (size_t i = 0, j = _MSPresets.get_count(); i < j; ++i)
        {
            const MSPreset & preset = _MSPresets[i];

            uSendMessageText(w, CB_ADDSTRING, 0, preset.name);

            if (preset.synth == (unsigned int)cfg_ms_synth && preset.bank == (unsigned int)cfg_ms_bank)
                PresetNumber = i;
        }

        ::SendMessage(w, CB_SETCURSEL, PresetNumber, 0);
    }

    w = GetDlgItem(IDC_FILTER_FLAVOR);

    uSendMessageText(w, CB_ADDSTRING, 0, "Default");
    uSendMessageText(w, CB_ADDSTRING, 0, "GM");
    uSendMessageText(w, CB_ADDSTRING, 0, "GM2");
    uSendMessageText(w, CB_ADDSTRING, 0, "GS SC-55");
    uSendMessageText(w, CB_ADDSTRING, 0, "GS SC-88");
    uSendMessageText(w, CB_ADDSTRING, 0, "GS SC-88 Pro");
    uSendMessageText(w, CB_ADDSTRING, 0, "GS SC-8820");
    uSendMessageText(w, CB_ADDSTRING, 0, "XG");
    ::SendMessage(w, CB_SETCURSEL, (WPARAM)cfg_midi_flavor, 0);

#ifndef BASSMIDISUPPORT
    uSetWindowText(GetDlgItem(IDC_CACHED), "No info.");
#endif

    SetTimer(ID_REFRESH, 20);

    busy = false;

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

    if ((plugin_selected >= _ReportedPluginCount) && plugin_selected < (int)(_ReportedPluginCount + _VSTiPlugins.get_count()))
    {
        busy = true;
        OnChanged();

        VSTiPlayer vstPlayer;

        if (vstPlayer.LoadVST(_VSTiPlugins[(size_t)(plugin_selected - _ReportedPluginCount)].PathName.c_str()))
        {
            if (vsti_config.size())
                vstPlayer.setChunk(&vsti_config[0], vsti_config.size());

            vstPlayer.displayEditorModal();
            vstPlayer.getChunk(vsti_config);
        }

        busy = false;

        OnChanged();
    }
}

void Preferences::OnPluginChange(UINT, int, CWindow w)
{
    // t_size vsti_count = _VSTiPlugins.get_size();
    int plugin_selected = (int)::SendMessage(w, CB_GETCURSEL, 0, 0);
//  int plugin_index = -1;
    int plugin = 0;

    if ((plugin_selected >= _ReportedPluginCount) && (plugin_selected < (int)(_ReportedPluginCount + _VSTiPlugins.get_count())))
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
    GetDlgItem(IDC_RESAMPLING).EnableWindow(plugin == 2 || plugin == 4);
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
        GetDlgItem(IDC_FILTER_FLAVOR_TEXT).EnableWindow(enable);
        GetDlgItem(IDC_FILTER_FLAVOR).EnableWindow(enable);
        GetDlgItem(IDC_FILTER_EFFECTS).EnableWindow(enable);
    }

    if (plugin == 3)
    {
        GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(FALSE);
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_SHOW);
    }
    else
    if (_VSTiPlugins.get_count() == 0)
    {
        GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(FALSE);
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_HIDE);
    }
    else
    {
        GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(FALSE);
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_HIDE);
    }

    GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(plugin_selected >= _ReportedPluginCount && plugin < _ReportedPluginCount + _VSTiPlugins.get_count() && _VSTiPlugins[(size_t)(plugin_selected - _ReportedPluginCount)].HasEditor);

    if ((plugin_selected >= _ReportedPluginCount) && (plugin_selected < (int)(_ReportedPluginCount + _VSTiPlugins.get_count())))
    {
        vsti_config = cfg_vst_config[_VSTiPlugins[(size_t)(plugin_selected - _ReportedPluginCount)].Id];
    }

    OnChanged();
}

void Preferences::OnSetFocus(UINT, int, CWindow w)
{
    SetFocus();

    if (w == GetDlgItem(IDC_SOUNDFONT))
    {
        pfc::string8 directory, filename;

        directory = m_soundfont;
        filename = m_soundfont;

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
            m_soundfont = filename;

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
            m_munt_path = path;

            t_size length = m_munt_path.length();

            if (length >= 1 && !pfc::is_path_separator((unsigned int)*(m_munt_path.get_ptr() + length - 1)))
                m_munt_path.add_byte('\\');

            const char * display_path;

            if (length)
                display_path = m_munt_path;
            else
                display_path = click_to_set;

            uSetWindowText(w, display_path);
            OnChanged();
        }
    }
}

void Preferences::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == ID_REFRESH)
    {
        GetDlgItem(IDC_SAMPLERATE).EnableWindow(CfgPluginId || !g_running);

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
    t_uint32 state = preferences_state::resettable | preferences_state::dark_mode_supported;
    if (HasChanged()) state |= preferences_state::changed;
    if (busy) state |= preferences_state::busy;
    return state;
}

void Preferences::reset()
{
    int default_plugin_entry = _PluginIdToIndex[DefaultPluginId];
    int plugin_selected = _PluginPresentMap[default_plugin_entry];

    SendDlgItemMessage(IDC_PLUGIN, CB_SETCURSEL, (WPARAM)plugin_selected);

    if (DefaultPluginId != 2 && DefaultPluginId != 4)
    {
        GetDlgItem(IDC_SOUNDFONT_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_SOUNDFONT).EnableWindow(FALSE);
        GetDlgItem(IDC_RESAMPLING_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_RESAMPLING).EnableWindow(FALSE);
        GetDlgItem(IDC_CACHED_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_CACHED).EnableWindow(FALSE);
    }
    if (DefaultPluginId != 6)
    {
        GetDlgItem(IDC_ADL_BANK_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_BANK).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_CHIPS_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_CHIPS).EnableWindow(FALSE);
        GetDlgItem(IDC_ADL_PANNING).EnableWindow(FALSE);
    }
    if (DefaultPluginId == 3)
    {
        GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(FALSE);
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_SHOW);
    }
    else
    if (_VSTiPlugins.get_count() == 0)
    {
        GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(FALSE);
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_HIDE);
    }
    else
    {
        GetDlgItem(IDC_VST_CONFIGURE).EnableWindow(FALSE);
        GetDlgItem(IDC_MUNT_WARNING).ShowWindow(SW_HIDE);
    }
    if (DefaultPluginId != 9)
    {
        GetDlgItem(IDC_MS_PRESET_TEXT).EnableWindow(FALSE);
        GetDlgItem(IDC_MS_PRESET).EnableWindow(FALSE);
        GetDlgItem(IDC_MS_PANNING).EnableWindow(FALSE);
    }
    {
        bool enable = (DefaultPluginId == 1) || (DefaultPluginId == 2) || (DefaultPluginId == 4) || (DefaultPluginId == 10);
        GetDlgItem(IDC_FILTER_GROUP).EnableWindow(enable);
        GetDlgItem(IDC_FILTER_FLAVOR_TEXT).EnableWindow(enable);
        GetDlgItem(IDC_FILTER_FLAVOR).EnableWindow(enable);
        GetDlgItem(IDC_FILTER_EFFECTS).EnableWindow(enable);
    }

    uSetDlgItemText(m_hWnd, IDC_SOUNDFONT, click_to_set);
    uSetDlgItemText(m_hWnd, IDC_MUNT, click_to_set);

    m_soundfont.reset();
    m_munt_path.reset();

    SetDlgItemInt(IDC_SAMPLERATE, default_cfg_srate, FALSE);

    if (!DefaultPluginId)
    {
        if (g_running)
            GetDlgItem(IDC_SAMPLERATE).EnableWindow(FALSE);
    }

    SendDlgItemMessage(IDC_LOOP, CB_SETCURSEL, default_cfg_loop_type);
    SendDlgItemMessage(IDC_CLOOP, CB_SETCURSEL, default_cfg_loop_type_other);
    SendDlgItemMessage(IDC_THLOOPZ, BM_SETCHECK, default_cfg_thloopz);
    SendDlgItemMessage(IDC_RPGMLOOPZ, BM_SETCHECK, default_cfg_rpgmloopz);
    SendDlgItemMessage(IDC_XMILOOPZ, BM_SETCHECK, default_cfg_xmiloopz);
    SendDlgItemMessage(IDC_FF7LOOPZ, BM_SETCHECK, default_cfg_ff7loopz);
    SendDlgItemMessage(IDC_EMIDI_EX, BM_SETCHECK, default_cfg_emidi_exclusion);
    SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_SETCHECK, default_cfg_filter_instruments);
    SendDlgItemMessage(IDC_FILTER_BANKS, BM_SETCHECK, default_cfg_filter_banks);
    SendDlgItemMessage(IDC_MS_PANNING, BM_SETCHECK, default_cfg_ms_panning);
    SendDlgItemMessage(IDC_FILTER_FLAVOR, CB_SETCURSEL, default_cfg_midi_flavor);
    SendDlgItemMessage(IDC_FILTER_EFFECTS, BM_SETCHECK, !default_cfg_midi_reverb);

    {
        unsigned bank_selected = 0;

        for (unsigned i = 0; i < m_bank_list.get_count(); ++i)
        {
            if (m_bank_list[i].number == default_cfg_adl_bank)
            {
                bank_selected = i;
                break;
            }
        }

        SendDlgItemMessage(IDC_ADL_BANK, CB_SETCURSEL, bank_selected);
    }

    SendDlgItemMessage(IDC_ADL_PANNING, BM_SETCHECK, default_cfg_adl_panning);
    SetDlgItemInt(IDC_ADL_CHIPS, default_cfg_adl_chips, 0);
//  SendDlgItemMessage( IDC_RECOVER, BM_SETCHECK, default_cfg_recover_tracks );

#ifdef FLUIDSYNTHSUPPORT
    SendDlgItemMessage(IDC_RESAMPLING, CB_SETCURSEL, 2 /* 4 */);
#else
    SendDlgItemMessage(IDC_RESAMPLING, CB_SETCURSEL, default_cfg_resampling);
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

    SendDlgItemMessage(IDC_FILTER_FLAVOR, CB_SETCURSEL, (WPARAM)cfg_midi_flavor);

    vsti_config.resize(0);

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
        cfg_srate = t;
    }

    {
        t = (int)SendDlgItemMessage(IDC_ADL_BANK, CB_GETCURSEL);

        if (t < 0 || t >= (int)m_bank_list.get_count())
            t = 0;

        cfg_adl_bank = m_bank_list[(t_size)t].number;
    }

    {
        t = (int)GetDlgItemInt(IDC_ADL_CHIPS, NULL, FALSE);

        if (t < 1)
            t = 1;
        else
        if (t > 100)
            t = 100;

        SetDlgItemInt(IDC_ADL_CHIPS, (UINT)t, FALSE);

        cfg_adl_chips = t;
    }

    cfg_adl_panning = (t_int32)SendDlgItemMessage(IDC_ADL_PANNING, BM_GETCHECK);
    cfg_munt_gm = (t_int32)SendDlgItemMessage(IDC_MUNT_GM, CB_GETCURSEL);

    {
        unsigned int preset_number = (unsigned int)SendDlgItemMessage(IDC_MS_PRESET, CB_GETCURSEL);

        if (preset_number >= _MSPresets.get_count())
            preset_number = 0;

        const MSPreset & preset = _MSPresets[preset_number];

        cfg_ms_synth = (t_int32)preset.synth;
        cfg_ms_bank = (t_int32)preset.bank;
    }

    {
        int plugin_selected = (int)SendDlgItemMessage(IDC_PLUGIN, CB_GETCURSEL);
        int plugin = -1;

        if (plugin_selected >= _ReportedPluginCount && plugin_selected < (int)(_ReportedPluginCount + _VSTiPlugins.get_count()))
            plugin = 1;
    #ifdef DXISUPPORT
        else if (plugin_selected >= plugins_reported + _VSTiPlugins.get_count())
            plugin = 5;
    #endif
        else
            plugin = _PluginPresentReverseMap[plugin_selected];

        cfg_vst_path = "";

        CfgPluginId = plugin;

        if (plugin == 1)
        {
            cfg_vst_path = _VSTiPlugins[(size_t)(plugin_selected - _ReportedPluginCount)].PathName.c_str();

            cfg_vst_config[_VSTiPlugins[(size_t)(plugin_selected - _ReportedPluginCount)].Id] = vsti_config;
        }
    #ifdef DXISUPPORT
        else if (plugin == 5)
        {
            cfg_dxi_plugin = dxi_plugins[plugin_selected - _VSTiPlugins.get_count() - plugins_reported];
        }
    #endif
    }

    cfg_soundfont_path = m_soundfont;
    cfg_munt_base_path = m_munt_path;

    cfg_loop_type = (t_int32)SendDlgItemMessage(IDC_LOOP, CB_GETCURSEL);
    cfg_loop_type_other = (t_int32)SendDlgItemMessage(IDC_CLOOP, CB_GETCURSEL);
    cfg_thloopz = (t_int32)SendDlgItemMessage(IDC_THLOOPZ, BM_GETCHECK);
    cfg_rpgmloopz = (t_int32)SendDlgItemMessage(IDC_RPGMLOOPZ, BM_GETCHECK);
    cfg_xmiloopz = (t_int32)SendDlgItemMessage(IDC_XMILOOPZ, BM_GETCHECK);
    cfg_ff7loopz = (t_int32)SendDlgItemMessage(IDC_FF7LOOPZ, BM_GETCHECK);
    cfg_emidi_exclusion = (t_int32)SendDlgItemMessage(IDC_EMIDI_EX, BM_GETCHECK);
    cfg_filter_instruments = (t_int32)SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_GETCHECK);
    cfg_filter_banks = (t_int32)SendDlgItemMessage(IDC_FILTER_BANKS, BM_GETCHECK);
    cfg_ms_panning = (t_int32)SendDlgItemMessage(IDC_MS_PANNING, BM_GETCHECK);
//  cfg_recover_tracks = SendDlgItemMessage( IDC_RECOVER, BM_GETCHECK );

#ifdef FLUIDSYNTHSUPPORT
    {
        int interp_method = SendDlgItemMessage(IDC_RESAMPLING, CB_GETCURSEL);
        if (interp_method == 2)
            interp_method = 4;
        else if (interp_method == 3)
            interp_method = 7;
        cfg_fluid_interp_method = interp_method;
    }
#endif

#ifdef BASSMIDISUPPORT
    cfg_resampling = (t_int32)SendDlgItemMessage(IDC_RESAMPLING, CB_GETCURSEL);
#endif
    cfg_midi_flavor = (t_int32)SendDlgItemMessage(IDC_FILTER_FLAVOR, CB_GETCURSEL);
    cfg_midi_reverb = !SendDlgItemMessage(IDC_FILTER_EFFECTS, BM_GETCHECK);

    OnChanged(); // our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool Preferences::HasChanged()
{
    // returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
    bool changed = false;

    if (!changed && GetDlgItemInt(IDC_SAMPLERATE, NULL, FALSE) != (UINT)cfg_srate) changed = true;
    if (!changed && SendDlgItemMessage(IDC_LOOP, CB_GETCURSEL) != cfg_loop_type) changed = true;
    if (!changed && SendDlgItemMessage(IDC_CLOOP, CB_GETCURSEL) != cfg_loop_type_other) changed = true;
    if (!changed && SendDlgItemMessage(IDC_THLOOPZ, BM_GETCHECK) != cfg_thloopz) changed = true;
    if (!changed && SendDlgItemMessage(IDC_RPGMLOOPZ, BM_GETCHECK) != cfg_rpgmloopz) changed = true;
    if (!changed && SendDlgItemMessage(IDC_XMILOOPZ, BM_GETCHECK) != cfg_xmiloopz) changed = true;
    if (!changed && SendDlgItemMessage(IDC_FF7LOOPZ, BM_GETCHECK) != cfg_ff7loopz) changed = true;
    if (!changed && SendDlgItemMessage(IDC_EMIDI_EX, BM_GETCHECK) != cfg_emidi_exclusion) changed = true;
    if (!changed && SendDlgItemMessage(IDC_FILTER_INSTRUMENTS, BM_GETCHECK) != cfg_filter_instruments) changed = true;
    if (!changed && SendDlgItemMessage(IDC_FILTER_BANKS, BM_GETCHECK) != cfg_filter_banks) changed = true;
    if (!changed && SendDlgItemMessage(IDC_MS_PANNING, BM_GETCHECK) != cfg_ms_panning) changed = true;

#ifdef FLUIDSYNTHSUPPORT
    if (!changed)
    {
        int interp_method = SendDlgItemMessage(IDC_RESAMPLING, CB_GETCURSEL);
        if (interp_method == 2)
            interp_method = 4;
        else if (interp_method == 3)
            interp_method = 7;
        if (interp_method != cfg_fluid_interp_method) changed = true;
    }
#endif

#ifdef BASSMIDISUPPORT
    if (!changed && SendDlgItemMessage(IDC_RESAMPLING, CB_GETCURSEL) != cfg_resampling)
        changed = true;
#endif
    if (!changed && SendDlgItemMessage(IDC_FILTER_FLAVOR, CB_GETCURSEL) != cfg_midi_flavor)
        changed = true;

    if (!changed && !SendDlgItemMessage(IDC_FILTER_EFFECTS, BM_GETCHECK) != cfg_midi_reverb)
        changed = true;

    if (!changed)
    {
        unsigned int preset_number = (unsigned int)SendDlgItemMessage(IDC_MS_PRESET, CB_GETCURSEL);

        if (preset_number >= _MSPresets.get_count())
            preset_number = 0;

        const MSPreset & preset = _MSPresets[preset_number];

        if (!(preset.synth == (unsigned int)cfg_ms_synth && preset.bank == (unsigned int)cfg_ms_bank))
            changed = true;
    }

    if (!changed)
    {
        int t = (int)SendDlgItemMessage(IDC_ADL_BANK, CB_GETCURSEL);

        if (t < 0 || t >= (int)m_bank_list.get_count())
            t = 0;

        if (m_bank_list[(t_size)t].number != (int)cfg_adl_bank)
            changed = true;
    }

    if (!changed && GetDlgItemInt(IDC_ADL_CHIPS, NULL, FALSE) != (UINT)cfg_adl_chips)
        changed = true;

    if (!changed && SendDlgItemMessage(IDC_ADL_PANNING, BM_GETCHECK) != cfg_adl_panning)
        changed = true;

    if (!changed && SendDlgItemMessage(IDC_MUNT_GM, CB_GETCURSEL) != cfg_munt_gm)
        changed = true;

    if (!changed)
    {
        int plugin_selected = (int)SendDlgItemMessage(IDC_PLUGIN, CB_GETCURSEL);
        int plugin = -1;

        if ((plugin_selected >= _ReportedPluginCount) && (plugin_selected < _ReportedPluginCount + (int)_VSTiPlugins.get_count()))
            plugin = 1;
    #ifdef DXISUPPORT
        else if (plugin_selected >= plugins_reported + _VSTiPlugins.get_count())
            plugin = 5;
    #endif
        else
            plugin = _PluginPresentReverseMap[plugin_selected];

        if (plugin != CfgPluginId)
            changed = true;

        if (!changed && plugin == 1)
        {
            if (stricmp_utf8(cfg_vst_path, _VSTiPlugins[(size_t)(plugin_selected - _ReportedPluginCount)].PathName.c_str()))
                changed = true;

            if (!changed)
            {
                t_uint32 unique_id = _VSTiPlugins[(size_t)(plugin_selected - _ReportedPluginCount)].Id;

                if (vsti_config.size() != cfg_vst_config[unique_id].size() || (vsti_config.size() && memcmp(&vsti_config[0], &cfg_vst_config[unique_id][0], vsti_config.size())))
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
        if (stricmp_utf8(m_soundfont, cfg_soundfont_path))
            changed = true;
    }

    if (!changed)
    {
        if (stricmp_utf8(m_munt_path, cfg_munt_base_path))
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
        _VSTiPlugins.set_size(0);

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
                {
                    pfc::string8 Text;

                    Text << "Examining \"" << PathName << "\"...";

                    console::print(Text);
                }

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

                    _VSTiPlugins.append_single(Plugin);
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
