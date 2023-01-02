
/** $VER: NukePlayer.cpp (2023.01.02) Nuke **/

#pragma warning(disable: 5045) // Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified

#include "NukePlayer.h"

#include <stdlib.h>
#include <string.h>

#include <Nuke/interface.h>

NukePlayer::NukePlayer()
{
    _Synth = 0;
}

NukePlayer::~NukePlayer()
{
    shutdown();
}

void NukePlayer::SetSynth(unsigned int synthId)
{
    shutdown();
    _SynthId = synthId;
}

void NukePlayer::SetBank(unsigned int bankId)
{
    shutdown();
    _BankId = bankId;
}

void NukePlayer::SetExtp(unsigned int extp)
{
    shutdown();
    _Extp = extp;
}

bool NukePlayer::startup()
{
    if (_Synth)
        return true;

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

    if (_Synth == nullptr)
        return false;

    if (!_Synth->midi_init(_SampleRate, _BankId, _Extp))
        return false;

    return true;
}

void NukePlayer::shutdown()
{
    delete _Synth;
    _Synth = nullptr;
}

void NukePlayer::render(audio_sample * samples, unsigned long count)
{
    audio_sample const ScaleFactor = 1.0f / 16384.0f;

    signed short Data[512];

    while (count)
    {
        unsigned int Todo = (count > 256) ? 256 : (unsigned int)count;

        _Synth->midi_generate(Data, Todo);

        for (unsigned int i = 0; i < Todo; ++i)
        {
            *samples++ = Data[i * 2 + 0] * ScaleFactor;
            *samples++ = Data[i * 2 + 1] * ScaleFactor;
        }

        count -= Todo;
    }
}

void NukePlayer::send_event(uint32_t message)
{
    _Synth->midi_write(message);
}

void NukePlayer::send_sysex(const uint8_t *, size_t, size_t)
{
}

/// <summary>
/// Enumerates all the supported synthesizers.
/// </summary>
void NukePlayer::EnumerateSynthesizers(SynthesizerEnumerator callback)
{
    nomidisynth * Synthsizers[] =
    {
        getsynth_doom(),
        getsynth_opl3w(),
        getsynth_apogee()
    };

    for (size_t j = 0; j < _countof(Synthsizers); j++)
    {
        nomidisynth * Synth = Synthsizers[j];

        const char * Name = Synth->midi_synth_name();

        const size_t BankCount = Synth->midi_bank_count();

        if (BankCount > 1)
        {
            for (size_t i = 0; i < BankCount; ++i)
            {
                char Text[512];

                ::strcpy_s(Text, Name);
                ::strcat_s(Text, " ");
                ::strcat_s(Text, Synth->midi_bank_name((unsigned int)i));

                callback((unsigned int)j, (unsigned int)i, Text);
            }
        }
        else
            callback((unsigned int)j, 0, Name);

        delete Synth;
    }
}
