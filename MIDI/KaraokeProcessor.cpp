
/** $VER: KaraokeProcessor.h (2025.06.20) **/

#include "pch.h"

#include "KaraokeProcessor.h"

#include <chrono>

/// <summary>
/// Adds a text line.
/// </summary>
void KaraokeProcessor::AddSyncedText(const char * text)
{
    _SyncedLyrics += u8"[by:";
    _SyncedLyrics += (const char8_t *) text;
    _SyncedLyrics += u8"]\r\n";
}

/// <summary>
/// Adds a lyrics line.
/// </summary>
void KaraokeProcessor::AddLyrics(std::u8string & lyrics, uint32_t timestamp, const char * text)
{
    char Timestamp[64] = { };

    std::string Line = text;

    {
        const auto n = Line.find("\\");

        if (n != std::string::npos)
        {
            lyrics += u8"\r\n";

            KaraokeProcessor::FormatTimestamp(timestamp, Timestamp, _countof(Timestamp));

            Line.replace(n, 1, Timestamp);
        }
    }

    {
        const auto n = Line.find("/");

        if (n != std::string::npos)
        {
            lyrics += u8"\r\n";

            KaraokeProcessor::FormatTimestamp(timestamp, Timestamp, _countof(Timestamp));

            Line.replace(n, 1, Timestamp);
        }
    }

    lyrics += (const char8_t *) Line.c_str();
}

/// <summary>
/// Formats a MIDI timestamp in ms to LRC timestamp format.
/// </summary>
/// <reference>https://en.wikipedia.org/wiki/LRC_(file_format)</reference>
void KaraokeProcessor::FormatTimestamp(uint32_t timestamp, char * text, size_t size)noexcept
{
    using namespace std::chrono;

    std::chrono::milliseconds ms = (std::chrono::milliseconds) timestamp;

    auto ss = duration_cast<seconds>(ms); ms -= duration_cast<milliseconds>(ss);
    auto mm = duration_cast<minutes>(ss); ss -= duration_cast<seconds>(mm);
    auto hh = duration_cast<hours>  (mm); mm -= duration_cast<minutes>(hh);

    ::sprintf_s(text, size, "[%02d:%02lld.%02lld]", mm.count(), ss.count(), ms.count() / 10);
}
