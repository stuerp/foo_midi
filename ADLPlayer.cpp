#include "ADLPlayer.h"

ADLPlayer::ADLPlayer() : MIDIPlayer()
{
    ::memset(_Player, 0, sizeof(_Player));
}

ADLPlayer::~ADLPlayer()
{
    shutdown();
}

bool ADLPlayer::startup()
{
    if (_Player[0] && _Player[1] && _Player[2])
        return true;

    int chips_per_port = _ChipCount / 3;
    int chips_round = (_ChipCount % 3) != 0;
    int chips_min = _ChipCount < 3;

    for (size_t i = 0; i < 3; i++)
    {
        ADL_MIDIPlayer * Player = this->_Player[i] = adl_init(_SampleRate);

        if (Player == nullptr)
            return false;

        ::adl_setBank(Player, _BankNumber);
        ::adl_setNumChips(Player, chips_per_port + chips_round * (i == 0) + chips_min * (i != 0));
        ::adl_setNumFourOpsChn(Player, _4OpCount);
        ::adl_setSoftPanEnabled(Player, _FullPanning);
        ::adl_setDeviceIdentifier(Player, i);
        ::adl_switchEmulator(Player, _EmuCore);
        ::adl_reset(Player);
    }

    _IsInitialized = true;

    setFilterMode(_FilterMode, _UseMIDIEffects);

    return true;
}

void ADLPlayer::shutdown()
{
    for (size_t i = 0; i < 3; i++)
    {
        ::adl_close(_Player[i]);
        _Player[i] = nullptr;
    }

    _IsInitialized = false;
}

void ADLPlayer::render(audio_sample * out, unsigned long count)
{
    int16_t buffer[256 * sizeof(audio_sample)];

    while (count)
    {
        size_t todo = count;

        if (todo > 256)
            todo = 256;

        ::memset(out, 0, (todo * 2) * sizeof(audio_sample));

        for (size_t i = 0; i < 3; i++)
        {
            ::adl_generate(_Player[i], (todo * 2), buffer);

            for (size_t j = 0, k = (todo * 2); j < k; j++)
                out[j] += (audio_sample) buffer[j] * (1.0f / 32768.0f);
        }

        out += (todo * 2);
        count -= todo;
    }
}

void ADLPlayer::setCore(unsigned emuCore)
{
    _EmuCore = emuCore;
}

void ADLPlayer::setBank(unsigned bankNumber)
{
    _BankNumber = bankNumber;
}

void ADLPlayer::setChipCount(unsigned chipCount)
{
    _ChipCount = chipCount;
}

void ADLPlayer::setFullPanning(bool enabled)
{
    _FullPanning = enabled;
}

void ADLPlayer::set4OpCount(unsigned count)
{
    _4OpCount = count;
}

void ADLPlayer::send_event(uint32_t message)
{
    ADL_UInt8 Event[3]
    {
        static_cast<ADL_UInt8>(message),
        static_cast<ADL_UInt8>(message >> 8),
        static_cast<ADL_UInt8>(message >> 16)
    };

    unsigned Port = (message >> 24) & 3;

    const ADL_UInt8 Channel = message & 0x0F;
    const ADL_UInt8 Command = message & 0xF0;

    if (Port > 2)
        Port = 0;

    switch (Command)
    {
        case 0x80:
            ::adl_rt_noteOff(_Player[Port], Channel, Event[1]);
            break;

        case 0x90:
            ::adl_rt_noteOn(_Player[Port], Channel, Event[1], Event[2]);
            break;

        case 0xA0:
            ::adl_rt_noteAfterTouch(_Player[Port], Channel, Event[1], Event[2]);
            break;

        case 0xB0:
            ::adl_rt_controllerChange(_Player[Port], Channel, Event[1], Event[2]);
            break;

        case 0xC0:
            ::adl_rt_patchChange(_Player[Port], Channel, Event[1]);
            break;

        case 0xD0:
            ::adl_rt_channelAfterTouch(_Player[Port], Channel, Event[1]);
            break;

        case 0xE0:
            ::adl_rt_pitchBendML(_Player[Port], Channel, Event[2], Event[1]);
            break;
    }
}

void ADLPlayer::send_sysex(const uint8_t * event, size_t size, size_t)
{
    ::adl_rt_systemExclusive(_Player[0], event, size);
    ::adl_rt_systemExclusive(_Player[1], event, size);
    ::adl_rt_systemExclusive(_Player[2], event, size);
}
