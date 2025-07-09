
/** $VER: NukedOPL3Player.cpp (2025.07.09) **/

#include "pch.h"

#include "NukedOPL3Player.h"

#include <stdlib.h>
#include <string.h>

#include <interface.h>

#include "Configuration.h"

static pfc::array_t<NukePreset> _NukePresets;

NukedOPL3Player::NukedOPL3Player()
{
    _Synth = 0;
}

NukedOPL3Player::~NukedOPL3Player()
{
    Shutdown();
}

void NukedOPL3Player::SetSynth(uint32_t synthId)
{
    Shutdown();
    _SynthId = synthId;
}

void NukedOPL3Player::SetBankNumber(uint32_t bankNumber)
{
    Shutdown();
    _BankId = bankNumber;
}

void NukedOPL3Player::SetExtp(uint32_t extp)
{
    Shutdown();
    _Extp = extp;
}

bool NukedOPL3Player::Startup()
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

void NukedOPL3Player::Shutdown()
{
    delete _Synth;
    _Synth = nullptr;
}

void NukedOPL3Player::Render(audio_sample * dstFrames, uint32_t dstCount)
{
    const uint32_t MaxFrames = 256;
    const uint32_t MaxChannels = 2;

    int16_t srcFrames[MaxFrames * MaxChannels];

    const audio_sample ScaleFactor = 1.0 / 16384.0;

    while (dstCount != 0)
    {
        const uint32_t srcCount = std::min(dstCount, MaxFrames);

        _Synth->midi_generate(srcFrames, srcCount);

        // Convert the format of the rendered output.
        for (size_t i = 0, j = 0; i < srcCount; ++i)
        {
            *dstFrames++ = srcFrames[j++] * ScaleFactor;
            *dstFrames++ = srcFrames[j++] * ScaleFactor;
        }

        dstCount -= srcCount;
    }
}

void NukedOPL3Player::SendEvent(uint32_t data)
{
    _Synth->midi_write(data);
}

/// <summary>
/// Initializes the presets list.
/// </summary>
void NukedOPL3Player::InitializePresets(std::function<void (const pfc::string name, unsigned int synth, unsigned int bank)> functor) noexcept
{
    nomidisynth * Synthesizers[] =
    {
        ::getsynth_doom(),
        ::getsynth_opl3w(),
        ::getsynth_apogee()
    };

    for (size_t j = 0; j < _countof(Synthesizers); j++)
    {
        nomidisynth * Synth = Synthesizers[j];

        const pfc::string Name = Synth->midi_synth_name();

        const size_t BankCount = Synth->midi_bank_count();

        if (BankCount > 1)
        {
            for (size_t i = 0; i < BankCount; ++i)
            {
                pfc::string Text;

                Text.prealloc(512);

                Text << Name << " " << Synth->midi_bank_name((unsigned int)i);

                functor(Text, (unsigned int)j, (unsigned int)i);
            }
        }
        else
            functor(Name, (unsigned int)j, 0);

        delete Synth;
    }
}

/// <summary>
/// Enumerates the presets of this instance.
/// </summary>
void NukedOPL3Player::EnumeratePresets(std::function<void (const pfc::string name, unsigned int synthId, unsigned int bankId)> functor) noexcept
{
    for (size_t i = 0; i < _NukePresets.get_count(); ++i)
    {
        const NukePreset & Preset = _NukePresets[i];

        functor(Preset.Name, Preset.SynthId, Preset.BankId);
    }
}

/// <summary>
/// Gets the Nuke preset with the specified name.
/// </summary>
void NukedOPL3Player::GetPreset(pfc::string name, unsigned int & synth, unsigned int & bank)
{
    for (size_t i = 0; i < _NukePresets.get_count(); ++i)
    {
        const NukePreset & Preset = _NukePresets[i];

        if (pfc::stricmp_ascii(Preset.Name, name) == 0)
        {
            synth = Preset.SynthId;
            bank = Preset.BankId;

            return;
        }
    }

    synth = DefaultNukeSynth;
    bank = DefaultNukeBank;
}

/// <summary>
/// Gets the Nuke preset at the specified index.
/// </summary>
void NukedOPL3Player::GetPreset(size_t index, unsigned int & synth, unsigned int & bank)
{
    if (index >= _NukePresets.get_count())
        index = 0;

    synth = _NukePresets[index].SynthId;
    bank = _NukePresets[index].BankId;
}

/// <summary>
/// Gets the name of the Nuke preset for the specified synthesizer and bank.
/// </summary>
pfc::string NukedOPL3Player::GetPresetName(unsigned int synth, unsigned int bank)
{
    for (size_t i = 0; i < _NukePresets.get_count(); ++i)
    {
        const NukePreset & Preset = _NukePresets[i];

        if ((Preset.SynthId == synth) && (Preset.BankId == bank))
            return Preset.Name;
    }

    return "Unknown Preset";
}

/// <summary>
/// Gets the index of the Nuke preset for the specified synthesizer and bank.
/// </summary>
size_t NukedOPL3Player::GetPresetIndex(unsigned int synth, unsigned int bank)
{
    for (size_t i = 0; i < _NukePresets.get_count(); ++i)
    {
        const NukePreset & Preset = _NukePresets[i];

        if (Preset.SynthId == synth && Preset.BankId == bank)
            return i;
    }

    return 0;
}

#pragma region Nuke Preset Importer

/// <summary>
/// Imports the presets of the Nuke player.
/// </summary>
class NukePresetsImporter
{
public:
    NukePresetsImporter()
    {
        NukedOPL3Player::InitializePresets([] (pfc::string name, unsigned int synth, unsigned int bank)
        {
            NukePreset Preset = { name, synth, bank };

            _NukePresets.append_single(Preset);
        });
    }
};

NukePresetsImporter _NukePresetsImporter;

#pragma endregion
