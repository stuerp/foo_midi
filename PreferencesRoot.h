
/** $VER: Preferences.h (2023.01.05) **/

#pragma warning(disable: 5045)

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

#include "resource.h"

#include "Configuration.h"
#include "MIDIPreset.h"

#include "ADLPlayer.h"
#include "BMPlayer.h"
#include "VSTiPlayer.h"

#pragma warning(disable: 4820)

#pragma hdrstop

extern cfg_dropdown_history CfgSampleRateHistory;

class Preferences : public CDialogImpl<Preferences>, public preferences_page_instance
{
public:
    Preferences(preferences_page_callback::ptr callback) noexcept : _Callback(callback), _IsBusy(false) { }

    Preferences(const Preferences&) = delete;
    Preferences(const Preferences&&) = delete;
    Preferences& operator=(const Preferences&) = delete;
    Preferences& operator=(Preferences&&) = delete;

    virtual ~Preferences() { };

    #pragma region("preferences_page_instance")
    t_uint32 get_state() final;
    void apply() final;
    void reset() final;
    #pragma endregion

    // WTL message map
    BEGIN_MSG_MAP_EX(Preferences)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_TIMER(OnTimer)

        COMMAND_HANDLER_EX(IDC_PLUGIN, CBN_SELCHANGE, OnPlugInChange)

        DROPDOWN_HISTORY_HANDLER(IDC_SAMPLERATE, CfgSampleRateHistory)
        COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_EDITCHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_SELCHANGE, OnSelectionChange)

        COMMAND_HANDLER_EX(IDC_VST_PATH, EN_SETFOCUS, OnSetFocus)
        COMMAND_HANDLER_EX(IDC_VST_CONFIGURE, BN_CLICKED, OnButtonConfig)

        COMMAND_HANDLER_EX(IDC_LOOP_PLAYBACK, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_LOOP_OTHER, CBN_SELCHANGE, OnSelectionChange)

        COMMAND_HANDLER_EX(IDC_RPGMLOOPZ, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_XMILOOPZ, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_FF7LOOPZ, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_EMIDI_EX, BN_CLICKED, OnButtonClick)

        COMMAND_HANDLER_EX(IDC_FILTER_INSTRUMENTS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_FILTER_BANKS, BN_CLICKED, OnButtonClick)

        COMMAND_HANDLER_EX(IDC_SOUNDFONT_FILE_PATH, EN_SETFOCUS, OnSetFocus)
        COMMAND_HANDLER_EX(IDC_RESAMPLING_MODE, CBN_SELCHANGE, OnSelectionChange)

        COMMAND_HANDLER_EX(IDC_MUNT_FILE_PATH, EN_SETFOCUS, OnSetFocus)

        COMMAND_HANDLER_EX(IDC_ADL_BANK, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_ADL_CHIPS, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_ADL_CHIPS, CBN_EDITCHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_ADL_PANNING, BN_CLICKED, OnButtonClick)

        COMMAND_HANDLER_EX(IDC_MUNT_GM_SET, CBN_SELCHANGE, OnSelectionChange)

        COMMAND_HANDLER_EX(IDC_NUKE_PRESET, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_NUKE_PANNING, BN_CLICKED, OnButtonClick)

        COMMAND_HANDLER_EX(IDC_MIDI_FLAVOR, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_MIDI_EFFECTS, BN_CLICKED, OnButtonClick)

#ifdef DEBUG_DIALOG
        MSG_WM_CTLCOLORDLG(OnCtlColorDlg)
#endif
    END_MSG_MAP()

    enum
    {
        IDD = IDD_PREFERENCES_ROOT
    };

    const UINT_PTR ID_REFRESH = 1000;

private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnTimer(UINT_PTR);

    void OnPlugInChange(UINT, int, CWindow);
    void OnEditChange(UINT, int, CWindow);
    void OnSelectionChange(UINT, int, CWindow);
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
    const preferences_page_callback::ptr _Callback;

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
    pfc::string8 _VSTiPath;
    pfc::string8 _VSTiSearchPath;

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

#pragma region("SoundFont")
    pfc::string8 _SoundFontPath;
#pragma endregion

#pragma region("BASS MIDI")
#ifdef BASSMIDISUPPORT
    pfc::string8 _CacheStatusText;
    pfc::string8 _CacheStatusTextCurrent;
#endif
#pragma endregion

#pragma region("Munt")
    pfc::string8 _MuntPath;
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
            if (c) return c < 0;
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

    std::map<int, int> _PlugInIdToIndex;
    std::map<int, int> _PluginPresentMap;
    std::map<int, int> _IndexToPlugInId;

    fb2k::CCoreDarkModeHooks _DarkModeHooks;
};

class PreferencesPage : public preferences_page_impl<Preferences>
{
public:
    PreferencesPage() noexcept { };

    PreferencesPage(const PreferencesPage & p_in) = delete;
    PreferencesPage(const PreferencesPage &&) = delete;
    PreferencesPage & operator=(const PreferencesPage &) = delete;
    PreferencesPage & operator=(PreferencesPage &&) = delete;

    virtual ~PreferencesPage() noexcept { };

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
