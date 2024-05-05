
/** $VER: EdMPlayer.cpp (2024.05.05) **/

#include "framework.h"

#include "EdMPlayer.h"

static const uint8_t SysExGMReset[]  = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
static const uint8_t SysExGM2Reset[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x03, 0xF7 };
static const uint8_t SysExGSReset[]  = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
static const uint8_t SysExXGReset[]  = { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

static bool IsGMReset(const uint8_t * data, size_t size) noexcept;
static bool IsGM2Reset(const uint8_t * data, size_t size) noexcept;
static bool IsGSReset(const uint8_t * data, size_t size) noexcept;
static bool IsXGReset(const uint8_t * data, size_t size) noexcept;

EdMPlayer::EdMPlayer() : MIDIPlayer()
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

    for (size_t i = 0; i < 8; ++i)
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
        for (size_t i = 0; i < 8; ++i)
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

void EdMPlayer::SendEvent(uint32_t data)
{
    dsa::CMIDIMsgInterpreter mi;

    uint8_t Data[3] = { (uint8_t) data, (uint8_t) (data >> 8), (uint8_t) (data >> 16) };

    uint32_t StatusCode = data & 0xF0;
    uint32_t Channel = data & 0x0F;

    mi.Interpret(Data[0]);

    if (Data[0] < 0xF8)
    {
        mi.Interpret(Data[1]);

        if (StatusCode != 0xC0 && StatusCode != 0xD0)
            mi.Interpret(Data[2]);
    }

    if (StatusCode == 0xB0 && Data[1] == 0)
    {
        if (_SynthMode == ModeXG)
        {
            if (Data[2] == 127)
                _DrumChannels[Channel] = 1;
            else
                _DrumChannels[Channel] = 0;
        }
        else
        if (_SynthMode == ModeGM2)
        {
            if (Data[2] == 120)
                _DrumChannels[Channel] = 1;
            else
            if (Data[2] == 121)
                _DrumChannels[Channel] = 0;
        }
    }
    else
    if (StatusCode == 0xC0)
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

void EdMPlayer::SendSysEx(const uint8_t * data, size_t size, uint32_t)
{
    dsa::CMIDIMsgInterpreter mi;

    for (size_t n = 0; n < size; ++n)
        mi.Interpret(data[n]);

    if (IsGMReset(data, size))
    {
        ResetDrumChannels();

        _SynthMode = ModeGM;
    }
    else
    if (IsGM2Reset(data, size))
    {
        ResetDrumChannels();

        _SynthMode = ModeGM2;
    }
    else
    if (IsGSReset(data, size))
    {
        ResetDrumChannels();

        _SynthMode = ModeGS;
    }
    else
    if (IsXGReset(data, size))
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

    for (size_t i = 0; i < 16; ++i)
        SetDrumChannel((int) i, _DrumChannels[i]);
}

void EdMPlayer::SetDrumChannel(int channel, int enable)
{
    for (size_t i = 0; i < 8; ++i)
        _Module[i].SetDrumChannel(channel, enable);
}

static bool IsGMReset(const uint8_t * data, size_t size) noexcept
{
    return (size == _countof(SysExGMReset) && ::memcmp(data, SysExGMReset, _countof(SysExGMReset)) == 0);
}

static bool IsGM2Reset(const uint8_t * data, size_t size) noexcept
{
    return (size == _countof(SysExGM2Reset) && ::memcmp(data, SysExGM2Reset, _countof(SysExGM2Reset)) == 0);
}

static bool IsGSReset(const uint8_t * data, size_t size) noexcept
{
    if (size != _countof(SysExGSReset))
        return false;

    if (::memcmp(data, SysExGSReset, 5) != 0)
        return false;

    if (::memcmp(data + 7, SysExGSReset + 7, 2) != 0)
        return false;

    if (((data[5] + data[6] + 1) & 127) != data[9])
        return false;

    if (data[10] != SysExGSReset[10])
        return false;

    return true;
}

static bool IsXGReset(const uint8_t * data, size_t size) noexcept
{
    return (size == _countof(SysExXGReset) && ::memcmp(data, SysExXGReset, _countof(SysExXGReset)) == 0);
}
