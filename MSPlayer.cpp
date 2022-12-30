
/** $VER: MSPlayer.cpp (2022.12.30) **/

#include "MSPlayer.h"

#include <stdlib.h>
#include <string.h>

#pragma warning(disable: 5045)

MSPlayer::MSPlayer()
{
    _Synth = 0;
}

MSPlayer::~MSPlayer()
{
    shutdown();
}

void MSPlayer::SetSynth(unsigned int synthId)
{
    shutdown();
    _SynthId = synthId;
}

void MSPlayer::SetBank(unsigned int bankId)
{
    shutdown();
    _BankId = bankId;
}

void MSPlayer::SetExtp(unsigned int extp)
{
    shutdown();
    _Extp = extp;
}

void MSPlayer::send_event(uint32_t message)
{
    _Synth->midi_write(message);
}

void MSPlayer::send_sysex(const uint8_t *, size_t, size_t)
{
}

void MSPlayer::render(audio_sample * out, unsigned long count)
{
    audio_sample const scaler = 1.0f / 16384.0f;

    short buffer[512];

    while (count)
    {
        unsigned long todo = count > 256 ? 256 : count;

        _Synth->midi_generate(buffer, (unsigned int) todo);

        for (unsigned long i = 0; i < todo; ++i)
        {
            *out++ = buffer[i * 2 + 0] * scaler;
            *out++ = buffer[i * 2 + 1] * scaler;
        }

        count -= todo;
    }
}

void MSPlayer::shutdown()
{
    delete _Synth;
    _Synth = 0;
}

bool MSPlayer::startup()
{
    if (_Synth) return true;

    switch (_SynthId)
    {
        default:
        case 0:
            _Synth = getsynth_doom();
            break;

        case 1:
            _Synth = getsynth_opl3w();
            break;

        case 2:
            _Synth = getsynth_apogee();
            break;
    }

    if (!_Synth) return false;

    if (!_Synth->midi_init((unsigned int) _SampleRate, _BankId, _Extp))
        return false;

    return true;
}

void MSPlayer::enum_synthesizers(enum_callback callback)
{
    char buffer[512];
    const char * synth_name;

    nomidisynth * synth = getsynth_doom();

    synth_name = synth->midi_synth_name();

    unsigned int count = synth->midi_bank_count();

    unsigned int i;

    if (count > 1)
    {
        for (i = 0; i < count; ++i)
        {
            ::strcpy_s(buffer, synth_name);
            ::strcat_s(buffer, " ");
            ::strcat_s(buffer, synth->midi_bank_name(i));
            callback(0, i, buffer);
        }
    }
    else
    {
        callback(0, 0, synth_name);
    }

    delete synth;

    synth = getsynth_opl3w();

    synth_name = synth->midi_synth_name();

    count = synth->midi_bank_count();

    if (count > 1)
    {
        for (i = 0; i < count; ++i)
        {
            ::strcpy_s(buffer, synth_name);
            ::strcat_s(buffer, " ");
            ::strcat_s(buffer, synth->midi_bank_name(i));
            callback(1, i, buffer);
        }
    }
    else
    {
        callback(1, 0, synth_name);
    }

    delete synth;

    synth = getsynth_apogee();

    synth_name = synth->midi_synth_name();

    count = synth->midi_bank_count();

    if (count > 1)
    {
        for (i = 0; i < count; ++i)
        {
            ::strcpy_s(buffer, synth_name);
            ::strcat_s(buffer, " ");
            ::strcat_s(buffer, synth->midi_bank_name(i));
            callback(2, i, buffer);
        }
    }
    else
    {
        callback(2, 0, synth_name);
    }

    delete synth;
}
