
/** $VER: MIDIPlayer.cpp (2023.05.24) **/

#include "MIDIPlayer.h"

MIDIPlayer::MIDIPlayer()
{
    #ifdef EXPERIMENT
    foo_vis_midi::IAPI::ptr api;

    if (fb2k::std_api_try_get(api))
        _MusicKeyboard = api->GetMusicKeyboard();
    #endif

    _SamplesRemaining = 0;
    _SampleRate = 1000;
    _CurrentTime = 0;
    _EndTime = 0;
    _LoopBeginTime = 0;
    _IsInitialized = false;
}

bool MIDIPlayer::Load(const MIDIContainer & midiContainer, unsigned subsongIndex, unsigned loopMode, unsigned cleanFlags)
{
    assert(_Stream.size() == 0);

    midiContainer.serialize_as_stream(subsongIndex, _Stream, _SysExMap, _LoopBegin, _LoopEnd, cleanFlags);

    if (_Stream.size() == 0)
        return false;

    _CurrentPosition = 0;
    _CurrentTime = 0;
    _EndTime = (size_t)midiContainer.GetDuration(subsongIndex, true) + 1000;

    _LoopMode = (LoopMode)loopMode;

    if (_LoopMode & LoopModeEnabled)
    {
        _LoopBeginTime = midiContainer.GetLoopBeginTimestamp(subsongIndex, true);

        size_t LoopEndTime = midiContainer.GetLoopEndTimestamp(subsongIndex, true);

        if (_LoopBeginTime != ~0UL || LoopEndTime != ~0UL)
            _LoopMode |= LoopModeForced;

        if (_LoopBeginTime == ~0UL)
            _LoopBeginTime = 0;

        if (LoopEndTime == ~0UL)
            LoopEndTime = _EndTime - 1000;

        if ((_LoopMode & LoopModeForced))
        {
            constexpr size_t NoteOnSize = (size_t)128 * 16;

            std::vector<uint8_t> NoteOn(NoteOnSize, 0);

            {
                size_t i;

                for (i = 0; (i < _Stream.size()) && (i < _LoopEnd); i++)
                {
                    uint32_t Message = _Stream.at(i).Data;

                    uint32_t Event = Message & 0x800000F0;

                    if (Event == 0x90 || Event == 0x80)
                    {
                        const unsigned long Port     = (Message >> 24) & 0x7F;
                        const unsigned long Velocity = (Message >> 16) & 0xFF;
                        const unsigned long Note     = (Message >>  8) & 0x7F;
                        const unsigned long Channel  =  Message        & 0x0F;

                        const bool IsNoteOn = (Event == 0x90) && (Velocity > 0);

                        const unsigned long bit = (unsigned long)(1 << Port);

                        size_t Index = (size_t)Channel * 128 + Note;

                        NoteOn.at(Index) = (uint8_t)((NoteOn.at(Index) & ~bit) | (bit * IsNoteOn));
                    }
                }

                _Stream.resize(i);

                _EndTime = LoopEndTime - 1;

                if (_EndTime < (size_t)_Stream.at(i - 1).Timestamp)
                    _EndTime = (size_t)_Stream.at(i - 1).Timestamp;
            }

            for (size_t i = 0; i < NoteOnSize; i++)
            {
                if (NoteOn.at(i))
                {
                    for (size_t j = 0; j < 8; j++)
                    {
                        if (NoteOn.at(i) & (1 << j))
                        {
                            _Stream.push_back(MIDIStreamEvent((unsigned long)_EndTime, static_cast<uint32_t>((j << 24) + (i >> 7) + ((i & 0x7F) << 8) + 0x90)));
                        }
                    }
                }
            }

            _EndTime = LoopEndTime;
        }
    }

    if (_SampleRate != 1000)
    {
        unsigned long SampleRate = static_cast<unsigned long>(_SampleRate);

        _SampleRate = 1000;

        SetSampleRate(SampleRate);
    }

    return true;
}

/// <summary>
/// Renders the specified number of samples to an audio sample buffer.
/// </summary>
size_t MIDIPlayer::Play(audio_sample * samples, size_t samplesSize)
{
    assert(_Stream.size());

    if (!Startup())
        return 0;

    size_t SamplesDone = 0;

    const uint32_t BlockSize = GetSampleBlockSize();

    size_t BlockOffset = 0;

    // This should be a multiple of block size, and have leftover
    while ((_SamplesRemaining > 0) && (SamplesDone < samplesSize))
    {
        size_t SamplesRemaining = _SamplesRemaining;

        {
            if (SamplesRemaining > samplesSize - SamplesDone)
                SamplesRemaining = samplesSize - SamplesDone;

            if (BlockSize && SamplesRemaining > BlockSize)
                SamplesRemaining = BlockSize;
        }

        if (SamplesRemaining < BlockSize)
        {
            _SamplesRemaining = 0;
            BlockOffset = SamplesRemaining;
            break;
        }

        {
            Render(samples + SamplesDone * 2, (unsigned long)SamplesRemaining);

            SamplesDone += SamplesRemaining;
            _CurrentTime += SamplesRemaining;
        }

        _SamplesRemaining -= SamplesRemaining;
    }

    while (SamplesDone < samplesSize)
    {
        size_t TimeToDo = _EndTime - _CurrentTime;

        if (TimeToDo > samplesSize - SamplesDone)
            TimeToDo = samplesSize - SamplesDone;

        const size_t TargetTime = _CurrentTime + TimeToDo;

        {
            size_t TargetPosition = _CurrentPosition;

            while ((TargetPosition < _Stream.size()) && (_Stream.at(TargetPosition).Timestamp < TargetTime))
                TargetPosition++;

            if (TargetPosition > _CurrentPosition)
            {
                for (; _CurrentPosition < TargetPosition; _CurrentPosition++)
                {
                    const MIDIStreamEvent & me = _Stream.at(_CurrentPosition);

                #ifdef EXPERIMENT
                    if (_MusicKeyboard.is_valid())
                        _MusicKeyboard->ProcessMessage(me.Data, me.Timestamp);
                #endif

                    size_t SamplesToDo = me.Timestamp - _CurrentTime - BlockOffset;

                    if (SamplesToDo > 0)
                    {
                        if (SamplesToDo > samplesSize - SamplesDone)
                        {
                            _SamplesRemaining = SamplesToDo - (samplesSize - SamplesDone);
                            SamplesToDo = samplesSize - SamplesDone;
                        }

                        if ((SamplesToDo > 0) && (BlockSize == 0))
                        {
                            Render(samples + SamplesDone * 2, (unsigned long)SamplesToDo);

                            SamplesDone += SamplesToDo;
                            _CurrentTime += SamplesToDo;
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
                            Render(samples + SamplesDone * 2, BlockSize);

                            SamplesDone += BlockSize;
                            BlockOffset -= BlockSize;
                            _CurrentTime += BlockSize;
                        }

                        SendEventWithTimeFiltered(me.Data, BlockOffset);
                    }
                    else
                        SendEventFiltered(me.Data);
                }
            }
        }

        if (SamplesDone < samplesSize)
        {
            size_t SamplesToDo;

            if (_CurrentPosition < _Stream.size())
                SamplesToDo = _Stream.at(_CurrentPosition).Timestamp;
            else
                SamplesToDo = _EndTime;

            SamplesToDo -= _CurrentTime;

            if (BlockSize > 0)
                BlockOffset = SamplesToDo;

            {
                if (SamplesToDo > samplesSize - SamplesDone)
                    SamplesToDo = samplesSize - SamplesDone;

                if ((BlockSize > 0) && (SamplesToDo > BlockSize))
                    SamplesToDo = BlockSize;
            }

            if (SamplesToDo >= BlockSize)
            {
                {
                    Render(samples + SamplesDone * 2, (unsigned long)SamplesToDo);

                    SamplesDone += SamplesToDo;
                    _CurrentTime += SamplesToDo;
                }

                if (BlockSize > 0)
                    BlockOffset -= SamplesToDo;
            }
        }

        if (BlockSize == 0)
            _CurrentTime = TargetTime;

        if (TargetTime >= _EndTime)
        {
            if (_CurrentPosition < _Stream.size())
            {
                for (; _CurrentPosition < _Stream.size(); _CurrentPosition++)
                {
                    if (BlockSize > 0)
                        SendEventWithTimeFiltered(_Stream.at(_CurrentPosition).Data, BlockOffset);
                    else
                        SendEventFiltered(_Stream.at(_CurrentPosition).Data);
                }
            }

            if ((_LoopMode & (LoopModeEnabled | LoopModeForced)) == (LoopModeEnabled | LoopModeForced))
            {
                if (_LoopBegin == ~0)
                {
                    _CurrentPosition = 0;
                    _CurrentTime = 0;
                }
                else
                {
                    _CurrentPosition = _LoopBegin;
                    _CurrentTime = _LoopBeginTime;
                }
            }
            else
                break;
        }
    }

    _SamplesRemaining = BlockOffset;

    return SamplesDone;
}

void MIDIPlayer::Seek(unsigned long seekTime)
{
    if (seekTime >= _EndTime)
    {
        if ((_LoopMode & (LoopModeEnabled | LoopModeForced)) == (LoopModeEnabled | LoopModeForced))
        {
            while (seekTime >= _EndTime)
                seekTime -= (unsigned long)(_EndTime - _LoopBeginTime);
        }
        else
        {
            seekTime = (unsigned long)_EndTime;
        }
    }

    if (_CurrentTime > seekTime)
    {
        _CurrentPosition = 0;

        if (!Reset())
            Shutdown();
    }

    if (!Startup())
        return;

    _CurrentTime = seekTime;

    size_t OldCurrentPosition = _CurrentPosition;

    {
        // Find the position in the MIDI stream that corresponds with the seek time.
        for (; (_CurrentPosition < _Stream.size()) && (_Stream.at(_CurrentPosition).Timestamp < _CurrentTime); _CurrentPosition++)
            ;

        if (_CurrentPosition == _Stream.size())
            _SamplesRemaining = _EndTime - _CurrentTime;
        else
            _SamplesRemaining = _Stream.at(_CurrentPosition).Timestamp - _CurrentTime;
    }

    if (_CurrentPosition <= OldCurrentPosition)
        return;

    std::vector<MIDIStreamEvent> FillerEvents;

    FillerEvents.resize(_CurrentPosition - OldCurrentPosition);
    FillerEvents.assign(&_Stream.at(OldCurrentPosition), &_Stream.at(_CurrentPosition));

    OldCurrentPosition = _CurrentPosition - OldCurrentPosition;

    for (size_t i = 0; i < OldCurrentPosition; i++)
    {
        MIDIStreamEvent & mse1 = FillerEvents.at(i);

        if ((mse1.Data & 0x800000F0) == 0x90 && (mse1.Data & 0xFF0000)) // note on
        {
            if ((mse1.Data & 0x0F) == 9) // hax
            {
                mse1.Data = 0;
                continue;
            }

            const uint32_t m  = (mse1.Data & 0x7F00FF0F) | 0x80; // note off
            const uint32_t m2 = (mse1.Data & 0x7F00FFFF); // also note off

            for (size_t j = i + 1; j < OldCurrentPosition; j++)
            {
                MIDIStreamEvent & mse2 = FillerEvents.at(j);

                if ((mse2.Data & 0xFF00FFFF) == m || mse2.Data == m2)
                {
                    // kill 'em
                    mse1.Data = 0;
                    mse2.Data = 0;
                    break;
                }
            }
        }
    }

    const uint32_t BlockSize = GetSampleBlockSize();

    if (BlockSize > 0)
    {
        audio_sample * temp = new audio_sample[BlockSize * 2];

        if (temp)
        {
            Render(temp, BlockSize); // flush events

            unsigned int JunkSize = 0;

            unsigned long LastTimestamp = 0;
            bool IsTimestampSet = false;

            for (size_t i = 0; i < OldCurrentPosition; i++)
            {
                if (FillerEvents[i].Data != 0)
                {
                    SendEventWithTimeFiltered(FillerEvents[i].Data, JunkSize);

                    if (IsTimestampSet && (FillerEvents[i].Timestamp != LastTimestamp))
                        JunkSize += 16;

                    LastTimestamp = FillerEvents[i].Timestamp;
                    IsTimestampSet = true;

                    if (JunkSize >= BlockSize)
                    {
                        Render(temp, BlockSize);
                        JunkSize -= BlockSize;
                    }
                }
            }

            Render(temp, BlockSize);

            delete[] temp;
        }
    }
    else
    {
        audio_sample * temp = new audio_sample[16 * 2];

        if (temp)
        {
            Render(temp, 16);

            unsigned long LastTimestamp = 0;
            bool IsTimestampSet = false;

            for (size_t i = 0; i < OldCurrentPosition; i++)
            {
                if (FillerEvents[i].Data != 0)
                {
                    if (IsTimestampSet && (FillerEvents[i].Timestamp != LastTimestamp))
                        Render(temp, 16);

                    LastTimestamp = FillerEvents[i].Timestamp;
                    IsTimestampSet = true;

                    SendEventFiltered(FillerEvents[i].Data);
                }
            }

            Render(temp, 16);

            delete[] temp;
        }
    }
}

void MIDIPlayer::SetSampleRate(unsigned long sampleRate)
{
    if (_Stream.size() > 0)
        for (size_t i = 0; i < _Stream.size(); i++)
            _Stream.at(i).Timestamp = (unsigned long) ((uint64_t) _Stream.at(i).Timestamp * sampleRate / _SampleRate);

    if (_CurrentTime > 0)
        _CurrentTime = static_cast<uint64_t>(_CurrentTime) * sampleRate / _SampleRate;

    if (_EndTime > 0)
        _EndTime = static_cast<uint64_t>(_EndTime) * sampleRate / _SampleRate;

    if (_LoopBeginTime > 0)
        _LoopBeginTime = static_cast<uint64_t>(_LoopBeginTime) * sampleRate / _SampleRate;

    _SampleRate = sampleRate;

    Shutdown();
}

void MIDIPlayer::SetLoopMode(LoopMode loopMode)
{
    if (_LoopMode == (unsigned int)loopMode)
        return;

    if (loopMode & LoopModeEnabled)
        _EndTime -= _SampleRate;
    else
        _EndTime += _SampleRate;

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
static bool IsSysExEqual(const uint8_t * a, const uint8_t * b);

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
