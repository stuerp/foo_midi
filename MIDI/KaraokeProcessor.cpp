
/** $VER: KaraokeProcessor.h (2024.05.05) **/

#include "framework.h"

#include "KaraokeProcessor.h"

#include <string>
#include <chrono>

/// <summary>
/// Adds a text line.
/// </summary>
void KaraokeProcessor::AddSyncedText(const char * text)
{
    _SyncedLyrics += "[by:";
    _SyncedLyrics += text;
    _SyncedLyrics += "]\r\n";
}

/// <summary>
/// Adds a lyrics line.
/// </summary>
void KaraokeProcessor::AddLyrics(pfc::string8 & lyrics, uint32_t timestamp, const char * text)
{
    char Timestamp[64];

    std::string Line = text;

    {
        const auto n = Line.find("\\");

        if (n != std::string::npos)
        {
            lyrics += "\r\n";

            KaraokeProcessor::FormatTimestamp(timestamp, Timestamp, _countof(Timestamp));

            Line.replace(n, 1, Timestamp);
        }
    }

    {
        const auto n = Line.find("/");

        if (n != std::string::npos)
        {
            lyrics += "\r\n";

            KaraokeProcessor::FormatTimestamp(timestamp, Timestamp, _countof(Timestamp));

            Line.replace(n, 1, Timestamp);
        }
    }

    lyrics += Line.c_str();
}

/// <summary>
/// Formats a MIDI timestamp in ms to LRC timestamp format.
/// </summary>
/// <reference>https://en.wikipedia.org/wiki/LRC_(file_format)</reference>
void KaraokeProcessor::FormatTimestamp(uint32_t timestamp, char * text, size_t size)noexcept
{
    using namespace std::chrono;

    std::chrono::milliseconds ms = (std::chrono::milliseconds)timestamp;

    auto ss = duration_cast<seconds>(ms);
    ms -= duration_cast<milliseconds>(ss);
    auto mm = duration_cast<minutes>(ss);
    ss -= duration_cast<seconds>(mm);
    auto hh = duration_cast<hours>(mm);
    mm -= duration_cast<minutes>(hh);

    ::sprintf_s(text, size, "[%02d:%02lld.%02lld]", mm.count(), ss.count(), ms.count() / 10);
}

/// <summary>
/// Is the data a EUC-JP string?
/// http://www.rikai.com/library/kanjitables/kanji_codes.euc.shtml
/// </summary>
static bool IsValidEUCJP(const char * data, size_t size)
{
    while (size != 0)
    {
        uint8_t d1 = (uint8_t) *data++;
        size--;

        if (d1 > 0x80)
        {
            if (size == 0)
                return true;

            uint8_t d2 = (uint8_t) *data++;
            size--;

            if (!((d1 >= 0xA1 && d1 <= 0xAD) || (d1 >= 0xB0 && d1 <= 0xFE)))
                return false;

            if (!(d2 >= 0xA0 && d1 <= 0xFF))
                return false;
        }
    }

    return true;
}

/// <summary>
/// Is the data a Shift-JIS string?
/// char ShiftJIS[] = { 0x82, 0xA0, 0x82, 0xA2, 0x82, 0xA4 }; / char UTF8[] = { 0xE3, 0x81, 0x82, 0xE3, 0x81, 0x84, 0xE3, 0x81, 0x86 };
/// http://www.rikai.com/library/kanjitables/kanji_codes.sjis.shtml
/// </summary>
static bool IsValidShiftJIS(const char * data, size_t size)
{
    while (size != 0)
    {
        uint8_t d1 = (uint8_t) *data++;
        size--;

        if (d1 > 0x80)
        {
            if (size == 0)
                return false;

            uint8_t d2 = (uint8_t) *data++;
            size--;

            if (!((d1 >= 0x81 && d1 <= 0x84) || (d1 >= 0x87 && d1 <= 0x9F) || (d1 >= 0xE0 && d1 <= 0xEF)))
                return false;

            if (!((d2 >= 0x40 && d2 <= 0x9E) || (d2 >= 0x9F && d2 <= 0xFC)))
                return false;
        }
    }

    return true;
}

/// <summary>
/// Returns true if the specified text is UTF-8 encoded.
/// </summary>
bool IsUTF8(const char * text, size_t size)
{
    size_t n;

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
            return false; //U+d800 to U+dfff
        else
        if ((c & 0xF0) == 0xE0)
            n = 2; // 1110bbbb
        else
        if ((c & 0xF8) == 0xF0)
            n = 3; // 11110bbb
//      else
//      if ((c & 0xFC) == 0xF8)
//          n = 4; // 111110bb //byte 5, unnecessary in 4 byte UTF-8
//      if ((c & 0xFE) == 0xFC)
//          n = 5; // 1111110b //byte 6, unnecessary in 4 byte UTF-8
        else
            return false;

        // n bytes matching 10bbbbbb follow?
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
bool IsASCII(const char * text)
{
    while (*text)
    {
        if (*text < 0)
            return false;

        text++;
    }

    return true;
}

/// <summary>
/// Convert an ANSI string to UTF-8.
/// </summary>
void KaraokeProcessor::UTF8Encode(pfc::string8 text, pfc::string8 & utf8)
{
    if (IsUTF8(text, ::strlen(text)))
        return;

    UINT CodePage = CP_ACP; // ANSI

    if (IsASCII(text))
    {
        if (IsValidShiftJIS(text, ::strlen(text)))
            CodePage = 932; // Shift-JIS
        else
        if (IsValidEUCJP(text, ::strlen(text)))
            CodePage = 20932; // EUC-JP
    }

//  const char * Ansi = "cañón";

    size_t Length = text.length(); 

    WCHAR * Wide = new WCHAR[Length + 1];

    if (Wide != nullptr)
    {
        // Convert the bytes to wide characters.
        int Result = ::MultiByteToWideChar(CodePage, 0, text, -1, Wide, (int) Length); // ANSI to Unicode

        Wide[Length] = '\0';

        // Determine the size of the UTF-8 buffer.
        const size_t UTF8Size = (size_t) ::WideCharToMultiByte(CP_UTF8, 0, Wide, -1, 0, 0, 0, 0); // Unicode to UTF-8

        if (UTF8Size == 0)
            return;

        char * UTF8 = new char[UTF8Size + 1];

        if (UTF8)
        {
            // Convert the wide characters to UTF-8.
            Result = ::WideCharToMultiByte(CP_UTF8, 0, Wide, -1, UTF8, (int) UTF8Size, 0, 0); // Unicode to UTF-8

            utf8 = UTF8;

            delete[] UTF8;
        }

        delete[] Wide;
    }
}
