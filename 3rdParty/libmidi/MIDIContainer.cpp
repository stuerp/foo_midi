
/** $VER: MIDIContainer.cpp (2023.12.24) **/

#include "MIDIContainer.h"

#include <string.h>

#pragma warning(disable: 4242)
#include <algorithm>
#pragma warning(default: 4242)

#pragma region MIDI Event

MIDIEvent::MIDIEvent(const MIDIEvent & other)
{
    Timestamp = other.Timestamp;
    Type = other.Type;
    ChannelNumber = other.ChannelNumber;
    Data = other.Data;
}

MIDIEvent::MIDIEvent(uint32_t timestamp, EventType eventType, uint32_t channelNumber, const uint8_t * data, size_t size)
{
    Timestamp = timestamp;
    Type = eventType;
    ChannelNumber = channelNumber;
    Data.assign(data, data + size);
}

#pragma endregion

#pragma region MIDI Track

void MIDITrack::AddEvent(const MIDIEvent & newEvent)
{
    auto it = _Events.end();

    if (_Events.size() > 0)
    {
        MIDIEvent & Event = *(it - 1);

        if ((Event.Type == MIDIEvent::Extended) && (Event.Data.size() >= 2) && (Event.Data[0] == StatusCodes::MetaData) && (Event.Data[1] == MetaDataTypes::EndOfTrack))
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
TempoItem::TempoItem(uint32_t timestamp, uint32_t tempo)
{
    Timestamp = timestamp;
    Tempo = tempo;
}

void TempoMap::Add(uint32_t tempo, uint32_t timestamp)
{
    auto it = _Items.end();

    while (it > _Items.begin())
    {
        if ((*(it - 1)).Timestamp <= timestamp)
            break;
        --it;
    }

    if (it > _Items.begin() && (*(it - 1)).Timestamp == timestamp)
    {
        (*(it - 1)).Tempo = tempo;
    }
    else
    {
        _Items.insert(it, TempoItem(timestamp, tempo));
    }
}

uint32_t TempoMap::TimestampToMS(uint32_t p_timestamp, uint32_t division) const
{
    uint32_t TimestampInMS = 0;

    auto Iterator = _Items.begin();

    uint32_t Tempo = 500000;

    uint32_t Timestamp = 0;
    uint32_t HalfDivision = division * 500;
    division = HalfDivision * 2;

    while ((Iterator < _Items.end()) && (Timestamp + p_timestamp >= (*Iterator).Timestamp))
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
SysExItem::SysExItem(const SysExItem & src)
{
    Offset = src.Offset;
    Size = src.Size;
    PortNumber = src.PortNumber;
}

SysExItem::SysExItem(uint8_t portNumber, std::size_t offset, std::size_t size)
{
    Offset = offset;
    Size = size;
    PortNumber = portNumber;
}

size_t SysExTable::AddItem(const uint8_t * data, std::size_t size, uint8_t portNumber)
{
    for (auto it = _Items.begin(); it < _Items.end(); ++it)
    {
        const SysExItem & Item = *it;

        if ((portNumber == Item.PortNumber) && (size == Item.Size) && (::memcmp(data, &_Data[Item.Offset], size) == 0))
            return ((uint32_t) (it - _Items.begin()));
    }

    SysExItem Item(portNumber, _Data.size(), size);

    _Data.insert(_Data.end(), data, data + size);
    _Items.push_back(Item);

    return (_Items.size() - 1);
}

bool SysExTable::GetItem(size_t index, const uint8_t * & data, std::size_t & size, uint8_t & portNumber) const
{
    if (index >= _Items.size())
        return false;

    const SysExItem & Item = _Items[index];

    data = &_Data[Item.Offset];
    size = Item.Size;
    portNumber = Item.PortNumber;

    return true;
}
#pragma endregion

#pragma region("MIDI Meta Data")
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

        if (::_stricmp(name, Item.Name.c_str()) == 0)
        {
            item = Item;
            return true;
        }
    }

    return false;
}

bool MIDIMetaData::GetBitmap(std::vector<uint8_t> & bitmap) const
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
void MIDIContainer::Initialize(uint32_t format, uint32_t timeDivision)
{
    _Format = format;
    _TimeDivision = timeDivision;

    if (format != 2)
    {
        _ChannelMask.resize(1);
        _ChannelMask[0] = 0;
        _TempoMaps.resize(1);

        _EndTimestamps.resize(1);
        _EndTimestamps[0] = 0;

        _Loop.resize(1);
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
            if ((Event.Data.size() >= 5) && (Event.Data[0] == StatusCodes::MetaData) && (Event.Data[1] == MetaDataTypes::SetTempo))
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
            if ((Event.Data.size() >= 3) && (Event.Data[0] == StatusCodes::MetaData))
            {
                if (Event.Data[1] == MetaDataTypes::InstrumentName || Event.Data[1] == MetaDataTypes::DeviceName)
                {
                    DeviceName.assign(Event.Data.begin() + 2, Event.Data.end());
                    std::transform(DeviceName.begin(), DeviceName.end(), DeviceName.begin(), ::tolower);
                }
                else
                if (Event.Data[1] == MetaDataTypes::MIDIPort)
                {
                    PortNumber = Event.Data[2];

                    LimitPortNumber(PortNumber);
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
                    if (_DeviceNames[ChannelNumber][j] == DeviceName)
                        break;
                }

                if (j < k)
                    PortNumber = (uint8_t)j;
                else
                {
                    _DeviceNames[ChannelNumber].push_back(DeviceName);
                    PortNumber = (uint8_t)k;
                }

                LimitPortNumber(PortNumber);
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

    if ((event.Type == MIDIEvent::Extended) && (event.Data.size() >= 5) && (event.Data[0] == StatusCodes::MetaData) && (event.Data[1] == MetaDataTypes::SetTempo))
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
    uint32_t LoopBeginTimestamp = GetLoopBeginTimestamp(subSongIndex);
    uint32_t LoopEndTimestamp = GetLoopEndTimestamp(subSongIndex);

    size_t LoopBegin = ~0UL;
    size_t LoopEnd = ~0UL;

    size_t TrackCount = _Tracks.size();

    std::vector<std::size_t> TrackPositions(TrackCount, 0);
    std::vector<uint8_t> PortNumbers(TrackCount, 0);
    std::vector<std::string> DeviceNames(TrackCount);

    bool CleanEMIDI = (cleanFlags & CleanFlagEMIDI) == CleanFlagEMIDI;
    bool CleanInstruments = (cleanFlags & CleanFlagInstruments) == CleanFlagInstruments;
    bool CleanBanks = (cleanFlags & CleanFlagBanks) == CleanFlagBanks;

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
                    if ((Event.Data[1] != 0) && (Event.Data[1] != 1) && (Event.Data[1] != 0x7F))
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

    std::vector<uint8_t> Data;

    for (;;)
    {
        size_t SelectedTrack = 0;

        // Select which track can provide the next event.
        {
            uint32_t NextTimestamp = ~0UL;

            for (size_t i = 0; i < TrackCount; ++i)
            {
                if (TrackPositions[i] >= _Tracks[i].GetLength())
                    continue;

                if (_Tracks[i][TrackPositions[i]].Timestamp < NextTimestamp)
                {
                    NextTimestamp = _Tracks[i][TrackPositions[i]].Timestamp;
                    SelectedTrack = i;
                }
            }

            if (NextTimestamp == ~0UL)
                break;
        }

        const MIDIEvent & Event = _Tracks[SelectedTrack][TrackPositions[SelectedTrack]];

        bool IsEventFiltered = false;

        if (CleanInstruments || CleanBanks)
        {
            if (CleanInstruments && (Event.Type == MIDIEvent::ProgramChange))
                IsEventFiltered = true;
            else
            if (CleanBanks && (Event.Type == MIDIEvent::ControlChange) && (Event.Data[0] == 0x00u || Event.Data[0] == 0x20u))
                IsEventFiltered = true;
        }

        if (!IsEventFiltered)
        {
            if ((LoopBegin == ~0UL) && (Event.Timestamp >= LoopBeginTimestamp))
                LoopBegin = midiStream.size();

            if ((LoopEnd == ~0UL) && (Event.Timestamp > LoopEndTimestamp))
                LoopEnd = midiStream.size();

            uint32_t TempoTrackIndex = ((_Format == 2) && (subSongIndex > 0)) ? (uint32_t) subSongIndex : 0;

            uint32_t TimestampInMS = TimestampToMS(Event.Timestamp, TempoTrackIndex);

            if (Event.Type != MIDIEvent::Extended)
            {
                if (DeviceNames[SelectedTrack].length() != 0)
                {
                    size_t i, j;

                    for (i = 0, j = _DeviceNames[Event.ChannelNumber].size(); i < j; ++i)
                    {
                        if (_DeviceNames[Event.ChannelNumber][i] == DeviceNames[SelectedTrack])
                            break;
                    }

                    PortNumbers[SelectedTrack] = (uint8_t) i;
                    DeviceNames[SelectedTrack].clear();

                    LimitPortNumber(PortNumbers[SelectedTrack]);
                }

                uint32_t Message = ((Event.Type + 8) << 4) + Event.ChannelNumber;

                if (Event.Data.size() >= 1)
                    Message += Event.Data[0] << 8;

                if (Event.Data.size() >= 2)
                    Message += Event.Data[1] << 16;

                Message += PortNumbers[SelectedTrack] << 24;

                midiStream.push_back(MIDIStreamEvent(TimestampInMS, Message));
            }
            else
            {
                size_t DataSize = Event.Data.size();

                if ((DataSize >= 3) && (Event.Data[0] == StatusCodes::SysEx))
                {
                    if (DeviceNames[SelectedTrack].length())
                    {
                        size_t i, j;

                        for (i = 0, j = _DeviceNames[Event.ChannelNumber].size(); i < j; ++i)
                        {
                            if (_DeviceNames[Event.ChannelNumber][i] == DeviceNames[SelectedTrack])
                                break;
                        }

                        PortNumbers[SelectedTrack] = (uint8_t) i;
                        DeviceNames[SelectedTrack].clear();

                        LimitPortNumber(PortNumbers[SelectedTrack]);
                    }

                    Data = Event.Data;

                    if (Data[DataSize - 1] == StatusCodes::SysExEnd)
                    {
                        uint32_t Index = (uint32_t) sysExTable.AddItem(&Data[0], DataSize, PortNumbers[SelectedTrack]) | 0x80000000u;

                        midiStream.push_back(MIDIStreamEvent(TimestampInMS, Index));
                    }
                }
                else
                if ((DataSize >= 3) && (Event.Data[0] == StatusCodes::MetaData))
                {
                    if (Event.Data[1] == MetaDataTypes::InstrumentName || Event.Data[1] == MetaDataTypes::DeviceName)
                    {
                        DeviceNames[SelectedTrack].assign(Event.Data.begin() + 2, Event.Data.end());

                        std::transform(DeviceNames[SelectedTrack].begin(), DeviceNames[SelectedTrack].end(), DeviceNames[SelectedTrack].begin(), ::tolower);
                    }
                    else
                    if (Event.Data[1] == MetaDataTypes::MIDIPort)
                    {
                        PortNumbers[SelectedTrack] = Event.Data[2];
                        DeviceNames[SelectedTrack].clear();

                        LimitPortNumber(PortNumbers[SelectedTrack]);
                    }
                }
                else
                if ((DataSize == 1) && (Event.Data[0] > StatusCodes::SysExEnd))
                {
                    if (DeviceNames[SelectedTrack].length())
                    {
                        size_t i, j;

                        for (i = 0, j = _DeviceNames[Event.ChannelNumber].size(); i < j; ++i)
                        {
                            if (_DeviceNames[Event.ChannelNumber][i] == DeviceNames[SelectedTrack])
                                break;
                        }

                        PortNumbers[SelectedTrack] = (uint8_t) i;
                        DeviceNames[SelectedTrack].clear();
                        LimitPortNumber(PortNumbers[SelectedTrack]);
                    }

                    uint32_t Message = (uint32_t)(PortNumbers[SelectedTrack] << 24);

                    Message += Event.Data[0];
                    midiStream.push_back(MIDIStreamEvent(TimestampInMS, Message));
                }
            }
        }

        TrackPositions[SelectedTrack]++;
    }

    loopBegin = (uint32_t) LoopBegin;
    loopEnd   = (uint32_t) LoopEnd;
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
    midiStream.push_back((uint8_t)(_TimeDivision >> 8));
    midiStream.push_back((uint8_t) _TimeDivision);

    std::vector<uint8_t> Data;

    for (const MIDITrack & Track : _Tracks)
    {
        const char ChunkType[] = "MTrk";

        midiStream.insert(midiStream.end(), ChunkType, ChunkType + 4);

        size_t ChunkSizeOffset = midiStream.size();

        midiStream.push_back(0);
        midiStream.push_back(0);
        midiStream.push_back(0);
        midiStream.push_back(0);

        uint32_t LastTimestamp = 0;
        uint8_t LastStatus = StatusCodes::MetaData;

        for (const MIDIEvent & Event : Track)
        {
            EncodeVariableLengthQuantity(midiStream, Event.Timestamp - LastTimestamp);

            LastTimestamp = Event.Timestamp;

            if (Event.Type != MIDIEvent::Extended)
            {
                const uint8_t Status = (uint8_t) (((Event.Type + 8) << 4) + Event.ChannelNumber);

                if (Status != LastStatus)
                {
                    midiStream.push_back(Status);
                    LastStatus = Status;
                }

                midiStream.insert(midiStream.end(), Event.Data.begin(), Event.Data.end());
            }
            else
            {
                uint32_t DataSize = (uint32_t) Event.Data.size();

                if (DataSize >= 1)
                {
                    if (Event.Data[0] == StatusCodes::SysEx)
                    {
                        --DataSize;

                        midiStream.push_back(StatusCodes::SysEx);
                        EncodeVariableLengthQuantity(midiStream, DataSize);

                        if (DataSize != 0)
                            midiStream.insert(midiStream.end(), Event.Data.begin() + 1, Event.Data.end());
                    }
                    else
                    if (Event.Data[0] == StatusCodes::MetaData && (DataSize >= 2))
                    {
                        DataSize -= 2;

                        midiStream.push_back(0xFFu);
                        midiStream.push_back(Event.Data[1]);

                        EncodeVariableLengthQuantity(midiStream, DataSize);

                        if (DataSize != 0)
                            midiStream.insert(midiStream.end(), Event.Data.begin() + 2, Event.Data.end());
                    }
                    else
                        midiStream.insert(midiStream.end(), Event.Data.begin() + 1, Event.Data.end());
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
        if ((event.Data[0] != StatusCodes::MetaData) || (event.Data.size() < 2) || (event.Data[1] != MetaDataTypes::EndOfTrack))
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

    uint32_t Timestamp = _Loop[0].Begin();

    if ((_Format == 2) && (subSongIndex > 0))
    {
        TrackIndex = subSongIndex;
        Timestamp = _Loop[subSongIndex].Begin();
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

    uint32_t Timestamp = _Loop[0].End();

    if ((_Format == 2) && (subSongIndex > 0))
    {
        TrackIndex = subSongIndex;
        Timestamp = _Loop[subSongIndex].End();
    }

    if (!ms)
        return Timestamp;

    if (Timestamp != ~0UL)
        return TimestampToMS(Timestamp, TrackIndex);

    return ~0UL;
}

void MIDIContainer::GetMetaData(size_t subSongIndex, MIDIMetaData & metaData)
{
    const char * TypeName = nullptr;
    uint32_t TypeTimestamp = 0;

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

            size_t DataSize = Event.Data.size();

            const char * TempTypeName = nullptr;

            if ((DataSize >= 1) && (Event.Data[0] == StatusCodes::SysEx))
            {
                switch (Event.Data[1])
                {
                    case 0x7Eu:
                    {
                        TempTypeName = "GM"; // 1991

                        if ((DataSize >= 5) && (((Event.Data[3] == 0x04) && (Event.Data[4] >= 0x05)) || (Event.Data[3] > 0x04)))
                            TempTypeName = "GM2"; // 1999, 2003 v1.1, 2007 v1.2
                        break;
                    }

                    case 0x43u:
                        TempTypeName = "XG"; // 1994 Level 1, 1997 Level 2, 1998, Level 3
                        break;

                    case 0x42u:
                        TempTypeName = "X5"; // 1994 Korg X5
                        break;

                    case 0x41u:
                    {
                        if (DataSize > 3)
                        {
                            switch (Event.Data[3])
                            {
                                case 0x42u:
                                    TempTypeName = "GS"; // 1991
                                    break;

                                case 0x16u:
                                    TempTypeName = "MT-32"; // 1987 Roland MT-32
                                    break;

                                case 0x14u:
                                    TempTypeName = "D-50"; // 1987 Roland D-50
                                    break;
                            }
                        }
                        break;
                    }
                }
            }
            else
            if ((DataSize > 2) && (Event.Data[0] == StatusCodes::MetaData))
            {
                char Name[32];

                std::string Text;

                DataSize -= 2;

//                std::vector<uint8_t> Data(Event.Data.begin() + 2, Event.Data.end());

                switch (Event.Data[1])
                {
                    case MetaDataTypes::Text:
                    {
                        if (!IsSoftKaraoke)
                        {
                            IsSoftKaraoke = (DataSize >= 19) && (::_strnicmp((const char *) Event.Data.data() + 2, "@KMIDI KARAOKE FILE", 19) == 0);

                            if (IsSoftKaraoke)
                            {
                                metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "lyrics_type", "Soft Karaoke"));
                            }
                            else
                            {
                                ::sprintf_s(Name, _countof(Name), "track_text_%02zd", i);
                                AssignString((const char *) Event.Data.data() + 2, DataSize, Text);

                                metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), Name, Text.c_str()));
                            }
                        }
                        else
                        {
                            if ((DataSize > 2) && (::_strnicmp((const char *) Event.Data.data() + 2, "@K", 2) == 0))
                            {
                                AssignString((const char *) Event.Data.data() + 4, DataSize - 2, Text);

                                metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "soft_karaoke_version", Text.c_str()));
                            }
                            else
                            if ((DataSize > 2) && (::_strnicmp((const char *) Event.Data.data() + 2, "@L", 2) == 0))
                            {
                                AssignString((const char *) Event.Data.data() + 4, DataSize - 2, Text);

                                metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "soft_karaoke_language", Text.c_str()));
                            }
                            else
                            if ((DataSize > 2) && (::_strnicmp((const char *) Event.Data.data() + 2, "@T", 2) == 0))
                            {
                                AssignString((const char *) Event.Data.data() + 4, DataSize - 2, Text);

                                metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "soft_karaoke_text", Text.c_str()));
                            }
                            else
                            if ((DataSize > 2) && (::_strnicmp((const char *) Event.Data.data() + 2, "@I", 2) == 0))
                            {
                                AssignString((const char *) Event.Data.data() + 4, DataSize - 2, Text);

                                metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "soft_karaoke_info", Text.c_str()));
                            }
                            else
                            if ((DataSize > 2) && (::_strnicmp((const char *) Event.Data.data() + 2, "@W", 2) == 0))
                            {
                                AssignString((const char *) Event.Data.data() + 4, DataSize - 2, Text);

                                metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "soft_karaoke_words", Text.c_str()));
                            }
                            else
                            if ((DataSize > 2) && (Event.Data[2] == '@'))
                            {
                                // Unknown Soft Karaoke tag
                                ::sprintf_s(Name, _countof(Name), "track_text_%02zd", i);
                                AssignString((const char *) Event.Data.data() + 2, DataSize, Text);

                                metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), Name, Text.c_str()));
                            }
                            else
                            {
                                AssignString((const char *) Event.Data.data() + 2, DataSize, Text);

                                metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "soft_karaoke_lyrics", Text.c_str()));
                            }
                        }
                        break;
                    }

                    case MetaDataTypes::Copyright:
                    {
                        AssignString((const char *) Event.Data.data() + 2, DataSize, Text);

                        metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "copyright", Text.c_str()));
                        break;
                    }

                    case MetaDataTypes::TrackName:
                    case MetaDataTypes::InstrumentName:
                    {
                        ::sprintf_s(Name, _countof(Name), "track_name_%02u", (unsigned int)i);
                        AssignString((const char *) Event.Data.data() + 2, DataSize, Text);

                        metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), Name, Text.c_str()));
                        break;
                    }

                    // Tune 1000 Karaoke format (https://www.mixagesoftware.com/en/midikit/help/HTML/karaoke_formats.html)
                    case MetaDataTypes::Lyrics:
                    {
                        AssignString((const char *) Event.Data.data() + 2, DataSize, Text);

                        metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "lyrics", Text.c_str()));
                        break;
                    }

                    case MetaDataTypes::Marker:
                    {
                        AssignString((const char *) Event.Data.data() + 2, DataSize, Text);

                        metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "track_marker", Text.c_str()));
                        break;
                    }

                    case MetaDataTypes::CueMarker:
                    {
                        AssignString((const char *) Event.Data.data() + 2, DataSize, Text);

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
                            ::sprintf_s(Name, _countof(Name), "%d/%d", Event.Data[2], (1 << Event.Data[3]));
                            metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "time_signature", Name));
                        }
                        break;
                    }

                    case MetaDataTypes::KeySignature:
                    {
                        if (DataSize == 2)
                        {
                            if (-7 <= (int8_t) Event.Data[2] && (int8_t) Event.Data[2] <= 7)
                            {
                                size_t Index = (size_t)((int8_t) Event.Data[2] + 7);

                                if (Event.Data[3] == 0)
                                {
                                    const char * MajorScales[] = { "Cb", "Gb", "Db", "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#", "C#" };

                                    metaData.AddItem(MIDIMetaDataItem(TimestampToMS(Event.Timestamp, TempoTrackIndex), "key_signature", MajorScales[Index]));
                                }
                                else
                                if (Event.Data[3] == 1)
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

            // Remember the container type name: MT-32 or GM < GM2 < GS < XG
            if (TempTypeName != nullptr)
            {
                if ((TypeName != nullptr) && (::_stricmp(TypeName, "MT-32") != 0))
                {
                    if ((::_stricmp(TypeName, "GM") == 0) && (::_stricmp(TempTypeName, "GM2") == 0))
                        TypeName = TempTypeName;
                    else
                    if (((::_stricmp(TypeName, "GM") == 0) || (::_stricmp(TypeName, "GM2") == 0)) && (::_stricmp(TempTypeName, "GS") == 0))
                        TypeName = TempTypeName;
                    else
                    if (::_stricmp(TempTypeName, "XG") == 0)
                        TypeName = TempTypeName;
                }
                else
                    TypeName = TempTypeName;
            }
        }
    }

    if (TypeName && (::_stricmp(TypeName, "GM") != 0))
        metaData.AddItem(MIDIMetaDataItem(TypeTimestamp, "type", TypeName));
    else
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

            if (_Loop[start].HasEnd())
                _Loop[start].SetEnd(_Loop[start].End() - timestamp_first_note);

            if (_Loop[start].HasBegin())
            {
                if (_Loop[start].Begin() > timestamp_first_note)
                    _Loop[start].SetBegin(_Loop[start].Begin() - timestamp_first_note);
                else
                    _Loop[start].SetBegin(0);
            }
        }
        else
        {
            TrimTempoMap(0, timestamp_first_note);

            _EndTimestamps[0] -= timestamp_first_note;

            if (_Loop[0].HasEnd())
                _Loop[0].SetEnd(_Loop[0].End() - timestamp_first_note);

            if (_Loop[0].HasBegin())
            {
                if (_Loop[0].Begin() > timestamp_first_note)
                    _Loop[0].SetBegin(_Loop[0].Begin() - timestamp_first_note);
                else
                    _Loop[0].SetBegin(0);
            }
        }
    }
}

void MIDIContainer::TrimTempoMap(size_t index, uint32_t base_timestamp)
{
    if (index >= _TempoMaps.size())
        return;

    TempoMap & Map = _TempoMaps[index];

    for (size_t i = 0, j = Map.Size(); i < j; ++i)
    {
        TempoItem & Entry = Map[i];

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

            if ((Event.Type == MIDIEvent::ProgramChange) || ((Event.Type == MIDIEvent::ControlChange) && (Event.Data[0] == ControlChangeNumbers::BankSelect || Event.Data[0] == ControlChangeNumbers::LSB)))
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

                            std::vector<uint8_t> Data(Name.length() + 2);

                            Data[0] = 0xFF;
                            Data[1] = 0x03;

                            std::copy(Name.begin(), Name.end(), Data.begin() + 2);

                            DstTrack.AddEvent(MIDIEvent(timestamp, MIDIEvent::Extended, 0, Data.data(), Data.size()));
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

#include <windows.h>

void MIDIContainer::DetectLoops(bool detectXMILoops, bool detectMarkerLoops, bool detectRPGMakerLoops, bool detectTouhouLoops)
{
    size_t SubSongCount = (_Format == 2) ? _Tracks.size() : 1;

    {
        _Loop.resize(SubSongCount);

        for (size_t i = 0; i < SubSongCount; ++i)
            _Loop[i].Clear();
    }

    // Project Touhou
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

                        _Loop[0].SetBegin(Event.Timestamp);
                    }

                    if (Event.Data[0] == 4)
                    {
                        if (Event.Data[1] != 0)
                        {
                            HasLoopError = true;
                            break;
                        }

                        _Loop[0].SetEnd(Event.Timestamp);
                    }
                }
            }
        }

        if (HasLoopError)
            _Loop[0].Clear();
    }

    // RPG Maker
    if (detectRPGMakerLoops)
    {
        bool HasEMIDIControlChanges = false;

        for (size_t i = 0; i < _Tracks.size(); ++i)
        {
            size_t SubSongIndex = (_Format != 2) ? 0 : i;

            const MIDITrack & Track = _Tracks[i];

            for (size_t j = 0; j < Track.GetLength(); ++j)
            {
                const MIDIEvent & Event = Track[j];

                if ((Event.Type == MIDIEvent::ControlChange) && (Event.Data[0] == 110 /* 0x6E */ || Event.Data[0] == 111 /* 0x6F */))
                {
                    if (Event.Data[0] == 110 /* 0x6E */)
                    {
                        HasEMIDIControlChanges = true;
                        break;
                    }

                    // Control Change 111 (The end of the loop is always the end of the song.
                    if (!_Loop[SubSongIndex].HasBegin() || (Event.Timestamp < _Loop[SubSongIndex].Begin()))
                        _Loop[SubSongIndex].SetBegin(Event.Timestamp);
                }
            }

            if (HasEMIDIControlChanges)
            {
                _Loop[SubSongIndex].Clear();
                break;
            }
        }
    }

    // EMIDI/XMI
    if (detectXMILoops)
    {
        for (size_t i = 0; i < _Tracks.size(); ++i)
        {
            size_t SubSongIndex = (_Format != 2) ? 0 : i;

            const MIDITrack & Track = _Tracks[i];

            for (size_t j = 0; j < Track.GetLength(); ++j)
            {
                const MIDIEvent & Event = Track[j];

                if ((Event.Type == MIDIEvent::ControlChange) && (Event.Data[0] >= 116 /* 0x74 */ && Event.Data[0] <= 119 /* 0x77 */))
                {
                #ifdef _DEBUG
                    wchar_t Line[256]; ::swprintf_s(Line, _countof(Line), L"EMIDI: %08X %3d %3d\n", Event.Timestamp, Event.Data[0], Event.Data[1]); ::OutputDebugStringW(Line);
                #endif
                    // 116 / 0x74, AIL loop: FOR loop = 1 to n, 118 / 0x76, AIL clear beat / measure count (AIL = Audio Interface Library)
                    if (Event.Data[0] == 116 || Event.Data[0] == 118)
                    {
                        if (!_Loop[SubSongIndex].HasBegin() || (Event.Timestamp < _Loop[SubSongIndex].Begin()))
                            _Loop[SubSongIndex].SetBegin(Event.Timestamp); // LoopCount = Event.Data[1]; // 0 = Forever, 1 - 127 = Finite
                    }
                    // 117 / 0x75, AIL loop: NEXT/BREAK, 119 / 0x77, AIL callback trigger
                    else
                    {
                        if (!_Loop[SubSongIndex].HasEnd() || (Event.Timestamp < _Loop[SubSongIndex].End()))
                            _Loop[SubSongIndex].SetEnd(Event.Timestamp); // Event.Data[1] should be 127.
                    }
                }
            }
        }
    }

    // Introduced in MIDI files from Final Fantasy VII.
    if (detectMarkerLoops)
    {
        for (size_t i = 0; i < _Tracks.size(); ++i)
        {
            size_t SubSongIndex = (_Format != 2) ? 0 : i;

            const MIDITrack & Track = _Tracks[i];

            for (size_t j = 0; j < Track.GetLength(); ++j)
            {
                const MIDIEvent & Event = Track[j];

                if ((Event.Type == MIDIEvent::Extended) && (Event.Data.size() >= 9) && (Event.Data[0] == 0xFF) && (Event.Data[1] == 0x06))
                {
                    size_t Size = Event.Data.size() - 2;

                    std::vector<uint8_t> Name(Event.Data.begin() + 2, Event.Data.end());

//                  Event.GetData(Name.data(), 2, Size);

                    if ((Size == 9) && (::_strnicmp((const char *) Name.data(), "loopStart", 9) == 0))
                    {
                        if (!_Loop[SubSongIndex].HasBegin() || (Event.Timestamp < _Loop[SubSongIndex].Begin()))
                            _Loop[SubSongIndex].SetBegin(Event.Timestamp);
                    }
                    else
                    if ((Size == 7) && (::_strnicmp((const char *) Name.data(), "loopEnd", 7) == 0))
                    {
                        if (!_Loop[SubSongIndex].HasEnd() || (Event.Timestamp > _Loop[SubSongIndex].End()))
                            _Loop[SubSongIndex].SetEnd(Event.Timestamp);
                    }
                }
            }
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

        if (_Loop[i].HasBegin() && (_Loop[i].IsEmpty() || (_Loop[i].Begin() == EndOfSongTimestamp)))
            _Loop[i].Clear();
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

uint32_t MIDIContainer::TimestampToMS(uint32_t timestamp, size_t subSongIndex) const
{
    uint32_t TimestampInMS = 0;

    uint32_t Tempo = 500000;

    size_t TempoMapCount = _TempoMaps.size();

    if ((subSongIndex > 0) && (TempoMapCount > 0))
    {
        for (size_t i = (std::min)(subSongIndex, TempoMapCount); --i; )
        {
            size_t Count = _TempoMaps[i].Size();

            if (Count > 0)
            {
                Tempo = _TempoMaps[i][Count - 1].Tempo;
                break;
            }
        }
    }

    uint32_t Timestamp = 0;
    uint32_t HalfDivision = _TimeDivision * 500;
    uint32_t Division = HalfDivision * 2;

    if (subSongIndex < TempoMapCount)
    {
        const TempoMap & TempoEntries = _TempoMaps[subSongIndex];

        size_t Index = 0;
        size_t Count = TempoEntries.Size();

        while ((Index < Count) && (Timestamp + timestamp >= TempoEntries[Index].Timestamp))
        {
            uint32_t Delta = TempoEntries[Index].Timestamp - Timestamp;

            TimestampInMS += ((uint64_t) Tempo * (uint64_t) Delta + HalfDivision) / Division;

            Tempo = TempoEntries[Index].Tempo;
            ++Index;

            Timestamp += Delta;
            timestamp -= Delta;
        }
    }

    TimestampInMS += ((uint64_t) Tempo * (uint64_t) timestamp + HalfDivision) / Division;

    return TimestampInMS;
}
#pragma endregion
