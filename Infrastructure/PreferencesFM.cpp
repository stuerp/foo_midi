
/** $VER: PreferencesFM.cpp (2025.07.13) P. Stuer **/

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

#include "PreferencesFM.h"
#include "Configuration.h"

#include "ADLPlayer.h"
#include "OPNPlayer.h"
#include "NukedOPL3Player.h"

#include "Encoding.h"

#pragma hdrstop

#pragma warning(disable: 4820) // x bytes padding added after data member

const std::vector<emulator_t> _ADLEmulators =
{
    { ADLMIDI_EMU_NUKED,          L"Nuked OPL3 v1.8" },
    { ADLMIDI_EMU_NUKED_174,      L"Nuked OPL3 v1.7.4" },
    { ADLMIDI_EMU_DOSBOX,         L"DOSBox" },
    { ADLMIDI_EMU_OPAL,           L"Opal" },
    { ADLMIDI_EMU_JAVA,           L"Java" },
    { ADLMIDI_EMU_ESFMu,          L"ESFMu" },
    { ADLMIDI_EMU_MAME_OPL2,      L"MAME OPL2" },
    { ADLMIDI_EMU_YMFM_OPL2,      L"YMFM OPL2" },
    { ADLMIDI_EMU_YMFM_OPL3,      L"YMFM OPL3" },
    { ADLMIDI_EMU_NUKED_OPL2_LLE, L"Nuked OPL2 LLE" },
    { ADLMIDI_EMU_NUKED_OPL3_LLE, L"Nuked OPL3 LLE" },
};

const std::vector<emulator_t> _OPNEmulators =
{
    { OPNMIDI_EMU_MAME,         L"MAME YM2612" },
    { OPNMIDI_EMU_MAME_2608,    L"MAME YM2608" },
    { OPNMIDI_EMU_NUKED_YM3438, L"Nuked OPN2 (YM3438 mode)" },
    { OPNMIDI_EMU_NUKED_YM2612, L"Nuked OPN2 (YM2612 mode)" },
    { OPNMIDI_EMU_GENS,         L"GENS/GS II OPN2" },
    { OPNMIDI_EMU_NP2,          L"Neko Project II Kai OPNA" },
    { OPNMIDI_EMU_YMFM_OPN2,    L"YMFM OPN2" },
    { OPNMIDI_EMU_YMFM_OPNA,    L"YMFM OPNA" },
};

const std::vector<std::string> _MT32EmuSets =
{
    "Roland",
    "Sierra / King's Quest 6",
};

/// <summary>
/// Implements a preferences page.
/// </summary>
class FMDialog : public CDialogImpl<FMDialog>, public preferences_page_instance
{
public:
    FMDialog(preferences_page_callback::ptr callback) noexcept : _Callback(callback)
    {
    }

    FMDialog(const FMDialog &) = delete;
    FMDialog(const FMDialog &&) = delete;
    FMDialog & operator=(const FMDialog &) = delete;
    FMDialog & operator=(FMDialog &&) = delete;

    virtual ~FMDialog() { };

    #pragma region preferences_page_instance

    t_uint32 get_state() final;
    void apply() final;
    void reset() final;

    #pragma endregion

    // WTL message map
    BEGIN_MSG_MAP_EX(FMDialog)
        MSG_WM_INITDIALOG(OnInitDialog)

        MSG_WM_CTLCOLORDLG(OnCtlColorDlg)

        // ADL Player
        COMMAND_HANDLER_EX(IDC_ADL_BANK, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_ADL_CORE, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_ADL_CHIPS, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_ADL_CHIPS, CBN_EDITCHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_ADL_SOFT_PANNING, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_ADL_BANK_FILE_PATH, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_ADL_BANK_FILE_PATH_SELECT, BN_CLICKED, OnButtonClicked)

        // OPN Player
        COMMAND_HANDLER_EX(IDC_OPN_BANK, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_OPN_CORE, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_OPN_CHIPS, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_OPN_CHIPS, CBN_EDITCHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_OPN_SOFT_PANNING, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_OPN_BANK_FILE_PATH, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_OPN_BANK_FILE_PATH_SELECT, BN_CLICKED, OnButtonClicked)

        // Nuked OPL3
        COMMAND_HANDLER_EX(IDC_NUKE_PRESET, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_NUKE_PANNING, BN_CLICKED, OnButtonClicked)

        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

    enum
    {
        IDD = IDD_PREFERENCES_FM
    };

private:
    BOOL OnInitDialog(CWindow, LPARAM) noexcept;
    HBRUSH OnCtlColorDlg(CDCHandle dc, CWindow wnd) const noexcept;

    void OnEditChange(UINT, int, CWindow) noexcept;
    void OnSelectionChange(UINT, int, CWindow) noexcept;
    void OnButtonClicked(UINT, int, CWindow) noexcept;

    bool HasChanged() noexcept;
    void OnChanged() const noexcept;

private:
    std::vector<bank_t> _ADLBanks;

    std::vector<bank_t> _OPNBanks;

    const preferences_page_callback::ptr _Callback;

    fb2k::CCoreDarkModeHooks _DarkModeHooks;
};

#pragma region preferences_page_instance

/// <summary>
/// Gets the state of the Preference dialog.
/// </summary>
t_uint32 FMDialog::get_state()
{
    t_uint32 State = preferences_state::resettable | preferences_state::dark_mode_supported;

    if (HasChanged())
        State |= preferences_state::changed;

    return State;
}

/// <summary>
/// Applies the changes to the preferences.
/// </summary>
void FMDialog::apply()
{
    // ADL
    {
        // ADL Bank
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_ADL_BANK, CB_GETCURSEL);

            if (SelectedIndex < 0 || SelectedIndex >= (int) _ADLBanks.size())
                SelectedIndex = 0;

            CfgADLBank = _ADLBanks[(size_t) SelectedIndex].Id;
        }

        // ADL Emulator Core
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_ADL_CORE, CB_GETCURSEL);

            if (SelectedIndex < 0 || SelectedIndex >= (int) _ADLEmulators.size())
                SelectedIndex = 0;

            CfgADLEmulator = _ADLEmulators[(size_t) SelectedIndex].Id;
        }

        // ADL Chip Count
        {
            int Value = std::clamp((int) GetDlgItemInt(IDC_ADL_CHIPS, NULL, FALSE), 1, 100);

            SetDlgItemInt(IDC_ADL_CHIPS, (UINT) Value, FALSE);

            CfgADLChipCount = Value;
        }

        // ADL Soft Panning
        CfgADLSoftPanning = (bool) SendDlgItemMessage(IDC_ADL_SOFT_PANNING, BM_GETCHECK);

        // ADL Bank File
        {
            wchar_t Text[MAX_PATH] = { };

            GetDlgItemText(IDC_ADL_BANK_FILE_PATH, Text, (int) _countof(Text));

            CfgADLBankFilePath = ::WideToUTF8(Text).c_str();
        }
    }

    // OPN
    {
        // OPN Bank
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_OPN_BANK, CB_GETCURSEL);

            if (SelectedIndex < 0 || SelectedIndex >= (int) _OPNBanks.size())
                SelectedIndex = 0;

            CfgOPNBank = _OPNBanks[(size_t) SelectedIndex].Id;
        }

        // OPN Emulator Core
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_OPN_CORE, CB_GETCURSEL);

            if (SelectedIndex < 0 || SelectedIndex >= (int) _OPNEmulators.size())
                SelectedIndex = 0;

            CfgOPNEmulator = _OPNEmulators[(size_t) SelectedIndex].Id;
        }

        // OPN Chip Count
        {
            int Value = std::clamp((int) GetDlgItemInt(IDC_OPN_CHIPS, NULL, FALSE), 1, 100);

            SetDlgItemInt(IDC_OPN_CHIPS, (UINT) Value, FALSE);

            CfgOPNChipCount = Value;
        }

        // OPN Soft Panning
        {
            CfgOPNSoftPanning = (bool) SendDlgItemMessage(IDC_OPN_SOFT_PANNING, BM_GETCHECK);
        }

        // OPN Bank File
        {
            wchar_t Text[MAX_PATH] = { };

            GetDlgItemText(IDC_OPN_BANK_FILE_PATH, Text, (int) _countof(Text));

            CfgOPNBankFilePath = ::WideToUTF8(Text).c_str();
        }
    }

    // Nuked OPL3
    {
        size_t SelectedIndex = (size_t) SendDlgItemMessage(IDC_NUKE_PRESET, CB_GETCURSEL);

        uint32_t Synth;
        uint32_t Bank;

        NukedOPL3Player::GetPreset(SelectedIndex, Synth, Bank);

        CfgNukeSynthesizer = (t_int32) Synth;
        CfgNukeBank        = (t_int32) Bank;
        CfgNukePanning     = (t_int32) SendDlgItemMessage(IDC_NUKE_PANNING, BM_GETCHECK);
    }

    OnChanged();
}

/// <summary>
/// Resets this page's content to the default values. Does not apply any changes - lets user preview the changes before hitting "apply".
/// </summary>
void FMDialog::reset()
{
    // ADL
    {
        // ADL Bank
        {
            int SelectedIndex = -1;
            int i = 0;

            for (const auto & Iter : _ADLBanks)
            {
                if (Iter.Id == DefaultADLBank)
                {
                    SelectedIndex = i;
                    break;
                }
            }

            SendDlgItemMessage(IDC_ADL_BANK, CB_SETCURSEL, (WPARAM) SelectedIndex);
        }

        // ADL Emulator Core
        {
            int SelectedIndex = DefaultADLEmulator;

            SendDlgItemMessage(IDC_ADL_CORE, CB_SETCURSEL, (WPARAM) SelectedIndex);
        }

        // ADL Chip Count
        SetDlgItemInt(IDC_ADL_CHIPS, DefaultADLChipCount, 0);

        // ADL Soft Panning
        SendDlgItemMessage(IDC_ADL_SOFT_PANNING, BM_SETCHECK, DefaultADLSoftPanning);

        // ADL Bank File
        SetDlgItemText(IDC_ADL_BANK_FILE_PATH, L"");
    }

    // OPN
    {
        // OPN Bank
        {
            int SelectedIndex = -1;
            int i = 0;

            for (const auto & Iter : _OPNBanks)
            {
                if (Iter.Id == DefaultOPNBank)
                {
                    SelectedIndex = i;
                    break;
                }
            }

            SendDlgItemMessage(IDC_OPN_BANK, CB_SETCURSEL, (WPARAM) SelectedIndex);
        }

        // OPN Emulator
        {
            int SelectedIndex = DefaultOPNEmulator;

            SendDlgItemMessage(IDC_OPN_CORE, CB_SETCURSEL, (WPARAM) SelectedIndex);
        }

        // OPN Chip Count
        SetDlgItemInt(IDC_OPN_CHIPS, DefaultOPNChipCount, 0);

        // OPN Soft Panning
        SendDlgItemMessage(IDC_OPN_SOFT_PANNING, BM_SETCHECK, DefaultOPNSoftPanning);

        // OPN Bank File
        SetDlgItemText(IDC_OPN_BANK_FILE_PATH, L"");
    }

    // Nuked OPL3
    {
        SendDlgItemMessage(IDC_NUKE_PRESET, CB_SETCURSEL, (WPARAM) NukedOPL3Player::GetPresetIndex(DefaultNukeSynth, DefaultNukeBank));
        SendDlgItemMessage(IDC_NUKE_PANNING, BM_SETCHECK, DefaultNukePanning);
    }

    OnChanged();
}

#pragma endregion

#pragma region CDialogImpl

/// <summary>
/// Initializes the dialog.
/// </summary>
BOOL FMDialog::OnInitDialog(CWindow window, LPARAM) noexcept
{
    // ADL
    {
        // ADL Bank
        {
            _ADLBanks.clear();

            {
                const char * const * BankNames = ::adl_getBankNames();
                const size_t BankCount = (size_t) ::adl_getBanksCount();

                if ((BankNames != nullptr) && (BankCount > 0))
                {
                    for (size_t i = 0; i < BankCount; ++i)
                        _ADLBanks.push_back(bank_t((int) i, ::UTF8ToWide(BankNames[i])));

                    std::sort(_ADLBanks.begin(), _ADLBanks.end(), [](bank_t a, bank_t b) { return a < b; });
                }
            }

            auto w = (CComboBox) GetDlgItem(IDC_ADL_BANK);

            for (size_t i = 0; i < _ADLBanks.size(); ++i)
            {
                w.AddString(_ADLBanks[i].Name.c_str());

                if (_ADLBanks[i].Id == CfgADLBank)
                    w.SetCurSel((int) i);
            }
        }

        // ADL Core
        {
            auto w = (CComboBox) GetDlgItem(IDC_ADL_CORE);

            for (size_t i = 0; i < _ADLEmulators.size(); ++i)
            {
                w.AddString(_ADLEmulators[i].Name.c_str());

                if (_ADLEmulators[i].Id == CfgADLEmulator)
                    w.SetCurSel((int) i);
            }
        }

        // ADL Chip Count
        {
            static const wchar_t * ChipCounts[] = { L"1", L"2", L"5", L"10", L"25", L"50", L"100" };

            auto w = (CComboBox) GetDlgItem(IDC_ADL_CHIPS);

            for (const auto & Iter : ChipCounts)
                w.AddString(Iter);

            SetDlgItemInt(IDC_ADL_CHIPS, (UINT) CfgADLChipCount, 0);
        }

        // ADL Soft Panning
        SendDlgItemMessage(IDC_ADL_SOFT_PANNING, BM_SETCHECK, (WPARAM) CfgADLSoftPanning);

        // ADL Bank File
        SetDlgItemText(IDC_ADL_BANK_FILE_PATH, ::UTF8ToWide(CfgADLBankFilePath.get().c_str()).c_str());
    }

    // OPN
    {
        // OPN Bank
        {
            _OPNBanks.clear();

            {
                _OPNBanks.push_back(bank_t(0, L"XG"));
                _OPNBanks.push_back(bank_t(1, L"GS (DMXOPN2)"));
                _OPNBanks.push_back(bank_t(2, L"GEMS FMLib GM"));
                _OPNBanks.push_back(bank_t(3, L"TomSoft's SegaMusic"));
                _OPNBanks.push_back(bank_t(4, L"FMMIDI"));

                std::sort(_OPNBanks.begin(), _OPNBanks.end(), [](bank_t a, bank_t b) { return a < b; });
            }

            auto w = (CComboBox) GetDlgItem(IDC_OPN_BANK);

            for (size_t i = 0; i < _OPNBanks.size(); ++i)
            {
                w.AddString(_OPNBanks[i].Name.c_str());

                if (_OPNBanks[i].Id == CfgOPNBank)
                    w.SetCurSel((int) i);
            }
        }

        // OPN Core
        {
            auto w = (CComboBox) GetDlgItem(IDC_OPN_CORE);

            for (size_t i = 0; i < _OPNEmulators.size(); ++i)
            {
                w.AddString(_OPNEmulators[i].Name.c_str());

                if (_OPNEmulators[i].Id == CfgOPNEmulator)
                    w.SetCurSel((int) i);
            }
        }

        // OPN Chip Count
        {
            static const wchar_t * ChipCounts[] = { L"1", L"2", L"5", L"10", L"25", L"50", L"100" };

            auto w = (CComboBox) GetDlgItem(IDC_OPN_CHIPS);

            for (const auto & Iter : ChipCounts)
                w.AddString(Iter);

            SetDlgItemInt(IDC_OPN_CHIPS, (UINT) CfgOPNChipCount, 0);
        }

        // OPN Soft Panning
        {
            SendDlgItemMessage(IDC_OPN_SOFT_PANNING, BM_SETCHECK, (WPARAM) CfgOPNSoftPanning);
        }

        // OPN Bank File
        SetDlgItemText(IDC_OPN_BANK_FILE_PATH, ::UTF8ToWide(CfgOPNBankFilePath.get().c_str()).c_str());
    }

    // Nuked OPL3
    {
        auto w = GetDlgItem(IDC_NUKE_PRESET);

        size_t PresetNumber = 0;

        NukedOPL3Player::EnumeratePresets([w, PresetNumber] (const std::string & name, uint32_t synth, uint32_t bank) mutable noexcept
        {
            ::uSendMessageText(w, CB_ADDSTRING, 0, name.c_str());

            if ((synth == (uint32_t) CfgNukeSynthesizer) && (bank == (uint32_t) CfgNukeBank))
                ::SendMessage(w, CB_SETCURSEL, PresetNumber, 0);

            PresetNumber++;
        });

        SendDlgItemMessage(IDC_NUKE_PANNING, BM_SETCHECK, (WPARAM) CfgNukePanning);
    }

/*
    for (const auto Id : { IDC_ADL_BANK_TEXT, IDC_ADL_BANK, IDC_ADL_CORE_TEXT, IDC_ADL_CORE, IDC_ADL_CHIPS_TEXT, IDC_ADL_CHIPS, IDC_ADL_SOFT_PANNING })
    {
        GetDlgItem(Id).EnableWindow(IsADL || IsOPN);
    }

    for (const auto Id : { IDC_ADL_BANK_FILE_PATH_TEXT, IDC_ADL_BANK_FILE_PATH, IDC_ADL_BANK_FILE_PATH_SELECT })
    {
        GetDlgItem(Id).EnableWindow(IsADL);
    }
*/
    _DarkModeHooks.AddDialogWithControls(*this);

    return FALSE;
}

/// <summary>
/// Sets the background color brush.
/// </summary>
HBRUSH FMDialog::OnCtlColorDlg(CDCHandle dc, CWindow wnd) const noexcept
{
#ifdef _DEBUG
    return ::CreateSolidBrush(RGB(250, 250, 250));
#else
    return FALSE;
#endif
}

/// <summary>
/// Handles the notification when a control loses focus.
/// </summary>
void FMDialog::OnEditChange(UINT code, int id, CWindow) noexcept
{
    if (code != EN_CHANGE)
        return;

    OnChanged();
}

/// <summary>
/// Handles a selection change in a combo box.
/// </summary>
void FMDialog::OnSelectionChange(UINT, int, CWindow) noexcept
{
    OnChanged();
}

/// <summary>
/// Handles a click on a button.
/// </summary>
void FMDialog::OnButtonClicked(UINT, int id, CWindow) noexcept
{
    switch (id)
    {
        case IDC_ADL_BANK_FILE_PATH_SELECT:
        {
            wchar_t Text[MAX_PATH] = { };

            GetDlgItemText(IDC_ADL_BANK_FILE_PATH, Text, (int) _countof(Text));

            pfc::string FilePath = ::WideToUTF8(Text).c_str();

            pfc::string DirectoryPath = FilePath;

            DirectoryPath.truncate_filename();

            if (::uGetOpenFileName(m_hWnd,
                    "WOPL files|*.wopl|"
                    "All files|*.*",
                0, "wopl", "Choose a bank...", DirectoryPath, FilePath, FALSE))
            {
                SetDlgItemText(IDC_ADL_BANK_FILE_PATH, ::UTF8ToWide(FilePath.c_str()).c_str());

                OnChanged();
            }
            break;
        }

        case IDC_OPN_BANK_FILE_PATH_SELECT:
        {
            wchar_t Text[MAX_PATH] = { };

            GetDlgItemText(IDC_OPN_BANK_FILE_PATH, Text, (int) _countof(Text));

            pfc::string FilePath = ::WideToUTF8(Text).c_str();

            pfc::string DirectoryPath = FilePath;

            DirectoryPath.truncate_filename();

            if (::uGetOpenFileName(m_hWnd,
                    "WOPN files|*.wopn|"
                    "All files|*.*",
                0, "wopn", "Choose a bank...", DirectoryPath, FilePath, FALSE))
            {
                SetDlgItemText(IDC_OPN_BANK_FILE_PATH, ::UTF8ToWide(FilePath.c_str()).c_str());

                OnChanged();
            }
            break;
        }

        case IDC_ADL_SOFT_PANNING:

        case IDC_OPN_SOFT_PANNING:
        {
            OnChanged();
            break;
        }
    }
}

/// <summary>
/// Returns whether our dialog content is different from the current configuration (whether the Apply button should be enabled or not)
/// </summary>
bool FMDialog::HasChanged() noexcept
{
    // ADL
    {
        // ADL Bank
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_ADL_BANK, CB_GETCURSEL);

            if ((SelectedIndex < 0) || (SelectedIndex >= (int) _ADLBanks.size()))
                SelectedIndex = 0;

            if (_ADLBanks[(t_size) SelectedIndex].Id != CfgADLBank)
                return true;
        }

        // ADL Emulator Core
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_ADL_CORE, CB_GETCURSEL);

            if ((SelectedIndex < 0) || (SelectedIndex >= (int) _ADLEmulators.size()))
                SelectedIndex = 0;

            if (SelectedIndex != CfgADLEmulator)
                return true;
        }

        // ADL Chip Count
        if (GetDlgItemInt(IDC_ADL_CHIPS, NULL, FALSE) != (UINT) CfgADLChipCount)
            return true;

        // ADL Soft Panning
        if (SendDlgItemMessage(IDC_ADL_SOFT_PANNING, BM_GETCHECK) != CfgADLSoftPanning)
            return true;

        // ADL Bank File
        {
            wchar_t Text[MAX_PATH] = { };

            GetDlgItemText(IDC_ADL_BANK_FILE_PATH, Text, (int) _countof(Text));

            if (CfgADLBankFilePath.get_value() != ::WideToUTF8(Text).c_str())
                return true;
        }
    }

    // OPN
    {
        // OPN Bank
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_OPN_BANK, CB_GETCURSEL);

            if ((SelectedIndex < 0) || (SelectedIndex >= (int) _OPNBanks.size()))
                SelectedIndex = 0;

            if (_OPNBanks[(t_size) SelectedIndex].Id != CfgOPNBank)
                return true;
        }

        // OPN Emulator Core
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_OPN_CORE, CB_GETCURSEL);

            if ((SelectedIndex < 0) || (SelectedIndex >= (int) _OPNEmulators.size()))
                SelectedIndex = 0;

            if (_OPNEmulators[(size_t) SelectedIndex].Id != CfgOPNEmulator)
                return true;
        }

        // OPN Chip Count
        if (GetDlgItemInt(IDC_OPN_CHIPS, NULL, FALSE) != (UINT) CfgOPNChipCount)
            return true;

        // OPN Soft Panning
        if (SendDlgItemMessage(IDC_OPN_SOFT_PANNING, BM_GETCHECK) != CfgOPNSoftPanning)
            return true;

        // OPN Bank File
        {
            wchar_t Text[MAX_PATH] = { };

            GetDlgItemText(IDC_OPN_BANK_FILE_PATH, Text, (int) _countof(Text));

            if (CfgOPNBankFilePath.get_value() != ::WideToUTF8(Text).c_str())
                return true;
        }
    }

    // Nuked OPL3
    {
        if (SendDlgItemMessage(IDC_NUKE_PANNING, BM_GETCHECK) != CfgNukePanning)
            return true;

        size_t PresetNumber = (size_t) SendDlgItemMessage(IDC_NUKE_PRESET, CB_GETCURSEL);

        uint32_t Synth;
        uint32_t Bank;

        NukedOPL3Player::GetPreset(PresetNumber, Synth, Bank);

        if (!(Synth == (uint32_t) CfgNukeSynthesizer && Bank == (uint32_t) CfgNukeBank))
            return true;
    }

    return false;
}

/// <summary>
/// Tells the host that our state has changed to enable/disable the Apply button appropriately.
/// </summary>
void FMDialog::OnChanged() const noexcept
{
    _Callback->on_state_changed();
}

#pragma endregion

class Page : public preferences_page_impl<FMDialog>
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
        return IDD_PREFERENCES_FM_NAME;
    }

    GUID get_guid() noexcept
    {
        return GUID_PREFS_FM;
    }

    GUID get_parent_guid() noexcept
    {
        return PreferencesPageGUID;
    }
};

static preferences_page_factory_t<Page> _Factory;
