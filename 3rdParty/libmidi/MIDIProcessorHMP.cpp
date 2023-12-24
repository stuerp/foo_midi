
/** $VER: MIDIProcessorHMP.cpp (2023.08.14) Human Machine Interfaces MIDI P (http://www.vgmpf.com/Wiki/index.php?title=HMP) **/

#include "MIDIProcessor.h"

bool MIDIProcessor::IsHMP(std::vector<uint8_t> const & data)
{
    if (data.size() < 8)
        return false;

    if (data[0] != 'H' || data[1] != 'M' || data[2] != 'I' || data[3] != 'M' || data[4] != 'I' || data[5] != 'D' || data[6] != 'I' || (data[7] != 'P' && data[7] != 'R'))
        return false;

    return true;
}

bool MIDIProcessor::ProcessHMP(std::vector<uint8_t> const & data, MIDIContainer & container)
{
    bool IsFunky = data[7] == 'R';

    uint8_t track_count_8;
    uint16_t dtx = 0xC0;

    uint32_t Offset = (uint32_t) (IsFunky ? 0x1A : 0x30);

    if (Offset >= data.size())
        return false;

    auto it = data.begin() + (int) Offset;
    auto end = data.end();

    track_count_8 = *it;

    if (IsFunky)
    {
        if (data.size() <= 0x4D)
            return false;

        dtx = (uint16_t) ((data[0x4C] << 16) | data[0x4D]);

        if (dtx == 0) // dtx == 0, will cause division by zero on tempo calculations
            return false;
    }

    container.Initialize(1, dtx);

    {
        MIDITrack Track;

        Track.AddEvent(MIDIEvent(0, MIDIEvent::Extended, 0, DefaultTempoHMP, _countof(DefaultTempoHMP)));
        Track.AddEvent(MIDIEvent(0, MIDIEvent::Extended, 0, MIDIEventEndOfTrack, _countof(MIDIEventEndOfTrack)));

        container.AddTrack(Track);
    }

    uint8_t Data[4] = { };

    if (it == end)
        return false;

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
        return false;

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

        MIDITrack track;

        uint32_t Timestamp = 0;

        std::vector<uint8_t> Temp(3);

        auto TrackDataEnd = it + (int) track_size_32;

        while (it != TrackDataEnd)
        {
            uint32_t delta = DecodeVariableLengthQuantityHMP(it, TrackDataEnd);

            Timestamp += delta;

            if (it == TrackDataEnd)
                return false;

            Temp[0] = *it++;

            if (Temp[0] == 0xFF)
            {
                if (it == TrackDataEnd)
                    return false;

                Temp[1] = *it++;

                int MetadataSize = DecodeVariableLengthQuantity(it, TrackDataEnd);

                if (MetadataSize < 0)
                    return false; /*throw exception_io_data( "Invalid HMP meta message" );*/

                if (TrackDataEnd - it < MetadataSize)
                    return false;

                Temp.resize((size_t) (MetadataSize + 2));
                std::copy(it, it + MetadataSize, Temp.begin() + 2);
                it += MetadataSize;

                track.AddEvent(MIDIEvent(Timestamp, MIDIEvent::Extended, 0, &Temp[0], (size_t) (MetadataSize + 2)));

                if (Temp[1] == 0x2F)
                    break;
            }
            else
            if (Temp[0] >= 0x80 && Temp[0] <= 0xEF)
            {
                unsigned bytes_read = 2;

                switch (Temp[0] & 0xF0)
                {
                    case 0xC0:
                    case 0xD0:
                        bytes_read = 1;
                }

                if ((unsigned long) (TrackDataEnd - it) < bytes_read)
                    return false;

                std::copy(it, it + (int) bytes_read, Temp.begin() + 1);
                it += bytes_read;

                track.AddEvent(MIDIEvent(Timestamp, (MIDIEvent::EventType) ((Temp[0] >> 4) - 8), (uint32_t) (Temp[0] & 0x0F), &Temp[1], bytes_read));
            }
            else return false; /*throw exception_io_data( "Unexpected status code in HMP track" );*/
        }

        Offset = (uint32_t) (IsFunky ? 0 : 4);

        if (end - it < (int) Offset)
            return false;

        it = TrackDataEnd + (int) Offset;

        container.AddTrack(track);
    }

    return true;
}

/// <summary>
/// Decodes a variable length quantity.
/// </summary>
uint32_t MIDIProcessor::DecodeVariableLengthQuantityHMP(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end) noexcept
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

const uint8_t MIDIProcessor::DefaultTempoHMP[5] = { StatusCodes::MetaData, MetaDataTypes::SetTempo, 0x18, 0x80, 0x00 };
