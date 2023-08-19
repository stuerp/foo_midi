
/** $VER: NukePlayer.cpp (2023.01.04) **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 5045 ALL_CPPCORECHECK_WARNINGS)

#include "NukePlayer.h"

#include <stdlib.h>
#include <string.h>

#include <Nuke/interface.h>

#include "Configuration.h"

static pfc::array_t<NukePreset> _NukePresets;

NukePlayer::NukePlayer()
{
    _Synth = 0;
}

NukePlayer::~NukePlayer()
{
    Shutdown();
}

void NukePlayer::SetSynth(unsigned int synthId)
{
    Shutdown();
    _SynthId = synthId;
}

void NukePlayer::SetBank(unsigned int bankId)
{
    Shutdown();
    _BankId = bankId;
}

void NukePlayer::SetExtp(unsigned int extp)
{
    Shutdown();
    _Extp = extp;
}

bool NukePlayer::Startup()
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

void NukePlayer::Shutdown()
{
    delete _Synth;
    _Synth = nullptr;
}

void NukePlayer::Render(audio_sample * sampleData, unsigned long sampleCount)
{
    const audio_sample ScaleFactor = 1.0f / 16384.0f;

    int16_t Data[512];

    while (sampleCount != 0)
    {
        unsigned long Todo = (sampleCount > 256) ? 256 : sampleCount;

        _Synth->midi_generate(Data, Todo);

        // Convert the format of the rendered output.
        for (size_t i = 0; i < Todo; ++i)
        {
            *sampleData++ = Data[i * 2 + 0] * ScaleFactor;
            *sampleData++ = Data[i * 2 + 1] * ScaleFactor;
        }

        sampleCount -= Todo;
    }
}

void NukePlayer::SendEvent(uint32_t data)
{
    _Synth->midi_write(data);
}

/// <summary>
/// Initializes the presets list.
/// </summary>
void NukePlayer::InitializePresets(std::function<void (const pfc::string8 name, unsigned int synth, unsigned int bank)> functor) noexcept
{
    nomidisynth * Synthsizers[] =
    {
        ::getsynth_doom(),
        ::getsynth_opl3w(),
        ::getsynth_apogee()
    };

    for (size_t j = 0; j < _countof(Synthsizers); j++)
    {
        nomidisynth * Synth = Synthsizers[j];

        const pfc::string8 Name = Synth->midi_synth_name();

        const size_t BankCount = Synth->midi_bank_count();

        if (BankCount > 1)
        {
            for (size_t i = 0; i < BankCount; ++i)
            {
                pfc::string8 Text;

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
void NukePlayer::EnumeratePresets(std::function<void (const pfc::string8 name, unsigned int synthId, unsigned int bankId)> functor) noexcept
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
void NukePlayer::GetPreset(pfc::string8 name, unsigned int & synth, unsigned int & bank)
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
void NukePlayer::GetPreset(size_t index, unsigned int & synth, unsigned int & bank)
{
    if (index >= _NukePresets.get_count())
        index = 0;

    synth = _NukePresets[index].SynthId;
    bank = _NukePresets[index].BankId;
}

/// <summary>
/// Gets the name of the Nuke preset for the specified synthesizer and bank.
/// </summary>
pfc::string8 NukePlayer::GetPresetName(unsigned int synth, unsigned int bank)
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
size_t NukePlayer::GetPresetIndex(unsigned int synth, unsigned int bank)
{
    for (size_t i = 0; i < _NukePresets.get_count(); ++i)
    {
        const NukePreset & Preset = _NukePresets[i];

        if (Preset.SynthId == synth && Preset.BankId == bank)
            return i;
    }

    return 0;
}

#pragma region("Nuke Preset Importer")
/// <summary>
/// Imports the presets of the Nuke player.
/// </summary>
class NukePresetsImporter
{
public:
    NukePresetsImporter()
    {
        NukePlayer::InitializePresets([] (pfc::string8 name, unsigned int synth, unsigned int bank)
        {
            NukePreset Preset = { name, synth, bank };

            _NukePresets.append_single(Preset);
        });
    }
};

NukePresetsImporter _NukePresetsImporter;
#pragma endregion
