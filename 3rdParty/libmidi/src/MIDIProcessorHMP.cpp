
/** $VER: MIDIProcessorHMP.cpp (2025.07.22) Human Machine Interfaces MIDI P/R (http://www.vgmpf.com/Wiki/index.php?title=HMP) **/

#include "pch.h"

#include "MIDIProcessor.h"
#include "Exception.h"

namespace midi
{

/// <summary>
/// Returns true if data points to an HMP sequence.
/// </summary>
bool processor_t::IsHMP(std::vector<uint8_t> const & data) noexcept
{
    if (data.size() < 8)
        return false;

    const char Id[] = { 'H', 'M', 'I', 'M', 'I', 'D', 'I' };

    return ((::memcmp(data.data(), Id, _countof(Id)) == 0) && (data[7] == 'P' || data[7] == 'R'));
}

/// <summary>
/// Processes the sequence data.
/// </summary>
bool processor_t::ProcessHMP(std::vector<uint8_t> const & data, container_t & container)
{
    container.FileFormat = FileFormat::HMP;

    const bool IsFunky = (data[7] == 'R');

    uint32_t Offset = (uint32_t) (IsFunky ? 0x1A : 0x30);

    if ((Offset == 0) || (Offset >= data.size()))
        throw midi::exception("Insufficient data");

    auto it = data.begin() + (int) Offset;
    auto end = data.end();

    uint8_t track_count_8 = *it;

    {
        uint16_t TimeDivision = 0xC0;

        if (IsFunky)
        {
            if (data.size() <= 0x4D)
                throw midi::exception("Insufficient data");

            TimeDivision = (uint16_t) ((data[0x4C] << 16) | data[0x4D]);

            if (TimeDivision == 0) // Will cause division by zero on tempo calculations.
                throw midi::exception("Invalid time division");
        }

        container.Initialize(1, TimeDivision);
    }

    // Add a conductor track.
    {
        track_t Track;

        uint8_t Data[] = { StatusCode::MetaData, MetaDataType::SetTempo, 0, 0, 0 };

        {
            uint32_t us = (uint32_t) (60 * 1000 * 1000) / _Options._DefaultTempo; // Convert from bpm to µs / quarter note.

            Data[4] = us & 0x7F; us >>= 7;

            if (us != 0) { Data[3] = 0x80 | (us & 0x7F); us >>= 7; }
            if (us != 0) { Data[2] = us & 0x7F; }
        }

        Track.AddEvent(event_t(0, event_t::Extended, 0, Data, _countof(Data)));
        Track.AddEvent(event_t(0, event_t::Extended, 0, MIDIEventEndOfTrack, _countof(MIDIEventEndOfTrack)));

        container.AddTrack(Track);
    }

    uint8_t Data[4] = { };

    Data[0] = *it++;

    while (it != end)
    {
        if (Data[0] != 0xFF)
        {
            Data[0] = *it++;
            continue;
        }

        if (it == end)
            break;

        Data[1] = *it++;

        if (Data[1] != 0x2F)
        {
            Data[0] = Data[1];
            continue;
        }
        break;
    }

    Offset = (uint32_t) (IsFunky ? 3 : 5);

    if ((unsigned long) (end - it) < Offset)
        throw midi::exception("Insufficient data");

    it += (int) Offset;

    uint32_t TrackCount = track_count_8;

    for (uint32_t i = 1; i < TrackCount; ++i)
    {
        uint16_t track_size_16;
        uint32_t track_size_32;

        if (IsFunky)
        {
            if (end - it < 4)
                break;

            track_size_16 = (uint16_t) (it[0] | (it[1] << 8));
            it += 2;

            track_size_32 = (uint32_t) (track_size_16 - 4);

            if ((unsigned long) (end - it) < track_size_32 + 2)
                break;

            it += 2;
        }
        else
        {
            if (end - it < 8)
                break;

            track_size_32 = (uint32_t) (it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24));
            it += 4;

            track_size_32 -= 12;

            if ((unsigned long) (end - it) < track_size_32 + 8)
                break;

            it += 4;
        }

        track_t track;

        uint32_t RunningTime = 0;

        {
            std::vector<uint8_t> Temp(3);

            auto TrackDataEnd = it + (int) track_size_32;

            while (it != TrackDataEnd)
            {
                uint32_t DeltaTime = DecodeVariableLengthQuantityHMP(it, TrackDataEnd);

                RunningTime += DeltaTime;

                if (it == TrackDataEnd)
                    throw midi::exception("Insufficient data");

                Temp[0] = *it++;

                if (Temp[0] == 0xFF)
                {
                    if (it == TrackDataEnd)
                        throw midi::exception("Insufficient data");

                    Temp[1] = *it++;

                    int MetadataSize = DecodeVariableLengthQuantity(it, TrackDataEnd);

                    if (MetadataSize < 0)
                        throw midi::exception("Invalid meta data event");

                    if (TrackDataEnd - it < MetadataSize)
                        throw midi::exception("Insufficient data");

                    Temp.resize((size_t) (MetadataSize + 2));
                    std::copy(it, it + MetadataSize, Temp.begin() + 2);
                    it += MetadataSize;

                    track.AddEvent(event_t(RunningTime, event_t::Extended, 0, &Temp[0], (size_t) (MetadataSize + 2)));

                    if (Temp[1] == 0x2F)
                        break;
                }
                else
                if (Temp[0] >= StatusCode::NoteOff && Temp[0] < StatusCode::SysEx)
                {
                    int BytesRead = 2;

                    switch (Temp[0] & 0xF0)
                    {
                        case StatusCode::ProgramChange:
                        case StatusCode::ChannelPressure:
                            BytesRead = 1;
                    }

                    if (TrackDataEnd - it < BytesRead)
                        throw midi::exception("Insufficient data");

                    std::copy(it, it + BytesRead, Temp.begin() + 1);
                    it += BytesRead;

                    track.AddEvent(event_t(RunningTime, (event_t::event_type_t) ((Temp[0] >> 4) - 8), (uint32_t) (Temp[0] & 0x0F), &Temp[1], (size_t) BytesRead));
                }
                else
                    throw midi::exception("Invalid status code");
            }

            Offset = (uint32_t) (IsFunky ? 0 : 4);

            if (end - it < (int) Offset)
                throw midi::exception("Insufficient data");

            it = TrackDataEnd + (int) Offset;
        }

        container.AddTrack(track);
    }

    return true;
}

/// <summary>
/// Decodes a variable length quantity.
/// </summary>
uint32_t processor_t::DecodeVariableLengthQuantityHMP(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end) noexcept
{
    uint32_t Quantity = 0;

    uint32_t Shift = 0;
    uint8_t Byte;

    do
    {
        if (it == end)
            return 0;

        Byte = *it++;
        Quantity = Quantity + ((Byte & 0x7F) << Shift);
        Shift += 7;
    }
    while (!(Byte & 0x80));

    return Quantity;
}

}
