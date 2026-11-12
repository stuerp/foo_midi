
/** $VER: NLS.h (2026.08.14) P. Stuer **/

#pragma once

#include <sdkddkver.h>
#include <Windows.h>

#include <string>

namespace msc
{

class locale_t
{
public:
    locale_t() noexcept : _LocaleName(LOCALE_NAME_USER_DEFAULT)
    {
        Initialize();
    }

    const std::wstring FormatNumber(int64_t number) const noexcept;

private:
    void Initialize() noexcept;

public:
    NUMBERFMTW _NumberFormat;

private:
    const wchar_t * _LocaleName; // L"fr-FR";

    wchar_t _DecimalSep[16]  = { };
    wchar_t _ThousandSep[16] = { };
};

}
