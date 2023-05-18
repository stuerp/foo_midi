#include "midi_processor.h"

bool midi_processor::IsSMF(std::vector<uint8_t> const & data)
{
    if (data.size() < 18)
        return false;

    if (::memcmp(&data[0], "MThd", 4) != 0)
        return false; // Bad MIDI header chunk type

    if (data[4] != 0 || data[5] != 0 || data[6] != 0 || data[7] != 6)
        return false; // Bad MIDI header chunk size

    if (data[10] == 0 && data[11] == 0)
        return false; // no tracks

    if (data[12] == 0 && data[13] == 0)
        return false; // dtx == 0, will cause division by zero on tempo calculations

    if (::memcmp(&data[14], "MTrk", 4) != 0)
        return false;

    return true;
}

bool midi_processor::GetTrackCount(std::vector<uint8_t> const & data, size_t & trackCount)
{
    trackCount = 0;

    if (::memcmp(&data[0], "MThd", 4) != 0)
        return false; // Bad MIDI header chunk type

    if (data[4] != 0 || data[5] != 0 || data[6] != 0 || data[7] != 6)
        return false; // Bad MIDI header chunk size

    std::vector<uint8_t>::const_iterator it = data.begin() + 8;

    uint16_t Format = (uint16_t)((it[0] << 8) | it[1]);

    if (Format > 2)
        return false;

    trackCount = (size_t)((it[2] << 8) | it[3]);

    return true;
}

bool midi_processor::ProcessSMF(std::vector<uint8_t> const & data, midi_container & container)
{
    if (data.size() < (4 + 4 + 6))
        return false;

    if (::memcmp(&data[0], "MThd", 4) != 0)
        return false; // Bad MIDI header chunk type

    if (data[4] != 0 || data[5] != 0 || data[6] != 0 || data[7] != 6)
        return false; // Bad MIDI header chunk size

    std::vector<uint8_t>::const_iterator Data = data.begin() + 8;
    std::vector<uint8_t>::const_iterator Tail = data.end();

    uint16_t Format = (uint16_t)((Data[0] << 8) | Data[1]);

    if (Format > 2)
        return false; // Unknown MIDI format

    size_t TrackCount = (size_t)((Data[2] << 8) | Data[3]);
    uint16_t Division = (uint16_t)((Data[4] << 8) | Data[5]);

    if ((TrackCount == 0) || (Division == 0))
        return false;

    container.Initialize(Format, Division);

    Data += 6;

    for (std::size_t i = 0; i < TrackCount; ++i)
    {
        if (Tail - Data < 8)
            return false;

        if (::memcmp(&Data[0], "MTrk", 4) != 0)
            return false;

        uint32_t TrackSize = (uint32_t)((Data[4] << 24) | (Data[5] << 16) | (Data[6] << 8) | Data[7]);

        Data += 8;

        if ((uint32_t)(Tail - Data) < TrackSize)
            return false;

        intptr_t TrackData = Data - data.begin();

        if (!ProcessSMFTrack(Data, Data + TrackSize, container, true))
            return false;

        TrackData += TrackSize;

        if (Data - data.begin() != TrackData)
            Data = data.begin() + TrackData;
    }

    return true;
}

bool midi_processor::ProcessSMFTrack(std::vector<uint8_t>::const_iterator & data, std::vector<uint8_t>::const_iterator tail, midi_container & container, bool trackNeedsEndMarker)
{
    MIDITrack Track;

    uint32_t Timestamp = 0;
    uint8_t LastStatusCode = 0xFF;

    uint32_t SysExSize = 0;
    uint32_t SysExTimestamp = 0;

    std::vector<uint8_t> Buffer;

    Buffer.resize(3);

    for (;;)
    {
        if (!trackNeedsEndMarker && (data == tail))
            break;

        int DeltaTime = DecodeVariableLengthQuantity(data, tail);

        if (!trackNeedsEndMarker && (data == tail))
            break;

        if (DeltaTime < 0)
            DeltaTime = -DeltaTime; // "Encountered negative delta: " << delta << "; flipping sign."

        Timestamp += DeltaTime;

        if (data == tail)
            return false;

        uint32_t BytesRead = 0;

        uint8_t StatusCode = *data++;

        if (StatusCode < 0x80)
        {
            if (LastStatusCode == 0xFF)
                return false; // "First MIDI track event is short encoded."

            Buffer.resize(3);

            Buffer[BytesRead++] = StatusCode;

            StatusCode = LastStatusCode;
        }

        if (StatusCode < StatusCodes::SysEx)
        {
            if (SysExSize > 0)
            {
                Track.AddEvent(MIDIEvent(SysExTimestamp, MIDIEvent::Extended, 0, &Buffer[0], SysExSize));
                SysExSize = 0;
            }

            LastStatusCode = StatusCode;

            if (!trackNeedsEndMarker && ((StatusCode & 0xF0) == 0xE0))
                continue;

            if (BytesRead == 0)
            {
                if (data == tail)
                    return false;

                Buffer.resize(3);

                Buffer[0] = *data++;
                BytesRead = 1;
            }

            switch (StatusCode & 0xF0)
            {
                case 0xC0:
                case 0xD0:
                    break;

                default:
                    if (data == tail)
                        return false;

                    Buffer[BytesRead++] = *data++;
            }

            Track.AddEvent(MIDIEvent(Timestamp, (MIDIEvent::EventType) ((StatusCode >> 4) - 8), (uint32_t)(StatusCode & 0x0F), &Buffer[0], BytesRead));
        }
        else
        if (StatusCode == StatusCodes::SysEx)
        {
            if (SysExSize > 0)
            {
                Track.AddEvent(MIDIEvent(SysExTimestamp, MIDIEvent::Extended, 0, &Buffer[0], SysExSize));
                SysExSize = 0;
            }

            int Size = DecodeVariableLengthQuantity(data, tail);

            if (Size < 0)
                return false; // Invalid System Exclusive message.

            if (tail - data < Size)
                return false;

            Buffer.resize((size_t)(Size + 1));

            Buffer[0] = StatusCodes::SysEx;

            std::copy(data, data + Size, Buffer.begin() + 1);
            data += Size;

            SysExSize = (uint32_t)(Size + 1);
            SysExTimestamp = Timestamp;
        }
        else
        if (StatusCode == StatusCodes::SysExContinuation)
        {
            if (SysExSize == 0)
                return false;

            // Add the SysEx continuation to the current SysEx message
            int Size = DecodeVariableLengthQuantity(data, tail);

            if (Size < 0)
                return false; // Invalid System Exclusive continuation.

            if (tail - data < Size)
                return false;

            Buffer.resize(SysExSize + Size);

            std::copy(data, data + Size, Buffer.begin() + SysExSize);
            data += Size;

            SysExSize += Size;
        }
        else
        if (StatusCode == StatusCodes::MetaData)
        {
            if (SysExSize > 0)
            {
                Track.AddEvent(MIDIEvent(SysExTimestamp, MIDIEvent::Extended, 0, &Buffer[0], SysExSize));
                SysExSize = 0;
            }

            if (data == tail)
                return false;

            uint8_t MetaDataType = *data++;

            int Size = DecodeVariableLengthQuantity(data, tail);

            if (Size < 0)
                return false; // Invalid meta message.

            if (tail - data < Size)
                return false;

            Buffer.resize((size_t)(Size + 2));

            Buffer[0] = StatusCodes::MetaData;
            Buffer[1] = MetaDataType;

            std::copy(data, data + Size, Buffer.begin() + 2);
            data += Size;

            Track.AddEvent(MIDIEvent(Timestamp, MIDIEvent::Extended, 0, &Buffer[0], (size_t)(Size + 2)));

            if (MetaDataType == MetaDataTypes::EndOfTrack) // Mandatory, Marks the end of the track.
            {
                trackNeedsEndMarker = true;
                break;
            }
        }
        else
        if ((StatusCodes::SysExContinuation < StatusCode) && (StatusCode < StatusCodes::MetaData)) //Sequencer specific events, single byte.
        {
            Buffer[0] = StatusCode;
            Track.AddEvent(MIDIEvent(Timestamp, MIDIEvent::Extended, 0, &Buffer[0], 1));
        }
        else
            return false; // Unknown MIDI status code.
    }

    if (!trackNeedsEndMarker)
    {
        Buffer[0] = StatusCodes::MetaData;
        Buffer[1] = MetaDataTypes::EndOfTrack;

        Track.AddEvent(MIDIEvent(Timestamp, MIDIEvent::Extended, 0, &Buffer[0], 2));
    }

    container.AddTrack(Track);

    return true;
}
