
/** $VER: SoundFont.h (2024.08.25) **/

#pragma once

#include <string>

#pragma warning(disable: 4820) // x bytes padding added after data member
class soundfont_t
{
public:
    soundfont_t() noexcept : _Volume(1.f), _BankOffset(), _IsEmbedded(false), _IsDLS(false)
    {
    }

    soundfont_t(const std::string & filePath, float volume, int bankOffset, bool isEmbedded, bool isDLS) noexcept
    {
        _FilePath = filePath;
        _Volume = volume;
        _BankOffset = bankOffset;
        _IsEmbedded = isEmbedded;
        _IsDLS = isDLS;
    }

    bool operator ==(const soundfont_t & other) const noexcept
    {
        if (_FilePath != other._FilePath)
            return false;

        if (_Volume != other._Volume)
            return false;

        if (_BankOffset != other._BankOffset)
            return false;

        if (_IsEmbedded != other._IsEmbedded)
            return false;

        return (_IsDLS == other._IsDLS);
    }

    const std::string & FilePath() const noexcept { return _FilePath; }
    float Volume() const noexcept { return _Volume; }
    int BankOffset() const noexcept { return _BankOffset; }
    bool IsEmbedded() const noexcept { return _IsEmbedded; }
    bool IsDLS() const noexcept { return _IsDLS; }

private:
    std::string _FilePath;
    float _Volume;
    int _BankOffset;
    bool _IsEmbedded;
    bool _IsDLS;
};
