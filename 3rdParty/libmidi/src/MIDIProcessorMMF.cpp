
/** $VER: MIDIProcessorMMF.cpp (2025.07.22) Mobile Music File / Synthetic-music Mobile Application Format (https://docs.fileformat.com/audio/mmf/) (SMAF) **/

#include "pch.h"

#include "MIDIProcessor.h"
#include "Exception.h"

#include <span>

#include "MMF.h"

namespace midi
{

template <class T> inline static uint32_t toInt32LE(T data)
{
    return static_cast<uint32_t>(data[0] << 24) | static_cast<uint32_t>(data[1] << 16) | static_cast<uint32_t>(data[2] << 8) | static_cast<uint32_t>(data[3]);
}

static void ProcessMetadata(const std::span<const uint8_t> & data, state_t & state, container_t & container);
static void ProcessOPDA(const std::span<const uint8_t> & data, state_t & state, container_t & container);
static void ProcessMTR(const std::span<const uint8_t> & data, state_t & state, container_t & container);
static void ProcessHPSTrack(const std::span<const uint8_t> & data, state_t & state, container_t & container);

static uint32_t GetHPSValue(std::span<const uint8_t>::iterator data) noexcept;
static uint32_t GetHPSValueEx(std::span<const uint8_t>::iterator & data) noexcept;

static std::string GetEncodingDescription(uint8_t encoding) noexcept;
/// <summary>
/// Returns true if the byte vector contains MMF data.
/// </summary>
bool processor_t::IsMMF(std::vector<uint8_t> const & data) noexcept
{
    if (data.size() < 8)
        return false;

    if (::memcmp(data.data(), "MMMD", 4) != 0)
        return false;

    const uint32_t Size = toInt32LE(data.data() + 4);

    if (data.size() < (size_t) Size + 8)
        return false;

    return true;
};

/// <summary>
/// Processes a byte vector with MMF data.
/// </summary>
bool processor_t::ProcessMMF(std::vector<uint8_t> const & data, container_t & container)
{
    container.FileFormat = FileFormat::MMF;

    if (data.size() < 8)
        throw midi::exception("Insufficient SMAF data");

    const uint32_t Size = toInt32LE(data.data() + 4);

    if (data.size() < (size_t) Size + 8)
        throw midi::exception("Insufficient SMAF data");

    state_t State = { };

    container.Initialize(1u, 500);

    auto it = data.begin() + 8; // Skip past "MMMD" <size>.

    const auto Tail = data.begin() + (ptrdiff_t) Size;

    while (it < Tail)
    {
        if (Tail - it < 8)
            break;

        const ptrdiff_t ChunkSize = (ptrdiff_t) toInt32LE(it + 4);

        if ((Tail - it) < ChunkSize)
            throw midi::exception("Insufficient SMAF data");

        const std::string ChunkId(it, it + 4);

        // Is it a "Contents Info" chunk?
        if (::memcmp(&it[0], "CNTI", 4) == 0)
        {
            ::_putws(msc::FormatText(L"Chunk \"CNTI\", %zu bytes (Contents Information)", ChunkSize).c_str());

            it += (ptrdiff_t) 8;

            uint8_t Class      = it[0]; // 0: "Yamaha"
            uint8_t Type       = it[1];
            uint8_t Encoding   = it[2];
            uint8_t CopyStatus = it[3];
            uint8_t CopyCounts = it[4];

            ::printf("- Class: %s\n", (Class == 0x00) ? "Yamaha" : "Other");

            if ((0x00 <= Type && Type <= 0x0F) || (0x30 <= Type && Type <= 0x33))
                ::printf("- Type: Ringtone (0x%02X)\n", Type);
            else
            if ((0x10 <= Type && Type <= 0x1F) || (0x40 <= Type && Type <= 0x42))
                ::printf("- Type: Karaoke (0x%02X)\n", Type);
            else
            if ((0x20 <= Type && Type <= 0x2F) || (0x50 <= Type && Type <= 0x53))
                ::printf("- Type: CM (0x%02X)\n", Type);
            else
                ::printf("- Type: Reserved (0x%02X)\n", Type);

            ::printf("- Encoding: 0x%02X (%s)\n", Encoding, GetEncodingDescription(Encoding).c_str());
            
            ::printf("- Status: ");

            if ((CopyStatus & 0x01) == 0x00)
                ::printf("Transferable, ");

            if ((CopyStatus & 0x02) == 0x00)
                ::printf("Can be saved, ");

            if ((CopyStatus & 0x04) == 0x00)
                ::printf("Editable, ");

            ::puts("");

            ::printf("- Copy Count: %d\n", CopyCounts);

            it += (ptrdiff_t) 5;

            ProcessMetadata(std::span<const uint8_t>(&it[0], (size_t) ChunkSize - 5), State, container);

            it += (ptrdiff_t) ChunkSize - 5;
        }
        else
        // Is it a "Optional Data" chunk?
        if (::memcmp(&it[0], "OPDA", 4) == 0)
        {
            ::_putws(msc::FormatText(L"Chunk \"OPDA\", %zu bytes (Optional Data)", ChunkSize).c_str());

            it += (ptrdiff_t) 8;

            ProcessOPDA(std::span<const uint8_t>(&it[0], (size_t) ChunkSize), State, container);

            it += (ptrdiff_t) ChunkSize;
        }
        else
        // Is it a "Score Track" chunk?
        if (::memcmp(&it[0], "MTR", 3) == 0)
        {
            ::_putws(msc::FormatText(L"Chunk \"MTR_\", %zu bytes (Score Track)", ChunkSize).c_str());

            it += (ptrdiff_t) 8;

            ProcessMTR(std::span<const uint8_t>(&it[0], (size_t) ChunkSize), State, container);
            State.ChannelOffset += 4;

            it += (ptrdiff_t) ChunkSize;
        }

        else
        // Is it an "PCM Audio Track" chunk? Stores PCM audio sounds such as ADPCM, MP3, and TwinVQ in event format.
        if (::memcmp(&it[0], "ATR", 3) == 0)
        {
            ::_putws(msc::FormatText(L"Chunk \"ATR_\", %zu bytes (PCM Audio Track)", ChunkSize).c_str());

            it += (ptrdiff_t) 8 + ChunkSize;
        }
        else
        // Is it a "Graphics Track" chunk? Stores background images, inserted still images, text data, and sequence data for playing these.
        if (::memcmp(&it[0], "GTR", 3) == 0)
        {
            ::_putws(msc::FormatText(L"Chunk \"ATR_\", %zu bytes (Graphics Track)", ChunkSize).c_str());

            it += (ptrdiff_t) 8 + ChunkSize;
        }
        else
        // Is it a "Master Track" chunk? Stores music information sequences synchronized with playback sequences such as the Score Track, and sequence data for controlling the SMAF playback system.
        if (::memcmp(&it[0], "MSTR", 4) == 0)
        {
            ::_putws(msc::FormatText(L"Chunk \"MSTR\", %zu bytes (Master Track)", ChunkSize).c_str());

            it += (ptrdiff_t) 8 + ChunkSize;
        }
#ifdef _DEBUG
        else
        {
            ::_putws(msc::FormatText(L"Unknown chunk \"%S\", %zu bytes", ChunkId.c_str(), ChunkSize).c_str());
            it += (ptrdiff_t) 8 + ChunkSize;
        }
#endif
    }

//  uint16_t CRC = (uint16_t) ((it[0] << 8) | it[1]);

    // Convert the metadata.
    if (!State.Metadata.empty())
    {
        metadata_table_t MetaData;

        const std::unordered_map<std::string, std::string> MetadataMap =
        {
            { "ST", "title" },  // Song Title
            { "CR", "copyright" },
            { "WW", "lyricist" },
            { "VN", "vendor" },
            { "CN", "carrier" },
            { "CA", "category" },
            { "AN", "artist" },
            { "SW", "composer" },
            { "AW", "arranger" },
            { "GR", "group" },
            { "MI", "management_info" },
            { "CD", "creation_date" },
            { "UP", "modification_date" },
            { "ES", "edit_status" },
            { "VC", "vcard" },
        };

        for (const auto & md : State.Metadata)
        {
            auto Pair = MetadataMap.find(md.first);

            if (Pair != MetadataMap.end())
                MetaData.AddItem(metadata_item_t(0, Pair->second.c_str(), md.second.c_str()));
            else
                MetaData.AddItem(metadata_item_t(0, md.first.c_str(), md.second.c_str()));
        }

        container.SetExtraMetaData(MetaData);
    }

    return true;
}

/// <summary>
/// Processes metadata in the CNTI chunk.
/// </summary>
static void ProcessMetadata(const std::span<const uint8_t> & data, state_t & state, container_t & container)
{
    auto it = data.begin();
    const auto Tail = data.end();

    auto Anchor = it;

    while (it < Tail)
    {
        std::string Name = std::string(&it[0], &it[2]);

        it += 3; // Skip past name and ':'.

        std::string Value;

        while (it < Tail)
        {
            // Escape character
            if (it[0] == '\\')
            {
                it++;
                Value += (char) *it++;
            }
            else
            if (it[0] == ',')
            {
                state.Metadata.insert({ Name, Value });

                it++;
                break;
            }
            else
                Value += (char) *it++;
        }
    }
}

/// <summary>
/// Processes an OPDA chunk.
/// </summary>
static void ProcessOPDA(const std::span<const uint8_t> & data, state_t & state, container_t & container)
{
    auto it = data.begin();
    const auto Tail = data.end();

    while (it < Tail)
    {
        if (Tail - it < 8)
            break;

        const uint32_t ChunkSize = toInt32LE(it + 4);

        if ((Tail - it) < (ptrdiff_t) ChunkSize)
            throw midi::exception("Insufficient SMAF data");

        const std::string ChunkId(it, it + 4);

        if (::memcmp(&it[0], "Dch", 3) == 0)
        {
            ::_putws(msc::FormatText(L"Chunk \"Dch_\", %zu bytes (Data)", ChunkSize).c_str());

            uint8_t Encoding = it[3];

            ::printf("- Encoding: 0x%02X (%s)\n", Encoding, GetEncodingDescription(Encoding).c_str());
        }
#ifdef _DEBUG
        else
            ::_putws(msc::FormatText(L"Unknown chunk \"%S\", %zu bytes", ChunkId.c_str(), ChunkSize).c_str());
#endif
        it += (ptrdiff_t) (8 + ChunkSize);
    }
}

/// <summary>
/// Processes an MTR chunk.
/// </summary>
static void ProcessMTR(const std::span<const uint8_t> & data, state_t & state, container_t & container)
{
    auto it = data.begin();
    const auto Tail = data.end();

    // Decode the track header.
    {
        state.FormatType   = it[0];
        state.SequenceType = it[1];
        state.DurationBase = it[2];
        state.GateTimeBase = it[3];

        ::printf("- Format        : %02X\n", state.FormatType);
        ::printf("- Sequence      : %02X\n", state.SequenceType);
        ::printf("- Duration base : %02X\n", state.DurationBase);
        ::printf("- Gate Time base: %02X\n", state.GateTimeBase);

        switch (state.DurationBase)
        {
            case 0x00: state.DurationBase =  1; break;
            case 0x01: state.DurationBase =  2; break;
            case 0x02: state.DurationBase =  4; break;
            case 0x03: state.DurationBase =  5; break;
            case 0x10: state.DurationBase = 10; break;
            case 0x11: state.DurationBase = 20; break;
            case 0x12: state.DurationBase = 40; break;
            case 0x13: state.DurationBase = 50; break;

            default:
                throw midi::exception("Unknown duration base");
        }

        switch (state.GateTimeBase)
        {
            case 0x00: state.GateTimeBase =  1; break;
            case 0x01: state.GateTimeBase =  2; break;
            case 0x02: state.GateTimeBase =  4; break;
            case 0x03: state.GateTimeBase =  5; break;
            case 0x10: state.GateTimeBase = 10; break;
            case 0x11: state.GateTimeBase = 20; break;
            case 0x12: state.GateTimeBase = 40; break;
            case 0x13: state.GateTimeBase = 50; break;

            default:
                throw midi::exception("Unknown gate time base");
        }

        switch (state.FormatType)
        {
            case HandyPhoneStandard: // HPS
            {
                std::vector<channel_t> Channels;

                Channels.push_back(channel_t(0, (it[0] & 0x80) == 0x80, (it[0] & 0x40) == 0x40, (it[0] >> 4) & 0x03, TrackFormat::MA2));
                Channels.push_back(channel_t(1, (it[0] & 0x08) == 0x08, (it[0] & 0x04) == 0x04, (it[0]     ) & 0x03, TrackFormat::MA2));
                Channels.push_back(channel_t(2, (it[1] & 0x80) == 0x80, (it[1] & 0x40) == 0x40, (it[1] >> 4) & 0x03, TrackFormat::MA2));
                Channels.push_back(channel_t(3, (it[1] & 0x08) == 0x08, (it[1] & 0x04) == 0x04, (it[1]     ) & 0x03, TrackFormat::MA2));

                it += (ptrdiff_t) 2;
                break;
            }

            case MobileStandard_Compress: // SMF
            {
                uint8_t Buffer[16] = { };

                std::vector<channel_t> Channels;

                for (int i = 0; i < 16; ++i)
                    Channels.push_back(channel_t(i, (int) Buffer[i]));

                it += (ptrdiff_t) 16;

                throw midi::exception("SMAF Format 1 not supported yet");
                break;
            }

            case MobileStandard_NoCompress: // SMF
            {
                uint8_t Buffer[16] = { };

                std::vector<channel_t> Channels;

                for (int i = 0; i < 16; ++i)
                    Channels.push_back(channel_t(i, (int) Buffer[i]));

                it += (ptrdiff_t) 16;
                throw midi::exception("SMAF Format 2 not supported yet");
                break;
            }

            case SEQU: // ???
            {
                uint8_t Buffer[32] = { };

                std::vector<channel_t> Channels;

                for (int i = 0; i < 32; ++i)
                    Channels.push_back(channel_t(i, (int) Buffer[i]));

                it += (ptrdiff_t) 32;
                throw midi::exception("SMAF Format 3 not supported yet");
                break;
            }

            default:
                throw midi::exception("Unknown SMAF Format");
        }

        it += (ptrdiff_t) 4;
    }

    if (state.FormatType != SMAFFormat::HandyPhoneStandard)
        return;

    while (it < Tail)
    {
        if (Tail - it < 8)
            break;

        const uint32_t ChunkSize = toInt32LE(it + 4);

        if ((Tail - it) < (ptrdiff_t) ChunkSize)
            throw midi::exception("Insufficient SMAF data");

        const std::string ChunkId(it, it + 4);

        if (::memcmp(&it[0], "Mtsu", 4) == 0)
        {
            ::_putws(msc::FormatText(L"Chunk \"%S\", %zu bytes (Setup Data)", ChunkId.c_str(), ChunkSize).c_str());

            state.IsMTSU = true;

            if (state.FormatType == 0)
                ProcessHPSTrack(std::span<const uint8_t>(&it[8], ChunkSize), state, container);
        }
        else
        if (::memcmp(&it[0], "Mtsq", 4) == 0)
        {
            ::_putws(msc::FormatText(L"Chunk \"%S\", %zu bytes (Sequence Data)", ChunkId.c_str(), ChunkSize).c_str());

            state.IsMTSU = false;

            if (state.FormatType == 0)
                ProcessHPSTrack(std::span<const uint8_t>(&it[8], ChunkSize), state, container);
        }
        else
        if (::memcmp(&it[0], "MspI", 4) == 0)
        {
            ::_putws(msc::FormatText(L"Chunk \"%S\", %zu bytes (Seek & Phrase Info)", ChunkId.c_str(), ChunkSize).c_str());
        }
        else
            ::_putws(msc::FormatText(L"Unknown chunk \"%S\", %zu bytes", ChunkId.c_str(), ChunkSize).c_str());

        it += (ptrdiff_t) (8 + ChunkSize);
    }
}

/// <summary>
/// Processes an HPS (Handy Phone Standard) track.
/// </summary>
static void ProcessHPSTrack(const std::span<const uint8_t> & data, state_t & state, container_t & container)
{
    track_t Track;

    uint32_t RunningTime = 0;
    int8_t OctaveShift[4] = { }; // 4 HPS channels.

    auto it = data.begin();
    const auto Tail = data.end();

    if (state.IsMTSU)
    {
        const uint8_t XGSystemOn[] = { 0xF0, 0x43, 0x00, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

        Track.AddEvent(event_t(RunningTime, event_t::Extended, 0, XGSystemOn, _countof(XGSystemOn)));
    }

    while (it < Tail)
    {
        uint32_t Duration = !state.IsMTSU ? GetHPSValue(it) * state.DurationBase : 0;

        RunningTime += Duration;

        // End of Sequence
        if (it[0] == 0x00 && it[1] == 0x00 && it[2] == 0x00 && it[3] == 0x00)
        {
            const uint8_t Data[] = { StatusCode::MetaData, MetaDataType::EndOfTrack };

            Track.AddEvent(event_t(RunningTime, event_t::Extended, (uint32_t) 0, Data, 2));
            break;
        }

        if (!state.IsMTSU)
            GetHPSValueEx(it);

        // Exclusive event?
        if (it[0] == 0xFF && it[1] == 0xF0)
        {
            if ((it[2] == 0x12 || it[2] == 0x1C) && (it[3] == 0x43) && (it[4] == 0x03) && (it[9] == 0x01))
            {
                CHPARAM chp = { };
                OPPARAM opp[4] = { };

                if (GetHPSExclusiveFMMessage(&it[2], &chp, opp))
                {
                    std::vector<uint8_t> Temp(48);

                    size_t Size = SetMA3ExclusiveMessage(Temp.data(), &chp, opp);

                    Track.AddEvent(event_t(RunningTime, event_t::Extended, 0, Temp.data(), Size));
                }
                else
                {
                    // Conversion failed. Just copy the SysEx.
                    const size_t Size = (size_t) it[2] + 2u;

                    std::vector<uint8_t> Temp(Size);

                    std::copy(it + 1, it + 1 + (ptrdiff_t) (Size), Temp.begin());

                    Track.AddEvent(event_t(RunningTime, event_t::Extended, 0, Temp.data(), Temp.size()));
                }
            }
            else
            {
                const size_t Size = (size_t) it[2] + 2u;

                std::vector<uint8_t> Temp(Size);

                std::copy(it + 1, it + 1 + (ptrdiff_t) (Size), Temp.begin());

                Track.AddEvent(event_t(RunningTime, event_t::Extended, 0, Temp.data(), Temp.size()));
            }

            it += (ptrdiff_t) 2 + it[2] + 1;
        }
        else
        // NOP
        if (it[0] == 0xFF && it[1] == 0x00)
        {
            it += 2;
        }
        else
        {
            // Note
            if (it[0] != 0x00)
            {
                uint8_t Channel = (it[0] >> 6 & 0x03u);
                uint8_t Octave  =  it[0] >> 4 & 0x03u;
                uint8_t Note    =  it[0]      & 0x0Fu;

                Note += 36u + ((Octave + OctaveShift[Channel]) * 12u); // MIDI's 69 = 440Hz(A) = Oct2,9

                const uint8_t Data[2] = { Note, 0x7Fu };

                Track.AddEvent(event_t(RunningTime, event_t::NoteOn,  (uint32_t) Channel + state.ChannelOffset, Data, 2));
                it += 1;

                uint32_t GateTime = GetHPSValueEx(it) * state.GateTimeBase;

                Track.AddEvent(event_t(RunningTime + GateTime, event_t::NoteOff, (uint32_t) Channel + state.ChannelOffset, Data, 2));
            }
            else
            {
                uint8_t Channel = (it[1] >> 6 & 0x03u) + state.ChannelOffset;

                if ((it[1] & 0x30) == 0x30)
                {
                    switch (it[1] & 0x0F)
                    {
                        // Program Change
                        case 0x00:
                        {
                            Track.AddEvent(event_t(RunningTime, event_t::ProgramChange, Channel, &it[2], 1));
                            it += 3;
                            break;
                        }

                        // Bank Select
                        case 0x01:
                        {
                            bool IsDrum  = (it[2] & 0x80u) == 0x80;

                            if (IsDrum)
                            {
                                const uint8_t Part = Channel;
                                const uint8_t Mode = 0x02; // Drum Setup 1

                                const uint8_t XGPartMode[] = { 0xF0, 0x43, 0x10, 0x4C, 0x08, Part, 0x07, Mode, 0xF7 };

                                Track.AddEvent(event_t(0, event_t::Extended, 0, XGPartMode, _countof(XGPartMode)));
                            }
                            else
                            {
                                uint8_t Data[2] = { 0x00, it[2] & 0x7Fu };

                                Track.AddEvent(event_t(RunningTime, event_t::ControlChange, Channel, Data, 2)); // MSB

                                Data[0] = 0x20u;
                                Data[1] = 0x00u;

                                Track.AddEvent(event_t(RunningTime, event_t::ControlChange, Channel, Data, 2)); // LSB
                            }

                            it += 3;
                            break;
                        }

                        // Octave Shift
                        case 0x02:
                        {
                            const size_t i = ((size_t) it[0] >> 6) & 0x03u;

                            if (0x01 <= it[2] && it[2] <= 0x04)
                                OctaveShift[i] = (int8_t) it[2];
                            else
                            if (0x81 <= it[2] && it[2] <= 0x84)
                                OctaveShift[i] = (int8_t) -(it[2] - 0x80);

                            it += 3;
                            break;
                        }

                        // Modulation
                        case 0x03:
                        {
                            const uint8_t Data[2] = { 0x01, it[2] };

                            Track.AddEvent(event_t(RunningTime, event_t::ControlChange, Channel, Data, 2));
                            it += 3;
                            break;
                        }

                        // Pitch Bend
                        case 0x04:
                        {
                            const uint8_t Data[2] = { 0x00, it[2] };

                            Track.AddEvent(event_t(RunningTime, event_t::PitchBendChange, Channel, Data, 2));
                            it += 3;
                            break;
                        }

                        // Volume
                        case 0x07:
                        {
                            const uint8_t Data[2] = { 0x07, it[2] };

                            Track.AddEvent(event_t(RunningTime, event_t::ControlChange, Channel, Data, 2));
                            it += 3;
                            break;
                        }

                        // Pan
                        case 0x0A:
                        {
                            const uint8_t Data[2] = { 0x0A, it[2] };

                            Track.AddEvent(event_t(RunningTime, event_t::ControlChange, Channel, Data, 2));
                            it += 3;
                            break;
                        }

                        // Expression
                        case 0x0B:
                        {
                            const uint8_t Data[2] = { 0x0B, it[2] };

                            Track.AddEvent(event_t(RunningTime, event_t::ControlChange, Channel, Data, 2));
                            it += 3;
                            break;
                        }

                        default:
                            ::printf("Unknown opcode 0x%02X\n", it[1] & 0x0F);
                    }
                }
                else
                {
                    switch (it[1] & 0x30)
                    {
                        // Expression
                        case 0x00:
                        {
                            const uint8_t Lookup[] =
                            {
                                0x00 /* Reserved */,
                                0x00, 0x1F,
                                0x27, 0x2F,
                                0x37, 0x3F,
                                0x47, 0x4F,
                                0x57, 0x5F,
                                0x67, 0x6F,
                                0x77, 0x7F,
                                0x00 /* Reserved */
                            };

                            const uint8_t Data[2] = { 0x0B, Lookup[it[1] & 0x0F] };

                            Track.AddEvent(event_t(RunningTime, event_t::ControlChange, Channel, Data, 2));
                            it += 2;
                            break;
                        }

                        // Pitch Bend
                        case 0x10:
                        {
                            const uint8_t Lookup[] =
                            {
                                0x00 /* Reserved */,
                                0x08, 0x10,
                                0x18, 0x20,
                                0x28, 0x30,
                                0x38, 0x40,
                                0x48, 0x50,
                                0x58, 0x60,
                                0x68, 0x70,
                                0x00 /* Reserved */
                            };

                            const uint8_t Data[2] = { 0x00, Lookup[it[1] & 0xF0] };

                            Track.AddEvent(event_t(RunningTime, event_t::PitchBendChange, Channel, Data, 2));
                            it += 2;
                            break;
                        }

                        // Modulation
                        case 0x20:
                        {
                            const uint8_t Lookup[] =
                            {
                                0x00 /* Reserved */,
                                0x08, 0x10,
                                0x18, 0x20,
                                0x28, 0x30,
                                0x38, 0x40,
                                0x48, 0x50,
                                0x60, 0x68,
                                0x70, 0x7F,
                                0x00 /* Reserved */
                            };

                            const uint8_t Data[2] = { 0x01, Lookup[it[1] & 0xF0] };

                            Track.AddEvent(event_t(RunningTime, event_t::ControlChange, Channel, Data, 2));
                            it += 2;
                            break;
                        }

                        default:
                            ::printf("Unknown opcode 0x%02X\n", it[1] & 0x30);
                    }
                }
            }
        }
    }

    container.AddTrack(Track);
}

/// <summary>
/// Gets an HPS-encoded value.
/// </summary>
static uint32_t GetHPSValue(std::span<const uint8_t>::iterator data) noexcept
{
    uint32_t Value = 0;

    if (data[0] & 0x80)
        Value = (uint32_t) (((*data++ & 0x7F) + 1) << 7);

    Value |= *data++;

    return Value;
}

/// <summary>
/// Gets an HPS-encoded value and advances the data pointer.
/// </summary>
static uint32_t GetHPSValueEx(std::span<const uint8_t>::iterator & data) noexcept
{
    uint32_t Value = 0;

    if (data[0] & 0x80)
        Value = (uint32_t) (((*data++ & 0x7F) + 1) << 7);

    Value |= *data++;

    return Value;
}

/// <summary>
/// Gets the description of an encoding type.
/// </summary>
static std::string GetEncodingDescription(uint8_t encoding) noexcept
{
    switch (encoding)
    {
        case 0x00: return std::string("Shift-JIS");             // Japanese
        case 0x01: return std::string("ISO 8859-1 (Latin-1)");
        case 0x02: return std::string("EUC-KR (KS)");           // Korean
        case 0x03: return std::string("HZ-GB-2312");            // Chinese (Simplified)
        case 0x04: return std::string("Big5");                  // Chinese (Simplified)
        case 0x05: return std::string("KOI8-R");                // Russian etc.
        case 0x06: return std::string("TCVN-5773:1993");        // Vietnamese

        case 0x20: return std::string("UCS-2");                 // Unicode
        case 0x21: return std::string("UCS-4");                 // Unicode
        case 0x22: return std::string("UTF-7");                 // Unicode
        case 0x23: return std::string("UTF-8");                 // Unicode
        case 0x24: return std::string("UTF-16");                // Unicode
        case 0x25: return std::string("UTF-32");                // Unicode

        case 0xFF: return std::string("Binary");                // Only used in OPDA chunk

        default:   return msc::FormatText("Unknown (0x%02X)\n", encoding);
    }
}

}
