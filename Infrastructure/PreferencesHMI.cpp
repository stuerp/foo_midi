
/** $VER: PreferencesHMI.cpp (2025.06.22) P. Stuer **/

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

#include "Resource.h"

#include "Configuration.h"

#pragma hdrstop

#pragma warning(disable: 4820) // x bytes padding added after data member

/// <summary>
/// Implements a preferences page.
/// </summary>
class DialogPage : public CDialogImpl<DialogPage>, public preferences_page_instance
{
public:
    DialogPage(preferences_page_callback::ptr callback) noexcept : _Callback(callback)
    {
    }

    DialogPage(const DialogPage &) = delete;
    DialogPage(const DialogPage &&) = delete;
    DialogPage & operator=(const DialogPage &) = delete;
    DialogPage & operator=(DialogPage &&) = delete;

    virtual ~DialogPage() { };

    #pragma region("preferences_page_instance")

    t_uint32 get_state() final;
    void apply() final;
    void reset() final;

    #pragma endregion

    // WTL message map
    BEGIN_MSG_MAP_EX(DialogPage)
        MSG_WM_INITDIALOG(OnInitDialog)
/*
        COMMAND_HANDLER_EX(IDC_VST_PATH, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_VST_PATH_SELECT, BN_CLICKED, OnButtonClicked)
*/
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

    enum
    {
        IDD = IDD_PREFERENCES_HMI
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
t_uint32 DialogPage::get_state()
{
    t_uint32 State = preferences_state::resettable | preferences_state::dark_mode_supported;

    if (HasChanged())
        State |= preferences_state::changed;

    return State;
}

/// <summary>
/// Applies the changes to the preferences.
/// </summary>
void DialogPage::apply()
{
//  AdvCfgVSTiPluginDirectoryPath.set(_VSTiPluginDirectoryPath);

    OnChanged();
}

/// <summary>
/// Resets this page's content to the default values. Does not apply any changes - lets user preview the changes before hitting "apply".
/// </summary>
void DialogPage::reset()
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
BOOL DialogPage::OnInitDialog(CWindow window, LPARAM) noexcept
{
    _DarkModeHooks.AddDialogWithControls(*this);

    UpdateDialog();

    return FALSE;
}

/// <summary>
/// Handles the notification when a control loses focus.
/// </summary>
void DialogPage::OnEditChange(UINT code, int id, CWindow) noexcept
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
void DialogPage::OnButtonClicked(UINT, int id, CWindow) noexcept
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
bool DialogPage::HasChanged() const noexcept
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
void DialogPage::OnChanged() const noexcept
{
    _Callback->on_state_changed();
}

/// <summary>
/// Updates the appearance of the dialog according to the values of the settings.
/// </summary>
void DialogPage::UpdateDialog() const noexcept
{
//  ::uSetDlgItemText(m_hWnd, IDC_VST_PATH, _VSTiPluginDirectoryPath);
}

#pragma endregion

const GUID PreferencesHMIPageGUID = { 0x089187c1, 0x6344, 0x4c86, { 0xa8, 0x94, 0xf5, 0x95, 0x1e, 0xf9, 0xdc, 0x6c } };

class Page : public preferences_page_impl<DialogPage>
{
public:
    Page() noexcept { };

    Page(const Page &) = delete;
    Page(const Page &&) = delete;
    Page & operator=(const Page &) = delete;
    Page & operator=(Page &&) = delete;

    virtual ~Page() noexcept { };

    const char * get_name() noexcept
    {
        return IDD_PREFERENCES_HMI_NAME;
    }

    GUID get_guid() noexcept
    {
        return PreferencesHMIPageGUID;
    }

    GUID get_parent_guid() noexcept
    {
        return PreferencesPageGUID;
    }
};

//static preferences_page_factory_t<Page> PreferencesPageFactory;
