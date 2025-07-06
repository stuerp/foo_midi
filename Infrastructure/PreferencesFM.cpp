
/** $VER: PreferencesFM.cpp (2025.07.06) P. Stuer **/

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

#include "ADLPlayer.h"
#include "OPNPlayer.h"

#include "Encoding.h"

#pragma hdrstop

#pragma warning(disable: 4820) // x bytes padding added after data member

const char * _MT32EmuSets[] =
{
    "Roland",
    "Sierra / King's Quest 6",
};

const size_t _MT32EmuSetCount = _countof(_MT32EmuSets);

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

        COMMAND_HANDLER_EX(IDC_MT32_CONVERSION_QUALITY, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_MT32_MAX_PARTIALS, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_MT32_ANALOG_OUTPUT_MODE, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_MT32_GM_SET, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_MT32_DAC_INPUT_MODE, CBN_SELCHANGE, OnSelectionChange)
        COMMAND_HANDLER_EX(IDC_MT32_NICE_AMP_RAMP, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_MT32_NICE_PANNING, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_MT32_NICE_PARTIAL_MIXING, BN_CLICKED, OnButtonClicked)

        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

    enum
    {
        IDD = IDD_PREFERENCES_FM
    };

private:
    BOOL OnInitDialog(CWindow, LPARAM) noexcept;

    void OnEditChange(UINT, int, CWindow) noexcept;
    void OnSelectionChange(UINT, int, CWindow) noexcept;
    void OnButtonClicked(UINT, int, CWindow) noexcept;

    bool HasChanged() noexcept;
    void OnChanged() const noexcept;

private:
    struct bank_t
    {
        int Id;
        std::wstring Name;

        bank_t() : Id(-1), Name() { }
        bank_t(const bank_t & other) : Id(other.Id), Name(other.Name) { }
        bank_t(int number, const std::wstring & name) : Id(number), Name(name) { }

        bank_t & operator =(const bank_t & other)
        {
            Id = other.Id;
            Name = other.Name;

            return *this;
        }

        bool operator ==(const bank_t & b) const { return Id == b.Id; }
        bool operator !=(const bank_t & b) const { return !operator ==(b); }
        bool operator < (const bank_t & b) const { return Name < b.Name; }
        bool operator > (const bank_t & b) const { return Name > b.Name; }
    };

    struct emulator_t
    {
        int Id;
        std::wstring Name;

        emulator_t() : Id(-1), Name() { }
        emulator_t(const emulator_t & other) : Id(other.Id), Name(other.Name) { }
        emulator_t(int number, const std::wstring & name) : Id(number), Name(name) { }

        emulator_t & operator =(const emulator_t & other)
        {
            Id = other.Id;
            Name = other.Name;

            return *this;
        }

        bool operator ==(const bank_t & b) const { return Id == b.Id; }
        bool operator !=(const bank_t & b) const { return !operator ==(b); }
        bool operator < (const bank_t & b) const { return Name < b.Name; }
        bool operator > (const bank_t & b) const { return Name > b.Name; }
    };

    std::vector<bank_t> _ADLBanks;

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

    std::vector<bank_t> _OPNBanks;

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
        CfgADLSoftPanning = (t_int32) SendDlgItemMessage(IDC_ADL_SOFT_PANNING, BM_GETCHECK);

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
            CfgOPNSoftPanning = (t_int32) SendDlgItemMessage(IDC_OPN_SOFT_PANNING, BM_GETCHECK);
        }
    }

    // MT32
    {
        // MT32 Conversion Quality
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_MT32_CONVERSION_QUALITY, CB_GETCURSEL);

            if (SelectedIndex < 0 || SelectedIndex > 3)
                SelectedIndex = 0;

            CfgMT32EmuConversionQuality = SelectedIndex;
        }

        // MT32 Max. Partials
        {
            int Value = std::clamp((int) GetDlgItemInt(IDC_MT32_MAX_PARTIALS, NULL, FALSE), 1, 32);

            SetDlgItemInt(IDC_MT32_MAX_PARTIALS, (UINT) Value, FALSE);

            CfgMT32EmuMaxPartials = Value;
        }

        // MT32 Analog Output Mode
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_MT32_ANALOG_OUTPUT_MODE, CB_GETCURSEL);

            if (SelectedIndex < 0 || SelectedIndex > 3)
                SelectedIndex = 0;

            CfgMT32EmuAnalogOutputMode = SelectedIndex;
        }

        // MT32 GM Set
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_MT32_GM_SET, CB_GETCURSEL);

            if (SelectedIndex < 0 || SelectedIndex >= (int) _countof(_MT32EmuSets))
                SelectedIndex = 0;

            CfgMT32EmuGMSet = SelectedIndex;
        }

        // MT32 DAC Input Mode
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_MT32_DAC_INPUT_MODE, CB_GETCURSEL);

            if (SelectedIndex < 0 || SelectedIndex > 3)
                SelectedIndex = 0;

            CfgMT32EmuDACInputMode = SelectedIndex;
        }

        // MT32 Nice Amp Ramp / Nice Panning / Nice Partial Mixing
        CfgMT32EmuNiceAmpRamp       = (t_int32) SendDlgItemMessage(IDC_MT32_NICE_AMP_RAMP, BM_GETCHECK);
        CfgMT32EmuNicePanning       = (t_int32) SendDlgItemMessage(IDC_MT32_NICE_PANNING, BM_GETCHECK);
        CfgMT32EmuNicePartialMixing = (t_int32) SendDlgItemMessage(IDC_MT32_NICE_PARTIAL_MIXING, BM_GETCHECK);
    }

    OnChanged();
}

/// <summary>
/// Resets this page's content to the default values. Does not apply any changes - lets user preview the changes before hitting "apply".
/// </summary>
void DialogPage::reset()
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
    }

    // MT32
    {
        // MT32 Conversion Quality
        SendDlgItemMessage(IDC_MT32_CONVERSION_QUALITY, CB_SETCURSEL, DefaultMT32EmuConversionQuality);

        // MT32 Max. Partials
        SetDlgItemInt(IDC_MT32_MAX_PARTIALS, DefaultMT32EmuMaxPartials, 0);

        // MT32 Analog Output Mode
        SendDlgItemMessage(IDC_MT32_ANALOG_OUTPUT_MODE, CB_SETCURSEL, DefaultMT32EmuAnalogOutputMode);

        // MT32 GM Set
        SendDlgItemMessage(IDC_MT32_GM_SET, CB_SETCURSEL, DefaultMT32EmuGMSet);

        // MT32 DAC Input Mode
        SendDlgItemMessage(IDC_MT32_DAC_INPUT_MODE, CB_SETCURSEL, DefaultMT32EmuDACInputMode);

        // Nice Amp Ramp / Nice Panning / Nice Partial Mixing
        CfgMT32EmuNiceAmpRamp       = (t_int32) SendDlgItemMessage(IDC_MT32_NICE_AMP_RAMP, BM_GETCHECK);
        CfgMT32EmuNicePanning       = (t_int32) SendDlgItemMessage(IDC_MT32_NICE_PANNING, BM_GETCHECK);
        CfgMT32EmuNicePartialMixing = (t_int32) SendDlgItemMessage(IDC_MT32_NICE_PARTIAL_MIXING, BM_GETCHECK);
    }

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

    // MT32Emu Conversion Quality
    {
        static const wchar_t * ConversionQualities[] = { L"Fastest", L"Fast", L"Good", L"Best" };

        auto w = (CComboBox) GetDlgItem(IDC_MT32_CONVERSION_QUALITY);

        for (const auto & Iter : ConversionQualities)
            w.AddString(Iter);

        w.SetCurSel((int) CfgMT32EmuConversionQuality);
    }

    // MT32Emu Max. Partials
    {
        SetDlgItemInt(IDC_MT32_MAX_PARTIALS, (UINT) CfgMT32EmuMaxPartials, 0);
    }

    // MT32Emu Analog Output Mode
    {
        static const wchar_t * AnalogOutputModes[] = { L"Digital Only", L"Coarse", L"Accurate", L"Oversampled" };

        auto w = (CComboBox) GetDlgItem(IDC_MT32_ANALOG_OUTPUT_MODE);

        for (const auto & Iter : AnalogOutputModes)
            w.AddString(Iter);

        w.SetCurSel((int) CfgMT32EmuAnalogOutputMode);
    }

    // MT32Emu GM Set
    {
        auto w = (CComboBox) GetDlgItem(IDC_MT32_GM_SET);

        for (const auto & Set : _MT32EmuSets)
            ::uSendMessageText(w, CB_ADDSTRING, 0, Set);

        w.SetCurSel((int) CfgMT32EmuGMSet);
    }

    // MT32Emu DAC Input Mode
    {
        static const wchar_t * DACInputModes[] = { L"Nice", L"Pure", L"Generation 1", L"Generation 2" };

        auto w = (CComboBox) GetDlgItem(IDC_MT32_DAC_INPUT_MODE);

        for (const auto & Iter : DACInputModes)
            w.AddString(Iter);

        w.SetCurSel((int) CfgMT32EmuDACInputMode);
    }

    // MT32Emu Nice Amp Ramp / Nice Panning / Nice Partial Mixing
    {
        SendDlgItemMessage(IDC_MT32_NICE_AMP_RAMP,       BM_SETCHECK, (WPARAM) CfgMT32EmuNiceAmpRamp);
        SendDlgItemMessage(IDC_MT32_NICE_PANNING,        BM_SETCHECK, (WPARAM) CfgMT32EmuNicePanning);
        SendDlgItemMessage(IDC_MT32_NICE_PARTIAL_MIXING, BM_SETCHECK, (WPARAM) CfgMT32EmuNicePartialMixing);
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

        case IDC_ADL_SOFT_PANNING:
        case IDC_OPN_SOFT_PANNING:
        case IDC_MT32_NICE_AMP_RAMP:
        case IDC_MT32_NICE_PANNING:
        case IDC_MT32_NICE_PARTIAL_MIXING:
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
    }

    // MT32
    {
        // MT32 Conversion Quality
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_MT32_CONVERSION_QUALITY, CB_GETCURSEL);

            if (SelectedIndex != CfgMT32EmuConversionQuality)
                return true;
        }

        // MT32 Max. Partials
        {
            if (GetDlgItemInt(IDC_MT32_MAX_PARTIALS, NULL, FALSE) != (UINT) CfgMT32EmuMaxPartials)
                return true;
        }

        // MT32 Analog Output Mode
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_MT32_ANALOG_OUTPUT_MODE, CB_GETCURSEL);

            if (SelectedIndex != CfgMT32EmuAnalogOutputMode)
                return true;
        }

        // MT32 GM Set
        {
            if (SendDlgItemMessage(IDC_MT32_GM_SET, CB_GETCURSEL) != (UINT) CfgMT32EmuGMSet)
                return true;
        }

        // MT32 DAC Input Mode
        {
            int SelectedIndex = (int) SendDlgItemMessage(IDC_MT32_DAC_INPUT_MODE, CB_GETCURSEL);

            if (SelectedIndex != CfgMT32EmuDACInputMode)
                return true;
        }

        // MT32 Nice Amp Ramp
        if (SendDlgItemMessage(IDC_MT32_NICE_AMP_RAMP, BM_GETCHECK) != CfgMT32EmuNiceAmpRamp)
            return true;

        // MT32 Nice Panning
        if (SendDlgItemMessage(IDC_MT32_NICE_PANNING, BM_GETCHECK) != CfgMT32EmuNicePanning)
            return true;

        // MT32 Nice Partial Mixing
        if (SendDlgItemMessage(IDC_MT32_NICE_PARTIAL_MIXING, BM_GETCHECK) != CfgMT32EmuNicePartialMixing)
            return true;
    }

    return false;
}

/// <summary>
/// Tells the host that our state has changed to enable/disable the Apply button appropriately.
/// </summary>
void DialogPage::OnChanged() const noexcept
{
    _Callback->on_state_changed();
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
        return GUID_PREFS_FM;
    }

    GUID get_parent_guid() noexcept
    {
        return PreferencesPageGUID;
    }
};

static preferences_page_factory_t<Page> PreferencesPageFactory;
