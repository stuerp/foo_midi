
/** $VER: MIDIPreset.cpp (2023.01.04) **/

#pragma warning(disable: 5045)

#include "MIDIPreset.h"

#include "Configuration.h"
#include "NukePlayer.h"

MIDIPreset::MIDIPreset() noexcept
{
    {
        _PlayerType = (unsigned int)CfgPlayerType;
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
                if (_PlayerType == 1)
                    _PlayerType = 0;
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
        _MuntGMSet = (unsigned int)CfgMuntGMSet;
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

void MIDIPreset::Serialize(pfc::string8 & text)
    {
        const char * const * banknames = adl_getBankNames();

        text.reset();
        text.prealloc(512);

        text += pfc::format_int(CurrentSchemaVersion);
        text += "|";

        text += pfc::format_int(_PlayerType);

        if (_PlayerType == 1)
        {
            text += "|";

            text += _VSTPathName;
            text += "|";

            for (unsigned i = 0; i < _VSTConfig.size(); ++i)
            {
                text += pfc::format_hex(_VSTConfig[i], 2);
            }

            text += "|";
            text += pfc::format_int(_MIDIStandard);
            text += "|";
            text += pfc::format_int(_UseMIDIEffects);
        }
        else
        if (_PlayerType == 2 || _PlayerType == 4)
        {
            text += "|";

            text += _SoundFontPathName;

            text += "|";

            text += pfc::format_int(_BASSMIDIEffects);

            text += "|";

            text += pfc::format_int(_BASSMIDIVoices);

            text += "|";

            text += pfc::format_int(_MIDIStandard);

            text += "|";

            text += pfc::format_int(_UseMIDIEffects);
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
        if (_PlayerType == 6)
        {
            text += "|";

            text += banknames[_ADLBankNumber];
            text += "|";

            text += pfc::format_int(_ADLChipCount);
            text += "|";

            text += pfc::format_int(_ADLUsePanning);
            text += "|";

            text += pfc::format_int(_ADLChorus);
            text += "|";

            text += pfc::format_int(_ADLEmulatorCore);
        }
        else
        if (_PlayerType == 3)
        {
            text += "|";

            text += _MuntGMSets[_MuntGMSet];
        }
        else
        if (_PlayerType == 9)
        {
            text += "|";
            text += NukePlayer::GetPresetName(_NukeSynth, _NukeBank);
            text += "|";
            text += pfc::format_int(_NukePanning);
        }
        else
        if (_PlayerType == 10)
        {
            text += "|";
            text += pfc::format_int(_MIDIStandard);
            text += "|";
            text += pfc::format_int(_UseMIDIEffects);
        }
        else
        if (_PlayerType == 7)
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
    }

void MIDIPreset::Deserialize(const char * text)
    {
        if (text == nullptr)
            return;

        const char * Separator = ::strchr(text, '|');

        if (Separator == nullptr)
            return;

        unsigned int SchemaVersion = pfc::atodec<unsigned>(text, (t_size)(Separator - text));

        if (SchemaVersion > CurrentSchemaVersion)
            return;

        {
            text = Separator + 1;

            Separator = ::strchr(text, '|');

            if (Separator == nullptr)
                Separator = text += ::strlen(text);
        }

        unsigned PlugInId = pfc::atodec<unsigned>(text, (t_size)(Separator - text));

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
            text = Separator + 1;

            Separator = ::strchr(text, '|');

            if (Separator == nullptr)
                Separator = text + ::strlen(text);
        }

        if (PlugInId == PlayerTypeVSTi)
        {
            VSTPath.set_string(text, (t_size)(Separator - text));

            {
                text = Separator + (*Separator == '|');

                Separator = ::strchr(text, '|');

                if (Separator == nullptr)
                    Separator = text + ::strlen(text);
            }

            while (*text && text < Separator)
            {
                VSTConfig.push_back(pfc::atohex<unsigned char>(text, 2));

                text += 2;
            }

            if (CurrentSchemaVersion >= 11 && *text == '|')
            {
                {
                    text = Separator + (*Separator == '|');

                    Separator = ::strchr(text, '|');

                    if (Separator == nullptr)
                        Separator = text + ::strlen(text);
                }

                MIDIFlavor = pfc::atodec<unsigned int>(text, (t_size)(Separator - text));

                {
                    text = Separator + (*Separator == '|');

                    Separator = ::strchr(text, '|');

                    if (Separator == nullptr)
                        Separator = text + ::strlen(text);
                }

                AllowMIDIEffects = !!pfc::atodec<unsigned int>(text, (t_size)(Separator - text));
            }
        }
        else
        if (PlugInId == PlayerTypeFluidSynth || PlugInId == PlayerTypeBASSMIDI)
        {
            SoundFontPath.set_string(text, (t_size)(Separator - text));

            {
                text = Separator + (*Separator == '|');

                Separator = ::strchr(text, '|');

                if (Separator == nullptr)
                    Separator = text + ::strlen(text);
            }

            if (Separator > text)
            {
                BASSMIDIEffects = pfc::atodec<bool>(text, 1);

                if (SchemaVersion >= 9)
                {
                    {
                        text = Separator + (*Separator == '|');

                        Separator = ::strchr(text, '|');

                        if (Separator == nullptr)
                            Separator = text + ::strlen(text);
                    }

                    if (Separator > text)
                    {
                        BASSMIDIVoices = pfc::atodec<unsigned int>(text, (t_size)(Separator - text));

                        if (CurrentSchemaVersion >= 11)
                        {
                            {
                                text = Separator + (*Separator == '|');

                                Separator = ::strchr(text, '|');

                                if (Separator == nullptr)
                                    Separator = text + ::strlen(text);
                            }

                            MIDIFlavor = pfc::atodec<unsigned int>(text, (t_size)(Separator - text));

                            {
                                text = Separator + (*Separator == '|');

                                Separator = ::strchr(text, '|');

                                if (Separator == nullptr)
                                    Separator = text + ::strlen(text);
                            }

                            AllowMIDIEffects = !!pfc::atodec<unsigned int>(text, (t_size)(Separator - text));
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
        if (PlugInId == PlayerTypeSuperMunt)
        {
            size_t i;

            for (i = 0; i < _MuntGMSetCount; ++i)
            {
                size_t len = ::strlen(_MuntGMSets[i]);

                if (len == (size_t)(Separator - text) && (::strncmp(text, _MuntGMSets[i], len) == 0))
                {
                    MuntGMSet = (unsigned int)i;
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
        if (PlugInId == PlayerTypeADL)
        {
            const char * const * banknames = adl_getBankNames();
            unsigned j = (unsigned int)adl_getBanksCount();
            unsigned i;

            for (i = 0; i < j; ++i)
            {
                size_t len = ::strlen(banknames[i]);

                if (len == (size_t)(Separator - text) && !strncmp(text, banknames[i], len))
                {
                    ADLBankNumber = i;
                    break;
                }
            }

            if (i == j)
                return;

            {
                text = Separator + (*Separator == '|');

                Separator = ::strchr(text, '|');

                if (Separator == nullptr)
                    Separator = text + ::strlen(text);

                if (!*text) return;
            }

            ADLChipCount = pfc::atodec<unsigned>(text, (t_size)(Separator - text));

            {
                text = Separator + (*Separator == '|');

                Separator = ::strchr(text, '|');

                if (Separator == nullptr)
                    Separator = text + ::strlen(text);

                if (!*text) return;
            }

            ADLUsePanning = !!pfc::atodec<unsigned>(text, (t_size)(Separator - text));

            if (CurrentSchemaVersion >= 3)
            {
                {
                    text = Separator + (*Separator == '|');

                    Separator = ::strchr(text, '|');

                    if (Separator == nullptr)
                        Separator = text + ::strlen(text);

                    if (!*text)
                        return;
                }

                ADLChorus = !!pfc::atodec<unsigned>(text, (t_size)(Separator - text));

                if (CurrentSchemaVersion >= 7)
                {
                    {
                        text = Separator + (*Separator == '|');

                        Separator = ::strchr(text, '|');

                        if (Separator == nullptr)
                            Separator = text + ::strlen(text);

                        if (!*text)
                            return;
                    }

                    ADLEmulatorCore = pfc::atodec<unsigned>(text, (t_size)(Separator - text));
                }
                else
                    ADLEmulatorCore = ADLMIDI_EMU_DOSBOX;
            }
            else
                ADLEmulatorCore = ADLMIDI_EMU_DOSBOX;
        }
        else
        if (PlugInId == PlayerTypeOPN)
        {
            if (CurrentSchemaVersion >= 10)
            {
                OPNBankNumber = pfc::atodec<unsigned>(text, (t_size)(Separator - text));

                {
                    text = Separator + (*Separator == '|');

                    Separator = ::strchr(text, '|');

                    if (Separator == nullptr)
                        Separator = text + ::strlen(text);

                    if (!*text)
                        return;
                }

                ADLChipCount = pfc::atodec<unsigned>(text, (t_size)(Separator - text));

                {
                    text = Separator + (*Separator == '|');
                    Separator = ::strchr(text, '|');
                    if (!Separator) Separator = text + ::strlen(text);
                    if (!*text) return;
                }

                ADLUsePanning = !!pfc::atodec<unsigned>(text, (t_size)(Separator - text));

                {
                    text = Separator + (*Separator == '|');
                    Separator = ::strchr(text, '|');
                    if (!Separator) Separator = text + ::strlen(text);
                    if (!*text) return;
                }

                OPNEmulatorCore = !!pfc::atodec<unsigned>(text, (t_size)(Separator - text));
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
        if (PlugInId == PlayerTypeNuke)
        {
            pfc::string8 temp;

            temp.set_string(text, (t_size)(Separator - text));

            NukePlayer::GetPreset(temp, NukeSynth, NukeBank);

            if (CurrentSchemaVersion >= 6)
            {
                {
                    text = Separator + (*Separator == '|');

                    Separator = ::strchr(text, '|');

                    if (Separator == nullptr)
                        Separator = text + ::strlen(text);

                    if (!*text)
                        return;
                }

                NukePanning = !!pfc::atodec<unsigned>(text, (t_size)(Separator - text));
            }
            else
            {
                NukePanning = true;
            }
        }
        else
        if (PlugInId == PlayerTypeSecretSauce)
        {
            SCFlavor = pfc::atodec<unsigned>(text, (t_size)(Separator - text));

            if (CurrentSchemaVersion >= 6)
            {
                {
                    text = Separator + (*Separator == '|');

                    Separator = ::strchr(text, '|');

                    if (Separator == nullptr)
                        Separator = text + ::strlen(text);

                    if (!*text)
                        return;
                }

                if (CurrentSchemaVersion >= 11)
                {
                    MIDIFlavor = pfc::atodec<unsigned>(text, (t_size)(Separator - text));

                    {
                        text = Separator + (*Separator == '|');

                        Separator = ::strchr(text, '|');

                        if (Separator == nullptr)
                            Separator = text + ::strlen(text);

                        if (!*text)
                            return;
                    }
                }
                else
                if (CurrentSchemaVersion >= 8)
                {
                    unsigned GSFlavor = pfc::atodec<unsigned>(text, (t_size)(Separator - text));

                    {
                        text = Separator + (*Separator == '|');

                        Separator = ::strchr(text, '|');

                        if (Separator == nullptr)
                            Separator = text + ::strlen(text);

                        if (*text == '\0')
                            return;
                    }

                    if (SCFlavor == 4)
                        MIDIFlavor = MIDIPlayer::filter_xg;
                    else
                    if (SCFlavor == 3)
                        MIDIFlavor = (GSFlavor == 0) ? MIDIPlayer::FilterNone : MIDIPlayer::filter_sc55 + (GSFlavor - 1);
                    else
                    if (SCFlavor <= 2)
                        MIDIFlavor = SCFlavor;
                }
                else
                    MIDIFlavor = SCFlavor;

                AllowMIDIEffects = !!pfc::atodec<unsigned>(text, (t_size)(Separator - text));
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
                MIDIFlavor = MIDIPlayer::FilterNone;
        }

        _PlayerType = PlugInId;

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
