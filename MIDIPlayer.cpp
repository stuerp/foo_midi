
/** $VER: MIDIPlayer.cpp (2023.01.09) **/

#include "MIDIPlayer.h"

MIDIPlayer::MIDIPlayer()
{
    foo_vis_midi::IAPI::ptr api;

    if (fb2k::std_api_try_get(api))
        _MusicKeyboard = api->GetMusicKeyboard();

    _SamplesRemaining = 0;
    _SampleRate = 1000;
    _TimeCurrent = 0;
    _TimeEnd = 0;
    _LoopStartTime = 0;
    _IsInitialized = false;
}

bool MIDIPlayer::Load(const midi_container & midiContainer, unsigned subsong, unsigned loopMode, unsigned cleanFlags)
{
    assert(_Stream.size() == 0);

    midiContainer.serialize_as_stream(subsong, _Stream, _SysExMap, _LoopStart, _LoopEnd, cleanFlags);

    if (_Stream.size() == 0)
        return false;

    _StreamCurrent = 0;
    _TimeCurrent = 0;
    _TimeEnd = (size_t)midiContainer.get_timestamp_end(subsong, true) + 1000;

    _LoopMode = (LoopMode)loopMode;

    if (_LoopMode & LoopModeEnabled)
    {
        _LoopStartTime = midiContainer.get_timestamp_loop_start(subsong, true);

        size_t LoopEndTime = midiContainer.get_timestamp_loop_end(subsong, true);

        if (_LoopStartTime != ~0UL || LoopEndTime != ~0UL)
            _LoopMode |= LoopModeForced;

        if (_LoopStartTime == ~0UL)
            _LoopStartTime = 0;

        if (LoopEndTime == ~0UL)
            LoopEndTime = _TimeEnd - 1000;

        if ((_LoopMode & LoopModeForced))
        {
            std::vector<uint8_t> NoteOn;

            constexpr size_t EventSize = (size_t)128 * 16;

            NoteOn.resize(EventSize, 0);

            ::memset(&NoteOn[0], 0, sizeof(NoteOn));

            size_t i;

            for (i = 0; (i < _Stream.size()) && (i < _LoopEnd); i++)
            {
                uint32_t Event = _Stream.at(i).m_event & 0x800000F0;

                if (Event == 0x90 || Event == 0x80)
                {
                    const unsigned long port = (_Stream.at(i).m_event & 0x7F000000) >> 24;
                    const unsigned long ch = _Stream.at(i).m_event & 0x0F;
                    const unsigned long note = (_Stream.at(i).m_event >> 8) & 0x7F;
                    const bool on = (Event == 0x90) && (_Stream.at(i).m_event & 0xFF0000);
                    const unsigned long bit = (unsigned long)(1 << port);

                    size_t Index = (size_t)ch * 128 + note;

                    NoteOn.at(Index) = (uint8_t)((NoteOn.at(Index) & ~bit) | (bit * on));
                }
            }

            _Stream.resize(i);

            _TimeEnd = LoopEndTime - 1;

            if (_TimeEnd < (size_t)_Stream.at(i - 1).m_timestamp)
                _TimeEnd = (size_t)_Stream.at(i - 1).m_timestamp;

            for (unsigned long j = 0; j < 128 * 16; j++)
            {
                if (NoteOn.at(j))
                {
                    for (unsigned long k = 0; k < 8; k++)
                    {
                        if (NoteOn.at(j) & (1 << k))
                        {
                            _Stream.push_back(midi_stream_event((unsigned long)_TimeEnd, static_cast<uint32_t>((k << 24) + (j >> 7) + (j & 0x7F) * 0x100 + 0x90)));
                        }
                    }
                }
            }

            _TimeEnd = LoopEndTime;
        }
    }

    if (_SampleRate != 1000)
    {
        unsigned long SampleRate = static_cast<unsigned long>(_SampleRate);

        _SampleRate = 1000;

        setSampleRate(SampleRate);
    }

    return true;
}

unsigned long MIDIPlayer::Play(audio_sample * samples, unsigned long samplesSize)
{
    assert(_Stream.size());

    if (!startup())
        return 0;

    size_t SamplesDone = 0;

    const unsigned int BlockSize = GetSampleBlockSize();

    size_t BlockOffset = 0;

    // This should be a multiple of block size, and have leftover
    while ((_SamplesRemaining > 0) && (SamplesDone < samplesSize))
    {
        size_t SamplesToDo = _SamplesRemaining;

        {
            if (SamplesToDo > samplesSize - SamplesDone)
                SamplesToDo = samplesSize - SamplesDone;

            if (BlockSize && SamplesToDo > BlockSize)
                SamplesToDo = BlockSize;
        }

        if (SamplesToDo < BlockSize)
        {
            _SamplesRemaining = 0;
            BlockOffset = SamplesToDo;
            break;
        }

        {
            render(samples + SamplesDone * 2, (unsigned long)SamplesToDo);

            SamplesDone += SamplesToDo;
            _TimeCurrent += SamplesToDo;
        }

        _SamplesRemaining -= SamplesToDo;
    }

    while (SamplesDone < samplesSize)
    {
        size_t TimeToDo = _TimeEnd - _TimeCurrent;

        if (TimeToDo > samplesSize - SamplesDone)
            TimeToDo = samplesSize - SamplesDone;

        const size_t TargetTime = TimeToDo + _TimeCurrent;
        size_t StreamEnd = _StreamCurrent;

        while ((StreamEnd < _Stream.size()) && (_Stream.at(StreamEnd).m_timestamp < TargetTime))
            StreamEnd++;

        if (StreamEnd > _StreamCurrent)
        {
            for (; _StreamCurrent < StreamEnd; _StreamCurrent++)
            {
                const midi_stream_event & me = _Stream.at(_StreamCurrent);

                size_t SamplesToDo = me.m_timestamp - _TimeCurrent - BlockOffset;

                if (SamplesToDo > 0)
                {
                    if (SamplesToDo > samplesSize - SamplesDone)
                    {
                        _SamplesRemaining = SamplesToDo - (samplesSize - SamplesDone);
                        SamplesToDo = samplesSize - SamplesDone;
                    }

                    if ((SamplesToDo > 0) && (BlockSize == 0))
                    {
                        render(samples + SamplesDone * 2, (unsigned long)SamplesToDo);

                        SamplesDone += SamplesToDo;
                        _TimeCurrent += SamplesToDo;
                    }

                    if (_SamplesRemaining > 0)
                    {
                        _SamplesRemaining += BlockOffset;
                        return (unsigned long)SamplesDone;
                    }
                }

                if (BlockSize > 0)
                {
                    BlockOffset += SamplesToDo;

                    while (BlockOffset >= BlockSize)
                    {
                        render(samples + SamplesDone * 2, BlockSize);

                        SamplesDone += BlockSize;
                        BlockOffset -= BlockSize;
                        _TimeCurrent += BlockSize;
                    }

                    SendEventWithTimeFiltered(me.m_event, BlockOffset);
                }
                else
                    SendEventFiltered(me.m_event);
            }
        }

        if (SamplesDone < samplesSize)
        {
            size_t SamplesToDo;

            if (_StreamCurrent < _Stream.size())
                SamplesToDo = _Stream.at(_StreamCurrent).m_timestamp;
            else
                SamplesToDo = _TimeEnd;

            SamplesToDo -= _TimeCurrent;

            if (BlockSize > 0)
                BlockOffset = SamplesToDo;

            {
                if (SamplesToDo > samplesSize - SamplesDone)
                    SamplesToDo = samplesSize - SamplesDone;

                if (BlockSize && SamplesToDo > BlockSize)
                    SamplesToDo = BlockSize;
            }

            if (SamplesToDo >= BlockSize)
            {
                {
                    render(samples + SamplesDone * 2, (unsigned long)SamplesToDo);

                    SamplesDone += SamplesToDo;
                    _TimeCurrent += SamplesToDo;
                }

                if (BlockSize > 0)
                    BlockOffset -= SamplesToDo;
            }
        }

        if (BlockSize == 0)
            _TimeCurrent = TargetTime;

        if (TargetTime >= _TimeEnd)
        {
            if (_StreamCurrent < _Stream.size())
            {
                for (; _StreamCurrent < _Stream.size(); _StreamCurrent++)
                {
                    if (BlockSize)
                        SendEventWithTimeFiltered(_Stream.at(_StreamCurrent).m_event, BlockOffset);
                    else
                        SendEventFiltered(_Stream.at(_StreamCurrent).m_event);
                }
            }

            if ((_LoopMode & (LoopModeEnabled | LoopModeForced)) == (LoopModeEnabled | LoopModeForced))
            {
                if (_LoopStart == ~0)
                {
                    _StreamCurrent = 0;
                    _TimeCurrent = 0;
                }
                else
                {
                    _StreamCurrent = _LoopStart;
                    _TimeCurrent = _LoopStartTime;
                }
            }
            else
                break;
        }
    }

    _SamplesRemaining = BlockOffset;

    return (unsigned long)SamplesDone;
}

void MIDIPlayer::Seek(unsigned long seekTime)
{
    if (seekTime >= _TimeEnd)
    {
        if ((_LoopMode & (LoopModeEnabled | LoopModeForced)) == (LoopModeEnabled | LoopModeForced))
        {
            while (seekTime >= _TimeEnd)
                seekTime -= (unsigned long)(_TimeEnd - _LoopStartTime);
        }
        else
        {
            seekTime = (unsigned long)_TimeEnd;
        }
    }

    if (_TimeCurrent > seekTime)
    {
        _StreamCurrent = 0;

        if (!reset())
            shutdown();
    }

    if (!startup())
        return;

    _TimeCurrent = seekTime;

    std::vector<midi_stream_event> filler;

    size_t stream_start = _StreamCurrent;

    for (; _StreamCurrent < _Stream.size() && _Stream.at(_StreamCurrent).m_timestamp < _TimeCurrent; _StreamCurrent++)
        ;

    if (_StreamCurrent == _Stream.size())
        _SamplesRemaining = _TimeEnd - _TimeCurrent;
    else
        _SamplesRemaining = _Stream.at(_StreamCurrent).m_timestamp - _TimeCurrent;

    if (_StreamCurrent > stream_start)
    {
        filler.resize(_StreamCurrent - stream_start);
        filler.assign(&_Stream.at(stream_start), &_Stream.at(_StreamCurrent));

        unsigned long i, j;

        for (i = 0, stream_start = _StreamCurrent - stream_start; i < stream_start; i++)
        {
            midi_stream_event & e = filler.at(i);

            if ((e.m_event & 0x800000F0) == 0x90 && (e.m_event & 0xFF0000)) // note on
            {
                if ((e.m_event & 0x0F) == 9) // hax
                {
                    e.m_event = 0;
                    continue;
                }

                const uint32_t m = (e.m_event & 0x7F00FF0F) | 0x80; // note off
                const uint32_t m2 = (e.m_event & 0x7F00FFFF); // also note off

                for (j = i + 1; j < stream_start; j++)
                {
                    midi_stream_event & e2 = filler.at(j);
                    if ((e2.m_event & 0xFF00FFFF) == m || e2.m_event == m2)
                    {
                        // kill 'em
                        e.m_event = 0;
                        e2.m_event = 0;
                        break;
                    }
                }
            }
        }

        audio_sample * temp;

        const unsigned int BlockSize = GetSampleBlockSize();

        if (BlockSize)
        {
            temp = new audio_sample[BlockSize * 2];

            if (temp)
            {
                render(temp, BlockSize); // flush events

                unsigned int render_junk = 0;
                bool timestamp_set = false;
                unsigned last_timestamp = 0;

                for (i = 0; i < stream_start; i++)
                {
                    if (filler[i].m_event)
                    {
                        SendEventWithTimeFiltered(filler[i].m_event, render_junk);

                        if (timestamp_set)
                        {
                            if (filler[i].m_timestamp != last_timestamp)
                            {
                                render_junk += 16;
                            }
                        }

                        last_timestamp = filler[i].m_timestamp;
                        timestamp_set = true;

                        if (render_junk >= BlockSize)
                        {
                            render(temp, BlockSize);
                            render_junk -= BlockSize;
                        }
                    }
                }

                render(temp, BlockSize);

                delete[] temp;
            }
        }
        else
        {
            temp = new audio_sample[16 * 2];

            if (temp)
            {
                render(temp, 16);

                bool timestamp_set = false;
                unsigned last_timestamp = 0;

                for (i = 0; i < stream_start; i++)
                {
                    if (filler[i].m_event)
                    {
                        if (timestamp_set)
                        {
                            if (filler[i].m_timestamp != last_timestamp)
                            {
                                render(temp, 16);
                            }
                        }

                        last_timestamp = filler[i].m_timestamp;
                        timestamp_set = true;

                        SendEventFiltered(filler[i].m_event);
                    }
                }

                render(temp, 16);

                delete[] temp;
            }
        }
    }
}

bool MIDIPlayer::GetErrorMessage(std::string & errorMessage)
{
    return getErrorMessage(errorMessage);
}

void MIDIPlayer::setSampleRate(unsigned long sampleRate)
{
    if (_Stream.size())
        for (size_t i = 0; i < _Stream.size(); i++)
            _Stream.at(i).m_timestamp = (unsigned long) ((uint64_t) _Stream.at(i).m_timestamp * sampleRate / _SampleRate);

    if (_TimeCurrent > 0)
        _TimeCurrent = static_cast<uint64_t>(_TimeCurrent) * sampleRate / _SampleRate;

    if (_TimeEnd > 0)
        _TimeEnd = static_cast<uint64_t>(_TimeEnd) * sampleRate / _SampleRate;

    if (_LoopStartTime > 0)
        _LoopStartTime = static_cast<uint64_t>(_LoopStartTime) * sampleRate / _SampleRate;

    _SampleRate = sampleRate;

    shutdown();
}

void MIDIPlayer::SetLoopMode(LoopMode loopMode)
{
    if (_LoopMode == (unsigned int)loopMode)
        return;

    if (loopMode & LoopModeEnabled)
        _TimeEnd -= _SampleRate;
    else
        _TimeEnd += _SampleRate;

    _LoopMode = loopMode;
}

void MIDIPlayer::SetFilter(FilterType filterType, bool filterEffects)
{
    _FilterType = filterType;
    _FilterEffects = filterEffects;

    if (_IsInitialized)
    {
        SendSysExReset(0, 0);
        SendSysExReset(1, 0);
        SendSysExReset(2, 0);
    }
}

void MIDIPlayer::SendEventFiltered(uint32_t event)
{
    if (_MusicKeyboard.is_valid())
        _MusicKeyboard->ProcessMessage();

    if (!(event & 0x80000000u))
    {
        if (_FilterEffects)
        {
            const uint32_t _b = event & 0x7FF0;

            if (_b == 0x5BB0 || _b == 0x5DB0)
                return;
        }

        SendEvent(event);
    }
    else
    {
        const unsigned int Index = event & 0xffffff;

        const uint8_t * Data;
        size_t Size, Port;

        _SysExMap.get_entry(Index, Data, Size, Port);

        SendSysExFiltered(Data, Size, Port);
    }
}

void MIDIPlayer::SendEventWithTimeFiltered(uint32_t event, size_t time)
{
    if (_MusicKeyboard.is_valid())
        _MusicKeyboard->ProcessMessage();

    if (!(event & 0x80000000u))
    {
        if (_FilterEffects)
        {
            const uint32_t _b = event & 0x7FF0;

            if (_b == 0x5BB0 || _b == 0x5DB0)
                return;
        }

        SendEventWithTime(event, (unsigned int)time);
    }
    else
    {
        const unsigned int Index = event & 0xffffff;

        const uint8_t * Data;
        size_t Size, Port;

        _SysExMap.get_entry(Index, Data, Size, Port);

        SendSysExWithTimeFiltered(Data, Size, Port, time);
    }
}

#pragma region("SysEx")
static const uint8_t SysExResetGM[]  = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
static const uint8_t SysExResetGM2[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x03, 0xF7 };
static const uint8_t SysExResetGS[]  = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
static const uint8_t SysExResetXG[]  = { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

static const uint8_t syx_gs_limit_bank_lsb[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x41, 0x00, 0x03, 0x00, 0xF7 };

static bool IsSysExReset(const uint8_t * data);

void MIDIPlayer::SendSysExFiltered(const uint8_t * data, size_t size, size_t port)
{
    SendSysEx(data, size, port);

    if (IsSysExReset(data) && (_FilterType != FilterNone))
        SendSysExReset(port, 0);
}

void MIDIPlayer::SendSysExWithTimeFiltered(const uint8_t * data, size_t size, size_t port, size_t time)
{
    SendSysExWithTime(data, size, port, (unsigned int)time);

    if (IsSysExReset(data) && (_FilterType != FilterNone))
        SendSysExReset(port, (unsigned int)time);
}

void MIDIPlayer::SendSysExReset(size_t port, unsigned int time)
{
    if (!_IsInitialized)
        return;

    if (time > 0)
    {
        SendSysExWithTime(&SysExResetXG[0], sizeof(SysExResetXG), port, time);
        SendSysExWithTime(&SysExResetGM2[0], sizeof(SysExResetGM2), port, time);
        SendSysExWithTime(&SysExResetGM[0], sizeof(SysExResetGM), port, time);
    }
    else
    {
        SendSysEx(&SysExResetXG[0], sizeof(SysExResetXG), port);
        SendSysEx(&SysExResetGM2[0], sizeof(SysExResetGM2), port);
        SendSysEx(&SysExResetGM[0], sizeof(SysExResetGM), port);
    }

    switch (_FilterType)
    {
        case FilterGMSysEx:
        /*
            if (time)
                SendSysExWithTime(SysExResetGM, sizeof(SysExResetGM), port, time);
            else
                SendSysEx(SysExResetGM, sizeof(SysExResetGM), port);
        */
            break;

        case FilterGM2SysEx:
            if (time > 0)
                SendSysExWithTime(&SysExResetGM2[0], sizeof(SysExResetGM2), port, time);
            else
                SendSysEx(&SysExResetGM2[0], sizeof(SysExResetGM2), port);
            break;

        case FilterSC55SysEx:
        case FilterSC88SysEx:
        case FilterSC88ProSysEx:
        case FilterSC8850SysEx:
        case FilterNone:
            if (time > 0)
                SendSysExWithTime(&SysExResetGS[0], sizeof(SysExResetGS), port, time);
            else
                SendSysEx(&SysExResetGS[0], sizeof(SysExResetGS), port);

            SendSysExResetSC(port, time);
            break;

        case FilterXGSysEx:
            if (time > 0)
                SendSysExWithTime(&SysExResetXG[0], sizeof(SysExResetXG), port, time);
            else
                SendSysEx(&SysExResetXG[0], sizeof(SysExResetXG), port);
            break;
    }

    {
        for (size_t i = 0; i < 16; ++i)
        {
            if (time)
            {
                SendEventWithTime((uint32_t)(0x78B0 + i + (port << 24)), time);
                SendEventWithTime((uint32_t)(0x79B0 + i + (port << 24)), time);

                if (_FilterType != FilterXGSysEx || i != 9)
                {
                    SendEventWithTime((uint32_t)(0x20B0 + i + (port << 24)), time);
                    SendEventWithTime((uint32_t)(0x00B0 + i + (port << 24)), time);
                    SendEventWithTime((uint32_t)(0x00C0 + i + (port << 24)), time);
                }
            }
            else
            {
                SendEvent((uint32_t)(0x78B0 + i + (port << 24)));
                SendEvent((uint32_t)(0x79B0 + i + (port << 24)));

                if (_FilterType != FilterXGSysEx || i != 9)
                {
                    SendEvent((uint32_t)(0x20B0 + i + (port << 24)));
                    SendEvent((uint32_t)(0x00B0 + i + (port << 24)));
                    SendEvent((uint32_t)(0x00C0 + i + (port << 24)));
                }
            }
        }
    }

    if (_FilterType == FilterXGSysEx)
    {
        if (time > 0)
        {
            SendEventWithTime((uint32_t)(0x0020B9 + (port << 24)), time);
            SendEventWithTime((uint32_t)(0x7F00B9 + (port << 24)), time);
            SendEventWithTime((uint32_t)(0x0000C9 + (port << 24)), time);
        }
        else
        {
            SendEvent((uint32_t)(0x0020B9 + (port << 24)));
            SendEvent((uint32_t)(0x7F00B9 + (port << 24)));
            SendEvent((uint32_t)(0x0000C9 + (port << 24)));
        }
    }

    if (_FilterEffects)
    {
        if (time > 0)
        {
            for (size_t i = 0; i < 16; ++i)
            {
                SendEventWithTime((uint32_t)(0x5BB0 + i + (port << 24)), time);
                SendEventWithTime((uint32_t)(0x5DB0 + i + (port << 24)), time);
            }
        }
        else
        {
            for (size_t i = 0; i < 16; ++i)
            {
                SendEvent((uint32_t)(0x5BB0 + i + (port << 24)));
                SendEvent((uint32_t)(0x5DB0 + i + (port << 24)));
            }
        }
    }
}

void MIDIPlayer::SendSysExResetSC(size_t port, unsigned int time)
{
    uint8_t Message[11] = { 0 };

    ::memcpy(&Message[0], &syx_gs_limit_bank_lsb[0], sizeof(Message));

    Message[7] = 1;

    switch (_FilterType)
    {
        case FilterSC55SysEx:
            Message[8] = 1;
            break;

        case FilterSC88SysEx:
            Message[8] = 2;
            break;

        case FilterSC88ProSysEx:
            Message[8] = 3;
            break;

        case FilterSC8850SysEx:
        case FilterNone:
            Message[8] = 4;
            break;

        case FilterGMSysEx:
        case FilterGM2SysEx:
        case FilterXGSysEx:
        default:
            break;
    }

    for (uint8_t i = 0x41; i <= 0x49; ++i)
    {
        Message[6] = i;
        SendSysExGS(&Message[0], sizeof(Message), port, time);
    }

    {
        Message[6] = 0x40;
        SendSysExGS(&Message[0], sizeof(Message), port, time);
    }

    for (uint8_t i = 0x4A; i <= 0x4F; ++i)
    {
        Message[6] = i;
        SendSysExGS(&Message[0], sizeof(Message), port, time);
    }
}

void MIDIPlayer::SendSysExGS(uint8_t * data, size_t size, size_t port, unsigned int time)
{
    size_t i;

    unsigned char Checksum = 0;

    for (i = 5; i + 1 < size && data[i + 1] != 0xF7; ++i)
        Checksum += data[i];

    data[i] = (unsigned char)((128 - Checksum) & 127);

    if (time > 0)
        SendSysExWithTime(data, size, port, time);
    else
        SendSysEx(data, size, port);
}


static bool IsSysExEqual(const uint8_t * a, const uint8_t * b);

static bool IsSysExReset(const uint8_t * data)
{
    return IsSysExEqual(data, &SysExResetGM[0]) || IsSysExEqual(data, &SysExResetGM2[0]) || IsSysExEqual(data, &SysExResetGS[0]) || IsSysExEqual(data, &SysExResetXG[0]);
}

static bool IsSysExEqual(const uint8_t * a, const uint8_t * b)
{
    while ((*a != 0xF7) && (*b != 0xF7) && (*a == *b))
    {
        a++;
        b++;
    }

    return (*a == *b);
}
#pragma endregion
