
/** $VER: MIDIProcessorMUS.cpp (2025.07.22) Created by Paul Radek for his DMX audio library. Used by id Software for Doom and several other games. (https://moddingwiki.shikadi.net/wiki/MUS_Format) **/

#include "pch.h"

#include "MIDIProcessor.h"

namespace midi
{

bool processor_t::IsMUS(std::vector<uint8_t> const & data) noexcept
{
    if (data.size() < 0x20)
        return false;

    if (data[0] != 'M' || data[1] != 'U' || data[2] != 'S' || data[3] != 0x1A)
        return false;

    uint16_t Length          = (uint16_t) (data[ 4] | (data[ 5] << 8)); // Song length in bytes
    uint16_t Offset          = (uint16_t) (data[ 6] | (data[ 7] << 8)); // Offset to song data
    uint16_t InstrumentCount = (uint16_t) (data[12] | (data[13] << 8)); // No. of primary channels used

    if (Offset >= (16 + (InstrumentCount * 2)) && Offset < (16 + (InstrumentCount * 4)) && (size_t) (Offset + Length) <= data.size())
        return true;

    return false;
}

bool processor_t::ProcessMUS(std::vector<uint8_t> const & data, container_t & container)
{
    uint16_t Length = (uint16_t) (data[ 4] | (data[ 5] << 8)); // Song length in bytes
    uint16_t Offset = (uint16_t) (data[ 6] | (data[ 7] << 8)); // Offset to song data

    if ((size_t) Offset >= data.size() || (size_t) (Offset + Length) > data.size())
        return false;

    container.FileFormat = FileFormat::MUS;

    container.Initialize(0, 0x59);

    {
        track_t Track;

        const uint8_t DefaultTempoMUS[5] = { StatusCode::MetaData, MetaDataType::SetTempo, 0x09, 0xA3, 0x1A };

        Track.AddEvent(event_t(0, event_t::Extended, 0, DefaultTempoMUS, _countof(DefaultTempoMUS)));
        Track.AddEvent(event_t(0, event_t::Extended, 0, MIDIEventEndOfTrack, _countof(MIDIEventEndOfTrack)));

        container.AddTrack(Track);
    }

    track_t Track;

    uint32_t Timestamp = 0;

    uint8_t VelocityLevels[16] = { 0 };

    const uint8_t MusControllers[15] = { 0, 0, 1, 7, 10, 11, 91, 93, 64, 67, 120, 123, 126, 127, 121 };

    auto it = data.begin() + Offset, end = data.begin() + Offset + Length;

    uint8_t Data[4];

    while (it != end)
    {
        Data[0] = *it++;

        if (Data[0] == 0x60)
            break;

        event_t::event_type_t EventType;
        uint32_t EventSize;

        uint32_t Channel = (uint32_t) (Data[0] & 0x0F);

        if (Channel == 0x0F)
            Channel = 9;
        else
        if (Channel >= 9)
            ++Channel;

        switch (Data[0] & 0x70)
        {
            // Release Note
            case 0x00:
                EventType = event_t::NoteOff;

                if (it == end)
                    return false;

                Data[1] = *it++;
                Data[2] = 0;
                EventSize = 2;
                break;

            // PLay Note
            case 0x10:
                EventType = event_t::NoteOn;

                if (it == end)
                    return false;

                Data[1] = *it++;

                if (Data[1] & 0x80)
                {
                    if (it == end)
                        return false;

                    Data[2] = *it++;
                    VelocityLevels[Channel] = Data[2];
                    Data[1] &= 0x7F;
                }
                else
                    Data[2] = VelocityLevels[Channel];

                EventSize = 2;
                break;

            // Pitch Bend
            case 0x20:
                EventType = event_t::PitchBendChange;

                if (it == end)
                    return false;

                Data[1] = *it++;
                Data[2] = (uint8_t) (Data[1] >> 1);
                Data[1] <<= 7;
                EventSize = 2;
                break;

            // System Event
            case 0x30:
                EventType = event_t::ControlChange;

                if (it == end)
                    return false;

                Data[1] = *it++;

                if (Data[1] >= 10 && Data[1] <= 14)
                {
                    Data[1] = MusControllers[Data[1]];
                    Data[2] = 1;
                    EventSize = 2;
                }
                else
                    return false; /*throw exception_io_data( "Unhandled MUS system event" );*/
                break;

            // Controller
            case 0x40:
                if (it == end)
                    return false;

                Data[1] = *it++;

                if (Data[1])
                {
                    if (Data[1] < 10)
                    {
                        EventType = event_t::ControlChange;

                        Data[1] = MusControllers[Data[1]];

                        if (it == end)
                            return false;

                        Data[2] = *it++;
                        EventSize = 2;
                    }
                    else
                        return false; /*throw exception_io_data( "Invalid MUS controller change event" );*/
                }
                else
                {
                    EventType = event_t::ProgramChange;

                    if (it == end)
                        return false;

                    Data[1] = *it++;
                    EventSize = 1;
                }
                break;

            // End of Measure
            case 0x50:

            // Finish
            case 0x60:

            // Unused
            case 0x70:

            default:
                return false; /*throw exception_io_data( "Invalid MUS status code" );*/
        }

        Track.AddEvent(event_t(Timestamp, EventType, Channel, Data + 1, EventSize));

        if (Data[0] & 0x80)
        {
            int Delta = DecodeVariableLengthQuantity(it, end);

            if (Delta < 0)
                return false; /*throw exception_io_data( "Invalid MUS delta" );*/

            Timestamp += Delta;
        }
    }

    Track.AddEvent(event_t(Timestamp, event_t::Extended, 0, MIDIEventEndOfTrack, _countof(MIDIEventEndOfTrack)));

    container.AddTrack(Track);

    return true;
}

}
