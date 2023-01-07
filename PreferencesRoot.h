
/** $VER: Preferences.h (2023.01.07) **/

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
        COMMAND_HANDLER_EX(IDC_PLAYER_TYPE, CBN_SELCHANGE, OnPlugInChange)

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
        #pragma endregion

        #pragma region("Miscellaneous")
        COMMAND_HANDLER_EX(IDC_EMIDI_EX, BN_CLICKED, OnButtonClick)

        COMMAND_HANDLER_EX(IDC_FILTER_INSTRUMENTS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_FILTER_BANKS, BN_CLICKED, OnButtonClick)

        COMMAND_HANDLER_EX(IDC_RPGMLOOPZ, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_XMILOOPZ, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_THLOOPZ, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_FF7LOOPZ, BN_CLICKED, OnButtonClick)
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

    void OnPlugInChange(UINT, int, CWindow);
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
#ifdef BASSMIDISUPPORT
    pfc::string8 _CacheStatusText;
    pfc::string8 _CacheStatusTextCurrent;
#endif
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
