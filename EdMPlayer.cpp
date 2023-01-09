
/** $VER: EdMPlayer.cpp (2023.01.02) **/

#pragma warning(disable: 26446 26481 26493)

#include "EdMPlayer.h"

static const uint8_t sysex_gm_reset[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
static const uint8_t sysex_gm2_reset[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x03, 0xF7 };
static const uint8_t sysex_gs_reset[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
static const uint8_t sysex_xg_reset[] = { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

static bool IsGSReset(const uint8_t * data, size_t size)
{
    if (size != _countof(sysex_gs_reset))
        return false;

    if (::memcmp(data, sysex_gs_reset, 5) != 0)
        return false;

    if (::memcmp(data + 7, sysex_gs_reset + 7, 2) != 0)
        return false;

    if (((data[5] + data[6] + 1) & 127) != data[9])
        return false;

    if (data[10] != sysex_gs_reset[10])
        return false;

    return true;
}

EdMPlayer::EdMPlayer() : MIDIPlayer()
{
    _Initialized = false;

    _SynthMode = ModeGM;
}

EdMPlayer::~EdMPlayer()
{
    shutdown();
}

bool EdMPlayer::startup()
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

    reset_drum_channels();

    _Initialized = true;

    return true;
}

void EdMPlayer::shutdown()
{
    if (_Initialized)
        for (unsigned i = 0; i < 8; ++i)
            delete _Module[i].DetachDevice();

    _Initialized = false;
}

void EdMPlayer::render(audio_sample * out, unsigned long count)
{
    INT32 b[256 * sizeof(audio_sample)];

    while (count)
    {
        unsigned long ToDo = 256;

        if (ToDo > count)
            ToDo = count;

        ::memset(b, 0, (ToDo * 2) * sizeof(audio_sample));

        for (size_t i = 0; i < 8; ++i)
        {
            for (size_t j = 0; j < ToDo; ++j)
            {
                INT32 c[2];

                _Module[i].Render(c);

                b[j * 2]     += c[0];
                b[j * 2 + 1] += c[1];
            }
        }

        audio_math::convert_from_int32((const t_int32 *) b, (ToDo * 2), out, 1 << 16);

        out   += (ToDo * 2);
        count -= ToDo;
    }
}

void EdMPlayer::SendEvent(uint32_t message)
{
    dsa::CMIDIMsgInterpreter mi;

    unsigned char event[3];
    event[0] = (unsigned char) message;
    event[1] = (unsigned char) (message >> 8);
    event[2] = (unsigned char) (message >> 16);
    unsigned channel = message & 0x0F;
    unsigned command = message & 0xF0;

    mi.Interpret(event[0]);

    if (event[0] < 0xF8)
    {
        mi.Interpret(event[1]);

        if ((message & 0xF0) != 0xC0 && (message & 0xF0) != 0xD0)
            mi.Interpret(event[2]);
    }

    if (command == 0xB0 && event[1] == 0)
    {
        if (_SynthMode == ModeXG)
        {
            if (event[2] == 127)
                _DrumChannels[channel] = 1;
            else
                _DrumChannels[channel] = 0;
        }
        else
        if (_SynthMode == ModeGM2)
        {
            if (event[2] == 120)
                _DrumChannels[channel] = 1;
            else
            if (event[2] == 121)
                _DrumChannels[channel] = 0;
        }
    }
    else
    if (command == 0xC0)
    {
        set_drum_channel((int)channel, _DrumChannels[channel]);
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

void EdMPlayer::SendSysEx(const uint8_t * event, size_t size, size_t)
{
    dsa::CMIDIMsgInterpreter mi;

    for (uint32_t n = 0; n < size; ++n)
        mi.Interpret(event[n]);

    if ((size == _countof(sysex_gm_reset) && !::memcmp(event, &sysex_gm_reset[0], _countof(sysex_gm_reset))) ||
        (size == _countof(sysex_gm2_reset) && !::memcmp(event, &sysex_gm2_reset[0], _countof(sysex_gm2_reset))) ||
        IsGSReset(event, size) ||
        (size == _countof(sysex_xg_reset) && !::memcmp(event, &sysex_xg_reset[0], _countof(sysex_xg_reset))))
    {
        reset_drum_channels();

        _SynthMode = (size == _countof(sysex_xg_reset)) ? ModeXG :
            (size == _countof(sysex_gs_reset)) ? ModeGS :
            (event[4] == 0x01) ? ModeGM :
            ModeGM2;
    }
    else
    if (_SynthMode == ModeGS && size == 11 &&
        event[0] == 0xF0 && event[1] == 0x41 && event[3] == 0x42 &&
        event[4] == 0x12 && event[5] == 0x40 && (event[6] & 0xF0) == 0x10 &&
        event[10] == 0xF7)
    {
        if (event[7] == 2)
        {
            // GS MIDI channel to part assign
            _GSPartToChannel[event[6] & 15] = event[8];
        }
        else
        if (event[7] == 0x15)
        {
            // GS part to rhythm allocation
            unsigned int drum_channel = _GSPartToChannel[event[6] & 15];

            if (drum_channel < 16)
                _DrumChannels[drum_channel] = event[8];
        }
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

void EdMPlayer::reset_drum_channels()
{
    static const uint8_t PartToChannel[16] = { 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15 };

    ::memset(_DrumChannels, 0, sizeof(_DrumChannels));
    _DrumChannels[9] = 1;

    ::memcpy(_GSPartToChannel, PartToChannel, sizeof(_GSPartToChannel));

    for (size_t i = 0; i < 16; ++i)
        set_drum_channel((int)i, _DrumChannels[i]);
}

void EdMPlayer::set_drum_channel(int channel, int enable)
{
    for (unsigned i = 0; i < 8; ++i)
        _Module[i].SetDrumChannel(channel, enable);
}
