#pragma once

#pragma warning(disable: 4514) // Unreferenced inline function has been removed
#pragma warning(disable: 5045) // Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified

#include <stdint.h>

#include <string>
#include <vector>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define snprintf sprintf_s
#endif

#pragma warning(disable: 4820) // Padding added after data member
struct midi_event
{
    enum
    {
        MaxStaticData = 16
    };

    enum event_type
    {
        note_off = 0,
        note_on,
        polyphonic_aftertouch,
        control_change,
        program_change,
        channel_aftertouch,
        pitch_wheel,
        extended
    };

    unsigned long _Timestamp;
    event_type _Type;
    unsigned _ChannelNumber;

    size_t _DataSize;
    uint8_t _Data[MaxStaticData];

    std::vector<uint8_t> _ExtendedData;

    midi_event() noexcept : _Timestamp(0), _Type(note_off), _ChannelNumber(0), _DataSize(0)
    {
        _Data[0] = 0;
    }

    midi_event(const midi_event & midiEvent);
    midi_event(unsigned long timestamp, event_type eventType, unsigned channel, const uint8_t * data, size_t size);

    size_t GetDataSize() const;
    void GetData(uint8_t * data, size_t offset, size_t length) const;
};
#pragma warning(default: 4820) // Padding added after data member

class midi_track
{
public:
    midi_track() noexcept { }

    midi_track(const midi_track & track);

    void AddEvent(const midi_event & event);
    void RemoveEvent(size_t index);

    size_t GetLength() const noexcept
    {
        return _Events.size();
    }

    const midi_event & operator [] (size_t index) const noexcept
    {
        return _Events[index];
    }

    midi_event & operator [] (std::size_t index) noexcept
    {
        return _Events[index];
    }

private:
    std::vector<midi_event> _Events;
};

struct tempo_entry
{
    unsigned long m_timestamp;
    unsigned m_tempo;

    tempo_entry() noexcept : m_timestamp(0), m_tempo(0)
    {
    }
    tempo_entry(unsigned long p_timestamp, unsigned p_tempo);
};

class tempo_map
{
public:
    void add_tempo(unsigned p_tempo, unsigned long p_timestamp);
    unsigned long timestamp_to_ms(unsigned long p_timestamp, unsigned p_dtx) const;

    std::size_t get_count() const;
    const tempo_entry & operator [] (std::size_t p_index) const;
    tempo_entry & operator [] (std::size_t p_index);

private:
    std::vector<tempo_entry> m_entries;
};

struct system_exclusive_entry
{
    std::size_t m_port;
    std::size_t m_offset;
    std::size_t m_length;
    system_exclusive_entry() noexcept : m_port(0), m_offset(0), m_length(0)
    {
    }
    system_exclusive_entry(const system_exclusive_entry & p_in);
    system_exclusive_entry(std::size_t p_port, std::size_t p_offset, std::size_t p_length);
};

class system_exclusive_table
{
public:
    unsigned add_entry(const uint8_t * p_data, std::size_t p_size, std::size_t p_port);
    void get_entry(unsigned p_index, const uint8_t *& p_data, std::size_t & p_size, std::size_t & p_port);

private:
    std::vector<uint8_t> m_data;
    std::vector<system_exclusive_entry> m_entries;
};

struct midi_stream_event
{
    unsigned long m_timestamp;
    uint32_t m_event;

    midi_stream_event() noexcept : m_timestamp(0), m_event(0)
    {
    }

    midi_stream_event(unsigned long p_timestamp, uint32_t p_event);
};

#pragma warning(disable: 4820) // Padding added after data member
struct midi_meta_data_item
{
    unsigned long Timestamp;
    std::string Name;
    std::string Value;

    midi_meta_data_item() noexcept : Timestamp(0)
    {
    }
    midi_meta_data_item(const midi_meta_data_item & item);
    midi_meta_data_item(unsigned long timestamp, const char * name, const char * value);
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
        m_device_names.resize(16);
    }

    void Initialize(unsigned format, unsigned division);

    void add_track(const midi_track & track);
    void add_track_event(size_t trackIndex, const midi_event & event);

    // These functions are really only designed to merge and later remove System Exclusive message dumps.
    void MergeTracks(const midi_container & p_source);
    void SetTrackCount(unsigned count);
    void set_extra_meta_data(const midi_meta_data & p_data);

    /*
     * Blah.
     * Hack 0: Remove channel 16
     * Hack 1: Remove channels 11-16
     */
    void apply_hackfix(unsigned hack);

    void serialize_as_stream(unsigned long subsong, std::vector<midi_stream_event> & p_stream, system_exclusive_table & p_system_exclusive, unsigned long & loop_start, unsigned long & loop_end, unsigned clean_flags) const;

    void serialize_as_standard_midi_file(std::vector<uint8_t> & p_midi_file) const;

    void promote_to_type1();

    void trim_start();

    typedef std::string(* SplitCallback)(uint8_t bank_msb, uint8_t bank_lsb, uint8_t instrument);

    void split_by_instrument_changes(SplitCallback callback = nullptr);

    unsigned long GetSubSongCount() const;
    unsigned long GetSubSong(size_t index) const;

    unsigned long GetDuration(size_t subsongIndex, bool ms = false) const;

    unsigned long GetFormat() const;
    unsigned long GetTrackCount() const;
    unsigned long GetChannelCount(size_t subSongIndex) const;

    unsigned long get_timestamp_loop_start(unsigned long subsong, bool ms = false) const;
    unsigned long get_timestamp_loop_end(unsigned long subsong, bool ms = false) const;

    void GetMetaData(size_t subSongIndex, midi_meta_data & data);

    void scan_for_loops(bool p_xmi_loops, bool p_marker_loops, bool p_rpgmaker_loops, bool p_touhou_loops);

    static void encode_delta(std::vector<uint8_t> & p_out, unsigned long delta);

public:
    enum
    {
        clean_flag_emidi = 1 << 0,
        clean_flag_instruments = 1 << 1,
        clean_flag_banks = 1 << 2,
    };

private:
    void trim_range_of_tracks(size_t start, size_t end);
    void trim_tempo_map(size_t index, unsigned long base_timestamp);

    unsigned long timestamp_to_ms(unsigned long timestamp, size_t subsongIndex) const;

    /*
     * Normalize port numbers properly
     */
    template <typename T> void limit_port_number(T & number)
    {
        for (size_t i = 0; i < m_port_numbers.size(); ++i)
        {
            if (m_port_numbers[i] == number)
            {
                number = (T)i;
                return;
            }
        }

        m_port_numbers.push_back((uint8_t) number);

        number = m_port_numbers.size() - 1;
    }

    template <typename T> void limit_port_number(T & number) const
    {
        for (unsigned i = 0; i < m_port_numbers.size(); i++)
        {
            if (m_port_numbers[i] == number)
            {
                number = (T)i;
                return;
            }
        }
    }

private:
    unsigned _Format;
    unsigned _Division;

    std::vector<uint64_t> m_channel_mask;
    std::vector<tempo_map> m_tempo_map;
    std::vector<midi_track> _Tracks;

    std::vector<uint8_t> m_port_numbers;

    std::vector<std::vector<std::string>> m_device_names;

    midi_meta_data m_extra_meta_data;

    std::vector<unsigned long> m_timestamp_end;

    std::vector<unsigned long> m_timestamp_loop_start;
    std::vector<unsigned long> m_timestamp_loop_end;
};
