
/** $VER: SoundfontList.h (2025.08.15) P. Stuer - Represents a soundfont. **/

#pragma once

#include <string>

#include <bassmidi.h>

#pragma warning(disable: 4820) // x bytes padding added after data member

class soundfont_t
{
public:
    soundfont_t() noexcept : Gain(), BankOffset(), IsEmbedded(false), IsDLS(false)
    {
    }

    soundfont_t(const fs::path & filePath, float gain, int bankOffset, bool isEmbedded, bool isDLS) noexcept
    {
        FilePath = filePath;
        Gain = gain;
        BankOffset = bankOffset;
        IsEmbedded = isEmbedded;
        IsDLS = isDLS;
    }

    bool operator ==(const soundfont_t & other) const noexcept
    {
        if (FilePath != other.FilePath)
            return false;

        if (Gain != other.Gain)
            return false;

        if (BankOffset != other.BankOffset)
            return false;

        if (IsEmbedded != other.IsEmbedded)
            return false;

        return (IsDLS == other.IsDLS);
    }

public:
    fs::path FilePath;
    float Gain;
    int BankOffset;
    bool IsEmbedded;
    bool IsDLS;

    std::vector<BASS_MIDI_FONTEX> Fonts;
};

std::vector<soundfont_t> LoadSoundfontList(const fs::path & filePath);
