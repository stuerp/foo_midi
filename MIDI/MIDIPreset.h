
/** $VER: MIDIPreset.h (2023.09.27) **/

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

    MIDIPreset(const MIDIPreset &) = delete;
    MIDIPreset(MIDIPreset &&) = delete;
    MIDIPreset & operator=(const MIDIPreset &) = delete;
    MIDIPreset & operator=(MIDIPreset &&) = delete;

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
    pfc::string8 _ADLBankFilePath;

    // OPN
    uint32_t _OPNBankNumber;
    uint32_t _OPNEmulatorCore;

    // Munt
    uint32_t _MuntGMSet;

    // FluidSynth / BASS MIDI
    bool _EffectsEnabled;
    uint32_t _VoiceCount;

    // Nuke
    uint32_t _NukeSynth;
    uint32_t _NukeBank;
    bool _NukeUsePanning;

    MIDIFlavor _MIDIFlavor;
    bool _UseMIDIEffects;
    bool _UseSuperMuntWithMT32;
    bool _UseVSTiWithXG;
};
#pragma warning(default: 4820) // x bytes padding added after data member
