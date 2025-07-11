
/** $VER: PreferencesPaths.cpp (2025.07.11) P. stuer **/

#include "pch.h"

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
#include "Preset.h"

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
        _VSTiPlugInDirectoryPath  = CfgVSTiPlugInDirectoryPath;
        _VSTiXGPlugInFilePath     = CfgVSTiXGPlugInFilePath;
        _CLAPPlugInDirectoryPath  = CfgCLAPPlugInDirectoryPath;
        _SoundFontFilePath        = CfgSoundFontFilePath;
        _MT32ROMDirectoryPath     = CfgMT32ROMDirectoryPath;
        _SecretSauceDirectoryPath = CfgSecretSauceDirectoryPath;
        _FluidSynthDirectoryPath  = CfgFluidSynthDirectoryPath;
        _FluidSynthConfigFilePath = CfgFluidSynthConfigFilePath;
        _ProgramsFilePath         = CfgProgramsFilePath;
    }

    PreferencesPathsPage(const PreferencesPathsPage&) = delete;
    PreferencesPathsPage(const PreferencesPathsPage&&) = delete;
    PreferencesPathsPage& operator=(const PreferencesPathsPage&) = delete;
    PreferencesPathsPage& operator=(PreferencesPathsPage&&) = delete;

    virtual ~PreferencesPathsPage() { };

    #pragma region preferences_page_instance
    t_uint32 get_state() final;
    void apply() final;
    void reset() final;
    #pragma endregion

    // WTL message map
    BEGIN_MSG_MAP_EX(PreferencesPathsPage)
        MSG_WM_INITDIALOG(OnInitDialog)

        COMMAND_HANDLER_EX(IDC_VSTi_PATH, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_VSTi_PATH_SELECT, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_VSTi_XG_FILE_PATH, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_VSTi_XG_FILE_PATH_SELECT, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_CLAP_PATH, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_CLAP_PATH_SELECT, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_SOUNDFONT_FILE_PATH, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_SOUNDFONT_FILE_PATH_SELECT, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_MT32EMU_FILE_PATH, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_MT32EMU_FILE_PATH_SELECT, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_SECRET_SAUCE_PATH, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_SECRET_SAUCE_PATH_SELECT, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_FLUIDSYNTH_PATH, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_FLUIDSYNTH_PATH_SELECT, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_FLUIDSYNTH_CONFIG_PATH, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_FLUIDSYNTH_CONFIG_PATH_SELECT, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_PROGRAMS_FILE_PATH, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_PROGRAMS_FILE_PATH_SELECT, BN_CLICKED, OnButtonClicked)
    END_MSG_MAP()

    enum
    {
        IDD = IDD_PREFERENCES_PATHS
    };

private:
    BOOL OnInitDialog(CWindow, LPARAM) noexcept;

    void OnEditChange(UINT, int, CWindow) noexcept;
    void OnButtonClicked(UINT, int, CWindow) noexcept;

    void UpdateDialog() const noexcept;

    bool HasChanged() const noexcept;
    void OnChanged() const noexcept;

private:
    // VSTi
    pfc::string _VSTiPlugInDirectoryPath;
    pfc::string _VSTiXGPlugInFilePath;

    // CLAP
    pfc::string _CLAPPlugInDirectoryPath;

    // SoundFont
    pfc::string _SoundFontFilePath;

    // LibMT32Emu
    pfc::string _MT32ROMDirectoryPath;

    // Secret Sauce
    pfc::string _SecretSauceDirectoryPath;

    // FluidSynth
    pfc::string _FluidSynthDirectoryPath;
    pfc::string _FluidSynthConfigFilePath;

    // FMMIDI
    pfc::string _ProgramsFilePath;

    const preferences_page_callback::ptr _Callback;

    fb2k::CCoreDarkModeHooks _DarkModeHooks;
};

#pragma region preferences_page_instance
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
    CfgVSTiPlugInDirectoryPath  = _VSTiPlugInDirectoryPath;
    CfgVSTiXGPlugInFilePath     = _VSTiXGPlugInFilePath;
    CfgCLAPPlugInDirectoryPath  = _CLAPPlugInDirectoryPath;
    CfgSoundFontFilePath        = _SoundFontFilePath;
    CfgMT32ROMDirectoryPath     = _MT32ROMDirectoryPath;
    CfgSecretSauceDirectoryPath = _SecretSauceDirectoryPath;
    CfgFluidSynthDirectoryPath  = _FluidSynthDirectoryPath;
    CfgFluidSynthConfigFilePath = _FluidSynthConfigFilePath;
    CfgProgramsFilePath         = _ProgramsFilePath;

    OnChanged();
}

/// <summary>
/// Resets this page's content to the default values. Does not apply any changes - lets user preview the changes before hitting "apply".
/// </summary>
void PreferencesPathsPage::reset()
{
    _VSTiPlugInDirectoryPath.reset();
    _CLAPPlugInDirectoryPath.reset();
    _SoundFontFilePath.reset();
    _MT32ROMDirectoryPath.reset();
    _SecretSauceDirectoryPath.reset();
    _FluidSynthDirectoryPath.reset();
    _ProgramsFilePath.reset();

    UpdateDialog();

    OnChanged();
}
#pragma endregion

#pragma region CDialogImpl
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
void PreferencesPathsPage::OnEditChange(UINT code, int id, CWindow) noexcept
{
    if (code != EN_CHANGE)
        return;

    WCHAR Text[MAX_PATH];

    GetDlgItemText(id, Text, _countof(Text));

    switch (id)
    {
        case IDC_VSTi_PATH:
            _VSTiPlugInDirectoryPath = pfc::utf8FromWide(Text);
            break;

        case IDC_VSTi_XG_FILE_PATH:
            _VSTiXGPlugInFilePath = pfc::utf8FromWide(Text);
            break;

        case IDC_CLAP_PATH:
            _CLAPPlugInDirectoryPath = pfc::utf8FromWide(Text);
            break;

        case IDC_SOUNDFONT_FILE_PATH:
            _SoundFontFilePath = pfc::utf8FromWide(Text);
            break;

        case IDC_MT32EMU_FILE_PATH:
            _MT32ROMDirectoryPath = pfc::utf8FromWide(Text);
            break;

        case IDC_SECRET_SAUCE_PATH:
            _SecretSauceDirectoryPath = pfc::utf8FromWide(Text);
            break;

        case IDC_FLUIDSYNTH_PATH:
            _FluidSynthDirectoryPath = pfc::utf8FromWide(Text);
            break;

        case IDC_FLUIDSYNTH_CONFIG_PATH:
            _FluidSynthConfigFilePath = pfc::utf8FromWide(Text);
            break;

        case IDC_PROGRAMS_FILE_PATH:
            _ProgramsFilePath = pfc::utf8FromWide(Text);
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
    switch (id)
    {
        case IDC_VSTi_PATH_SELECT:
        {
            pfc::string DirectoryPath = _VSTiPlugInDirectoryPath;

            if (::uBrowseForFolder(m_hWnd, "Locate VSTi plug-ins...", DirectoryPath))
            {
                _VSTiPlugInDirectoryPath = DirectoryPath;

                pfc::wstringLite w = pfc::wideFromUTF8(DirectoryPath);

                SetDlgItemText(IDC_VSTi_PATH, w);

                OnChanged();
            }
            break;
        }

        case IDC_VSTi_XG_FILE_PATH_SELECT:
        {
            pfc::string DirectoryPath = _VSTiXGPlugInFilePath;

            DirectoryPath.truncate_filename();

            pfc::string FilePath = _VSTiXGPlugInFilePath;

            if (::uGetOpenFileName(m_hWnd, "All files|*.*"
                ,
                0, "dll", "Choose the plug-in file...", DirectoryPath, FilePath, FALSE))
            {
                _VSTiXGPlugInFilePath = FilePath;

                pfc::wstringLite w = pfc::wideFromUTF8(FilePath);

                SetDlgItemText(IDC_VSTi_XG_FILE_PATH, w);

                OnChanged();
            }
            break;
        }

        case IDC_SOUNDFONT_FILE_PATH_SELECT:
        {
            pfc::string DirectoryPath = _SoundFontFilePath;

            DirectoryPath.truncate_filename();

            pfc::string FilePath = _SoundFontFilePath;

            if (::uGetOpenFileName(m_hWnd,
                    "SoundFont and list files|*.sf2;*.sf3;*.sf2pack;*.sfogg;*.sflist;*.json|"
                    "SoundFont files|*.sf2;*.sf3;*.sf2pack;*.sfogg|"
                    "SoundFont list files|*.sflist;*.json"
                ,
                0, "sf2", "Choose a SoundFont bank or list...", DirectoryPath, FilePath, FALSE))
            {
                _SoundFontFilePath = FilePath;

                pfc::wstringLite w = pfc::wideFromUTF8(FilePath);

                SetDlgItemText(IDC_SOUNDFONT_FILE_PATH, w);

                OnChanged();
            }
            break;
        }

        case IDC_MT32EMU_FILE_PATH_SELECT:
        {
            pfc::string DirectoryPath = _MT32ROMDirectoryPath;

            if (::uBrowseForFolder(m_hWnd, "Locate MT-32 or CM-32L ROM sets...", DirectoryPath))
            {
                _MT32ROMDirectoryPath = DirectoryPath;

                pfc::wstringLite w = pfc::wideFromUTF8(DirectoryPath);

                SetDlgItemText(IDC_MT32EMU_FILE_PATH, w);

                OnChanged();
            }
            break;
        }

        case IDC_SECRET_SAUCE_PATH_SELECT:
        {
            pfc::string DirectoryPath = _SecretSauceDirectoryPath;

            if (::uBrowseForFolder(m_hWnd, "Locate Secret Sauce...", DirectoryPath))
            {
                _SecretSauceDirectoryPath = DirectoryPath;

                pfc::wstringLite w = pfc::wideFromUTF8(DirectoryPath);

                SetDlgItemText(IDC_SECRET_SAUCE_PATH, w);

                OnChanged();
            }
            break;
        }

        case IDC_FLUIDSYNTH_PATH_SELECT:
        {
            pfc::string DirectoryPath = _FluidSynthDirectoryPath;

            if (::uBrowseForFolder(m_hWnd, "Locate FluidSynth...", DirectoryPath))
            {
                _FluidSynthDirectoryPath = DirectoryPath;

                pfc::wstringLite w = pfc::wideFromUTF8(DirectoryPath);

                SetDlgItemText(IDC_FLUIDSYNTH_PATH, w);

                OnChanged();
            }
            break;
        }

        case IDC_FLUIDSYNTH_CONFIG_PATH_SELECT:
        {
            pfc::string DirectoryPath = _FluidSynthConfigFilePath;

            if (!DirectoryPath.isEmpty())
                DirectoryPath.truncate_filename();
            else
                DirectoryPath = pfc::io::path::getParent(core_api::get_my_full_path());

            pfc::string FilePath = _FluidSynthConfigFilePath;

            if (::uGetOpenFileName(m_hWnd, "FluidSynth configuration files|*.cfg|All files|*.*", 0, "cfg", "Choose a configuration file...", DirectoryPath, FilePath, FALSE))
            {
                _FluidSynthConfigFilePath = FilePath;

                pfc::wstringLite w = pfc::wideFromUTF8(FilePath);

                SetDlgItemText(IDC_FLUIDSYNTH_CONFIG_PATH, w);

                OnChanged();
            }
            break;
        }

        case IDC_PROGRAMS_FILE_PATH_SELECT:
        {
            pfc::string DirectoryPath = _ProgramsFilePath;

            if (!DirectoryPath.isEmpty())
                DirectoryPath.truncate_filename();
            else
                DirectoryPath = pfc::io::path::getParent(core_api::get_my_full_path());

            pfc::string FilePath = _ProgramsFilePath;

            if (::uGetOpenFileName(m_hWnd, "FMMIDI Program files|*.*", 0, "txt", "Choose a Programs file...", DirectoryPath, FilePath, FALSE))
            {
                _ProgramsFilePath = FilePath;

                pfc::wstringLite w = pfc::wideFromUTF8(FilePath);

                SetDlgItemText(IDC_PROGRAMS_FILE_PATH, w);

                OnChanged();
            }
            break;
        }

        case IDC_CLAP_PATH_SELECT:
        {
            pfc::string DirectoryPath = _CLAPPlugInDirectoryPath;

            if (::uBrowseForFolder(m_hWnd, "Locate CLAP plug-ins...", DirectoryPath))
            {
                _CLAPPlugInDirectoryPath = DirectoryPath;

                pfc::wstringLite w = pfc::wideFromUTF8(DirectoryPath);

                SetDlgItemText(IDC_CLAP_PATH, w);

                OnChanged();
            }
            break;
        }
    }
}

/// <summary>
/// Returns whether our dialog content is different from the current configuration (whether the Apply button should be enabled or not)
/// </summary>
bool PreferencesPathsPage::HasChanged() const noexcept
{
    bool HasChanged = false;

    if (_VSTiPlugInDirectoryPath != CfgVSTiPlugInDirectoryPath)
        HasChanged = true;

    if (_VSTiXGPlugInFilePath != CfgVSTiXGPlugInFilePath)
        HasChanged = true;

    if (_CLAPPlugInDirectoryPath != CfgCLAPPlugInDirectoryPath)
        HasChanged = true;

    if (_SoundFontFilePath != CfgSoundFontFilePath)
        HasChanged = true;

    if (_MT32ROMDirectoryPath != CfgMT32ROMDirectoryPath)
        HasChanged = true;

    if (_SecretSauceDirectoryPath != CfgSecretSauceDirectoryPath)
        HasChanged = true;

    if (_FluidSynthDirectoryPath != CfgFluidSynthDirectoryPath)
        HasChanged = true;

    if (_FluidSynthConfigFilePath != CfgFluidSynthConfigFilePath)
        HasChanged = true;

    if (_ProgramsFilePath != CfgProgramsFilePath)
        HasChanged = true;

    GetDlgItem(IDC_PATHS_MESSAGE).ShowWindow(HasChanged ? SW_SHOW : SW_HIDE);

    return HasChanged;
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
    ::uSetDlgItemText(m_hWnd, IDC_VSTi_PATH,              _VSTiPlugInDirectoryPath);
    ::uSetDlgItemText(m_hWnd, IDC_VSTi_XG_FILE_PATH,      _VSTiXGPlugInFilePath);
    ::uSetDlgItemText(m_hWnd, IDC_CLAP_PATH,              _CLAPPlugInDirectoryPath);
    ::uSetDlgItemText(m_hWnd, IDC_SOUNDFONT_FILE_PATH,    _SoundFontFilePath);
    ::uSetDlgItemText(m_hWnd, IDC_MT32EMU_FILE_PATH,      _MT32ROMDirectoryPath);
    ::uSetDlgItemText(m_hWnd, IDC_SECRET_SAUCE_PATH,      _SecretSauceDirectoryPath);
    ::uSetDlgItemText(m_hWnd, IDC_FLUIDSYNTH_PATH,        _FluidSynthDirectoryPath);
    ::uSetDlgItemText(m_hWnd, IDC_FLUIDSYNTH_CONFIG_PATH, _FluidSynthConfigFilePath);
    ::uSetDlgItemText(m_hWnd, IDC_PROGRAMS_FILE_PATH,     _ProgramsFilePath);
}
#pragma endregion

static const GUID PreferencesPathsPageGUID = { 0x9d601e5c, 0xd542, 0x435e, { 0x8a, 0x05, 0x4e, 0x88, 0xd1, 0x4d, 0xa3, 0xed } };

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
