#include "MIDIProcessor.h"

bool MIDIProcessor::IsSMF(std::vector<uint8_t> const & data)
{
    if (data.size() < 18)
        return SetLastErrorCode(MIDIError::InsufficientData);

    if (::memcmp(&data[0], "MThd", 4) != 0)
        return SetLastErrorCode(MIDIError::SMFBadHeaderChunkType);

    if (data[4] != 0 || data[5] != 0 || data[6] != 0 || data[7] != 6)
        return SetLastErrorCode(MIDIError::SMFBadHeaderChunkSize);

    int Format = (data[8] << 8) | data[9];

    if (Format > 2)
        return SetLastErrorCode(MIDIError::SMFBadHeaderFormat);

    int TrackCount = (data[10] << 8) | data[11];

    if ((TrackCount == 0) || ((Format == 0) && (TrackCount != 1)))
        return SetLastErrorCode(MIDIError::SMFBadHeaderTrackCount);

    if (data[12] == 0 && data[13] == 0)
        return SetLastErrorCode(MIDIError::SMFBadHeaderTimeDivision);

    if (::memcmp(&data[14], "MTrk", 4) != 0)
        return SetLastErrorCode(MIDIError::SMFUnknownChunkType);

    return true;
}

bool MIDIProcessor::ProcessSMF(std::vector<uint8_t> const & data, MIDIContainer & container)
{
    if (data.size() < 18)
        return SetLastErrorCode(MIDIError::InsufficientData);

    if (::memcmp(&data[0], "MThd", 4) != 0)
        return SetLastErrorCode(MIDIError::SMFBadHeaderChunkType);

    if (data[4] != 0 || data[5] != 0 || data[6] != 0 || data[7] != 6)
        return SetLastErrorCode(MIDIError::SMFBadHeaderChunkSize);

    int Format = (data[8] << 8) | data[9];

    if (Format > 2)
        return SetLastErrorCode(MIDIError::SMFBadHeaderFormat);

    int TrackCount = (data[10] << 8) | data[11];

    if ((TrackCount == 0) || ((Format == 0) && (TrackCount != 1)))
        return SetLastErrorCode(MIDIError::SMFBadHeaderTrackCount);

    int TimeDivision = (data[12] << 8) | data[13];

    if ((TimeDivision == 0))
        return SetLastErrorCode(MIDIError::SMFBadHeaderTimeDivision);

    container.Initialize(Format, TimeDivision);

    std::vector<uint8_t>::const_iterator Data = data.begin() + 14;
    std::vector<uint8_t>::const_iterator Tail = data.end();

    for (int i = 0; i < TrackCount; ++i)
    {
        if (Tail - Data < 8)
            return SetLastErrorCode(MIDIError::InsufficientData);

        uint32_t ChunkSize = (uint32_t)((Data[4] << 24) | (Data[5] << 16) | (Data[6] << 8) | Data[7]);

        if (::memcmp(&Data[0], "MTrk", 4) == 0)
        {
            if (Tail - Data < 8 + ChunkSize)
                return SetLastErrorCode(MIDIError::InsufficientData);

            Data += 8;

            std::vector<uint8_t>::const_iterator ChunkTail = Data + ChunkSize;

            if (!ProcessSMFTrack(Data, ChunkTail, container, true))
                return false;

            Data = ChunkTail; // In case no all track data gets used.
        }
        // Skip unknown chunks in the stream.
        else
        {
            if (Tail - Data < 8 + ChunkSize)
                return SetLastErrorCode(MIDIError::InsufficientData);

            Data += 8 + ChunkSize;

            continue;
        }
    }

    return true;
}

bool MIDIProcessor::ProcessSMFTrack(std::vector<uint8_t>::const_iterator & data, std::vector<uint8_t>::const_iterator tail, MIDIContainer & container, bool trackNeedsEndMarker)
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

        if (data == tail)
            return SetLastErrorCode(MIDIError::InsufficientData);

        if (DeltaTime < 0)
            DeltaTime = -DeltaTime; // "Encountered negative delta: " << delta << "; flipping sign."

        Timestamp += DeltaTime;

        uint32_t BytesRead = 0;

        uint8_t StatusCode = *data++;

        if (StatusCode < StatusCodes::NoteOff)
        {
            if (LastStatusCode == 0xFF)
                return SetLastErrorCode(MIDIError::SMFBadFirstMessage);

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

            if (!trackNeedsEndMarker && ((StatusCode & 0xF0) == StatusCodes::PitchBendChange))
                continue;

            if (BytesRead == 0)
            {
                if (data == tail)
                    return SetLastErrorCode(MIDIError::InsufficientData);

                Buffer.resize(3);

                Buffer[0] = *data++;
                BytesRead = 1;
            }

            switch (StatusCode & 0xF0)
            {
                case StatusCodes::ProgramChange:
                case StatusCodes::ChannelAftertouch:
                    break;

                default:
                    if (data == tail)
                        return SetLastErrorCode(MIDIError::InsufficientData);

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
                return SetLastErrorCode(MIDIError::InvalidSysExMessage);

            if (data + Size > tail)
                return SetLastErrorCode(MIDIError::InsufficientData);

            {
                Buffer.resize((size_t)(Size + 1));

                Buffer[0] = StatusCodes::SysEx;

                std::copy(data, data + Size, Buffer.begin() + 1);
                data += Size;

                SysExSize = Size + 1;
                SysExTimestamp = Timestamp;
            }
        }
        else
        if (StatusCode == StatusCodes::SysExEnd)
        {
            if (SysExSize == 0)
                return SetLastErrorCode(MIDIError::InvalidSysExEndMessage);

            // Add the SysEx continuation to the current SysEx message
            int Size = DecodeVariableLengthQuantity(data, tail);

            if (Size < 0)
                return SetLastErrorCode(MIDIError::InvalidSysExMessageContinuation);

            if (data + Size > tail)
                return SetLastErrorCode(MIDIError::InsufficientData);

            {
                Buffer.resize(SysExSize + Size);

                std::copy(data, data + Size, Buffer.begin() + SysExSize);
                data += Size;

                SysExSize += Size;
            }
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
                return SetLastErrorCode(MIDIError::InvalidMetaDataMessage);

            uint8_t MetaDataType = *data++;

            int Size = DecodeVariableLengthQuantity(data, tail);

            if (Size < 0)
                return SetLastErrorCode(MIDIError::InvalidMetaDataMessage);

            if (data + Size > tail)
                return SetLastErrorCode(MIDIError::InsufficientData);

            {
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
        }
        else
        if ((StatusCodes::SysExEnd < StatusCode) && (StatusCode < StatusCodes::MetaData)) //Sequencer specific events, single byte.
        {
            Buffer[0] = StatusCode;
            Track.AddEvent(MIDIEvent(Timestamp, MIDIEvent::Extended, 0, &Buffer[0], 1));
        }
        else
            return SetLastErrorCode(MIDIError::UnknownStatusCode);
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
