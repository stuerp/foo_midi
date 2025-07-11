
/** $VER: Preset.cpp (2025.07.08) **/

#include "pch.h"

#include "Preset.h"

#include "PreferencesFM.h"
#include "Configuration.h"

#include "NukedOPL3Player.h"

static void GetValue(const char * & separator, const char * & text);

preset_t::preset_t() noexcept
{
/* Test Code
    for (_PlayerType = 0; _PlayerType < PlayerType::Max; _PlayerType++)
    {
        pfc::string Preset;

        Serialize(Preset);
        Deserialize(Preset);
    }
*/
    {
        _PlayerType = (PlayerTypes) (uint32_t) CfgPlayerType;

        _PlugInFilePath = CfgPlugInFilePath;
        _CLAPPlugInIndex = (uint32_t) CfgCLAPIndex;

        // Get the configuration of the VSTi.
        if (_PlayerType == PlayerTypes::VSTi)
        {
            try
            {
                auto Player = new VSTi::Player;

                if (Player->LoadVST((const char8_t *) _PlugInFilePath.c_str()))
                {
                    _VSTiConfig = CfgVSTiConfig[Player->Id];

                    delete Player;
                }
            }
            catch (...)
            {
                _PlayerType = PlayerTypes::Default;
            }
        }
        else
        // Get the configuration of the CLAP plug-in.
        if (_PlayerType == PlayerTypes::CLAP)
        {
        }

        _SoundFontFilePath = CfgSoundFontFilePath;
    }

    if (_PlayerType == PlayerTypes::FluidSynth)
    {
        _EffectsEnabled = CfgFluidSynthProcessEffects;
        _VoiceCount = (uint32_t) CfgFluidSynthMaxVoices;
    }
    else
    if (_PlayerType == PlayerTypes::BASSMIDI)
    {
        _EffectsEnabled = CfgBASSMIDIProcessEffects;
        _VoiceCount = (uint32_t) CfgBASSMIDIMaxVoices;
    }
    else
        _VoiceCount = 256;

    {
        _MT32EmuGMSet = (uint32_t) CfgMT32EmuGMSet;
    }

    // LibADLMIDI
    {
        _ADLBankNumber = (uint32_t) CfgADLBank;
        _ADLEmulatorCore = (uint32_t) CfgADLEmulator;
        _ADLChipCount = (uint32_t) CfgADLChipCount;
        _ADLSoftPanning = (bool) CfgADLSoftPanning;
        _ADLBankFilePath = CfgADLBankFilePath.get();
    }

    // LibOPNMIDI
    {
        _OPNBankNumber = (uint32_t) CfgOPNBank;
        _OPNEmulatorCore = (uint32_t) CfgOPNEmulator;
        _OPNChipCount = (uint32_t) CfgOPNChipCount;
        _OPNSoftPanning = (bool) CfgOPNSoftPanning;
        _OPNBankFilePath = CfgOPNBankFilePath.get();
    }

    // Nuke
    {
        _NukeSynth = (uint32_t) CfgNukeSynthesizer;
        _NukeBank = (uint32_t) CfgNukeBank;
        _NukeUsePanning = (bool) CfgNukePanning;
    }

    {
        _MIDIFlavor = (MIDIFlavors) (uint32_t) CfgMIDIFlavor;
        _UseMIDIEffects = (bool) CfgUseMIDIEffects;
        _UseMT32EmuWithMT32 = (bool) CfgUseMT32EmuWithMT32;
        _UseVSTiWithXG = (bool) CfgUseVSTiWithXG;
        _DetectExtraDrum = (bool) CfgDetectExtraDrum;
    }
}

void preset_t::Serialize(pfc::string & text)
{
    text.reset();
    text.prealloc(512);

    text += pfc::format_int(CurrentSchemaVersion);

    text += "|";
    text += pfc::format_int((t_int64) _PlayerType);

    if (_PlayerType == PlayerTypes::VSTi)
    {
        text += "|";
        text += _PlugInFilePath;

        text += "|";

        for (auto Byte : _VSTiConfig)
            text += pfc::format_hex(Byte, 2);
    }
    else
    if (_PlayerType == PlayerTypes::FluidSynth || _PlayerType == PlayerTypes::BASSMIDI)
    {
        text += "|";
        text += _SoundFontFilePath;

        text += "|";
        text += pfc::format_int(_EffectsEnabled);

        text += "|";
        text += pfc::format_int(_VoiceCount);
    }
    else
    if (_PlayerType == PlayerTypes::MT32Emu)
    {
        text += "|";
        text += _MT32EmuSets[_MT32EmuGMSet].c_str();
    }
    else
    if (_PlayerType == PlayerTypes::ADL)
    {
        const char * const * BankNames = adl_getBankNames();

        text += "|";
        text += BankNames[_ADLBankNumber];

        text += "|";
        text += pfc::format_int(_ADLChipCount);

        text += "|";
        text += pfc::format_int(_ADLSoftPanning);

        text += "|";
        text += pfc::format_int(_ADLUseChorus);

        text += "|";
        text += pfc::format_int(_ADLEmulatorCore);
    }
    else
    if (_PlayerType == PlayerTypes::OPN)
    {
        text += "|";
        text += pfc::format_int(_OPNBankNumber);

        text += "|";
        text += pfc::format_int(_ADLChipCount);

        text += "|";
        text += pfc::format_int(_ADLSoftPanning);

        text += "|";
        text += pfc::format_int(_OPNEmulatorCore);
    }
    else
    if (_PlayerType == PlayerTypes::NukedOPL3)
    {
        text += "|";
        text += NukedOPL3Player::GetPresetName(_NukeSynth, _NukeBank).c_str();

        text += "|";
        text += pfc::format_int(_NukeUsePanning);
    }
    else
    if (_PlayerType == PlayerTypes::SecretSauce)
    {
        // No player specific settings
    }

    text += "|";
    text += pfc::format_int((uint32_t) _MIDIFlavor);

    text += "|";
    text += pfc::format_int(_UseMIDIEffects);

    text += "|";
    text += pfc::format_int(_UseMT32EmuWithMT32);

    text += "|";
    text += pfc::format_int(_UseVSTiWithXG);
}

void preset_t::Deserialize(const char * text)
{
    if (text == nullptr)
        return;

    // Get the schema version.
    const char * Separator = text;

    GetValue(Separator, text);

    uint32_t SchemaVersion = pfc::atodec<uint32_t>(text, (t_size)(Separator - text));

    if (SchemaVersion > CurrentSchemaVersion)
        return;

    // Get the player type.
    GetValue(Separator, text);

    auto PlayerType = (PlayerTypes) (uint8_t) pfc::atodec<uint32_t>(text, (t_size) (Separator - text));

    pfc::string VSTiPath;
    std::vector<uint8_t> VSTiConfig;
    pfc::string SoundFontPath;

    bool ProcessEffects = false;
    uint32_t MaxVoices = 256;

    uint32_t ADLBankNumber = 0;
    uint32_t ADLChipCount = 0;
    bool ADLUsePanning = false;
    bool ADLUseChorus = false;
    uint32_t ADLEmulatorCore = 0;

    uint32_t OPNBankNumber = 0;
    uint32_t OPNEmulatorCore = 0;

    uint32_t MT32EmuGMSet = 0;

    uint32_t NukeSynth = 0;
    uint32_t NukeBank = 0;
    bool NukeUsePanning = false;

    MIDIFlavors Flavor = MIDIFlavors::Default;

    bool UseMIDIEffects = false;
    bool UseMT32EmuWithMT32 = false;
    bool UseSecretSauceWithXG = false;

    GetValue(Separator, text);

    if (PlayerType == PlayerTypes::VSTi)
    {
        VSTiPath.set_string(text, (t_size) (Separator - text));

        GetValue(Separator, text);

        while (*text && (text < Separator))
        {
            VSTiConfig.push_back(pfc::atohex<unsigned char>(text, 2));

            text += 2;
        }

        if (CurrentSchemaVersion >= 11)
        {
            GetValue(Separator, text);
            Flavor = (MIDIFlavors) pfc::atodec<uint32_t>(text, (t_size) (Separator - text));

            GetValue(Separator, text);
            UseMIDIEffects = pfc::atodec<bool>(text, (t_size) (Separator - text));
        }
    }
    else
    if (PlayerType == PlayerTypes::FluidSynth || PlayerType == PlayerTypes::BASSMIDI)
    {
        SoundFontPath.set_string(text, (t_size)(Separator - text));

        {
            if (PlayerType == PlayerTypes::FluidSynth)
            {
                ProcessEffects = (bool) CfgFluidSynthProcessEffects;
                MaxVoices = (uint32_t) CfgFluidSynthMaxVoices;
            }
            else
            if (PlayerType == PlayerTypes::BASSMIDI)
            {
                ProcessEffects = (bool) CfgBASSMIDIProcessEffects;
                MaxVoices = (uint32_t) CfgBASSMIDIMaxVoices;
            }
        }

        GetValue(Separator, text);

        if (Separator > text)
        {
            ProcessEffects = pfc::atodec<bool>(text, 1);

            if (SchemaVersion >= 9)
            {
                GetValue(Separator, text);

                if (Separator > text)
                {
                    MaxVoices = pfc::atodec<uint32_t>(text, (t_size) (Separator - text));

                    if (CurrentSchemaVersion >= 11)
                    {
                        GetValue(Separator, text);
                        Flavor = (MIDIFlavors) pfc::atodec<uint32_t>(text, (t_size) (Separator - text));

                        GetValue(Separator, text);
                        UseMIDIEffects = pfc::atodec<bool>(text, (t_size) (Separator - text));
                    }
                }
            }
        }
    }
    else
    if (PlayerType == PlayerTypes::MT32Emu)
    {
        size_t i;

        for (i = 0; i < _MT32EmuSets.size(); ++i)
        {
            size_t len = ::strlen(_MT32EmuSets[i].c_str());

            if (len == (size_t) (Separator - text) && (::strncmp(text, _MT32EmuSets[i].c_str(), len) == 0))
            {
                MT32EmuGMSet = (uint32_t) i;
                break;
            }
        }

        if (i == _MT32EmuSets.size())
            return;
    }
    else
    if (PlayerType == PlayerTypes::ADL)
    {
        {
            const char * const * BankNames = adl_getBankNames();
            uint32_t BankCount = (uint32_t) adl_getBanksCount();

            uint32_t i;

            for (i = 0; i < BankCount; ++i)
            {
                size_t len = ::strlen(BankNames[i]);

                if (len == (size_t)(Separator - text) && (::strncmp(text, BankNames[i], len) == 0))
                {
                    ADLBankNumber = i;
                    break;
                }
            }

            if (i == BankCount)
                return;
        }

        GetValue(Separator, text);
        ADLChipCount = pfc::atodec<uint32_t>(text, (t_size) (Separator - text));

        GetValue(Separator, text);
        ADLUsePanning = pfc::atodec<bool>(text, (t_size) (Separator - text));

        ADLEmulatorCore = ADLMIDI_EMU_DOSBOX;

        if (CurrentSchemaVersion >= 3)
        {
            GetValue(Separator, text);
            ADLUseChorus = pfc::atodec<bool>(text, (t_size) (Separator - text));

            if (CurrentSchemaVersion >= 7)
            {
                GetValue(Separator, text);
                ADLEmulatorCore = pfc::atodec<uint32_t>(text, (t_size) (Separator - text));
            }
        }
    }
    else
    if (PlayerType == PlayerTypes::OPN)
    {
        if (CurrentSchemaVersion >= 10)
        {
            OPNBankNumber = pfc::atodec<uint32_t>(text, (t_size) (Separator - text));

            GetValue(Separator, text);
            ADLChipCount = pfc::atodec<uint32_t>(text, (t_size) (Separator - text));

            GetValue(Separator, text);
            ADLUsePanning = pfc::atodec<bool>(text, (t_size) (Separator - text));

            GetValue(Separator, text);
            OPNEmulatorCore = pfc::atodec<uint32_t>(text, (t_size) (Separator - text));
        }
        else
        {
            OPNBankNumber = 4;
            ADLChipCount = 10;
            ADLUsePanning = true;
            OPNEmulatorCore = 0;
        }
    }
    else
    if (PlayerType == PlayerTypes::NukedOPL3)
    {
        std::string Text(text, (t_size) (Separator - text));

        NukedOPL3Player::GetPreset(Text, NukeSynth, NukeBank);

        if (CurrentSchemaVersion >= 6)
        {
            GetValue(Separator, text);
            NukeUsePanning = pfc::atodec<bool>(text, (t_size) (Separator - text));
        }
        else
            NukeUsePanning = true;
    }
    else
    if (PlayerType == PlayerTypes::SecretSauce)
    {
        if (CurrentSchemaVersion >= 11)
        {
            Flavor = (MIDIFlavors) pfc::atodec<uint32_t>(text, (t_size) (Separator - text));

            if (Flavor > MIDIFlavors::XG)
                Flavor = MIDIFlavors::Default;

            GetValue(Separator, text);
            UseMIDIEffects = pfc::atodec<bool>(text, (t_size) (Separator - text));
        }
    }

    if (CurrentSchemaVersion >= 12)
    {
        GetValue(Separator, text);
        UseMT32EmuWithMT32 = pfc::atodec<bool>(text, (t_size) (Separator - text));

        GetValue(Separator, text);
        UseSecretSauceWithXG = pfc::atodec<bool>(text, (t_size) (Separator - text));
    }

    _PlayerType = PlayerType;

    _PlugInFilePath = VSTiPath;
    _VSTiConfig = VSTiConfig;

    _SoundFontFilePath = SoundFontPath;

    _EffectsEnabled = ProcessEffects;
    _VoiceCount = MaxVoices;

    _ADLBankNumber = ADLBankNumber;
    _ADLEmulatorCore = ADLEmulatorCore;
    _ADLChipCount = ADLChipCount;
    _ADLSoftPanning = ADLUsePanning;
    _ADLUseChorus = ADLUseChorus;

    _OPNBankNumber = OPNBankNumber;
    _OPNEmulatorCore = OPNEmulatorCore;

    _MT32EmuGMSet = MT32EmuGMSet;

    _NukeSynth = NukeSynth;
    _NukeBank = NukeBank;
    _NukeUsePanning = NukeUsePanning;

    _MIDIFlavor = Flavor;
    _UseMIDIEffects = UseMIDIEffects;
}

static void GetValue(const char * & separator, const char * & text)
{
    text = separator + (*separator == '|');

    separator = ::strchr(text, '|');

    if (separator == nullptr)
        separator = text + ::strlen(text);
}
