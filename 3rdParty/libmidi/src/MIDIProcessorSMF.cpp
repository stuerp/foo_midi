
/** $VER: MIDIProcessorSMF.cpp (2026.04.10) Standard MIDI File **/

#include "pch.h"

#include "MIDIProcessor.h"

#include "MIDI.h"
#include "MIDIContainer.h"

#include "Exception.h"

#include <Encoding.h>

namespace midi
{

/// <summary>
/// Returns true if the data contains an SMF file.
/// </summary>
bool processor_t::IsSMF(std::vector<uint8_t> const & data) noexcept
{
    if (data.size() < 18)
        return false;

    if (::memcmp(data.data(), "MThd", 4) != 0)
        return false;

    if (data[4] != 0 || data[5] != 0 || data[6] != 0 || data[7] != 6)
        return false;

    int Format = (data[8] << 8) | data[9];

    if (Format > 2)
        return false;

    int TrackCount = (data[10] << 8) | data[11];

    if ((TrackCount == 0) || ((Format == 0) && (TrackCount != 1)))
        return false;

    if (data[12] == 0 && data[13] == 0)
        return false;

    if (::memcmp(&data[14], "MTrk", 4) != 0)
        return false;

    return true;
}

/// <summary>
/// Processes the data as an SMF file and returns an intialized container.
/// </summary>
bool processor_t::ProcessSMF(std::vector<uint8_t> const & data, container_t & container)
{
    container.FileFormat = FileFormat::SMF;

    if (data.size() < 18)
        throw midi::exception("Insufficient SMF data");

    if (::memcmp(&data[0], "MThd", 4) != 0)
        throw midi::exception("Invalid SMF header chunk type");

    if (data[4] != 0 || data[5] != 0 || data[6] != 0 || data[7] != 6)
        throw midi::exception("Invalid SMF header chunk size");

    const int Format = (data[8] << 8) | data[9];

    if (Format > 2)
        throw midi::exception(msc::FormatText("Unrecognized MIDI format: %d", Format));

    const size_t TrackCount = (size_t) ((data[10] << 8) | data[11]);

    if ((TrackCount == 0) || ((Format == 0) && (TrackCount != 1)))
        throw midi::exception("Invalid track count");

    const int TimeDivision = (data[12] << 8) | data[13];

    if ((TimeDivision == 0))
        throw midi::exception("Invalid time division");

    container.Initialize((uint32_t) Format, (uint32_t) TimeDivision);

    const auto Tail = data.end();

    auto Data = data.begin() + 14;

    for (size_t i = 0; i < TrackCount; ++i)
    {
        if (Tail - Data < 8)
            throw midi::exception("Insufficient SMF data");

        const ptrdiff_t ChunkSize = (ptrdiff_t)((Data[4] << 24) | (Data[5] << 16) | (Data[6] << 8) | Data[7]);

        if (Tail - Data < (ptrdiff_t) (8 + ChunkSize))
            throw midi::exception("Insufficient SMF data");

        if (::memcmp(&Data[0], "MTrk", 4) == 0)
        {
            Data += 8;

            const auto ChunkTail = Data + ChunkSize;

            if (!ProcessSMFTrack(Data, ChunkTail, container))
                return false;

            Data = ChunkTail; // In case not all track data gets used.
        }
        // Skip unknown chunks in the stream.
        else
        {
            Data += (ptrdiff_t) (8 + ChunkSize);

            continue;
        }
    }

    return true;
}

/// <summary>
/// Processes an SMF track.
/// </summary>
bool processor_t::ProcessSMFTrack(std::vector<uint8_t>::const_iterator & data, std::vector<uint8_t>::const_iterator tail, container_t & container)
{
    track_t Track;

    uint32_t RunningTime = 0;
    uint8_t RunningStatus = 0xFF;

    uint32_t SysExSize = 0;
    uint32_t SysExTime = 0;

    std::vector<uint8_t> Temp(3);

    bool FoundEndOfTrack = false;
    bool DetectedPercussionText = false;

    for (;;)
    {
        // Workaround for invalid SMF files that have tracks without an End of Track message.
        if (!_Options._IsEndOfTrackRequired && (data == tail))
            break;

        int DeltaTime = DecodeVariableLengthQuantity(data, tail);

        if (data == tail)
            throw midi::exception("Insufficient SMF data");

        if (DeltaTime < 0)
            DeltaTime = -DeltaTime; // "Encountered negative delta: " << delta << "; flipping sign."

        RunningTime += DeltaTime;

        uint8_t StatusCode = *data++;
        uint32_t BytesRead = 0;

        // Is it a data byte?
        if (StatusCode < StatusCode::NoteOff)
        {
            // Flush any pending SysEx.
            if (SysExSize > 0)
            {
                Track.AddEvent(event_t(SysExTime, event_t::Extended, 0, Temp.data(), SysExSize));
                SysExSize = 0;
            }

            if (RunningStatus == 0xFF)
                throw midi::exception("Invalid first status code");

            Temp.resize(3);

            Temp[BytesRead++] = StatusCode;

            StatusCode = RunningStatus;
        }

        // Is it a Voice Category message?
        if (StatusCode < StatusCode::SysEx)
        {
            // Flush any pending SysEx.
            if (SysExSize > 0)
            {
                Track.AddEvent(event_t(SysExTime, event_t::Extended, 0, Temp.data(), SysExSize));
                SysExSize = 0;
            }

            RunningStatus = StatusCode; // Set the running status

            if (BytesRead == 0)
            {
                if (data == tail)
                    throw midi::exception("Insufficient SMF data");

                Temp.resize(3);

                Temp[BytesRead++] = *data++;
            }

            switch (StatusCode & 0xF0)
            {
                case StatusCode::ProgramChange:
                case StatusCode::ChannelPressure:
                    break;

                default:
                {
                    if (data == tail)
                        throw midi::exception("Insufficient SMF data");

                    Temp[BytesRead++] = *data++;
                }
            }

            const uint32_t ChannelNumber = (uint32_t) (StatusCode & 0x0F);

            // Assign percussion to channel 16 if it's first message was preceeded with meta data containing the word "drum".
            if ((ChannelNumber == 0x0F) && DetectedPercussionText)
            {
                const uint8_t SysExUseForRhythmPartCh16[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x1F, 0x15, 0x02, 0x0A, 0xF7 }; // Use channel 16 for rhythm.

                Track.AddEvent(event_t(0, event_t::Extended, 0, SysExUseForRhythmPartCh16, _countof(SysExUseForRhythmPartCh16)));

                container.SetExtraPercussionChannel(ChannelNumber);

                DetectedPercussionText = false;
            }

            Track.AddEvent(event_t(RunningTime, (event_t::event_type_t) ((StatusCode >> 4) - 8), ChannelNumber, Temp.data(), BytesRead));
        }
        else
        {
            // Is it a SysEx message?
            if (StatusCode == StatusCode::SysEx)
            {
                // Flush any pending SysEx.
                if (SysExSize > 0)
                {
                    Track.AddEvent(event_t(SysExTime, event_t::Extended, 0, Temp.data(), SysExSize));
                    SysExSize = 0;
                }

                const int Size = DecodeVariableLengthQuantity(data, tail);

                if (Size < 0)
                    throw midi::exception("Invalid System Exclusive event");

                if (Size > tail - data)
                    throw midi::exception("Insufficient data for System Exclusive event");

                {
                    Temp.resize((size_t) (Size + 1));

                    Temp[0] = StatusCode::SysEx;

                    std::copy(data, data + Size, Temp.begin() + 1);
                    data += Size;

                    SysExSize = (uint32_t) (Size + 1);
                    SysExTime = RunningTime;
                }
            }
            else
            if (StatusCode == StatusCode::SysExEnd)
            {
                if (SysExSize == 0)
                    throw midi::exception("Invalid System Exclusive End event");

                // Add the SysEx continuation to the current SysEx message
                const int Size = DecodeVariableLengthQuantity(data, tail);

                if (Size < 0)
                    throw midi::exception("Invalid System Exclusive event");

                if (Size > tail - data)
                    throw midi::exception("Insufficient data for System Exclusive event continuation");

                {
                    Temp.resize((size_t) SysExSize + Size);

                    std::copy(data, data + Size, Temp.begin() + (ptrdiff_t) SysExSize);
                    data += Size;

                    SysExSize += Size;
                }
            }
            else
            if (StatusCode == StatusCode::MetaData)
            {
                // Flush any pending SysEx.
                if (SysExSize > 0)
                {
                    Track.AddEvent(event_t(SysExTime, event_t::Extended, 0, Temp.data(), SysExSize));
                    SysExSize = 0;
                }

                if (data == tail)
                    throw midi::exception("Insufficient data for meta data event");

                const uint8_t MetaDataType = *data++;

                if (MetaDataType > MetaDataType::SequencerSpecific)
                    throw midi::exception("Invalid meta data type");

                int Size = DecodeVariableLengthQuantity(data, tail);

                if (Size < 0)
                    throw midi::exception("Invalid meta data event");

                if (Size > tail - data)
                    throw midi::exception("Insufficient data for meta data event");

                // Remember when the track or instrument name contains the word "drum". We'll need it later.
                if (_Options._DetectExtraPercussionChannel && ((MetaDataType == MetaDataType::Text) || (MetaDataType == MetaDataType::TrackName) || (MetaDataType == MetaDataType::InstrumentName)))
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

                    Temp[0] = StatusCode::MetaData;
                    Temp[1] = MetaDataType;

                    std::copy(data, data + Size, Temp.begin() + 2);
                    data += Size;

                    if ((MetaDataType != MetaDataType::MIDIPort) || ((MetaDataType == MetaDataType::MIDIPort) && Track.IsPortSet()))
                        Track.AddEvent(event_t(RunningTime, event_t::Extended, 0, Temp.data(), (size_t) (Size + 2)));
                    else
                        Track.AddEventToStart(event_t(0, event_t::Extended, 0, Temp.data(), (size_t) (Size + 2)));
                }

                if (MetaDataType == MetaDataType::EndOfTrack) // Mandatory, Marks the end of the track.
                {
                    FoundEndOfTrack = true;
                    break;
                }
            }
            else

            // Is it a RealTime Category message?
            if ((StatusCode::SysExEnd < StatusCode) && (StatusCode < StatusCode::MetaData)) // Sequencer specific events, single byte.
            {
                Temp[0] = StatusCode;

                Track.AddEvent(event_t(RunningTime, event_t::Extended, 0, Temp.data(), 1));
            }
            else
                throw midi::exception("Invalid status code");
        }
    }

    if (!FoundEndOfTrack)
    {
        const uint8_t EventData[] = { StatusCode::MetaData, MetaDataType::EndOfTrack };

        Track.AddEvent(event_t(RunningTime, event_t::Extended, 0, EventData, _countof(EventData)));
    }

    container.AddTrack(Track);

    return true;
}

}
