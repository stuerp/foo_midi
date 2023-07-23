
/** $VER: MIDIPreset.h (2023.07.22) **/

#pragma once

#include <pfc/pfc-lite.h>

#include "ADLPlayer.h"
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

    // version 0
    uint32_t _PlayerType;

    // v0 - plug-in == 1 - VSTi
    pfc::string8 _VSTiFilePath;
    std::vector<uint8_t> _VSTiConfig;

    // v0 - plug-in == 2/4 - SoundFont synthesizer
    pfc::string8 _SoundFontFilePath;

#ifdef DXISUPPORT
    // v0 - plug-in == 5 - DXi
    GUID dxi_plugin;
#endif

 #pragma region("ADL")
    // v0 - plug-in == 6 - adlmidi
    // v0 - plug-in == 8 - oplmidi
    uint32_t _ADLBankNumber;

    // v0 - plug-in == 6 - adlmidi
    uint32_t _ADLChipCount;
    bool _ADLUsePanning;
    // v3 - chorus
    bool _ADLUseChorus;
    // v7 - emulator core
    uint32_t _ADLEmulatorCore;
#pragma endregion

 #pragma region("OPN")
    // v10 - plug-in == 7 - libOPNMIDI (previously FMMIDI)
    uint32_t _OPNBankNumber; // hard coded to fmmidi for previous versions
    uint32_t _OPNEmulatorCore;
#pragma endregion

 #pragma region("Munt")
    // v1 - plug-in == 3 - Munt
    uint32_t _MuntGMSet;
#pragma endregion

 #pragma region("BASS MIDI")
    // v2 - plug-in == 2/4 - SoundFont synthesizer
    bool _BASSMIDIEffectsEnabled;
    // v9 - plug-in == 2/4 - Maximum voices
    uint32_t _BASSMIDIVoices;
#pragma endregion

 #pragma region("Nuke")
    // v4 - plug-in == 9 - Nuke
    uint32_t _NukeSynth;
    uint32_t _NukeBank;

    // v6 - panning
    bool _NukeUsePanning;
#pragma endregion

    // v5 - plug-in == 10 - Secret Sauce
// uint32_t sc_flavor;

    // v6 - reverb
//  bool sc_reverb;

    // v8 - GS flavor, also sc_flavor has new values
//  uint32_t gs_flavor;

    // v11: Most players
    uint32_t _MIDIStandard;
    bool _UseMIDIEffects;

    // v11: Most players
    bool _UseSuperMuntWithMT32;
    bool _UseSecretSauceWithXG;
};
#pragma warning(default: 4820) // x bytes padding added after data member
