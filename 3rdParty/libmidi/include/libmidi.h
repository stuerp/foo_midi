
/** $VER: libmidi.h (2026.05.09) P. Stuer **/

#pragma once

#include <SDKDDKVer.h>

#define NOMINMAX

#include <winsock2.h>
#include <windows.h>
#include <wincodec.h>

#include <utility>

#ifdef __TRACE
extern uint32_t __TRACE_LEVEL;

#define TRACE_RESET()           { __TRACE_LEVEL = 0; }
#define TRACE_INDENT()          { ::printf("%*s{\n", __TRACE_LEVEL * 4, ""); __TRACE_LEVEL++; }
#define TRACE_UNINDENT()        { __TRACE_LEVEL--; ::printf("%*s}\n", __TRACE_LEVEL * 4, ""); }

#else

#define TRACE_RESET()           {  }
#define TRACE_INDENT()          {  }
#define TRACE_FORM(type, size)  {  }
#define TRACE_LIST(type, size)  {  }
#define TRACE_CHUNK(id, size)   {  }
#define TRACE_UNINDENT()        {  }

#endif

namespace midi
{

/// <summary>
/// Creates a MIDI message by packing the status code, data bytes and port number into a single 32-bit integer.
/// </summary>
inline uint32_t PackMessage(uint8_t statusCode, uint8_t data1, uint8_t data2, uint8_t portNumber) noexcept
{
    return (uint32_t) ((portNumber << 24) | (data2 << 16) | (data1 << 8) | statusCode);
}

/// <summary>
/// Converts  a pitch bend value (-8192 to +8191) to two MIDI data bytes (LSB, MSB).
/// </summary>
inline std::pair<uint8_t, uint8_t> PitchBendToBytes(int32_t value) noexcept
{
    // Clamp to valid 14-bit range.
    value = msc::Clamp(value, -8192, 8191);

    const auto Value = (uint16_t) (value + 8192);

    const uint8_t LSB = (uint8_t) Value       & 0x7F; // Lower 7 bits
    const uint8_t MSB =          (Value >> 7) & 0x7F; // Upper 7 bits

    return { LSB, MSB };
}

// Convert two MIDI data bytes back to pitch bend value (-8192 to +8191)
inline int32_t BytesToPitchBend(uint8_t lsb, uint8_t msb) noexcept
{
    uint16_t Value = ((uint16_t) msb << 7) | lsb;

    return (int32_t) Value - 8192;
}

}
