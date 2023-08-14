
/** $VER: MIDIProcessorHMI.cpp (2023.08.14) Human Machine Interface (http://www.vgmpf.com/Wiki/index.php?title=HMI) **/

#include "MIDIProcessor.h"

bool MIDIProcessor::IsHMI(std::vector<uint8_t> const & p_file)
{
    if (p_file.size() < 12)
        return false;

    if (p_file[0] != 'H' || p_file[1] != 'M' || p_file[ 2] != 'I' || p_file[ 3] != '-' ||
        p_file[4] != 'M' || p_file[5] != 'I' || p_file[ 6] != 'D' || p_file[ 7] != 'I' ||
        p_file[8] != 'S' || p_file[9] != 'O' || p_file[10] != 'N' || p_file[11] != 'G') return false;

    return true;
}

bool MIDIProcessor::ProcessHMI(std::vector<uint8_t> const & data, MIDIContainer & container)
{
    std::vector<uint8_t> Data;

    auto it = data.begin() + 0xE4;

    uint32_t TrackCount       = (uint32_t) (it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24));
    uint32_t TrackTableOffset = (uint32_t) (it[4] | (it[5] << 8) | (it[6] << 16) | (it[7] << 24));

    if (TrackTableOffset >= data.size() || TrackTableOffset + (size_t) (TrackCount * 4) > data.size())
        return false;

    it = data.begin() + TrackTableOffset;

    std::vector<uint32_t> TrackOffsets;

    TrackOffsets.resize(TrackCount);

    for (size_t i = 0; i < TrackCount; ++i)
    {
        TrackOffsets[i] = (uint32_t) (it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24));
        it += 4;
    }

    container.Initialize(1, 0xC0);

    {
        MIDITrack track;

        track.AddEvent(MIDIEvent(0, MIDIEvent::Extended, 0, DefaultTempoHMP, _countof(DefaultTempoHMP)));
        track.AddEvent(MIDIEvent(0, MIDIEvent::Extended, 0, MIDIEventEndOfTrack, _countof(MIDIEventEndOfTrack)));

        container.AddTrack(track);
    }

    for (uint32_t i = 0; i < TrackCount; ++i)
    {
        uint32_t TrackOffset = TrackOffsets[i];

        uint32_t TrackLength;

        if (i + 1 < TrackCount)
            TrackLength = TrackOffsets[(size_t) (i + 1)] - TrackOffset;
        else
            TrackLength = (uint32_t) data.size() - TrackOffset;

        if (TrackOffset >= data.size() || (size_t) (TrackOffset + TrackLength) > data.size())
            return false;

        auto TrackData    = data.begin() + TrackOffset;
        auto TrackDataEnd = TrackData + TrackLength;

        if (TrackLength < 13)
            return false;

        if (TrackData[0] != 'H' || TrackData[1] != 'M' || TrackData[ 2] != 'I' || TrackData[ 3] != '-' ||
            TrackData[4] != 'M' || TrackData[5] != 'I' || TrackData[ 6] != 'D' || TrackData[ 7] != 'I' ||
            TrackData[8] != 'T' || TrackData[9] != 'R' || TrackData[10] != 'A' || TrackData[11] != 'C' || TrackData[12] != 'K')
            return false;

        MIDITrack Track;

        uint32_t Timestamp = 0;

        uint8_t LastStatusCode = 0xFF;
        uint32_t LastStatusTimestamp = 0;

        if (TrackLength < 0x4B + 4)
            return false;

        uint32_t MetaOffset = (uint32_t) (TrackData[0x4B] | (TrackData[0x4C] << 8) | (TrackData[0x4D] << 16) | (TrackData[0x4E] << 24));

        if (MetaOffset && MetaOffset + 1 < TrackLength)
        {
            Data.resize(2);
            std::copy(TrackData + MetaOffset, TrackData + MetaOffset + 2, Data.begin());

            uint32_t MetadataSize = Data[1];

            if (MetaOffset + 2 + MetadataSize > TrackLength)
                return false;

            Data.resize((size_t) (MetadataSize + 2));
            std::copy(TrackData + MetaOffset + 2, TrackData + MetaOffset + 2 + MetadataSize, Data.begin() + 2);

            while (MetadataSize > 0 && Data[(size_t) (MetadataSize + 1)] == ' ')
                --MetadataSize;

            if (MetadataSize > 0)
            {
                Data[0] = StatusCodes::MetaData;
                Data[1] = MetaDataTypes::Text;

                Track.AddEvent(MIDIEvent(0, MIDIEvent::Extended, 0, &Data[0], MetadataSize + 2));
            }
        }

        if (TrackLength < 0x57 + 4)
            return false;

        uint32_t TrackDataOffset = (uint32_t) (TrackData[0x57] | (TrackData[0x58] << 8) | (TrackData[0x59] << 16) | (TrackData[0x5A] << 24));

        it = TrackData + TrackDataOffset;

        Data.resize(3);

        while (it != TrackDataEnd)
        {
            int delta = DecodeVariableLengthQuantity(it, TrackDataEnd);

            if (delta > 0xFFFF || delta < 0)
            {
                Timestamp = LastStatusTimestamp; /*console::formatter() << "[foo_midi] Large HMI delta detected, shunting.";*/
            }
            else
            {
                Timestamp += delta;

                if (Timestamp > LastStatusTimestamp)
                    LastStatusTimestamp = Timestamp;
            }

            if (it == TrackDataEnd)
                return false;

            Data[0] = *it++;

            if (Data[0] == StatusCodes::MetaData)
            {
                LastStatusCode = 0xFF;

                if (it == TrackDataEnd)
                    return false;

                Data[1] = *it++;

                int MetadataSize = DecodeVariableLengthQuantity(it, TrackDataEnd);

                if (MetadataSize < 0)
                    return false; /*throw exception_io_data( "Invalid HMI meta message" );*/

                if (TrackDataEnd - it < MetadataSize)
                    return false;

                Data.resize((size_t) (MetadataSize + 2));
                std::copy(it, it + MetadataSize, Data.begin() + 2);

                it += MetadataSize;

                if ((Data[1] == MetaDataTypes::EndOfTrack) && (LastStatusTimestamp > Timestamp))
                    Timestamp = LastStatusTimestamp;

                Track.AddEvent(MIDIEvent(Timestamp, MIDIEvent::Extended, 0, &Data[0], (size_t) (MetadataSize + 2)));

                if (Data[1] == MetaDataTypes::EndOfTrack)
                    break;
            }
            else
            if (Data[0] == StatusCodes::SysEx)
            {
                LastStatusCode = 0xFF;

                int SysExSize = DecodeVariableLengthQuantity(it, TrackDataEnd);

                if (SysExSize < 0)
                    return false; /*throw exception_io_data( "Invalid HMI System Exclusive message" );*/

                if (TrackDataEnd - it < SysExSize)
                    return false;

                Data.resize((size_t) (SysExSize + 1));
                std::copy(it, it + SysExSize, Data.begin() + 1);

                it += SysExSize;
                Track.AddEvent(MIDIEvent(Timestamp, MIDIEvent::Extended, 0, &Data[0], (size_t) (SysExSize + 1)));
            }
            else
            if (Data[0] == StatusCodes::ActiveSensing)
            {
                LastStatusCode = 0xFF;

                if (it == TrackDataEnd)
                    return false;

                Data[1] = *it++;

                if (Data[1] == 0x10)
                {
                    if (TrackDataEnd - it < 3)
                        return false;

                    it += 2;
                    Data[2] = *it++;

                    if (TrackDataEnd - it < Data[2] + 4)
                        return false;

                    it += Data[2] + 4;
                }
                else
                if (Data[1] == 0x12)
                {
                    if (TrackDataEnd - it < 2)
                        return false;

                    it += 2;
                }
                else
                if (Data[1] == 0x13)
                {
                    if (TrackDataEnd - it < 10)
                        return false;

                    it += 10;
                }
                else
                if (Data[1] == 0x14)
                {
                    if (TrackDataEnd - it < 2)
                        return false;

                    it += 2;
                    container.AddEventToTrack(0, MIDIEvent(Timestamp, MIDIEvent::Extended, 0, LoopBeginMarker, _countof(LoopBeginMarker)));
                }
                else
                if (Data[1] == 0x15)
                {
                    if (TrackDataEnd - it < 6)
                        return false;

                    it += 6;
                    container.AddEventToTrack(0, MIDIEvent(Timestamp, MIDIEvent::Extended, 0, LoopEndMarker, _countof(LoopEndMarker)));
                }
                else
                    return false; /*throw exception_io_data( "Unexpected HMI meta event" );*/
            }
            else
            if (Data[0] < StatusCodes::SysEx)
            {
                uint32_t BytesRead = 1;

                if (Data[0] >= 0x80)
                {
                    if (it == TrackDataEnd)
                        return false;

                    Data[1] = *it++;
                    LastStatusCode = Data[0];
                }
                else
                {
                    if (LastStatusCode == 0xFF)
                        return false; /*throw exception_io_data( "HMI used shortened event after Meta or SysEx message" );*/

                    Data[1] = Data[0];
                    Data[0] = LastStatusCode;
                }

                MIDIEvent::EventType Type = (MIDIEvent::EventType) ((Data[0] >> 4) - 8);

                uint32_t Channel = (uint32_t) (Data[0] & 0x0F);

                if ((Type != MIDIEvent::ProgramChange) && (Type != MIDIEvent::ChannelAftertouch))
                {
                    if (it == TrackDataEnd)
                        return false;

                    Data[2] = *it++;
                    BytesRead = 2;
                }

                Track.AddEvent(MIDIEvent(Timestamp, Type, Channel, &Data[1], BytesRead));

                if (Type == MIDIEvent::NoteOn)
                {
                    Data[2] = 0x00;

                    int note_length = DecodeVariableLengthQuantity(it, TrackDataEnd);

                    if (note_length < 0)
                        return false; /*throw exception_io_data( "Invalid HMI note message" );*/

                    uint32_t note_end_timestamp = Timestamp + note_length;

                    if (note_end_timestamp > LastStatusTimestamp)
                        LastStatusTimestamp = note_end_timestamp;

                    Track.AddEvent(MIDIEvent(note_end_timestamp, MIDIEvent::NoteOn, Channel, &Data[1], BytesRead));
                }
            }
            else
                return false; /*throw exception_io_data( "Unexpected HMI status code" );*/
        }

        container.AddTrack(Track);
    }

    return true;
}
