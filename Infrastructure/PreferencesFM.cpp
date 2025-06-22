
/** $VER: PreferencesFM.cpp (2025.06.22) P. Stuer **/

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

#include "resource.h"

#include "Configuration.h"

#include "ADLPlayer.h"

#include "Encoding.h"

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

    #pragma region preferences_page_instance

    t_uint32 get_state() final;
    void apply() final;
    void reset() final;

    #pragma endregion

    // WTL message map
    BEGIN_MSG_MAP_EX(DialogPage)
        MSG_WM_INITDIALOG(OnInitDialog)

        COMMAND_HANDLER_EX(IDC_ADL_BANK, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_ADL_CORE, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_ADL_CHIPS, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_ADL_CHIPS, CBN_EDITCHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_ADL_SOFT_PANNING, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_ADL_BANK_FILE_PATH, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_ADL_BANK_FILE_PATH_SELECT, BN_CLICKED, OnButtonClicked)

        COMMAND_HANDLER_EX(IDC_OPN_BANK, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_OPN_CORE, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_OPN_CHIPS, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_OPN_CHIPS, CBN_EDITCHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_OPN_SOFT_PANNING, BN_CLICKED, OnButtonClicked)

        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

    enum
    {
        IDD = IDD_PREFERENCES_FM
    };

private:
    BOOL OnInitDialog(CWindow, LPARAM) noexcept;

    void OnShowWindow(CWindow wndOther) noexcept;

    void OnEditChange(UINT, int, CWindow) noexcept;
    void OnSelectionChange(UINT, int, CWindow) noexcept;
    void OnButtonClicked(UINT, int, CWindow) noexcept;

    void UpdateDialog() const noexcept;

    bool HasChanged() noexcept;
    void OnChanged() const noexcept;

private:
    struct bank_t
    {
        int Number;
        std::wstring Name;

        bank_t() : Number(-1), Name() { }
        bank_t(const bank_t & other) : Number(other.Number), Name(other.Name) { }
        bank_t(int number, const std::wstring & name) : Number(number), Name(name) { }

        bank_t & operator =(const bank_t & other)
        {
            Number = other.Number;
            Name = other.Name;

            return *this;
        }

        bool operator ==(const bank_t & b) const { return Number == b.Number; }
        bool operator !=(const bank_t & b) const { return !operator ==(b); }
        bool operator < (const bank_t & b) const { return Name < b.Name; }
        bool operator > (const bank_t & b) const { return Name > b.Name; }
    };

    std::vector<bank_t> _ADLBanks;

    const std::vector<std::wstring> _ADLCores =
    {
        L"Nuked OPL3 v1.8",
        L"Nuked OPL3 v1.7.4",
        L"DOSBox",
        L"Opal",
        L"Java",
        L"ESFMu",
        L"MAME OPL2",
        L"YMFM OPL2",
        L"YMFM OPL3",
        L"Nuked OPL2 LLE",
        L"Nuked OPL3 LLE"
    };

    std::vector<bank_t> _OPNBanks;

    const std::vector<std::wstring> _OPNCores =
    {
        L"MAME YM2612",
        L"Nuked OPN2",
        L"GENS",
        L"Genesis Plus GX",
        L"Neko Project II OPNA",
        L"MAME YM2608",
        L"PMDWin OPNA",
    };

    const preferences_page_callback::ptr _Callback;

    fb2k::CCoreDarkModeHooks _DarkModeHooks;
};

#pragma region preferences_page_instance

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
    // Bank
    {
        int SelectedIndex = (int) SendDlgItemMessage(IDC_ADL_BANK, CB_GETCURSEL);

        if (SelectedIndex < 0 || SelectedIndex >= (int) _ADLBanks.size())
            SelectedIndex = 0;

        CfgADLBank = _ADLBanks[(size_t) SelectedIndex].Number;
    }

    // Emulator Core
    {
        int SelectedIndex = (int) SendDlgItemMessage(IDC_ADL_CORE, CB_GETCURSEL);

        if (SelectedIndex < 0 || SelectedIndex >= (int) _ADLCores.size())
            SelectedIndex = 0;

        CfgADLCore = SelectedIndex;
    }

    // Chip Count
    {
        int Value = std::clamp((int) GetDlgItemInt(IDC_ADL_CHIPS, NULL, FALSE), 1, 100);

        SetDlgItemInt(IDC_ADL_CHIPS, (UINT) Value, FALSE);

        CfgADLChipCount = Value;
    }

    // Soft Panning
    CfgADLSoftPanning = (t_int32) SendDlgItemMessage(IDC_ADL_SOFT_PANNING, BM_GETCHECK);

    // Bank File
    {
        wchar_t Text[MAX_PATH] = { };

        GetDlgItemText(IDC_ADL_BANK_FILE_PATH, Text, (int) _countof(Text));

        CfgADLBankFilePath = ::WideToUTF8(Text).c_str();
    }

    // OPN Bank
    {
        int SelectedIndex = (int) SendDlgItemMessage(IDC_OPN_BANK, CB_GETCURSEL);

        if (SelectedIndex < 0 || SelectedIndex >= (int) _OPNBanks.size())
            SelectedIndex = 0;

        CfgOPNBank = _OPNBanks[(size_t) SelectedIndex].Number;
    }

    // OPN Emulator Core
    {
        int SelectedIndex = (int) SendDlgItemMessage(IDC_OPN_CORE, CB_GETCURSEL);

        if (SelectedIndex < 0 || SelectedIndex >= (int) _OPNCores.size())
            SelectedIndex = 0;

        CfgOPNCore = SelectedIndex;
    }

    // OPN Chip Count
    {
        int Value = std::clamp((int) GetDlgItemInt(IDC_OPN_CHIPS, NULL, FALSE), 1, 100);

        SetDlgItemInt(IDC_OPN_CHIPS, (UINT) Value, FALSE);

        CfgOPNChipCount = Value;
    }

    // OPN Soft Panning
    CfgOPNSoftPanning = (t_int32) SendDlgItemMessage(IDC_OPN_SOFT_PANNING, BM_GETCHECK);

    OnChanged();
}

/// <summary>
/// Resets this page's content to the default values. Does not apply any changes - lets user preview the changes before hitting "apply".
/// </summary>
void DialogPage::reset()
{
    // ADL Bank
    {
        int SelectedIndex = -1;
        int i = 0;

        for (const auto & Iter : _ADLBanks)
        {
            if (Iter.Number == DefaultADLBank)
            {
                SelectedIndex = i;
                break;
            }
        }

        SendDlgItemMessage(IDC_ADL_BANK, CB_SETCURSEL, (WPARAM) SelectedIndex);
    }

    // ADL Emulator Core
    {
        int SelectedIndex = DefaultADLCore;

        SendDlgItemMessage(IDC_ADL_CORE, CB_SETCURSEL, (WPARAM) SelectedIndex);
    }

    // ADL Chip Count
    SetDlgItemInt(IDC_ADL_CHIPS, DefaultADLChipCount, 0);

    // ADL Soft Panning
    SendDlgItemMessage(IDC_ADL_SOFT_PANNING, BM_SETCHECK, DefaultADLSoftPanning);

    // ADL Bank File
    SetDlgItemText(IDC_ADL_BANK_FILE_PATH, L"");

    // OPN Bank
    {
        int SelectedIndex = -1;
        int i = 0;

        for (const auto & Iter : _OPNBanks)
        {
            if (Iter.Number == DefaultOPNBank)
            {
                SelectedIndex = i;
                break;
            }
        }

        SendDlgItemMessage(IDC_OPN_BANK, CB_SETCURSEL, (WPARAM) SelectedIndex);
    }

    // OPN Emulator Core
    {
        int SelectedIndex = DefaultOPNCore;

        SendDlgItemMessage(IDC_OPN_CORE, CB_SETCURSEL, (WPARAM) SelectedIndex);
    }

    // OPN Chip Count
    SetDlgItemInt(IDC_OPN_CHIPS, DefaultOPNChipCount, 0);

    // OPN Soft Panning
    SendDlgItemMessage(IDC_OPN_SOFT_PANNING, BM_SETCHECK, DefaultOPNSoftPanning);

    OnChanged();
}

#pragma endregion

#pragma region CDialogImpl

/// <summary>
/// Initializes the dialog.
/// </summary>
BOOL DialogPage::OnInitDialog(CWindow window, LPARAM) noexcept
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

            if (_ADLBanks[i].Number == CfgADLBank)
                w.SetCurSel((int) i);
        }
    }

    // ADL Core
    {
        auto w = (CComboBox) GetDlgItem(IDC_ADL_CORE);

        for (size_t i = 0; i < _ADLCores.size(); ++i)
        {
            w.AddString(_ADLCores[i].c_str());

            if ((int) i == CfgADLCore)
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

            if (_OPNBanks[i].Number == CfgOPNBank)
                w.SetCurSel((int) i);
        }
    }

    // OPN Core
    {
        auto w = (CComboBox) GetDlgItem(IDC_OPN_CORE);

        for (size_t i = 0; i < _OPNCores.size(); ++i)
        {
            w.AddString(_OPNCores[i].c_str());

            if ((int) i == CfgOPNCore)
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
    SendDlgItemMessage(IDC_OPN_SOFT_PANNING, BM_SETCHECK, (WPARAM) CfgOPNSoftPanning);
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
/// Handles the notification when a control loses focus.
/// </summary>
void DialogPage::OnEditChange(UINT code, int id, CWindow) noexcept
{
    if (code != EN_CHANGE)
        return;

    OnChanged();
}

/// <summary>
/// Handles a selection change in a combo box.
/// </summary>
void DialogPage::OnSelectionChange(UINT, int, CWindow) noexcept
{
    OnChanged();
}

/// <summary>
/// Handles a click on a button.
/// </summary>
void DialogPage::OnButtonClicked(UINT, int id, CWindow) noexcept
{
    switch (id)
    {
        case IDC_ADL_SOFT_PANNING:
        {
            OnChanged();
            break;
        }

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
bool DialogPage::HasChanged() noexcept
{
    // Bank
    {
        int SelectedIndex = (int) SendDlgItemMessage(IDC_ADL_BANK, CB_GETCURSEL);

        if ((SelectedIndex < 0) || (SelectedIndex >= (int) _ADLBanks.size()))
            SelectedIndex = 0;

        if (_ADLBanks[(t_size) SelectedIndex].Number != CfgADLBank)
            return true;
    }

    // Emulator Core
    {
        int SelectedIndex = (int) SendDlgItemMessage(IDC_ADL_CORE, CB_GETCURSEL);

        if ((SelectedIndex < 0) || (SelectedIndex >= (int) _ADLCores.size()))
            SelectedIndex = 0;

        if (SelectedIndex != CfgADLCore)
            return true;
    }

    // Chip Count
    if (GetDlgItemInt(IDC_ADL_CHIPS, NULL, FALSE) != (UINT) CfgADLChipCount)
        return true;

    // Soft Panning
    if (SendDlgItemMessage(IDC_ADL_SOFT_PANNING, BM_GETCHECK) != CfgADLSoftPanning)
        return true;

    // Bank File
    {
        wchar_t Text[MAX_PATH] = { };

        GetDlgItemText(IDC_ADL_BANK_FILE_PATH, Text, (int) _countof(Text));

        if (CfgADLBankFilePath.get_value() != ::WideToUTF8(Text).c_str())
            return true;
    }

    // OPN Bank
    {
        int SelectedIndex = (int) SendDlgItemMessage(IDC_OPN_BANK, CB_GETCURSEL);

        if ((SelectedIndex < 0) || (SelectedIndex >= (int) _OPNBanks.size()))
            SelectedIndex = 0;

        if (_OPNBanks[(t_size) SelectedIndex].Number != CfgOPNBank)
            return true;
    }

    // OPN Emulator Core
    {
        int SelectedIndex = (int) SendDlgItemMessage(IDC_OPN_CORE, CB_GETCURSEL);

        if ((SelectedIndex < 0) || (SelectedIndex >= (int) _OPNCores.size()))
            SelectedIndex = 0;

        if (SelectedIndex != CfgOPNCore)
            return true;
    }

    // OPN Chip Count
    if (GetDlgItemInt(IDC_OPN_CHIPS, NULL, FALSE) != (UINT) CfgOPNChipCount)
        return true;

    // OPN Soft Panning
    if (SendDlgItemMessage(IDC_OPN_SOFT_PANNING, BM_GETCHECK) != CfgOPNSoftPanning)
        return true;

    return false;
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
}

#pragma endregion

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
        return IDD_PREFERENCES_FM_NAME;
    }

    GUID get_guid() noexcept
    {
        return { 0xbe9aebaa, 0x5b66, 0x4374, { 0xa3, 0xad, 0x94, 0x6e, 0xb6, 0xc7, 0xe2, 0x61 } };;
    }

    GUID get_parent_guid() noexcept
    {
        return PreferencesPageGUID;
    }
};

static preferences_page_factory_t<Page> PreferencesPageFactory;
