
/** $VER: EdMPlayer.cpp (2025.08.03) **/

#include "pch.h"

#include "EdMPlayer.h"

#include <CSccDevice.hpp>
#include <COpllDevice.hpp>

#include <SysEx.h>

EdMPlayer::EdMPlayer() : player_t()
{
    _SynthMode = ModeGM;

    _SrcFrames.resize(MaxFrames * MaxChannels);

    _IsStarted = false;
}

EdMPlayer::~EdMPlayer()
{
    Shutdown();
}

bool EdMPlayer::Startup()
{
    if (_IsStarted)
        return true;

    for (size_t i = 0; i < _countof(_Modules); ++i)
    {
        if (i & 1)
            _Modules[i].AttachDevice(new dsa::CSccDevice(_SampleRate, 2));
        else
            _Modules[i].AttachDevice(new dsa::COpllDevice(_SampleRate, 2));

        _Modules[i].Reset();
    }

    InitializeDrumChannels();

    _IsStarted = true;

    return true;
}

void EdMPlayer::Shutdown()
{
    if (!_IsStarted)
        return;

    for (size_t i = 0; i < _countof(_Modules); ++i)
        delete _Modules[i].DetachDevice();

    _IsStarted = false;
}

/// <summary>
/// Resets all modules.
/// </summary>
bool EdMPlayer::Reset()
{
    for (auto & Module : _Modules)
        Module.Reset();

    return true;
}

/// <summary>
/// Renders the samples.
/// </summary>
void EdMPlayer::Render(audio_sample * dstFrames, uint32_t dstCount)
{
    int32_t Channel[MaxChannels];

    while (dstCount != 0)
    {
        const size_t srcCount = std::min(dstCount, MaxFrames);

        ::memset(_SrcFrames.data(), 0, _SrcFrames.size() * sizeof(_SrcFrames[0]));

        for (auto & Module : _Modules)
        {
            int32_t * Src = _SrcFrames.data();

            for (size_t i = 0; i < srcCount; ++i)
            {
                Module.Render(Channel);

                *Src++ += Channel[0];
                *Src++ += Channel[1];
            }
        }

        // Convert the format of the rendered output.
        audio_math::convert_from_int32(_SrcFrames.data(), (srcCount * MaxChannels), dstFrames, 1 << 16);

        dstFrames += (srcCount * MaxChannels);
        dstCount -= (uint32_t) srcCount;
    }
}

/// <summary>
/// Sends a message to the library.
/// </summary>
void EdMPlayer::SendEvent(uint32_t data)
{
    const uint8_t Status  = (uint8_t) data & 0xF0u;
    const uint8_t Channel = (uint8_t) data & 0x0Fu;

    const uint8_t Data1   = (uint8_t) (data >>  8);
    const uint8_t Data2   = (uint8_t) (data >> 16);

    auto & LModule = _Modules[ (Channel * 2)      % _countof(_Modules)];
    auto & RModule = _Modules[((Channel * 2) + 1) % _countof(_Modules)];

    switch (Status)
    {
        case midi::NoteOff:
        {
            LModule.SendNoteOff(Channel, Data1, Data2);

            if (!_IsDrumChannel[Channel])
                RModule.SendNoteOff(Channel, Data1, Data2);
            break;
        }

        case midi::NoteOn:
        {
            LModule.SendNoteOn(Channel, Data1, Data2);

            if (!_IsDrumChannel[Channel])
                RModule.SendNoteOn(Channel, Data1, Data2);
            break;
        }

        case midi::KeyPressure:
        {
            break;
        }

        case midi::ControlChange:
        {
            if (Data1 == midi::Controller::BankSelect)
            {
                if (_SynthMode == ModeXG)
                {
                    _IsDrumChannel[Channel] = (Data2 == 127);
                }
                else
                if (_SynthMode == ModeGM2)
                {
                    if (Data2 == 120)
                        _IsDrumChannel[Channel] = true;
                    else
                    if (Data2 == 121)
                        _IsDrumChannel[Channel] = false;
                }
            }

            LModule.SendControlChange(Channel, Data2, Data1);

            if (!_IsDrumChannel[Channel])
                RModule.SendControlChange(Channel, Data2, Data1);
            break;
        }

        case midi::ProgramChange:
        {
            SetDrumChannel((int) Channel, _IsDrumChannel[Channel]);

            LModule.SendProgramChange(Channel, Data1);

            if (!_IsDrumChannel[Channel])
                RModule.SendProgramChange(Channel, Data1);
            break;
        }

        case midi::ChannelPressure:
        {
            LModule.SendChannelPressure(Channel, Data1);

            if (!_IsDrumChannel[Channel])
                RModule.SendChannelPressure(Channel, Data1);
            break;
        }

        case midi::PitchBendChange:
        {
            LModule.SendPitchBend(Channel, Data2, Data1);

            if (!_IsDrumChannel[Channel])
                RModule.SendPitchBend(Channel, Data2, Data1);
            break;
        }

        case midi::SysEx:
        case midi::MIDITimeCodeQtrFrame:
        case midi::SongPositionPointer:
        case midi::SongSelect:

        case midi::TuneRequest:
        case midi::SysExEnd:

        // Real-time events
        case midi::TimingClock:

        case midi::Start:
        case midi::Continue:
        case midi::Stop:

        case midi::ActiveSensing:
        case midi::MetaData:
            break;
    }
}

/// <summary>
/// Sends a SysEx message to the library.
/// </summary>
void EdMPlayer::SendSysEx(const uint8_t * data, size_t size, uint32_t)
{
    if (midi::sysex_t::IsGMSystemOn(data, size))
    {
        InitializeDrumChannels();

        _SynthMode = ModeGM;
    }
    else
    if (midi::sysex_t::IsGM2SystemOn(data, size))
    {
        InitializeDrumChannels();

        _SynthMode = ModeGM2;
    }
    else
    if (midi::sysex_t::IsGSSystemOn(data, size))
    {
        InitializeDrumChannels();

        _SynthMode = ModeGS;
    }
    else
    if (midi::sysex_t::IsXGSystemOn(data, size))
    {
        InitializeDrumChannels();

        _SynthMode = ModeXG;
    }

    if (_SynthMode == ModeGS)
    {
        if (data[7] == 0x02)
        {
            // GS MIDI channel to part assign
            _PartToChannel[data[6] & 15] = data[8];
        }
        else
        if (data[7] == 0x15)
        {
            // GS part to rhythm allocation
            uint32_t DrumChannel = _PartToChannel[data[6] & 15];

            if (DrumChannel < 16)
                _IsDrumChannel[DrumChannel] = (bool) data[8];
        }
    }

    // Not supported yet by libedmidi.
}

/// <summary>
/// Initializes the drum channels.
/// </summary>
void EdMPlayer::InitializeDrumChannels() noexcept
{
    static const uint8_t DefaultPartToChannel[16] = { 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15 };

    ::memcpy(_PartToChannel, DefaultPartToChannel, sizeof(_PartToChannel));

    // Default GM set-up: channel 10 is a drum channel.
    ::memset(_IsDrumChannel, 0, sizeof(_IsDrumChannel));
    _IsDrumChannel[9] = true;

    for (size_t i = 0; i < _countof(_IsDrumChannel); ++i)
        SetDrumChannel((int) i, _IsDrumChannel[i]);
}

/// <summary>
/// Sets or unsets a channel as a drum channel.
/// </summary>
void EdMPlayer::SetDrumChannel(int channel, bool enable) noexcept
{
    for (size_t i = 0; i < _countof(_Modules); ++i)
        _Modules[i].SetDrumChannel(channel, (int) enable);
}
