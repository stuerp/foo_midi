#include "midi_container.h"

#include <string.h>

#pragma warning(disable: 4242)
#include <algorithm>
#pragma warning(default: 4242)

#pragma region("MIDI Event")
midi_event::midi_event(const midi_event & p_in)
{
    _Timestamp = p_in._Timestamp;
    _ChannelNumber = p_in._ChannelNumber;
    _Type = p_in._Type;
    _DataSize = p_in._DataSize;
    memcpy(_Data, p_in._Data, _DataSize);
    _ExtendedData = p_in._ExtendedData;
}

midi_event::midi_event(unsigned long timestamp, event_type eventType, unsigned channelNumber, const uint8_t * data, size_t size)
{
    _Timestamp = timestamp;
    _Type = eventType;
    _ChannelNumber = channelNumber;

    if (size <= MaxStaticData)
    {
        _DataSize = size;
        ::memcpy(_Data, data, size);
    }
    else
    {
        _DataSize = MaxStaticData;
        ::memcpy(_Data, data, MaxStaticData);

        _ExtendedData.assign(data + MaxStaticData, data + size);
    }
}

size_t midi_event::GetDataSize() const
{
    return _DataSize + _ExtendedData.size();
}

void midi_event::GetData(uint8_t * data, size_t offset, size_t length) const
{
    size_t Size = _DataSize + _ExtendedData.size();

    size_t max_count = (offset <= Size) ? Size - offset : 0;

    length = std::min(length, max_count);

    if (offset < MaxStaticData)
    {
        size_t _max_count = MaxStaticData - offset;
        size_t count = std::min(_max_count, length);

        ::memcpy(data, _Data + offset, count);

        data += count;
        length -= count;
    }

    if (length > 0)
        ::memcpy(data, &_ExtendedData[0], length);
}
#pragma endregion

#pragma region("MIDI Track")
midi_track::midi_track(const midi_track & track)
{
    _Events = track._Events;
}

void midi_track::AddEvent(const midi_event & newEvent)
{
    auto it = _Events.end();

    if (_Events.size() > 0)
    {
        midi_event & Event = *(it - 1);

        if ((Event._Type == midi_event::extended) && (Event.GetDataSize() >= 2) && (Event._Data[0] == 0xFF) && (Event._Data[1] == 0x2F))
        {
            --it;

            if (Event._Timestamp < newEvent._Timestamp)
                Event._Timestamp = newEvent._Timestamp;
        }

        while (it > _Events.begin())
        {
            if ((*(it - 1))._Timestamp <= newEvent._Timestamp)
                break;

            --it;
        }
    }

    _Events.insert(it, newEvent);
}

void midi_track::RemoveEvent(size_t index)
{
    _Events.erase(_Events.begin() + index);
}

#pragma endregion

#pragma region("Tempo Map")
tempo_entry::tempo_entry(unsigned long p_timestamp, unsigned p_tempo)
{
    m_timestamp = p_timestamp;
    m_tempo = p_tempo;
}

void tempo_map::add_tempo(unsigned p_tempo, unsigned long p_timestamp)
{
    auto it = m_entries.end();

    while (it > m_entries.begin())
    {
        if ((*(it - 1)).m_timestamp <= p_timestamp) break;
        --it;
    }

    if (it > m_entries.begin() && (*(it - 1)).m_timestamp == p_timestamp)
    {
        (*(it - 1)).m_tempo = p_tempo;
    }
    else
    {
        m_entries.insert(it, tempo_entry(p_timestamp, p_tempo));
    }
}

unsigned long tempo_map::timestamp_to_ms(unsigned long p_timestamp, unsigned p_dtx) const
{
    unsigned long timestamp_ms = 0;
    unsigned long timestamp = 0;
    auto tempo_it = m_entries.begin();
    unsigned current_tempo = 500000;

    unsigned half_dtx = p_dtx * 500;
    p_dtx = half_dtx * 2;

    while (tempo_it < m_entries.end() && timestamp + p_timestamp >= (*tempo_it).m_timestamp)
    {
        unsigned long delta = (*tempo_it).m_timestamp - timestamp;
        timestamp_ms += ((uint64_t) current_tempo * (uint64_t) delta + half_dtx) / p_dtx;
        current_tempo = (*tempo_it).m_tempo;
        ++tempo_it;
        timestamp += delta;
        p_timestamp -= delta;
    }

    timestamp_ms += ((uint64_t) current_tempo * (uint64_t) p_timestamp + half_dtx) / p_dtx;

    return timestamp_ms;
}

std::size_t tempo_map::get_count() const
{
    return m_entries.size();
}

const tempo_entry & tempo_map::operator [] (std::size_t p_index) const
{
    return m_entries[p_index];
}

tempo_entry & tempo_map::operator [] (std::size_t p_index)
{
    return m_entries[p_index];
}
#pragma endregion

#pragma region("System Exclusive Table")
system_exclusive_entry::system_exclusive_entry(const system_exclusive_entry & p_in)
{
    m_port = p_in.m_port;
    m_offset = p_in.m_offset;
    m_length = p_in.m_length;
}

system_exclusive_entry::system_exclusive_entry(std::size_t p_port, std::size_t p_offset, std::size_t p_length)
{
    m_port = p_port;
    m_offset = p_offset;
    m_length = p_length;
}

unsigned system_exclusive_table::add_entry(const uint8_t * p_data, std::size_t p_size, std::size_t p_port)
{
    for (auto it = m_entries.begin(); it < m_entries.end(); ++it)
    {
        const system_exclusive_entry & entry = *it;

        if (p_port == entry.m_port && p_size == entry.m_length && !memcmp(p_data, &m_data[entry.m_offset], p_size))
            return ((unsigned) (it - m_entries.begin()));
    }

    system_exclusive_entry entry(p_port, m_data.size(), p_size);

    m_data.insert(m_data.end(), p_data, p_data + p_size);
    m_entries.push_back(entry);

    return ((unsigned) (m_entries.size() - 1));
}

void system_exclusive_table::get_entry(unsigned p_index, const uint8_t *& p_data, std::size_t & p_size, std::size_t & p_port)
{
    const system_exclusive_entry & entry = m_entries[p_index];

    p_data = &m_data[entry.m_offset];
    p_size = entry.m_length;
    p_port = entry.m_port;
}
#pragma endregion

#pragma region("MIDI Stream Event")
midi_stream_event::midi_stream_event(unsigned long p_timestamp, unsigned p_event)
{
    m_timestamp = p_timestamp;
    m_event = p_event;
}
#pragma endregion

#pragma region("MIDI Meta Data")
midi_meta_data_item::midi_meta_data_item(const midi_meta_data_item & item)
{
    Timestamp = item.Timestamp;
    Name = item.Name;
    Value = item.Value;
}

midi_meta_data_item::midi_meta_data_item(unsigned long timestamp, const char * name, const char * value)
{
    Timestamp = timestamp;
    Name = name;
    Value = value;
}

void midi_meta_data::AddItem(const midi_meta_data_item & item)
{
    _Items.push_back(item);
}

void midi_meta_data::Append(const midi_meta_data & data)
{
    _Items.insert(_Items.end(), data._Items.begin(), data._Items.end());
    _Bitmap = data._Bitmap;
}

bool midi_meta_data::GetItem(const char * name, midi_meta_data_item & item) const
{
    for (size_t i = 0; i < _Items.size(); ++i)
    {
        const midi_meta_data_item & Item = _Items[i];

        if (strcasecmp(name, Item.Name.c_str()) == 0)
        {
            item = Item;
            return true;
        }
    }

    return false;
}

bool midi_meta_data::GetBitmap(std::vector<uint8_t> & bitmap)
{
    bitmap = _Bitmap;

    return bitmap.size() != 0;
}

void midi_meta_data::AssignBitmap(std::vector<uint8_t>::const_iterator const & begin, std::vector<uint8_t>::const_iterator const & end)
{
    _Bitmap.assign(begin, end);
}

std::size_t midi_meta_data::GetCount() const
{
    return _Items.size();
}

const midi_meta_data_item & midi_meta_data::operator [] (std::size_t p_index) const
{
    return _Items[p_index];
}
#pragma endregion

#pragma region("MIDI Container")
void midi_container::Initialize(unsigned format, unsigned division)
{
    _Format = format;
    _Division = division;

    if (format != 2)
    {
        m_channel_mask.resize(1);
        m_channel_mask[0] = 0;
        m_tempo_map.resize(1);
        m_timestamp_end.resize(1);
        m_timestamp_end[0] = 0;
        m_timestamp_loop_start.resize(1);
        m_timestamp_loop_end.resize(1);
    }
}

void midi_container::add_track(const midi_track & track)
{
    _Tracks.push_back(track);

    std::string DeviceName;
    uint8_t PortNumber = 0;

    size_t i;

    for (i = 0; i < track.GetLength(); ++i)
    {
        const midi_event & Event = track[i];

        if (Event._Type == midi_event::extended)
        {
            if ((Event.GetDataSize() >= 5) && (Event._Data[0] == 0xFF) && (Event._Data[1] == 0x51))
            {
                unsigned tempo = (unsigned)((Event._Data[2] << 16) | (Event._Data[3] << 8) | Event._Data[4]);

                if (_Format != 2)
                {
                    m_tempo_map[0].add_tempo(tempo, Event._Timestamp);
                }
                else
                {
                    m_tempo_map.resize(_Tracks.size());
                    m_tempo_map[_Tracks.size() - 1].add_tempo(tempo, Event._Timestamp);
                }
            }
            else
            if ((Event.GetDataSize() >= 3) && (Event._Data[0] == 0xFF))
            {
                if (Event._Data[1] == 0x04 || Event._Data[1] == 0x09)
                {
                    std::vector<uint8_t> data;

                    size_t Size = Event.GetDataSize() - 2;

                    data.resize(Size);
                    Event.GetData(&data[0], 2, Size);

                    DeviceName.assign(data.begin(), data.begin() + Size);
                    std::transform(DeviceName.begin(), DeviceName.end(), DeviceName.begin(), ::tolower);
                }
                else
                if (Event._Data[1] == 0x21)
                {
                    PortNumber = Event._Data[2];

                    limit_port_number(PortNumber);
                    DeviceName.clear();
                }
            }
        }
        else
        if (Event._Type == midi_event::note_on || Event._Type == midi_event::note_off)
        {
            unsigned ChannelNumber = Event._ChannelNumber;

            if (DeviceName.length())
            {
                size_t j, k;

                for (j = 0, k = m_device_names[ChannelNumber].size(); j < k; ++j)
                {
                    if (::strcmp(m_device_names[ChannelNumber][j].c_str(), DeviceName.c_str()) == 0)
                        break;
                }

                if (j < k)
                    PortNumber = (uint8_t)j;
                else
                {
                    m_device_names[ChannelNumber].push_back(DeviceName);
                    PortNumber = (uint8_t)k;
                }

                limit_port_number(PortNumber);
                DeviceName.clear();
            }

            ChannelNumber += 16 * PortNumber;
            ChannelNumber %= 48;

            if (_Format != 2)
                m_channel_mask[0] |= 1ULL << ChannelNumber;
            else
            {
                m_channel_mask.resize(_Tracks.size(), 0);
                m_channel_mask[_Tracks.size() - 1] |= 1ULL << ChannelNumber;
            }
        }
    }

    if ((_Format != 2) && (i > 0) && (track[i - 1]._Timestamp > m_timestamp_end[0]))
    {
        m_timestamp_end[0] = track[i - 1]._Timestamp;
    }
    else
    if (_Format == 2)
    {
        if (i > 0)
            m_timestamp_end.push_back(track[i - 1]._Timestamp);
        else
            m_timestamp_end.push_back((unsigned) 0);
    }
}

void midi_container::add_track_event(std::size_t trackNumber, const midi_event & event)
{
    midi_track & Track = _Tracks[trackNumber];

    Track.AddEvent(event);

    if ((event._Type == midi_event::extended) && (event.GetDataSize() >= 5) && (event._Data[0] == 0xFF) && (event._Data[1] == 0x51))
    {
        unsigned tempo = (unsigned)((event._Data[2] << 16) | (event._Data[3] << 8) | event._Data[4]);

        if (_Format != 2)
        {
            m_tempo_map[0].add_tempo(tempo, event._Timestamp);
        }
        else
        {
            m_tempo_map.resize(_Tracks.size());
            m_tempo_map[trackNumber].add_tempo(tempo, event._Timestamp);
        }
    }
    else
    if (event._Type == midi_event::note_on || event._Type == midi_event::note_off)
    {
        if (_Format != 2)
        {
            m_channel_mask[0] |= 1ULL << event._ChannelNumber;
        }
        else
        {
            m_channel_mask.resize(_Tracks.size(), 0);
            m_channel_mask[trackNumber] |= 1ULL << event._ChannelNumber;
        }
    }

    if ((_Format != 2) && (event._Timestamp > m_timestamp_end[0]))
    {
        m_timestamp_end[0] = event._Timestamp;
    }
    else
    if ((_Format == 2) && (event._Timestamp > m_timestamp_end[trackNumber]))
    {
        m_timestamp_end[trackNumber] = event._Timestamp;
    }
}

void midi_container::MergeTracks(const midi_container & source)
{
    for (unsigned i = 0; i < source._Tracks.size(); i++)
        add_track(source._Tracks[i]);
}

void midi_container::SetTrackCount(unsigned count)
{
    _Tracks.resize(count);
}

void midi_container::set_extra_meta_data(const midi_meta_data & data)
{
    m_extra_meta_data = data;
}

void midi_container::apply_hackfix(unsigned hack)
{
    switch (hack)
    {
        case 0:
            for (unsigned i = 0; i < _Tracks.size(); ++i)
            {
                midi_track & t = _Tracks[i];

                for (unsigned j = 0; j < t.GetLength(); )
                {
                    if (t[j]._Type != midi_event::extended && t[j]._ChannelNumber == 16)
                    {
                        t.RemoveEvent(j);
                    }
                    else
                    {
                        ++j;
                    }
                }
            }
            break;

        case 1:
            for (unsigned i = 0; i < _Tracks.size(); ++i)
            {
                midi_track & t = _Tracks[i];

                for (unsigned j = 0; j < t.GetLength(); )
                {
                    if (t[j]._Type != midi_event::extended && (t[j]._ChannelNumber - 10 < 6))
                    {
                        t.RemoveEvent(j);
                    }
                    else
                    {
                        ++j;
                    }
                }
            }
            break;
    }
}

void midi_container::serialize_as_stream(unsigned long subSongIndex, std::vector<midi_stream_event> & midiStream, system_exclusive_table & p_system_exclusive, unsigned long & loop_start, unsigned long & loop_end, unsigned cleanFlags) const
{
    std::vector<uint8_t> data;
    std::vector<std::size_t> track_positions;
    std::vector<uint8_t> port_numbers;
    std::vector<std::string> device_names;

    size_t TrackCount = _Tracks.size();

    unsigned long tick_loop_start = get_timestamp_loop_start(subSongIndex);
    unsigned long tick_loop_end = get_timestamp_loop_end(subSongIndex);

    size_t local_loop_start = ~0UL;
    size_t local_loop_end = ~0UL;

    track_positions.resize(TrackCount, 0);
    port_numbers.resize(TrackCount, 0);
    device_names.resize(TrackCount);

    bool clean_emidi = !!(cleanFlags & clean_flag_emidi);
    bool clean_instruments = !!(cleanFlags & clean_flag_instruments);
    bool clean_banks = !!(cleanFlags & clean_flag_banks);

    if (clean_emidi)
    {
        for (size_t i = 0; i < TrackCount; ++i)
        {
            bool SkipTrack = false;

            const midi_track & Track = _Tracks[i];

            for (size_t j = 0; j < Track.GetLength(); ++j)
            {
                const midi_event & Event = Track[j];

                if ((Event._Type == midi_event::control_change) && Event._Data[0] == 110)
                {
                    if (Event._Data[1] != 0 && Event._Data[1] != 1 && Event._Data[1] != 127)
                    {
                        SkipTrack = true;
                        break;
                    }
                }
            }

            if (SkipTrack)
                track_positions[i] = Track.GetLength();
        }
    }

    if (_Format == 2)
    {
        for (size_t i = 0; i < TrackCount; ++i)
            if (i != subSongIndex)
                track_positions[i] = _Tracks[i].GetLength();
    }

    for (;;)
    {
        unsigned long NextTimestamp = ~0UL;

        size_t NextTrack = 0;

        for (size_t i = 0; i < TrackCount; ++i)
        {
            if (track_positions[i] >= _Tracks[i].GetLength())
                continue;

            if (_Tracks[i][track_positions[i]]._Timestamp < NextTimestamp)
            {
                NextTimestamp = _Tracks[i][track_positions[i]]._Timestamp;
                NextTrack = i;
            }
        }

        if (NextTimestamp == ~0UL)
            break;

        bool filtered = false;

        if (clean_instruments || clean_banks)
        {
            const midi_event & Event = _Tracks[NextTrack][track_positions[NextTrack]];

            if (clean_instruments && Event._Type == midi_event::program_change)
                filtered = true;
            else
            if (clean_banks && Event._Type == midi_event::control_change && (Event._Data[0] == 0x00 || Event._Data[0] == 0x20))
                filtered = true;
        }

        if (!filtered)
        {
            unsigned long TempoTrackIndex = 0;

            if (_Format == 2 && subSongIndex)
                TempoTrackIndex = subSongIndex;

            const midi_event & Event = _Tracks[NextTrack][track_positions[NextTrack]];

            if (local_loop_start == ~0UL && Event._Timestamp >= tick_loop_start)
                local_loop_start = midiStream.size();

            if (local_loop_end == ~0UL && Event._Timestamp > tick_loop_end)
                local_loop_end = midiStream.size();

            unsigned long timestamp_ms = timestamp_to_ms(Event._Timestamp, TempoTrackIndex);

            if (Event._Type != midi_event::extended)
            {
                if (device_names[NextTrack].length())
                {
                    size_t i, j;

                    for (i = 0, j = m_device_names[Event._ChannelNumber].size(); i < j; ++i)
                    {
                        if (::strcmp(m_device_names[Event._ChannelNumber][i].c_str(), device_names[NextTrack].c_str()) == 0)
                            break;
                    }

                    port_numbers[NextTrack] = (uint8_t) i;
                    device_names[NextTrack].clear();

                    limit_port_number(port_numbers[NextTrack]);
                }

                uint32_t event_code = ((Event._Type + 8) << 4) + Event._ChannelNumber;

                if (Event._DataSize >= 1)
                    event_code += Event._Data[0] << 8;

                if (Event._DataSize >= 2)
                    event_code += Event._Data[1] << 16;

                event_code += port_numbers[NextTrack] << 24;

                midiStream.push_back(midi_stream_event(timestamp_ms, event_code));
            }
            else
            {
                size_t DataSize = Event.GetDataSize();

                if (DataSize >= 3 && Event._Data[0] == 0xF0)
                {
                    if (device_names[NextTrack].length())
                    {
                        size_t i, j;

                        for (i = 0, j = m_device_names[Event._ChannelNumber].size(); i < j; ++i)
                        {
                            if (::strcmp(m_device_names[Event._ChannelNumber][i].c_str(), device_names[NextTrack].c_str()) == 0)
                                break;
                        }

                        port_numbers[NextTrack] = (uint8_t) i;
                        device_names[NextTrack].clear();
                        limit_port_number(port_numbers[NextTrack]);
                    }

                    data.resize(DataSize);
                    Event.GetData(&data[0], 0, DataSize);

                    if (data[DataSize - 1] == 0xF7)
                    {
                        uint32_t system_exclusive_index = p_system_exclusive.add_entry(&data[0], DataSize, port_numbers[NextTrack]);

                        midiStream.push_back(midi_stream_event(timestamp_ms, system_exclusive_index | 0x80000000));
                    }
                }
                else
                if (DataSize >= 3 && Event._Data[0] == 0xFF)
                {
                    if (Event._Data[1] == 4 || Event._Data[1] == 9)
                    {
                        size_t _data_count = Event.GetDataSize() - 2;

                        data.resize(_data_count);
                        Event.GetData(&data[0], 2, _data_count);

                        device_names[NextTrack].clear();
                        device_names[NextTrack].assign(data.begin(), data.begin() + _data_count);
                        std::transform(device_names[NextTrack].begin(), device_names[NextTrack].end(), device_names[NextTrack].begin(), ::tolower);
                    }
                    else
                    if (Event._Data[1] == 0x21)
                    {
                        port_numbers[NextTrack] = Event._Data[2];
                        device_names[NextTrack].clear();
                        limit_port_number(port_numbers[NextTrack]);
                    }
                }
                else
                if (DataSize == 1 && Event._Data[0] >= 0xF8)
                {
                    if (device_names[NextTrack].length())
                    {
                        size_t i, j;

                        for (i = 0, j = m_device_names[Event._ChannelNumber].size(); i < j; ++i)
                        {
                            if (::strcmp(m_device_names[Event._ChannelNumber][i].c_str(), device_names[NextTrack].c_str()) == 0)
                                break;
                        }

                        port_numbers[NextTrack] = (uint8_t) i;
                        device_names[NextTrack].clear();
                        limit_port_number(port_numbers[NextTrack]);
                    }

                    uint32_t EventCode = (uint32_t)(port_numbers[NextTrack] << 24);

                    EventCode += Event._Data[0];
                    midiStream.push_back(midi_stream_event(timestamp_ms, EventCode));
                }
            }
        }

        track_positions[NextTrack]++;
    }

    loop_start = (unsigned)local_loop_start;
    loop_end = (unsigned)local_loop_end;
}

void midi_container::serialize_as_standard_midi_file(std::vector<uint8_t> & midiStream) const
{
    if (_Tracks.size() == 0)
        return;

    const char signature[] = "MThd";

    midiStream.insert(midiStream.end(), signature, signature + 4);

    midiStream.push_back(0);
    midiStream.push_back(0);
    midiStream.push_back(0);
    midiStream.push_back(6);

    midiStream.push_back(0);
    midiStream.push_back((uint8_t) _Format);
    midiStream.push_back((uint8_t)(_Tracks.size() >> 8));
    midiStream.push_back((uint8_t) _Tracks.size());
    midiStream.push_back((uint8_t)(_Division >> 8));
    midiStream.push_back((uint8_t) _Division);

    std::vector<uint8_t> Data;

    for (size_t i = 0; i < _Tracks.size(); ++i)
    {
        const char _signature[] = "MTrk";

        midiStream.insert(midiStream.end(), _signature, _signature + 4);

        size_t ChunkSizeOffset = midiStream.size();

        midiStream.push_back(0);
        midiStream.push_back(0);
        midiStream.push_back(0);
        midiStream.push_back(0);

        const midi_track & Track = _Tracks[i];

        unsigned long LastTimestamp = 0;
        uint8_t LastEventCode = 0xFF;

        for (size_t j = 0; j < Track.GetLength(); ++j)
        {
            const midi_event & Event = Track[j];

            encode_delta(midiStream, Event._Timestamp - LastTimestamp);

            LastTimestamp = Event._Timestamp;

            if (Event._Type != midi_event::extended)
            {
                const uint8_t EventCode = (uint8_t)(((Event._Type + 8) << 4) + Event._ChannelNumber);

                if (EventCode != LastEventCode)
                {
                    midiStream.push_back(EventCode);
                    LastEventCode = EventCode;
                }

                midiStream.insert(midiStream.end(), Event._Data, Event._Data + Event._DataSize);
            }
            else
            {
                std::size_t DataSize = Event.GetDataSize();

                if (DataSize >= 1)
                {
                    if (Event._Data[0] == 0xF0)
                    {
                        --DataSize;
                        midiStream.push_back(0xF0);
                        encode_delta(midiStream, (unsigned)DataSize);

                        if (DataSize)
                        {
                            Data.resize(DataSize);
                            Event.GetData(&Data[0], 1, DataSize);
                            midiStream.insert(midiStream.end(), Data.begin(), Data.begin() + DataSize);
                        }
                    }
                    else
                    if (Event._Data[0] == 0xFF && (DataSize >= 2))
                    {
                        DataSize -= 2;
                        midiStream.push_back(0xFF);
                        midiStream.push_back(Event._Data[1]);
                        encode_delta(midiStream, (unsigned)DataSize);

                        if (DataSize)
                        {
                            Data.resize(DataSize);
                            Event.GetData(&Data[0], 2, DataSize);
                            midiStream.insert(midiStream.end(), Data.begin(), Data.begin() + DataSize);
                        }
                    }
                    else
                    {
                        Data.resize(DataSize);
                        Event.GetData(&Data[0], 1, DataSize);
                        midiStream.insert(midiStream.end(), Data.begin(), Data.begin() + DataSize);
                    }
                }
            }
        }

        size_t TrackLength = midiStream.size() - ChunkSizeOffset - 4;

        midiStream[ChunkSizeOffset + 0] = (uint8_t) (TrackLength >> 24);
        midiStream[ChunkSizeOffset + 1] = (uint8_t) (TrackLength >> 16);
        midiStream[ChunkSizeOffset + 2] = (uint8_t) (TrackLength >>  8);
        midiStream[ChunkSizeOffset + 3] = (uint8_t)  TrackLength;
    }
}

void midi_container::promote_to_type1()
{
    if (_Format == 0 && _Tracks.size() <= 2)
    {
        bool meter_track_present = false;

        midi_track new_tracks[17];
        midi_track original_data_track = _Tracks[_Tracks.size() - 1];

        if (_Tracks.size() > 1)
        {
            new_tracks[0] = _Tracks[0];
            meter_track_present = true;
        }

        _Tracks.resize(0);

        for (std::size_t i = 0; i < original_data_track.GetLength(); ++i)
        {
            const midi_event & event = original_data_track[i];

            if (event._Type != midi_event::extended)
            {
                new_tracks[1 + event._ChannelNumber].AddEvent(event);
            }
            else
            {
                if (event._Data[0] != 0xFF || event.GetDataSize() < 2 || event._Data[1] != 0x2F)
                {
                    new_tracks[0].AddEvent(event);
                }
                else
                {
                    if (!meter_track_present)
                        new_tracks[0].AddEvent(event);

                    for (std::size_t j = 1; j < 17; ++j)
                    {
                        new_tracks[j].AddEvent(event);
                    }
                }
            }
        }

        for (std::size_t i = 0; i < 17; ++i)
        {
            if (new_tracks[i].GetLength() > 1)
                add_track(new_tracks[i]);
        }

        _Format = 1;
    }
}

unsigned long midi_container::GetSubSongCount() const
{
    unsigned long subsong_count = 0;

    for (unsigned i = 0; i < m_channel_mask.size(); ++i)
    {
        if (m_channel_mask[i])
            ++subsong_count;
    }

    return subsong_count;
}

unsigned long midi_container::GetSubSong(size_t index) const
{
    for (size_t i = 0; i < m_channel_mask.size(); ++i)
    {
        if (m_channel_mask[i])
        {
            if (index == 0)
                return i;

            --index;
        }
    }

    return 0;
}

unsigned long midi_container::GetDuration(size_t subSongIndex, bool ms /* = false */) const
{
    size_t SubSongIndex = 0;

    unsigned long Timestamp = m_timestamp_end[0];

    if ((_Format == 2) && (subSongIndex != 0))
    {
        SubSongIndex = subSongIndex;

        Timestamp = m_timestamp_end[subSongIndex];
    }

    return ms ? Timestamp : timestamp_to_ms(Timestamp, SubSongIndex);
}

unsigned long midi_container::GetFormat() const
{
    return _Format;
}

unsigned long midi_container::GetTrackCount() const
{
    return (unsigned) _Tracks.size();
}

unsigned long midi_container::GetChannelCount(size_t subSongIndex) const
{
    unsigned long Count = 0;
    uint64_t j = 1;

    for (size_t i = 0; i < 48; ++i, j <<= 1)
    {
        if (m_channel_mask[subSongIndex] & j)
            ++Count;
    }

    return Count;
}

unsigned long midi_container::get_timestamp_loop_start(unsigned long subsong, bool ms /* = false */) const
{
    unsigned long tempo_track = 0;
    unsigned long timestamp = m_timestamp_loop_start[0];

    if (_Format == 2 && subsong)
    {
        tempo_track = subsong;
        timestamp = m_timestamp_loop_start[subsong];
    }

    if (!ms)
        return timestamp;

    if (timestamp != ~0UL)
        return timestamp_to_ms(timestamp, tempo_track);

    return ~0UL;
}

unsigned long midi_container::get_timestamp_loop_end(unsigned long subsong, bool ms /* = false */) const
{
    unsigned long tempo_track = 0;
    unsigned long timestamp = m_timestamp_loop_end[0];

    if (_Format == 2 && subsong)
    {
        tempo_track = subsong;
        timestamp = m_timestamp_loop_end[subsong];
    }

    if (!ms)
        return timestamp;

    if (timestamp != ~0UL)
        return timestamp_to_ms(timestamp, tempo_track);

    return ~0UL;
}

/* TODO: Use iconv or libintl or something to probe for code pages and convert some mess to UTF-8 */
static void convert_mess_to_utf8(const char * src, size_t srcLength, std::string & dst)
{
    dst.assign(src, src + srcLength);
}

void midi_container::GetMetaData(size_t subSongIndex, midi_meta_data & metaData)
{
    std::vector<uint8_t> Data;
    std::string Text;

    bool TypeFound = false;
    bool NonGMTypeFound = false;

    for (size_t i = 0; i < _Tracks.size(); ++i)
    {
        if ((_Format == 2) && (i != subSongIndex))
            continue;

        size_t TempoTrackIndex = (_Format == 2) ? i : 0;

        const midi_track & Track = _Tracks[i];

        for (size_t j = 0; j < Track.GetLength(); ++j)
        {
            const midi_event & Event = Track[j];

            if (Event._Type == midi_event::extended)
            {
                std::size_t DataSize = Event.GetDataSize();

                if (!NonGMTypeFound && (DataSize >= 1) && (Event._Data[0] == 0xF0))
                {
                    uint8_t test = 0, test2 = 0;

                    if (DataSize > 1)
                        test = Event._Data[1];

                    if (DataSize > 3)
                        test2 = Event._Data[3];

                    const char * TypeName = nullptr;

                    switch (test)
                    {
                        case 0x7E:
                            TypeFound = true;
                            break;

                        case 0x43:
                            TypeName = "XG";
                            break;

                        case 0x42:
                            TypeName = "X5";
                            break;

                        case 0x41:
                            if (test2 == 0x42)
                                TypeName = "GS";
                            else
                            if (test2 == 0x16)
                                TypeName = "MT-32";
                            else
                            if (test2 == 0x14)
                                TypeName = "D-50";
                    }

                    if (TypeName)
                    {
                        TypeFound = true;
                        NonGMTypeFound = true;

                        metaData.AddItem(midi_meta_data_item(timestamp_to_ms(Event._Timestamp, TempoTrackIndex), "type", TypeName));
                    }
                }
                else
                if ((DataSize > 2) && (Event._Data[0] == 0xFF))
                {
                    char Name[32];

                    DataSize -= 2;

                    switch (Event._Data[1])
                    {
                        case 6:
                            Data.resize(DataSize);
                            Event.GetData(&Data[0], 2, DataSize);
                            convert_mess_to_utf8((const char *) &Data[0], DataSize, Text);
                            metaData.AddItem(midi_meta_data_item(timestamp_to_ms(Event._Timestamp, TempoTrackIndex), "track_marker", Text.c_str()));
                            break;

                        case 2:
                            Data.resize(DataSize);
                            Event.GetData(&Data[0], 2, DataSize);
                            convert_mess_to_utf8((const char *) &Data[0], DataSize, Text);
                            metaData.AddItem(midi_meta_data_item(timestamp_to_ms(Event._Timestamp, TempoTrackIndex), "copyright", Text.c_str()));
                            break;

                        case 1:
                            Data.resize(DataSize);
                            Event.GetData(&Data[0], 2, DataSize);
                            convert_mess_to_utf8((const char *) &Data[0], DataSize, Text);

                            ::snprintf(Name, _countof(Name) - 1, "track_text_%02lu", (unsigned long)i);
                            metaData.AddItem(midi_meta_data_item(timestamp_to_ms(Event._Timestamp, TempoTrackIndex), Name, Text.c_str()));
                            break;

                        case 3:
                        case 4:
                            Data.resize(DataSize);
                            Event.GetData(&Data[0], 2, DataSize);
                            convert_mess_to_utf8((const char *) &Data[0], DataSize, Text);

                            ::snprintf(Name, _countof(Name) - 1, "track_name_%02lu", (unsigned long)i);
                            metaData.AddItem(midi_meta_data_item(timestamp_to_ms(Event._Timestamp, TempoTrackIndex), Name, Text.c_str()));
                            break;
                    }
                }
            }
        }
    }

    if (TypeFound && !NonGMTypeFound)
        metaData.AddItem(midi_meta_data_item(0, "type", "GM"));

    metaData.Append(m_extra_meta_data);
}

void midi_container::trim_tempo_map(size_t index, unsigned long base_timestamp)
{
    if (index < m_tempo_map.size())
    {
        tempo_map & map = m_tempo_map[index];

        for (size_t i = 0, j = map.get_count(); i < j; ++i)
        {
            tempo_entry & entry = map[i];

            if (entry.m_timestamp >= base_timestamp)
                entry.m_timestamp -= base_timestamp;
            else
                entry.m_timestamp = 0;
        }
    }
}

void midi_container::trim_range_of_tracks(size_t start, size_t end)
{
    unsigned long timestamp_first_note = ~0UL;

    for (size_t i = start; i <= end; ++i)
    {
        size_t j, k;

        const midi_track & track = _Tracks[i];

        for (j = 0, k = track.GetLength(); j < k; ++j)
        {
            const midi_event & event = track[j];

            if (event._Type == midi_event::note_on && event._Data[0])
                break;
        }

        if (j < k)
        {
            if (track[j]._Timestamp < timestamp_first_note)
                timestamp_first_note = track[j]._Timestamp;
        }
    }

    if (timestamp_first_note < ~0UL && timestamp_first_note > 0)
    {
        for (size_t i = start; i <= end; ++i)
        {
            midi_track & track = _Tracks[i];

            for (size_t j = 0, k = track.GetLength(); j < k; ++j)
            {
                midi_event & event = track[j];

                if (event._Timestamp >= timestamp_first_note)
                    event._Timestamp -= timestamp_first_note;
                else
                    event._Timestamp = 0;
            }
        }

        if (start == end)
        {
            trim_tempo_map(start, timestamp_first_note);

            m_timestamp_end[start] -= timestamp_first_note;

            if (m_timestamp_loop_end[start] != ~0UL)
                m_timestamp_loop_end[start] -= timestamp_first_note;

            if (m_timestamp_loop_start[start] != ~0UL)
            {
                if (m_timestamp_loop_start[start] > timestamp_first_note)
                    m_timestamp_loop_start[start] -= timestamp_first_note;
                else
                    m_timestamp_loop_start[start] = 0;
            }
        }
        else
        {
            trim_tempo_map(0, timestamp_first_note);

            m_timestamp_end[0] -= timestamp_first_note;

            if (m_timestamp_loop_end[0] != ~0UL)
                m_timestamp_loop_end[0] -= timestamp_first_note;

            if (m_timestamp_loop_start[0] != ~0UL)
            {
                if (m_timestamp_loop_start[0] > timestamp_first_note)
                    m_timestamp_loop_start[0] -= timestamp_first_note;
                else
                    m_timestamp_loop_start[0] = 0;
            }
        }
    }
}

void midi_container::trim_start()
{
    if (_Format == 2)
    {
        for (size_t i = 0, j = _Tracks.size(); i < j; ++i)
            trim_range_of_tracks(i, i);
    }
    else
        trim_range_of_tracks(0, _Tracks.size() - 1);
}

void midi_container::split_by_instrument_changes(SplitCallback callback)
{
    if (_Format != 1) /* This would literally die on anything else */
        return;

    for (size_t i = 0; i < _Tracks.size(); ++i)
    {
        midi_track SrcTrack = _Tracks[0];

        _Tracks.erase(_Tracks.begin());

        midi_track DstTrack;

        midi_track program_change;

        for (size_t k = 0; k < SrcTrack.GetLength(); ++k)
        {
            const midi_event & Event = SrcTrack[k];

            if (Event._Type == midi_event::program_change || (Event._Type == midi_event::control_change && (Event._Data[0] == 0x00 || Event._Data[0] == 0x20)))
            {
                program_change.AddEvent(Event);
            }
            else
            {
                if (program_change.GetLength() > 0)
                {
                    if (DstTrack.GetLength())
                        _Tracks.push_back(DstTrack);

                    DstTrack = program_change;

                    if (callback)
                    {
                        unsigned long timestamp = 0;
                        uint8_t bank_msb = 0, bank_lsb = 0, instrument = 0;

                        for (size_t m = 0; m < program_change.GetLength(); ++m)
                        {
                            const midi_event & ev = program_change[m];

                            if (ev._Type == midi_event::program_change)
                                instrument = ev._Data[0];
                            else
                                if (ev._Data[0] == 0)
                                    bank_msb = ev._Data[1];
                                else
                                    bank_lsb = ev._Data[1];

                            if (ev._Timestamp > timestamp)
                                timestamp = ev._Timestamp;
                        }

                        std::string Name = callback(bank_msb, bank_lsb, instrument);

                        std::vector<uint8_t> Data;

                        Data.resize(Name.length() + 2);

                        Data[0] = 0xFF;
                        Data[1] = 0x03;

                        std::copy(Name.begin(), Name.end(), Data.begin() + 2);

                        DstTrack.AddEvent(midi_event(timestamp, midi_event::extended, 0, &Data[0], Data.size()));
                    }

                    program_change = midi_track();
                }

                DstTrack.AddEvent(Event);
            }
        }

        if (DstTrack.GetLength())
            _Tracks.push_back(DstTrack);
    }
}

void midi_container::scan_for_loops(bool p_xmi_loops, bool p_marker_loops, bool p_rpgmaker_loops, bool p_touhou_loops)
{
    std::vector<uint8_t> data;

    size_t subsong_count = (_Format == 2) ? _Tracks.size() : 1;

    m_timestamp_loop_start.resize(subsong_count);
    m_timestamp_loop_end.resize(subsong_count);

    for (unsigned long i = 0; i < subsong_count; ++i)
    {
        m_timestamp_loop_start[i] = ~0UL;
        m_timestamp_loop_end[i] = ~0UL;
    }

    if (p_touhou_loops && _Format == 0)
    {
        bool loop_start_found = false;
        bool loop_end_found = false;
        bool errored = false;

        for (unsigned long i = 0; !errored && i < _Tracks.size(); ++i)
        {
            const midi_track & track = _Tracks[i];

            for (unsigned long j = 0; !errored && j < track.GetLength(); ++j)
            {
                const midi_event & event = track[j];

                if (event._Type == midi_event::control_change)
                {
                    if (event._Data[0] == 2)
                    {
                        if (event._Data[1] != 0)
                        {
                            errored = true;
                            break;
                        }

                        m_timestamp_loop_start[0] = event._Timestamp;
                        loop_start_found = true;
                    }

                    if (event._Data[0] == 4)
                    {
                        if (event._Data[1] != 0)
                        {
                            errored = true;
                            break;
                        }

                        m_timestamp_loop_end[0] = event._Timestamp;
                        loop_end_found = true;
                    }
                }
            }
        }

        if (errored)
        {
            m_timestamp_loop_start[0] = ~0UL;
            m_timestamp_loop_end[0] = ~0UL;
        }
    }

    if (p_rpgmaker_loops)
    {
        bool emidi_commands_found = false;

        for (unsigned long i = 0; i < _Tracks.size(); ++i)
        {
            unsigned long subsong = 0;

            if (_Format == 2)
                subsong = i;

            const midi_track & track = _Tracks[i];

            for (unsigned long j = 0; j < track.GetLength(); ++j)
            {
                const midi_event & event = track[j];

                if (event._Type == midi_event::control_change && (event._Data[0] == 110 || event._Data[0] == 111))
                {
                    if (event._Data[0] == 110)
                    {
                        emidi_commands_found = true;
                        break;
                    }

                    {
                        if (m_timestamp_loop_start[subsong] == ~0UL || m_timestamp_loop_start[subsong] > event._Timestamp)
                        {
                            m_timestamp_loop_start[subsong] = event._Timestamp;
                        }
                    }
                }
            }

            if (emidi_commands_found)
            {
                m_timestamp_loop_start[subsong] = ~0UL;
                m_timestamp_loop_end[subsong] = ~0UL;
                break;
            }
        }
    }

    if (p_xmi_loops)
    {
        for (unsigned long i = 0; i < _Tracks.size(); ++i)
        {
            unsigned long subsong = 0;

            if (_Format == 2)
                subsong = i;

            const midi_track & track = _Tracks[i];

            for (unsigned long j = 0; j < track.GetLength(); ++j)
            {
                const midi_event & event = track[j];

                if (event._Type == midi_event::control_change && (event._Data[0] >= 0x74 && event._Data[0] <= 0x77))
                {
                    if (event._Data[0] == 0x74 || event._Data[0] == 0x76)
                    {
                        if (m_timestamp_loop_start[subsong] == ~0UL || m_timestamp_loop_start[subsong] > event._Timestamp)
                        {
                            m_timestamp_loop_start[subsong] = event._Timestamp;
                        }
                    }
                    else
                    {
                        if (m_timestamp_loop_end[subsong] == ~0UL || m_timestamp_loop_end[subsong] < event._Timestamp)
                        {
                            m_timestamp_loop_end[subsong] = event._Timestamp;
                        }
                    }
                }
            }
        }
    }

    if (p_marker_loops)
    {
        for (size_t i = 0; i < _Tracks.size(); ++i)
        {
            size_t SubsongIndex = 0;

            if (_Format == 2)
                SubsongIndex = i;

            const midi_track & track = _Tracks[i];

            for (size_t j = 0; j < track.GetLength(); ++j)
            {
                const midi_event & event = track[j];

                if (event._Type == midi_event::extended && event.GetDataSize() >= 9 && event._Data[0] == 0xFF && event._Data[1] == 0x06)
                {
                    size_t data_count = event.GetDataSize() - 2;

                    data.resize(data_count);
                    event.GetData(&data[0], 2, data_count);

                    if (data_count == 9 && !strncasecmp((const char *) &data[0], "loopStart", 9))
                    {
                        if (m_timestamp_loop_start[SubsongIndex] == ~0UL || m_timestamp_loop_start[SubsongIndex] > event._Timestamp)
                        {
                            m_timestamp_loop_start[SubsongIndex] = event._Timestamp;
                        }
                    }
                    else
                        if (data_count == 7 && !strncasecmp((const char *) &data[0], "loopEnd", 7))
                        {
                            if (m_timestamp_loop_end[SubsongIndex] == ~0UL || m_timestamp_loop_end[SubsongIndex] < event._Timestamp)
                            {
                                m_timestamp_loop_end[SubsongIndex] = event._Timestamp;
                            }
                        }
                }
            }
        }
    }

    // Sanity
    for (size_t i = 0; i < subsong_count; ++i)
    {
        unsigned long timestamp_song_end;

        if (_Format == 2)
            timestamp_song_end = _Tracks[i][_Tracks[i].GetLength() - 1]._Timestamp;
        else
        {
            timestamp_song_end = 0;

            for (size_t j = 0; j < _Tracks.size(); ++j)
            {
                const midi_track & track = _Tracks[j];
                unsigned long timestamp = track[track.GetLength() - 1]._Timestamp;

                if (timestamp > timestamp_song_end)
                    timestamp_song_end = timestamp;
            }
        }

        if (m_timestamp_loop_start[i] != ~0UL && ((m_timestamp_loop_start[i] == m_timestamp_loop_end[i]) || (m_timestamp_loop_start[i] == timestamp_song_end)))
        {
            m_timestamp_loop_start[i] = ~0UL;
            m_timestamp_loop_end[i] = ~0UL;
        }
    }
}

void midi_container::encode_delta(std::vector<uint8_t> & p_out, unsigned long delta)
{
    unsigned shift = 7 * 4;

    while (shift && !(delta >> shift))
    {
        shift -= 7;
    }

    while (shift > 0)
    {
        p_out.push_back((unsigned char) (((delta >> shift) & 0x7F) | 0x80));
        shift -= 7;
    }

    p_out.push_back((unsigned char) (delta & 0x7F));
}

unsigned long midi_container::timestamp_to_ms(unsigned long p_timestamp, size_t subsongIndex) const
{
    unsigned long timestamp_ms = 0;
    unsigned long timestamp = 0;
    std::size_t tempo_index = 0;
    unsigned current_tempo = 500000;

    unsigned half_dtx = _Division * 500;
    unsigned p_dtx = half_dtx * 2;

    size_t subsong_count = m_tempo_map.size();

    if ((subsongIndex > 0) && (subsong_count > 0))
    {
        for (size_t i = std::min(subsongIndex, subsong_count); --i; )
        {
            size_t count = m_tempo_map[i].get_count();

            if (count > 0)
            {
                current_tempo = m_tempo_map[i][count - 1].m_tempo;
                break;
            }
        }
    }

    if (subsongIndex < subsong_count)
    {
        const tempo_map & m_entries = m_tempo_map[subsongIndex];

        std::size_t tempo_count = m_entries.get_count();

        while (tempo_index < tempo_count && timestamp + p_timestamp >= m_entries[tempo_index].m_timestamp)
        {
            unsigned long delta = m_entries[tempo_index].m_timestamp - timestamp;

            timestamp_ms += ((uint64_t) current_tempo * (uint64_t) delta + half_dtx) / p_dtx;
            current_tempo = m_entries[tempo_index].m_tempo;
            ++tempo_index;

            timestamp += delta;
            p_timestamp -= delta;
        }
    }

    timestamp_ms += ((uint64_t) current_tempo * (uint64_t) p_timestamp + half_dtx) / p_dtx;

    return timestamp_ms;
}
#pragma endregion
