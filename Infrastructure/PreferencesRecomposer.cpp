
/** $VER: PreferencesRecomposer.cpp (2024.05.16) P. Stuer **/

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

#include "resource.h"

#include "Configuration.h"

#pragma hdrstop

#pragma warning(disable: 4820) // x bytes padding added after data member

/// <summary>
/// Implements a preferences page.
/// </summary>
class PreferencesRecomposerPage : public CDialogImpl<PreferencesRecomposerPage>, public preferences_page_instance
{
public:
    PreferencesRecomposerPage(preferences_page_callback::ptr callback) noexcept : _Callback(callback)
    {
    }

    PreferencesRecomposerPage(const PreferencesRecomposerPage &) = delete;
    PreferencesRecomposerPage(const PreferencesRecomposerPage &&) = delete;
    PreferencesRecomposerPage & operator=(const PreferencesRecomposerPage &) = delete;
    PreferencesRecomposerPage & operator=(PreferencesRecomposerPage &&) = delete;

    virtual ~PreferencesRecomposerPage() { };

    #pragma region("preferences_page_instance")

    t_uint32 get_state() final;
    void apply() final;
    void reset() final;

    #pragma endregion

    // WTL message map
    BEGIN_MSG_MAP_EX(PreferencesRecomposerPage)
        MSG_WM_INITDIALOG(OnInitDialog)
/*
        COMMAND_HANDLER_EX(IDC_VST_PATH, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_VST_PATH_SELECT, BN_CLICKED, OnButtonClicked)
*/
    END_MSG_MAP()

    enum
    {
        IDD = IDD_PREFERENCES_RECOMPOSER
    };

private:
    BOOL OnInitDialog(CWindow, LPARAM) noexcept;

    void OnEditChange(UINT, int, CWindow) noexcept;
    void OnButtonClicked(UINT, int, CWindow) noexcept;

    void UpdateDialog() const noexcept;

    bool HasChanged() const noexcept;
    void OnChanged() const noexcept;

private:
    const preferences_page_callback::ptr _Callback;

    fb2k::CCoreDarkModeHooks _DarkModeHooks;
};

#pragma region("preferences_page_instance")

/// <summary>
/// Gets the state of the Preference dialog.
/// </summary>
t_uint32 PreferencesRecomposerPage::get_state()
{
    t_uint32 State = preferences_state::resettable | preferences_state::dark_mode_supported;

    if (HasChanged())
        State |= preferences_state::changed;

    return State;
}

/// <summary>
/// Applies the changes to the preferences.
/// </summary>
void PreferencesRecomposerPage::apply()
{
//  AdvCfgVSTiPluginDirectoryPath.set(_VSTiPluginDirectoryPath);

    OnChanged();
}

/// <summary>
/// Resets this page's content to the default values. Does not apply any changes - lets user preview the changes before hitting "apply".
/// </summary>
void PreferencesRecomposerPage::reset()
{
//  _VSTiPluginDirectoryPath.reset();

    UpdateDialog();

    OnChanged();
}

#pragma endregion

#pragma region("CDialogImpl")

/// <summary>
/// Initializes the dialog.
/// </summary>
BOOL PreferencesRecomposerPage::OnInitDialog(CWindow, LPARAM) noexcept
{
    _DarkModeHooks.AddDialogWithControls(*this);

    UpdateDialog();

    return FALSE;
}

/// <summary>
/// Handles the notification when a control loses focus.
/// </summary>
void PreferencesRecomposerPage::OnEditChange(UINT code, int id, CWindow) noexcept
{
    if (code != EN_CHANGE)
        return;

    WCHAR Text[MAX_PATH];

    GetDlgItemText(id, Text, _countof(Text));
/*
    switch (id)
    {
        case IDC_VST_PATH:
            _VSTiPluginDirectoryPath = pfc::utf8FromWide(Text);
            break;

        default:
            return;
    }
*/
    OnChanged();
}

/// <summary>
/// Handles a click on a button.
/// </summary>
void PreferencesRecomposerPage::OnButtonClicked(UINT, int id, CWindow) noexcept
{
/*
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
*/
}

/// <summary>
/// Returns whether our dialog content is different from the current configuration (whether the Apply button should be enabled or not)
/// </summary>
bool PreferencesRecomposerPage::HasChanged() const noexcept
{
    bool HasChanged = false;
/*
    if (_VSTiPluginDirectoryPath != DirectoryPath)
        HasChanged = true;

    GetDlgItem(IDC_PATHS_MESSAGE).ShowWindow(HasChanged ? SW_SHOW : SW_HIDE);
*/
    return HasChanged;
}

/// <summary>
/// Tells the host that our state has changed to enable/disable the Apply button appropriately.
/// </summary>
void PreferencesRecomposerPage::OnChanged() const noexcept
{
    _Callback->on_state_changed();
}

/// <summary>
/// Updates the appearance of the dialog according to the values of the settings.
/// </summary>
void PreferencesRecomposerPage::UpdateDialog() const noexcept
{
//  ::uSetDlgItemText(m_hWnd, IDC_VST_PATH, _VSTiPluginDirectoryPath);
}

#pragma endregion

class PreferencesRecomposerPageImpl : public preferences_page_impl<PreferencesRecomposerPage>
{
public:
    PreferencesRecomposerPageImpl() noexcept { };

    PreferencesRecomposerPageImpl(const PreferencesRecomposerPageImpl &) = delete;
    PreferencesRecomposerPageImpl(const PreferencesRecomposerPageImpl &&) = delete;
    PreferencesRecomposerPageImpl & operator=(const PreferencesRecomposerPageImpl &) = delete;
    PreferencesRecomposerPageImpl & operator=(PreferencesRecomposerPageImpl &&) = delete;

    virtual ~PreferencesRecomposerPageImpl() noexcept { };

    const char * get_name() noexcept
    {
        return IDD_PREFERENCES_RECOMPOSER_NAME;
    }

    GUID get_guid() noexcept
    {
        return PreferencesRecomposerPageGUID;
    }

    GUID get_parent_guid() noexcept
    {
        return PreferencesPageGUID;
    }
};

static preferences_page_factory_t<PreferencesRecomposerPageImpl> PreferencesPageFactory;
