
/** $VER: KaraokeProcessor.h (2025.06.20) **/

#pragma once

#include <stdint.h>
#include <string>

class KaraokeProcessor
{
public:
    KaraokeProcessor() noexcept { }

    std::u8string GetUnsyncedLyrics() const { return _UnsyncedLyrics; }
    std::u8string GetSyncedLyrics() const { return _SyncedLyrics; }

    void AddUnsyncedLyrics(uint32_t timestamp, const char * text)
    {
        AddLyrics(_UnsyncedLyrics, timestamp, text);
    }

    void AddSyncedLyrics(uint32_t timestamp, const char * text)
    {
        AddLyrics(_SyncedLyrics, timestamp, text);
    }

    void AddSyncedText(const char * text);

private:
    void AddLyrics(std::u8string & lyrics, uint32_t timestamp, const char * text);

    static void FormatTimestamp(uint32_t timestamp, char * text, size_t size) noexcept;

private:
    std::u8string _UnsyncedLyrics;
    std::u8string _SyncedLyrics;
};
