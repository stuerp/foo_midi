
/** $VER: Stream.h (2026.05.08) P. Stuer - Based on Valley Bell's mmd2mid (https://github.com/ValleyBell/MidiConverters). **/

#pragma once

#include <CppCoreCheck\Warnings.h>

#pragma warning(disable: 4100 4514 4625 4626 4710 4711 4738 5045 ALL_CPPCORECHECK_WARNINGS)

#include <cstdint>

#include "RunningNotes.h"

namespace mmd
{

inline void WriteBE32(uint8_t * buffer, uint32_t value);
inline void WriteBE16(uint8_t * buffer, uint16_t value);

class stream_t
{
public:
    stream_t() : ChannelNumber(), DeltaTime(), Data(), Offset(), _Size(), _RunningStatus(), _TrackOffset() {}

    stream_t(size_t initialSize) : Data((uint8_t *) ::malloc(initialSize)), Offset(), _Size(initialSize) {}

    void BeginTrack(uint8_t channelNumber) noexcept
    {
        ChannelNumber = channelNumber;

        _RunningNotes._Count = 0;
    }

    void EndTrack() noexcept
    {
        _RunningNotes.Flush(*this);
    }

    void Add(uint8_t channel, uint8_t note, uint8_t velocity, uint32_t duration) noexcept
    {
        _RunningNotes.Add(channel, note, velocity, duration);
    }

    bool EmitNote(stream_t & stream, uint8_t note, uint8_t duration) noexcept
    {
        return _RunningNotes.EmitNote(stream, note, duration);
    }

    void WriteHeader(uint16_t format, uint16_t tracks, uint16_t resolution) noexcept;

    void WriteTrackBegin() noexcept;
    void WriteTrackEnd() noexcept;

    void WriteEvent(uint8_t event, uint8_t param1, uint8_t param2) noexcept;
    void WriteEvent(uint8_t event, const void * data, uint32_t size) noexcept;
    void WriteMetaEvent(uint8_t metaType, const void * data, uint32_t dataLen) noexcept;

    void WriteVariableLengthQuantity(uint32_t value) noexcept;

    void Grow(uint32_t bytesNeeded) noexcept;

private:
    void WriteDeltaTime(uint32_t & deltaTime) noexcept;
    void WriteEvent_(uint8_t event, uint8_t param1, uint8_t param2) noexcept;

public:
    uint8_t ChannelNumber;
    uint32_t DeltaTime;

    uint8_t * Data;
    size_t Offset;

private:
    size_t _Size;

    uint8_t _RunningStatus;
    uint32_t _TrackOffset;

    running_notes_t _RunningNotes;
};

/// <summary>
/// Writes a 32-bit unsigned integer in big-endian format to the specified buffer.
/// </summary>
inline void WriteBE32(uint8_t * data, uint32_t value)
{
    data[0x00] = (value >> 24) & 0xFFu;
    data[0x01] = (value >> 16) & 0xFFu;
    data[0x02] = (value >>  8) & 0xFFu;
    data[0x03] = (value >>  0) & 0xFFu;
}

/// <summary>
/// Writes a 16-bit unsigned integer in big-endian format to the specified buffer.
/// </summary>
inline void WriteBE16(uint8_t * data, uint16_t value)
{
    data[0x00] = (value >> 8) & 0xFFu;
    data[0x01] = (value >> 0) & 0xFFu;
}

}
