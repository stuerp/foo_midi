
/** $VER: MIDIProcessorHMI.cpp (2026.11.21) Human Machine Interface (http://www.vgmpf.com/Wiki/index.php?title=HMI) */

#include "pch.h"

#include "HMIProcessor.h"

#include <algorithm>
#include <limits>

namespace midi
{

namespace hmi
{
    static constexpr size_t HMIHeaderSize = 0xEC;
    static constexpr uint16_t HMIDivision = 192;
    static constexpr uint16_t HMISourceDivision = 40;
    static constexpr uint8_t HMIEventPrefix = 0xFE;

    static constexpr uint32_t MaximumTrackCount = 4096;

    /// <summary>
    /// Returns true if data points to an HMI sequence.
    /// </summary>
    bool processor_t::IsHMI(bytes_t const & data) noexcept
    {
        const char Id[] = { 'H', 'M', 'I', '-', 'M', 'I', 'D', 'I', 'S', 'O', 'N', 'G', '0', '6', '1', '5', '9', '5' };

        return (data.size() >= _countof(Id)) && (::memcmp(data.data(), Id, _countof(Id)) == 0);
    }

    /// <summary>
    /// Processes the sequence data.
    /// </summary>
    bool processor_t::Process(bytes_t const & data, container_t & container)
    {
        if (!IsHMI(data))
            return false;

        if (data.size() < HMIHeaderSize)
            throw midi::exception("Insufficient data for HMI header");

        container.FileFormat = FileFormat::HMI;

        const uint32_t TrackCount = ReadLE32(data, 0xE4);
        const uint32_t Offsets    = ReadLE32(data, 0xE8);

        if ((TrackCount == 0) || (TrackCount > MaximumTrackCount))
            throw midi::exception("Invalid HMI track count");

        if ((size_t) Offsets > data.size())
            throw midi::exception("Invalid HMI track offset table");

        if ((size_t) TrackCount > (data.size() - (size_t) Offsets) / 4)
            throw midi::exception("Insufficient data for track offsets");

        // Read and validate the track offsets.
        std::vector<uint32_t> TrackOffsets(TrackCount);

        for (size_t i = 0; i < TrackCount; ++i)
        {
            TrackOffsets[i] = ReadLE32(data, (size_t) Offsets + i * 4);

            if ((size_t) TrackOffsets[i] >= data.size())
                throw midi::exception(msc::FormatText("Invalid offset for track %d", i + 1));

            if ((i > 0) && (TrackOffsets[i] <= TrackOffsets[i - 1]))
                throw midi::exception(msc::FormatText("Invalid offset order for track %d", i + 1));
        }

        // HMI uses 60 ticks per quarter note. The output container uses HMIDivision ticks per quarter note. Event times and note durations are rescaled below while DefaultTempo remains a BPM setting.
        container.Initialize(1, HMIDivision);

        if (_Options.DefaultTempo == 0)
            throw midi::exception("Invalid default tempo");

        const uint32_t Tempo = 60'000'000u / _Options.DefaultTempo; // Convert from BPM to µs / quarter note.

        if ((Tempo == 0) || (Tempo > 0x00FFFFFF))
            throw midi::exception("Invalid default tempo");

        // Add the conductor track. Do not add EndOfTrack yet because loop markers can be added while the source tracks are being decoded.
        {
            const uint8_t Data[5] =
            {
                StatusCode::MetaData,
                MetaDataType::SetTempo,
                (uint8_t) (Tempo >> 16),
                (uint8_t) (Tempo >>  8),
                (uint8_t)  Tempo
            };

            track_t Track;

            Track.AddEvent(event_t(0, event_t::Extended, 0, Data, _countof(Data)));

            container.AddTrack(Track);
        }

        uint32_t SequenceEndTime = 0;

        // Process each track.
        bytes_t Temp;

        for (uint32_t i = 0; i < TrackCount; ++i)
        {
            const size_t Offs = (size_t) TrackOffsets[i];
            const size_t End  = (i + 1 < TrackCount) ? (size_t) TrackOffsets[(size_t) i + 1] : data.size();

            if ((End < Offs) || (End - Offs < 13))
                throw midi::exception(msc::FormatText("Insufficient data for track %d", i + 1));

            const size_t Size = End - Offs;

            auto Data = data.begin() + (ptrdiff_t) Offs;
            auto Tail = data.begin() + (ptrdiff_t) End;

            const char Id[] = { 'H', 'M', 'I', '-', 'M', 'I', 'D', 'I', 'T', 'R', 'A', 'C', 'K' };

            if (::memcmp(&Data[0], Id, _countof(Id)) != 0)
                throw midi::exception(msc::FormatText("Invalid data for track %d", i + 1));

            track_t Track;

            uint32_t RunningTime   = 0;
            uint32_t LastTime      = 0;
            uint32_t TimeRemainder = 0;

            uint8_t RunningStatus = 0;

            bool EndOfTrackFound = false;

            if (Size < 0x4B + 4)
                throw midi::exception("Insufficient data for metadata");

            // Convert the track metadata.
            {
                const uint32_t MetaOffset = ReadLE32(Data + 0x4B, Tail);

                if (MetaOffset > 0)
                {
                    if (((size_t) MetaOffset > Size) || (Size - (size_t) MetaOffset < 2))
                        throw midi::exception("Insufficient data for metadata");

                    const uint8_t MetadataType = Data[MetaOffset];

                    size_t MetadataSize = Data[(ptrdiff_t) MetaOffset + 1];

                    if (MetadataSize > Size - (size_t) MetaOffset - 2)
                        throw midi::exception("Insufficient data for metadata");

                    Temp.resize(MetadataSize + 2);

                    Temp[0] = StatusCode::MetaData;
                    Temp[1] = MetaDataType::Text;

                    std::copy(Data + (ptrdiff_t) MetaOffset + 2, Data + (ptrdiff_t) MetaOffset + 2 + (ptrdiff_t) MetadataSize, Temp.begin() + 2);

                    // The metadata type is currently not documented. Known files store fixed-width text here.
                    (void) MetadataType;

                    // Truncate trailing spaces.
                    while ((MetadataSize > 0) && (Temp[MetadataSize + 1] == ' '))
                        --MetadataSize;

                    if (MetadataSize > 0)
                        Track.AddEvent(event_t(0, event_t::Extended, 0, Temp.data(), MetadataSize + 2));
                }
            }

            if (Size < 0x57 + 4)
                throw midi::exception("Insufficient data for HMI events");

            const uint32_t TrackDataOffset = ReadLE32(Data + 0x57, Tail);

            if (((size_t) TrackDataOffset < 0x5B) || ((size_t) TrackDataOffset > Size))
                throw midi::exception("Invalid HMI event offset");

            auto it = Data + (ptrdiff_t) TrackDataOffset;

            Temp.resize(3);

            while (it != Tail)
            {
                const int DeltaTime = DecodeVariableLengthQuantity(it, Tail);

                if (DeltaTime < 0)
                    throw midi::exception("Invalid HMI delta time");

                // Convert this delta from the HMI timebase to the MIDI output timebase.
                const uint64_t ScaledDelta = ((uint64_t)(uint32_t) DeltaTime * HMIDivision) + TimeRemainder;
                const uint64_t MIDIDelta = ScaledDelta / HMISourceDivision;

                TimeRemainder = (uint32_t) (ScaledDelta % HMISourceDivision);

                if (MIDIDelta > std::numeric_limits<uint32_t>::max() - RunningTime)
                    throw midi::exception("HMI timestamp overflow");

                RunningTime += (uint32_t) MIDIDelta;

                if (RunningTime > LastTime)
                    LastTime = RunningTime;

                if (it == Tail)
                    throw midi::exception("Insufficient data for HMI events");

                Temp[0] = *it++;

                if (Temp[0] == StatusCode::MetaData)
                {
                    RunningStatus = 0;

                    if (it == Tail)
                        throw midi::exception("Insufficient data for HMI metadata event");

                    Temp[1] = *it++;

                    const int MetadataSize = DecodeVariableLengthQuantity(it, Tail);

                    if (MetadataSize < 0)
                        throw midi::exception("Invalid HMI metadata event");

                    if (Tail - it < MetadataSize)
                        throw midi::exception("Insufficient data for HMI metadata event");

                    Temp.resize((size_t) MetadataSize + 2);

                    std::copy(it, it + MetadataSize, Temp.begin() + 2);

                    it += MetadataSize;

                    // DefaultTempo controls HMI playback.
                    if (Temp[1] != MetaDataType::SetTempo)
                    {
                        if ((Temp[1] == MetaDataType::EndOfTrack) && (LastTime > RunningTime))
                            RunningTime = LastTime;

                        Track.AddEvent(event_t(RunningTime, event_t::Extended, 0, Temp.data(), (size_t) MetadataSize + 2));
                    }

                    if (Temp[1] == MetaDataType::EndOfTrack)
                    {
                        EndOfTrackFound = true;
                        break;
                    }
                }
                else
                if ((Temp[0] == StatusCode::SysEx) || (Temp[0] == 0xF7))
                {
                    RunningStatus = 0;

                    const int SysExSize = DecodeVariableLengthQuantity(it, Tail);

                    if (SysExSize < 0)
                        throw midi::exception("Invalid HMI SysEx event");

                    if (Tail - it < SysExSize)
                        throw midi::exception("Insufficient data for HMI SysEx event");

                    Temp.resize((size_t) SysExSize + 1);

                    std::copy(it, it + SysExSize, Temp.begin() + 1);

                    it += SysExSize;

                    Track.AddEvent(event_t(RunningTime, event_t::Extended, 0, Temp.data(), (size_t) SysExSize + 1));
                }
                else
                if (Temp[0] == HMIEventPrefix)
                {
                    RunningStatus = 0;

                    DecodeHMIControlEvent(it, Tail, RunningTime, container);
                }
                else
                if (Temp[0] < StatusCode::SysEx)
                {
                    size_t BytesRead = 1;

                    if (IsChannelStatus(Temp[0]))
                    {
                        if (it == Tail)
                            throw midi::exception("Insufficient data for HMI message");

                        Temp[1] = *it++;

                        RunningStatus = Temp[0];
                    }
                    else
                    {
                        if (RunningStatus == 0)
                            throw midi::exception("Invalid shortened HMI event after metadata, SysEx or control event");

                        Temp[1] = Temp[0];
                        Temp[0] = RunningStatus;
                    }

                    const event_t::event_type_t Type = (event_t::event_type_t) ((Temp[0] >> 4) - 8);

                    const uint32_t Channel = (uint32_t) (Temp[0] & 0x0F);

                    if ((Type != event_t::ProgramChange) && (Type != event_t::ChannelPressure))
                    {
                        if (it == Tail)
                            throw midi::exception("Insufficient data for HMI event");

                        Temp[2] = *it++;
                        BytesRead = 2;
                    }

                    Track.AddEvent(event_t(RunningTime, Type, Channel, Temp.data() + 1, BytesRead));

                    // HMI NoteOn events are followed by a duration. Convert the duration from the HMI timebase to the output MIDI timebase and add the corresponding NoteOff event.
                    if (Type == event_t::NoteOn)
                    {
                        const int Duration = DecodeVariableLengthQuantity(it, Tail);

                        if (Duration < 0)
                            throw midi::exception("Invalid HMI note event");

                        const uint64_t ScaledDuration = (uint64_t) (uint32_t) Duration * HMIDivision;

                        // Round note durations to the nearest output tick.
                        const uint64_t MIDIDuration = (ScaledDuration + HMISourceDivision / 2) / HMISourceDivision;

                        if (MIDIDuration > std::numeric_limits<uint32_t>::max() - RunningTime)
                            throw midi::exception("HMI note duration overflow");

                        const uint32_t EndTime = RunningTime + (uint32_t) MIDIDuration;

                        if (EndTime > LastTime)
                            LastTime = EndTime;

                        Temp[2] = 0;

                        Track.AddEvent(event_t(EndTime, event_t::NoteOff, Channel, Temp.data() + 1, BytesRead));
                    }
                }
                else
                    throw midi::exception("Invalid status code");
            }

            if (!EndOfTrackFound)
                Track.AddEvent(event_t(LastTime, event_t::Extended, 0, MIDIEventEndOfTrack, _countof(MIDIEventEndOfTrack)));

            if (LastTime > SequenceEndTime)
                SequenceEndTime = LastTime;

            container.AddTrack(Track);
        }

        // Finish the conductor track only after all source tracks and loop markers have been processed.
        container.AddEventToTrack(0, event_t(SequenceEndTime, event_t::Extended, 0, MIDIEventEndOfTrack, _countof(MIDIEventEndOfTrack)));

        return true;
    }

    /// <summary>
    /// 
    /// </summary>
    void processor_t::AddLoopMarker(container_t & container, uint32_t time, bool isBegin)
    {
        if (isBegin)
            container.AddEventToTrack(0, event_t(time, event_t::Extended, 0, LoopBeginMarker, _countof(LoopBeginMarker)));
        else
            container.AddEventToTrack(0, event_t(time, event_t::Extended, 0, LoopEndMarker, _countof(LoopEndMarker)));
    }

    /// <summary>
    /// 
    /// </summary>
    void processor_t::DecodeHMIControlEvent(iterator_t & data, iterator_t tail, uint32_t tunningTime, container_t & container)
    {
        if (data == tail)
            throw midi::exception("Insufficient data for HMI control event");

        const uint8_t Type = *data++;

        if (Type == 0x10)
        {
            Skip(data, tail, 2, "Insufficient data for HMI control event 0x10");

            if (data == tail)
                throw midi::exception("Insufficient data for HMI control event 0x10");

            const uint8_t Size = *data++;
            Skip(data, tail, (size_t) Size + 4, "Insufficient data for HMI control event 0x10");
        }
        else
        if (Type == 0x12)
        {
            Skip(data, tail, 2, "Insufficient data for HMI control event 0x12");
        }
        else
        if (Type == 0x13)
        {
            Skip(data, tail, 10, "Insufficient data for HMI control event 0x13");
        }
        else
        if (Type == 0x14)
        {
            Skip(data, tail, 2, "Insufficient data for HMI loop begin event");
            AddLoopMarker(container, tunningTime, true);
        }
        else
        if (Type == 0x15)
        {
            Skip(data, tail, 6, "Insufficient data for HMI loop end event");
            AddLoopMarker(container, tunningTime, false);
        }
        else
            throw midi::exception("Invalid HMI control event");
    }
}

}
