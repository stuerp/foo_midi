
/** $VER: NLS.cpp (2026.08.14) P. Stuer - National Language Support routines **/

#include "pch.h"

namespace msc
{

/// <summary>
/// Formats a number.
/// </summary>
const std::wstring locale_t::FormatNumber(int64_t number) const noexcept
{
    wchar_t Text[32] = { };

    ::_i64tow_s(number, Text, _countof(Text), 10);

    wchar_t FormattedText[64] = { };

    (void) ::GetNumberFormatEx(_LocaleName, 0, Text, &_NumberFormat, FormattedText, _countof(FormattedText));

    return FormattedText;
}

/// <summary>
/// Initializes this instance.
/// </summary>
void locale_t::Initialize() noexcept
{
    wchar_t Data[16];

    int Result = ::GetLocaleInfoEx(_LocaleName, LOCALE_IDIGITS, Data, _countof(Data));

    if (Result == 0)
        return;

    _NumberFormat.NumDigits = (UINT) _wtoi(Data);

    Result = ::GetLocaleInfoEx(_LocaleName, LOCALE_ILZERO, Data, _countof(Data));

    if (Result == 0)
        return;

    _NumberFormat.LeadingZero = (UINT) _wtoi(Data);

    Result = ::GetLocaleInfoEx(_LocaleName, LOCALE_SGROUPING, Data, _countof(Data));

    if (Result == 0)
        return;

    _NumberFormat.Grouping = (Data[0] == L'3') ? 3 : 0;  // More robust conversion needed for other patterns.

    Result = ::GetLocaleInfoEx(_LocaleName, LOCALE_SDECIMAL,  _DecimalSep,  _countof(_DecimalSep));

    if (Result == 0)
        return;

    _NumberFormat.lpDecimalSep = _DecimalSep;

    Result = ::GetLocaleInfoEx(_LocaleName, LOCALE_STHOUSAND, _ThousandSep, _countof(_ThousandSep));

    if (Result == 0)
        return;

    _NumberFormat.lpThousandSep = _ThousandSep;

    Result = ::GetLocaleInfoEx(_LocaleName, LOCALE_INEGNUMBER, Data, _countof(Data));

    if (Result == 0)
        return;

    _NumberFormat.NegativeOrder = (UINT) _wtoi(Data);
}

}
