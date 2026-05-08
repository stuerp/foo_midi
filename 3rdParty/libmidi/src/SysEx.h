
/** $VER: SysEx.h (2025.08.26) **/

#pragma once

#include "pch.h"

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 ALL_CPPCORECHECK_WARNINGS)

#include "MIDI.h"

namespace midi
{

#pragma warning(disable: 4820)

class sysex_t
{
public:
    sysex_t(const std::vector<uint8_t> & data) noexcept
    {
        _Data = data;
    }

    sysex_t(const uint8_t * data, size_t size) noexcept
    {
        _Data.resize(size);
        ::memcpy(_Data.data(), data, size);
    }

    void Identify() noexcept;

    std::string Manufacturer;
    std::string Model;
    std::string Command;
    std::string Description;

    bool IsChecksumValid;

    static bool IsSystemOn(const uint8_t * data) noexcept
    {
        return IsEqual(data, GM1SystemOn) || IsEqual(data, GM2SystemOn) || IsEqual(data, GSReset) || IsEqual(data, XGSystemOn);
    }

    static bool IsEqual(const uint8_t * a, const uint8_t * b) noexcept
    {
        while ((*a != StatusCode::SysExEnd) && (*b != StatusCode::SysExEnd) && (*a == *b))
        {
            a++;
            b++;
        }

        return (*a == *b);
    }

    static bool IsGMSystemOn(const uint8_t * data, size_t size) noexcept
    {
        return (size == _countof(GM1SystemOn) && ::memcmp(data, GM1SystemOn, _countof(GM1SystemOn)) == 0);
    }

    static bool IsGM2SystemOn(const uint8_t * data, size_t size) noexcept
    {
        return (size == _countof(GM2SystemOn) && ::memcmp(data, GM2SystemOn, _countof(GM2SystemOn)) == 0);
    }

    static bool IsGSSystemOn(const uint8_t * data, size_t size) noexcept
    {
        if (size != _countof(GSReset))
            return false;

        if (::memcmp(data, GSReset, 5) != 0)
            return false;

        if (::memcmp(data + 7, GSReset + 7, 2) != 0)
            return false;

        if (((data[5] + data[6] + 1) & 127) != data[9])
            return false;

        if (data[10] != GSReset[10])
            return false;

        return true;
    }

    static bool IsXGSystemOn(const uint8_t * data, size_t size) noexcept
    {
        return (size == _countof(XGSystemOn) && ::memcmp(data, XGSystemOn, _countof(XGSystemOn)) == 0);
    }

    /*
        Calculates the Roland SysEx checksum

        1. Add the values together, but if the answer to any sum exceeds 127 then subtract 128.
        2. Subtract the final answer from 128.

        F0 41 10 42 12 (40 11 00 41 63) 0B F7

            64 + 17 =  81
            81 +  0 =  81
            81 + 65 = 146
                (146 - 128 = 18)
            18 + 99 = 117

            128 - 117 = 11 <- Checksum
    */
    static uint8_t CalculateRolandCheckSum(uint8_t * data, size_t size) noexcept
    {
        if ((data == nullptr) || (size < 7))
            return 0;

        uint8_t Checksum = 0;

        for (size_t i = 5; (i + 1 < size) && (data[i + 1] != StatusCode::SysExEnd); ++i)
            Checksum += data[i];

        return (uint8_t) ((128 - Checksum) & 127);
    }

    static const uint8_t GM1SystemOn[6];
    static const uint8_t GM1SystemOff[6];

    static const uint8_t GM2SystemOn[6];

    static const uint8_t D50Reset[10];
    static const uint8_t MT32Reset[10];

    static const uint8_t GSReset[11];
    static const uint8_t GSToneMapNumber[11];

    static const uint8_t XGSystemOn[9];
    static const uint8_t XGReset[9];

    static const uint8_t DLSOn[6];
    static const uint8_t DLSOff[6];
    static const uint8_t DLSStaticVoiceAllocationOff[6];
    static const uint8_t DLSStaticVoiceAllocationOn[6];

private:
    uint32_t IdentifyManufacturer() noexcept;

    void IdentifyRoland() noexcept;
    void IdentifyYamaha() noexcept;

    static int GSBlockToPart(int value) noexcept;

    static const char * IdentifyGSReverbMacro(uint8_t value) noexcept;
    static const char * IdentifyGSChorusMacro(uint8_t value) noexcept;
    static const char * IdentifyGSDelayMacro(uint8_t value) noexcept;
    static const char * IdentifyGSRhythmPart(uint8_t value) noexcept;
    static const char * IdentifyGSToneMap(uint8_t value) noexcept;

private:
    std::vector<uint8_t> _Data;
    std::vector<uint8_t>::iterator _Iter;
};

/// <summary>
/// Maps a value from one range (srcMin, srcMax) to another (dstMin, dstMax).
/// </summary>
template<class T, class U>
inline static U Map(T value, T srcMin, T srcMax, U dstMin, U dstMax)
{
    return dstMin + (U) (((double) (value - srcMin) * (double) (dstMax - dstMin)) / (double) (srcMax - srcMin));
}

}
