
/** $VER: Configuration.h (2023.09.27) **/

#pragma once

#include <sdk/foobar2000-lite.h>
#include <sdk/cfg_var.h>
#include <sdk/advconfig_impl.h>

#include "MIDIPlayer.h"

#include "ConfigurationMap.h"

#include <fluidsynth.h>

extern const GUID PreferencesPageGUID;
extern const GUID PreferencesPathsPageGUID;

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

    Max = SecretSauce,

    Default = ADL,
};

enum
{
    DefaultSampleRate = 44100,
    DefaultPlaybackLoopType = 0,
    DefaultOtherLoopType = 0,
    DefaultDecayTime = 1000,

    default_cfg_thloopz = 1,
    default_cfg_rpgmloopz = 1,
    default_cfg_xmiloopz = 1,
    default_cfg_ff7loopz = 1,

    DefaultMIDIFlavor = MIDIPlayer::ConfigurationType::None,
    DefaultUseMIDIEffects = 1,
    DefaultUseSuperMuntWithMT32 = 1,
    DefaultUseSecretSauceWithXG = 0,

    DefaultEmuDeMIDIExclusion = 1,
    DefaultFilterInstruments = 0,
    DefaultFilterBanks = 0,

    DefaultFluidSynthInterpolationMethod = FLUID_INTERP_DEFAULT,

    DefaultBASSMIDIInterpolationMode = 1,

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

enum LoopType
{
    NeverLoop = 0,                      // Never loop
    NeverLoopAddDecayTime = 1,          // Never loop, add configured decay time at the end

    LoopAndFadeWhenDetected = 2,        // Loop and fade when detected
    LoopAndFadeAlways = 3,              // Loop and fade always

    PlayIndefinitelyWhenDetected = 4,   // Play indefinitely when detected
    PlayIndefinitely = 5,               // Play indefinitely
};

extern cfg_int
    CfgPlayerType,
    CfgSampleRate,

    CfgLoopTypePlayback,
    CfgLoopTypeOther,
    CfgDecayTime,

    CfgDetectXMILoops,
    CfgDetectFF7Loops,
    CfgDetectRPGMakerLoops,
    CfgDetectTouhouLoops,

    CfgEmuDeMIDIExclusion,

    CfgFilterInstruments,
    CfgFilterBanks,

    CfgFluidSynthInterpolationMode,
    CfgBASSMIDIInterpolationMode,

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

extern cfg_string
    CfgVSTiFilePath,
    CfgSoundFontFilePath,
    CfgMT32ROMDirectoryPath,
    CfgFluidSynthDirectoryPath;

extern cfg_map
    CfgVSTiConfig;

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

extern const char TagChannels[];
extern const char TagEncoding[];

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

extern const char TagSampleRate[];
extern const char TagMIDIActiveVoices[];
extern const char TagMIDIPeakVoices[];

extern const char TagMIDIPlayer[];
extern const char TagExtraPercusionChannel[];

extern bool IsMIDIFileExtension(const char * fileExtension);
extern bool IsSysExFileExtension(const char * ext);
