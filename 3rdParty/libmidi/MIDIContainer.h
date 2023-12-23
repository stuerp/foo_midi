
/** $VER: MIDIContainer.h (2023.10.31) **/

#pragma once

#pragma warning(disable: 4514) // Unreferenced inline function has been removed
#pragma warning(disable: 5045) // Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified

#include <stdint.h>

#include <string>
#include <vector>

#include "Range.h"

enum StatusCodes
{
    NoteOff = 0x80,
    NoteOn = 0x90,

    PolyphonicAftertouch = 0xA0,
    ControlChange = 0xB0,
    ProgramChange = 0xC0,
    ChannelAftertouch = 0xD0,
    PitchBendChange = 0xE0,

    SysEx = 0xF0,
    MIDITimeCodeQtrFrame = 0xF1,
    SongPositionPointer = 0xF2,
    SongSelect = 0xF3,

    TuneRequest = 0xF6,
    SysExEnd = 0xF7,
    TimingClock = 0xF8,

    Start = 0xFA,
    Continue = 0xFB,
    Stop = 0xFC,

    ActiveSensing = 0xFE,
    MetaData = 0xFF
};

enum ControlChangeNumbers
{
    BankSelect = 0x00,

    LSB = 0x20,                 // LSB for Control Changes 0 to 31
};

enum MetaDataTypes
{
    SequenceNumber = 0x00,      // Sequence number in type 0 and 1 MIDI files; pattern number in type 2 MIDI files. (0..65535, default 0, occurs at delta time 0)
    Text = 0x01,                // General "Text" Meta Message. Can be used for any text based data. (string)
    Copyright = 0x02,           // Provides information about a MIDI file’s copyright. (string, occurs at delta time 0 in the first track)
    TrackName = 0x03,           // Track name (string, occurs at delta time 0)
    InstrumentName = 0x04,      // Instrument name (string)
    Lyrics = 0x05,              // Stores the lyrics of a song. Typically one syllable per Meta Message. (string)
    Marker = 0x06,              // Marks a point of interest in a MIDI file. Can be used as the marker for the beginning of a verse, solo, etc. (string)
    CueMarker = 0x07,           // Marks a cue. IE: ‘Cue performer 1’, etc (string)

    DeviceName = 0x09,          // Gives the name of the device. (string)

    ChannelPrefix = 0x20,       // Gives the prefix for the channel on which events are played. (0..255, default 0)
    MIDIPort = 0x21,            // Gives the MIDI Port on which events are played. (0..255, default 0)

    EndOfTrack = 0x2F,          // An empty Meta Message that marks the end of a track. Occurs at the end of each track.

    SetTempo = 0x51,            // Tempo is in microseconds per beat (quarter note). (0..16777215, default 500000). Occurs anywhere but usually in the first track.

    SMPTEOffset = 0x54,         // SMPTE time to denote playback offset from the beginning. Occurs at the beginning of a track and in the first track of files with MIDI format type 1.

    TimeSignature = 0x58,       // 
    KeySignature = 0x59,        // Valid values: A A#m Ab Abm Am B Bb Bbm Bm C C# C#m Cb Cm D D#m Db Dm E Eb Ebm Em F F# F#m Fm G G#m Gb Gm

    SequencerSpecific = 0x7F    // An unprocessed sequencer specific message containing raw data.
};

#pragma warning(disable: 4820) // Padding added after data member
struct MIDIEvent
{
    enum
    {
        MaxStaticData = 16
    };

    enum EventType
    {
        NoteOff = 0,
        NoteOn,
        PolyphonicAftertouch,
        ControlChange,
        ProgramChange,
        ChannelAftertouch,
        PitchWheel,
        Extended
    };

    uint32_t Timestamp;
    EventType Type;
    uint32_t ChannelNumber;

    size_t DataSize;
    uint8_t Data[MaxStaticData];

    std::vector<uint8_t> ExtendedData;

    MIDIEvent() noexcept : Timestamp(0), Type(EventType::NoteOff), ChannelNumber(0), DataSize(0)
    {
        Data[0] = 0;
    }

    MIDIEvent(const MIDIEvent & midiEvent);
    MIDIEvent(uint32_t timestamp, EventType eventType, uint32_t channel, const uint8_t * data, size_t size);

    size_t GetDataSize() const noexcept
    {
        return DataSize + ExtendedData.size();
    }

    void GetData(uint8_t * data, size_t offset, size_t length) const;
};
#pragma warning(default: 4820) // Padding added after data member

class MIDITrack
{
public:
    MIDITrack() noexcept { }

    MIDITrack(const MIDITrack & track) noexcept
    {
        _Events = track._Events;
    }
    MIDITrack & operator=(const MIDITrack & track)
    {
        _Events = track._Events;

        return *this;
    }

    void AddEvent(const MIDIEvent & event);
    void RemoveEvent(size_t index);

    size_t GetLength() const noexcept
    {
        return _Events.size();
    }

    const MIDIEvent & operator [] (size_t index) const noexcept
    {
        return _Events[index];
    }

    MIDIEvent & operator [] (std::size_t index) noexcept
    {
        return _Events[index];
    }

private:
    std::vector<MIDIEvent> _Events;
};

struct TempoItem
{
    uint32_t Timestamp;
    uint32_t Tempo;

    TempoItem() noexcept : Timestamp(0), Tempo(0)
    {
    }
    TempoItem(uint32_t timestamp, uint32_t tempo);
};

class TempoMap
{
public:
    void Add(uint32_t tempo, uint32_t timestamp);
    uint32_t TimestampToMS(uint32_t timestamp, uint32_t division) const;

    size_t Size() const
    {
        return _Items.size();
    }

    const TempoItem & operator [] (std::size_t p_index) const
    {
        return _Items[p_index];
    }

    TempoItem & operator [] (size_t index)
    {
        return _Items[index];
    }

private:
    std::vector<TempoItem> _Items;
};

#pragma warning(disable: 4820)
struct SysExItem
{
    size_t Offset;
    size_t Size;
    uint8_t PortNumber;

    SysExItem() noexcept : Offset(0), Size(0), PortNumber(0) { }
    SysExItem(const SysExItem & src);
    SysExItem(uint8_t portNumber, std::size_t offset, std::size_t size);
};
#pragma warning(default: 4820)

class SysExTable
{
public:
    size_t AddItem(const uint8_t * data, std::size_t size, uint8_t portNumber);
    bool GetItem(size_t index, const uint8_t * & data, std::size_t & size, uint8_t & portNumber) const;

    size_t Size() const
    {
        return _Items.size();
    }

private:
    std::vector<SysExItem> _Items;
    std::vector<uint8_t> _Data;
};

struct MIDIStreamEvent
{
    uint32_t Timestamp; // in ms
    uint32_t Data;

    MIDIStreamEvent() noexcept : Timestamp(0), Data(0)
    {
    }

    MIDIStreamEvent(uint32_t timestamp, uint32_t data);
};

#pragma warning(disable: 4820) // Padding added after data member
struct MIDIMetaDataItem
{
    uint32_t Timestamp;
    std::string Name;
    std::string Value;

    MIDIMetaDataItem() noexcept : Timestamp(0) { }

    MIDIMetaDataItem(const MIDIMetaDataItem & item) noexcept { operator=(item); };
    MIDIMetaDataItem & operator=(const MIDIMetaDataItem & other) noexcept
    {
        Timestamp = other.Timestamp;
        Name = other.Name;
        Value = other.Value;

        return *this;
    }

    MIDIMetaDataItem(MIDIMetaDataItem && item) { operator=(item); }
    MIDIMetaDataItem & operator=(MIDIMetaDataItem && other)
    {
        Timestamp = other.Timestamp;
        Name = std::move(Name);
        Value = std::move(Value);

        return *this;
    }

    virtual ~MIDIMetaDataItem() { }

    MIDIMetaDataItem(uint32_t timestamp, const char * name, const char * value) noexcept
    {
        Timestamp = timestamp;
        Name = name;
        Value = value;
    }
};
#pragma warning(default: 4820) // Padding added after data member

class MIDIMetaData
{
public:
    MIDIMetaData() noexcept { }

    void AddItem(const MIDIMetaDataItem & item);

    void Append(const MIDIMetaData & data);

    bool GetItem(const char * name, MIDIMetaDataItem & item) const;

    bool GetBitmap(std::vector<uint8_t> & bitmap);

    void AssignBitmap(std::vector<uint8_t>::const_iterator const & begin, std::vector<uint8_t>::const_iterator const & end);

    std::size_t GetCount() const;

    const MIDIMetaDataItem & operator [] (size_t index) const;

private:
    std::vector<MIDIMetaDataItem> _Items;
    std::vector<uint8_t> _Bitmap;
};

#pragma warning(disable: 4820)
class MIDIContainer
{
public:
    MIDIContainer() : _Format(0), _TimeDivision(0), _ExtraPercussionChannel(~0u)
    {
        _DeviceNames.resize(16);
    }

    void Initialize(uint32_t format, uint32_t division);

    void AddTrack(const MIDITrack & track);
    void AddEventToTrack(size_t trackIndex, const MIDIEvent & event);

    // These functions are really only designed to merge and later remove System Exclusive message dumps.
    void MergeTracks(const MIDIContainer & source);
    void SetTrackCount(uint32_t count);
    void SetExtraMetaData(const MIDIMetaData & data);

    void ApplyHack(uint32_t hack);

    void SerializeAsStream(size_t subSongIndex, std::vector<MIDIStreamEvent> & stream, SysExTable & sysExTable, uint32_t & loopBegin, uint32_t & loopEnd, uint32_t cleanFlags) const;

    void SerializeAsSMF(std::vector<uint8_t> & data) const;

    void PromoteToType1();

    void TrimStart();

    typedef std::string(* SplitCallback)(uint8_t bank_msb, uint8_t bank_lsb, uint8_t instrument);

    void SplitByInstrumentChanges(SplitCallback callback = nullptr);

    size_t GetSubSongCount() const;
    size_t GetSubSong(size_t index) const;

    uint32_t GetDuration(size_t subsongIndex, bool ms = false) const;

    uint32_t GetFormat() const;
    uint32_t GetTrackCount() const;
    uint32_t GetChannelCount(size_t subSongIndex) const;

    uint32_t GetLoopBeginTimestamp(size_t subSongIndex, bool ms = false) const;
    uint32_t GetLoopEndTimestamp(size_t subSongIndex, bool ms = false) const;

    void GetMetaData(size_t subSongIndex, MIDIMetaData & data);

    void SetExtraPercussionChannel(uint32_t channelNumber) noexcept { _ExtraPercussionChannel = channelNumber; }
    uint32_t GetExtraPercussionChannel() const noexcept { return _ExtraPercussionChannel; }

    void DetectLoops(bool detectXMILoops, bool detectMarkerLoops, bool detectRPGMakerLoops, bool detectTouhouLoops);

    static void EncodeVariableLengthQuantity(std::vector<uint8_t> & data, uint32_t delta);

public:
    enum
    {
        CleanFlagEMIDI = 1 << 0,
        CleanFlagInstruments = 1 << 1,
        CleanFlagBanks = 1 << 2,
    };

private:
    void TrimRange(size_t start, size_t end);
    void TrimTempoMap(size_t index, uint32_t base_timestamp);

    uint32_t TimestampToMS(uint32_t timestamp, size_t subsongIndex) const;

    #pragma warning(disable: 4267)
    /*
     * Normalize port numbers properly
     */
    template <typename T> void limit_port_number(T & number)
    {
        for (size_t i = 0; i < _PortNumbers.size(); ++i)
        {
            if (_PortNumbers[i] == number)
            {
                number = (T)i;
                return;
            }
        }

        _PortNumbers.push_back((uint8_t) number);

        number = _PortNumbers.size() - 1;
    }

    template <typename T> void limit_port_number(T & number) const
    {
        for (size_t i = 0; i < _PortNumbers.size(); i++)
        {
            if (_PortNumbers[i] == number)
            {
                number = (T)i;
                return;
            }
        }
    }
    #pragma warning(default: 4267)

    void AssignString(const char * src, size_t srcLength, std::string & dst) const
    {
        dst.assign(src, src + srcLength);
    }

private:
    uint32_t _Format;
    uint32_t _TimeDivision;
    uint32_t _ExtraPercussionChannel;

    std::vector<uint64_t> _ChannelMask;
    std::vector<TempoMap> _TempoMaps;
    std::vector<MIDITrack> _Tracks;

    std::vector<uint8_t> _PortNumbers;

    std::vector<std::vector<std::string>> _DeviceNames;

    MIDIMetaData _ExtraMetaData;

    std::vector<uint32_t> _EndTimestamps;

    std::vector<Range> _Loop;
};
#pragma warning(default: 4820)
