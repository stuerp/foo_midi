
/** $VER: PreferencesPaths.cpp (2023.01.06) **/

#pragma warning(disable: 5045 26481 26485)

#include "PreferencesPaths.h"

#include "NukePlayer.h"

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

    if (_IsBusy)
        State |= preferences_state::busy;

    return State;
}

void PreferencesPaths::apply()
{
    CfgVSTiPluginDirectoryPath.set(_VSTiSearchPath);
    CfgSoundFontFilePath = _SoundFontPath;
    CfgMT32ROMDirectoryPath = _MT32ROMDirectoryPath;

    OnChanged();
}

void PreferencesPaths::reset()
{
    ::uSetDlgItemText(m_hWnd, IDC_VST_PATH, DefaultPathMessage);
    ::uSetDlgItemText(m_hWnd, IDC_SOUNDFONT_FILE_PATH, DefaultPathMessage);
    ::uSetDlgItemText(m_hWnd, IDC_MUNT_FILE_PATH, DefaultPathMessage);

    _VSTiSearchPath.reset();
    _SoundFontPath.reset();
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
        CfgVSTiPluginDirectoryPath.get(_VSTiSearchPath);

        ::uSetDlgItemText(m_hWnd, IDC_VST_PATH, !_VSTiSearchPath.is_empty() ? _VSTiSearchPath : DefaultPathMessage);
    }
#pragma endregion

#pragma region("SoundFont")
    {
        _SoundFontPath = CfgSoundFontFilePath;

        ::uSetDlgItemText(m_hWnd, IDC_SOUNDFONT_FILE_PATH, !_SoundFontPath.is_empty() ? _SoundFontPath : DefaultPathMessage);
    }
#pragma endregion

#pragma region("Munt")
    {
        _MT32ROMDirectoryPath = CfgMT32ROMDirectoryPath;

        ::uSetDlgItemText(m_hWnd, IDC_MUNT_FILE_PATH, !_MT32ROMDirectoryPath.is_empty() ? _MT32ROMDirectoryPath : DefaultPathMessage);
    }
#pragma endregion

    _DarkModeHooks.AddDialogWithControls(*this);

    return FALSE;
}

void PreferencesPaths::OnEditChange(UINT, int, CWindow)
{
    OnChanged();
}

void PreferencesPaths::OnSelectionChange(UINT, int, CWindow)
{
    OnChanged();
}

void PreferencesPaths::OnButtonClick(UINT, int, CWindow)
{
    OnChanged();
}

void PreferencesPaths::OnButtonConfig(UINT, int, CWindow)
{
}

void PreferencesPaths::OnPlugInChange(UINT, int, CWindow)
{
}

void PreferencesPaths::OnSetFocus(UINT, int, CWindow w)
{
    SetFocus();

    if (w == GetDlgItem(IDC_VST_PATH))
    {
        pfc::string8 DirectoryPath = _VSTiSearchPath;

        DirectoryPath.truncate_to_parent_path();

        if (::uBrowseForFolder(m_hWnd, "Locate VSTi plug-ins...", DirectoryPath))
        {
            _VSTiSearchPath = DirectoryPath;

            ::uSetWindowText(w, !_VSTiSearchPath.is_empty() ? _VSTiSearchPath : DefaultPathMessage);
        }
    }
    else
    if (w == GetDlgItem(IDC_SOUNDFONT_FILE_PATH))
    {
        pfc::string8 DirectoryPath = _SoundFontPath;

        DirectoryPath.truncate_to_parent_path();

        pfc::string8 FilePath = _SoundFontPath;

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
            _SoundFontPath = FilePath;

            ::uSetWindowText(w, !_SoundFontPath.isEmpty() ? _SoundFontPath : DefaultPathMessage);

            OnChanged();
        }
    }
    else
    if (w == GetDlgItem(IDC_MUNT_FILE_PATH))
    {
        pfc::string8 DirectoryPath = _MT32ROMDirectoryPath;

        DirectoryPath.truncate_to_parent_path();

        if (::uBrowseForFolder(m_hWnd, "Locate MT-32 or CM-32L ROM sets...", DirectoryPath))
        {
            _MT32ROMDirectoryPath = DirectoryPath;

            ::uSetWindowText(w, !_MT32ROMDirectoryPath.isEmpty() ? _MT32ROMDirectoryPath : DefaultPathMessage);

            OnChanged();
        }
    }
}

void PreferencesPaths::OnTimer(UINT_PTR)
{
}

bool PreferencesPaths::HasChanged()
{
    bool changed = false;

    pfc::string8 VSTiSearchPath; CfgVSTiPluginDirectoryPath.get(VSTiSearchPath);

    if (!changed && (_VSTiSearchPath != VSTiSearchPath))
        changed = true;

    if (!changed && (_SoundFontPath != CfgSoundFontFilePath))
        changed = true;

    if (!changed && (_MT32ROMDirectoryPath != CfgMT32ROMDirectoryPath))
        changed = true;

    return changed;
}

void PreferencesPaths::OnChanged()
{
    _Callback->on_state_changed();
}
#pragma endregion

static preferences_page_factory_t<PreferencesPathsPage> PreferencesPageFactory;
