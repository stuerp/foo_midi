
/** $VER: MIDIProcessorSMF.cpp (2023.11.01) Standard MIDI File **/

#include "MIDIProcessor.h"

  const uint8_t SysExUseForRhythmPartCh16[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x1F, 0x15, 0x02, 0x0A, 0xF7 }; // Use channel 16 for rhythm.

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
    SetLastErrorCode(MIDIError::None);

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

    container.Initialize((uint32_t) Format, (uint32_t) TimeDivision);

    std::vector<uint8_t>::const_iterator Data = data.begin() + 14;
    std::vector<uint8_t>::const_iterator Tail = data.end();

    for (int i = 0; i < TrackCount; ++i)
    {
        if (Tail - Data < 8)
            return SetLastErrorCode(MIDIError::InsufficientData);

        uint32_t ChunkSize = (uint32_t)((Data[4] << 24) | (Data[5] << 16) | (Data[6] << 8) | Data[7]);

        if (::memcmp(&Data[0], "MTrk", 4) == 0)
        {
            if (Tail - Data < (ptrdiff_t) (8 + ChunkSize))
                return SetLastErrorCode(MIDIError::InsufficientData);

            Data += 8;

            std::vector<uint8_t>::const_iterator ChunkTail = Data + (int) ChunkSize;

            if (!ProcessSMFTrack(Data, ChunkTail, container, true))
                return false;

            Data = ChunkTail; // In case no all track data gets used.
        }
        // Skip unknown chunks in the stream.
        else
        {
            if (Tail - Data < (ptrdiff_t) (8 + ChunkSize))
                return SetLastErrorCode(MIDIError::InsufficientData);

            Data += (int64_t)(8) + ChunkSize;

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

    std::vector<uint8_t> Temp(3);

    bool DetectedPercussionText = false;

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

            Temp.resize(3);

            Temp[BytesRead++] = StatusCode;

            StatusCode = LastStatusCode;
        }

        if (StatusCode < StatusCodes::SysEx)
        {
            if (SysExSize > 0)
            {
                Track.AddEvent(MIDIEvent(SysExTimestamp, MIDIEvent::Extended, 0, Temp.data(), SysExSize));
                SysExSize = 0;
            }

            LastStatusCode = StatusCode;

            if (!trackNeedsEndMarker && ((StatusCode & 0xF0) == StatusCodes::PitchBendChange))
                continue;

            if (BytesRead == 0)
            {
                if (data == tail)
                    return SetLastErrorCode(MIDIError::InsufficientData);

                Temp.resize(3);

                Temp[0] = *data++;
                BytesRead = 1;
            }

            switch (StatusCode & 0xF0)
            {
                case StatusCodes::ProgramChange:
                case StatusCodes::ChannelPressureAftertouch:
                    break;

                default:
                    if (data == tail)
                        return SetLastErrorCode(MIDIError::InsufficientData);

                    Temp[BytesRead++] = *data++;
            }

            uint32_t ChannelNumber = (uint32_t) (StatusCode & 0x0F);

            // Assign percussion to channel 16 if it's first message was preceded with meta data containing the word "drum".
            {

                if ((ChannelNumber == 0x0F) && DetectedPercussionText)
                {
                    Track.AddEvent(MIDIEvent(0, MIDIEvent::Extended, 0, SysExUseForRhythmPartCh16, _countof(SysExUseForRhythmPartCh16)));

                    container.SetExtraPercussionChannel(ChannelNumber);

                    DetectedPercussionText = false;
                }
            }

            Track.AddEvent(MIDIEvent(Timestamp, (MIDIEvent::EventType) ((StatusCode >> 4) - 8), ChannelNumber, Temp.data(), BytesRead));
        }
        else
        if (StatusCode == StatusCodes::SysEx)
        {
            if (SysExSize > 0)
            {
                Track.AddEvent(MIDIEvent(SysExTimestamp, MIDIEvent::Extended, 0, Temp.data(), SysExSize));
                SysExSize = 0;
            }

            int Size = DecodeVariableLengthQuantity(data, tail);

            if (Size < 0)
                return SetLastErrorCode(MIDIError::InvalidSysExMessage);

            if (Size > tail - data)
                return SetLastErrorCode(MIDIError::InsufficientData);

            {
                Temp.resize((size_t) (Size + 1));

                Temp[0] = StatusCodes::SysEx;

                std::copy(data, data + Size, Temp.begin() + 1);
                data += Size;

                SysExSize = (uint32_t) (Size + 1);
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

            if (Size > tail - data)
                return SetLastErrorCode(MIDIError::InsufficientData);

            {
                Temp.resize((size_t) SysExSize + Size);

                std::copy(data, data + Size, Temp.begin() + (int) SysExSize);
                data += Size;

                SysExSize += Size;
            }
        }
        else
        if (StatusCode == StatusCodes::MetaData)
        {
            if (SysExSize > 0)
            {
                Track.AddEvent(MIDIEvent(SysExTimestamp, MIDIEvent::Extended, 0, Temp.data(), SysExSize));
                SysExSize = 0;
            }

            if (data == tail)
                return SetLastErrorCode(MIDIError::InvalidMetaDataMessage);

            uint8_t MetaDataType = *data++;

            if (MetaDataType > MetaDataTypes::SequencerSpecific)
                return SetLastErrorCode(MIDIError::InvalidMetaDataMessage);

            int Size = DecodeVariableLengthQuantity(data, tail);

            if (Size < 0)
                return SetLastErrorCode(MIDIError::InvalidMetaDataMessage);

            if (Size > tail - data)
                return SetLastErrorCode(MIDIError::InsufficientData);

            // Remember when the track or instrument name contains the word "drum". We'll need it later.
            if ((MetaDataType == MetaDataTypes::Text) || (MetaDataType == MetaDataTypes::TrackName) || (MetaDataType == MetaDataTypes::InstrumentName))
            {
                const char * p = (const char *) &data[0];

                for (int n = Size; n > 3; --n, p++)
                {
                    if (::_strnicmp(p, "drum", 4) == 0)
                    {
                        DetectedPercussionText = true;
                        break;
                    }
                }
            }

            {
                Temp.resize((size_t)(Size + 2));

                Temp[0] = StatusCodes::MetaData;
                Temp[1] = MetaDataType;

                std::copy(data, data + Size, Temp.begin() + 2);
                data += Size;

                Track.AddEvent(MIDIEvent(Timestamp, MIDIEvent::Extended, 0, Temp.data(), (size_t) (Size + 2)));

                if (MetaDataType == MetaDataTypes::EndOfTrack) // Mandatory, Marks the end of the track.
                {
                    trackNeedsEndMarker = true;
                    break;
                }
            }
        }
        else
        if ((StatusCodes::SysExEnd < StatusCode) && (StatusCode < StatusCodes::MetaData)) // Sequencer specific events, single byte.
        {
            Temp[0] = StatusCode;

            Track.AddEvent(MIDIEvent(Timestamp, MIDIEvent::Extended, 0, Temp.data(), 1));
        }
        else
            return SetLastErrorCode(MIDIError::UnknownStatusCode);
    }

    if (!trackNeedsEndMarker)
    {
        Temp[0] = StatusCodes::MetaData;
        Temp[1] = MetaDataTypes::EndOfTrack;

        Track.AddEvent(MIDIEvent(Timestamp, MIDIEvent::Extended, 0, Temp.data(), 2));
    }

    container.AddTrack(Track);

    return true;
}
