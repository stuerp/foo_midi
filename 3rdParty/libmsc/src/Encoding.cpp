
/** $VER: Encoding.cpp (2025.09.06) P. Stuer - Encoding conversion routines **/

#include "pch.h"

#include <map>

namespace msc
{

static const std::map<const std::string, uint32_t> EncodingToCodePage
{
    { "shift-jis",        932 },    // Shift-JIS

    { "utf-8",          65001 },    // UTF-8

    { "windows-1250",    1250 },    // Central Europe
    { "windows-1251",    1251 },    // Cyrillic
    { "windows-1252",    1252 },    // Western
    { "windows-1253",    1253 },    // Greek
    { "windows-1254",    1254 },    // Turkish
    { "windows-1255",    1255 },    // Hebrew
    { "windows-1256",    1256 },    // Arabic
    { "windows-1257",    1257 },    // Baltic
    { "windows-1258",    1258 },    // Vietnamese
};

/// <summary>
/// Converts an UTF-16 string of UTF-8.
/// </summary>
std::string WideToUTF8(const wchar_t * wide, size_t size = 0) noexcept
{
    if (size == 0)
        size = ::wcslen(wide);

    int Size = ::WideCharToMultiByte(CP_UTF8, 0, wide, (int) size, nullptr, 0, nullptr, nullptr);

    std::string UTF8;

    UTF8.resize((size_t) Size);

    ::WideCharToMultiByte(CP_UTF8, 0, wide,  (int) size, UTF8.data(), (int) UTF8.size(), nullptr, nullptr);

    return UTF8;
}

/// <summary>
/// Converts a string encoded with the specified code page to an UTF-16 string.
/// </summary>
std::wstring CodePageToWide(uint32_t codePage, const char * text, size_t size) noexcept
{
    int Size = ::MultiByteToWideChar(codePage, 0, text, (int) size, nullptr, 0);

    std::wstring Wide;

    Wide.resize((size_t) Size);

    ::MultiByteToWideChar(codePage, 0, text,  (int) size, Wide.data(), (int) Wide.size());

    return Wide;
}

/// <summary>
/// Converts a string encoded with the specified code page to an UTF-8 string.
/// </summary>
std::string CodePageToUTF8(uint32_t codePage, const char * text, size_t size) noexcept
{
    return WideToUTF8(CodePageToWide(codePage, text, size));
}

/// <summary>
/// Formats a string using a format specification and a list of arguments.
/// </summary>
std::string FormatText(const char * format, ...) noexcept
{
   va_list vl;

   va_start(vl, format);

    std::string Text;

    Text.resize(256);

    ::vsprintf_s(Text.data(), Text.size(), format, vl);

    va_end(vl);

    return Text;
}

/// <summary>
/// Formats a string using a format specification and a list of arguments.
/// </summary>
std::wstring FormatText(const wchar_t * format, ...) noexcept
{
   va_list vl;

   va_start(vl, format);

    std::wstring Text;

    Text.resize(256);

    ::vswprintf_s(Text.data(), Text.size(), format, vl);

    va_end(vl);

    return Text;
}

/// <summary>
/// Returns true if the specified text is EUC-JP encoded. (http://www.rikai.com/library/kanjitables/kanji_codes.euc.shtml)
/// </summary>
bool IsEUCJP(const char * text, size_t size) noexcept
{
    while (size != 0)
    {
        uint8_t d1 = (uint8_t) *text++;
        size--;

        if (!((d1 >= 0xA1 && d1 <= 0xAD) || (d1 >= 0xB0 && d1 <= 0xFE)))
            continue;

        if (size == 0)
            return false;

        uint8_t d2 = (uint8_t) *text++;
        size--;

        if (!(d2 >= 0xA0 && d1 <= 0xFF))
            return false;
    }

    return true;
}

/// <summary>
/// Returns true if the specified text is Shift-JIS encoded. (http://www.rikai.com/library/kanjitables/kanji_codes.sjis.shtml)
/// </summary>
bool IsShiftJIS(const char * text, size_t size) noexcept
{
    while (size != 0)
    {
        uint8_t d1 = (uint8_t) *text++;
        size--;

        if (!((d1 >= 0x81 && d1 <= 0x84) || (d1 >= 0x87 && d1 <= 0x9F) || (d1 >= 0xE0 && d1 <= 0xEF)))
            continue;

        if (size == 0)
            return false;

        uint8_t d2 = (uint8_t) *text++;
        size--;

        if (!((d2 >= 0x40 && d2 <= 0x9E) || (d2 >= 0x9F && d2 <= 0xFC)))
            return false;
    }

    return true;
}

/// <summary>
/// Returns true if the specified text is UTF-8 encoded.
/// </summary>
bool IsUTF8(const char * text, size_t size) noexcept
{
    size_t n = 0;

    for (size_t i = 0; i < size; ++i)
    {
        uint8_t c = (uint8_t ) text[i];

        //if (c==0x09 || c==0x0a || c==0x0d || (0x20 <= c && c <= 0x7e) ) n = 0; // is_printable_ascii
        if ((0x00 <= c) && (c <= 0x7F))
            n = 0; // 0bbbbbbb
        else
        if ((c & 0xE0) == 0xC0)
            n = 1; // 110bbbbb
        else
        if ((c == 0xED) && (i < (size - 1)) && (((uint8_t) text[i + 1] & 0xA0) == 0xA0))
            return false; // U+d800 to U+dfff

        if ((c & 0xF0) == 0xE0)
            n = 2; // 1110bbbb
        else
        if ((c & 0xF8) == 0xF0)
            n = 3; // 11110bbb
//      else
//      if ((c & 0xFC) == 0xF8)
//          n = 4; // 111110bb // byte 5, unnecessary in 4 byte UTF-8
//      if ((c & 0xFE) == 0xFC)
//          n = 5; // 1111110b // byte 6, unnecessary in 4 byte UTF-8
        else
            return false;

        // Are there n bytes matching 10bbbbbb following?
        for (size_t j = 0; (j < n) && (i < size); ++j)
        { 
            if ((++i == size) || (((uint8_t) text[i] & 0xC0) != 0x80))
                return false;
        }
    }

    return true;
}

/// <summary>
/// Returns true if the specified text is ASCII encoded.
/// </summary>
bool IsASCII(const char * text) noexcept
{
    while (*text)
    {
        if (*text < 0)
            return false;

        ++text;
    }

    return true;
}

/// <summary>
/// Returns true if the specified text is ASCII encoded.
/// </summary>
bool IsASCII(const char * text, size_t size) noexcept
{
    while (size != 0)
    {
        if (*text < 0)
            return false;

        ++text;
        --size;
    }

    return true;
}

/// <summary>
/// Converts a string in a unknown encoding to UTF-16.
/// </summary>
std::wstring TextToWide(const char * text, size_t size) noexcept
{
    if (size == 0)
        size = ::strlen(text);

    if (IsASCII(text))
        return UTF8ToWide(text);

    if (IsUTF8(text, size))
        return UTF8ToWide(text);

    if (IsShiftJIS(text, size))
        return CodePageToWide(932, text);

    if (IsEUCJP(text, size))
        return CodePageToWide(20932, text);

    return CodePageToWide(51932, text);
}

/// <summary>
/// Converts a string in a unknown encoding to UTF-8.
/// </summary>
std::string TextToUTF8(const char * text, size_t size) noexcept
{
    if (size == 0)
        size = ::strlen(text);

    if (IsASCII(text, size) || IsUTF8(text, size))
    {
        std::string Text;

        Text.resize(size + 1);

        ::memcpy(Text.data(), text, size);
        Text[size] = '\0';

        return Text;
    }

    if (IsShiftJIS(text, size))
        return CodePageToUTF8(932, text, size);

    if (IsEUCJP(text, size))
        return CodePageToUTF8(20932, text, size);

    return CodePageToUTF8(51932, text, size);
}

/// <summary>
/// Gets the matching Windows code page for the specfied encoding.
/// </summary>
bool GetCodePageFromEncoding(const std::string & encoding, uint32_t & codePage) noexcept
{
    std::string Key;

    Key.resize(encoding.size());

    std::transform(encoding.begin(), encoding.end(), Key.begin(), ::tolower);

    auto Item = EncodingToCodePage.find(Key);

    if (Item == EncodingToCodePage.end())
        return false;

    codePage = Item->second;

    return true;
}

#ifdef _DEBUG
// Test cases
const uint8_t Data1[] =
{
    0x46, 0x69, 0x6e, 0x61, 0x6c, 0x20, 0x46, 0x61, 0x6e, 0x74, 0x61, 0x73, 0x79, 0x20, 0x35, 0x20, 0x5b, 0x20, 0x83, 0x72, 0x83, 0x62, 0x83, 0x4f,
    0x83, 0x75, 0x83, 0x8a, 0x83, 0x62, 0x83, 0x61, 0x82, 0xcc, 0x8e, 0x80, 0x93, 0xac, 0x20, 0x81, 0x66, 0x82, 0x58, 0x82, 0x58, 0x20, 0x2d, 0x73,
    0x69, 0x6e, 0x67, 0x6c, 0x65, 0x20, 0x65, 0x64, 0x69, 0x74, 0x2d, 0x20, 0x5d, 0x20, 0x66, 0x6f, 0x72, 0x20, 0x53, 0x43, 0x2d, 0x38, 0x38, 0x50,
    0x72, 0x6f, 0x20, 0x32, 0x50, 0x6f, 0x72, 0x74, 0x20, 0x76, 0x65, 0x72, 0x37, 0x2e, 0x30, 0x20, 0x62, 0x79, 0x20, 0x4c, 0x69, 0x78, 0x00
};
const WCHAR * Text1 = L"Final Fantasy 5 [ ビッグブリッヂの死闘 ’９９ -single edit- ] for SC-88Pro 2Port ver7.0 by Lix";

struct Test
{
    const uint8_t * Data;
    const WCHAR * Text;
} Tests[] =
{
    { Data1, Text1 },   // Mixed ASCII and Shift-JIS
};
#endif

}
