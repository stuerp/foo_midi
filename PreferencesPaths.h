
/** $VER: PreferencesPaths.h (2023.01.06) **/

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

#pragma hdrstop

#pragma warning(disable: 4820)
class PreferencesPathsPage : public CDialogImpl<PreferencesPathsPage>, public preferences_page_instance
{
public:
    PreferencesPathsPage(preferences_page_callback::ptr callback) noexcept : _Callback(callback)
    {
        AdvCfgVSTiPluginDirectoryPath.get(_VSTiPluginDirectoryPath);
        _SoundFontFilePath = CfgSoundFontFilePath;
        _MT32ROMDirectoryPath = CfgMuntDirectoryPath;
        AdvCfgSecretSauceDirectoryPath.get(_SecretSauceDirectoryPath);
    }

    PreferencesPathsPage(const PreferencesPathsPage&) = delete;
    PreferencesPathsPage(const PreferencesPathsPage&&) = delete;
    PreferencesPathsPage& operator=(const PreferencesPathsPage&) = delete;
    PreferencesPathsPage& operator=(PreferencesPathsPage&&) = delete;

    virtual ~PreferencesPathsPage() { };

    #pragma region("preferences_page_instance")
    t_uint32 get_state() final;
    void apply() final;
    void reset() final;
    #pragma endregion

    // WTL message map
    BEGIN_MSG_MAP_EX(PreferencesPathsPage)
        MSG_WM_INITDIALOG(OnInitDialog)

        COMMAND_HANDLER_EX(IDC_VST_PATH, EN_KILLFOCUS, OnLostFocus)
        COMMAND_HANDLER_EX(IDC_VST_PATH_SELECT, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_SOUNDFONT_FILE_PATH, EN_KILLFOCUS, OnLostFocus)
        COMMAND_HANDLER_EX(IDC_SOUNDFONT_FILE_PATH_SELECT, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_MUNT_FILE_PATH, EN_KILLFOCUS, OnLostFocus)
        COMMAND_HANDLER_EX(IDC_MUNT_FILE_PATH_SELECT, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_SECRET_SAUCE_PATH, EN_KILLFOCUS, OnLostFocus)
        COMMAND_HANDLER_EX(IDC_SECRET_SAUCE_PATH_SELECT, BN_CLICKED, OnButtonClicked)
    END_MSG_MAP()

    enum
    {
        IDD = IDD_PREFERENCES_PATHS
    };

private:
    BOOL OnInitDialog(CWindow, LPARAM) noexcept;

    void OnLostFocus(UINT, int, CWindow) noexcept;
    void OnButtonClicked(UINT, int, CWindow) noexcept;

    void UpdateDialog() const noexcept;

    bool HasChanged() const noexcept;
    void OnChanged() const noexcept;

private:
#pragma region("VSTi")
    pfc::string8 _VSTiPluginDirectoryPath;
#pragma endregion

#pragma region("SoundFont")
    pfc::string8 _SoundFontFilePath;
#pragma endregion

#pragma region("Munt")
    pfc::string8 _MT32ROMDirectoryPath;
#pragma endregion

#pragma region("Secret Sauce")
    pfc::string8 _SecretSauceDirectoryPath;
#pragma endregion

    const preferences_page_callback::ptr _Callback;

    fb2k::CCoreDarkModeHooks _DarkModeHooks;
};

class PreferencesPathsPageImpl : public preferences_page_impl<PreferencesPathsPage>
{
public:
    PreferencesPathsPageImpl() noexcept { };

    PreferencesPathsPageImpl(const PreferencesPathsPageImpl &) = delete;
    PreferencesPathsPageImpl(const PreferencesPathsPageImpl &&) = delete;
    PreferencesPathsPageImpl & operator=(const PreferencesPathsPageImpl &) = delete;
    PreferencesPathsPageImpl & operator=(PreferencesPathsPageImpl &&) = delete;

    virtual ~PreferencesPathsPageImpl() noexcept { };

    const char * get_name() noexcept
    {
        return IDD_PREFERENCES_PATHS_NAME;
    }

    GUID get_guid() noexcept
    {
        return PreferencesPathsPageGUID;
    }

    GUID get_parent_guid() noexcept
    {
        return PreferencesPageGUID;
    }
};
