
/** $VER: RCP.h (2026.11.25) P. Stuer - Based on Valley Bell's rpc2mid (https://github.com/ValleyBell/MidiConverters). **/

#pragma once

#include "pch.h"

#include "MIDIStream.h"
#include "Support.h"

namespace rcp
{

class options_t
{
public:
    uint16_t RCPLoopExpansion = 2;

    bool WriteBarMarkers = false;
    bool WriteSysExNames = false;
    bool ExpandLoops = true;
    bool WolfteamLoopMode = false;
    bool KeepMutedChannels = false;
    bool IncludeControlData = true;
};

class string_t
{
public:
    string_t() = default;

    uint32_t Assign(const uint8_t * data, uint16_t size)
    {
        Data = (const char *) data;
        Size = size;

        Len = GetTrimmedLength(Data, Size, ' ', false);

        return size;
    }

    uint32_t AssignSpecial(const uint8_t * data, uint16_t size)
    {
        Data = (const char *) data;
        Size = size;

        // special trimming used for file names
        const char * Tail = (const char *) ::memchr(Data, '\0', Size);

        Len = (Tail != nullptr) ? (uint16_t) (Tail - Data) : Size; // Stop at first '\0'.

        Len = GetTrimmedLength(Data, Len, ' ', false); // Trim spaces.

        return size;
    }

public:
    const char * Data = nullptr;
    uint16_t Size = 0;
    uint16_t Len = 0; // Length after trimming
};

class sysex_t
{
public:
    string_t Name;
    const uint8_t * Data = nullptr;
    uint32_t Size = 0;
};

class track_t
{
public:
    size_t Offset;
    uint32_t Size;
    uint32_t Duration;      // In ticks

    size_t LoopStartOffs;   // Offset of the start of the loop
    uint32_t LoopStartTick; // Tick of the start of the loop
    uint16_t LoopCount;     // Number of loops
};

class rcp_t
{
public:
    rcp_t(const options_t & options) : _Options(options) { }

    rcp_t(const rcp_t &) = delete;
    rcp_t(const rcp_t &&) = delete;

    rcp_t & operator=(const rcp_t &) = delete;
    rcp_t & operator=(rcp_t &&) = delete;

    void ParseTrack(const uint8_t * data, uint32_t size, size_t offset, track_t * track) const;
    void ConvertTrack(const uint8_t * data, uint32_t size, size_t & offset, track_t * track, midi_stream_t & midiStream) const;

private:
    uint16_t GetCommandSize(const uint8_t * data, uint32_t size, size_t offset, uint8_t flags) const noexcept;
    uint16_t GetCommand(const uint8_t * srcData, uint32_t srcSize, size_t & srcOffset, uint8_t * dstData, uint32_t dstSize, uint8_t flags) const noexcept;

public:
    uint8_t _Version            = 0;
    uint16_t _TrackCount        = 0;
    uint16_t _TicksPerQuarter   = 0;
    uint16_t _Tempo             = 0;
    uint8_t _BPMNumerator       = 0;
    uint8_t _BPMDenominator     = 0;
    uint8_t _KeySignature       = 0;
    int8_t _GlobalTransposition = 0;
    uint16_t _CommentSize       = 0;

    string_t _Title;
    string_t _Comments;
    string_t _CM6FileName;
    string_t _GSD1FileName;
    string_t _GSD2FileName;

    sysex_t _SysEx[8] = { };

private:
    const options_t & _Options;
};

class cm6_t
{
public:
    cm6_t() = default;

    void Read(const buffer_t & cm6Data);

public:
    uint8_t DeviceType = 0; // 0 - MT-32, 3 - CM-64
    string_t Comment;

    // MT-32 (LA) data
    const uint8_t * laSystem        = nullptr;
    const uint8_t * laChnVol        = nullptr;
    const uint8_t * laPatchTemp     = nullptr;
    const uint8_t * laRhythmTemp    = nullptr;
    const uint8_t * laTimbreTemp    = nullptr;
    const uint8_t * laPatchMem      = nullptr;
    const uint8_t * laTimbreMem     = nullptr;

    // CM-32P (PCM) data
    const uint8_t * pcmPatchTemp    = nullptr;
    const uint8_t * pcmPatchMem     = nullptr;
    const uint8_t * pcmSystem       = nullptr;
    const uint8_t * pcmChnVol       = nullptr;

private:
    buffer_t _Data;
};

class gsd_t
{
public:
    gsd_t() = default;

    void Read(const buffer_t & gsdData);

public:
    const uint8_t * sysParams       = nullptr;
    const uint8_t * reverbParams    = nullptr;
    const uint8_t * chorusParams    = nullptr;
    const uint8_t * partParams      = nullptr;
    const uint8_t * drumSetup       = nullptr;
    const uint8_t * masterTune      = nullptr;

private:
    buffer_t _Data;
};

class converter_t
{
public:
    void SetFilePath(const std::wstring & filePath) noexcept { _FilePath = filePath; }

    void ToSMF(const buffer_t & srcData, buffer_t & dstData, const std::wstring & dstType = L"mid");

    void ConvertSequence(const buffer_t & rcpData, buffer_t & midData);
    void ConvertControl(const buffer_t & rcpData, buffer_t & midData, uint8_t fileType, uint8_t outMode);

    void Convert(const cm6_t & cm6File, midi_stream_t & midiStream, uint8_t mode);
    void Convert(const gsd_t & gsdFile, midi_stream_t & midiStream, uint8_t mode);

    static uint8_t GetFileType(const buffer_t & rcpData) noexcept;

private:
    static uint16_t BalanceTrackTimes(std::vector<track_t> & rcpTracks, uint32_t minLoopTicks, uint8_t verbose);
    static uint8_t HandleDuration(midi_stream_t * fileInfo, uint32_t & duration);

public:
    options_t _Options;

    std::wstring _FilePath;
};

const uint8_t SysExHeaderMT32[] = { 0x41, 0x10, 0x16, 0x12 };
const uint8_t SysExHeaderSC55[] = { 0x41, 0x10, 0x42, 0x12 };
const uint8_t MT32PatchChange[] = { 0xFF, 0xFF, 0x18, 0x32, 0x0C, 0x00, 0x01 };

void RCP2MIDITimeSignature(uint8_t numerator, uint8_t denominator, uint8_t buffer[4]);
void RCP2MIDIKeySignature(uint8_t keySignature, uint8_t buffer[2]);

}
