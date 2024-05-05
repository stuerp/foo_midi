
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
/// Convert an ANSI string to UTF-8.
/// </summary>
void KaraokeProcessor::UTF8Encode(pfc::string8 text, pfc::string8 & utf8)
{
    if (pfc::is_valid_utf8(text))
        return;

    UINT CodePage = CP_ACP; // ANSI

    if (!pfc::is_lower_ascii(text))
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
