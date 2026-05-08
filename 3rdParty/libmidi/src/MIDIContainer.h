
/** $VER: MIDIContainer.h (2026.05.01) **/

#pragma once

#include "pch.h"

#include "MIDI.h"
#include "Range.h"

#pragma warning(disable: 4820) // x bytes padding added after data member 'y'

namespace midi
{

/// <summary>
/// Represents a MIDI event.
/// </summary>
struct event_t
{
    enum event_type_t
    {
        NoteOff = 0,        // 0x80
        NoteOn,             // 0x90
        KeyPressure,        // 0xA0
        ControlChange,      // 0xB0
        ProgramChange,      // 0xC0
        ChannelPressure,    // 0xD0
        PitchBendChange,    // 0xE0
        Extended            // 0xF0
    };

    uint32_t Time;              // Absolute time
    event_type_t Type;
    uint32_t ChannelNumber;
    std::vector<uint8_t> Data;

    event_t() noexcept : Time(), Type(event_type_t::NoteOff), ChannelNumber()
    {
    }

    event_t(const event_t & other) noexcept
    {
        Time = other.Time;
        Type = other.Type;
        ChannelNumber = other.ChannelNumber;
        Data = other.Data;
    }

    event_t & operator =(const event_t & other) noexcept
    {
        Time = other.Time;
        Type = other.Type;
        ChannelNumber = other.ChannelNumber;
        Data = other.Data;

        return *this;
    }

    event_t(uint32_t time, event_type_t eventType, uint32_t channelNumber, const uint8_t * data, size_t size) noexcept
    {
        Time = time;
        Type = eventType;
        ChannelNumber = channelNumber;
        Data.assign(data, data + size);
    }

    bool IsSetTempo() const noexcept    { return (Type == event_t::Extended) && (Data.size() >= 5) && (Data[0] == StatusCode::MetaData) && (Data[1] == MetaDataType::SetTempo); }
    bool IsMarker() const noexcept      { return (Type == event_t::Extended) && (Data.size() >= 9) && (Data[0] == StatusCode::MetaData) && (Data[1] == MetaDataType::Marker); }
    bool IsPort() const noexcept        { return (Type == event_t::Extended) && (Data.size() >= 2) && (Data[0] == StatusCode::MetaData) && (Data[1] == MetaDataType::MIDIPort); }
    bool IsEndOfTrack() const noexcept  { return (Type == event_t::Extended) && (Data.size() >= 2) && (Data[0] == StatusCode::MetaData) && (Data[1] == MetaDataType::EndOfTrack); }
};

/// <summary>
/// Represents a track in a MIDI file.
/// </summary>
class track_t
{
public:
    track_t() noexcept : _IsPortSet(false) { }

    track_t(const track_t & track) noexcept : _IsPortSet(false)
    {
        _Events = track._Events;
    }

    track_t & operator=(const track_t & track)
    {
        _Events = track._Events;

        return *this;
    }

    void AddEvent(const event_t & event);
    void AddEventToStart(const event_t & event);
    void RemoveEvent(size_t index);

    size_t GetLength() const noexcept
    {
        return _Events.size();
    }

    const event_t & operator[](size_t index) const noexcept
    {
        return _Events[index];
    }

    event_t & operator[](std::size_t index) noexcept
    {
        return _Events[index];
    }

    bool IsPortSet() const noexcept { return _IsPortSet; }

public:
    using events_t = std::vector<event_t>;

    using iterator       = events_t::iterator;
    using const_iterator = events_t::const_iterator;

    iterator begin() { return _Events.begin(); }
    iterator end() { return _Events.end(); }

    const_iterator begin() const { return _Events.begin(); }
    const_iterator end() const { return _Events.end(); }

    const_iterator cbegin() const { return _Events.cbegin(); }
    const_iterator cend() const { return _Events.cend(); }

    const event_t & front() const noexcept { return _Events.front(); }
    const event_t & back() const noexcept { return _Events.back(); }

private:
    std::vector<event_t> _Events;
    bool _IsPortSet;                        // True if the track contains at least 1 MIDI Port event.
};

/// <summary>
/// Represents a tempo item in the tempo map.
/// </summary>
struct tempo_item_t
{
    uint32_t Time;
    uint32_t Tempo;

    tempo_item_t() noexcept : Time(0), Tempo(0)
    {
    }

    tempo_item_t(uint32_t timestamp, uint32_t tempo);
};

/// <summary>
/// Implements a tempo map.
/// </summary>
class tempo_map_t
{
public:
    void Add(uint32_t tempo, uint32_t timestamp);
    uint32_t TimestampToMS(uint32_t timestamp, uint32_t division) const;

    size_t Size() const noexcept { return _Items.size(); }

    const tempo_item_t & operator[](std::size_t p_index) const
    {
        return _Items[p_index];
    }

    tempo_item_t & operator[](size_t index)
    {
        return _Items[index];
    }

private:
    std::vector<tempo_item_t> _Items;
};

/// <summary>
/// Implements an item in the SysEx table.
/// </summary>
struct sysex_item_t
{
    size_t Offset;
    size_t Size;
    uint8_t PortNumber;

    sysex_item_t() noexcept : Offset(0), Size(0), PortNumber(0) { }
    sysex_item_t(const sysex_item_t & other);
    sysex_item_t(uint8_t portNumber, std::size_t offset, std::size_t size);
};

/// <summary>
/// Implements a table with all the unique SysEx events.
/// </summary>
class sysex_table_t
{
public:
    size_t AddItem(const uint8_t * data, std::size_t size, uint8_t portNumber);
    bool GetItem(size_t index, const uint8_t * & data, std::size_t & size, uint8_t & portNumber) const noexcept;

    size_t Size() const noexcept { return _Items.size(); }

private:
    std::vector<sysex_item_t> _Items;
    std::vector<uint8_t> _Data;
};

/// <summary>
/// Implements a metadata item in the metadata table.
/// </summary>
struct metadata_item_t
{
    uint32_t Timestamp;
    std::string Name;
    std::string Value;

    metadata_item_t() noexcept : Timestamp(0) { }

    metadata_item_t(const metadata_item_t & item) noexcept { operator=(item); };
    metadata_item_t & operator=(const metadata_item_t & other) noexcept
    {
        Timestamp = other.Timestamp;
        Name = other.Name;
        Value = other.Value;

        return *this;
    }

    metadata_item_t(metadata_item_t && item) { operator=(item); }
    metadata_item_t & operator=(metadata_item_t && other)
    {
        Timestamp = other.Timestamp;
        Name = std::move(Name);
        Value = std::move(Value);

        return *this;
    }

    virtual ~metadata_item_t() { }

    metadata_item_t(uint32_t timestamp, const char * name, const char * value) noexcept
    {
        Timestamp = timestamp;
        Name = name;
        Value = value;
    }
};

/// <summary>
/// Implements a table with the metadata items.
/// </summary>
class metadata_table_t
{
public:
    metadata_table_t() noexcept { }

    void AddItem(const metadata_item_t & item);
    void Append(const metadata_table_t & data);
    bool GetItem(const char * name, metadata_item_t & item) const noexcept;
    bool GetBitmap(std::vector<uint8_t> & bitmap) const;
    void AssignBitmap(std::vector<uint8_t>::const_iterator const & begin, std::vector<uint8_t>::const_iterator const & end);

    std::size_t GetCount() const noexcept { return _Items.size(); }

    const metadata_item_t & operator[](size_t index) const;

    using midi_metadata_items_t = std::vector<metadata_item_t>;

    using iterator       = midi_metadata_items_t::iterator;
    using const_iterator = midi_metadata_items_t::const_iterator;

    iterator begin() { return _Items.begin(); }
    iterator end() { return _Items.end(); }

    const_iterator begin() const { return _Items.begin(); }
    const_iterator end() const { return _Items.end(); }

    const_iterator cbegin() const { return _Items.cbegin(); }
    const_iterator cend() const { return _Items.cend(); }

    const metadata_item_t & front() const noexcept { return _Items.front(); }
    const metadata_item_t & back() const noexcept { return _Items.back(); }

private:
    std::vector<metadata_item_t> _Items;
    std::vector<uint8_t> _Bitmap;
};

/// <summary>
/// Implements an MIDI message in the container.
/// </summary>
struct message_t
{
    uint32_t Time; // in ms
    uint32_t Data;

    bool IsSysEx() const noexcept { return ((Data & 0x80000000u) == 0x80000000u); }
};

/// <summary>
/// Represents the format of the file used to create a container.
/// </summary>
enum FileFormat
{
    SMF,
    RMI,
    XMI,
    XFM,
    MDS,
    HMP,
    HMI,
    MUS,
    LDS,
    GMF,
    RCP,
    XMF,
    MMF,
    MMD,
    SYX,

#ifdef _DEBUG
    TST,
#endif

    Unknown = -1
};

/// <summary>
/// Implements a container for the MIDI messages.
/// </summary>
class container_t
{
public:
    container_t() : FileFormat(FileFormat::Unknown), BankOffset(0), _Format(), _TimeDivision(), _ExtraPercussionChannel(~0u)
    {
        _DeviceNames.resize(16);
    }

    container_t(const container_t &) = delete;
    container_t(container_t &&) = delete;
    container_t & operator=(const container_t &) = delete;
    container_t & operator=(container_t &&) = delete;

    virtual ~container_t() noexcept { };

    void Initialize(uint32_t format, uint32_t division);

    void AddTrack(const track_t & track);
    void AddEventToTrack(size_t trackIndex, const event_t & event);

    // These functions are really only designed to merge and later remove System Exclusive message dumps.
    void MergeTracks(const container_t & source);
    void SetTrackCount(uint32_t count);
    void SetExtraMetaData(const metadata_table_t & data);

    void ApplyHack(uint32_t hack);

    void SerializeAsStream(size_t subSongIndex, std::vector<message_t> & stream, sysex_table_t & sysExTable, std::vector<uint8_t> & portNumbers, uint32_t & loopBegin, uint32_t & loopEnd, uint32_t cleanFlags) const;
    void SerializeAsSMF(std::vector<uint8_t> & data) const;

    void PromoteToType1();

    void TrimStart();

    typedef std::string(* SplitCallback)(uint8_t bank_msb, uint8_t bank_lsb, uint8_t instrument);

    void SplitByInstrumentChanges(SplitCallback callback = nullptr);

    bool IsEmpty() const noexcept { return _Tracks.empty(); }

    size_t GetSubSongCount() const;
    size_t GetSubSong(size_t index) const;

    uint32_t GetDuration(size_t subsongIndex, bool ms = false) const;

    uint32_t GetFormat() const;
    uint32_t GetTrackCount() const noexcept { return (uint32_t) _Tracks.size(); }
;
    uint32_t GetChannelCount(size_t subSongIndex) const;

    uint32_t GetLoopBeginTimestamp(size_t subSongIndex, bool ms = false) const;
    uint32_t GetLoopEndTimestamp(size_t subSongIndex, bool ms = false) const;

    std::vector<track_t> & GetTracks() { return _Tracks; }

    const std::vector<uint8_t> & GetArtwork() const noexcept { return _Artwork; }
    void SetArtwork(const std::vector<uint8_t> & artwork) noexcept { _Artwork = artwork; }

    void GetMetaData(size_t subSongIndex, metadata_table_t & data);

    void SetExtraPercussionChannel(uint32_t channelNumber) noexcept { _ExtraPercussionChannel = channelNumber; }
    uint32_t GetExtraPercussionChannel() const noexcept { return _ExtraPercussionChannel; }

    void DetectLoops(bool detectXMILoops, bool detectMarkerLoops, bool detectRPGMakerLoops, bool detectTouhouLoops, bool detectLeapFrogLoops);

    uint32_t TimestampToMS(uint32_t timestamp, size_t subsongIndex) const;

    static void EncodeVariableLengthQuantity(std::vector<uint8_t> & data, uint32_t delta);

public:
    using miditracks_t = std::vector<track_t>;
    using iterator = miditracks_t::iterator;
    using const_iterator = miditracks_t::const_iterator;

    iterator begin() { return _Tracks.begin(); }
    iterator end() { return _Tracks.end(); }

    const_iterator begin() const { return _Tracks.begin(); }
    const_iterator end() const { return _Tracks.end(); }
    const_iterator cbegin() const { return _Tracks.cbegin(); }
    const_iterator cend() const { return _Tracks.cend(); }

public:
    enum
    {
        CleanFlagEMIDI = 1 << 0,
        CleanFlagInstruments = 1 << 1,
        CleanFlagBanks = 1 << 2,
    };

    FileFormat FileFormat;

    std::vector<uint8_t> SoundFont;
    int BankOffset;                 // Bank offset for MIDI files that contain an embedded soundfont. See https://github.com/spessasus/sf2-rmidi-specification?tab=readme-ov-file#dbnk-chunk

private:
    void TrimRange(size_t start, size_t end);
    void TrimTempoMap(size_t index, uint32_t base_timestamp);

    #pragma warning(disable: 4267)

    /// <summary>
    /// Normalizes the specified port number so that we get increasing port numbers starting at 0 and incrementing by 1.
    /// </summary>
    template <typename T> void NormalizePortNumber(T & number)
    {
        for (size_t i = 0; i < _PortNumbers.size(); ++i)
        {
            if (number == _PortNumbers[i])
            {
                number = (T) i;

                return;
            }
        }

        _PortNumbers.push_back((uint8_t) number);

        number = _PortNumbers.size() - 1;
    }

    /// <summary>
    /// Gets the normalizel portnumber of the specified port.
    /// </summary>
    template <typename T> void NormalizePortNumber(T & number) const
    {
        for (size_t i = 0; i < _PortNumbers.size(); ++i)
        {
            if (number == _PortNumbers[i])
            {
                number = (T) i;

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
    uint32_t _TimeDivision;             // 0x0000 - 0x7FFF: "Ticks per Beat" or "Pulses per Quarter Note (PPQ)" / 0x8000 - 0xFFFF: Frames per Second

    uint32_t _ExtraPercussionChannel;

    const size_t MaxChannels = 48;

    std::vector<uint64_t> _ChannelMask;
    std::vector<tempo_map_t> _TempoMaps;
    std::vector<track_t> _Tracks;

    std::vector<uint8_t> _PortNumbers;

    std::vector<std::vector<std::string>> _DeviceNames;

    metadata_table_t _ExtraMetaData;

    std::vector<uint32_t> _EndTimestamps;   // Largest timestamp for each track.

    std::vector<range_t> _Loop;
    std::vector<uint8_t> _Artwork;
};

}
