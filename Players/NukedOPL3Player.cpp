
/** $VER: NukedOPL3Player.cpp (2025.07.09) **/

#include "pch.h"

#include "NukedOPL3Player.h"

static std::vector<NukedPreset> _NukedPresets;

NukedOPL3Player::NukedOPL3Player() noexcept
{
    _Synth = nullptr;
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

void NukedOPL3Player::SetSoftPanning(uint32_t extp)
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
void NukedOPL3Player::InitializePresets(std::function<void (const std::string & name, uint32_t synth, uint32_t bank)> functor) noexcept
{
    nomidisynth * Synthesizers[] =
    {
        ::getsynth_doom(),
        ::getsynth_opl3w(),
        ::getsynth_apogee()
    };

    for (uint32_t j = 0; j < _countof(Synthesizers); ++j)
    {
        auto * Synth = Synthesizers[j];

        const std::string Name = Synth->midi_synth_name();

        const uint32_t BankCount = Synth->midi_bank_count();

        if (BankCount > 1)
        {
            for (uint32_t i = 0; i < BankCount; ++i)
            {
                std::string FullName = Name + " " + Synth->midi_bank_name(i);

                functor(FullName, j, i);
            }
        }
        else
            functor(Name, j, 0);

        delete Synth;
    }
}

/// <summary>
/// Enumerates the presets of this instance.
/// </summary>
void NukedOPL3Player::EnumeratePresets(std::function<void (const std::string & name, uint32_t synthId, uint32_t bankId)> functor) noexcept
{
    for (const auto & Preset : _NukedPresets)
    {
        functor(Preset.Name, Preset.SynthId, Preset.BankId);
    }
}

/// <summary>
/// Gets the synthesizer and bank number of the preset with the specified name.
/// </summary>
void NukedOPL3Player::GetPreset(const std::string & name, uint32_t & synth, uint32_t & bank) noexcept
{
    for (const auto & Preset : _NukedPresets)
    {
        if (::_stricmp(Preset.Name.c_str(), name.c_str()) == 0)
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
/// Gets the synthesizer and bank number of the preset at the specified index.
/// </summary>
void NukedOPL3Player::GetPreset(size_t index, uint32_t & synth, uint32_t & bank) noexcept
{
    if (index >= _NukedPresets.size())
        index = 0;

    synth = _NukedPresets[index].SynthId;
    bank  = _NukedPresets[index].BankId;
}

/// <summary>
/// Gets the name of the preset with the specified synthesizer and bank.
/// </summary>
std::string NukedOPL3Player::GetPresetName(uint32_t synth, uint32_t bank) noexcept
{
    for (const auto & Preset : _NukedPresets)
    {
        if ((Preset.SynthId == synth) && (Preset.BankId == bank))
            return Preset.Name;
    }

    return "Unknown Preset";
}

/// <summary>
/// Gets the index of the Nuke preset for the specified synthesizer and bank.
/// </summary>
size_t NukedOPL3Player::GetPresetIndex(uint32_t synth, uint32_t bank) noexcept
{
    size_t i = 0;

    for (const auto & Preset : _NukedPresets)
    {
        if (Preset.SynthId == synth && Preset.BankId == bank)
            return i;

        ++i;
    }

    return 0;
}

#pragma region Nuke Preset Importer

/// <summary>
/// Imports the presets of the NukedOPL3 player.
/// </summary>
class NukedPresetsImporter
{
public:
    NukedPresetsImporter()
    {
        NukedOPL3Player::InitializePresets([] (std::string name, uint32_t synth, uint32_t bank)
        {
            NukedPreset Preset = { name, synth, bank };

            _NukedPresets.push_back(Preset);
        });
    }
};

NukedPresetsImporter _NukedPresetsImporter;

#pragma endregion
