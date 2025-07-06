
/** $VER: Player.cpp (2025.07.05) **/

#include "pch.h"

#include "Player.h"

#include "Channels.h"

/// <summary>
/// Initializes a new instance.
/// </summary>
player_t::player_t()
{
    _SampleRate = 1'000;

    _Time = 0;
    _TotalTime = 0;
    _LoopBeginTime = 0;

    _TimeRemaining = 0;

    _IsStarted = false;

#ifdef HAVE_FOO_VIS_MIDI
    {
        foo_vis_midi::IAPI::ptr api;

        if (fb2k::std_api_try_get(api))
            _MusicKeyboard = api->GetMusicKeyboard();

        if (_MusicKeyboard.is_valid())
            _MusicKeyboard->Initialize(foo_vis_midi::InterfaceVersion);
    }
#endif
}

/// <summary>
/// Loads the specified MIDI container.
/// </summary>
bool player_t::Load(const midi::container_t & midiContainer, uint32_t subsongIndex, LoopTypes loopType, uint32_t cleanFlags)
{
    _LoopType = loopType;

    // Initialize the event stream.
    {
        assert(_Messages.size() == 0);

        _MessageIndex = 0;

        midiContainer.SerializeAsStream(subsongIndex, _Messages, _SysExMap, _Ports, _LoopBeginIndex, _LoopEndIndex, cleanFlags);

        if (_Messages.size() == 0)
            return false;
    }

    // Initialize the sample stream.
    {
        _Time = 0;
        _TotalTime = midiContainer.GetDuration(subsongIndex, true);

        if (_LoopType == LoopTypes::NeverLoopAddDecayTime)
            _TotalTime += (uint32_t) CfgDecayTime;
        else
        if (_LoopType >= LoopTypes::LoopAndFadeWhenDetected)
        {
            _LoopBeginTime = midiContainer.GetLoopBeginTimestamp(subsongIndex, true);

            if (_LoopBeginTime == ~0UL)
                _LoopBeginTime = 0;

            uint32_t LoopEndTime = midiContainer.GetLoopEndTimestamp(subsongIndex, true);

            if (LoopEndTime == ~0UL)
                LoopEndTime =  _TotalTime;

            // FIXME: I don't have a clue what this does.
            {
                constexpr size_t NoteOnSize = (size_t) 128 * 16;

                std::vector<uint8_t> NoteOn(NoteOnSize, 0);

                {
                    size_t i;

                    for (i = 0; (i < _Messages.size()) && (i < _LoopEndIndex); ++i)
                    {
                        uint32_t Message = _Messages[i].Data;

                        uint32_t Event = Message & 0x800000F0;

                        if (Event == midi::NoteOn || Event == midi::NoteOff)
                        {
                            const unsigned long Port     = (Message >> 24) & 0x7F;
                            const unsigned long Velocity = (Message >> 16) & 0xFF;
                            const unsigned long Note     = (Message >>  8) & 0x7F;
                            const unsigned long Channel  =  Message        & 0x0F;

                            const bool IsNoteOn = (Event == midi::NoteOn) && (Velocity > 0);

                            const unsigned long bit = (unsigned long) (1 << Port);

                            size_t Index = (size_t) Channel * 128 + Note;

                            NoteOn[Index] = (uint8_t) ((NoteOn[Index] & ~bit) | (bit * IsNoteOn));
                        }
                    }

                    _Messages.resize(i);

                    _TotalTime = std::max(LoopEndTime - 1, _Messages[i - 1].Time);
                }

                for (size_t i = 0; i < NoteOnSize; ++i)
                {
                    if (NoteOn[i] != 0x00)
                    {
                        for (size_t j = 0; j < 8; ++j)
                        {
                            if (NoteOn[i] & (1 << j))
                            {
                                _Messages.push_back(midi::message_t(_TotalTime, (uint32_t) ((j << 24) + (i >> 7) + ((i & 0x7F) << 8) + 0x90)));
                            }
                        }
                    }
                }

                _TotalTime = LoopEndTime;
            }
        }
    }

    {
        if (_SampleRate != 1000)
        {
            uint32_t NewSampleRate = _SampleRate;

            _SampleRate = 1000;

            SetSampleRate(NewSampleRate);
        }
    }

    _CfgChannelsVersion = ~0u;
    CfgChannels.Get(_EnabledChannels, sizeof(_EnabledChannels), _CfgChannelsVersion);

    return true;
}

/// <summary>
/// Renders the specified number of samples to an audio sample buffer.
/// </summary>
/// <remarks>All calculations are in samples. The MIDIStreamEvent::Timestamps have already been from ms to samples before playing starts.</remarks>
uint32_t player_t::Play(audio_sample * frameData, uint32_t frameCount) noexcept
{
    assert(_Messages.size());

    if (!Startup())
        return 0;

#ifdef HAVE_FOO_VIS_MIDI
    uint32_t OldPosition = _Time;
#endif

#ifdef _DEBUG
//  wchar_t Line[256]; ::swprintf_s(Line, _countof(Line), L"Event: %6d/%6d | Sample: %8d/%8d | Chunk: %6d, Rem: %8d\n", (int) _MessageIndex, (int) _Messages.size(), (int) _Time, (int) _TotalTime, (int) size, (int) _TimeRemaining); ::OutputDebugStringW(Line);
#endif

    const uint32_t BlockSize = GetBlockSize();
    const uint32_t ChannelCount = 2;

    uint32_t FrameIndex = 0;
    uint32_t BlockToDo = 0;

    while ((FrameIndex < frameCount) && (_TimeRemaining != 0))
    {
        uint32_t Remainder = _TimeRemaining;

        if (Remainder > frameCount - FrameIndex)
            Remainder = frameCount - FrameIndex;

        if ((BlockSize != 0) && (Remainder > BlockSize))
            Remainder = BlockSize;

        if (Remainder < BlockSize)
        {
            _TimeRemaining = 0;
            BlockToDo = Remainder;
            break;
        }

        {
            Render(frameData + (FrameIndex * ChannelCount), Remainder);

            FrameIndex += Remainder;
            _Time += Remainder;
        }

        _TimeRemaining -= Remainder;
    }

    while (FrameIndex < frameCount)
    {
        uint32_t Remainder = _TotalTime - _Time;

        if (Remainder > frameCount - FrameIndex)
            Remainder = frameCount - FrameIndex;

        const uint32_t NewPosition = _Time + Remainder;

        {
            // Determine how many messages to process to reach the new position in the sample stream.
            size_t NewMessageIndex = _MessageIndex;

            while ((NewMessageIndex < _Messages.size()) && (_Messages[NewMessageIndex].Time < NewPosition))
                NewMessageIndex++;

            // Process MIDI events until we've generated enough samples to reach the new position in the sample stream.
            if (NewMessageIndex > _MessageIndex)
            {
                for (; _MessageIndex < NewMessageIndex; ++_MessageIndex)
                {
                    const midi::message_t & mse = _Messages[_MessageIndex];

                #ifdef HAVE_FOO_VIS_MIDI
                    if (_MusicKeyboard.is_valid())
                        _MusicKeyboard->ProcessMessage(mse.Data, mse.Time);
                #endif

                    int64_t ToDo = (int64_t) mse.Time - (int64_t) _Time - (int64_t) BlockToDo;

                    if (ToDo > 0)
                    {
                        if (ToDo > (int64_t) (frameCount - FrameIndex))
                        {
                            _TimeRemaining = (uint32_t) (ToDo - (int64_t) (frameCount - FrameIndex));
                            ToDo = (int64_t) (frameCount - FrameIndex);
                        }

                        if ((ToDo > 0) && (BlockSize == 0))
                        {
                            Render(frameData + (FrameIndex * 2), (uint32_t) ToDo);

                            FrameIndex += (uint32_t) ToDo;
                            _Time += (uint32_t) ToDo;
                        }

                        if (_TimeRemaining != 0)
                        {
                            _TimeRemaining += BlockToDo;

                            return FrameIndex;
                        }
                    }

                    if (BlockSize == 0)
                        SendEventFiltered(mse.Data);
                    else
                    {
                        BlockToDo += (uint32_t) ToDo;

                        while (BlockToDo >= BlockSize)
                        {
                            Render(frameData + (FrameIndex * 2), BlockSize);

                            FrameIndex += BlockSize;
                            BlockToDo -= BlockSize;
                            _Time += BlockSize;
                        }

                        SendEventFiltered(mse.Data, BlockToDo);
                    }
                }
            }
        }

        if (FrameIndex < frameCount)
        {
            Remainder = ((_MessageIndex < _Messages.size()) ? _Messages[_MessageIndex].Time : _TotalTime) - _Time;

            if (BlockSize != 0)
                BlockToDo = Remainder;

            {
                if (Remainder > frameCount - FrameIndex)
                    Remainder = frameCount - FrameIndex;

                if ((BlockSize != 0) && (Remainder > BlockSize))
                    Remainder = BlockSize;
            }

            if (Remainder >= BlockSize)
            {
                {
                    Render(frameData + (FrameIndex * 2), Remainder);

                    FrameIndex += Remainder;
                    _Time += Remainder;
                }

                if (BlockSize != 0)
                    BlockToDo -= Remainder;
            }
        }

        if (BlockSize == 0)
            _Time = NewPosition;

        // Have we reached the end of the song?
        if (NewPosition >= _TotalTime)
        {
            // Process any remaining messages.
            for (; _MessageIndex < _Messages.size(); ++_MessageIndex)
            {
                if (BlockSize == 0)
                    SendEventFiltered(_Messages[_MessageIndex].Data);
                else
                    SendEventFiltered(_Messages[_MessageIndex].Data, BlockToDo);
            }

            if ((_LoopType == LoopTypes::LoopAndFadeWhenDetected) || (_LoopType == LoopTypes::PlayIndefinitelyWhenDetected))
            {
                if (_LoopBeginIndex != ~0)
                {
                    _MessageIndex = _LoopBeginIndex;
                    _Time = _LoopBeginTime;
                }
                else
                    break;
            }
            else
            if ((_LoopType == LoopTypes::LoopAndFadeAlways) || (_LoopType == LoopTypes::PlayIndefinitely))
            {
                _MessageIndex = 0;
                _Time = 0;
            }
            else
                break;
        }
    }

    _TimeRemaining = BlockToDo;

#ifdef HAVE_FOO_VIS_MIDI
    if (_MusicKeyboard.is_valid())
        _MusicKeyboard->SetPosition(OldPosition);
#endif

    return FrameIndex;
}

/// <summary>
/// Seeks to the specified time (in samples)
/// </summary>
void player_t::Seek(uint32_t timeInSamples)
{
    if (timeInSamples >= _TotalTime)
    {
        if (_LoopType >= LoopTypes::LoopAndFadeWhenDetected)
        {
            while (timeInSamples >= _TotalTime)
                timeInSamples -= _TotalTime - _LoopBeginTime;
        }
        else
        {
            timeInSamples = _TotalTime;
        }
    }

    if (_Time > timeInSamples)
    {
        _MessageIndex = 0;

        if (!Reset())
            Shutdown();
    }

    if (!Startup())
        return;

    _Time = timeInSamples;

    size_t OldMessageIndex = _MessageIndex;

    {
        // Find the message that corresponds with the seek time.
        for (; (_MessageIndex < _Messages.size()) && (_Messages[_MessageIndex].Time < _Time); _MessageIndex++)
            ;

        if (_MessageIndex == _Messages.size())
            _TimeRemaining = _TotalTime - _Time;
        else
            _TimeRemaining = _Messages[_MessageIndex].Time - _Time;
    }

    if (_MessageIndex <= OldMessageIndex)
        return;

    std::vector<midi::message_t> FillerEvents(_MessageIndex - OldMessageIndex);

    FillerEvents.assign(&_Messages[OldMessageIndex], &_Messages[_MessageIndex]);

    for (size_t i = 0; i < FillerEvents.size(); ++i)
    {
        midi::message_t & mse1 = FillerEvents[i];

        if ((mse1.Data & 0x800000F0) == 0x90 && (mse1.Data & 0x00FF0000)) // note on
        {
            if ((mse1.Data & 0x0F) == 9) // hax
            {
                mse1.Data = 0;
                continue;
            }

            const uint32_t m1 = (mse1.Data & 0x7F00FF0F) | 0x80; // note off
            const uint32_t m2 = (mse1.Data & 0x7F00FFFF); // also note off

            for (size_t j = i + 1; j < FillerEvents.size(); ++j)
            {
                midi::message_t & mse2 = FillerEvents[j];

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

    const uint32_t BlockSize = GetBlockSize();

    if (BlockSize == 0)
    {
        audio_sample * Temp = new audio_sample[16 * 2];

        if (Temp != nullptr)
        {
            Render(Temp, 16); // Flush events

            uint32_t LastTimestamp = 0;
            bool IsTimestampSet = false;

            for (const midi::message_t & Event : FillerEvents)
            {
                if (Event.Data != 0)
                {
                    if (IsTimestampSet && (Event.Time != LastTimestamp))
                        Render(Temp, 16); // Flush events

                    LastTimestamp = Event.Time;
                    IsTimestampSet = true;

                    SendEventFiltered(Event.Data);
                }
            }

            Render(Temp, 16); // Flush events

            delete[] Temp;
        }
    }
    else
    {
        audio_sample * Temp = new audio_sample[BlockSize * 2];

        if (Temp != nullptr)
        {
            Render(Temp, BlockSize); // Flush events

            uint32_t JunkSize = 0;

            uint32_t LastTimestamp = 0;
            bool IsTimestampSet = false;

            for (const midi::message_t & Event : FillerEvents)
            {
                if (Event.Data != 0)
                {
                    SendEventFiltered(Event.Data, JunkSize);

                    if (IsTimestampSet && (Event.Time != LastTimestamp))
                        JunkSize += 16;

                    LastTimestamp = Event.Time;
                    IsTimestampSet = true;

                    if (JunkSize >= BlockSize)
                    {
                        Render(Temp, BlockSize); // Flush events
                        JunkSize -= BlockSize;
                    }
                }
            }

            Render(Temp, BlockSize); // Flush events

            delete[] Temp;
        }
    }
}

/// <summary>
/// Re-calculates the timestamps whenever the sample rate changes.
/// </summary>
void player_t::SetSampleRate(uint32_t sampleRate)
{
    if (_SampleRate == sampleRate)
        return;

    for (midi::message_t & it : _Messages)
        it.Time = (uint32_t) ::MulDiv((int) it.Time, (int) sampleRate, (int) _SampleRate);

    if (_Time != 0)
        _Time = (uint32_t) ::MulDiv((int) _Time, (int) sampleRate, (int) _SampleRate);

    if (_TotalTime != 0)
        _TotalTime = (uint32_t) ::MulDiv((int) _TotalTime, (int) sampleRate, (int) _SampleRate);

    if (_LoopBeginTime != 0)
        _LoopBeginTime = (uint32_t) ::MulDiv((int) _LoopBeginTime, (int) sampleRate, (int) _SampleRate);

    _SampleRate = sampleRate;

//  Shutdown(); FIXME
}

/// <summary>
/// Configures the MIDI player.
/// </summary>
void player_t::Configure(MIDIFlavors midiFlavor, bool filterEffects)
{
    _MIDIFlavor = midiFlavor;
    _FilterEffects = filterEffects;
/*
    if (_IsStarted)
    {
        for (uint8_t PortNumber = 0; PortNumber < 3; ++PortNumber)
            ResetPort(PortNumber, 0);
    }
*/
}

/// <summary>
/// Sends the event to the engine unless it gets filtered out.
/// </summary>
void player_t::SendEventFiltered(uint32_t data)
{
    if (!(data & 0x80000000u))
    {
        if (FilterEvent(data))
            return;

        if (FilterEffect(data))
            return;

        SendEvent(data);
    }
    else
    {
        const uint32_t Index = data & 0x00FFFFFFu;

        const uint8_t * Data;
        size_t Size;
        uint8_t Port;

        if (_SysExMap.GetItem(Index, Data, Size, Port))
            SendSysExFiltered(Data, Size, Port);
    }
}

/// <summary>
/// Sends the event to the engine unless it gets filtered out.
/// </summary>
void player_t::SendEventFiltered(uint32_t data, uint32_t time)
{
    if (!(data & 0x80000000u))
    {
        if (FilterEvent(data))
            return;

        if (FilterEffect(data))
            return;

        SendEvent(data, time);
    }
    else
    {
        const uint32_t Index = data & 0x00FFFFFFu;

        const uint8_t * Data;
        size_t Size;
        uint8_t Port;

        if (_SysExMap.GetItem(Index, Data, Size, Port))
            SendSysExFiltered(Data, Size, Port, time);
    }
}

/// <summary>
/// Returns true if the event needs to be filtered out.
/// </summary>
bool player_t::FilterEvent(uint32_t data) noexcept
{
    // Send an All Notes Off channel mode message for all disabled channels whenever the selection changes.
    if (CfgChannels.HasChanged(_CfgChannelsVersion))
    {
        CfgChannels.Get(_EnabledChannels, sizeof(_EnabledChannels), _CfgChannelsVersion);

        for (const auto & Port : _Ports)
        {
            uint16_t Mask = 1;

            for (uint8_t Channel = 0; Channel < 16; ++Channel, Mask <<= 1)
            {
                if (_EnabledChannels[Port] & Mask)
                    continue; // because the channel is enabled.

                SendEvent((uint32_t) ((Port << 24) | (midi::ChannelModeMessages::AllNotesOff << 8) | midi::ControlChange | Channel));
            }
        }
    }

    const size_t Port = (data >> 24) & 0x7F;
    const uint8_t StatusCode = data & 0xF0;
    const uint8_t Channel = data & 0x0F;

    // Filter Note On events for the disabled channels.
    return ((StatusCode == midi::NoteOn) && (((uint16_t) _EnabledChannels[Port] & (1U << Channel)) == 0));
}

/// <summary>
/// Returns true if the effect needs to be filtered out.
/// </summary>
bool player_t::FilterEffect(uint32_t data) const noexcept
{
    if (!_FilterEffects)
        return false;

    const uint32_t Data = data & 0x00007FF0u;

    // Filter Control Change "Effects 1 (External Effects) Depth" (0x5B) and "Effects 3 (Chorus) Depth" (0x5D)
    return (Data == 0x5BB0 || Data == 0x5DB0);
}

#pragma region SysEx

/// <summary>
/// Sends a SysEx.
/// </summary>
void player_t::SendSysExFiltered(const uint8_t * data, size_t size, uint8_t portNumber)
{
    SendSysEx(data, size, portNumber);

    if (midi::sysex_t::IsReset(data) && (_MIDIFlavor != MIDIFlavors::Default))
        ResetPort(portNumber, 0);
}

/// <summary>
/// Sends a SysEx with a timestamp.
/// </summary>
void player_t::SendSysExFiltered(const uint8_t * data, size_t size, uint8_t portNumber, uint32_t time)
{
    SendSysEx(data, size, portNumber, time);

    if (midi::sysex_t::IsReset(data) && (_MIDIFlavor != MIDIFlavors::Default))
        ResetPort(portNumber, time);
}

/// <summary>
/// Resets the specified port.
/// </summary>
void player_t::ResetPort(uint8_t portNumber, uint32_t time)
{
    if (!_IsStarted)
        return;

    if (time == 0)
    {
        SendSysEx(midi::sysex_t::XGReset, _countof(midi::sysex_t::XGReset), portNumber);
        SendSysEx(midi::sysex_t::GM2Reset, _countof(midi::sysex_t::GM2Reset), portNumber);
        SendSysEx(midi::sysex_t::GSReset, _countof(midi::sysex_t::GSReset), portNumber);
        SendSysEx(midi::sysex_t::GMReset, _countof(midi::sysex_t::GMReset), portNumber);
    }
    else
    {
        SendSysEx(midi::sysex_t::XGReset, _countof(midi::sysex_t::XGReset), portNumber, time);
        SendSysEx(midi::sysex_t::GM2Reset, _countof(midi::sysex_t::GM2Reset), portNumber, time);
        SendSysEx(midi::sysex_t::GSReset, _countof(midi::sysex_t::GSReset), portNumber, time);
        SendSysEx(midi::sysex_t::GMReset, _countof(midi::sysex_t::GMReset), portNumber, time);
    }

    switch (_MIDIFlavor)
    {
        case MIDIFlavors::GM:
        {
            if (time != 0)
                SendSysEx(midi::sysex_t::GMReset, _countof(midi::sysex_t::GMReset), portNumber, time);
            else
                SendSysEx(midi::sysex_t::GMReset, _countof(midi::sysex_t::GMReset), portNumber);
            break;
        }

        case MIDIFlavors::GM2:
        {
            if (time != 0)
                SendSysEx(midi::sysex_t::GM2Reset, _countof(midi::sysex_t::GM2Reset), portNumber, time);
            else
                SendSysEx(midi::sysex_t::GM2Reset, _countof(midi::sysex_t::GM2Reset), portNumber);
            break;
        }

        case MIDIFlavors::SC55:
        case MIDIFlavors::SC88:
        case MIDIFlavors::SC88Pro:
        case MIDIFlavors::SC8850:
//      case MIDIFlavors::Default: // Removed in v2.19: Default flavor = Don't send anything
        {
            if (time != 0)
                SendSysEx(midi::sysex_t::GSReset, _countof(midi::sysex_t::GSReset), portNumber, time);
            else
                SendSysEx(midi::sysex_t::GSReset, _countof(midi::sysex_t::GSReset), portNumber);

            SendSysExSetToneMapNumber(portNumber, time);
            break;
        }

        case MIDIFlavors::XG:
        {
            if (time != 0)
                SendSysEx(midi::sysex_t::XGReset, _countof(midi::sysex_t::XGReset), portNumber, time);
            else
                SendSysEx(midi::sysex_t::XGReset, _countof(midi::sysex_t::XGReset), portNumber);
            break;
        }

        case MIDIFlavors::Default:
            break;
    }

    {
        for (uint8_t i = 0; i < 16; ++i)
        {
            if (time != 0)
            {
                SendEvent((uint32_t) ((0x78B0 + i) + (portNumber << 24)), time); // CC 120 Channel Mute / Sound Off
                SendEvent((uint32_t) ((0x79B0 + i) + (portNumber << 24)), time); // CC 121 Reset All Controllers

                if (_MIDIFlavor != MIDIFlavors::XG || i != 9)
                {
                    SendEvent((uint32_t) ((0x20B0 + i) + (portNumber << 24)), time); // CC 32 Bank select LSB
                    SendEvent((uint32_t) ((0x00B0 + i) + (portNumber << 24)), time); // CC  0 Bank select MSB
                    SendEvent((uint32_t) ((0x00C0 + i) + (portNumber << 24)), time); // Program Change 0
                }
            }
            else
            {
                SendEvent((uint32_t) ((0x78B0 + i) + (portNumber << 24))); // CC 120 Channel Mute / Sound Off
                SendEvent((uint32_t) ((0x79B0 + i) + (portNumber << 24))); // CC 121 Reset All Controllers

                if (_MIDIFlavor != MIDIFlavors::XG || i != 9)
                {
                    SendEvent((uint32_t) ((0x20B0 + i) + (portNumber << 24))); // CC 32 Bank select LSB
                    SendEvent((uint32_t) ((0x00B0 + i) + (portNumber << 24))); // CC  0 Bank select MSB
                    SendEvent((uint32_t) ((0x00C0 + i) + (portNumber << 24))); // Program Change 0
                }
            }
        }
    }

    // Configure channel 10 as drum kit in XG mode.
    if (_MIDIFlavor == MIDIFlavors::XG)
    {
        if (time != 0)
        {
            SendEvent((uint32_t) (0x0020B9 + (portNumber << 24)), time); // CC 32 Bank select LSB
            SendEvent((uint32_t) (0x7F00B9 + (portNumber << 24)), time); // CC  0 Bank select MSB. Selects Drum Kit in XG mode.
            SendEvent((uint32_t) (0x0000C9 + (portNumber << 24)), time); // Program Change 0
        }
        else
        {
            SendEvent((uint32_t) (0x0020B9 + (portNumber << 24))); // CC 32 Bank select LSB
            SendEvent((uint32_t) (0x7F00B9 + (portNumber << 24))); // CC  0 Bank select MSB. Selects Drum Kit in XG mode.
            SendEvent((uint32_t) (0x0000C9 + (portNumber << 24))); // Program Change 0
        }
    }

    if (_FilterEffects)
    {
        if (time != 0)
        {
            for (uint8_t  i = 0; i < 16; ++i)
            {
                SendEvent((uint32_t) (0x5BB0 + i + (portNumber << 24)), time); // CC 91 Effect 1 (Reverb) Set Level to 0
                SendEvent((uint32_t) (0x5DB0 + i + (portNumber << 24)), time); // CC 93 Effect 3 (Chorus) Set Level to 0
            }
        }
        else
        {
            for (uint8_t i = 0; i < 16; ++i)
            {
                SendEvent((uint32_t) (0x5BB0 + i + (portNumber << 24))); // CC 91 Effect 1 (Reverb) Set Level to 0
                SendEvent((uint32_t) (0x5DB0 + i + (portNumber << 24))); // CC 93 Effect 3 (Chorus) Set Level to 0
            }
        }
    }
}

/// <summary>
/// Sends a GS SET TONE MAP-0 NUMBER message.
/// </summary>
void player_t::SendSysExSetToneMapNumber(uint8_t portNumber, uint32_t time)
{
    uint8_t Data[11] = { 0 };

    ::memcpy(Data, midi::sysex_t::GSToneMapNumber, _countof(Data));

    Data[7] = 1; // Tone Map-0 Number

    switch (_MIDIFlavor)
    {
        case MIDIFlavors::SC55:
            Data[8] = 1;
            break;

        case MIDIFlavors::SC88:
            Data[8] = 2;
            break;

        case MIDIFlavors::SC88Pro:
            Data[8] = 3;
            break;

        case MIDIFlavors::SC8850:
        case MIDIFlavors::Default:
            Data[8] = 4;
            break;

        case MIDIFlavors::GM:
        case MIDIFlavors::GM2:
        case MIDIFlavors::XG:
        default:
            Data[8] = 3; // Use SC88Pro Map (3)
            break; 
    }

    for (uint8_t i = 0x41; i <= 0x49; ++i)
    {
        Data[6] = i;
        SendSysExGS(Data, _countof(Data), portNumber, time);
    }

    {
        Data[6] = 0x40;
        SendSysExGS(Data, _countof(Data), portNumber, time);
    }

    for (uint8_t i = 0x4A; i <= 0x4F; ++i)
    {
        Data[6] = i;
        SendSysExGS(Data, _countof(Data), portNumber, time);
    }
}

/// <summary>
/// Sends a Roland GS message after re-calculating the checksum.
/// </summary>
void player_t::SendSysExGS(uint8_t * data, size_t size, uint8_t portNumber, uint32_t time)
{
    midi::sysex_t::SetRolandCheckSum(data, size);

    if (time > 0)
        SendSysEx(data, size, portNumber, time);
    else
        SendSysEx(data, size, portNumber);
}

#pragma endregion

#pragma region Private

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
uint32_t player_t::GetProcessorArchitecture(const fs::path & filePath) const
{
    constexpr size_t MZHeaderSize = 0x40;
    constexpr size_t PEHeaderSize = (size_t) 4 + 20 + 224;

    uint8_t PEHeader[PEHeaderSize];

    pfc::string URI = "file://";

    URI += (const char *) filePath.u8string().c_str();

    try
    {
        file::ptr File;

        filesystem::g_open(File, URI.c_str(), filesystem::open_mode_read, fb2k::noAbort);

        File->read_object(PEHeader, MZHeaderSize, fb2k::noAbort);

        if (GetWord(PEHeader) != 0x5A4D)
            return 0;

        uint32_t OffsetPEHeader = GetDWord(PEHeader + 0x3C);

        File->seek(OffsetPEHeader, fb2k::noAbort);
        File->read_object(PEHeader, PEHeaderSize, fb2k::noAbort);

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
