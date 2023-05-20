
/** $VER: MIDIPreset.h (2022.01.08) **/

#pragma once

#pragma warning(disable: 5045)

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
    const unsigned int CurrentSchemaVersion = 11;

    // version 0
    unsigned int _PlayerType;

    // v0 - plug-in == 1 - VSTi
    pfc::string8 _VSTiFilePath;
    std::vector<uint8_t> _VSTConfig;

    // v0 - plug-in == 2/4 - SoundFont synthesizer
    pfc::string8 _SoundFontFilePath;

#ifdef DXISUPPORT
    // v0 - plug-in == 5 - DXi
    GUID dxi_plugin;
#endif

 #pragma region("ADL")
    // v0 - plug-in == 6 - adlmidi
    // v0 - plug-in == 8 - oplmidi
    unsigned int _ADLBankNumber;

    // v0 - plug-in == 6 - adlmidi
    unsigned int _ADLChipCount;
    bool _ADLUsePanning;
    // v3 - chorus
    bool _ADLChorus;
    // v7 - emulator core
    unsigned int _ADLEmulatorCore;
#pragma endregion

 #pragma region("OPN")
    // v10 - plug-in == 7 - libOPNMIDI (previously FMMIDI)
    unsigned int _OPNBankNumber; // hard coded to fmmidi for previous versions
    unsigned int _OPNEmulatorCore;
#pragma endregion

 #pragma region("Munt")
    // v1 - plug-in == 3 - Munt
    unsigned int _MuntGMSet;
#pragma endregion

 #pragma region("BASS MIDI")
    // v2 - plug-in == 2/4 - SoundFont synthesizer
    bool _BASSMIDIEffects;
    // v9 - plug-in == 2/4 - Maximum voices
    unsigned int _BASSMIDIVoices;
#pragma endregion

 #pragma region("Nuke")
    // v4 - plug-in == 9 - Nuke
    unsigned int _NukeSynth;
    unsigned int _NukeBank;
    // v6 - panning
    bool _NukePanning;
#pragma endregion

    // v5 - plug-in == 10 - Secret Sauce
// unsigned int sc_flavor;

    // v6 - reverb
//  bool sc_reverb;

    // v8 - GS flavor, also sc_flavor has new values
//  unsigned int gs_flavor;

    // v11 - most plugins
    uint32_t _MIDIStandard;
    bool _UseMIDIEffects;
};
#pragma warning(default: 4820) // x bytes padding added after data member
