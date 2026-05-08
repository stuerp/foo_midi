
/** $VER: Support.cpp (2025.03.21) P. Stuer - Based on Valley Bell's rpc2mid (https://github.com/ValleyBell/MidiConverters). **/

#include "pch.h"

#include "Support.h"

namespace rcp
{

/// <summary>
/// 
/// </summary>
uint16_t GetTrimmedLength(const char * data, uint16_t size, char trimChar, bool leaveLast)
{
    uint16_t i;

    for (i = size; i > 0; --i)
    {
        if (data[i - 1] != trimChar)
            break;
    }

    if (leaveLast && (i < size))
        i++;

    return i;
}

/// <summary>
/// Converts RCP BPM to MIDI ticks.
/// </summary>
uint32_t BPM2Ticks(uint16_t bpm, uint8_t scale)
{
//  formula: Ticks = (60 000 000.0 / bpm) * (scale / 64.0)
    const uint32_t Denominator = (uint32_t) (bpm * scale);

    return ((uint64_t) (60u * 1000u * 1000u) * 64u) / Denominator;
}

/// <summary>
/// Gets the address of the file name in the specified file path.
/// </summary>
const wchar_t * GetFileName(const wchar_t * filePath)
{
    const wchar_t * Sep1 = ::wcsrchr(filePath, '/');
    const wchar_t * Sep2 = ::wcsrchr(filePath, '\\');

    if (Sep1 == nullptr)
        Sep1 = Sep2;
    else
    if ((Sep2 != nullptr) && (Sep2 > Sep1))
        Sep1 = Sep2;

    return (Sep1 != nullptr) ? (Sep1 + 1) : filePath;
}

}
