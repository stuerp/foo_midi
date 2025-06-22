
/** $VER: EdMPlayer.cpp (2025.06.21) **/

#include "pch.h"

#include "EdMPlayer.h"

#include <SysEx.h>

EdMPlayer::EdMPlayer() : player_t()
{
    _Initialized = false;

    _SynthMode = ModeGM;

    _Buffer = new int32_t[MaxSamples * ChannelCount];
}

EdMPlayer::~EdMPlayer()
{
    delete[] _Buffer;

    Shutdown();
}

bool EdMPlayer::Startup()
{
    if (_Initialized)
        return true;

    for (size_t i = 0; i < _countof(_Module); ++i)
    {
        if (i & 1)
            _Module[i].AttachDevice(new dsa::CSccDevice(_SampleRate, 2));
        else
            _Module[i].AttachDevice(new dsa::COpllDevice(_SampleRate, 2));

        _Module[i].Reset();
    }

    ResetDrumChannels();

    _Initialized = true;

    return true;
}

void EdMPlayer::Shutdown()
{
    if (_Initialized)
        for (size_t i = 0; i < _countof(_Module); ++i)
            delete _Module[i].DetachDevice();

    _Initialized = false;
}

void EdMPlayer::Render(audio_sample * sampleData, uint32_t sampleCount)
{
    while (sampleCount != 0)
    {
        const size_t ToDo = std::min(sampleCount, MaxSamples);
        const size_t SampleCount = ToDo * ChannelCount;

        ::memset(_Buffer, 0, SampleCount * sizeof(_Buffer[0]));

        for (auto & m : _Module)
        {
            int32_t * Src = _Buffer;

            for (size_t i = 0; i < ToDo; ++i)
            {
                int32_t Channel[ChannelCount];

                m.Render(Channel);

                *Src++ += Channel[0];
                *Src++ += Channel[1];
            }
        }

        // Convert the format of the rendered output.
        audio_math::convert_from_int32(_Buffer, SampleCount, sampleData, 1 << 16);

        sampleData  += SampleCount;
        sampleCount -= (uint32_t) ToDo;
    }
}

/// <summary>
/// Sends a message to the library.
/// </summary>
void EdMPlayer::SendEvent(uint32_t data)
{
    const uint8_t Event[3]
    {
        (uint8_t) (data),        // Status
        (uint8_t) (data >>  8),  // Param 1
        (uint8_t) (data >> 16)   // Param 2
    };

    const uint8_t Status   = Event[0] & 0xF0u;
    const uint32_t Channel = Event[0] & 0x0Fu;

    dsa::CMIDIMsgInterpreter mi;

    mi.Interpret(Event[0]);

    if (Event[0] < midi::TimingClock)
    {
        mi.Interpret(Event[1]);

        if (Status != midi::ProgramChange && Status != midi::ChannelPressure)
            mi.Interpret(Event[2]);
    }

    if (Status == midi::ControlChange)
    {
        if (Event[1] == 0)
        {
            if (_SynthMode == ModeXG)
            {
                if (Event[2] == 127)
                    _DrumChannels[Channel] = 1;
                else
                    _DrumChannels[Channel] = 0;
            }
            else
            if (_SynthMode == ModeGM2)
            {
                if (Event[2] == 120)
                    _DrumChannels[Channel] = 1;
                else
                if (Event[2] == 121)
                    _DrumChannels[Channel] = 0;
            }
        }
    }
    else
    if (Status == midi::ProgramChange)
    {
        SetDrumChannel((int)Channel, _DrumChannels[Channel]);
    }

    while (mi.GetMsgCount())
    {
        const dsa::CMIDIMsg & msg = mi.GetMsg();

        _Module[(msg.m_ch * 2) & 7].SendMIDIMsg(msg);

        if (!_DrumChannels[msg.m_ch])
            _Module[(msg.m_ch * 2 + 1) & 7].SendMIDIMsg(msg);

        mi.PopMsg();
    }
}

/// <summary>
/// Sends a SysEx message to the library.
/// </summary>
void EdMPlayer::SendSysEx(const uint8_t * data, size_t size, uint32_t)
{
    dsa::CMIDIMsgInterpreter mi;

    for (size_t n = 0; n < size; ++n)
        mi.Interpret(data[n]);

    if (midi::sysex_t::IsGMReset(data, size))
    {
        ResetDrumChannels();

        _SynthMode = ModeGM;
    }
    else
    if (midi::sysex_t::IsGM2Reset(data, size))
    {
        ResetDrumChannels();

        _SynthMode = ModeGM2;
    }
    else
    if (midi::sysex_t::IsGSReset(data, size))
    {
        ResetDrumChannels();

        _SynthMode = ModeGS;
    }
    else
    if (midi::sysex_t::IsXGReset(data, size))
    {
        ResetDrumChannels();

        _SynthMode = ModeXG;
    }
    else
    if ((_SynthMode == ModeGS) && (size == 11) && (data[0] == 0xF0) && (data[1] == 0x41) && (data[3] == 0x42) && (data[4] == 0x12) && (data[5] == 0x40) && ((data[6] & 0xF0) == 0x10) && (data[10] == 0xF7))
    {
        if (data[7] == 0x02)
        {
            // GS MIDI channel to part assign
            _GSPartToChannel[data[6] & 15] = data[8];
        }
        else
        if (data[7] == 0x15)
        {
            // GS part to rhythm allocation
            uint32_t DrumChannel = _GSPartToChannel[data[6] & 15];

            if (DrumChannel < 16)
                _DrumChannels[DrumChannel] = data[8];
        }
    }

    while (mi.GetMsgCount())
    {
        const dsa::CMIDIMsg & msg = mi.GetMsg();

        _Module[(msg.m_ch * 2) & 7].SendMIDIMsg(msg);

        if (_DrumChannels[msg.m_ch] == 0)
            _Module[(msg.m_ch * 2 + 1) & 7].SendMIDIMsg(msg);

        mi.PopMsg();
    }
}

void EdMPlayer::ResetDrumChannels()
{
    static const uint8_t PartToChannel[16] = { 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15 };

    ::memset(_DrumChannels, 0, sizeof(_DrumChannels));
    _DrumChannels[9] = 1;

    ::memcpy(_GSPartToChannel, PartToChannel, sizeof(_GSPartToChannel));

    for (size_t i = 0; i < _countof(_DrumChannels); ++i)
        SetDrumChannel((int) i, _DrumChannels[i]);
}

void EdMPlayer::SetDrumChannel(int channel, int enable)
{
    for (size_t i = 0; i < _countof(_Module); ++i)
        _Module[i].SetDrumChannel(channel, enable);
}
