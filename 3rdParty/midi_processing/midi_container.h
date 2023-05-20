#pragma once

#pragma warning(disable: 4514) // Unreferenced inline function has been removed
#pragma warning(disable: 5045) // Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified

#include <stdint.h>

#include <string>
#include <vector>

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

enum MetaDataTypes
{
    SequenceNumber = 0x00,      // Sequence number in type 0 and 1 MIDI files; pattern number in type 2 MIDI files. (0..65535, default 0)
    Text = 0x01,                // General "Text" Meta Message. Can be used for any text based data. (string)
    Copyright = 0x02,           // Provides information about a MIDI file�s copyright. (string)
    TrackName = 0x03,           // Track name (string)
    InstrumentName = 0x04,      // Instrument name (string)
    Lyrics = 0x05,              // Stores the lyrics of a song. Typically one syllable per Meta Message. (string)
    Marker = 0x06,              // Marks a point of interest in a MIDI file. Can be used as the marker for the beginning of a verse, solo, etc. (string)
    CueMarker = 0x07,           // Marks a cue. IE: �Cue performer 1�, etc (string)

    DeviceName = 0x09,          // Gives the name of the device. (string)

    ChannelPrefix = 0x20,       // Gives the prefix for the channel on which events are played. (0..255, default 0)
    MIDIPort = 0x21,            // Gives the MIDI Port on which events are played. (0..255, default 0)

    EndOfTrack = 0x2F,          // An empty Meta Message that marks the end of a track.

    SetTempo = 0x51,            // Tempo is in microseconds per beat (quarter note). (0..16777215, default 500000)

    SMPTEOffset = 0x54,         // 

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

    size_t GetDataSize() const
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

    MIDITrack(const MIDITrack & track);

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

struct TempoEntry
{
    uint32_t Timestamp;
    uint32_t Tempo;

    TempoEntry() noexcept : Timestamp(0), Tempo(0)
    {
    }
    TempoEntry(uint32_t timestamp, uint32_t tempo);
};

class TempoMap
{
public:
    void Add(uint32_t tempo, uint32_t timestamp);
    uint32_t TimestampToMS(uint32_t timestamp, uint32_t division) const;

    size_t GetCount() const
    {
        return _Entries.size();
    }

    const TempoEntry & operator [] (std::size_t p_index) const
    {
        return _Entries[p_index];
    }

    TempoEntry & operator [] (size_t index)
    {
        return _Entries[index];
    }

private:
    std::vector<TempoEntry> _Entries;
};

struct system_exclusive_entry
{
    size_t m_port;
    size_t m_offset;
    size_t m_length;

    system_exclusive_entry() noexcept : m_port(0), m_offset(0), m_length(0)
    {
    }
    system_exclusive_entry(const system_exclusive_entry & p_in);
    system_exclusive_entry(std::size_t p_port, std::size_t p_offset, std::size_t p_length);
};

class system_exclusive_table
{
public:
    uint32_t add_entry(const uint8_t * p_data, std::size_t p_size, std::size_t p_port);
    void get_entry(uint32_t p_index, const uint8_t *& p_data, std::size_t & p_size, std::size_t & p_port);

private:
    std::vector<uint8_t> m_data;
    std::vector<system_exclusive_entry> m_entries;
};

struct midi_stream_event
{
    uint32_t m_timestamp;
    uint32_t m_event;

    midi_stream_event() noexcept : m_timestamp(0), m_event(0)
    {
    }

    midi_stream_event(uint32_t timestamp, uint32_t event);
};

#pragma warning(disable: 4820) // Padding added after data member
struct midi_meta_data_item
{
    uint32_t Timestamp;
    std::string Name;
    std::string Value;

    midi_meta_data_item() noexcept : Timestamp(0)
    {
    }
    midi_meta_data_item(const midi_meta_data_item & item);
    midi_meta_data_item(uint32_t timestamp, const char * name, const char * value);
};
#pragma warning(default: 4820) // Padding added after data member

class midi_meta_data
{
public:
    midi_meta_data() noexcept { }

    void AddItem(const midi_meta_data_item & item);

    void Append(const midi_meta_data & data);

    bool GetItem(const char * name, midi_meta_data_item & item) const;

    bool GetBitmap(std::vector<uint8_t> & bitmap);

    void AssignBitmap(std::vector<uint8_t>::const_iterator const & begin, std::vector<uint8_t>::const_iterator const & end);

    std::size_t GetCount() const;

    const midi_meta_data_item & operator [] (size_t index) const;

private:
    std::vector<midi_meta_data_item> _Items;
    std::vector<uint8_t> _Bitmap;
};

class midi_container
{
public:
    midi_container() : _Format(0), _Division(0)
    {
        _DeviceNames.resize(16);
    }

    void Initialize(uint32_t format, uint32_t division);

    void AddTrack(const MIDITrack & track);
    void AddEventToTrack(size_t trackIndex, const MIDIEvent & event);

    // These functions are really only designed to merge and later remove System Exclusive message dumps.
    void MergeTracks(const midi_container & source);
    void SetTrackCount(uint32_t count);
    void SetExtraMetaData(const midi_meta_data & data);

    void ApplyHack(uint32_t hack);

    void serialize_as_stream(size_t subSongIndex, std::vector<midi_stream_event> & p_stream, system_exclusive_table & p_system_exclusive, uint32_t & loop_start, uint32_t & loop_end, uint32_t clean_flags) const;

    void serialize_as_standard_midi_file(std::vector<uint8_t> & data) const;

    void promote_to_type1();

    void trim_start();

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

    void GetMetaData(size_t subSongIndex, midi_meta_data & data);

    void DetectLoops(bool detectXMILoops, bool detectMarkerLoops, bool detectRPGMakerLoops, bool detectTouhouLoops);

    static void EncodeVariableLengthQuantity(std::vector<uint8_t> & p_out, uint32_t delta);

public:
    enum
    {
        CleanFlagEMIDI = 1 << 0,
        clean_flag_instruments = 1 << 1,
        clean_flag_banks = 1 << 2,
    };

private:
    void trim_range_of_tracks(size_t start, size_t end);
    void trim_tempo_map(size_t index, uint32_t base_timestamp);

    uint32_t timestamp_to_ms(uint32_t timestamp, size_t subsongIndex) const;

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

private:
    uint32_t _Format;
    uint32_t _Division;

    std::vector<uint64_t> _ChannelMask;
    std::vector<TempoMap> _TempoMaps;
    std::vector<MIDITrack> _Tracks;

    std::vector<uint8_t> _PortNumbers;

    std::vector<std::vector<std::string>> _DeviceNames;

    midi_meta_data _ExtraMetaData;

    std::vector<uint32_t> _EndTimestamps;

    std::vector<uint32_t> _LoopBeginTimestamps;
    std::vector<uint32_t> _LoopEndTimestamps;
};
