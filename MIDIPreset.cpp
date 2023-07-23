
/** $VER: MIDIPreset.cpp (2023.07.22) **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 5045 ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPreset.h"

#include "Configuration.h"
#include "NukePlayer.h"

static void GetValue(const char * & separator, const char * & text);

MIDIPreset::MIDIPreset() noexcept
{
/* Test Code
    for (_PlayerType = 0; _PlayerType < PlayerTypeMax; _PlayerType++)
    {
        pfc::string8 Preset;

        Serialize(Preset);
        Deserialize(Preset);
    }
*/
    {
        _PlayerType = (uint32_t) CfgPlayerType;
        _VSTiFilePath = CfgVSTiFilePath;

        {
            try
            {
                auto Player = new VSTiPlayer;

                if (Player->LoadVST(_VSTiFilePath))
                {
                    _VSTiConfig = CfgVSTiConfig[Player->GetUniqueID()];

                    delete Player;
                }
            }
            catch (...)
            {
                if (_PlayerType == PlayerTypeVSTi)
                    _PlayerType = PlayerTypeEmuDeMIDI;
            }
        }

        _SoundFontFilePath = CfgSoundFontFilePath;
    }

#ifdef FLUIDSYNTHSUPPORT
    {
        effects = cfg_fluidsynth_effects;
        voices = (uint32_t)(int)cfg_fluidsynth_voices;
    }
#endif

    {
        _BASSMIDIEffectsEnabled = (bool) AdvCfgBASSMIDIEffectsEnabled;
        _BASSMIDIVoices = (uint32_t) AdvCfgBASSMIDIVoices;
    }

    {
        _MuntGMSet = (uint32_t) CfgMuntGMSet;
    }

#ifdef DXISUPPORT
    {
        dxi_plugin = cfg_dxi_plugin;
    }
#endif

    {
        _ADLBankNumber = (uint32_t) CfgADLBank;
        _ADLChipCount = (uint32_t) CfgADLChipCount;
        _ADLUsePanning = (bool) CfgADLPanning;

        if (AdvCfgADLCoreDOSBox)
            _ADLEmulatorCore = ADLMIDI_EMU_DOSBOX;
        else
        if (AdvCfgADLCoreNuked074)
            _ADLEmulatorCore = ADLMIDI_EMU_NUKED_174;
        else
        if (AdvCfgADLCoreNuked)
            _ADLEmulatorCore = ADLMIDI_EMU_NUKED;
        else
            _ADLEmulatorCore = ADLMIDI_EMU_DOSBOX;
    }

    {
        if (AdvCfgOPNCoreMAME)
            _OPNEmulatorCore = OPNMIDI_EMU_MAME;
        else
        if (AdvCfgOPNCoreNuked)
            _OPNEmulatorCore = OPNMIDI_EMU_NUKED;
        else
            _OPNEmulatorCore = OPNMIDI_EMU_GENS;
        if (AdvCfgOPNBankXG)
            _OPNBankNumber = 0;
        else
        if (AdvCfgOPNBankGS)
            _OPNBankNumber = 1;
        else
        if (AdvCfgOPNBankGEMS)
            _OPNBankNumber = 2;
        else
        if (AdvCfgOPNBankTomSoft)
            _OPNBankNumber = 3;
        else
            _OPNBankNumber = 4;
    }

    {
        _NukeSynth = (uint32_t) CfgNukeSynthesizer;
        _NukeBank = (uint32_t) CfgNukeBank;
        _NukeUsePanning = (bool) CfgNukePanning;
    }

    {
        _MIDIStandard = (uint32_t) CfgMIDIFlavor;
        _UseMIDIEffects = (bool) CfgUseMIDIEffects;
        _UseSuperMuntWithMT32 = (bool) CfgUseSuperMuntWithMT32;
        _UseSecretSauceWithXG = (bool) CfgUseVSTiWithXG;
    }
}

void MIDIPreset::Serialize(pfc::string8 & text)
{
    text.reset();
    text.prealloc(512);

    text += pfc::format_int(CurrentSchemaVersion);

    text += "|";
    text += pfc::format_int(_PlayerType);

    if (_PlayerType == PlayerTypeVSTi)
    {
        text += "|";
        text += _VSTiFilePath;

        text += "|";

        for (size_t i = 0; i < _VSTiConfig.size(); ++i)
            text += pfc::format_hex(_VSTiConfig[i], 2);
    }
    else
    if (_PlayerType == PlayerTypeFluidSynth || _PlayerType == PlayerTypeBASSMIDI)
    {
        text += "|";
        text += _SoundFontFilePath;

        text += "|";
        text += pfc::format_int(_BASSMIDIEffectsEnabled);

        text += "|";
        text += pfc::format_int(_BASSMIDIVoices);
    }
    else
    if (_PlayerType == PlayerTypeSuperMunt)
    {
        text += "|";
        text += _MuntGMSets[_MuntGMSet];
    }
#ifdef DXISUPPORT
    else
    if (plugin == PlayerTypeDirectX)
    {
        p_out += "|";
        p_out += pfc::format_hex(dxi_plugin.Data1, 8);
        p_out += "-";
        p_out += pfc::format_hex(dxi_plugin.Data2, 4);
        p_out += "-";
        p_out += pfc::format_hex(dxi_plugin.Data3, 4);
        p_out += "-";
        p_out += pfc::format_hex(dxi_plugin.Data4[0], 2);
        p_out += pfc::format_hex(dxi_plugin.Data4[1], 2);
        p_out += "-";
        p_out += pfc::format_hex(dxi_plugin.Data4[2], 2);
        p_out += pfc::format_hex(dxi_plugin.Data4[3], 2);
        p_out += pfc::format_hex(dxi_plugin.Data4[4], 2);
        p_out += pfc::format_hex(dxi_plugin.Data4[5], 2);
        p_out += pfc::format_hex(dxi_plugin.Data4[6], 2);
        p_out += pfc::format_hex(dxi_plugin.Data4[7], 2);
    }
#endif
    else
    if (_PlayerType == PlayerTypeADL)
    {
        const char * const * BankNames = adl_getBankNames();

        text += "|";
        text += BankNames[_ADLBankNumber];

        text += "|";
        text += pfc::format_int(_ADLChipCount);

        text += "|";
        text += pfc::format_int(_ADLUsePanning);

        text += "|";
        text += pfc::format_int(_ADLUseChorus);

        text += "|";
        text += pfc::format_int(_ADLEmulatorCore);
    }
    else
    if (_PlayerType == PlayerTypeOPN)
    {
        text += "|";
        text += pfc::format_int(_OPNBankNumber);

        text += "|";
        text += pfc::format_int(_ADLChipCount);

        text += "|";
        text += pfc::format_int(_ADLUsePanning);

        text += "|";
        text += pfc::format_int(_OPNEmulatorCore);
    }
    else
    if (_PlayerType == PlayerTypeNuke)
    {
        text += "|";
        text += NukePlayer::GetPresetName(_NukeSynth, _NukeBank);

        text += "|";
        text += pfc::format_int(_NukeUsePanning);
    }
    else
    if (_PlayerType == PlayerTypeSecretSauce)
    {
        // No player specific settings
    }

    text += "|";
    text += pfc::format_int(_MIDIStandard);

    text += "|";
    text += pfc::format_int(_UseMIDIEffects);

    text += "|";
    text += pfc::format_int(_UseSuperMuntWithMT32);

    text += "|";
    text += pfc::format_int(_UseSecretSauceWithXG);
}

void MIDIPreset::Deserialize(const char * text)
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

    uint32_t PlayerType = pfc::atodec<uint32_t>(text, (t_size)(Separator - text));

    pfc::string8 VSTiPath;
    std::vector<uint8_t> VSTiConfig;
    pfc::string8 SoundFontPath;

    bool BASSMIDIEffectsEnabled = false;
    uint32_t BASSMIDIVoices = 0;

#ifdef DXISUPPORT
    GUID DirectXGUID = { 0 };
#endif

    uint32_t ADLBankNumber = 0;
    uint32_t ADLChipCount = 0;
    bool ADLUsePanning = false;
    bool ADLUseChorus = false;
    uint32_t ADLEmulatorCore = 0;

    uint32_t OPNBankNumber = 0;
    uint32_t OPNEmulatorCore = 0;

    uint32_t MuntGMSet = 0;

    uint32_t NukeSynth = 0;
    uint32_t NukeBank = 0;
    bool NukeUsePanning = false;

    uint32_t MIDIStandard = (uint32_t) CfgMIDIFlavor;
    bool UseMIDIEffects = (bool) CfgUseMIDIEffects;
    bool UseSuperMuntWithMT32 = (bool) CfgUseSuperMuntWithMT32;
    bool UseSecretSauceWithXG = (bool) CfgUseVSTiWithXG;

    GetValue(Separator, text);

    if (PlayerType == PlayerTypeVSTi)
    {
        VSTiPath.set_string(text, (t_size)(Separator - text));

        GetValue(Separator, text);

        while (*text && (text < Separator))
        {
            VSTiConfig.push_back(pfc::atohex<unsigned char>(text, 2));

            text += 2;
        }

        if (CurrentSchemaVersion >= 11)
        {
            GetValue(Separator, text);
            MIDIStandard = pfc::atodec<uint32_t>(text, (t_size)(Separator - text));

            GetValue(Separator, text);
            UseMIDIEffects = pfc::atodec<bool>(text, (t_size)(Separator - text));
        }
    }
    else
    if (PlayerType == PlayerTypeFluidSynth || PlayerType == PlayerTypeBASSMIDI)
    {
        SoundFontPath.set_string(text, (t_size)(Separator - text));

        GetValue(Separator, text);

        if (Separator > text)
        {
            BASSMIDIEffectsEnabled = pfc::atodec<bool>(text, 1);

            if (SchemaVersion >= 9)
            {
                GetValue(Separator, text);

                if (Separator > text)
                {
                    BASSMIDIVoices = pfc::atodec<uint32_t>(text, (t_size)(Separator - text));

                    if (CurrentSchemaVersion >= 11)
                    {
                        GetValue(Separator, text);
                        MIDIStandard = pfc::atodec<uint32_t>(text, (t_size)(Separator - text));

                        GetValue(Separator, text);
                        UseMIDIEffects = pfc::atodec<bool>(text, (t_size)(Separator - text));
                    }
                }
                else
                {
                #ifdef BASSMIDISUPPORT
                    BASSMIDIVoices = (uint32_t) (int) AdvCfgBASSMIDIVoices;
                #elif defined(FLUIDSYNTHSUPPORT)
                    BASSMIDIVoices = (uint32_t) (int) cfg_fluidsynth_voices;
                #else
                    BASSMIDIVoices = 256;
                #endif
                }
            }
            else
            {
            #ifdef BASSMIDISUPPORT
                BASSMIDIVoices = (uint32_t) (int) AdvCfgBASSMIDIVoices;
            #elif defined(FLUIDSYNTHSUPPORT)
                BASSMIDIVoices = (uint32_t) (int) cfg_fluidsynth_voices;
            #else
                BASSMIDIVoices = 256;
            #endif
            }
        }
        else
        {
        #ifdef BASSMIDISUPPORT
            BASSMIDIEffectsEnabled = AdvCfgBASSMIDIEffectsEnabled;
            BASSMIDIVoices = (uint32_t) (int) AdvCfgBASSMIDIVoices;
        #elif defined(FLUIDSYNTHSUPPORT)
            BASSMIDIEffects = cfg_fluidsynth_effects;
            BASSMIDIVoices = (uint32_t) (int) cfg_fluidsynth_voices;
        #else
            BASSMIDIEffects = 1;
            BASSMIDIVoices = 256;
        #endif
        }
    }
    else
    if (PlayerType == PlayerTypeSuperMunt)
    {
        size_t i;

        for (i = 0; i < _MuntGMSetCount; ++i)
        {
            size_t len = ::strlen(_MuntGMSets[i]);

            if (len == (size_t)(Separator - text) && (::strncmp(text, _MuntGMSets[i], len) == 0))
            {
                MuntGMSet = (uint32_t) i;
                break;
            }
        }

        if (i == _MuntGMSetCount)
            return;
    }
    else
#ifdef DXISUPPORT
    else
    if (in_plugin == PlayerTypeDirectX)
    {
        if (bar_pos - p_in < 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12)
            return;

        DirectXGUID.Data1 = pfc::atohex<t_uint32>(p_in, 8);
        DirectXGUID.Data2 = pfc::atohex<t_uint16>(p_in + 8 + 1, 4);
        DirectXGUID.Data3 = pfc::atohex<t_uint16>(p_in + 8 + 1 + 4 + 1, 4);
        DirectXGUID.Data4[0] = pfc::atohex<t_uint16>(p_in + 8 + 1 + 4 + 1 + 4 + 1, 2);
        DirectXGUID.Data4[1] = pfc::atohex<t_uint16>(p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2, 2);
        DirectXGUID.Data4[2] = pfc::atohex<t_uint16>(p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2 + 2 + 1, 2);
        DirectXGUID.Data4[3] = pfc::atohex<t_uint16>(p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2 + 2 + 1 + 2, 2);
        DirectXGUID.Data4[4] = pfc::atohex<t_uint16>(p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2 + 2 + 1 + 2 + 2, 2);
        DirectXGUID.Data4[5] = pfc::atohex<t_uint16>(p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2 + 2 + 1 + 2 + 2 + 2, 2);
        DirectXGUID.Data4[6] = pfc::atohex<t_uint16>(p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2 + 2 + 1 + 2 + 2 + 2 + 2, 2);
        DirectXGUID.Data4[7] = pfc::atohex<t_uint16>(p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2 + 2 + 1 + 2 + 2 + 2 + 2 + 2, 2);
    }
#endif
    if (PlayerType == PlayerTypeADL)
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
        ADLChipCount = pfc::atodec<uint32_t>(text, (t_size)(Separator - text));

        GetValue(Separator, text);
        ADLUsePanning = pfc::atodec<bool>(text, (t_size)(Separator - text));

        ADLEmulatorCore = ADLMIDI_EMU_DOSBOX;

        if (CurrentSchemaVersion >= 3)
        {
            GetValue(Separator, text);
            ADLUseChorus = pfc::atodec<bool>(text, (t_size)(Separator - text));

            if (CurrentSchemaVersion >= 7)
            {
                GetValue(Separator, text);
                ADLEmulatorCore = pfc::atodec<uint32_t>(text, (t_size)(Separator - text));
            }
        }
    }
    else
    if (PlayerType == PlayerTypeOPN)
    {
        if (CurrentSchemaVersion >= 10)
        {
            OPNBankNumber = pfc::atodec<uint32_t>(text, (t_size)(Separator - text));

            GetValue(Separator, text);
            ADLChipCount = pfc::atodec<uint32_t>(text, (t_size)(Separator - text));

            GetValue(Separator, text);
            ADLUsePanning = pfc::atodec<bool>(text, (t_size)(Separator - text));

            GetValue(Separator, text);
            OPNEmulatorCore = pfc::atodec<uint32_t>(text, (t_size)(Separator - text));
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
    if (PlayerType == PlayerTypeNuke)
    {
        pfc::string8 Text;

        Text.set_string(text, (t_size)(Separator - text));

        NukePlayer::GetPreset(Text, NukeSynth, NukeBank);

        if (CurrentSchemaVersion >= 6)
        {
            GetValue(Separator, text);
            NukeUsePanning = pfc::atodec<bool>(text, (t_size)(Separator - text));
        }
        else
            NukeUsePanning = true;
    }
    else
    if (PlayerType == PlayerTypeSecretSauce)
    {
        if (CurrentSchemaVersion >= 11)
        {
            MIDIStandard = pfc::atodec<uint32_t>(text, (t_size)(Separator - text));

            if (MIDIStandard > MIDIPlayer::FilterXGSysEx)
                MIDIStandard = MIDIPlayer::FilterNone;

            GetValue(Separator, text);
            UseMIDIEffects = pfc::atodec<bool>(text, (t_size)(Separator - text));
        }
    }

    if (CurrentSchemaVersion >= 12)
    {
        GetValue(Separator, text);
        UseSuperMuntWithMT32 = pfc::atodec<bool>(text, (t_size)(Separator - text));

        GetValue(Separator, text);
        UseSecretSauceWithXG = pfc::atodec<bool>(text, (t_size)(Separator - text));
    }

    _PlayerType = PlayerType;

    _VSTiFilePath = VSTiPath;
    _VSTiConfig = VSTiConfig;

    _SoundFontFilePath = SoundFontPath;

    _BASSMIDIEffectsEnabled = BASSMIDIEffectsEnabled;
    _BASSMIDIVoices = BASSMIDIVoices;

#ifdef DXISUPPORT
    dxi_plugin = DirectXGUID;
#endif

    _ADLBankNumber = ADLBankNumber;
    _ADLChipCount = ADLChipCount;
    _ADLUsePanning = ADLUsePanning;
    _ADLUseChorus = ADLUseChorus;
    _ADLEmulatorCore = ADLEmulatorCore;

    _OPNBankNumber = OPNBankNumber;
    _OPNEmulatorCore = OPNEmulatorCore;

    _MuntGMSet = MuntGMSet;

    _NukeSynth = NukeSynth;
    _NukeBank = NukeBank;
    _NukeUsePanning = NukeUsePanning;

    _MIDIStandard = MIDIStandard;
    _UseMIDIEffects = UseMIDIEffects;
}

static void GetValue(const char * & separator, const char * & text)
{
    text = separator + (*separator == '|');

    separator = ::strchr(text, '|');

    if (separator == nullptr)
        separator = text + ::strlen(text);
}
