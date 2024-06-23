
/** $VER: KaraokeProcessor.h (2024.05.18) **/

#include "framework.h"

#include "KaraokeProcessor.h"

#include <string>
#include <chrono>

#include <Encoding.h>

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
