
/** $VER: Preset.h (2025.07.09) **/

#pragma once

#include <pfc/pfc-lite.h>

#include "ADLPlayer.h"
#include "Configuration.h"
#include "OPNPlayer/OPNPlayer.h"
#include "VSTiPlayer.h"

#pragma warning(disable: 4820) // x bytes padding added after data member

class preset_t
{
public:
    preset_t() noexcept;

    preset_t(const preset_t &) = delete;
    preset_t(preset_t &&) = delete;
    preset_t & operator=(const preset_t &) = delete;
    preset_t & operator=(preset_t &&) = delete;

    virtual ~preset_t() { };

    void Serialize(pfc::string & text);
    void Deserialize(const char * text);

public:
    const uint32_t CurrentSchemaVersion = 12;

    PlayerTypes _PlayerType;

    pfc::string _PlugInFilePath; // VSTi or CLAP
    std::vector<uint8_t> _VSTiConfig;
    uint32_t _CLAPPlugInIndex; // index in the CLAP plug-in the factory

    pfc::string _SoundfontFilePath;

    // ADL
    uint32_t _ADLBankNumber;
    uint32_t _ADLEmulatorCore;
    uint32_t _ADLChipCount;
    bool _ADLSoftPanning;
    bool _ADLUseChorus;
    std::string _ADLBankFilePath;

    // OPN
    uint32_t _OPNBankNumber;
    uint32_t _OPNEmulatorCore;
    uint32_t _OPNChipCount;
    bool _OPNSoftPanning;
    std::string _OPNBankFilePath;

    // MT32Emu
    uint32_t _MT32EmuGMSet;

    // FluidSynth / BASS MIDI
    bool _EffectsEnabled;
    uint32_t _VoiceCount;

    // Nuke
    uint32_t _NukeSynth;
    uint32_t _NukeBank;
    bool _NukeUsePanning;

    MIDIFlavors _MIDIFlavor;
    bool _UseMIDIEffects;
    bool _UseMT32EmuWithMT32;
    bool _UseVSTiWithXG;
    bool _DetectExtraDrum;
};

#pragma warning(default: 4820) // x bytes padding added after data member
