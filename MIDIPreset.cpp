
/** $VER: MIDIPreset.cpp (2022.12.30) **/

#include "MIDIPreset.h"

#include "MSPlayer.h"

MIDIPreset::MIDIPreset() noexcept
{
    {
        _PluginId = (unsigned int)CfgPluginId;
        _VSTPathName = cfg_vst_path;

        {
            VSTiPlayer * Player = nullptr;

            try
            {
                Player = new VSTiPlayer;

                if (Player->LoadVST(_VSTPathName))
                {
                    vst_config = cfg_vst_config[(unsigned int)(Player->getUniqueID())];
                }
            }
            catch (...)
            {
                if (_PluginId == 1)
                    _PluginId = 0;
            }

            delete Player;
        }

        _SoundFontPathName = cfg_soundfont_path;
    }

#ifdef BASSMIDISUPPORT
    {
        _BASSMIDIEffects = cfg_bassmidi_effects;
        _BASSMIDIVoices = (unsigned int) (int) cfg_bassmidi_voices;
    }
#endif

#ifdef FLUIDSYNTHSUPPORT
    {
        effects = cfg_fluidsynth_effects;
        voices = (unsigned int) (int) cfg_fluidsynth_voices;
    }
#endif

#ifdef DXISUPPORT
    {
        dxi_plugin = cfg_dxi_plugin;
    }
#endif

    {
        _ADLBankNumber = (unsigned int)cfg_adl_bank;
        _ADLChipCount = (unsigned int)cfg_adl_chips;
        _ADLUsePanning = !!cfg_adl_panning;

        if (cfg_adl_core_dosbox)
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
            opn_emu_core = OPNMIDI_EMU_MAME;
        else
        if (cfg_opn_core_nuked)
            opn_emu_core = OPNMIDI_EMU_NUKED;
        else
            opn_emu_core = OPNMIDI_EMU_GENS;
        if (cfg_opn_bank_xg)
            opn_bank = 0;
        else
        if (cfg_opn_bank_gs)
            opn_bank = 1;
        else
        if (cfg_opn_bank_gems)
            opn_bank = 2;
        else
        if (cfg_opn_bank_tomsoft)
            opn_bank = 3;
        else
            opn_bank = 4;
    }

    {
        munt_gm_set = (unsigned int)cfg_munt_gm;
    }

    {
        ms_synth = (unsigned int)cfg_ms_synth;
        ms_bank = (unsigned int)cfg_ms_bank;
        ms_panning = (bool)cfg_ms_panning;
    }

    {
        _MIDIStandard = (unsigned int)cfg_midi_flavor;
        _UseMIDIReverb = (bool)cfg_midi_reverb;
    }
}

/// <summary>
/// Gets the name of the MSPreset for the specified synthesizer and bank.
/// </summary>
const char * MIDIPreset::GetMSPresetName(unsigned int synth, unsigned int bank)
{
    for (size_t i = 0; i < _MSPresets.get_count(); ++i)
    {
        const MSPreset & Preset = _MSPresets[i];

        if ((Preset.synth == synth) && (Preset.bank == bank))
            return Preset.name;
    }

    return "Unknown Preset";
}

void MIDIPreset::serialize(pfc::string8 & p_out)
    {
        const char * const * banknames = adl_getBankNames();

        p_out.reset();

        p_out += pfc::format_int(Version);
        p_out += "|";

        p_out += pfc::format_int(_PluginId);

        if (_PluginId == 1)
        {
            p_out += "|";

            p_out += _VSTPathName;
            p_out += "|";

            for (unsigned i = 0; i < vst_config.size(); ++i)
            {
                p_out += pfc::format_hex(vst_config[i], 2);
            }

            p_out += "|";
            p_out += pfc::format_int(_MIDIStandard);
            p_out += "|";
            p_out += pfc::format_int(_UseMIDIReverb);
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

            p_out += pfc::format_int(_UseMIDIReverb);
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

            p_out += MuntBankNames[munt_gm_set];
        }
        else
        if (_PluginId == 9)
        {
            p_out += "|";
            p_out += GetMSPresetName(ms_synth, ms_bank);
            p_out += "|";
            p_out += pfc::format_int(ms_panning);
        }
        else
        if (_PluginId == 10)
        {
            p_out += "|";
            p_out += pfc::format_int(_MIDIStandard);
            p_out += "|";
            p_out += pfc::format_int(_UseMIDIReverb);
        }
        else
        if (_PluginId == 7)
        {
            p_out += "|";
            p_out += pfc::format_int(opn_bank);
            p_out += "|";
            p_out += pfc::format_int(_ADLChipCount);
            p_out += "|";
            p_out += pfc::format_int(_ADLUsePanning);
            p_out += "|";
            p_out += pfc::format_int(opn_emu_core);
        }
    }

void MIDIPreset::unserialize(const char * data)
    {
        if (data == nullptr)
            return;

        const char * Separator = ::strchr(data, '|');

        if (Separator == nullptr)
            return;

        unsigned in_version = pfc::atodec<unsigned>(data, Separator - data);

        if (in_version > Version)
            return;

        {
            data = Separator + 1;

            Separator = ::strchr(data, '|');

            if (Separator == nullptr)
                Separator = data += ::strlen(data);
        }

        unsigned in_plugin = pfc::atodec<unsigned>(data, Separator - data);

        pfc::string8 in_vst_path;
        std::vector<uint8_t> in_vst_config;
        pfc::string8 in_soundfont_path;
        bool in_effects;
        unsigned int in_voices;
        GUID in_dxi_plugin = { 0 };
        unsigned in_adl_bank, in_adl_chips;
        bool in_adl_panning;
        bool in_adl_chorus;
        unsigned in_adl_emu_core;
        unsigned in_munt_gm_set;
        unsigned in_ms_synth, in_ms_bank;
        unsigned in_sc_flavor;
        unsigned in_gs_flavor;
        bool in_ms_panning;
        unsigned in_opn_bank;
        unsigned in_opn_emu_core;
        unsigned in_midi_flavor = (unsigned)cfg_midi_flavor;
        bool in_midi_reverb = (bool)cfg_midi_reverb;

        if (*Separator)
        {
            {
                data = Separator + 1;

                Separator = ::strchr(data, '|');

                if (Separator == nullptr)
                    Separator = data + ::strlen(data);
            }

            if (in_plugin == 1)
            {
                in_vst_path.set_string(data, Separator - data);

                {
                    data = Separator + (*Separator == '|');

                    Separator = ::strchr(data, '|');

                    if (Separator == nullptr)
                        Separator = data + ::strlen(data);
                }

                while (*data && data < Separator)
                {
                    in_vst_config.push_back(pfc::atohex<unsigned char>(data, 2));

                    data += 2;
                }

                if (Version >= 11 && *data == '|')
                {
                    {
                        data = Separator + (*Separator == '|');

                        Separator = ::strchr(data, '|');

                        if (Separator == nullptr)
                            Separator = data + ::strlen(data);
                    }

                    in_midi_flavor = pfc::atodec<unsigned int>(data, Separator - data);

                    {
                        data = Separator + (*Separator == '|');

                        Separator = ::strchr(data, '|');

                        if (Separator == nullptr)
                            Separator = data + ::strlen(data);
                    }

                    in_midi_reverb = !!pfc::atodec<unsigned int>(data, Separator - data);
                }
            }
            else
            if (in_plugin == 2 || in_plugin == 4)
            {
                in_soundfont_path.set_string(data, Separator - data);

                {
                    data = Separator + (*Separator == '|');

                    Separator = ::strchr(data, '|');

                    if (Separator == nullptr)
                        Separator = data + ::strlen(data);
                }

                if (Separator > data)
                {
                    in_effects = pfc::atodec<bool>(data, 1);

                    if (in_version >= 9)
                    {
                        {
                            data = Separator + (*Separator == '|');

                            Separator = ::strchr(data, '|');

                            if (Separator == nullptr)
                                Separator = data + ::strlen(data);
                        }

                        if (Separator > data)
                        {
                            in_voices = pfc::atodec<unsigned int>(data, Separator - data);

                            if (Version >= 11)
                            {
                                {
                                    data = Separator + (*Separator == '|');

                                    Separator = ::strchr(data, '|');

                                    if (Separator == nullptr)
                                        Separator = data + ::strlen(data);
                                }

                                in_midi_flavor = pfc::atodec<unsigned int>(data, Separator - data);

                                {
                                    data = Separator + (*Separator == '|');

                                    Separator = ::strchr(data, '|');

                                    if (Separator == nullptr)
                                        Separator = data + ::strlen(data);
                                }

                                in_midi_reverb = !!pfc::atodec<unsigned int>(data, Separator - data);
                            }
                        }
                        else
                        {
                        #ifdef BASSMIDISUPPORT
                            in_voices = (unsigned int) (int) cfg_bassmidi_voices;
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
                        in_voices = (unsigned int) (int) cfg_bassmidi_voices;
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
                    in_effects = cfg_bassmidi_effects;
                    in_voices = (unsigned int) (int) cfg_bassmidi_voices;
                #elif defined(FLUIDSYNTHSUPPORT)
                    in_effects = cfg_fluidsynth_effects;
                    in_voices = (unsigned int) (int) cfg_fluidsynth_voices;
                #else
                    in_effects = 1;
                    in_voices = 256;
                #endif
                }
            }
        #ifdef DXISUPPORT
            else
            if (in_plugin == 5)
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
            else
            if (in_plugin == 6)
            {
                const char * const * banknames = adl_getBankNames();
                unsigned j = (unsigned int)adl_getBanksCount();
                unsigned i;

                for (i = 0; i < j; ++i)
                {
                    size_t len = ::strlen(banknames[i]);

                    if (len == (size_t)(Separator - data) && !strncmp(data, banknames[i], len))
                    {
                        in_adl_bank = i;
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

                in_adl_chips = pfc::atodec<unsigned>(data, Separator - data);

                {
                    data = Separator + (*Separator == '|');

                    Separator = ::strchr(data, '|');

                    if (Separator == nullptr)
                        Separator = data + ::strlen(data);

                    if (!*data) return;
                }

                in_adl_panning = !!pfc::atodec<unsigned>(data, Separator - data);

                if (Version >= 3)
                {
                    {
                        data = Separator + (*Separator == '|');

                        Separator = ::strchr(data, '|');

                        if (Separator == nullptr)
                            Separator = data + ::strlen(data);

                        if (!*data)
                            return;
                    }

                    in_adl_chorus = !!pfc::atodec<unsigned>(data, Separator - data);

                    if (Version >= 7)
                    {
                        {
                            data = Separator + (*Separator == '|');

                            Separator = ::strchr(data, '|');

                            if (Separator == nullptr)
                                Separator = data + ::strlen(data);

                            if (!*data)
                                return;
                        }

                        in_adl_emu_core = pfc::atodec<unsigned>(data, Separator - data);
                    }
                    else
                        in_adl_emu_core = ADLMIDI_EMU_DOSBOX;
                }
                else
                    in_adl_emu_core = ADLMIDI_EMU_DOSBOX;
            }
            else
            if (in_plugin == 3)
            {
                unsigned i, j;

                for (i = 0, j = _countof(MuntBankNames); i < j; ++i)
                {
                    size_t len = ::strlen(MuntBankNames[i]);

                    if (len == (size_t)(Separator - data) && !strncmp(data, MuntBankNames[i], len))
                    {
                        in_munt_gm_set = i;
                        break;
                    }
                }

                if (i == j)
                    return;
            }
            else
            if (in_plugin == 9)
            {
                pfc::string8 temp;

                temp.set_string(data, Separator - data);

                GetMSPreset(temp, in_ms_synth, in_ms_bank);

                if (Version >= 6)
                {
                    {
                        data = Separator + (*Separator == '|');

                        Separator = ::strchr(data, '|');

                        if (Separator == nullptr)
                            Separator = data + ::strlen(data);

                        if (!*data)
                            return;
                    }

                    in_ms_panning = !!pfc::atodec<unsigned>(data, Separator - data);
                }
                else
                {
                    in_ms_panning = true;
                }
            }
            else
            if (in_plugin == 10)
            {
                in_sc_flavor = pfc::atodec<unsigned>(data, Separator - data);

                if (Version >= 6)
                {
                    {
                        data = Separator + (*Separator == '|');

                        Separator = ::strchr(data, '|');

                        if (Separator == nullptr)
                            Separator = data + ::strlen(data);

                        if (!*data)
                            return;
                    }

                    if (Version >= 11)
                    {
                        in_midi_flavor = pfc::atodec<unsigned>(data, Separator - data);

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
                    if (Version >= 8)
                    {
                        in_gs_flavor = pfc::atodec<unsigned>(data, Separator - data);

                        {
                            data = Separator + (*Separator == '|');

                            Separator = ::strchr(data, '|');

                            if (Separator == nullptr)
                                Separator = data + ::strlen(data);

                            if (!*data)
                                return;
                        }

                        if (in_sc_flavor == 4)
                            in_midi_flavor = MIDIPlayer::filter_xg;
                        else
                        if (in_sc_flavor == 3)
                            in_midi_flavor = (in_gs_flavor == 0) ? MIDIPlayer::filter_default : MIDIPlayer::filter_sc55 + (in_gs_flavor - 1);
                        else
                        if (in_sc_flavor >= 0 && in_sc_flavor <= 2)
                            in_midi_flavor = in_sc_flavor;
                    }
                    else
                        in_midi_flavor = in_sc_flavor;

                    in_midi_reverb = !!pfc::atodec<unsigned>(data, Separator - data);
                }
                else
                {
                    if (in_sc_flavor >= 3 && in_sc_flavor <= 6)
                        in_midi_flavor = in_sc_flavor;
                    else
                    if (in_sc_flavor == 7)
                        in_midi_flavor = in_sc_flavor;

                    in_midi_reverb = true;
                }

                if (in_midi_flavor > MIDIPlayer::filter_xg)
                    in_midi_flavor = MIDIPlayer::filter_default;
            }
            else
            if (in_plugin == 7)
            {
                if (Version >= 10)
                {
                    in_opn_bank = pfc::atodec<unsigned>(data, Separator - data);

                    {
                        data = Separator + (*Separator == '|');

                        Separator = ::strchr(data, '|');

                        if (Separator == nullptr)
                            Separator = data + ::strlen(data);

                        if (!*data)
                            return;
                    }

                    in_adl_chips = pfc::atodec<unsigned>(data, Separator - data);

                    {
                        data = Separator + (*Separator == '|');
                        Separator = ::strchr(data, '|');
                        if (!Separator) Separator = data + ::strlen(data);
                        if (!*data) return;
                    }

                    in_adl_panning = !!pfc::atodec<unsigned>(data, Separator - data);

                    {
                        data = Separator + (*Separator == '|');
                        Separator = ::strchr(data, '|');
                        if (!Separator) Separator = data + ::strlen(data);
                        if (!*data) return;
                    }

                    in_opn_emu_core = !!pfc::atodec<unsigned>(data, Separator - data);
                }
                else
                {
                    in_opn_bank = 4;
                    in_adl_chips = 10;
                    in_adl_panning = true;
                    in_opn_emu_core = 0;
                }
            }

            _PluginId = in_plugin;
            _VSTPathName = in_vst_path;
            vst_config = in_vst_config;
            _SoundFontPathName = in_soundfont_path;

            _BASSMIDIEffects = in_effects;
            _BASSMIDIVoices = in_voices;

        #ifdef DXISUPPORT
            dxi_plugin = in_dxi_plugin;
        #endif

            _ADLBankNumber = in_adl_bank;
            _ADLChipCount = in_adl_chips;
            _ADLUsePanning = in_adl_panning;
            _ADLChorus = in_adl_chorus;
            _ADLEmulatorCore = in_adl_emu_core;

            opn_bank = in_opn_bank;
            opn_emu_core = in_opn_emu_core;

            munt_gm_set = in_munt_gm_set;

            ms_synth = in_ms_synth;
            ms_bank = in_ms_bank;
            ms_panning = in_ms_panning;

            _MIDIStandard = in_midi_flavor;
            _UseMIDIReverb = in_midi_reverb;
        }
    }

/// <summary>
/// Gets the MSPreset with the specified name.
/// </summary>
void MIDIPreset::GetMSPreset(const char * name, unsigned int & synth, unsigned int & bank)
{
    for (size_t i = 0; i < _MSPresets.get_count(); ++i)
    {
        const MSPreset & Preset = _MSPresets[i];

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

#pragma region("MSPresets")
pfc::array_t<MSPreset> _MSPresets;

/// <summary>
/// Imports the synthesizer settings from the MS player.
/// </summary>
class MSPresetsImporter
{
public:
    MSPresetsImporter()
    {
        MSPlayer::enum_synthesizers(EnumCallback);
    }

private:
    static void EnumCallback(unsigned int synth, unsigned int bank, const char * name)
    {
        MSPreset t = { synth, bank, name };

        _MSPresets.append_single(t);
    }
};

MSPresetsImporter _MSPresetsImporter;
#pragma endregion
