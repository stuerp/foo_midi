
/** $VER: MIDIProcessorHMP.cpp (2026.11.21) Human Machine Interfaces MIDI P/R (http://www.vgmpf.com/Wiki/index.php?title=HMP) **/

#include "pch.h"

#include "HMPProcessor.h"

namespace midi
{
    namespace hmp
    {
        constexpr uint32_t HMPTempoUSPerQuarterNote = 1'000'000u; // HMP timing is conventionally 60 BPM.
        constexpr uint32_t MaximumTrackCount = 4096u;

        /// <summary>
        /// <summary>Returns true if data points to an HMP sequence.
        /// </summary>
        bool processor_t::IsHMP(bytes_t const & data) noexcept
        {
            return StartsWith(data, 0, "HMIMIDIP", 8) || StartsWith(data, 0, "HMIMIDIR", 8);
        }

        /// <summary>
        /// Processes an HMI MIDI P/R sequence.
        /// </summary>
        bool processor_t::Process(std::vector<uint8_t> const & data, container_t & container)
        {
            if (!IsHMP(data))
                return false;

            container.FileFormat = FileFormat::HMP;

            const bool IsMIDIR = StartsWith(data, 0, "HMIMIDIR", 8);

            if (!IsMIDIR)
            {
                if (!HasBytes(data, 0, 0x40))
                    throw midi::exception("Truncated HMP header");

                const bool Is013195 = StartsWith(data, 8, "013195", 6);

                const size_t FirstChunkOffset = Is013195 ? 0x388u : 0x308u;

                const uint32_t FileSize   = ReadLE32(data, 0x20);
                const uint32_t ChunkCount = ReadLE32(data, 0x30);
                const uint32_t Division32 = ReadLE32(data, 0x38);

                if (FileSize != 0 && FileSize > data.size())
                    throw midi::exception("Truncated HMP file");

                if (ChunkCount == 0 || ChunkCount > MaximumTrackCount)
                    throw midi::exception("Invalid HMP chunk count");

                if (Division32 == 0 || Division32 > 0x7FFFu)
                    throw midi::exception("Invalid HMP time division");

                if (FirstChunkOffset >= data.size())
                    throw midi::exception("Missing HMP chunk data");

                container.Initialize(1, (uint16_t) Division32);

                // Add the conductor track.
                AddTempoTrack(container, HMPTempoUSPerQuarterNote);

                size_t ChunkOffset = FirstChunkOffset;

                for (uint32_t ChunkIndex = 0; ChunkIndex < ChunkCount; ++ChunkIndex)
                {
                    if (!HasBytes(data, ChunkOffset, 12))
                        throw midi::exception("Truncated HMP chunk header");

                    const uint32_t ChunkLength = ReadLE32(data, ChunkOffset + 4);

                    if (ChunkLength < 12)
                        throw midi::exception("Invalid HMP chunk length");

                    if (!HasBytes(data, ChunkOffset, ChunkLength))
                        throw midi::exception("Truncated HMP chunk");

                    const intptr_t TrackDataOffset = (intptr_t) ChunkOffset + 12;
                    const intptr_t TrackDataLength = (intptr_t) ChunkLength - 12;

                    track_t Track;

                    DecodeTrack(data.begin() + TrackDataOffset, data.begin() + TrackDataOffset + TrackDataLength, Track);

                    container.AddTrack(Track);

                    ChunkOffset += ChunkLength;
                }

                return true;
            }

            // HMIMIDIR uses the compact layout supported by the historical decoder. Its 16-bit fields are little-endian. Keep this parser isolated from the structurally different HMIMIDIP variants.
            if (!HasBytes(data, 0x4C, 2) || !HasBytes(data, 0x1A, 1))
                throw midi::exception("Truncated HMIMIDIR header");

            const uint16_t Division   = ReadLE16(data, 0x4C);
            const uint32_t TrackCount = data[0x1A];

            if (Division == 0 || Division > 0x7FFFu)
                throw midi::exception("Invalid HMIMIDIR time division");

            if (TrackCount == 0 || TrackCount > MaximumTrackCount)
                throw midi::exception("Invalid HMIMIDIR track count");

            container.Initialize(1, Division);

            AddTempoTrack(container, HMPTempoUSPerQuarterNote);

            size_t Offset = 0x1B;

            for (uint32_t TrackIndex = 0; TrackIndex < TrackCount; ++TrackIndex)
            {
                if (!HasBytes(data, Offset, 4))
                    throw midi::exception("Truncated HMIMIDIR track header");

                const uint16_t StoredLength = ReadLE16(data, Offset);

                if (StoredLength < 4)
                    throw midi::exception("Invalid HMIMIDIR track length");

                const intptr_t TrackDataOffset = (intptr_t) Offset + 4;
                const intptr_t TrackDataLength = (intptr_t) StoredLength - 4;

                if (!HasBytes(data, (size_t) TrackDataOffset, (size_t) TrackDataLength))
                    throw midi::exception("Truncated HMIMIDIR track");

                track_t Track;

                DecodeTrack(data.begin() + TrackDataOffset, data.begin() + TrackDataOffset + TrackDataLength, Track);

                container.AddTrack(Track);

                Offset += StoredLength;
            }

            return true;
        }

        /// <summary>
        /// Adds a conductor track.
        /// </summary>
        void processor_t::AddTempoTrack(container_t & container, uint32_t Tempo)
        {
            if (Tempo == 0 || Tempo > 0x00FFFFFFu)
                throw midi::exception("Invalid MIDI tempo");

            const uint8_t TempoEvent[5] =
            {
                StatusCode::MetaData,
                MetaDataType::SetTempo,
                (uint8_t) ((Tempo >> 16) & 0xFFu),
                (uint8_t) ((Tempo >>  8) & 0xFFu),
                (uint8_t) ( Tempo        & 0xFFu)
            };

            track_t Track;

            Track.AddEvent(event_t(0, event_t::Extended, 0, TempoEvent, _countof(TempoEvent)));
            Track.AddEvent(event_t(0, event_t::Extended, 0, MIDIEventEndOfTrack, _countof(MIDIEventEndOfTrack)));

            container.AddTrack(Track);
        }

        /// <summary>
        /// 
        /// </summary>
        void processor_t::DecodeTrack(iterator_t begin, iterator_t end, track_t & track)
        {
            iterator_t it = begin;

            uint64_t RunningTime = 0;
            uint8_t RunningStatus = 0;

            bool HasEndOfTrack = false;

            while (it != end)
            {
                uint32_t DeltaTime = 0;

                if (!DecodeVariableLengthQuantity(it, end, DeltaTime))
                    throw midi::exception("Invalid HMP delta time");

                RunningTime += DeltaTime;

                if (RunningTime > std::numeric_limits<uint32_t>::max())
                    throw midi::exception("HMP timestamp overflow");

                if (it == end)
                    throw midi::exception("Missing HMP event");

                const uint8_t FirstByte = *it++;

                uint8_t Status = FirstByte;

                uint8_t FirstDataByte = 0;

                bool HasFirstDataByte = false;

                if (FirstByte < 0x80u)
                {
                    if (RunningStatus == 0)
                        throw midi::exception("Running status without a previous channel status");

                    Status = RunningStatus;
                    FirstDataByte = FirstByte;

                    HasFirstDataByte = true;
                }
                else
                if (Status < 0xF0u)
                {
                    RunningStatus = Status;
                }
                else
                {
                    RunningStatus = 0;
                }

                const uint32_t EventTime = (uint32_t) RunningTime;

                if (Status == StatusCode::MetaData)
                {
                    if (it == end)
                        throw midi::exception("Truncated meta event");

                    const uint8_t MetaType = *it++;

                    uint32_t PayloadSize = 0;

                    if (!DecodeMIDIVariableLengthQuantity(it, end, PayloadSize))
                        throw midi::exception("Invalid meta event length");

                    if ((ptrdiff_t) (end - it) < PayloadSize)
                        throw midi::exception("Truncated meta event");

                    AddExtendedEvent(track, EventTime, Status, MetaType, it, PayloadSize);

                    it += PayloadSize;

                    if (MetaType == MetaDataType::EndOfTrack)
                    {
                        HasEndOfTrack = true;
                        break;
                    }

                    continue;
                }

                if (Status == 0xF0u || Status == 0xF7u)
                {
                    uint32_t PayloadSize = 0;

                    if (!DecodeMIDIVariableLengthQuantity(it, end, PayloadSize))
                        throw midi::exception("Invalid system-exclusive event length");

                    if ((ptrdiff_t) (end - it) < PayloadSize)
                        throw midi::exception("Truncated system-exclusive event");

                    bytes_t Event((size_t) PayloadSize + 1u);

                    Event[0] = Status;

                    std::copy(it, it + PayloadSize, Event.begin() + 1);
                    it += PayloadSize;

                    track.AddEvent(event_t(EventTime, event_t::Extended, 0, Event.data(), Event.size()));
                    continue;
                }

                if (Status < StatusCode::NoteOff || Status >= 0xF0u)
                    throw midi::exception("Invalid MIDI status code");

                const uint8_t Message = Status & 0xF0u;
                const size_t DataSize = (Message == StatusCode::ProgramChange || Message == StatusCode::ChannelPressure) ? 1u : 2u;

                uint8_t DataBytes[2] = { 0, 0 };

                size_t DataIndex = 0;

                if (HasFirstDataByte)
                    DataBytes[DataIndex++] = FirstDataByte;

                while (DataIndex < DataSize)
                {
                    if (it == end)
                        throw midi::exception("Truncated MIDI channel event");

                    DataBytes[DataIndex++] = *it++;
                }

                track.AddEvent(event_t(EventTime, (event_t::event_type_t)((Status >> 4) - 8), (uint32_t)(Status & 0x0Fu), DataBytes, DataSize));
            }

            if (!HasEndOfTrack)
                track.AddEvent(event_t((uint32_t)(RunningTime), event_t::Extended, 0, MIDIEventEndOfTrack, _countof(MIDIEventEndOfTrack)));
        }

        /// <summary>
        /// 
        /// </summary>
        void processor_t::AddExtendedEvent(track_t & track, uint32_t time, uint8_t status, uint8_t type, iterator_t payload, uint32_t payloadSize)
        {
            bytes_t Event((size_t) payloadSize + 2u);

            Event[0] = status;
            Event[1] = type;

            std::copy(payload, payload + payloadSize, Event.begin() + 2);

            track.AddEvent(event_t(time, event_t::Extended, 0, Event.data(), Event.size()));
        }
    }
}
