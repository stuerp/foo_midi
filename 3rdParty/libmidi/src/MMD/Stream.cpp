
/** $VER: Stream.cpp (2026.05.07) P. Stuer - Based on Valley Bell's mmd2mid (https://github.com/ValleyBell/MidiConverters). **/

#include "pch.h"

#include <CppCoreCheck\Warnings.h>

#pragma warning(disable: 4100 4514 4625 4626 4710 4711 4738 5045 ALL_CPPCORECHECK_WARNINGS)

#include <cstdint>

#include "Stream.h"

#include <MIDI.h>

namespace mmd
{

/// <summary>
/// Writes the SMF header.
/// </summary>
void stream_t::WriteHeader(uint16_t format, uint16_t trackCount, uint16_t resolution) noexcept
{
    Grow(0x08 + 0x06);

    WriteBE32(&Data[Offset + 0x00], 0x4D546864);    // 'MThd'
    WriteBE32(&Data[Offset + 0x04], 0x00000006);    // Header Length
    Offset += 0x08;

    WriteBE16(&Data[Offset + 0x00], format);        // MIDI Format (0/1/2)
    WriteBE16(&Data[Offset + 0x02], trackCount);    // number of tracks
    WriteBE16(&Data[Offset + 0x04], resolution);    // Ticks per Quarter
    Offset += 0x06;
}

/// <summary>
/// Writes the SMF track header.
/// </summary>
void stream_t::WriteTrackBegin() noexcept
{
    Grow(0x08);

    WriteBE32(&Data[Offset + 0x00], 0x4D54726B);    // 'MTrk'
    WriteBE32(&Data[Offset + 0x04], 0x00000000);    // Write dummy length
    Offset += 0x08;

    RunningStatus = 0x00;

    TrackOffset   = (uint32_t) Offset;
    DeltaTime     = 0;
}

/// <summary>
/// Writes the SMF track end.
/// </summary>
void stream_t::WriteTrackEnd() noexcept
{
    const uint32_t n = (uint32_t) (Offset - TrackOffset);

    WriteBE32(&Data[TrackOffset - 0x04], n);
}

/// <summary>
/// Writes a MIDI event.
/// </summary>
void stream_t::WriteEvent(uint8_t statusCode, uint8_t param1, uint8_t param2) noexcept
{
    RunningStatus = 0x00;

    WriteEvent_(statusCode, param1, param2);
}

/// <summary>
/// 
/// </summary>
void stream_t::WriteEvent(uint8_t statusCode, const void * data, uint32_t size) noexcept
{
    WriteDeltaTime(DeltaTime);

    Grow(0x01 + 0x04 + size); // Worst case

    RunningStatus = 0x00;

    Data[Offset++] = statusCode;

    WriteVariableLengthQuantity(size);

    ::memcpy(&Data[Offset], data, size);
    Offset += size;
}

/// <summary>
/// 
/// </summary>
void stream_t::WriteMetaEvent(uint8_t metaType, const void * data, uint32_t size) noexcept
{
    WriteDeltaTime(DeltaTime);

    Grow(0x02 + 0x05 + size); // worst case

    RunningStatus = 0x00;

    Data[Offset++] = midi::StatusCode::MetaData;
    Data[Offset++] = metaType;

    WriteVariableLengthQuantity(size);

    ::memcpy(&Data[Offset], data, size);
    Offset += size;
}

/// <summary>
/// 
/// </summary>
void stream_t::WriteEvent_(uint8_t statusCode, uint8_t param1, uint8_t param2) noexcept
{
    WriteDeltaTime(DeltaTime);

    const uint8_t Status = (uint8_t) (statusCode | ChannelNumber);

    Grow(3);

    switch (statusCode & 0xF0)
    {
        case midi::StatusCode::NoteOff:
        case midi::StatusCode::NoteOn:
        case midi::StatusCode::KeyPressure:
        case midi::StatusCode::ControlChange:
        case midi::StatusCode::PitchBendChange:
        {
            if (RunningStatus != Status)
            {
                RunningStatus = Status;

                Data[Offset++] = Status;
            }

            Data[Offset++] = param1;
            Data[Offset++] = param2;
            break;
        }

        case midi::StatusCode::ProgramChange:
        case midi::StatusCode::ChannelPressure:
        {
            if (RunningStatus != Status)
            {
                RunningStatus = Status;

                Data[Offset++] = Status;
            }

            Data[Offset++] = param1;
            break;
        }

        case 0xF0: // Meta Event
        {
            RunningStatus = 0x00;

            Data[Offset++] = statusCode;
            Data[Offset++] = param1;
            Data[Offset++] = param2;
            break;
        }

        default:
            break;
    }
}

/// <summary>
/// 
/// </summary>
void stream_t::WriteDeltaTime(uint32_t & deltaTime) noexcept
{
    GetDeltaTime(deltaTime);
    WriteVariableLengthQuantity(deltaTime);
    deltaTime = 0;
}

/// <summary>
/// 
/// </summary>
void stream_t::WriteVariableLengthQuantity(uint32_t value) noexcept
{
    uint8_t ByteCount = 0;

    {
        uint32_t v = value;

        do
        {
            v >>= 7;
            ByteCount++;
        }
        while (v);
    }

    Grow(ByteCount);

    uint8_t * p = &Data[Offset];

    {
        uint32_t v = value;
        uint32_t n = ByteCount;

        do
        {
            p[--n] = 0x80u | (v & 0x7Fu);
            v >>= 7;
        }
        while (v);
    }

    p[ByteCount - 1] &= 0x7Fu;

    Offset += ByteCount;
}

/// <summary>
/// 
/// </summary>
void stream_t::Grow(uint32_t bytesNeeded) noexcept
{
    const size_t NewOffset = Offset + bytesNeeded;

    if (NewOffset <= Size)
        return;

    const size_t REALLOC_STEP = 0x8000u;

    while (NewOffset > Size)
        Size += REALLOC_STEP;

    auto p = (uint8_t *) realloc(Data, Size);

    if (p != nullptr)
        Data = p;
}

/// <summary>
/// 
/// </summary>
void stream_t::GetDeltaTime(uint32_t & deltaTime) noexcept
{
    _RunningNotes.Update(*this, deltaTime);

    if (deltaTime != 0)
    {
        for (size_t i = 0; i < _RunningNotes._Count; ++i)
            _RunningNotes._Items[i].Duration -= deltaTime;
    }
}

}
