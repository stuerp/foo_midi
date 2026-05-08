
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
    rcp_file_t RCPFile(_Options);

    RCPFile._Version = GetFileType(rcpData);

    if (RCPFile._Version >= 0x10)
        throw std::runtime_error("Unknown RCP file type");

#ifdef _RCP_VERBOSE
    ::printf("RCP version %u\n\n", RCPFile._Version);
#endif

    uint32_t Offset = 0;

    const uint8_t * RCPData = rcpData.Data;

    if (RCPFile._Version == 2)
    {
        RCPFile._Title.Assign(&RCPData[Offset + 0x020], 0x40);

        RCPFile._CommentSize = 28;

        RCPFile._Comments.Assign(&RCPData[Offset + 0x060], 12 * 28); // 12 lines, 28 characters

        RCPFile._TicksPerQuarter     = (uint16_t) ((RCPData[Offset + 0x1E7] << 8) | RCPData[Offset + 0x1C0]); // Hi- and lo-byte.
        RCPFile._Tempo               = RCPData[Offset + 0x1C1]; // In bpm
        RCPFile._BPMNumerator        = RCPData[Offset + 0x1C2]; // Time signature numerator (in bpm)
        RCPFile._BPMDenominator      = RCPData[Offset + 0x1C3]; // Time signature denominator (in bpm)
        RCPFile._KeySignature        = RCPData[Offset + 0x1C4];
        RCPFile._GlobalTransposition = (int8_t) RCPData[Offset + 0x1C5];

        RCPFile._CM6FileName .AssignSpecial(&RCPData[Offset + 0x1C6], 0x10);
        RCPFile._GSD1FileName.AssignSpecial(&RCPData[Offset + 0x1D6], 0x10);

        RCPFile._TrackCount =  RCPData[Offset + 0x1E6];         // 0 (RCP v0), 18 (RCP v1) or 36 (RCP v2)

        Offset += 0x1E8 + 0x0E + 0x10;
        Offset += 32 * (14 + 2); // Skip rhythm definitions (16 bytes each)
    }
    else
 // if (RCPFile._Version == 3)
    {
        RCPFile._Title.Assign(&RCPData[Offset + 0x020], 0x80);

        RCPFile._CommentSize = 30;

        RCPFile._Comments.Assign(&RCPData[Offset + 0x0A0], 12 * 30); // 12 lines, 30 characters

        RCPFile._TrackCount          = ReadLE16(&RCPData[Offset + 0x0208]);
        RCPFile._TicksPerQuarter     = ReadLE16(&RCPData[Offset + 0x020A]);
        RCPFile._Tempo               = ReadLE16(&RCPData[Offset + 0x020C]); // In bpm
        RCPFile._BPMNumerator        = RCPData[Offset + 0x020E];            // Time signature numerator (in bpm)
        RCPFile._BPMDenominator      = RCPData[Offset + 0x020F];            // Time signature denominator (in bpm)
        RCPFile._KeySignature        = RCPData[Offset + 0x0210];
        RCPFile._GlobalTransposition = (int8_t) RCPData[Offset + 0x0211];

        RCPFile._GSD1FileName.AssignSpecial(&RCPData[Offset + 0x298], 0x10);
        RCPFile._GSD2FileName.AssignSpecial(&RCPData[Offset + 0x2A8], 0x10);
        RCPFile._CM6FileName .AssignSpecial(&RCPData[Offset + 0x2B8], 0x10);

        Offset += 0x318;
        Offset += 128 * (14 + 2); // skip rhythm definitions (16 bytes each)
    }

    // Sanitize the values we read.
    if (RCPFile._TrackCount == 0)
        RCPFile._TrackCount = 18; // Early RCP files have the value set to 0 and alway assume 18 tracks.

    if (RCPFile._Tempo < 8 || RCPFile._Tempo > 250)
        RCPFile._Tempo = 120;

    if (RCPFile._KeySignature > 32)
        RCPFile._KeySignature = 0;

    if ((RCPFile._GlobalTransposition < -36) || (RCPFile._GlobalTransposition > 36))
        RCPFile._GlobalTransposition = 0;

    const uint32_t NameSize = 24;
    const uint32_t DataSize = 24;

    // Read the RCP tracks.
    std::vector<rcp_track_t> RCPTracks(RCPFile._TrackCount);

    {
        uint32_t TrackOffset = Offset + (8 * (NameSize + DataSize)); // Skip the User SysEx messages for now.
        uint16_t n = 0;

        for (auto & Track : RCPTracks)
        {
            if (TrackOffset >= rcpData.Size)
            {
            #ifdef _RCP_VERBOSE
                ::printf("%04X: Warning: Insufficient track data.\n", TrackOffset);
            #endif

                RCPFile._TrackCount = n;
                RCPTracks.resize(n);
                break;
            }

            RCPFile.ParseTrack(rcpData.Data, rcpData.Size, TrackOffset, &Track);

            Track.LoopCount = (uint16_t) ((Track.LoopStartOffs != 0) ? _Options._RCPLoopCount : 0);
            TrackOffset += Track.Size;

            ++n;
        }
    }

    if (_Options._ExtendLoops)
        BalanceTrackTimes(RCPTracks, (uint32_t) (RCPFile._TicksPerQuarter / 4), 0xFF);

    uint8_t ControlTrackCount = 0; // Number of tracks that contain control sequences.

    cm6_file_t CM6File;
    gsd_file_t GSD1File;
    gsd_file_t GSD2File;

    if (_Options._IncludeControlData)
    {
        wchar_t FilePath[260];

        ::wcscpy_s(FilePath, _countof(FilePath), _FilePath.c_str());

        wchar_t * FileName = (wchar_t *) GetFileName(FilePath); *FileName = '\0';

        char FileNameA[260];

        // Roland MT-32 / Roland CM-64 (Combines the CM-32L and CM-32P)
        if (RCPFile._CM6FileName.Len > 0)
        {
            ::memcpy(FileNameA, RCPFile._CM6FileName.Data, RCPFile._CM6FileName.Len), FileNameA[RCPFile._CM6FileName.Len] = '\0';

            ::wcscat_s(FilePath, _countof(FilePath), msc::UTF8ToWide(FileNameA).c_str());

            buffer_t CM6Data;

            try
            {
                CM6Data.ReadFile(FilePath);

                CM6File.Read(CM6Data);

                #ifdef _RCP_VERBOSE
                ::wprintf(L"Using CM6 %s control file \"%.*hs\".\n", (CM6File.DeviceType ? L"CM-64" : L"MT-32"), RCPFile._CM6FileName.Len, RCPFile._CM6FileName.Data);
                #endif
                ControlTrackCount++;
            }
            catch (std::exception & e)
            {
                throw std::runtime_error(std::string("Failed to load CM6 control file: ") + e.what());
            }
        }

        // Roland SC-55
        if (RCPFile._GSD1FileName.Len > 0)
        {
            ::memcpy(FileNameA, RCPFile._GSD1FileName.Data, RCPFile._GSD1FileName.Len), FileNameA[RCPFile._GSD1FileName.Len] = '\0';

            ::wcscat_s(FilePath, _countof(FilePath), msc::UTF8ToWide(FileNameA).c_str());

            buffer_t GSDData;

            try
            {
                GSDData.ReadFile(FilePath);

                GSD1File.Read(GSDData);

                #ifdef _RCP_VERBOSE
                ::printf("Using GSD control file \"%.*hs\".\n", RCPFile._GSD1FileName.Len, RCPFile._GSD1FileName.Data);
                #endif
                ControlTrackCount++;
            }
            catch (std::exception & e)
            {
                throw std::runtime_error(std::string("Failed to load GSD control file: ") + e.what());
            }
        }

        // Roland SC-55
        if (RCPFile._GSD2FileName.Len > 0)
        {
            ::memcpy(FileName, RCPFile._GSD2FileName.Data, RCPFile._GSD2FileName.Len), FileName[RCPFile._GSD2FileName.Len] = '\0';

            ::wcscat_s(FilePath, _countof(FilePath), msc::UTF8ToWide(FileNameA).c_str());

            buffer_t GSDData;

            try
            {
                GSDData.ReadFile(FilePath);

                GSD2File.Read(GSDData);

                #ifdef _RCP_VERBOSE
                ::printf("Using GSD control file \"%.*hs\" for port B.\n", RCPFile._GSD2FileName.Len, RCPFile._GSD2FileName.Data);
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
    MIDIStream.WriteMIDIHeader(1, (uint16_t) (1 + ControlTrackCount + RCPFile._TrackCount), RCPFile._TicksPerQuarter);

    uint8_t Temp[32];

    // Write the conductor track.
    {
        ::puts("Creating conductor track...");

        _MIDITickCount = 0;

        MIDIStream.BeginWriteMIDITrack();

        if (RCPFile._Title.Len > 0)
            MIDIStream.WriteMetaEvent(midi::TrackName, RCPFile._Title.Data, RCPFile._Title.Len);

        // Comments
        if (RCPFile._Comments.Len > 0)
        {
            // The comments section consists of 12 lines with 28 or 30 characters each.
            for (uint16_t i = 0; i < RCPFile._Comments.Len; i += RCPFile._CommentSize)
            {
                const char * Text = &RCPFile._Comments.Data[i];

                uint32_t Size = GetTrimmedLength(Text, RCPFile._CommentSize, ' ', false);

                if (Size == 0)
                    Size = 1; // Some sequencers remove empty events, so keep at least 1 space.

                MIDIStream.WriteMetaEvent(midi::Text, Text, Size);
            }
        }

        {
            uint32_t Tempo = BPM2Ticks(RCPFile._Tempo, 64);

            MIDIStream.SetTempo(Tempo);
            MIDIStream.WriteMetaEvent(midi::SetTempo, Tempo, 3);

            #ifdef _RCP_VERBOSE
            ::printf("Tempo: %u bpm, %u ticks.\n", RCPFile._Tempo, Tempo);
            #endif
        }

        if (RCPFile._BPMNumerator > 0) // time signature being 0/0 happened in AB_AFT32.RCP
        {
            RCP2MIDITimeSignature(RCPFile._BPMNumerator, RCPFile._BPMDenominator, Temp);

            MIDIStream.WriteMetaEvent(midi::TimeSignature, Temp, 4);

            #ifdef _RCP_VERBOSE
            ::printf("Time signature: %u/%u.\n", RCPFile._BPMNumerator, RCPFile._BPMDenominator);
            #endif
        }

        {
            RCP2MIDIKeySignature(RCPFile._KeySignature, Temp);

            MIDIStream.WriteMetaEvent(midi::KeySignature, Temp, 2);

            #ifdef _RCP_VERBOSE
            ::printf("Key signature: %u.\n", RCPFile._KeySignature);
            #endif
        }

        MIDIStream.WriteEvent(midi::MetaData, midi::EndOfTrack, 0);

        MIDIStream.EndWriteMIDITrack();
    }

    uint32_t RunningTime = 0;

    if (ControlTrackCount > 0)
    {
        if (RCPFile._CM6FileName.Len > 0)
        {
            _MIDITickCount = 0;

            MIDIStream.BeginWriteMIDITrack();

            MIDIStream.WriteMetaEvent(midi::TrackName, RCPFile._CM6FileName.Data, RCPFile._CM6FileName.Len);

            MIDIStream.WriteRolandSysEx(SysExHeaderMT32, 0x7F0000, nullptr, 0, 0); // MT-32 Reset

            // Add a delay of ~400 ms.
            {
                uint32_t Delay = MulDivRound(400, MIDIStream.GetTicksPerQuarter() * 1000, MIDIStream.GetTempo()); // (N ms / 1000 ms) / (tempoInTicks / 1 000 000)

                uint32_t Timestamp = MIDIStream.GetDuration() + Delay;

                MIDIStream.SetDuration(Timestamp);
            }

            Convert(CM6File, MIDIStream, 0x11);

            RunningTime += _MIDITickCount;

            MIDIStream.WriteEvent(midi::MetaData, midi::EndOfTrack, 0);

            MIDIStream.EndWriteMIDITrack();
        }

        if (RCPFile._GSD1FileName.Len > 0)
        {
            _MIDITickCount = 0;

            MIDIStream.BeginWriteMIDITrack();

            MIDIStream.WriteMetaEvent(midi::TrackName, RCPFile._GSD1FileName.Data, RCPFile._GSD1FileName.Len);

            if (RCPFile._GSD2FileName.Len > 0)
            {
                Temp[0] = 0x00; // Port A
                MIDIStream.WriteMetaEvent(midi::MIDIPort, Temp, 1);
            }

            Convert(GSD1File, MIDIStream, 0x11);

            RunningTime += _MIDITickCount;

            MIDIStream.WriteEvent(midi::MetaData, midi::EndOfTrack, 0);

            MIDIStream.EndWriteMIDITrack();
        }

        if (RCPFile._GSD2FileName.Len > 0)
        {
            _MIDITickCount = 0;

            MIDIStream.BeginWriteMIDITrack();

            MIDIStream.WriteMetaEvent(midi::TrackName, RCPFile._GSD2FileName.Data, RCPFile._GSD2FileName.Len);

            Temp[0] = 0x01; // Port B
            MIDIStream.WriteMetaEvent(midi::MIDIPort, Temp, 1);

            Convert(GSD2File, MIDIStream, 0x11);

            RunningTime += _MIDITickCount;

            MIDIStream.WriteEvent(midi::MetaData, midi::EndOfTrack, 0);

            MIDIStream.EndWriteMIDITrack();
        }
    }

    // Read the User SysEx messages.
    {
        for (auto & SysEx : RCPFile._SysEx)
        {
            const uint32_t SyxOffset = Offset;

            SysEx.Name.Assign(RCPData + Offset, NameSize);

            Offset += NameSize;

            SysEx.Data = RCPData + Offset; // // Without leading 0xF0, padded with 0xF7
            SysEx.Size = GetTrimmedLength((const char *) SysEx.Data, DataSize, (char) (std::byte) 0xF7, true);

            Offset += DataSize;

        #ifdef _RCP_VERBOSE
            ::printf("%04X: User SysEx \"%s\",", SyxOffset, msc::TextToUTF8(SysEx.Name.Data, SysEx.Name.Len).c_str());

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

        if (RCPFile._BPMNumerator == 0 || RCPFile._BPMDenominator == 0)
            TicksPerBar = MIDIStream.GetTicksPerQuarter() * 4; // Assume 4/4 time signature.
        else
            TicksPerBar = RCPFile._BPMNumerator * (MIDIStream.GetTicksPerQuarter() * 4) / RCPFile._BPMDenominator;

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
            RCPFile.ConvertTrack(rcpData.Data, rcpData.Size, &Offset, &RCPTrack, MIDIStream);
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
    MIDIStream.WriteMIDIHeader(1, (uint16_t) (1 + ControlTrackCount + RCPFile._TrackCount), RCPFile._TicksPerQuarter);

    ::puts("Done.");
}

/// <summary>
/// Converts an RCP control file.
/// </summary>
void converter_t::ConvertControl(const buffer_t & ctrlData, buffer_t & midiData, uint8_t fileType, uint8_t outMode)
{
    cm6_file_t CM6File;
    gsd_file_t GSDFile;

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
uint16_t converter_t::BalanceTrackTimes(std::vector<rcp_track_t> & rcpTracks, uint32_t minLoopTicks, uint8_t verbose)
{
    uint32_t maxTicks = 0;

    for (auto & RCPTrack : rcpTracks)
    {
        uint32_t Duration = 0;

        if (RCPTrack.LoopCount != 0)
        {
            uint32_t LoopTicks = (RCPTrack.Duration - RCPTrack.LoopStartTick);

            Duration = RCPTrack.Duration + LoopTicks * (RCPTrack.LoopCount - 1);
        }
        else
            Duration = RCPTrack.Duration;

        if (maxTicks < Duration)
            maxTicks = Duration;
    }

    uint16_t adjustCnt = 0;
    uint32_t i = 0;

    for (auto & RCPTrack : rcpTracks)
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
