
/** $VER: Configuration.h (2025.06.09) **/

#pragma once

#include <sdk/foobar2000-lite.h>

#include <sdk/cfg_var.h>
#include <sdk/advconfig_impl.h>

#include "ConfigurationMap.h"

#include <fluidsynth.h>

using namespace cfg_var_modern;

extern const GUID PreferencesPageGUID;
extern const GUID PreferencesPathsPageGUID;
extern const GUID PreferencesProcessingPageGUID;
extern const GUID PreferencesHMIPageGUID;

enum class PlayerType : int8_t
{
    Unknown = -1,

    Min = 0,

    EmuDeMIDI = Min,
    VSTi = 1,
    FluidSynth = 2,
    SuperMunt = 3,
    BASSMIDI = 4,
    DirectX = 5,
    ADL = 6,
    OPN = 7,
    OPL = 8,
    Nuke = 9,
    SecretSauce = 10,
    MCI = 11,
    NukedSC55 = 12,

    Max = NukedSC55,

    Default = ADL,
};

enum class LoopType
{
    NeverLoop = 0,                      // Never loop
    NeverLoopAddDecayTime = 1,          // Never loop, add configured decay time at the end

    LoopAndFadeWhenDetected = 2,        // Loop and fade when detected
    LoopAndFadeAlways = 3,              // Loop and fade always

    PlayIndefinitelyWhenDetected = 4,   // Play indefinitely when detected
    PlayIndefinitely = 5,               // Play indefinitely
};

enum class MIDIFlavor
{
    Default = 0,                        // Defaults to SC88.

    GM,
    GM2,

    SC55,
    SC88,
    SC88Pro,
    SC8850,

    XG
};

enum
{
    DefaultSampleRate = 44100,
    DefaultPlaybackLoopType = 0,
    DefaultOtherLoopType = 0,
    DefaultDecayTime = 1000,

    DefaultDetectTouhouLoops = 1,
    DefaultDetectRPGMakerLoops = 1,
    DefaultDetectLeapFrogLoops = 0,
    DefaultDetectXMILoops = 1,
    DefaultDetectFF7Loops = 1,

    DefaultMIDIFlavor = MIDIFlavor::Default,
    DefaultUseMIDIEffects = 1,
    DefaultUseSuperMuntWithMT32 = 1,
    DefaultUseVSTiWithXG = 0,

    DefaultEmuDeMIDIExclusion = 1,
    DefaultFilterInstruments = 0,
    DefaultFilterBanks = 0,

    DefaultFluidSynthInterpolationMethod = FLUID_INTERP_DEFAULT,

    DefaultBASSMIDIResamplingMode = 1,

    DefaultGMSet = 0,

    // Munt
    DefaultNukeSynth = 0,
    DefaultNukeBank = 2,
    DefaultNukePanning = 0,

    DefaultADLBank = 72,
    DefaultADLChipCount = 10,
    DefaultADLPanning = 1,
//  DefaultADL4Op = 14,
};

const float DefaultBASSMIDIVolume = 0.15f;

const uint16_t DefaultEnabledChannels = 0xFFFF;

extern cfg_var_modern::cfg_int
    CfgPlayerType,
    CfgSampleRate,

    CfgLoopTypePlayback,
    CfgLoopTypeOther,
    CfgDecayTime,

    CfgDetectTouhouLoops,
    CfgDetectRPGMakerLoops,
    CfgDetectLeapFrogLoops,
    CfgDetectXMILoops,
    CfgDetectFF7Loops,

    CfgEmuDeMIDIExclusion,

    CfgFilterInstruments,
    CfgFilterBanks,

    CfgFluidSynthInterpolationMode,

    CfgBASSMIDIResamplingMode,

    CfgADLBank,
    CfgADLChipCount,
    CfgADLPanning,

    CfgMuntGMSet,

    CfgNukeSynthesizer,
    CfgNukeBank,
    CfgNukePanning,

    CfgMIDIStandard,
    CfgUseMIDIEffects,
    CfgUseSuperMuntWithMT32,
    CfgUseVSTiWithXG;

extern cfg_var_modern::cfg_bool
    CfgWriteBarMarkers,
    CfgWriteSysExNames,
    CfgExtendLoops,
    CfgWolfteamLoopMode,
    CfgKeepMutedChannels,
    CfgIncludeControlData;

extern cfg_var_modern::cfg_float
    CfgBASSMIDIVolume;

extern cfg_var_modern::cfg_string
    CfgVSTiFilePath,
    CfgSoundFontFilePath,
    CfgMT32ROMDirectoryPath,
    CfgFluidSynthDirectoryPath;

extern cfg_map
    CfgVSTiConfig;

// RCP
extern cfg_var_modern::cfg_int  CfgLoopExpansion;

// HMI
extern cfg_var_modern::cfg_int  CfgDefaultTempo;

// Channel Filtering
extern cfg_var_modern::cfg_blob CfgEnabledChannels;
extern cfg_var_modern::cfg_bool HaveEnabledChannelsChanged;

extern advconfig_string_factory AdvCfgVSTiPluginDirectoryPath;
extern advconfig_string_factory AdvCfgVSTiXGPlugin;
extern advconfig_string_factory_MT AdvCfgSecretSauceDirectoryPath;
extern advconfig_checkbox_factory AdvCfgSkipToFirstNote;

extern advconfig_integer_factory AdvCfgLoopCount;
extern advconfig_integer_factory AdvCfgFadeTimeInMS;

extern advconfig_radio_factory AdvCfgADLCoreNuked;
extern advconfig_radio_factory AdvCfgADLCoreNuked174;
extern advconfig_radio_factory AdvCfgADLCoreDOSBox;
extern advconfig_radio_factory AdvCfgADLCoreOpal;
extern advconfig_radio_factory AdvCfgADLCoreJava;

extern advconfig_string_factory AdvCfgADLBankFilePath;

extern advconfig_radio_factory AdvCfgOPNCoreMAME;
extern advconfig_radio_factory AdvCfgOPNCoreNuked;
extern advconfig_radio_factory AdvCfgOPNCoreGens;

extern advconfig_radio_factory AdvCfgOPNBankXG;
extern advconfig_radio_factory AdvCfgOPNBankGS;
extern advconfig_radio_factory AdvCfgOPNBankGEMS;
extern advconfig_radio_factory AdvCfgOPNBankTomSoft;
extern advconfig_radio_factory AdvCfgOPNBankFMMIDI;

extern advconfig_integer_factory AdvCfgFluidSynthVoices;
extern advconfig_checkbox_factory AdvCfgFluidSynthEffectsEnabled;
extern advconfig_checkbox_factory AdvCfgLoadSoundFontDynamically;

extern advconfig_checkbox_factory AdvCfgBASSMIDIEffectsEnabled;
extern advconfig_integer_factory AdvCfgBASSMIDIVoices;

extern const char * _MuntGMSets[];
extern const size_t _MuntGMSetCount;

extern const char * _FileExtensions[];
extern const size_t _FileExtensionCount;

extern const char * _SysExFileExtensions[];
extern const size_t _SysExFileExtensionCount;

extern const char TagMIDIFormat[];
extern const char TagMIDITrackCount[];
extern const char TagMIDIChannelCount[];
extern const char TagMIDITicks[];
extern const char TagMIDIType[];
extern const char TagMIDILoopStart[];
extern const char TagMIDILoopEnd[];
extern const char TagMIDILoopStartInMs[];
extern const char TagMIDILoopEndInMs[];
extern const char TagMIDIPreset[];
extern const char TagMIDISysExDumps[];
extern const char TagMIDILyricsType[];
extern const char TagMIDIHash[];
extern const char TagMIDIEmbeddedSoundFont[];

extern const char TagSampleRate[];
extern const char TagMIDIActiveVoices[];
extern const char TagMIDIPeakVoices[];

extern const char TagMIDIPlayer[];
extern const char TagMIDIExtraPercusionChannel[];

extern bool IsMIDIFileExtension(const char * fileExtension);
extern bool IsSysExFileExtension(const char * ext);
