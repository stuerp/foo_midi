
/** $VER: RCPConverter.cpp (2025.06.09) P. Stuer - Based on Valley Bell's rpc2mid (https://github.com/ValleyBell/MidiConverters). **/

#include "pch.h"

#include "RCP.h"
#include "RunningNotes.h"

namespace rcp
{

uint32_t _MIDITickCount = 0;

running_notes_t RunningNotes;

/// <summary>
/// Converts the RCP data.
/// </summary>
void converter_t::ToSMF(const buffer_t & srcData, buffer_t & dstData, const std::wstring & dstType)
{
    uint8_t FileType = converter_t::GetFileType(srcData);

    if (FileType < 0x10)
    {
        try
        {
            ConvertSequence(srcData, dstData);
        }
        catch (std::exception & e)
        {
            throw std::runtime_error(std::string("Recomposer sequence file conversion failed: ") + e.what());
        }
    }
    else
    if (FileType < 0x20)
    {
        uint8_t Mode;

        if (::_wcsicmp(dstType.c_str(), L"syx") == 0)
            Mode = 0x00;
        else
        if (::_wcsicmp(dstType.c_str(), L"mid") == 0)
            Mode = 0x01;
        else
            throw std::runtime_error(std::string("Unknown output format: ") + msc::WideToUTF8(dstType));

        try
        {
            ConvertControl(srcData, dstData, FileType, Mode);
        }
        catch (std::exception & e)
        {
            throw std::runtime_error(std::string("Recomposer control file conversion failed: ") + e.what());
        }
    }
    else
        throw std::runtime_error("Unknown RCP file type");
}

/// <summary>
/// Converts an RCP sequence.
/// </summary>
void converter_t::ConvertSequence(const buffer_t & rcpData, buffer_t & midData)
{
    rcp_t RCP(_Options);

    RCP._Version = GetFileType(rcpData);

    if (RCP._Version >= 0x10)
        throw std::runtime_error("Unknown RCP file type");

#ifdef _RCP_VERBOSE
    ::printf("RCP version %u\n\n", RCP._Version);
#endif

    size_t Offset = 0;

    const uint8_t * RCPData = rcpData.Data;

    if (RCP._Version == 2)
    {
        RCP._Title.Assign(&RCPData[Offset + 0x020], 0x40);

        RCP._CommentSize = 28;

        RCP._Comments.Assign(&RCPData[Offset + 0x060], 12 * 28); // 12 lines, 28 characters

        RCP._TicksPerQuarter     = (uint16_t) ((RCPData[Offset + 0x1E7] << 8) | RCPData[Offset + 0x1C0]); // Hi- and lo-byte.
        RCP._Tempo               = RCPData[Offset + 0x1C1]; // In bpm
        RCP._BPMNumerator        = RCPData[Offset + 0x1C2]; // Time signature numerator (in bpm)
        RCP._BPMDenominator      = RCPData[Offset + 0x1C3]; // Time signature denominator (in bpm)
        RCP._KeySignature        = RCPData[Offset + 0x1C4];
        RCP._GlobalTransposition = (int8_t) RCPData[Offset + 0x1C5];

        RCP._CM6FileName .AssignSpecial(&RCPData[Offset + 0x1C6], 0x10);
        RCP._GSD1FileName.AssignSpecial(&RCPData[Offset + 0x1D6], 0x10);

        RCP._TrackCount =  RCPData[Offset + 0x1E6];         // 0 (RCP v0), 18 (RCP v1) or 36 (RCP v2)

        Offset += 0x1E8 + 0x0E + 0x10;
        Offset += 32 * (14 + 2); // Skip rhythm definitions (16 bytes each)
    }
    else
    {
        RCP._Title.Assign(&RCPData[Offset + 0x020], 0x80);

        RCP._CommentSize = 30;

        RCP._Comments.Assign(&RCPData[Offset + 0x0A0], 12 * 30); // 12 lines, 30 characters

        RCP._TrackCount          = ReadLE16(&RCPData[Offset + 0x0208]);
        RCP._TicksPerQuarter     = ReadLE16(&RCPData[Offset + 0x020A]);
        RCP._Tempo               = ReadLE16(&RCPData[Offset + 0x020C]); // In bpm
        RCP._BPMNumerator        = RCPData[Offset + 0x020E];            // Time signature numerator (in bpm)
        RCP._BPMDenominator      = RCPData[Offset + 0x020F];            // Time signature denominator (in bpm)
        RCP._KeySignature        = RCPData[Offset + 0x0210];
        RCP._GlobalTransposition = (int8_t) RCPData[Offset + 0x0211];

        RCP._GSD1FileName.AssignSpecial(&RCPData[Offset + 0x298], 0x10);
        RCP._GSD2FileName.AssignSpecial(&RCPData[Offset + 0x2A8], 0x10);
        RCP._CM6FileName .AssignSpecial(&RCPData[Offset + 0x2B8], 0x10);

        Offset += 0x318;
        Offset += 128 * (14 + 2); // skip rhythm definitions (16 bytes each)
    }

    // Sanitize the values we read.
    if (RCP._TrackCount == 0)
        RCP._TrackCount = 18; // Early RCP files have the value set to 0 and alway assume 18 tracks.

    if (RCP._Tempo < 8 || RCP._Tempo > 250)
        RCP._Tempo = 120;

    if (RCP._KeySignature > 32)
        RCP._KeySignature = 0;

    if ((RCP._GlobalTransposition < -36) || (RCP._GlobalTransposition > 36))
        RCP._GlobalTransposition = 0;

    const uint32_t NameSize = 24;
    const uint32_t DataSize = 24;

    // Read the RCP tracks.
    std::vector<track_t> RCPTracks(RCP._TrackCount);

    {
        size_t TrackOffset = Offset + (8 * (NameSize + DataSize)); // Skip the User SysEx messages for now.
        uint16_t n = 0;

        for (auto & Track : RCPTracks)
        {
            if (TrackOffset >= rcpData.Size)
            {
            #ifdef _RCP_VERBOSE
                ::printf("%04X: Warning: Insufficient track data.\n", (int) TrackOffset);
            #endif

                RCP._TrackCount = n;
                RCPTracks.resize(n);
                break;
            }

            RCP.ParseTrack(rcpData.Data, rcpData.Size, TrackOffset, &Track);

            Track.LoopCount = (uint16_t) ((Track.LoopStartOffs != 0) ? _Options.RCPLoopExpansion : 0);
            TrackOffset += Track.Size;

            ++n;
        }
    }

    if (_Options.ExpandLoops)
        BalanceTrackTimes(RCPTracks, (uint32_t) (RCP._TicksPerQuarter / 4), 0xFF);

    uint8_t ControlTrackCount = 0; // Number of tracks that contain control sequences.

    cm6_t CM6;
    gsd_t GSD1;
    gsd_t GSD2;

    if (_Options.IncludeControlData)
    {
        wchar_t FilePath[260];

        ::wcscpy_s(FilePath, _countof(FilePath), _FilePath.c_str());

        wchar_t * FileName = (wchar_t *) GetFileName(FilePath); *FileName = '\0';

        char FileNameA[260];

        // Roland MT-32 / Roland CM-64 (Combines the CM-32L and CM-32P)
        if (RCP._CM6FileName.Len > 0)
        {
            ::memcpy(FileNameA, RCP._CM6FileName.Data, RCP._CM6FileName.Len), FileNameA[RCP._CM6FileName.Len] = '\0';

            ::wcscat_s(FilePath, _countof(FilePath), msc::UTF8ToWide(FileNameA).c_str());

            buffer_t CM6Data;

            try
            {
                CM6Data.ReadFile(FilePath);

                CM6.Read(CM6Data);

                #ifdef _RCP_VERBOSE
                ::wprintf(L"Using CM6 %s control file \"%.*hs\".\n", (CM6.DeviceType ? L"CM-64" : L"MT-32"), RCP._CM6FileName.Len, RCP._CM6FileName.Data);
                #endif
                ControlTrackCount++;
            }
            catch (std::exception & e)
            {
                throw std::runtime_error(std::string("Failed to load CM6 control file: ") + e.what());
            }
        }

        // Roland SC-55
        if (RCP._GSD1FileName.Len > 0)
        {
            ::memcpy(FileNameA, RCP._GSD1FileName.Data, RCP._GSD1FileName.Len), FileNameA[RCP._GSD1FileName.Len] = '\0';

            ::wcscat_s(FilePath, _countof(FilePath), msc::UTF8ToWide(FileNameA).c_str());

            buffer_t GSDData;

            try
            {
                GSDData.ReadFile(FilePath);

                GSD1.Read(GSDData);

                #ifdef _RCP_VERBOSE
                ::printf("Using GSD control file \"%.*hs\".\n", RCP._GSD1FileName.Len, RCP._GSD1FileName.Data);
                #endif
                ControlTrackCount++;
            }
            catch (std::exception & e)
            {
                throw std::runtime_error(std::string("Failed to load GSD control file: ") + e.what());
            }
        }

        // Roland SC-55
        if (RCP._GSD2FileName.Len > 0)
        {
            ::memcpy(FileName, RCP._GSD2FileName.Data, RCP._GSD2FileName.Len), FileName[RCP._GSD2FileName.Len] = '\0';

            ::wcscat_s(FilePath, _countof(FilePath), msc::UTF8ToWide(FileNameA).c_str());

            buffer_t GSDData;

            try
            {
                GSDData.ReadFile(FilePath);

                GSD2.Read(GSDData);

                #ifdef _RCP_VERBOSE
                ::printf("Using GSD control file \"%.*hs\" for port B.\n", RCP._GSD2FileName.Len, RCP._GSD2FileName.Data);
                #endif
                ControlTrackCount++;
            }
            catch (std::exception & e)
            {
                throw std::runtime_error(std::string("Failed to load GSD control file: ") + e.what());
            }
        }
    }

    #ifdef _RCP_VERBOSE
    ::puts("Converting...");
    #endif

    midi_stream_t MIDIStream(0x20000);

    MIDIStream.SetDurationHandler(HandleDuration);

    // Write the MIDI header.
    MIDIStream.WriteMIDIHeader(1, (uint16_t) (1 + ControlTrackCount + RCP._TrackCount), RCP._TicksPerQuarter);

    uint8_t Temp[32];

    // Write the conductor track.
    {
        ::puts("Creating conductor track...");

        _MIDITickCount = 0;

        MIDIStream.BeginWriteMIDITrack();

        if (RCP._Title.Len > 0)
            MIDIStream.WriteMetaEvent(midi::TrackName, RCP._Title.Data, RCP._Title.Len);

        // Comments
        if (RCP._Comments.Len > 0)
        {
            // The comments section consists of 12 lines with 28 or 30 characters each.
            for (uint16_t i = 0; i < RCP._Comments.Len; i += RCP._CommentSize)
            {
                const char * Text = &RCP._Comments.Data[i];

                uint32_t Size = GetTrimmedLength(Text, RCP._CommentSize, ' ', false);

                if (Size == 0)
                    Size = 1; // Some sequencers remove empty events, so keep at least 1 space.

                MIDIStream.WriteMetaEvent(midi::Text, Text, Size);
            }
        }

        {
            uint32_t Tempo = BPM2Ticks(RCP._Tempo, 64);

            MIDIStream.SetTempo(Tempo);
            MIDIStream.WriteMetaEvent(midi::SetTempo, Tempo, 3);

            #ifdef _RCP_VERBOSE
            ::printf("Tempo: %u bpm, %u ticks.\n", RCP._Tempo, Tempo);
            #endif
        }

        if (RCP._BPMNumerator > 0) // time signature being 0/0 happened in AB_AFT32.RCP
        {
            RCP2MIDITimeSignature(RCP._BPMNumerator, RCP._BPMDenominator, Temp);

            MIDIStream.WriteMetaEvent(midi::TimeSignature, Temp, 4);

            #ifdef _RCP_VERBOSE
            ::printf("Time signature: %u/%u.\n", RCP._BPMNumerator, RCP._BPMDenominator);
            #endif
        }

        {
            RCP2MIDIKeySignature(RCP._KeySignature, Temp);

            MIDIStream.WriteMetaEvent(midi::KeySignature, Temp, 2);

            #ifdef _RCP_VERBOSE
            ::printf("Key signature: %u.\n", RCP._KeySignature);
            #endif
        }

        MIDIStream.WriteEvent(midi::MetaData, midi::EndOfTrack, 0);

        MIDIStream.EndWriteMIDITrack();
    }

    uint32_t RunningTime = 0;

    if (ControlTrackCount > 0)
    {
        if (RCP._CM6FileName.Len > 0)
        {
            _MIDITickCount = 0;

            MIDIStream.BeginWriteMIDITrack();

            MIDIStream.WriteMetaEvent(midi::TrackName, RCP._CM6FileName.Data, RCP._CM6FileName.Len);

            MIDIStream.WriteRolandSysEx(SysExHeaderMT32, 0x7F0000, nullptr, 0, 0); // MT-32 Reset

            // Add a delay of ~400 ms.
            {
                uint32_t Delay = MulDivRound(400, MIDIStream.GetTicksPerQuarter() * 1000, MIDIStream.GetTempo()); // (N ms / 1,000 ms) / (tempoInTicks / 1,000,000)

                uint32_t Timestamp = MIDIStream.GetDuration() + Delay;

                MIDIStream.SetDuration(Timestamp);
            }

            Convert(CM6, MIDIStream, 0x11);

            RunningTime += _MIDITickCount;

            MIDIStream.WriteEvent(midi::MetaData, midi::EndOfTrack, 0);

            MIDIStream.EndWriteMIDITrack();
        }

        if (RCP._GSD1FileName.Len > 0)
        {
            _MIDITickCount = 0;

            MIDIStream.BeginWriteMIDITrack();

            MIDIStream.WriteMetaEvent(midi::TrackName, RCP._GSD1FileName.Data, RCP._GSD1FileName.Len);

            if (RCP._GSD2FileName.Len > 0)
            {
                Temp[0] = 0x00; // Port A
                MIDIStream.WriteMetaEvent(midi::MIDIPort, Temp, 1);
            }

            Convert(GSD1, MIDIStream, 0x11);

            RunningTime += _MIDITickCount;

            MIDIStream.WriteEvent(midi::MetaData, midi::EndOfTrack, 0);

            MIDIStream.EndWriteMIDITrack();
        }

        if (RCP._GSD2FileName.Len > 0)
        {
            _MIDITickCount = 0;

            MIDIStream.BeginWriteMIDITrack();

            MIDIStream.WriteMetaEvent(midi::TrackName, RCP._GSD2FileName.Data, RCP._GSD2FileName.Len);

            Temp[0] = 0x01; // Port B
            MIDIStream.WriteMetaEvent(midi::MIDIPort, Temp, 1);

            Convert(GSD2, MIDIStream, 0x11);

            RunningTime += _MIDITickCount;

            MIDIStream.WriteEvent(midi::MetaData, midi::EndOfTrack, 0);

            MIDIStream.EndWriteMIDITrack();
        }
    }

    // Read the User SysEx messages.
    {
        for (auto & SysEx : RCP._SysEx)
        {
            const size_t SyxOffset = Offset;

            SysEx.Name.Assign(RCPData + Offset, NameSize);

            Offset += NameSize;

            SysEx.Data = RCPData + Offset; // // Without leading 0xF0, padded with 0xF7
            SysEx.Size = GetTrimmedLength((const char *) SysEx.Data, DataSize, (char) (std::byte) 0xF7, true);

            Offset += DataSize;

        #ifdef _RCP_VERBOSE
            ::printf("%04X: User SysEx \"%s\",", (int) SyxOffset, msc::TextToUTF8(SysEx.Name.Data, SysEx.Name.Len).c_str());

            if (SysEx.Size != 0)
            {
                for (size_t i = 0; i < SysEx.Size; ++i)
                    ::printf(" %02X", SysEx.Data[i]);

                ::putchar('\n');
            }
            else
                ::puts(" Empty");
        #endif
        }
    }

    if (RunningTime > 0)
    {
        uint32_t TicksPerBar;

        if (RCP._BPMNumerator == 0 || RCP._BPMDenominator == 0)
            TicksPerBar = MIDIStream.GetTicksPerQuarter() * 4; // Assume 4/4 time signature.
        else
            TicksPerBar = RCP._BPMNumerator * (MIDIStream.GetTicksPerQuarter() * 4) / RCP._BPMDenominator;

        // Round the initial timestamp up to a full bar.
        RunningTime = (RunningTime + TicksPerBar - 1) / TicksPerBar * TicksPerBar;

    #ifdef _RCP_VERBOSE
        ::printf("Initial timestamp: %u ticks (%u bars)\n", RunningTime, RunningTime / TicksPerBar);
    #endif
    }
    #ifdef _RCP_VERBOSE
    else
        ::printf("Initial timestamp: %u ticks\n", RunningTime);
    #endif

    for (auto & RCPTrack : RCPTracks)
    {
        _MIDITickCount = 0;

        MIDIStream.BeginWriteMIDITrack();

        MIDIStream.SetDuration(RunningTime);

        try
        {
            RCP.ConvertTrack(rcpData.Data, rcpData.Size, Offset, &RCPTrack, MIDIStream);
        }
        catch (std::exception &)
        {
        // Assume that early EOF is not an error.
        #ifdef _RCP_VERBOSE
            ::puts("Warning: Early end-of-track.");
        #endif
        }

        MIDIStream.WriteEvent(midi::MetaData, midi::EndOfTrack, 0);

        MIDIStream.EndWriteMIDITrack();
    }

    midData.Copy(MIDIStream.GetData(), MIDIStream.GetOffs());

    MIDIStream.Reset();
    MIDIStream.WriteMIDIHeader(1, (uint16_t) (1 + ControlTrackCount + RCP._TrackCount), RCP._TicksPerQuarter);

    ::puts("Done.");
}

/// <summary>
/// Converts an RCP control file.
/// </summary>
void converter_t::ConvertControl(const buffer_t & ctrlData, buffer_t & midiData, uint8_t fileType, uint8_t outMode)
{
    cm6_t CM6File;
    gsd_t GSDFile;

    if (fileType == 0x10)
    {
        CM6File.Read(ctrlData);

    #ifdef _RCP_VERBOSE
        ::printf("CM6 control file, %s mode.\n", CM6File.DeviceType ? "CM-64" : "MT-32");
    #endif
    }
    else
    if (fileType == 0x11)
    {
        GSDFile.Read(ctrlData);

    #ifdef _RCP_VERBOSE
        ::printf("GSD control file\n");
    #endif
    }
    else
        throw std::runtime_error("Unknown file type");

    midi_stream_t MIDIFile(0x10000);

    if (outMode & 0x01) // MIDI mode
    {
        _MIDITickCount = 0;

        MIDIFile.WriteMIDIHeader(1, 1, 48);

        MIDIFile.BeginWriteMIDITrack();

        if (fileType == 0x10)
            Convert(CM6File, MIDIFile, outMode);
        else
            Convert(GSDFile, MIDIFile, outMode);

        MIDIFile.WriteEvent(midi::MetaData, midi::EndOfTrack, 0);

        MIDIFile.EndWriteMIDITrack();
    }
    else
    {
        if (fileType == 0x10)
            Convert(CM6File, MIDIFile, outMode);
        else
            Convert(GSDFile, MIDIFile, outMode);
    }

    midiData.Copy(MIDIFile.GetData(), MIDIFile.GetOffs());
}

/// <summary>
/// Adjusts the value of trkInf->loopTimes so that all tracks play for the approximately same time.
/// When a track's loop has less ticks than minLoopTicks, then it is ignored.
/// Returns the number of adjusted tracks.
/// </summary>
uint16_t converter_t::BalanceTrackTimes(std::vector<track_t> & tracks, uint32_t minLoopTicks, uint8_t verbose)
{
    uint32_t maxTicks = 0;

    for (auto & Track : tracks)
    {
        uint32_t Duration = 0;

        if (Track.LoopCount != 0)
        {
            uint32_t LoopTicks = (Track.Duration - Track.LoopStartTick);

            Duration = Track.Duration + LoopTicks * (Track.LoopCount - 1);
        }
        else
            Duration = Track.Duration;

        if (maxTicks < Duration)
            maxTicks = Duration;
    }

    uint16_t adjustCnt = 0;
    uint32_t i = 0;

    for (auto & RCPTrack : tracks)
    {
        uint32_t Duration = 0;

        uint32_t LoopTicks = RCPTrack.LoopCount ? (RCPTrack.Duration - RCPTrack.LoopStartTick) : 0;

        // Ignore tracks with very short loops.
        if (LoopTicks < minLoopTicks)
        {
            #ifdef _RCP_VERBOSE
            if (LoopTicks > 0 && (verbose & 0x02))
                ::printf("Track %u: Ignoring micro-loop (%u ticks).\n", i, LoopTicks);
            #endif
            continue;
        }

        // Heuristic: The track needs additional loops, if the longest track is longer than the current track + 1/4 loop.
        Duration = RCPTrack.Duration + LoopTicks * (RCPTrack.LoopCount - 1);

        if (Duration + LoopTicks / 4 < maxTicks)
        {
            Duration = maxTicks - RCPTrack.LoopStartTick; // desired length of the loop
            RCPTrack.LoopCount = (uint16_t)((Duration + LoopTicks / 3) / LoopTicks);
            adjustCnt++;

            #ifdef _RCP_VERBOSE
            if (verbose & 0x01)
                ::printf("Track %u: Extended loop to %u times.\n", i, RCPTrack.LoopCount);
            #endif
        }

        ++i;
    }

    return adjustCnt;
}

/// <summary>
/// Gets the file type of the data.
/// </summary>
uint8_t converter_t::GetFileType(const buffer_t & rcpData) noexcept
{
    const char * Magic = (const char *) rcpData.Data;

    if (rcpData.Size < 0x20)
        return 0xFE; // Incomplete

    if (::strcmp(Magic, "RCM-PC98V2.0(C)COME ON MUSIC\r\n") == 0)
        return 0x02;
    else
    if (:: strcmp(Magic, "COME ON MUSIC RECOMPOSER RCP3.0") == 0)
        return 0x03;

    if (::strcmp(Magic, "COME ON MUSIC") == 0)
    {
        if (::memcmp(&Magic[0x0E], "\0\0R ", 0x04) == 0)
            return 0x10; // CM6 file
        else
        if (:: strcmp(&Magic[0x0E], "GS CONTROL 1.0") == 0)
            return 0x11; // GSD file
    }

    return 0xFF; // Unknown
}

/// <summary>
/// 
/// </summary>
uint8_t converter_t::HandleDuration(midi_stream_t * midiStream, uint32_t & duration)
{
    _MIDITickCount += duration;

    RunningNotes.Update(*midiStream, duration);

    if (duration != 0)
    {
        for (uint16_t i = 0; i < RunningNotes._Count; ++i)
        {
            assert(RunningNotes._Notes[i].Duration > duration);

            RunningNotes._Notes[i].Duration -= duration;
        }
    }

    return 0;
}

}
