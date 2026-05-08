
/** $VER: Converter.cpp (2026.05.08) P. Stuer - Based on Valley Bell's mmd2mid (https://github.com/ValleyBell/MidiConverters). **/

#include "pch.h"

#include <CppCoreCheck\Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4738 4820 5045 ALL_CPPCORECHECK_WARNINGS)

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <vector>

#include "MMD.h"

#include <MIDI.h>

namespace mmd
{

struct loop_t
{
    uint8_t Command[4];
    uint32_t Offset;
    uint32_t Length;
    uint16_t Count;
};

}

namespace mmd
{

static size_t GetSysExSize(const uint8_t * data, size_t size, size_t startOffset) noexcept;
static void GetSysEx(const uint8_t * srcData, uint8_t param1, uint8_t param2, uint8_t channelNumber, std::vector<uint8_t> & dstData) noexcept;

static uint32_t MMDTempo2MIDITempo(uint16_t bpm, uint8_t scale) noexcept;
static uint16_t ReadLE16(const uint8_t * data) noexcept;

const uint16_t MIDIResolution = 48u;

// Options
const uint16_t MaxLoopExpansions = 2u;  // Expand loops this many times.
const bool ExpandLoops = false;
const bool IgnoreMutedTracks = true;

/// <summary>
/// Converts the MMD data.
/// </summary>
bool converter_t::ToSMF(const uint8_t * srcData, uint32_t srcSize, std::vector<uint8_t> & dstData) noexcept
{
    _Tempo     = srcData[0x00];
    _Transpose = (int8_t) srcData[0x01];

    uint32_t Offset = 2;

    std::vector<track_t> Tracks(18);

    for (auto & Track : Tracks)
    {
        Track.Offset    = ReadLE16(&srcData[Offset]);
        Track.Transpose = (int8_t) srcData[Offset + 2];
        Track.Channel   = srcData[Offset + 3];

        if (Track.Offset >= srcSize)
            return false; // Invalid track offset

        Offset += 4;
    }

    Offset = ReadLE16(&srcData[0x4A]);

    // Princess Maker 1 (MMD v2.0) has the first track starting at 0x4A and no User SysEx pointers or data.
    if ((Tracks[0].Offset >= 0x4C) && (Offset != 0) && (Offset < srcSize))
    {
        for (size_t i = 0; (i < _countof(_SysEx)) && (Offset + 0x01 < srcSize); ++i, Offset += 0x02)
        {
            const uint16_t SysExOffset = ReadLE16(&srcData[Offset]);

            if (SysExOffset >= srcSize)
                continue;

            _SysEx[i] = srcData + SysExOffset;
        }
    }

    if (Tracks[0].Offset > 0x50)
        _Title = (const char *) &srcData[0x50];

    for (auto & Track : Tracks)
        GetLoops(srcData, srcSize, Track);

    if (ExpandLoops)
        AdjustTracks(Tracks, (uint32_t) (MIDIResolution / 4));

    bool Success = false;

    {
        stream_t Stream(0x20000); // 128 KB

        // Write the MIDI header with default values.
        Stream.WriteHeader(0x0001, 0, MIDIResolution);

        uint8_t TrackNumber = 0;

        for (auto & Track : Tracks)
        {
            Stream.WriteTrackBegin();

            if (TrackNumber == 0)
            {
                if ((_Title != nullptr) && (_Title[0] != '\0'))
                    Stream.WriteMetaEvent(midi::MetaDataType::TrackName, _Title, (uint32_t) ::strlen(_Title));

                const uint32_t Tempo = MMDTempo2MIDITempo(_Tempo, 64);

                uint8_t Data[32] = { };

                WriteBE32(Data, Tempo);

                Stream.WriteMetaEvent(midi::MetaDataType::SetTempo, Data + 1, 3);
            }

            Success = ConvertTrack(srcData, srcSize, Track, Stream, TrackNumber);

            Stream.WriteEvent(midi::StatusCode::MetaData, midi::MetaDataType::EndOfTrack, 0x00);

            Stream.WriteTrackEnd();

            if (!Success)
                break;

            TrackNumber++;
        }

        // Update the MIDI header with the actual track count and length.
        {
            Offset = (uint32_t) Stream.Offset;

            Stream.Offset = 0;
            Stream.WriteHeader(0x0001, TrackNumber, MIDIResolution);

            dstData.assign(Stream.Data, Stream.Data + Offset);
        }
    }

    return Success;
}

/// <summary>
/// Adjusts the value of LoopCount so that all tracks play for the approximately same time.
/// Ignore loops that are shorter than minLoopTicks.
/// Returns the number of adjusted tracks.
/// </summary>
void converter_t::AdjustTracks(std::vector<track_t> & tracks, uint32_t minLoopTicks) noexcept
{
    uint32_t MaxLength = 0;

    // Determine the longest track.
    for (auto & Track : tracks)
    {
        uint32_t TrackLength = Track.Length;

        if (Track.MaxLoopExpansions != 0)
        {
            const uint32_t LoopLength = Track.Length - Track.LoopLength;

            TrackLength += (LoopLength * (Track.MaxLoopExpansions - 1));
        }

        if (MaxLength < TrackLength)
            MaxLength = TrackLength;
    }

    for (auto & Track : tracks)
    {
        const uint32_t LoopLength = (Track.MaxLoopExpansions != 0) ? (Track.Length - Track.MaxLoopExpansions) : 0;

        if (LoopLength < minLoopTicks)
            continue; // Ignore tracks with very short loops

        // heuristic: The track needs additional loops, if the longest track is longer than the current track + 1/4 loop.
        uint32_t TrackLength = Track.Length + LoopLength * (Track.MaxLoopExpansions - 1);

        if (TrackLength + LoopLength / 4 < MaxLength)
        {
            TrackLength = MaxLength - Track.LoopLength; // desired length of the loop

            Track.MaxLoopExpansions = (uint16_t)((TrackLength + LoopLength / 3) / LoopLength);
        }
    }
}

/// <summary>
/// Gets the loop information for the specified track. Returns false if the track is invalid.
/// </summary>
bool converter_t::GetLoops(const uint8_t * data, uint32_t size, track_t & track) noexcept
{
    size_t Offset = track.Offset;

    bool EndOfTrack = false;

    uint8_t Command[4] = { };
    loop_t Loops[16] = { };

    uint8_t LoopCount = 0;

    while (Offset < size && !EndOfTrack)
    {
        uint8_t CommandCode = data[Offset];

        if (msc::InRange((int) CommandCode, 0x80, 0x8F))
        {
            uint8_t CommandMask = (uint8_t) (CommandCode & 0x0F);

            Offset++;

            for (size_t i = 0; i < _countof(Command); ++i, CommandMask <<= 1)
            {
                if (CommandMask & 0x08)
                    Command[i] = data[Offset++];
            }
        }
        else
        {
            for (size_t i = 0; i < _countof(Command); ++i)
                Command[i] = data[Offset++];
        }

        CommandCode = Command[0];

        const uint8_t CommandDelay = (CommandCode >= 0xF0) ? 0u : Command[1];

        switch (CommandCode)
        {
            case 0x98: // Send SysEx command
            {
                Offset += GetSysExSize(data, size, Offset);
                break;
            }

            case 0xF8: // Loop End
            {
                if (LoopCount > 0)
                {
                    Loops[--LoopCount].Count++;

                    if ((Command[1] == 0) || (Command[1] >= 0x7F))
                    {
                        track.LoopOffset = Loops[LoopCount].Offset;
                        track.LoopLength = Loops[LoopCount].Length;

                        EndOfTrack = true;
                    }
                    else
                    if (Loops[LoopCount].Count < Command[1])
                    {
                        ::memcpy(Command, Loops[LoopCount].Command, sizeof(Command));

                        Offset = Loops[LoopCount++].Offset;
                    }
                }
                break;
            }

            case 0xF9: // Loop Begin
            {
                if (LoopCount < _countof(Loops))
                {
                    ::memcpy(Loops[LoopCount].Command, Command, sizeof(Command));

                    Loops[LoopCount].Offset = (uint32_t) Offset;
                    Loops[LoopCount].Length = track.Length;
                    Loops[LoopCount].Count = 0;

                    if ((LoopCount > 0) && (Loops[LoopCount].Offset == Loops[LoopCount - 1].Offset))
                        LoopCount--; // Ignore this loop command.

                    LoopCount++;
                }
                break;
            }

            case 0xFE: // Track End
            {
                EndOfTrack = true;
                break;
            }
        }

        track.Length += CommandDelay; 
    }

    track.MaxLoopExpansions = (track.LoopOffset != 0) ? MaxLoopExpansions : 0u;

    return true;
}

/// <summary>
/// Converts an MMD track to MIDI events.
/// </summary>
bool converter_t::ConvertTrack(const uint8_t * data, uint32_t size, track_t & track, stream_t & stream, uint8_t trackNumber) noexcept
{
    if (track.Offset >= size)
        return false;

    uint8_t PortNumber = 0;
    uint8_t ChannelNumber = 0;

    if (track.Channel == 0xFF)
    {
        PortNumber = IgnoreMutedTracks ? 0xFF : 0x00;
        ChannelNumber = 0;

        track.Channel = 0x00;
    }
    else
    {
        PortNumber = (uint8_t) (track.Channel >> 4);
        ChannelNumber = track.Channel & 0x0Fu;
    }

    if (PortNumber != 0xFF)
    {
        stream.WriteMetaEvent(midi::MetaDataType::MIDIPort, &PortNumber, 1);
        stream.WriteMetaEvent(midi::MetaDataType::ChannelPrefix, &ChannelNumber, 1);
    }

    int8_t Transpose = track.Transpose;

    if (Transpose & 0x80)
    {
        Transpose = 0; // Ignore transposition setting for rhythm tracks
    }
    else
    {
        Transpose = (Transpose & 0x40) ? (-0x80 + Transpose) : Transpose; // 7-bit -> 8-bit sign extension

        Transpose += _Transpose; // Add global transposition
    }

    bool EndOfTrack = false;

    stream.BeginTrack(ChannelNumber);

    size_t Offset = track.Offset;
    size_t LoopCount = 0;

    uint8_t Command[4] = { };
    loop_t Loops[16] = { };
    uint8_t GSParameters[6] = { }; // 0 device ID, 1 model ID, 2 address high, 3 address low

    while ((Offset < size) && !EndOfTrack)
    {
        uint8_t CommandCode = data[Offset];

        if (msc::InRange((int) CommandCode, 0x80, 0x8F))
        {
            uint8_t CommandMask = CommandCode & 0x0Fu;

            Offset++;

            for (size_t i = 0; i < _countof(Command); ++i, CommandMask <<= 1)
            {
                if (CommandMask & 0x08)
                    Command[i] = data[Offset++];
            }
        }
        else
        {
            for (size_t i = 0; i < _countof(Command); ++i)
                Command[i] = data[Offset++];
        }

        CommandCode = Command[0];

        const uint8_t CommandDelay = (CommandCode >= 0xF0) ? 0u : Command[1];

        if (CommandCode < 0x80)
        {
            uint8_t Note = 0;

            const uint8_t Duration = Command[2];

            // Emit a note if the duration and the velocity are non-zero.
            bool EmitNote = (Duration > 0) && (Command[3] > 0);

            if (EmitNote)
            {
                Note = (CommandCode + Transpose) & 0x7Fu;

                EmitNote = stream.EmitNote(stream, Note, Duration);
            }

            if (EmitNote && (PortNumber != 0xFF))
            {
                stream.WriteEvent(midi::StatusCode::NoteOn, Note, Command[3]);

                stream.Add(ChannelNumber, Note, 0x80, Duration);
            }
        }
        else
        {
            switch (CommandCode)
            {
                case 0x90: case 0x91: case 0x92: case 0x93: // Send User SysEx
                case 0x94: case 0x95: case 0x96: case 0x97:
                {
                    if (PortNumber == 0xFF)
                        break;

                    const uint8_t * UserSysEx = _SysEx[CommandCode & 0x07];

                    if (UserSysEx == nullptr)
                        break; // Undefined User SysEx

                    std::vector<uint8_t> SysEx;

                    GetSysEx(UserSysEx, Command[2], Command[3], ChannelNumber, SysEx);

                    if (SysEx.size() > 1)
                        stream.WriteEvent(midi::StatusCode::SysEx, SysEx.data(), (uint32_t) SysEx.size());
                    break;
                }

                case 0x98: // Send SysEx
                {
                    const size_t SrcSize = GetSysExSize(data, size, Offset);

                    if (PortNumber != 0xFF)
                    {
                        std::vector<uint8_t> SysEx;

                        GetSysEx(&data[Offset], Command[2], Command[3], ChannelNumber, SysEx);

                        if (SysEx.size() > 1)
                            stream.WriteEvent(midi::StatusCode::SysEx, SysEx.data(), (uint32_t) SysEx.size());
                    }

                    Offset += SrcSize;
                    break;
                }

                case 0xC0: // DX7 Function
                case 0xC1: // DX Parameter
                case 0xC2: // DX RERF
                case 0xC3: // TX Function

                case 0xC7: // TX81Z V VCED
                case 0xC8: // TX81Z A ACED
                case 0xC9: // TX81Z P PCED

                case 0xCC: // DX7-2 R Remote SW
                case 0xCD: // DX7-2 A ACED
                case 0xCE: // DX7-2 P PCED
                {
                    if (PortNumber == 0xFF)
                        break;

                    static const uint8_t DXParameters[16] =
                    {
                        0x08, 0x00, 0x04, 0x11, 0xFF, 0x15, 0xFF, 0x12,
                        0x13, 0x10, 0xFF, 0xFF, 0x1B, 0x18, 0x19, 0x1A,
                    };

                    const uint8_t Data[] =
                    {
                        0x43u, // Yamaha
                        0x10u | stream.ChannelNumber,
                        (uint8_t) (DXParameters[CommandCode & 0x0F] | (Command[2] >> 7)),
                        Command[2],
                        Command[3],
                        0xF7u
                    };

                    stream.WriteEvent(midi::StatusCode::SysEx, Data, 6);
                    break;
                }

                case 0xC5: // FB-01 P Parameter
                {
                    if (PortNumber == 0xFF)
                        break;

                    uint8_t Data[64] = { };

                    Data[0] = 0x43u; // Yamaha
                    Data[1] = 0x10u | stream.ChannelNumber;
                    Data[2] = 0x15u;
                    Data[3] = Command[2];

                    if (Command[2] < 0x40u)
                    {
                        Data[4] = Command[3];
                        Data[5] = Command[3] & 0x0Fu;
                        Data[6] = (uint8_t) (Command[3] >> 4);
                        Data[7] = 0xF7u;

                        stream.WriteEvent(midi::StatusCode::SysEx, Data, 8);
                    }
                    else
                    {
                        Data[4] = Command[3] & 0x0Fu;
                        Data[5] = (uint8_t) (Command[3] >> 4);
                        Data[6] = 0xF7u;

                        stream.WriteEvent(midi::StatusCode::SysEx, Data, 7);
                    }
                    break;
                }

                case 0xC6: // FB-01 S System
                {
                    if (PortNumber == 0xFF)
                        break;

                    const uint8_t Data[] =
                    {
                        0x43, // Yamaha
                        0x75,
                        stream.ChannelNumber,
                        0x10,
                        Command[2],
                        Command[3],
                        0xF7
                    };

                    stream.WriteEvent(midi::StatusCode::SysEx, Data, 7);
                    break;
                }

                case 0xCA: // TX81Z S System
                case 0xCB: // TX81Z E EFFECT
                {
                    if (PortNumber == 0xFF)
                        break;

                    const uint8_t Data[] =
                    {
                        0x43, // Yamaha
                        (uint8_t) (0x10 | stream.ChannelNumber),
                        (uint8_t) (0x7B + (CommandCode - 0xCA)), // command CA -> param = 7B, command CB -> param = 7C
                        Command[2],
                        Command[3],
                        0xF7
                    };

                    stream.WriteEvent(0xF0, Data, 6);
                    break;
                }

                case 0xCF: // TX802 P PCED
                {
                    if (PortNumber == 0xFF)
                        break;

                    uint8_t Data[64] = { };

                    Data[0] = 0x43; // Yamaha
                    Data[1] = (uint8_t) (0x10 | stream.ChannelNumber);
                    Data[2] = 0x1A;
                    Data[3] = Command[2];

                    if ((Command[2] < 0x1B) || (Command[2] >= 0x60))
                    {
                        Data[4] = Command[3];
                        Data[5] = 0xF7;

                        stream.WriteEvent(midi::StatusCode::SysEx, Data, 6);
                    }
                    else
                    {
                        Data[4] = (uint8_t) (Command[3] >> 7);
                        Data[5] = (uint8_t) (Command[3] & 0x7F);
                        Data[6] = 0xF7;

                        stream.WriteEvent(midi::StatusCode::SysEx, Data, 7);
                    }
                    break;
                }

            //  case 0xD0: // Yamaha Base Address
            //  case 0xD1: // Yamaha Device Data
            //  case 0xD2: // Yamaha Address / Parameter
            //  case 0xD3: // Yamaha XG Address / Parameter D0..D3 is not supported by MMD 2.2

                case 0xDC: // Roland MKS-7
                {
                    if (PortNumber == 0xFF)
                        break;

                    const uint8_t Data[64] =
                    {
                        0x41, // Roland
                        0x32,
                        stream.ChannelNumber,
                        Command[2],
                        Command[3],
                        0xF7
                    };

                    stream.WriteEvent(midi::StatusCode::SysEx, Data, 6);
                    break;
                }

                case 0xDD: // Roland Base Address
                {
                    GSParameters[2] = Command[2];
                    GSParameters[3] = Command[3];
                    break;
                }

                case 0xDE: // Roland Parameter
                {
                    GSParameters[4] = Command[2];
                    GSParameters[5] = Command[3];

                    if (PortNumber == 0xFF)
                        break;

                    uint8_t Data[64] = { };

                    Data[0] = 0x41; // Roland
                    Data[1] = GSParameters[0];
                    Data[2] = GSParameters[1];
                    Data[3] = 0x12; // DT1

                    uint8_t Checksum = 0;

                    for (size_t i = 0; i < 4; ++i)
                    {
                        Data[4 + i] = GSParameters[2 + i];
                        Checksum += GSParameters[2 + i];
                    }

                    Data[8] = (uint8_t) ((0x100 - Checksum) & 0x7F);
                    Data[9] = 0xF7;

                    stream.WriteEvent(midi::StatusCode::SysEx, Data, 10);
                    break;
                }

                case 0xDF: // Roland Device
                {
                    GSParameters[0] = Command[2];
                    GSParameters[1] = Command[3];
                    break;
                }

                case 0xE2: // Set GS instrument
                {
                    if (PortNumber == 0xFF)
                        break;

                    stream.WriteEvent(midi::StatusCode::ControlChange, midi::Controller::BankSelect, Command[3]);
                    stream.WriteEvent(midi::StatusCode::ControlChange, midi::Controller::BankSelectLSB, 0x00);
                    stream.WriteEvent(midi::StatusCode::ProgramChange, Command[2], 0x00);
                    break;
                }

                case 0xE6: // MIDI channel number
                {
                    const uint8_t Byte = Command[2] - 1u; // It's same as in the track header, except 1 added.

                    if (Byte == 0xFF)
                    {
                        if (IgnoreMutedTracks)
                        {
                            PortNumber = 0xFF;
                            ChannelNumber = 0x00;
                        }
                    }
                    else
                    {
                        PortNumber  = (uint8_t) (Byte >> 4); // port ID
                        ChannelNumber = (uint8_t) (Byte & 0x0F); // channel ID

                        stream.WriteMetaEvent(midi::MetaDataType::MIDIPort, &ChannelNumber, 1);
                        stream.WriteMetaEvent(midi::MetaDataType::ChannelPrefix, &ChannelNumber, 1);
                    }

                    stream.ChannelNumber = ChannelNumber;
                    break;
                }

                case 0xE7: // Tempo Modifier
                {
//                  if (Command[3] != 0)
//                      ::printf("Track %u: Gradual Tempo Change 0x%02X at 0x%04X.\n", trackNumber, Command[3], OldOffset);

                    const uint32_t Tempo = MMDTempo2MIDITempo(_Tempo, Command[2]);

                    uint8_t Data[32] = { };

                    WriteBE32(Data, Tempo);

                    stream.WriteMetaEvent(midi::MetaDataType::SetTempo, Data + 1, 3);
                    break;
                }

                case 0xEA: // Channel Aftertouch
                {
                    if (PortNumber == 0xFF)
                        break;

                    stream.WriteEvent(midi::StatusCode::ChannelPressure, Command[2], 0x00);
                    break;
                }

                case 0xEB: // Control Change
                {
                    if (PortNumber == 0xFF)
                        break;

                    stream.WriteEvent(midi::StatusCode::ControlChange, Command[2], Command[3]);
                    break;
                }

                case 0xEC: // Instrument
                {
                    if (PortNumber == 0xFF)
                        break;

                    stream.WriteEvent(midi::StatusCode::ProgramChange, Command[2], 0x00);
                    break;
                }

                case 0xED: // Note Aftertouch
                {
                    if (PortNumber == 0xFF)
                        break;

                    stream.WriteEvent(midi::StatusCode::KeyPressure, Command[2], Command[3]);
                    break;
                }

                case 0xEE: // Pitch Bend
                {
                    if (PortNumber == 0xFF)
                        break;

                    stream.WriteEvent(midi::StatusCode::PitchBendChange, Command[2], Command[3]);
                    break;
                }

            //  case 0xF6: // Comment
            //  case 0xF7: // Continuation of previous command

                case 0xF8: // Loop End
                {
                    if (LoopCount == 0)
                        break; // Loop End without Loop Begin

                    LoopCount--;

                    Loops[LoopCount].Count++;

                    bool TakeLoop = false;

                    if ((Command[1] == 0) || (Command[1] >= 0x7F))
                    {
                        // Infinite loop
                        if ((Loops[LoopCount].Count < 0x80) && (PortNumber != 0xFF))
                            stream.WriteEvent(midi::StatusCode::ControlChange, 0x6F, (uint8_t) Loops[LoopCount].Count); // Set an RPG Maker Loop marker.

                        if (Loops[LoopCount].Count < track.MaxLoopExpansions)
                            TakeLoop = true;
                    }
                    else
                    {
                        if (Loops[LoopCount].Count < Command[1])
                            TakeLoop = true;
                    }

                    if (TakeLoop)
                    {
                        ::memcpy(Command, Loops[LoopCount].Command, sizeof(Command));

                        Offset = Loops[LoopCount++].Offset;
                    }

                    break;
                }

                case 0xF9: // Loop Begin
                {
                    if (LoopCount >= _countof(Loops))
                        break; // Too many nested loops

                    if ((Offset == track.LoopOffset) && (PortNumber != 0xFF))
                        stream.WriteEvent(midi::StatusCode::ControlChange, 0x6F, 0); // Set an RPG Maker Loop marker.

                    ::memcpy(Loops[LoopCount].Command, Command, sizeof(Command));

                    Loops[LoopCount].Offset = (uint32_t) Offset;
                    Loops[LoopCount].Count = 0;

                    if ((LoopCount > 0) && (Loops[LoopCount].Offset == Loops[LoopCount - 1].Offset))
                        LoopCount--; // Ignore bad loop command

                    LoopCount++;
                    break;
                }

                case 0xFE: // Track End
                case 0xFF:
                {
                    EndOfTrack = true;
                    break;
                }

                default:
                {
                    // Unknown MMD command
                    stream.WriteEvent(midi::StatusCode::ControlChange, 0x70, CommandCode & 0x7Fu);
                    break;
                }
            }
        }

        stream.DeltaTime += CommandDelay;
    }

    stream.EndTrack();

    if (PortNumber == 0xFF)
        stream.DeltaTime = 0;

    return true;
}

/// <summary>
/// 
/// </summary>
static size_t GetSysExSize(const uint8_t * data, size_t size, size_t startOffset) noexcept
{
    size_t Offset = startOffset;

    while ((Offset < size) && (data[Offset] != 0xF7))
        Offset++;

    if (Offset < size)
        Offset++; // Include terminating F7 byte

    return Offset - startOffset;
}

/// <summary>
/// 
/// </summary>
static void GetSysEx(const uint8_t * srcData, uint8_t param1, uint8_t param2, uint8_t channelNumber, std::vector<uint8_t> & dstData) noexcept
{
    if (srcData == nullptr)
        return;

    uint8_t Checksum = 0;

    for (size_t SrcOffset = 0; ; ++SrcOffset)
    {
        uint8_t Data = srcData[SrcOffset];

        if (Data & 0x80)
        {
            switch (Data)
            {
                case 0x80: // Store data value
                    Data = param1;
                    break;

                case 0x81: // Store data value
                    Data = param2;
                    break;

                case 0x82: // Store data value
                    Data = channelNumber;
                    break;

                case 0x83: // Initialize Roland checksum.
                    Checksum = 0x00;
                    break;

                case 0x84: // Store the Roland checksum.
                    Data = (uint8_t) ((0x100 - Checksum) & 0x7F);
                    break;

                case 0xF7: // SysEx end
                    dstData.push_back(Data) ;

                    return;

                default:
                    break; // Unknown MMD command
            }
        }

        if (!(Data & 0x80))
        {
            dstData.push_back(Data);
            Checksum += Data;
        }
    }
}

/// <summary>
/// Converts MMD tempo to MIDI tempo. (60 000 000.0 / bpm) * (scale / 64.0)
/// </summary>
static uint32_t MMDTempo2MIDITempo(uint16_t bpm, uint8_t scale) noexcept
{
    const uint32_t Denominator = (uint32_t) (bpm * scale);

    return (60000000u * 64u) / Denominator;
}

/// <summary>
/// Reads a little-endian 16-bit unsigned integer from the given data.
/// </summary>
static uint16_t ReadLE16(const uint8_t * data) noexcept
{
    return (uint16_t) ((data[0x01] << 8) | (data[0x00] << 0));
}

}
