#include "MIDIContainer.h"

#include <string.h>

#pragma warning(disable: 4242)
#include <algorithm>
#pragma warning(default: 4242)

#pragma region("MIDI Event")
MIDIEvent::MIDIEvent(const MIDIEvent & other)
{
    Timestamp = other.Timestamp;
    Type = other.Type;
    ChannelNumber = other.ChannelNumber;
    DataSize = other.DataSize;
    ::memcpy(Data, other.Data, DataSize);
    ExtendedData = other.ExtendedData;
}

MIDIEvent::MIDIEvent(uint32_t timestamp, EventType eventType, uint32_t channelNumber, const uint8_t * data, size_t size)
{
    Timestamp = timestamp;
    Type = eventType;
    ChannelNumber = channelNumber;

    if (size <= MaxStaticData)
    {
        DataSize = size;
        ::memcpy(Data, data, size);
    }
    else
    {
        DataSize = MaxStaticData;
        ::memcpy(Data, data, MaxStaticData);

        ExtendedData.assign(data + MaxStaticData, data + size);
    }
}

void MIDIEvent::GetData(uint8_t * data, size_t offset, size_t length) const
{
    size_t Size = GetDataSize();

    size_t max_count = (offset <= Size) ? Size - offset : 0;

    length = std::min(length, max_count);

    if (offset < MaxStaticData)
    {
        size_t _max_count = MaxStaticData - offset;
        size_t count = std::min(_max_count, length);

        ::memcpy(data, Data + offset, count);

        data += count;
        length -= count;
    }

    if (length > 0)
        ::memcpy(data, &ExtendedData[0], length);
}
#pragma endregion

#pragma region("MIDI Track")
MIDITrack::MIDITrack(const MIDITrack & track)
{
    _Events = track._Events;
}

void MIDITrack::AddEvent(const MIDIEvent & newEvent)
{
    auto it = _Events.end();

    if (_Events.size() > 0)
    {
        MIDIEvent & Event = *(it - 1);

        if ((Event.Type == MIDIEvent::Extended) && (Event.GetDataSize() >= 2) && (Event.Data[0] == StatusCodes::MetaData) && (Event.Data[1] == MetaDataTypes::EndOfTrack))
        {
            --it;

            if (Event.Timestamp < newEvent.Timestamp)
                Event.Timestamp = newEvent.Timestamp;
        }

        while (it > _Events.begin())
        {
            if ((*(it - 1)).Timestamp <= newEvent.Timestamp)
                break;

            --it;
        }
    }

    _Events.insert(it, newEvent);
}

void MIDITrack::RemoveEvent(size_t index)
{
    _Events.erase(_Events.begin() + (int)index);
}

#pragma endregion

#pragma region("Tempo Map")
TempoEntry::TempoEntry(uint32_t timestamp, uint32_t tempo)
{
    Timestamp = timestamp;
    Tempo = tempo;
}

void TempoMap::Add(uint32_t tempo, uint32_t timestamp)
{
    auto it = _Entries.end();

    while (it > _Entries.begin())
    {
        if ((*(it - 1)).Timestamp <= timestamp)
            break;
        --it;
    }

    if (it > _Entries.begin() && (*(it - 1)).Timestamp == timestamp)
    {
        (*(it - 1)).Tempo = tempo;
    }
    else
    {
        _Entries.insert(it, TempoEntry(timestamp, tempo));
    }
}

uint32_t TempoMap::TimestampToMS(uint32_t p_timestamp, uint32_t division) const
{
    uint32_t TimestampInMS = 0;

    auto Iterator = _Entries.begin();

    uint32_t Tempo = 500000;

    uint32_t Timestamp = 0;
    uint32_t HalfDivision = division * 500;
    division = HalfDivision * 2;

    while ((Iterator < _Entries.end()) && (Timestamp + p_timestamp >= (*Iterator).Timestamp))
    {
        uint32_t Delta = (*Iterator).Timestamp - Timestamp;

        TimestampInMS += ((uint64_t) Tempo * (uint64_t) Delta + HalfDivision) / division;

        Tempo = (*Iterator).Tempo;
        ++Iterator;

        Timestamp += Delta;
        p_timestamp -= Delta;
    }

    TimestampInMS += ((uint64_t) Tempo * (uint64_t) p_timestamp + HalfDivision) / division;

    return TimestampInMS;
}
#pragma endregion

#pragma region("System Exclusive Table")
SysExEntry::SysExEntry(const SysExEntry & p_in)
{
    m_port = p_in.m_port;
    m_offset = p_in.m_offset;
    m_length = p_in.m_length;
}

SysExEntry::SysExEntry(std::size_t p_port, std::size_t p_offset, std::size_t p_length)
{
    m_port = p_port;
    m_offset = p_offset;
    m_length = p_length;
}

uint32_t SysExTable::AddEntry(const uint8_t * p_data, std::size_t p_size, std::size_t p_port)
{
    for (auto it = m_entries.begin(); it < m_entries.end(); ++it)
    {
        const SysExEntry & entry = *it;

        if (p_port == entry.m_port && p_size == entry.m_length && !memcmp(p_data, &m_data[entry.m_offset], p_size))
            return ((uint32_t) (it - m_entries.begin()));
    }

    SysExEntry entry(p_port, m_data.size(), p_size);

    m_data.insert(m_data.end(), p_data, p_data + p_size);
    m_entries.push_back(entry);

    return ((uint32_t) (m_entries.size() - 1));
}

void SysExTable::GetEntry(uint32_t p_index, const uint8_t *& p_data, std::size_t & p_size, std::size_t & p_port)
{
    const SysExEntry & entry = m_entries[p_index];

    p_data = &m_data[entry.m_offset];
    p_size = entry.m_length;
    p_port = entry.m_port;
}
#pragma endregion

#pragma region("MIDI Stream Event")
MIDIStreamEvent::MIDIStreamEvent(uint32_t timestamp, uint32_t data)
{
    Timestamp = timestamp;
    Data = data;
}
#pragma endregion

#pragma region("MIDI Meta Data")
MIDIMetaDataItem::MIDIMetaDataItem(const MIDIMetaDataItem & item)
{
    Timestamp = item.Timestamp;
    Name = item.Name;
    Value = item.Value;
}

MIDIMetaDataItem::MIDIMetaDataItem(uint32_t timestamp, const char * name, const char * value)
{
    Timestamp = timestamp;
    Name = name;
    Value = value;
}

void MIDIMetaData::AddItem(const MIDIMetaDataItem & item)
{
    _Items.push_back(item);
}

void MIDIMetaData::Append(const MIDIMetaData & data)
{
    _Items.insert(_Items.end(), data._Items.begin(), data._Items.end());
    _Bitmap = data._Bitmap;
}

bool MIDIMetaData::GetItem(const char * name, MIDIMetaDataItem & item) const
{
    for (size_t i = 0; i < _Items.size(); ++i)
    {
        const MIDIMetaDataItem & Item = _Items[i];

        if (_stricmp(name, Item.Name.c_str()) == 0)
        {
            item = Item;
            return true;
        }
    }

    return false;
}

bool MIDIMetaData::GetBitmap(std::vector<uint8_t> & bitmap)
{
    bitmap = _Bitmap;

    return bitmap.size() != 0;
}

void MIDIMetaData::AssignBitmap(std::vector<uint8_t>::const_iterator const & begin, std::vector<uint8_t>::const_iterator const & end)
{
    _Bitmap.assign(begin, end);
}

std::size_t MIDIMetaData::GetCount() const
{
    return _Items.size();
}

const MIDIMetaDataItem & MIDIMetaData::operator [] (std::size_t p_index) const
{
    return _Items[p_index];
}
#pragma endregion

#pragma region("MIDI Container")
void MIDIContainer::Initialize(uint32_t format, uint32_t division)
{
    _Format = format;
    _Division = division;

    if (format != 2)
    {
        _ChannelMask.resize(1);
        _ChannelMask[0] = 0;
        _TempoMaps.resize(1);
        _EndTimestamps.resize(1);
        _EndTimestamps[0] = 0;
        _LoopBeginTimestamps.resize(1);
        _LoopEndTimestamps.resize(1);
    }
}

void MIDIContainer::AddTrack(const MIDITrack & track)
{
    _Tracks.push_back(track);

    std::string DeviceName;
    uint8_t PortNumber = 0;

    size_t EventIndex;

    for (EventIndex = 0; EventIndex < track.GetLength(); ++EventIndex)
    {
        const MIDIEvent & Event = track[EventIndex];

        if (Event.Type == MIDIEvent::Extended)
        {
            if ((Event.GetDataSize() >= 5) && (Event.Data[0] == StatusCodes::MetaData) && (Event.Data[1] == MetaDataTypes::SetTempo))
            {
                uint32_t Tempo = (uint32_t)((Event.Data[2] << 16) | (Event.Data[3] << 8) | Event.Data[4]);

                if (_Format != 2)
                {
                    _TempoMaps[0].Add(Tempo, Event.Timestamp);
                }
                else
                {
                    _TempoMaps.resize(_Tracks.size());
                    _TempoMaps[_Tracks.size() - 1].Add(Tempo, Event.Timestamp);
                }
            }
            else
            if ((Event.GetDataSize() >= 3) && (Event.Data[0] == StatusCodes::MetaData))
            {
                if (Event.Data[1] == MetaDataTypes::InstrumentName || Event.Data[1] == MetaDataTypes::DeviceName)
                {
                    size_t Size = Event.GetDataSize() - 2;

                    std::vector<uint8_t> Data(Size);
                    Event.GetData(&Data[0], 2, Size);

                    DeviceName.assign(Data.begin(), Data.begin() + (int)Size);
                    std::transform(DeviceName.begin(), DeviceName.end(), DeviceName.begin(), ::tolower);
                }
                else
                if (Event.Data[1] == MetaDataTypes::MIDIPort)
                {
                    PortNumber = Event.Data[2];

                    limit_port_number(PortNumber);
                    DeviceName.clear();
                }
            }
        }
        else
        if (Event.Type == MIDIEvent::NoteOn || Event.Type == MIDIEvent::NoteOff)
        {
            uint32_t ChannelNumber = Event.ChannelNumber;

            if (DeviceName.length())
            {
                size_t j, k;

                for (j = 0, k = _DeviceNames[ChannelNumber].size(); j < k; ++j)
                {
                    if (::strcmp(_DeviceNames[ChannelNumber][j].c_str(), DeviceName.c_str()) == 0)
                        break;
                }

                if (j < k)
                    PortNumber = (uint8_t)j;
                else
                {
                    _DeviceNames[ChannelNumber].push_back(DeviceName);
                    PortNumber = (uint8_t)k;
                }

                limit_port_number(PortNumber);
                DeviceName.clear();
            }

            ChannelNumber += 16 * PortNumber;
            ChannelNumber %= 48;

            if (_Format != 2)
                _ChannelMask[0] |= 1ULL << ChannelNumber;
            else
            {
                _ChannelMask.resize(_Tracks.size(), 0);
                _ChannelMask[_Tracks.size() - 1] |= 1ULL << ChannelNumber;
            }
        }
    }

    if ((_Format != 2) && (EventIndex > 0) && (track[EventIndex - 1].Timestamp > _EndTimestamps[0]))
    {
        _EndTimestamps[0] = track[EventIndex - 1].Timestamp;
    }
    else
    if (_Format == 2)
    {
        if (EventIndex > 0)
            _EndTimestamps.push_back(track[EventIndex - 1].Timestamp);
        else
            _EndTimestamps.push_back((uint32_t) 0);
    }
}

void MIDIContainer::AddEventToTrack(size_t trackNumber, const MIDIEvent & event)
{
    MIDITrack & Track = _Tracks[trackNumber];

    Track.AddEvent(event);

    if ((event.Type == MIDIEvent::Extended) && (event.GetDataSize() >= 5) && (event.Data[0] == StatusCodes::MetaData) && (event.Data[1] == MetaDataTypes::SetTempo))
    {
        uint32_t Tempo = (uint32_t)((event.Data[2] << 16) | (event.Data[3] << 8) | event.Data[4]);

        if (_Format != 2)
        {
            _TempoMaps[0].Add(Tempo, event.Timestamp);
        }
        else
        {
            _TempoMaps.resize(_Tracks.size());
            _TempoMaps[trackNumber].Add(Tempo, event.Timestamp);
        }
    }
    else
    if (event.Type == MIDIEvent::NoteOn || event.Type == MIDIEvent::NoteOff)
    {
        if (_Format != 2)
        {
            _ChannelMask[0] |= 1ULL << event.ChannelNumber;
        }
        else
        {
            _ChannelMask.resize(_Tracks.size(), 0);
            _ChannelMask[trackNumber] |= 1ULL << event.ChannelNumber;
        }
    }

    if ((_Format != 2) && (event.Timestamp > _EndTimestamps[0]))
    {
        _EndTimestamps[0] = event.Timestamp;
    }
    else
    if ((_Format == 2) && (event.Timestamp > _EndTimestamps[trackNumber]))
    {
        _EndTimestamps[trackNumber] = event.Timestamp;
    }
}

void MIDIContainer::MergeTracks(const MIDIContainer & source)
{
    for (size_t i = 0; i < source._Tracks.size(); i++)
        AddTrack(source._Tracks[i]);
}

void MIDIContainer::SetTrackCount(uint32_t count)
{
    _Tracks.resize(count);
}

void MIDIContainer::SetExtraMetaData(const MIDIMetaData & data)
{
    _ExtraMetaData = data;
}

void MIDIContainer::ApplyHack(uint32_t hack)
{
    switch (hack)
    {
        case 0: // Hack 0: Remove channel 16
            for (size_t i = 0; i < _Tracks.size(); ++i)
            {
                MIDITrack & t = _Tracks[i];

                for (size_t j = 0; j < t.GetLength(); )
                {
                    if ((t[j].Type != MIDIEvent::Extended) && (t[j].ChannelNumber == 16))
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

        case 1: // Hack 1: Remove channels 11-16
            for (size_t i = 0; i < _Tracks.size(); ++i)
            {
                MIDITrack & t = _Tracks[i];

                for (size_t j = 0; j < t.GetLength(); )
                {
                    if (t[j].Type != MIDIEvent::Extended && (t[j].ChannelNumber - 10 < 6))
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

void MIDIContainer::SerializeAsStream(size_t subSongIndex, std::vector<MIDIStreamEvent> & midiStream, SysExTable & sysExTable, uint32_t & loopBegin, uint32_t & loopEnd, uint32_t cleanFlags) const
{
    std::vector<uint8_t> data;
    std::vector<std::size_t> TrackPositions;
    std::vector<uint8_t> PortNumbers;
    std::vector<std::string> DeviceNames;

    size_t TrackCount = _Tracks.size();

    uint32_t LoopBeginTimestamp = GetLoopBeginTimestamp(subSongIndex);
    uint32_t LoopEndTimestamp = GetLoopEndTimestamp(subSongIndex);

    size_t LoopBegin = ~0UL;
    size_t LoopEnd = ~0UL;

    TrackPositions.resize(TrackCount, 0);
    PortNumbers.resize(TrackCount, 0);
    DeviceNames.resize(TrackCount);

    bool CleanEMIDI = !!(cleanFlags & CleanFlagEMIDI);
    bool CleanInstruments = !!(cleanFlags & clean_flag_instruments);
    bool CleanBanks = !!(cleanFlags & clean_flag_banks);

    if (CleanEMIDI) // Apogee Expanded MIDI (EMIDI) API v1.1
    {
        for (size_t i = 0; i < TrackCount; ++i)
        {
            bool SkipTrack = false;

            const MIDITrack & Track = _Tracks[i];

            for (size_t j = 0; j < Track.GetLength(); ++j)
            {
                const MIDIEvent & Event = Track[j];

                if ((Event.Type == MIDIEvent::ControlChange) && (Event.Data[0] == 110))
                {
                    if (Event.Data[1] != 0 && Event.Data[1] != 1 && Event.Data[1] != 127)
                    {
                        SkipTrack = true;
                        break;
                    }
                }
            }

            if (SkipTrack)
                TrackPositions[i] = Track.GetLength();
        }
    }

    if (_Format == 2)
    {
        for (size_t i = 0; i < TrackCount; ++i)
            if (i != subSongIndex)
                TrackPositions[i] = _Tracks[i].GetLength();
    }

    for (;;)
    {
        uint32_t NextTimestamp = ~0UL;
        size_t NextTrack = 0;

        for (size_t i = 0; i < TrackCount; ++i)
        {
            if (TrackPositions[i] >= _Tracks[i].GetLength())
                continue;

            if (_Tracks[i][TrackPositions[i]].Timestamp < NextTimestamp)
            {
                NextTimestamp = _Tracks[i][TrackPositions[i]].Timestamp;
                NextTrack = i;
            }
        }

        if (NextTimestamp == ~0UL)
            break;

        bool IsEventFiltered = false;

        if (CleanInstruments || CleanBanks)
        {
            const MIDIEvent & Event = _Tracks[NextTrack][TrackPositions[NextTrack]];

            if (CleanInstruments && (Event.Type == MIDIEvent::ProgramChange))
                IsEventFiltered = true;
            else
            if (CleanBanks && (Event.Type == MIDIEvent::ControlChange) && (Event.Data[0] == 0x00 || Event.Data[0] == 0x20))
                IsEventFiltered = true;
        }

        if (!IsEventFiltered)
        {
            uint32_t TempoTrackIndex = 0;

            if (_Format == 2 && subSongIndex)
                TempoTrackIndex = (uint32_t)subSongIndex;

            const MIDIEvent & Event = _Tracks[NextTrack][TrackPositions[NextTrack]];

            if ((LoopBegin == ~0UL) && (Event.Timestamp >= LoopBeginTimestamp))
                LoopBegin = midiStream.size();

            if ((LoopEnd == ~0UL) && (Event.Timestamp > LoopEndTimestamp))
                LoopEnd = midiStream.size();

            uint32_t TimestampInMS = TimestampToMS(Event.Timestamp, TempoTrackIndex);

            if (Event.Type != MIDIEvent::Extended)
            {
                if (DeviceNames[NextTrack].length())
                {
                    size_t i, j;

                    for (i = 0, j = _DeviceNames[Event.ChannelNumber].size(); i < j; ++i)
                    {
                        if (::strcmp(_DeviceNames[Event.ChannelNumber][i].c_str(), DeviceNames[NextTrack].c_str()) == 0)
                            break;
                    }

                    PortNumbers[NextTrack] = (uint8_t) i;
                    DeviceNames[NextTrack].clear();

                    limit_port_number(PortNumbers[NextTrack]);
                }

                uint32_t event_code = ((Event.Type + 8) << 4) + Event.ChannelNumber;

                if (Event.DataSize >= 1)
                    event_code += Event.Data[0] << 8;

                if (Event.DataSize >= 2)
                    event_code += Event.Data[1] << 16;

                event_code += PortNumbers[NextTrack] << 24;

                midiStream.push_back(MIDIStreamEvent(TimestampInMS, event_code));
            }
            else
            {
                size_t DataSize = Event.GetDataSize();

                if ((DataSize >= 3) && (Event.Data[0] == StatusCodes::SysEx))
                {
                    if (DeviceNames[NextTrack].length())
                    {
                        size_t i, j;

                        for (i = 0, j = _DeviceNames[Event.ChannelNumber].size(); i < j; ++i)
                        {
                            if (::strcmp(_DeviceNames[Event.ChannelNumber][i].c_str(), DeviceNames[NextTrack].c_str()) == 0)
                                break;
                        }

                        PortNumbers[NextTrack] = (uint8_t) i;
                        DeviceNames[NextTrack].clear();
                        limit_port_number(PortNumbers[NextTrack]);
                    }

                    data.resize(DataSize);
                    Event.GetData(&data[0], 0, DataSize);

                    if (data[DataSize - 1] == 0xF7)
                    {
                        uint32_t system_exclusive_index = sysExTable.AddEntry(&data[0], DataSize, PortNumbers[NextTrack]);

                        midiStream.push_back(MIDIStreamEvent(TimestampInMS, system_exclusive_index | 0x80000000));
                    }
                }
                else
                if ((DataSize >= 3) && (Event.Data[0] == StatusCodes::MetaData))
                {
                    if (Event.Data[1] == MetaDataTypes::InstrumentName || Event.Data[1] == MetaDataTypes::DeviceName)
                    {
                        DataSize -= 2;

                        data.resize(DataSize);
                        Event.GetData(&data[0], 2, DataSize);

                        DeviceNames[NextTrack].clear();
                        DeviceNames[NextTrack].assign(data.begin(), data.begin() + (int)DataSize);
                        std::transform(DeviceNames[NextTrack].begin(), DeviceNames[NextTrack].end(), DeviceNames[NextTrack].begin(), ::tolower);
                    }
                    else
                    if (Event.Data[1] == MetaDataTypes::MIDIPort)
                    {
                        PortNumbers[NextTrack] = Event.Data[2];
                        DeviceNames[NextTrack].clear();
                        limit_port_number(PortNumbers[NextTrack]);
                    }
                }
                else
                if ((DataSize == 1) && (Event.Data[0] > StatusCodes::SysExEnd))
                {
                    if (DeviceNames[NextTrack].length())
                    {
                        size_t i, j;

                        for (i = 0, j = _DeviceNames[Event.ChannelNumber].size(); i < j; ++i)
                        {
                            if (::strcmp(_DeviceNames[Event.ChannelNumber][i].c_str(), DeviceNames[NextTrack].c_str()) == 0)
                                break;
                        }

                        PortNumbers[NextTrack] = (uint8_t) i;
                        DeviceNames[NextTrack].clear();
                        limit_port_number(PortNumbers[NextTrack]);
                    }

                    uint32_t EventCode = (uint32_t)(PortNumbers[NextTrack] << 24);

                    EventCode += Event.Data[0];
                    midiStream.push_back(MIDIStreamEvent(TimestampInMS, EventCode));
                }
            }
        }

        TrackPositions[NextTrack]++;
    }

    loopBegin = (uint32_t)LoopBegin;
    loopEnd = (uint32_t)LoopEnd;
}

void MIDIContainer::SerializeAsSMF(std::vector<uint8_t> & midiStream) const
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
        const char ChunkType[] = "MTrk";

        midiStream.insert(midiStream.end(), ChunkType, ChunkType + 4);

        size_t ChunkSizeOffset = midiStream.size();

        midiStream.push_back(0);
        midiStream.push_back(0);
        midiStream.push_back(0);
        midiStream.push_back(0);

        const MIDITrack & Track = _Tracks[i];

        uint32_t LastTimestamp = 0;
        uint8_t LastEventCode = StatusCodes::MetaData;

        for (size_t j = 0; j < Track.GetLength(); ++j)
        {
            const MIDIEvent & Event = Track[j];

            EncodeVariableLengthQuantity(midiStream, Event.Timestamp - LastTimestamp);

            LastTimestamp = Event.Timestamp;

            if (Event.Type != MIDIEvent::Extended)
            {
                const uint8_t EventCode = (uint8_t)(((Event.Type + 8) << 4) + Event.ChannelNumber);

                if (EventCode != LastEventCode)
                {
                    midiStream.push_back(EventCode);
                    LastEventCode = EventCode;
                }

                midiStream.insert(midiStream.end(), Event.Data, Event.Data + Event.DataSize);
            }
            else
            {
                size_t DataSize = Event.GetDataSize();

                if (DataSize >= 1)
                {
                    if (Event.Data[0] == StatusCodes::SysEx)
                    {
                        --DataSize;
                        midiStream.push_back(StatusCodes::SysEx);
                        EncodeVariableLengthQuantity(midiStream, (uint32_t)DataSize);

                        if (DataSize)
                        {
                            Data.resize(DataSize);
                            Event.GetData(&Data[0], 1, DataSize);
                            midiStream.insert(midiStream.end(), Data.begin(), Data.begin() + (int)DataSize);
                        }
                    }
                    else
                    if (Event.Data[0] == StatusCodes::MetaData && (DataSize >= 2))
                    {
                        DataSize -= 2;
                        midiStream.push_back(0xFF);
                        midiStream.push_back(Event.Data[1]);
                        EncodeVariableLengthQuantity(midiStream, (uint32_t)DataSize);

                        if (DataSize)
                        {
                            Data.resize(DataSize);
                            Event.GetData(&Data[0], 2, DataSize);
                            midiStream.insert(midiStream.end(), Data.begin(), Data.begin() + (int)DataSize);
                        }
                    }
                    else
                    {
                        Data.resize(DataSize);
                        Event.GetData(&Data[0], 1, DataSize);
                        midiStream.insert(midiStream.end(), Data.begin(), Data.begin() + (int)DataSize);
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

void MIDIContainer::PromoteToType1()
{
    if (_Format != 0)
        return;

    if (_Tracks.size() > 2)
        return;

    bool meter_track_present = false;

    MIDITrack new_tracks[17];
    MIDITrack original_data_track = _Tracks[_Tracks.size() - 1];

    if (_Tracks.size() > 1)
    {
        new_tracks[0] = _Tracks[0];
        meter_track_present = true;
    }

    _Tracks.resize(0);

    for (std::size_t i = 0; i < original_data_track.GetLength(); ++i)
    {
        const MIDIEvent & event = original_data_track[i];

        if (event.Type != MIDIEvent::Extended)
        {
            new_tracks[1 + event.ChannelNumber].AddEvent(event);
        }
        else
        if (event.Data[0] != StatusCodes::MetaData || event.GetDataSize() < 2 || event.Data[1] != MetaDataTypes::EndOfTrack)
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

    for (std::size_t i = 0; i < 17; ++i)
    {
        if (new_tracks[i].GetLength() > 1)
            AddTrack(new_tracks[i]);
    }

    _Format = 1;
}

size_t MIDIContainer::GetSubSongCount() const
{
    size_t SubSongCount = 0;

    for (size_t i = 0; i < _ChannelMask.size(); ++i)
    {
        if (_ChannelMask[i])
            ++SubSongCount;
    }

    return SubSongCount;
}

size_t MIDIContainer::GetSubSong(size_t index) const
{
    for (size_t i = 0; i < _ChannelMask.size(); ++i)
    {
        if (_ChannelMask[i])
        {
            if (index == 0)
                return i;

            --index;
        }
    }

    return 0;
}

uint32_t MIDIContainer::GetDuration(size_t subSongIndex, bool ms /* = false */) const
{
    size_t SubSongIndex = 0;

    uint32_t Timestamp = _EndTimestamps[0];

    if ((_Format == 2) && (subSongIndex != 0))
    {
        SubSongIndex = subSongIndex;

        Timestamp = _EndTimestamps[subSongIndex];
    }

    return ms ? TimestampToMS(Timestamp, SubSongIndex) : Timestamp;
}

uint32_t MIDIContainer::GetFormat() const
{
    return _Format;
}

uint32_t MIDIContainer::GetTrackCount() const
{
    return (uint32_t) _Tracks.size();
}

uint32_t MIDIContainer::GetChannelCount(size_t subSongIndex) const
{
    uint32_t Count = 0;
    uint64_t j = 1;

    for (size_t i = 0; i < 48; ++i, j <<= 1)
    {
        if (_ChannelMask[subSongIndex] & j)
            ++Count;
    }

    return Count;
}

uint32_t MIDIContainer::GetLoopBeginTimestamp(size_t subSongIndex, bool ms /* = false */) const
{
    size_t TrackIndex = 0;

    uint32_t Timestamp = _LoopBeginTimestamps[0];

    if ((_Format == 2) && (subSongIndex > 0))
    {
        TrackIndex = subSongIndex;
        Timestamp = _LoopBeginTimestamps[subSongIndex];
    }

    if (!ms)
        return Timestamp;

    if (Timestamp != ~0UL)
        return TimestampToMS(Timestamp, TrackIndex);

    return ~0UL;
}

uint32_t MIDIContainer::GetLoopEndTimestamp(size_t subSongIndex, bool ms /* = false */) const
{
    size_t TrackIndex = 0;

    uint32_t Timestamp = _LoopEndTimestamps[0];

    if ((_Format == 2) && (subSongIndex > 0))
    {
        TrackIndex = subSongIndex;
        Timestamp = _LoopEndTimestamps[subSongIndex];
    }

    if (!ms)
        return Timestamp;

    if (Timestamp != ~0UL)
        return TimestampToMS(Timestamp, TrackIndex);

    return ~0UL;
}

void MIDIContainer::GetMetaData(size_t subSongIndex, MIDIMetaData & metaData)
{
    bool TypeFound = false;
    bool IsSoftKaraoke = false;

    for (size_t i = 0; i < _Tracks.size(); ++i)
    {
        if ((_Format == 2) && (i != subSongIndex))
            continue;

        size_t TempoTrackIndex = (_Format == 2) ? i : 0;

        const MIDITrack & Track = _Tracks[i];

        for (size_t j = 0; j < Track.GetLength(); ++j)
        {
            const MIDIEvent & Event = Track[j];

            if (Event.Type != MIDIEvent::Extended)
                continue;

            size_t DataSize = Event.GetDataSize();

            if ((DataSize >= 1) && (Event.Data[0] == StatusCodes::SysEx) && !TypeFound)
            {
                const char * TypeName = nullptr;

                switch (Event.Data[1])
                {
                    case 0x7E:
                        TypeName = "GM";
                        break;

                    case 0x43:
                        TypeName = "XG";
                        break;

                    case 0x42:
                        TypeName = "X5";
                        break;

                    case 0x41:
                    {
                        if (DataSize > 3)
                        {
                            switch (Event.Data[3])
                            {
                                case 0x42:
                                    TypeName = "GS";
                                    break;

                                case 0x16:
                                    TypeName = "MT-32";
                                    break;

                                case 0x14:
                                    TypeName = "D-50";
                                    break;
                            }
                        }
                        break;
                    }
                }

                if (TypeName && ::_strnicmp(TypeName, "GM", 2) != 0)
                {
                    TypeFound = true;

                    metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "type", TypeName));
                }
            }
            else
            if ((DataSize > 2) && (Event.Data[0] == StatusCodes::MetaData))
            {
                char Name[32];

                std::vector<uint8_t> Data;
                std::string Text;

                DataSize -= 2;

                Data.resize(DataSize);
                Event.GetData(&Data[0], 2, DataSize);

                switch (Event.Data[1])
                {
                    case MetaDataTypes::Text:
                    {
                        if (!IsSoftKaraoke)
                        {
                            IsSoftKaraoke = (DataSize >= 19) && (::_strnicmp((const char *) &Data[0], "@KMIDI KARAOKE FILE", 19) == 0);

                            if (IsSoftKaraoke)
                            {
                                metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "lyrics_type", "Soft Karaoke"));
                            }
                            else
                            {
                                ::sprintf_s(Name, _countof(Name), "track_text_%02zd", i);
                                AssignString((const char *) &Data[0], DataSize, Text);

                                metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), Name, Text.c_str()));
                            }
                        }
                        else
                        {
                            if ((DataSize >= 2) && (_strnicmp((const char *) &Data[0], "@K", 2) == 0))
                            {
                                AssignString((const char *) &Data[2], DataSize - 2, Text);

                                metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "soft_karaoke_version", Text.c_str()));
                            }
                            else
                            if ((DataSize >= 2) && (_strnicmp((const char *) &Data[0], "@L", 2) == 0))
                            {
                                AssignString((const char *) &Data[2], DataSize - 2, Text);

                                metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "soft_karaoke_language", Text.c_str()));
                            }
                            else
                            if ((DataSize >= 2) && (_strnicmp((const char *) &Data[0], "@T", 2) == 0))
                            {
                                AssignString((const char *) &Data[2], DataSize - 2, Text);

                                metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "soft_karaoke_text", Text.c_str()));
                            }
                            else
                            if ((DataSize >= 2) && (_strnicmp((const char *) &Data[0], "@I", 2) == 0))
                            {
                                AssignString((const char *) &Data[2], DataSize - 2, Text);

                                metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "soft_karaoke_info", Text.c_str()));
                            }
                            else
                            if ((DataSize >= 2) && (_strnicmp((const char *) &Data[0], "@W", 2) == 0))
                            {
                                AssignString((const char *) &Data[2], DataSize - 2, Text);

                                metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "soft_karaoke_words", Text.c_str()));
                            }
                            else
                            if ((DataSize >= 2) && (Data[0] == '@'))
                            {
                                // Unknown Soft Karaoke tag
                                ::sprintf_s(Name, _countof(Name), "track_text_%02zd", i);
                                AssignString((const char *) &Data[0], DataSize, Text);

                                metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), Name, Text.c_str()));
                            }
                            else
                            {
                                AssignString((const char *) &Data[0], DataSize, Text);

                                metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "soft_karaoke_lyrics", Text.c_str()));
                            }
                        }
                        break;
                    }

                    case MetaDataTypes::Copyright:
                    {
                        AssignString((const char *) &Data[0], DataSize, Text);

                        metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "copyright", Text.c_str()));
                        break;
                    }

                    case MetaDataTypes::TrackName:
                    case MetaDataTypes::InstrumentName:
                    {
                        ::sprintf_s(Name, _countof(Name), "track_name_%02u", (unsigned int)i);
                        AssignString((const char *) &Data[0], DataSize, Text);

                        metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), Name, Text.c_str()));
                        break;
                    }

                    // Tune 1000 Karaoke format (https://www.mixagesoftware.com/en/midikit/help/HTML/karaoke_formats.html)
                    case MetaDataTypes::Lyrics:
                    {
                        AssignString((const char *) &Data[0], DataSize, Text);

                        metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "lyrics", Text.c_str()));
                        break;
                    }

                    case MetaDataTypes::Marker:
                    {
                        AssignString((const char *) &Data[0], DataSize, Text);

                        metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "track_marker", Text.c_str()));
                        break;
                    }

                    case MetaDataTypes::CueMarker:
                    {
                        AssignString((const char *) &Data[0], DataSize, Text);

                        metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "cue_marker", Text.c_str()));
                        break;
                    }

                    case MetaDataTypes::SetTempo:
                    {
                        break;
                    }

                    case MetaDataTypes::TimeSignature:
                    {
                        if (DataSize == 4)
                        {
                            ::sprintf_s(Name, _countof(Name), "%d/%d", Data[0], (1 << Data[1]));
                            metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "time_signature", Name));
                        }
                        break;
                    }

                    case MetaDataTypes::KeySignature:
                    {
                        if (DataSize == 2)
                        {
                            if (-7 <= (int8_t)Data[0] && (int8_t)Data[0] <= 7)
                            {
                                size_t Index = (size_t)((int8_t)Data[0] + 7);

                                if (Data[1] == 0)
                                {
                                    const char * MajorScales[] = { "Cb", "Gb", "Db", "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#", "C#" };

                                    metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "key_signature", MajorScales[Index]));
                                }
                                else
                                if (Data[1] == 1)
                                {
                                    const char * MinorScales[] = { "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#", "C#", "G#", "D#", "A#" };

                                    metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "key_signature", MinorScales[Index]));
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }
    }

    if (!TypeFound)
        metaData.AddItem(MIDIMetaDataItem(0, "type", "GM"));

    metaData.Append(_ExtraMetaData);
}

void MIDIContainer::TrimStart()
{
    if (_Format == 2)
    {
        for (size_t i = 0, j = _Tracks.size(); i < j; ++i)
            TrimRange(i, i);
    }
    else
        TrimRange(0, _Tracks.size() - 1);
}

void MIDIContainer::TrimRange(size_t start, size_t end)
{
    uint32_t timestamp_first_note = ~0UL;

    for (size_t i = start; i <= end; ++i)
    {
        size_t j, k;

        const MIDITrack & Track = _Tracks[i];

        for (j = 0, k = Track.GetLength(); j < k; ++j)
        {
            const MIDIEvent & Event = Track[j];

            if (Event.Type == MIDIEvent::NoteOn && Event.Data[0])
                break;
        }

        if (j < k)
        {
            if (Track[j].Timestamp < timestamp_first_note)
                timestamp_first_note = Track[j].Timestamp;
        }
    }

    if (timestamp_first_note < ~0UL && timestamp_first_note > 0)
    {
        for (size_t i = start; i <= end; ++i)
        {
            MIDITrack & Track = _Tracks[i];

            for (size_t j = 0, k = Track.GetLength(); j < k; ++j)
            {
                MIDIEvent & Event = Track[j];

                if (Event.Timestamp >= timestamp_first_note)
                    Event.Timestamp -= timestamp_first_note;
                else
                    Event.Timestamp = 0;
            }
        }

        if (start == end)
        {
            TrimTempoMap(start, timestamp_first_note);

            _EndTimestamps[start] -= timestamp_first_note;

            if (_LoopEndTimestamps[start] != ~0UL)
                _LoopEndTimestamps[start] -= timestamp_first_note;

            if (_LoopBeginTimestamps[start] != ~0UL)
            {
                if (_LoopBeginTimestamps[start] > timestamp_first_note)
                    _LoopBeginTimestamps[start] -= timestamp_first_note;
                else
                    _LoopBeginTimestamps[start] = 0;
            }
        }
        else
        {
            TrimTempoMap(0, timestamp_first_note);

            _EndTimestamps[0] -= timestamp_first_note;

            if (_LoopEndTimestamps[0] != ~0UL)
                _LoopEndTimestamps[0] -= timestamp_first_note;

            if (_LoopBeginTimestamps[0] != ~0UL)
            {
                if (_LoopBeginTimestamps[0] > timestamp_first_note)
                    _LoopBeginTimestamps[0] -= timestamp_first_note;
                else
                    _LoopBeginTimestamps[0] = 0;
            }
        }
    }
}

void MIDIContainer::TrimTempoMap(size_t index, uint32_t base_timestamp)
{
    if (index >= _TempoMaps.size())
        return;

    TempoMap & Map = _TempoMaps[index];

    for (size_t i = 0, j = Map.GetCount(); i < j; ++i)
    {
        TempoEntry & Entry = Map[i];

        if (Entry.Timestamp >= base_timestamp)
            Entry.Timestamp -= base_timestamp;
        else
            Entry.Timestamp = 0;
    }
}

void MIDIContainer::SplitByInstrumentChanges(SplitCallback callback)
{
    if (_Format != 1)
        return;

    for (size_t i = 0; i < _Tracks.size(); ++i)
    {
        MIDITrack SrcTrack = _Tracks[0];

        _Tracks.erase(_Tracks.begin());

        MIDITrack DstTrack;

        MIDITrack ProgramChangeTrack;

        for (size_t k = 0; k < SrcTrack.GetLength(); ++k)
        {
            const MIDIEvent & Event = SrcTrack[k];

            if (Event.Type == MIDIEvent::ProgramChange || (Event.Type == MIDIEvent::ControlChange && (Event.Data[0] == 0x00 || Event.Data[0] == 0x20)))
            {
                ProgramChangeTrack.AddEvent(Event);
            }
            else
            {
                if (ProgramChangeTrack.GetLength() > 0)
                {
                    if (DstTrack.GetLength())
                        _Tracks.push_back(DstTrack);

                    DstTrack = ProgramChangeTrack;

                    if (callback)
                    {
                        uint32_t timestamp = 0;
                        uint8_t bank_msb = 0, bank_lsb = 0, instrument = 0;

                        for (size_t m = 0; m < ProgramChangeTrack.GetLength(); ++m)
                        {
                            const MIDIEvent & ev = ProgramChangeTrack[m];

                            if (ev.Type == MIDIEvent::ProgramChange)
                                instrument = ev.Data[0];
                            else
                                if (ev.Data[0] == 0)
                                    bank_msb = ev.Data[1];
                                else
                                    bank_lsb = ev.Data[1];

                            if (ev.Timestamp > timestamp)
                                timestamp = ev.Timestamp;
                        }

                        {
                            std::string Name = callback(bank_msb, bank_lsb, instrument);

                            std::vector<uint8_t> Data;

                            Data.resize(Name.length() + 2);

                            Data[0] = 0xFF;
                            Data[1] = 0x03;

                            std::copy(Name.begin(), Name.end(), Data.begin() + 2);

                            DstTrack.AddEvent(MIDIEvent(timestamp, MIDIEvent::Extended, 0, &Data[0], Data.size()));
                        }
                    }

                    ProgramChangeTrack = MIDITrack();
                }

                DstTrack.AddEvent(Event);
            }
        }

        if (DstTrack.GetLength())
            _Tracks.push_back(DstTrack);
    }
}

void MIDIContainer::DetectLoops(bool detectXMILoops, bool detectMarkerLoops, bool detectRPGMakerLoops, bool detectTouhouLoops)
{
    std::vector<uint8_t> data;

    size_t SubSongCount = (_Format == 2) ? _Tracks.size() : 1;

    _LoopBeginTimestamps.resize(SubSongCount);
    _LoopEndTimestamps.resize(SubSongCount);

    for (size_t i = 0; i < SubSongCount; ++i)
    {
        _LoopBeginTimestamps[i] = ~0UL;
        _LoopEndTimestamps[i] = ~0UL;
    }

    if (detectXMILoops)
    {
        for (size_t i = 0; i < _Tracks.size(); ++i)
        {
            size_t SubSongIndex = 0;

            if (_Format == 2)
                SubSongIndex = i;

            const MIDITrack & Track = _Tracks[i];

            for (size_t j = 0; j < Track.GetLength(); ++j)
            {
                const MIDIEvent & event = Track[j];

                if (event.Type == MIDIEvent::ControlChange && (event.Data[0] >= 0x74 && event.Data[0] <= 0x77))
                {
                    if (event.Data[0] == 0x74 || event.Data[0] == 0x76)
                    {
                        if (_LoopBeginTimestamps[SubSongIndex] == ~0UL || _LoopBeginTimestamps[SubSongIndex] > event.Timestamp)
                        {
                            _LoopBeginTimestamps[SubSongIndex] = event.Timestamp;
                        }
                    }
                    else
                    {
                        if (_LoopEndTimestamps[SubSongIndex] == ~0UL || _LoopEndTimestamps[SubSongIndex] < event.Timestamp)
                        {
                            _LoopEndTimestamps[SubSongIndex] = event.Timestamp;
                        }
                    }
                }
            }
        }
    }

    if (detectMarkerLoops)
    {
        for (size_t i = 0; i < _Tracks.size(); ++i)
        {
            size_t SubsongIndex = 0;

            if (_Format == 2)
                SubsongIndex = i;

            const MIDITrack & Track = _Tracks[i];

            for (size_t j = 0; j < Track.GetLength(); ++j)
            {
                const MIDIEvent & Event = Track[j];

                if ((Event.Type == MIDIEvent::Extended) && (Event.GetDataSize() >= 9) && Event.Data[0] == 0xFF && Event.Data[1] == 0x06)
                {
                    size_t Size = Event.GetDataSize() - 2;

                    data.resize(Size);
                    Event.GetData(&data[0], 2, Size);

                    if ((Size == 9) && (::_strnicmp((const char *) &data[0], "loopStart", 9) == 0))
                    {
                        if (_LoopBeginTimestamps[SubsongIndex] == ~0UL || _LoopBeginTimestamps[SubsongIndex] > Event.Timestamp)
                        {
                            _LoopBeginTimestamps[SubsongIndex] = Event.Timestamp;
                        }
                    }
                    else
                    if ((Size == 7) && (::_strnicmp((const char *) &data[0], "loopEnd", 7) == 0))
                    {
                        if (_LoopEndTimestamps[SubsongIndex] == ~0UL || _LoopEndTimestamps[SubsongIndex] < Event.Timestamp)
                        {
                            _LoopEndTimestamps[SubsongIndex] = Event.Timestamp;
                        }
                    }
                }
            }
        }
    }

    if (detectRPGMakerLoops)
    {
        bool HasEMIDIControlChanges = false;

        for (size_t i = 0; i < _Tracks.size(); ++i)
        {
            size_t SubSongIndex = 0;

            if (_Format == 2)
                SubSongIndex = i;

            const MIDITrack & Track = _Tracks[i];

            for (size_t j = 0; j < Track.GetLength(); ++j)
            {
                const MIDIEvent & Event = Track[j];

                if ((Event.Type == MIDIEvent::ControlChange) && (Event.Data[0] == 110 || Event.Data[0] == 111))
                {
                    if (Event.Data[0] == 110)
                    {
                        HasEMIDIControlChanges = true;
                        break;
                    }

                    {
                        if (_LoopBeginTimestamps[SubSongIndex] == ~0UL || _LoopBeginTimestamps[SubSongIndex] > Event.Timestamp)
                        {
                            _LoopBeginTimestamps[SubSongIndex] = Event.Timestamp;
                        }
                    }
                }
            }

            if (HasEMIDIControlChanges)
            {
                _LoopBeginTimestamps[SubSongIndex] = ~0UL;
                _LoopEndTimestamps[SubSongIndex] = ~0UL;
                break;
            }
        }
    }

    if (detectTouhouLoops && (_Format == 0))
    {
        bool HasLoopError = false;

        for (size_t i = 0; !HasLoopError && (i < _Tracks.size()); ++i)
        {
            const MIDITrack & Track = _Tracks[i];

            for (size_t j = 0; !HasLoopError && (j < Track.GetLength()); ++j)
            {
                const MIDIEvent & Event = Track[j];

                if (Event.Type == MIDIEvent::ControlChange)
                {
                    if (Event.Data[0] == 2)
                    {
                        if (Event.Data[1] != 0)
                        {
                            HasLoopError = true;
                            break;
                        }

                        _LoopBeginTimestamps[0] = Event.Timestamp;
                    }

                    if (Event.Data[0] == 4)
                    {
                        if (Event.Data[1] != 0)
                        {
                            HasLoopError = true;
                            break;
                        }

                        _LoopEndTimestamps[0] = Event.Timestamp;
                    }
                }
            }
        }

        if (HasLoopError)
        {
            _LoopBeginTimestamps[0] = ~0UL;
            _LoopEndTimestamps[0] = ~0UL;
        }
    }

    // Sanity
    for (size_t i = 0; i < SubSongCount; ++i)
    {
        uint32_t EndOfSongTimestamp;

        if (_Format == 2)
            EndOfSongTimestamp = _Tracks[i][_Tracks[i].GetLength() - 1].Timestamp;
        else
        {
            EndOfSongTimestamp = 0;

            for (size_t j = 0; j < _Tracks.size(); ++j)
            {
                const MIDITrack & Track = _Tracks[j];

                uint32_t Timestamp = Track[Track.GetLength() - 1].Timestamp;

                if (Timestamp > EndOfSongTimestamp)
                    EndOfSongTimestamp = Timestamp;
            }
        }

        if ((_LoopBeginTimestamps[i] != ~0UL) && ((_LoopBeginTimestamps[i] == _LoopEndTimestamps[i]) || (_LoopBeginTimestamps[i] == EndOfSongTimestamp)))
        {
            _LoopBeginTimestamps[i] = ~0UL;
            _LoopEndTimestamps[i] = ~0UL;
        }
    }
}

void MIDIContainer::EncodeVariableLengthQuantity(std::vector<uint8_t> & data, uint32_t quantity)
{
    uint32_t Shift = 7 * 4;

    while (Shift && !(quantity >> Shift))
    {
        Shift -= 7;
    }

    while (Shift > 0)
    {
        data.push_back((uint8_t) (((quantity >> Shift) & 0x7F) | 0x80));
        Shift -= 7;
    }

    data.push_back((uint8_t) (quantity & 0x7F));
}

uint32_t MIDIContainer::TimestampToMS(uint32_t p_timestamp, size_t subSongIndex) const
{
    uint32_t TimestampInMS = 0;

    uint32_t Tempo = 500000;

    size_t TempoMapCount = _TempoMaps.size();

    if ((subSongIndex > 0) && (TempoMapCount > 0))
    {
        for (size_t i = std::min(subSongIndex, TempoMapCount); --i; )
        {
            size_t Count = _TempoMaps[i].GetCount();

            if (Count > 0)
            {
                Tempo = _TempoMaps[i][Count - 1].Tempo;
                break;
            }
        }
    }

    uint32_t Timestamp = 0;
    uint32_t HalfDivision = _Division * 500;
    uint32_t Division = HalfDivision * 2;

    if (subSongIndex < TempoMapCount)
    {
        const TempoMap & TempoEntries = _TempoMaps[subSongIndex];

        size_t Index = 0;
        size_t Count = TempoEntries.GetCount();

        while ((Index < Count) && (Timestamp + p_timestamp >= TempoEntries[Index].Timestamp))
        {
            uint32_t Delta = TempoEntries[Index].Timestamp - Timestamp;

            TimestampInMS += ((uint64_t) Tempo * (uint64_t) Delta + HalfDivision) / Division;

            Tempo = TempoEntries[Index].Tempo;
            ++Index;

            Timestamp += Delta;
            p_timestamp -= Delta;
        }
    }

    TimestampInMS += ((uint64_t) Tempo * (uint64_t) p_timestamp + HalfDivision) / Division;

    return TimestampInMS;
}
#pragma endregion
