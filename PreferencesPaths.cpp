
/** $VER: PreferencesPaths.cpp (2023.01.06) **/

#pragma warning(disable: 5045 26481 26485)

#include "PreferencesPaths.h"

#include "NukePlayer.h"

#include <pfc/pathUtils.h>

//#define DEBUG_DIALOG

#pragma hdrstop

extern char _DLLFileName[];
extern volatile int _IsRunning;

static const char * DefaultPathMessage = "Click to set.";

#pragma region("preferences_page_instance")
/// <summary>
/// Gets the state of the Preference dialog.
/// </summary>
t_uint32 PreferencesPaths::get_state()
{
    t_uint32 State = preferences_state::resettable | preferences_state::dark_mode_supported;

    if (HasChanged())
        State |= preferences_state::changed;

    return State;
}

void PreferencesPaths::apply()
{
    CfgVSTiPluginDirectoryPath.set(_VSTiPluginDirectoryPath);
    CfgSoundFontFilePath = _SoundFontFilePath;
    CfgMuntDirectoryPath = _MT32ROMDirectoryPath;

    OnChanged();
}

void PreferencesPaths::reset()
{
    ::uSetDlgItemText(m_hWnd, IDC_VST_PATH_SELECT, DefaultPathMessage);
    ::uSetDlgItemText(m_hWnd, IDC_SOUNDFONT_FILE_PATH_SELECT, DefaultPathMessage);
    ::uSetDlgItemText(m_hWnd, IDC_MUNT_FILE_PATH_SELECT, DefaultPathMessage);

    _VSTiPluginDirectoryPath.reset();
    _SoundFontFilePath.reset();
    _MT32ROMDirectoryPath.reset();

    OnChanged();
}
#pragma endregion

#pragma region("CDialogImpl")
/// <summary>
/// Initializes the dialog.
/// </summary>
BOOL PreferencesPaths::OnInitDialog(CWindow, LPARAM)
{
#ifdef DEBUG_DIALOG
    long BaseUnits = ::GetDialogBaseUnits();

    int TemplateUnitX = ::MulDiv(498, 4, LOWORD(BaseUnits));
    int TemplateUnitY = ::MulDiv(468, 8, HIWORD(BaseUnits));
#endif

 #pragma region("VSTi")
    {
        CfgVSTiPluginDirectoryPath.get(_VSTiPluginDirectoryPath);

        ::uSetDlgItemText(m_hWnd, IDC_VST_PATH, !_VSTiPluginDirectoryPath.is_empty() ? _VSTiPluginDirectoryPath : DefaultPathMessage);
    }
#pragma endregion

#pragma region("SoundFont")
    {
        _SoundFontFilePath = CfgSoundFontFilePath;

        ::uSetDlgItemText(m_hWnd, IDC_SOUNDFONT_FILE_PATH, !_SoundFontFilePath.is_empty() ? _SoundFontFilePath : DefaultPathMessage);
    }
#pragma endregion

#pragma region("Munt")
    {
        _MT32ROMDirectoryPath = CfgMuntDirectoryPath;

        ::uSetDlgItemText(m_hWnd, IDC_MUNT_FILE_PATH, !_MT32ROMDirectoryPath.is_empty() ? _MT32ROMDirectoryPath : DefaultPathMessage);
    }
#pragma endregion

    _DarkModeHooks.AddDialogWithControls(*this);

    return FALSE;
}

void PreferencesPaths::OnButtonClicked(UINT, int, CWindow w)
{
    if (w == GetDlgItem(IDC_VST_PATH_SELECT))
    {
        pfc::string8 DirectoryPath = _VSTiPluginDirectoryPath;

        if (::uBrowseForFolder(m_hWnd, "Locate VSTi plug-ins...", DirectoryPath))
        {
            _VSTiPluginDirectoryPath = DirectoryPath;

            ::uSetWindowText(w, !_VSTiPluginDirectoryPath.is_empty() ? _VSTiPluginDirectoryPath : DefaultPathMessage);

            OnChanged();
        }
    }
    else
    if (w == GetDlgItem(IDC_SOUNDFONT_FILE_PATH_SELECT))
    {
        pfc::string8 DirectoryPath = _SoundFontFilePath;

        DirectoryPath.truncate_filename();

        pfc::string8 FilePath = _SoundFontFilePath;

        if (::uGetOpenFileName(m_hWnd, "SoundFont and list files|*.sf2;*.sf3;*.sflist"
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
            0, "sf2", "Choose a SoundFont bank or list...", DirectoryPath, FilePath, FALSE))
        {
            _SoundFontFilePath = FilePath;

            ::uSetWindowText(w, !_SoundFontFilePath.isEmpty() ? _SoundFontFilePath : DefaultPathMessage);

            OnChanged();
        }
    }
    else
    if (w == GetDlgItem(IDC_MUNT_FILE_PATH_SELECT))
    {
        pfc::string8 DirectoryPath = _MT32ROMDirectoryPath;

        if (::uBrowseForFolder(m_hWnd, "Locate MT-32 or CM-32L ROM sets...", DirectoryPath))
        {
            _MT32ROMDirectoryPath = DirectoryPath;

            ::uSetWindowText(w, !_MT32ROMDirectoryPath.isEmpty() ? _MT32ROMDirectoryPath : DefaultPathMessage);

            OnChanged();
        }
    }
}

/// <summary>
/// Returns true if the dialog state has changed.
/// </summary>
bool PreferencesPaths::HasChanged()
{
    pfc::string8 DirectoryPath;

    CfgVSTiPluginDirectoryPath.get(DirectoryPath);

    if (_VSTiPluginDirectoryPath != DirectoryPath)
        return true;

    if (_SoundFontFilePath != CfgSoundFontFilePath)
        return true;

    if (_MT32ROMDirectoryPath != CfgMuntDirectoryPath)
        return true;

    return false;
}

/// <summary>
/// Notifies the parent that the dialog state has changed.
/// </summary>
void PreferencesPaths::OnChanged()
{
    _Callback->on_state_changed();
}
#pragma endregion

static preferences_page_factory_t<PreferencesPathsPage> PreferencesPageFactory;
