
/** $VER: MIDIPreset.cpp (2023.01.02) **/

#pragma warning(disable: 5045)

#include "MIDIPreset.h"

#include "Configuration.h"
#include "NukePlayer.h"

pfc::array_t<NukePreset> _NukePresets;

MIDIPreset::MIDIPreset() noexcept
{
    {
        _PluginId = (unsigned int)CfgPlugInId;
        _VSTPathName = CfgVSTiPath;

        {
            VSTiPlayer * Player = nullptr;

            try
            {
                Player = new VSTiPlayer;

                if (Player->LoadVST(_VSTPathName))
                {
                    _VSTConfig = CfgVSTiConfig[(unsigned int)(Player->getUniqueID())];
                }
            }
            catch (...)
            {
                if (_PluginId == 1)
                    _PluginId = 0;
            }

            delete Player;
        }

        _SoundFontPathName = CfgSoundFontPath;
    }

#ifdef FLUIDSYNTHSUPPORT
    {
        effects = cfg_fluidsynth_effects;
        voices = (unsigned int)(int)cfg_fluidsynth_voices;
    }
#endif

#ifdef BASSMIDISUPPORT
    {
        _BASSMIDIEffects = CfgBASSMIDIEffects;
        _BASSMIDIVoices = (unsigned int)(int)CfgBASSMIDIVoices;
    }
#endif

    {
        _MuntGMSet = (unsigned int)CfgMUNTGMSet;
    }

#ifdef DXISUPPORT
    {
        dxi_plugin = cfg_dxi_plugin;
    }
#endif

    {
        _ADLBankNumber = (unsigned int)CfgADLBank;
        _ADLChipCount = (unsigned int)CfgADLChipCount;
        _ADLUsePanning = !!CfgADLPanning;

        if (CfgADLCoreDOSBox)
            _ADLEmulatorCore = ADLMIDI_EMU_DOSBOX;
        else
        if (cfg_adl_core_nuked_174)
            _ADLEmulatorCore = ADLMIDI_EMU_NUKED_174;
        else
        if (cfg_adl_core_nuked)
            _ADLEmulatorCore = ADLMIDI_EMU_NUKED;
        else
            _ADLEmulatorCore = ADLMIDI_EMU_DOSBOX;
    }

    {
        if (cfg_opn_core_mame)
            _OPNEmulatorCore = OPNMIDI_EMU_MAME;
        else
        if (cfg_opn_core_nuked)
            _OPNEmulatorCore = OPNMIDI_EMU_NUKED;
        else
            _OPNEmulatorCore = OPNMIDI_EMU_GENS;
        if (cfg_opn_bank_xg)
            _OPNBankNumber = 0;
        else
        if (cfg_opn_bank_gs)
            _OPNBankNumber = 1;
        else
        if (cfg_opn_bank_gems)
            _OPNBankNumber = 2;
        else
        if (cfg_opn_bank_tomsoft)
            _OPNBankNumber = 3;
        else
            _OPNBankNumber = 4;
    }

    {
        _NukeSynth = (unsigned int)CfgMSSynthesizer;
        _NukeBank = (unsigned int)CfgMSBank;
        _NukePanning = (bool)CfgMSPanning;
    }

    {
        _MIDIStandard = (unsigned int)CfgMIDIFlavor;
        _UseMIDIEffects = (bool)CfgAllowMIDIEffects;
    }
}

/// <summary>
/// Gets the name of the MSPreset for the specified synthesizer and bank.
/// </summary>
const char * MIDIPreset::GetMSPresetName(unsigned int synth, unsigned int bank)
{
    for (size_t i = 0; i < _NukePresets.get_count(); ++i)
    {
        const NukePreset & Preset = _NukePresets[i];

        if ((Preset.synth == synth) && (Preset.bank == bank))
            return Preset.name;
    }

    return "Unknown Preset";
}

void MIDIPreset::serialize(pfc::string8 & p_out)
    {
        const char * const * banknames = adl_getBankNames();

        p_out.reset();

        p_out += pfc::format_int(CurrentSchemaVersion);
        p_out += "|";

        p_out += pfc::format_int(_PluginId);

        if (_PluginId == 1)
        {
            p_out += "|";

            p_out += _VSTPathName;
            p_out += "|";

            for (unsigned i = 0; i < _VSTConfig.size(); ++i)
            {
                p_out += pfc::format_hex(_VSTConfig[i], 2);
            }

            p_out += "|";
            p_out += pfc::format_int(_MIDIStandard);
            p_out += "|";
            p_out += pfc::format_int(_UseMIDIEffects);
        }
        else
        if (_PluginId == 2 || _PluginId == 4)
        {
            p_out += "|";

            p_out += _SoundFontPathName;

            p_out += "|";

            p_out += pfc::format_int(_BASSMIDIEffects);

            p_out += "|";

            p_out += pfc::format_int(_BASSMIDIVoices);

            p_out += "|";

            p_out += pfc::format_int(_MIDIStandard);

            p_out += "|";

            p_out += pfc::format_int(_UseMIDIEffects);
        }
    #ifdef DXISUPPORT
        else
        if (plugin == 5)
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
        if (_PluginId == 6)
        {
            p_out += "|";

            p_out += banknames[_ADLBankNumber];
            p_out += "|";

            p_out += pfc::format_int(_ADLChipCount);
            p_out += "|";

            p_out += pfc::format_int(_ADLUsePanning);
            p_out += "|";

            p_out += pfc::format_int(_ADLChorus);
            p_out += "|";

            p_out += pfc::format_int(_ADLEmulatorCore);
        }
        else
        if (_PluginId == 3)
        {
            p_out += "|";

            p_out += _MUNTGMSets[_MuntGMSet];
        }
        else
        if (_PluginId == 9)
        {
            p_out += "|";
            p_out += GetMSPresetName(_NukeSynth, _NukeBank);
            p_out += "|";
            p_out += pfc::format_int(_NukePanning);
        }
        else
        if (_PluginId == 10)
        {
            p_out += "|";
            p_out += pfc::format_int(_MIDIStandard);
            p_out += "|";
            p_out += pfc::format_int(_UseMIDIEffects);
        }
        else
        if (_PluginId == 7)
        {
            p_out += "|";
            p_out += pfc::format_int(_OPNBankNumber);
            p_out += "|";
            p_out += pfc::format_int(_ADLChipCount);
            p_out += "|";
            p_out += pfc::format_int(_ADLUsePanning);
            p_out += "|";
            p_out += pfc::format_int(_OPNEmulatorCore);
        }
    }

void MIDIPreset::unserialize(const char * data)
    {
        if (data == nullptr)
            return;

        const char * Separator = ::strchr(data, '|');

        if (Separator == nullptr)
            return;

        unsigned SchemaVersion = pfc::atodec<unsigned>(data, (t_size)(Separator - data));

        if (SchemaVersion > CurrentSchemaVersion)
            return;

        {
            data = Separator + 1;

            Separator = ::strchr(data, '|');

            if (Separator == nullptr)
                Separator = data += ::strlen(data);
        }

        unsigned PlugInId = pfc::atodec<unsigned>(data, (t_size)(Separator - data));

        if (*Separator == '\0')
            return;

        pfc::string8 VSTPath;
        std::vector<uint8_t> VSTConfig;
        pfc::string8 SoundFontPath;

        bool BASSMIDIEffects = false;
        unsigned int BASSMIDIVoices = 0;
        GUID in_dxi_plugin = { 0 };

        unsigned ADLBankNumber = 0;
        unsigned ADLChipCount = 0;
        bool ADLUsePanning = false;
        bool ADLChorus = false;
        unsigned ADLEmulatorCore = 0;

        unsigned OPNBankNumber = 0;
        unsigned OPNEmulatorCore = 0;

        unsigned MuntGMSet = 0;

        unsigned NukeSynth = 0;
        unsigned NukeBank = 0;
        bool NukePanning = false;

        unsigned SCFlavor = 0;

        unsigned MIDIFlavor = (unsigned)CfgMIDIFlavor;
        bool AllowMIDIEffects = (bool)CfgAllowMIDIEffects;

        {
            data = Separator + 1;

            Separator = ::strchr(data, '|');

            if (Separator == nullptr)
                Separator = data + ::strlen(data);
        }

        if (PlugInId == VSTiPlugInId)
        {
            VSTPath.set_string(data, (t_size)(Separator - data));

            {
                data = Separator + (*Separator == '|');

                Separator = ::strchr(data, '|');

                if (Separator == nullptr)
                    Separator = data + ::strlen(data);
            }

            while (*data && data < Separator)
            {
                VSTConfig.push_back(pfc::atohex<unsigned char>(data, 2));

                data += 2;
            }

            if (CurrentSchemaVersion >= 11 && *data == '|')
            {
                {
                    data = Separator + (*Separator == '|');

                    Separator = ::strchr(data, '|');

                    if (Separator == nullptr)
                        Separator = data + ::strlen(data);
                }

                MIDIFlavor = pfc::atodec<unsigned int>(data, (t_size)(Separator - data));

                {
                    data = Separator + (*Separator == '|');

                    Separator = ::strchr(data, '|');

                    if (Separator == nullptr)
                        Separator = data + ::strlen(data);
                }

                AllowMIDIEffects = !!pfc::atodec<unsigned int>(data, (t_size)(Separator - data));
            }
        }
        else
        if (PlugInId == FluidSynthPlugInId || PlugInId == BASSMIDIPlugInId)
        {
            SoundFontPath.set_string(data, (t_size)(Separator - data));

            {
                data = Separator + (*Separator == '|');

                Separator = ::strchr(data, '|');

                if (Separator == nullptr)
                    Separator = data + ::strlen(data);
            }

            if (Separator > data)
            {
                BASSMIDIEffects = pfc::atodec<bool>(data, 1);

                if (SchemaVersion >= 9)
                {
                    {
                        data = Separator + (*Separator == '|');

                        Separator = ::strchr(data, '|');

                        if (Separator == nullptr)
                            Separator = data + ::strlen(data);
                    }

                    if (Separator > data)
                    {
                        BASSMIDIVoices = pfc::atodec<unsigned int>(data, (t_size)(Separator - data));

                        if (CurrentSchemaVersion >= 11)
                        {
                            {
                                data = Separator + (*Separator == '|');

                                Separator = ::strchr(data, '|');

                                if (Separator == nullptr)
                                    Separator = data + ::strlen(data);
                            }

                            MIDIFlavor = pfc::atodec<unsigned int>(data, (t_size)(Separator - data));

                            {
                                data = Separator + (*Separator == '|');

                                Separator = ::strchr(data, '|');

                                if (Separator == nullptr)
                                    Separator = data + ::strlen(data);
                            }

                            AllowMIDIEffects = !!pfc::atodec<unsigned int>(data, (t_size)(Separator - data));
                        }
                    }
                    else
                    {
                    #ifdef BASSMIDISUPPORT
                        BASSMIDIVoices = (unsigned int) (int) CfgBASSMIDIVoices;
                    #elif defined(FLUIDSYNTHSUPPORT)
                        in_voices = (unsigned int) (int) cfg_fluidsynth_voices;
                    #else
                        in_voices = 256;
                    #endif
                    }
                }
                else
                {
                #ifdef BASSMIDISUPPORT
                    BASSMIDIVoices = (unsigned int) (int) CfgBASSMIDIVoices;
                #elif defined(FLUIDSYNTHSUPPORT)
                    in_voices = (unsigned int) (int) cfg_fluidsynth_voices;
                #else
                    in_voices = 256;
                #endif
                }
            }
            else
            {
            #ifdef BASSMIDISUPPORT
                BASSMIDIEffects = CfgBASSMIDIEffects;
                BASSMIDIVoices = (unsigned int) (int) CfgBASSMIDIVoices;
            #elif defined(FLUIDSYNTHSUPPORT)
                in_effects = cfg_fluidsynth_effects;
                in_voices = (unsigned int) (int) cfg_fluidsynth_voices;
            #else
                in_effects = 1;
                in_voices = 256;
            #endif
            }
        }
        else
        if (PlugInId == SuperMUNTPlugInId)
        {
            size_t i;

            for (i = 0; i < _MUNTGMSetCount; ++i)
            {
                size_t len = ::strlen(_MUNTGMSets[i]);

                if (len == (size_t)(Separator - data) && (::strncmp(data, _MUNTGMSets[i], len) == 0))
                {
                    MuntGMSet = (unsigned int)i;
                    break;
                }
            }

            if (i == _MUNTGMSetCount)
                return;
        }
        else
    #ifdef DXISUPPORT
        else
        if (in_plugin == DirectXPlugInId)
        {
            if (bar_pos - p_in < 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12) return;
            in_dxi_plugin.Data1 = pfc::atohex<t_uint32>(p_in, 8);
            in_dxi_plugin.Data2 = pfc::atohex<t_uint16>(p_in + 8 + 1, 4);
            in_dxi_plugin.Data3 = pfc::atohex<t_uint16>(p_in + 8 + 1 + 4 + 1, 4);
            in_dxi_plugin.Data4[0] = pfc::atohex<t_uint16>(p_in + 8 + 1 + 4 + 1 + 4 + 1, 2);
            in_dxi_plugin.Data4[1] = pfc::atohex<t_uint16>(p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2, 2);
            in_dxi_plugin.Data4[2] = pfc::atohex<t_uint16>(p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2 + 2 + 1, 2);
            in_dxi_plugin.Data4[3] = pfc::atohex<t_uint16>(p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2 + 2 + 1 + 2, 2);
            in_dxi_plugin.Data4[4] = pfc::atohex<t_uint16>(p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2 + 2 + 1 + 2 + 2, 2);
            in_dxi_plugin.Data4[5] = pfc::atohex<t_uint16>(p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2 + 2 + 1 + 2 + 2 + 2, 2);
            in_dxi_plugin.Data4[6] = pfc::atohex<t_uint16>(p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2 + 2 + 1 + 2 + 2 + 2 + 2, 2);
            in_dxi_plugin.Data4[7] = pfc::atohex<t_uint16>(p_in + 8 + 1 + 4 + 1 + 4 + 1 + 2 + 2 + 1 + 2 + 2 + 2 + 2 + 2, 2);
        }
    #endif
        if (PlugInId == ADLPlugInId)
        {
            const char * const * banknames = adl_getBankNames();
            unsigned j = (unsigned int)adl_getBanksCount();
            unsigned i;

            for (i = 0; i < j; ++i)
            {
                size_t len = ::strlen(banknames[i]);

                if (len == (size_t)(Separator - data) && !strncmp(data, banknames[i], len))
                {
                    ADLBankNumber = i;
                    break;
                }
            }

            if (i == j)
                return;

            {
                data = Separator + (*Separator == '|');

                Separator = ::strchr(data, '|');

                if (Separator == nullptr)
                    Separator = data + ::strlen(data);

                if (!*data) return;
            }

            ADLChipCount = pfc::atodec<unsigned>(data, (t_size)(Separator - data));

            {
                data = Separator + (*Separator == '|');

                Separator = ::strchr(data, '|');

                if (Separator == nullptr)
                    Separator = data + ::strlen(data);

                if (!*data) return;
            }

            ADLUsePanning = !!pfc::atodec<unsigned>(data, (t_size)(Separator - data));

            if (CurrentSchemaVersion >= 3)
            {
                {
                    data = Separator + (*Separator == '|');

                    Separator = ::strchr(data, '|');

                    if (Separator == nullptr)
                        Separator = data + ::strlen(data);

                    if (!*data)
                        return;
                }

                ADLChorus = !!pfc::atodec<unsigned>(data, (t_size)(Separator - data));

                if (CurrentSchemaVersion >= 7)
                {
                    {
                        data = Separator + (*Separator == '|');

                        Separator = ::strchr(data, '|');

                        if (Separator == nullptr)
                            Separator = data + ::strlen(data);

                        if (!*data)
                            return;
                    }

                    ADLEmulatorCore = pfc::atodec<unsigned>(data, (t_size)(Separator - data));
                }
                else
                    ADLEmulatorCore = ADLMIDI_EMU_DOSBOX;
            }
            else
                ADLEmulatorCore = ADLMIDI_EMU_DOSBOX;
        }
        else
        if (PlugInId == OPNPlugInId)
        {
            if (CurrentSchemaVersion >= 10)
            {
                OPNBankNumber = pfc::atodec<unsigned>(data, (t_size)(Separator - data));

                {
                    data = Separator + (*Separator == '|');

                    Separator = ::strchr(data, '|');

                    if (Separator == nullptr)
                        Separator = data + ::strlen(data);

                    if (!*data)
                        return;
                }

                ADLChipCount = pfc::atodec<unsigned>(data, (t_size)(Separator - data));

                {
                    data = Separator + (*Separator == '|');
                    Separator = ::strchr(data, '|');
                    if (!Separator) Separator = data + ::strlen(data);
                    if (!*data) return;
                }

                ADLUsePanning = !!pfc::atodec<unsigned>(data, (t_size)(Separator - data));

                {
                    data = Separator + (*Separator == '|');
                    Separator = ::strchr(data, '|');
                    if (!Separator) Separator = data + ::strlen(data);
                    if (!*data) return;
                }

                OPNEmulatorCore = !!pfc::atodec<unsigned>(data, (t_size)(Separator - data));
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
        if (PlugInId == NukePlugInId)
        {
            pfc::string8 temp;

            temp.set_string(data, (t_size)(Separator - data));

            GetNukePreset(temp, NukeSynth, NukeBank);

            if (CurrentSchemaVersion >= 6)
            {
                {
                    data = Separator + (*Separator == '|');

                    Separator = ::strchr(data, '|');

                    if (Separator == nullptr)
                        Separator = data + ::strlen(data);

                    if (!*data)
                        return;
                }

                NukePanning = !!pfc::atodec<unsigned>(data, (t_size)(Separator - data));
            }
            else
            {
                NukePanning = true;
            }
        }
        else
        if (PlugInId == SecretSaucePlugInId)
        {
            SCFlavor = pfc::atodec<unsigned>(data, (t_size)(Separator - data));

            if (CurrentSchemaVersion >= 6)
            {
                {
                    data = Separator + (*Separator == '|');

                    Separator = ::strchr(data, '|');

                    if (Separator == nullptr)
                        Separator = data + ::strlen(data);

                    if (!*data)
                        return;
                }

                if (CurrentSchemaVersion >= 11)
                {
                    MIDIFlavor = pfc::atodec<unsigned>(data, (t_size)(Separator - data));

                    {
                        data = Separator + (*Separator == '|');

                        Separator = ::strchr(data, '|');

                        if (Separator == nullptr)
                            Separator = data + ::strlen(data);

                        if (!*data)
                            return;
                    }
                }
                else
                if (CurrentSchemaVersion >= 8)
                {
                    unsigned GSFlavor = pfc::atodec<unsigned>(data, (t_size)(Separator - data));

                    {
                        data = Separator + (*Separator == '|');

                        Separator = ::strchr(data, '|');

                        if (Separator == nullptr)
                            Separator = data + ::strlen(data);

                        if (*data == '\0')
                            return;
                    }

                    if (SCFlavor == 4)
                        MIDIFlavor = MIDIPlayer::filter_xg;
                    else
                    if (SCFlavor == 3)
                        MIDIFlavor = (GSFlavor == 0) ? MIDIPlayer::filter_default : MIDIPlayer::filter_sc55 + (GSFlavor - 1);
                    else
                    if (SCFlavor <= 2)
                        MIDIFlavor = SCFlavor;
                }
                else
                    MIDIFlavor = SCFlavor;

                AllowMIDIEffects = !!pfc::atodec<unsigned>(data, (t_size)(Separator - data));
            }
            else
            {
                if (SCFlavor >= 3 && SCFlavor <= 6)
                    MIDIFlavor = SCFlavor;
                else
                if (SCFlavor == 7)
                    MIDIFlavor = SCFlavor;

                AllowMIDIEffects = true;
            }

            if (MIDIFlavor > MIDIPlayer::filter_xg)
                MIDIFlavor = MIDIPlayer::filter_default;
        }

        _PluginId = PlugInId;

        _VSTPathName = VSTPath;
        _VSTConfig = VSTConfig;

        _SoundFontPathName = SoundFontPath;

        _BASSMIDIEffects = BASSMIDIEffects;
        _BASSMIDIVoices = BASSMIDIVoices;

    #ifdef DXISUPPORT
        dxi_plugin = in_dxi_plugin;
    #endif

        _ADLBankNumber = ADLBankNumber;
        _ADLChipCount = ADLChipCount;
        _ADLUsePanning = ADLUsePanning;
        _ADLChorus = ADLChorus;
        _ADLEmulatorCore = ADLEmulatorCore;

        _OPNBankNumber = OPNBankNumber;
        _OPNEmulatorCore = OPNEmulatorCore;

        _MuntGMSet = MuntGMSet;

        _NukeSynth = NukeSynth;
        _NukeBank = NukeBank;
        _NukePanning = NukePanning;

        _MIDIStandard = MIDIFlavor;
        _UseMIDIEffects = AllowMIDIEffects;
    }

#pragma region("NukePresets")
/// <summary>
/// Gets the Nuke preset with the specified name.
/// </summary>
void MIDIPreset::GetNukePreset(const char * name, unsigned int & synth, unsigned int & bank)
{
    for (size_t i = 0; i < _NukePresets.get_count(); ++i)
    {
        const NukePreset & Preset = _NukePresets[i];

        if (::strcmp(Preset.name, name) == 0)
        {
            synth = Preset.synth;
            bank = Preset.bank;

            return;
        }
    }

    synth = DefaultMSSynth;
    bank = DefaultMSBank;
}

/// <summary>
/// Imports the synthesizer settings from the Nuke player.
/// </summary>
class NukePresetsImporter
{
public:
    NukePresetsImporter()
    {
        NukePlayer::EnumerateSynthesizers(EnumCallback);
    }

private:
    static void EnumCallback(unsigned int synth, unsigned int bank, const char * name)
    {
        NukePreset Preset = { synth, bank, name };

        _NukePresets.append_single(Preset);
    }
};

NukePresetsImporter _NukePresetsImporter;
#pragma endregion
