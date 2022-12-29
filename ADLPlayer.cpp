// Remove Windows versions and use std namespace versions
#undef min
#undef max

#include "ADLPlayer.h"

#include <libADLMIDI/include/adlmidi.h>

ADLPlayer::ADLPlayer()
    : MIDIPlayer()
{
    memset(midiplay, 0, sizeof(midiplay));
}

ADLPlayer::~ADLPlayer()
{
    shutdown();
}

void ADLPlayer::send_event(uint32_t b)
{
    uint8_t event[3];
    event[0] = static_cast<uint8_t>(b);
    event[1] = static_cast<uint8_t>(b >> 8);
    event[2] = static_cast<uint8_t>(b >> 16);
    unsigned port = (b >> 24) & 3;
    const unsigned channel = b & 0x0F;
    const unsigned command = b & 0xF0;
    if (port > 2)
        port = 0;
    switch (command)
    {
        case 0x80:
            adl_rt_noteOff(midiplay[port], channel, event[1]);
            break;

        case 0x90:
            adl_rt_noteOn(midiplay[port], channel, event[1], event[2]);
            break;

        case 0xA0:
            adl_rt_noteAfterTouch(midiplay[port], channel, event[1], event[2]);
            break;

        case 0xB0:
            adl_rt_controllerChange(midiplay[port], channel, event[1], event[2]);
            break;

        case 0xC0:
            adl_rt_patchChange(midiplay[port], channel, event[1]);
            break;

        case 0xD0:
            adl_rt_channelAfterTouch(midiplay[port], channel, event[1]);
            break;

        case 0xE0:
            adl_rt_pitchBendML(midiplay[port], channel, event[2], event[1]);
            break;
    }
}

void ADLPlayer::send_sysex(const uint8_t * event, size_t size, size_t)
{
    adl_rt_systemExclusive(midiplay[0], event, size);
    adl_rt_systemExclusive(midiplay[1], event, size);
    adl_rt_systemExclusive(midiplay[2], event, size);
}

void ADLPlayer::render(audio_sample * out, unsigned long count)
{
    signed short buffer[256 * sizeof(audio_sample)];

    while (count)
    {
        unsigned todo = count;
        if (todo > 256) todo = 256;

        memset(out, 0, (todo * 2) * sizeof(audio_sample));

        for (unsigned i = 0; i < 3; i++)
        {
            adl_generate(midiplay[i], (todo * 2), buffer);

            for (unsigned j = 0, k = (todo * 2); j < k; j++)
            {
                out[j] += (audio_sample) buffer[j] * (1.0f / 32768.0f);
            }
        }

        out += (todo * 2);
        count -= todo;
    }
}

void ADLPlayer::setCore(unsigned core)
{
    uEmuCore = core;
}

void ADLPlayer::setBank(unsigned bank)
{
    uBankNumber = bank;
}

void ADLPlayer::setChipCount(unsigned count)
{
    uChipCount = count;
}

void ADLPlayer::set4OpCount(unsigned count)
{
    u4OpCount = count;
}

void ADLPlayer::setFullPanning(bool enable)
{
    bFullPanning = enable;
}

void ADLPlayer::shutdown()
{
    for (unsigned i = 0; i < 3; i++)
        adl_close(midiplay[i]);
    memset(midiplay, 0, sizeof(midiplay));
    _IsInitialized = false;
}

bool ADLPlayer::startup()
{
    if (midiplay[0] && midiplay[1] && midiplay[2]) return true;

    int chips_per_port = uChipCount / 3;
    int chips_round = (uChipCount % 3) != 0;
    int chips_min = uChipCount < 3;

    for (unsigned i = 0; i < 3; i++)
    {
        ADL_MIDIPlayer * midiplay = this->midiplay[i] = adl_init(_SampleRate);
        if (!midiplay) return false;

        adl_setBank(midiplay, uBankNumber);
        adl_setNumChips(midiplay, chips_per_port + chips_round * (i == 0) + chips_min * (i != 0));
        adl_setNumFourOpsChn(midiplay, u4OpCount);
        adl_setSoftPanEnabled(midiplay, bFullPanning);
        adl_setDeviceIdentifier(midiplay, i);
        adl_switchEmulator(midiplay, uEmuCore);
        adl_reset(midiplay);
    }

    _IsInitialized = true;

    setFilterMode(_FilterMode, _IsReverbChorusDisabled);

    return true;
}
