
/** $VER: MIDIPreset.h (2023.07.25) **/

#pragma once

#include <pfc/pfc-lite.h>

#include "ADLPlayer.h"
#include "Configuration.h"
#include "OPNPlayer/OPNPlayer.h"
#include "VSTiPlayer.h"

#pragma warning(disable: 4820) // x bytes padding added after data member
class MIDIPreset
{
public:
    MIDIPreset() noexcept;

    MIDIPreset(const MIDIPreset&) = delete;
    MIDIPreset(const MIDIPreset&&) = delete;
    MIDIPreset& operator=(const MIDIPreset&) = delete;
    MIDIPreset& operator=(MIDIPreset&&) = delete;

    virtual ~MIDIPreset() { };

    void Serialize(pfc::string8 & text);
    void Deserialize(const char * text);

public:
    const uint32_t CurrentSchemaVersion = 12;

    PlayerType _PlayerType;

    pfc::string8 _VSTiFilePath;
    std::vector<uint8_t> _VSTiConfig;

    pfc::string8 _SoundFontFilePath;

#ifdef DXISUPPORT
    GUID dxi_plugin;
#endif

    // ADL
    uint32_t _ADLBankNumber;
    uint32_t _ADLChipCount;
    bool _ADLUsePanning;
    bool _ADLUseChorus;
    uint32_t _ADLEmulatorCore;

    // OPN
    uint32_t _OPNBankNumber;
    uint32_t _OPNEmulatorCore;

    // Munt
    uint32_t _MuntGMSet;

    // FluidSynth
    bool _FluidSynthEffectsEnabled;
    uint32_t _FluidSynthVoices;

    // BASS MIDI
    bool _BASSMIDIEffectsEnabled;
    uint32_t _BASSMIDIVoices;

    // Nuke
    uint32_t _NukeSynth;
    uint32_t _NukeBank;
    bool _NukeUsePanning;

    MIDIPlayer::ConfigurationType _ConfigurationType;
    bool _UseMIDIEffects;
    bool _UseSuperMuntWithMT32;
    bool _UseSecretSauceWithXG;
};
#pragma warning(default: 4820) // x bytes padding added after data member
