
/** $VER: KaraokeProcessor.h (2024.05.18) **/

#pragma once

#include <stdint.h>

#include <pfc/pfc-lite.h>

class KaraokeProcessor
{
public:
    KaraokeProcessor() noexcept { }

    pfc::string8 GetUnsyncedLyrics() const { return _UnsyncedLyrics; }
    pfc::string8 GetSyncedLyrics() const { return _SyncedLyrics; }

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
    void AddLyrics(pfc::string8 & lyrics, uint32_t timestamp, const char * text);

    static void FormatTimestamp(uint32_t timestamp, char * text, size_t size) noexcept;

private:
    pfc::string8 _UnsyncedLyrics;
    pfc::string8 _SyncedLyrics;
};
