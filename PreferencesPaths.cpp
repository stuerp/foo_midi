
/** $VER: PreferencesPaths.cpp (2023.01.06) **/

#pragma warning(disable: 5045 26481 26485)

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

#include "resource.h"

#include "Configuration.h"
#include "MIDIPreset.h"

#include "ADLPlayer.h"
#include "BMPlayer.h"
#include "NukePlayer.h"
#include "VSTiPlayer.h"

#pragma hdrstop

/// <summary>
/// Implements a preferences page.
/// </summary>
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

#pragma region("preferences_page_instance")
/// <summary>
/// Gets the state of the Preference dialog.
/// </summary>
t_uint32 PreferencesPathsPage::get_state()
{
    t_uint32 State = preferences_state::resettable | preferences_state::dark_mode_supported;

    if (HasChanged())
        State |= preferences_state::changed;

    return State;
}

/// <summary>
/// Applies the changes to the preferences.
/// </summary>
void PreferencesPathsPage::apply()
{
    AdvCfgVSTiPluginDirectoryPath.set(_VSTiPluginDirectoryPath);
    CfgSoundFontFilePath = _SoundFontFilePath;
    CfgMuntDirectoryPath = _MT32ROMDirectoryPath;
    AdvCfgSecretSauceDirectoryPath.set(_SecretSauceDirectoryPath);

    OnChanged();
}

/// <summary>
/// Resets this page's content to the default values. Does not apply any changes - lets user preview the changes before hitting "apply".
/// </summary>
void PreferencesPathsPage::reset()
{
    _VSTiPluginDirectoryPath.reset();
    _SoundFontFilePath.reset();
    _MT32ROMDirectoryPath.reset();
    _SecretSauceDirectoryPath.reset();

    UpdateDialog();

    OnChanged();
}
#pragma endregion

#pragma region("CDialogImpl")
/// <summary>
/// Initializes the dialog.
/// </summary>
BOOL PreferencesPathsPage::OnInitDialog(CWindow, LPARAM) noexcept
{
    _DarkModeHooks.AddDialogWithControls(*this);

    UpdateDialog();

    return FALSE;
}

/// <summary>
/// Handles the notification when a control loses focus.
/// </summary>
void PreferencesPathsPage::OnLostFocus(UINT code, int id, CWindow) noexcept
{
    if (code != EN_KILLFOCUS)
        return;

    WCHAR Text[MAX_PATH];

    GetDlgItemText(id, Text, _countof(Text));

    switch (id)
    {
        case IDC_VST_PATH:
            _VSTiPluginDirectoryPath = pfc::utf8FromWide(Text);
            break;

        case IDC_SOUNDFONT_FILE_PATH:
            _SoundFontFilePath = pfc::utf8FromWide(Text);
            break;

        case IDC_MUNT_FILE_PATH:
            _MT32ROMDirectoryPath = pfc::utf8FromWide(Text);
            break;

        case IDC_SECRET_SAUCE_PATH:
            _SecretSauceDirectoryPath = pfc::utf8FromWide(Text);
            break;

        default:
            return;
    }

    OnChanged();
}

/// <summary>
/// Handles a click on a button.
/// </summary>
void PreferencesPathsPage::OnButtonClicked(UINT, int id, CWindow) noexcept
{
    if (id == IDC_VST_PATH_SELECT)
    {
        pfc::string8 DirectoryPath = _VSTiPluginDirectoryPath;

        if (::uBrowseForFolder(m_hWnd, "Locate VSTi plug-ins...", DirectoryPath))
        {
            _VSTiPluginDirectoryPath = DirectoryPath;

            pfc::wstringLite w = pfc::wideFromUTF8(DirectoryPath);

            SetDlgItemText(IDC_VST_PATH, w);

            OnChanged();
        }
    }
    else
    if (id == IDC_SOUNDFONT_FILE_PATH_SELECT)
    {
        pfc::string8 DirectoryPath = _SoundFontFilePath;

        DirectoryPath.truncate_filename();

        pfc::string8 FilePath = _SoundFontFilePath;

        if (::uGetOpenFileName(m_hWnd, "SoundFont and list files|*.sf2;*.sf3;*.sflist"
        #ifdef SF2PACK
            "*.sf2pack;*.sfogg;"
        #endif
            ";*.json"

            "*.sflist|SoundFont files|*.sf2;*.sf3"
        #ifdef SF2PACK
            ";*.sf2pack;*.sfogg;"
        #endif

            "|SoundFont list files|*.sflist;*.json"
            ,
            0, "sf2", "Choose a SoundFont bank or list...", DirectoryPath, FilePath, FALSE))
        {
            _SoundFontFilePath = FilePath;

            pfc::wstringLite w = pfc::wideFromUTF8(FilePath);

            SetDlgItemText(IDC_SOUNDFONT_FILE_PATH, w);

            OnChanged();
        }
    }
    else
    if (id == IDC_MUNT_FILE_PATH_SELECT)
    {
        pfc::string8 DirectoryPath = _MT32ROMDirectoryPath;

        if (::uBrowseForFolder(m_hWnd, "Locate MT-32 or CM-32L ROM sets...", DirectoryPath))
        {
            _MT32ROMDirectoryPath = DirectoryPath;

            pfc::wstringLite w = pfc::wideFromUTF8(DirectoryPath);

            SetDlgItemText(IDC_MUNT_FILE_PATH, w);

            OnChanged();
        }
    }
    else
    if (id == IDC_SECRET_SAUCE_PATH_SELECT)
    {
        pfc::string8 DirectoryPath = _SecretSauceDirectoryPath;

        if (::uBrowseForFolder(m_hWnd, "Locate Secret Sauce...", DirectoryPath))
        {
            _SecretSauceDirectoryPath = DirectoryPath;

            pfc::wstringLite w = pfc::wideFromUTF8(DirectoryPath);

            SetDlgItemText(IDC_SECRET_SAUCE_PATH, w);

            OnChanged();
        }
    }
}

/// <summary>
/// Returns whether our dialog content is different from the current configuration (whether the Apply button should be enabled or not)
/// </summary>
bool PreferencesPathsPage::HasChanged() const noexcept
{
    pfc::string8 DirectoryPath;

    AdvCfgVSTiPluginDirectoryPath.get(DirectoryPath);

    if (_VSTiPluginDirectoryPath != DirectoryPath)
        return true;

    if (_SoundFontFilePath != CfgSoundFontFilePath)
        return true;

    if (_MT32ROMDirectoryPath != CfgMuntDirectoryPath)
        return true;

    AdvCfgSecretSauceDirectoryPath.get(DirectoryPath);

    if (_SecretSauceDirectoryPath != DirectoryPath)
        return true;

    return false;
}

/// <summary>
/// Tells the host that our state has changed to enable/disable the Apply button appropriately.
/// </summary>
void PreferencesPathsPage::OnChanged() const noexcept
{
    _Callback->on_state_changed();
}

/// <summary>
/// Updates the appearance of the dialog according to the values of the settings.
/// </summary>
void PreferencesPathsPage::UpdateDialog() const noexcept
{
    ::uSetDlgItemText(m_hWnd, IDC_VST_PATH, _VSTiPluginDirectoryPath);
    ::uSetDlgItemText(m_hWnd, IDC_SOUNDFONT_FILE_PATH, _SoundFontFilePath);
    ::uSetDlgItemText(m_hWnd, IDC_MUNT_FILE_PATH, _MT32ROMDirectoryPath);
    ::uSetDlgItemText(m_hWnd, IDC_SECRET_SAUCE_PATH, _SecretSauceDirectoryPath);
}
#pragma endregion

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

static preferences_page_factory_t<PreferencesPathsPageImpl> PreferencesPageFactory;
