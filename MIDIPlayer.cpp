
/** $VER: MIDIPlayer.cpp (2023.08.13) **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 5045 ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPlayer.h"
#include "Configuration.h"

/// <summary>
/// Initializes a new instance.
/// </summary>
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

/// <summary>
/// Loads the specified MIDI container.
/// </summary>
bool MIDIPlayer::Load(const MIDIContainer & midiContainer, unsigned subsongIndex, LoopMode loopMode, unsigned cleanFlags)
{
    assert(_Stream.size() == 0);

    midiContainer.SerializeAsStream(subsongIndex, _Stream, _SysExMap, _LoopBegin, _LoopEnd, cleanFlags);

    if (_Stream.size() == 0)
        return false;

    _CurrentPosition = 0;
    _CurrentTime = 0;
    _EndTime = (size_t) midiContainer.GetDuration(subsongIndex, true) + CfgDecayTime;

    _LoopMode = (LoopMode) loopMode;

    if (_LoopMode & LoopModeEnabled)
    {
        _LoopBeginTime = midiContainer.GetLoopBeginTimestamp(subsongIndex, true);

        size_t LoopEndTime = midiContainer.GetLoopEndTimestamp(subsongIndex, true);

        if (_LoopBeginTime != ~0UL || LoopEndTime != ~0UL)
            _LoopMode |= LoopModeForced;

        if (_LoopBeginTime == ~0UL)
            _LoopBeginTime = 0;

        if (LoopEndTime == ~0UL)
            LoopEndTime =  _EndTime - CfgDecayTime;

        if ((_LoopMode & LoopModeForced))
        {
            constexpr size_t NoteOnSize = (size_t) 128 * 16;

            std::vector<uint8_t> NoteOn(NoteOnSize, 0);

            {
                size_t i;

                for (i = 0; (i < _Stream.size()) && (i < _LoopEnd); ++i)
                {
                    uint32_t Message = _Stream.at(i).Data;

                    uint32_t Event = Message & 0x800000F0;

                    if (Event == StatusCodes::NoteOn || Event == StatusCodes::NoteOff)
                    {
                        const unsigned long Port     = (Message >> 24) & 0x7F;
                        const unsigned long Velocity = (Message >> 16) & 0xFF;
                        const unsigned long Note     = (Message >>  8) & 0x7F;
                        const unsigned long Channel  =  Message        & 0x0F;

                        const bool IsNoteOn = (Event == StatusCodes::NoteOn) && (Velocity > 0);

                        const unsigned long bit = (unsigned long) (1 << Port);

                        size_t Index = (size_t) Channel * 128 + Note;

                        NoteOn.at(Index) = (uint8_t) ((NoteOn.at(Index) & ~bit) | (bit * IsNoteOn));
                    }
                }

                _Stream.resize(i);

                _EndTime = LoopEndTime - 1;

                if (_EndTime < (size_t) _Stream.at(i - 1).Timestamp)
                    _EndTime = (size_t) _Stream.at(i - 1).Timestamp;
            }

            for (size_t i = 0; i < NoteOnSize; ++i)
            {
                if (NoteOn.at(i))
                {
                    for (size_t j = 0; j < 8; ++j)
                    {
                        if (NoteOn.at(i) & (1 << j))
                        {
                            _Stream.push_back(MIDIStreamEvent((uint32_t) _EndTime, (uint32_t) ((j << 24) + (i >> 7) + ((i & 0x7F) << 8) + 0x90)));
                        }
                    }
                }
            }

            _EndTime = LoopEndTime;
        }
    }

    if (_SampleRate != 1000)
    {
        uint32_t SampleRate = _SampleRate;

        _SampleRate = 1000;

        SetSampleRate(SampleRate);
    }

    return true;
}

/// <summary>
/// Renders the specified number of samples to an audio sample buffer.
/// </summary>
size_t MIDIPlayer::Play(audio_sample * sampleData, size_t sampleCount)
{
    assert(_Stream.size());

    if (!Startup())
        return 0;

    const uint32_t BlockSize = GetSampleBlockSize();

    size_t BlockOffset = 0;

    size_t Done = 0;

    while ((_SamplesRemaining > 0) && (Done < sampleCount))
    {
        size_t ToDo = _SamplesRemaining;

        {
            if (ToDo > sampleCount - Done)
                ToDo = sampleCount - Done;

            if (BlockSize && ToDo > BlockSize)
                ToDo = BlockSize;
        }

        if (ToDo < BlockSize)
        {
            _SamplesRemaining = 0;
            BlockOffset = ToDo;
            break;
        }

        {
            Render(sampleData + Done * 2, (unsigned long) ToDo);

            Done += ToDo;
            _CurrentTime += ToDo;
        }

        _SamplesRemaining -= ToDo;
    }

    while (Done < sampleCount)
    {
        size_t TimeToDo = _EndTime - _CurrentTime;

        if (TimeToDo > sampleCount - Done)
            TimeToDo = sampleCount - Done;

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

                    int64_t ToDo = (int64_t) me.Timestamp - (int64_t) _CurrentTime - (int64_t) BlockOffset;

                    if (ToDo > 0)
                    {
                        if (ToDo > (int64_t)(sampleCount - Done))
                        {
                            _SamplesRemaining = (size_t) (ToDo - (int64_t) (sampleCount - Done));
                            ToDo = (int64_t) (sampleCount - Done);
                        }

                        if ((ToDo > 0) && (BlockSize == 0))
                        {
                            Render(sampleData + Done * 2, (unsigned long) ToDo);

                            Done += ToDo;
                            _CurrentTime += (uint32_t)ToDo;
                        }

                        if (_SamplesRemaining > 0)
                        {
                            _SamplesRemaining += BlockOffset;
                            return (unsigned long) Done;
                        }
                    }

                    if (BlockSize > 0)
                    {
                        BlockOffset += ToDo;

                        while (BlockOffset >= BlockSize)
                        {
                            Render(sampleData + (Done * 2), BlockSize);

                            Done += BlockSize;
                            BlockOffset -= BlockSize;
                            _CurrentTime += BlockSize;
                        }

                        SendEventFiltered(me.Data, (uint32_t) BlockOffset);
                    }
                    else
                        SendEventFiltered(me.Data);
                }
            }
        }

        if (Done < sampleCount)
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
                if (SamplesToDo > sampleCount - Done)
                    SamplesToDo = sampleCount - Done;

                if ((BlockSize > 0) && (SamplesToDo > BlockSize))
                    SamplesToDo = BlockSize;
            }

            if (SamplesToDo >= BlockSize)
            {
                {
                    Render(sampleData + Done * 2, (unsigned long) SamplesToDo);

                    Done += SamplesToDo;
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
                        SendEventFiltered(_Stream.at(_CurrentPosition).Data, (uint32_t) BlockOffset);
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

    return Done;
}

/// <summary>
/// Seeks to the specified time (in ms)
/// </summary>
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
            _SamplesRemaining = (int64_t)_EndTime - _CurrentTime;
        else
            _SamplesRemaining = (int64_t)_Stream.at(_CurrentPosition).Timestamp - _CurrentTime;
    }

    if (_CurrentPosition <= OldCurrentPosition)
        return;

    std::vector<MIDIStreamEvent> FillerEvents;

    FillerEvents.resize(_CurrentPosition - OldCurrentPosition);
    FillerEvents.assign(&_Stream.at(OldCurrentPosition), &_Stream.at(_CurrentPosition));

    OldCurrentPosition = _CurrentPosition - OldCurrentPosition;

    for (size_t i = 0; i < OldCurrentPosition; ++i)
    {
        MIDIStreamEvent & mse1 = FillerEvents.at(i);

        if ((mse1.Data & 0x800000F0) == 0x90 && (mse1.Data & 0xFF0000)) // note on
        {
            if ((mse1.Data & 0x0F) == 9) // hax
            {
                mse1.Data = 0;
                continue;
            }

            const uint32_t m1 = (mse1.Data & 0x7F00FF0F) | 0x80; // note off
            const uint32_t m2 = (mse1.Data & 0x7F00FFFF); // also note off

            for (size_t j = i + 1; j < OldCurrentPosition; ++j)
            {
                MIDIStreamEvent & mse2 = FillerEvents.at(j);

                if ((mse2.Data & 0xFF00FFFF) == m1 || mse2.Data == m2)
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

    if (BlockSize != 0)
    {
        audio_sample * Temp = new audio_sample[BlockSize * 2];

        if (Temp)
        {
            Render(Temp, BlockSize); // flush events

            uint32_t JunkSize = 0;

            unsigned long LastTimestamp = 0;
            bool IsTimestampSet = false;

            for (size_t i = 0; i < OldCurrentPosition; ++i)
            {
                if (FillerEvents[i].Data != 0)
                {
                    SendEventFiltered(FillerEvents[i].Data, JunkSize);

                    if (IsTimestampSet && (FillerEvents[i].Timestamp != LastTimestamp))
                        JunkSize += 16;

                    LastTimestamp = FillerEvents[i].Timestamp;
                    IsTimestampSet = true;

                    if (JunkSize >= BlockSize)
                    {
                        Render(Temp, BlockSize);
                        JunkSize -= BlockSize;
                    }
                }
            }

            Render(Temp, BlockSize);

            delete[] Temp;
        }
    }
    else
    {
        audio_sample * Temp = new audio_sample[16 * 2];

        if (Temp)
        {
            Render(Temp, 16);

            unsigned long LastTimestamp = 0;
            bool IsTimestampSet = false;

            for (size_t i = 0; i < OldCurrentPosition; ++i)
            {
                if (FillerEvents[i].Data != 0)
                {
                    if (IsTimestampSet && (FillerEvents[i].Timestamp != LastTimestamp))
                        Render(Temp, 16);

                    LastTimestamp = FillerEvents[i].Timestamp;
                    IsTimestampSet = true;

                    SendEventFiltered(FillerEvents[i].Data);
                }
            }

            Render(Temp, 16);

            delete[] Temp;
        }
    }
}

void MIDIPlayer::SetSampleRate(unsigned long sampleRate)
{
    if (_Stream.size() > 0)
        for (size_t i = 0; i < _Stream.size(); ++i)
            _Stream.at(i).Timestamp = (unsigned long) ((uint64_t) _Stream.at(i).Timestamp * sampleRate / _SampleRate);

    if (_CurrentTime > 0)
        _CurrentTime = (uint64_t)(_CurrentTime) * sampleRate / _SampleRate;

    if (_EndTime > 0)
        _EndTime = (uint64_t)(_EndTime) * sampleRate / _SampleRate;

    if (_LoopBeginTime > 0)
        _LoopBeginTime = (uint64_t)(_LoopBeginTime) * sampleRate / _SampleRate;

    _SampleRate = sampleRate;

    Shutdown();
}

void MIDIPlayer::SetLoopMode(LoopMode loopMode)
{
    if (_LoopMode == (uint32_t)loopMode)
        return;

    if (loopMode & LoopModeEnabled)
        _EndTime -= _SampleRate;
    else
        _EndTime += _SampleRate;

    _LoopMode = loopMode;
}

/// <summary>
/// Configures the MIDI player.
/// </summary>
void MIDIPlayer::Configure(ConfigurationType configurationType, bool filterEffects)
{
    _ConfigurationType = configurationType;
    _FilterEffects = filterEffects;

    if (_IsInitialized)
    {
        SendSysExReset(0, 0);
        SendSysExReset(1, 0);
        SendSysExReset(2, 0);
    }
}

void MIDIPlayer::SendEventFiltered(uint32_t data)
{
    if (!(data & 0x80000000u))
    {
        if (_FilterEffects)
        {
            const uint32_t Data = data & 0x00007FF0u;

            // Control Change "Effects 1 (External Effects) Depth" (0x5B) and "Effects 3 (Chorus) Depth" (0x5D)
            if (Data == 0x5BB0 || Data == 0x5DB0)
                return;
        }

        SendEvent(data);
    }
    else
    {
        const uint32_t Index = data & 0x00FFFFFFu;

        const uint8_t * Data;
        size_t Size;
        uint8_t Port;

        _SysExMap.GetItem(Index, Data, Size, Port);

        SendSysExFiltered(Data, Size, Port);
    }
}

void MIDIPlayer::SendEventFiltered(uint32_t data, uint32_t time)
{
    if (!(data & 0x80000000u))
    {
        if (_FilterEffects)
        {
            const uint32_t Data = data & 0x00007FF0u;

            // Control Change "Effects 1 (External Effects) Depth" (0x5B) and "Effects 3 (Chorus) Depth" (0x5D)
            if (Data == 0x5BB0 || Data == 0x5DB0)
                return;
        }

        SendEvent(data, time);
    }
    else
    {
        const uint32_t Index = data & 0x00FFFFFFu;

        const uint8_t * Data;
        size_t Size;
        uint8_t Port;

        _SysExMap.GetItem(Index, Data, Size, Port);

        SendSysExFiltered(Data, Size, Port, time);
    }
}

#pragma region("SysEx")
static const uint8_t SysExResetGM[]         = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
static const uint8_t SysExResetGM2[]        = { 0xF0, 0x7E, 0x7F, 0x09, 0x03, 0xF7 };
static const uint8_t SysExResetGS[]         = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
static const uint8_t SysExResetXG[]         = { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

static const uint8_t SysExGSBankSelectLSB[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x41, 0x00, 0x03, 0x00, 0xF7 };

static bool IsSysExReset(const uint8_t * data);
static bool IsSysExEqual(const uint8_t * a, const uint8_t * b);

/// <summary>
/// Sends a SysEx.
/// </summary>
void MIDIPlayer::SendSysExFiltered(const uint8_t * data, size_t size, uint8_t portNumber)
{
    SendSysEx(data, size, portNumber);

    if (IsSysExReset(data) && (_ConfigurationType != ConfigurationType::None))
        SendSysExReset(portNumber, 0);
}

/// <summary>
/// Sends a SysEx with a timestamp.
/// </summary>
void MIDIPlayer::SendSysExFiltered(const uint8_t * data, size_t size, uint8_t portNumber, uint32_t time)
{
    SendSysEx(data, size, portNumber, time);

    if (IsSysExReset(data) && (_ConfigurationType != ConfigurationType::None))
        SendSysExReset(portNumber, time);
}

/// <summary>
/// Sends a SysEx reset message.
/// </summary>
void MIDIPlayer::SendSysExReset(uint8_t portNumber, uint32_t time)
{
    if (!_IsInitialized)
        return;

    if (time != 0)
    {
        SendSysEx(SysExResetXG, sizeof(SysExResetXG), portNumber, time);
        SendSysEx(SysExResetGM2, sizeof(SysExResetGM2), portNumber, time);
        SendSysEx(SysExResetGM, sizeof(SysExResetGM), portNumber, time);
    }
    else
    {
        SendSysEx(SysExResetXG, sizeof(SysExResetXG), portNumber);
        SendSysEx(SysExResetGM2, sizeof(SysExResetGM2), portNumber);
        SendSysEx(SysExResetGM, sizeof(SysExResetGM), portNumber);
    }

    switch (_ConfigurationType)
    {
        case ConfigurationType::GM:
            if (time != 0)
                SendSysEx(SysExResetGM, sizeof(SysExResetGM), portNumber, time);
            else
                SendSysEx(SysExResetGM, sizeof(SysExResetGM), portNumber);
            break;

        case ConfigurationType::GM2:
            if (time != 0)
                SendSysEx(SysExResetGM2, sizeof(SysExResetGM2), portNumber, time);
            else
                SendSysEx(SysExResetGM2, sizeof(SysExResetGM2), portNumber);
            break;

        case ConfigurationType::SC55:
        case ConfigurationType::SC88:
        case ConfigurationType::SC88Pro:
        case ConfigurationType::SC8850:
        case ConfigurationType::None:
            if (time != 0)
                SendSysEx(SysExResetGS, sizeof(SysExResetGS), portNumber, time);
            else
                SendSysEx(SysExResetGS, sizeof(SysExResetGS), portNumber);

            SendSysExResetSC(portNumber, time);
            SendSysEx(SysExResetGM, sizeof(SysExResetGM), portNumber, time);
            break;

        case ConfigurationType::XG:
            if (time != 0)
                SendSysEx(SysExResetXG, sizeof(SysExResetXG), portNumber, time);
            else
                SendSysEx(SysExResetXG, sizeof(SysExResetXG), portNumber);
            break;
    }

    {
        for (uint8_t i = 0; i < 16; ++i)
        {
            if (time != 0)
            {
                SendEvent((uint32_t) (0x78B0 + i + (portNumber << 24)), time);
                SendEvent((uint32_t) (0x79B0 + i + (portNumber << 24)), time);

                if (_ConfigurationType != ConfigurationType::XG || i != 9)
                {
                    SendEvent((uint32_t) (0x20B0 + i + (portNumber << 24)), time);
                    SendEvent((uint32_t) (0x00B0 + i + (portNumber << 24)), time);
                    SendEvent((uint32_t) (0x00C0 + i + (portNumber << 24)), time);
                }
            }
            else
            {
                SendEvent((uint32_t) (0x78B0 + i + (portNumber << 24)));
                SendEvent((uint32_t) (0x79B0 + i + (portNumber << 24)));

                if (_ConfigurationType != ConfigurationType::XG || i != 9)
                {
                    SendEvent((uint32_t) (0x20B0 + i + (portNumber << 24)));
                    SendEvent((uint32_t) (0x00B0 + i + (portNumber << 24)));
                    SendEvent((uint32_t) (0x00C0 + i + (portNumber << 24)));
                }
            }
        }
    }

    if (_ConfigurationType == ConfigurationType::XG)
    {
        if (time != 0)
        {
            SendEvent((uint32_t) (0x0020B9 + (portNumber << 24)), time);
            SendEvent((uint32_t) (0x7F00B9 + (portNumber << 24)), time);
            SendEvent((uint32_t) (0x0000C9 + (portNumber << 24)), time);
        }
        else
        {
            SendEvent((uint32_t) (0x0020B9 + (portNumber << 24)));
            SendEvent((uint32_t) (0x7F00B9 + (portNumber << 24)));
            SendEvent((uint32_t) (0x0000C9 + (portNumber << 24)));
        }
    }

    if (_FilterEffects)
    {
        if (time != 0)
        {
            for (uint8_t  i = 0; i < 16; ++i)
            {
                SendEvent((uint32_t) (0x5BB0 + i + (portNumber << 24)), time);
                SendEvent((uint32_t) (0x5DB0 + i + (portNumber << 24)), time);
            }
        }
        else
        {
            for (uint8_t i = 0; i < 16; ++i)
            {
                SendEvent((uint32_t) (0x5BB0 + i + (portNumber << 24)));
                SendEvent((uint32_t) (0x5DB0 + i + (portNumber << 24)));
            }
        }
    }
}

/// <summary>
/// Sends a reset message specific to a partular Roland Sound Canvas model.
/// </summary>
void MIDIPlayer::SendSysExResetSC(uint32_t port, uint32_t time)
{
    uint8_t Data[11] = { 0 };

    ::memcpy(Data, SysExGSBankSelectLSB, sizeof(Data));

    Data[7] = 1;

    switch (_ConfigurationType)
    {
        case ConfigurationType::SC55:
            Data[8] = 1;
            break;

        case ConfigurationType::SC88:
            Data[8] = 2;
            break;

        case ConfigurationType::SC88Pro:
            Data[8] = 3;
            break;

        case ConfigurationType::SC8850:
        case ConfigurationType::None:
            Data[8] = 4;
            break;

        case ConfigurationType::GM:
        case ConfigurationType::GM2:
        case ConfigurationType::XG:
        default:
            break;
    }

    for (uint8_t i = 0x41; i <= 0x49; ++i)
    {
        Data[6] = i;
        SendSysExGS(Data, sizeof(Data), port, time);
    }

    {
        Data[6] = 0x40;
        SendSysExGS(Data, sizeof(Data), port, time);
    }

    for (uint8_t i = 0x4A; i <= 0x4F; ++i)
    {
        Data[6] = i;
        SendSysExGS(Data, sizeof(Data), port, time);
    }
}

/// <summary>
/// Sends a Roland GS message after re-calculating the checksum.
/// </summary>
void MIDIPlayer::SendSysExGS(uint8_t * data, size_t size, uint32_t portNumber, uint32_t time)
{
    uint8_t Checksum = 0;
    size_t i;

    for (i = 5; (i + 1 < size) && (data[i + 1] != StatusCodes::SysExEnd); ++i)
        Checksum += data[i];

    data[i] = (uint8_t) ((128 - Checksum) & 127);

    if (time > 0)
        SendSysEx(data, size, portNumber, time);
    else
        SendSysEx(data, size, portNumber);
}

static bool IsSysExReset(const uint8_t * data)
{
    return IsSysExEqual(data, SysExResetGM) || IsSysExEqual(data, SysExResetGM2) || IsSysExEqual(data, SysExResetGS) || IsSysExEqual(data, SysExResetXG);
}

static bool IsSysExEqual(const uint8_t * a, const uint8_t * b)
{
    while ((*a != StatusCodes::SysExEnd) && (*b != StatusCodes::SysExEnd) && (*a == *b))
    {
        a++;
        b++;
    }

    return (*a == *b);
}
#pragma endregion

#pragma region("Private")
static uint16_t GetWord(const uint8_t * data) noexcept
{
    return (uint16_t) (data[0] | (((uint16_t) data[1]) << 8));
}

static uint32_t GetDWord(const uint8_t * data) noexcept
{
    return data[0] | (((uint32_t) data[1]) << 8) | (((uint32_t) data[2]) << 16) | (((uint32_t) data[3]) << 24);
}

/// <summary>
/// Determines the processor architecture of a Windows binary file.
/// </summary>
uint32_t MIDIPlayer::GetProcessorArchitecture(const std::string & filePath) const
{
    constexpr size_t MZHeaderSize = 0x40;
    constexpr size_t PEHeaderSize = (size_t)4 + 20 + 224;

    uint8_t PEHeader[PEHeaderSize];

    std::string URI = "file://";

    URI += filePath;

    try
    {
        file::ptr File;
        abort_callback_dummy AbortHandler;

        filesystem::g_open(File, URI.c_str(), filesystem::open_mode_read, AbortHandler);

        File->read_object(PEHeader, MZHeaderSize, AbortHandler);

        if (GetWord(PEHeader) != 0x5A4D)
            return 0;

        uint32_t OffsetPEHeader = GetDWord(PEHeader + 0x3C);

        File->seek(OffsetPEHeader, AbortHandler);
        File->read_object(PEHeader, PEHeaderSize, AbortHandler);

        if (GetDWord(PEHeader) != 0x00004550)
            return 0;

        switch (GetWord(PEHeader + 4))
        {
            case 0x014C:
                return 32;

            case 0x8664:
                return 64;

            default:
                return 0;
        }
    }
    catch (...)
    {
    }

    return 0;
}

#pragma endregion