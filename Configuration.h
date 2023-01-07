
/** $VER: Configuration.h (2023.01.06) **/

#pragma once

#pragma warning(disable: 5045)

#include <sdk/foobar2000-lite.h>
#include <sdk/cfg_var.h>
#include <sdk/advconfig_impl.h>

#include "MIDIPlayer.h"

#include "cfg_map.h"

extern const GUID PreferencesPageGUID;
extern const GUID PreferencesPathsPageGUID;

enum
{
    PlayerTypeEmuDeMIDI = 0,
    PlayerTypeVSTi = 1,
    PlayerTypeFluidSynth = 2,
    PlayerTypeSuperMunt = 3,
    PlayerTypeBASSMIDI = 4,
    PlayerTypeDirectX = 5,
    PlayerTypeADL = 6,
    PlayerTypeOPN = 7,
    PlayerTypeOPL = 8,
    PlayerTypeNuke = 9,
    PlayerTypeSecretSauce = 10,
};

enum
{
    DefaultPlayerType = PlayerTypeADL,
    DefaultSampleRate = 44100,
    DefaultPlaybackLoopType = 0,
    DefaultOtherLoopType = 0,

    default_cfg_thloopz = 1,
    default_cfg_rpgmloopz = 1,
    default_cfg_xmiloopz = 1,
    default_cfg_ff7loopz = 1,

    DefaultEmuDeMIDIExclusion = 1,

    default_cfg_filter_instruments = 0,
    default_cfg_filter_banks = 0,

//  default_cfg_recover_tracks = 0,

    DefaultResamplingMode = 1,

    DefaultGMSet = 0,

    DefaultADLBank = 72,
    DefaultADLChipCount = 10,
    DefaultADLPanning = 1,
//  DefaultADL4Op = 14,

    // Munt
    DefaultNukeSynth = 0,
    DefaultNukeBank = 2,
    DefaultNukePanning = 0,

    DefaultMIDIFlavor = MIDIPlayer::FilterNone,
    DefaultMIDIEffects = 1,
#ifdef FLUIDSYNTHSUPPORT
    DefaultFluidSynthInterpolationMethod = FLUID_INTERP_DEFAULT
#endif
};

extern cfg_int
    CfgPlayerType,
    CfgSampleRate,

    CfgLoopTypePlayback,
    CfgLoopTypeOther,

    cfg_thloopz,
    cfg_rpgmloopz,
    cfg_xmiloopz,
    cfg_ff7loopz,

    CfgEmuDeMIDIExclusion,

    CfgFilterInstruments,
    CfgFilterBanks,

    CfgResamplingMode,

    CfgADLBank,
    CfgADLChipCount,
    CfgADLPanning,

    CfgMuntGMSet,

    CfgNukeSynthesizer,
    CfgNukeBank,
    CfgNukePanning,

    CfgMIDIFlavor,
    CfgAllowMIDIEffects
#ifdef FLUIDSYNTHSUPPORT
,
    Cfg_FluidSynthInterpolationMethod
#endif
;

extern cfg_string
    CfgVSTiFilePath,
    CfgSoundFontFilePath,
    CfgMuntDirectoryPath;

extern cfg_map
    CfgVSTiConfig;

extern advconfig_string_factory AdvCfgVSTiPluginDirectoryPath;

extern advconfig_string_factory_MT AdvCfgSecretSaucePath;

extern advconfig_branch_factory CfgMIDITimingBranch;

extern advconfig_integer_factory AdvCfgLoopCount;
extern advconfig_integer_factory AdvCfgFadeTimeInMS;

extern advconfig_branch_factory AdvCfgADLCoreBranch;

    extern advconfig_radio_factory AdvCfgADLCoreNuked;
    extern advconfig_radio_factory AdvCfgADLCoreNuked074;
    extern advconfig_radio_factory AdvCfgADLCoreDOSBox;

extern advconfig_branch_factory AdvCfgOPNCoreBranch;

    extern advconfig_radio_factory AdvCfgOPNCoreMAME;
    extern advconfig_radio_factory AdvCfgOPNCoreNuked;
    extern advconfig_radio_factory AdvCfgOPNCoreGens;

extern advconfig_branch_factory AdvCfgOPNBankBranch;

    extern advconfig_radio_factory AdvCfgOPNBankXG;
    extern advconfig_radio_factory AdvCfgOPNBankGS;
    extern advconfig_radio_factory AdvCfgOPNBankGEMS;
    extern advconfig_radio_factory AdvCfgOPNBankTomSoft;
    extern advconfig_radio_factory AdvCfgOPNBankFMMIDI;

extern advconfig_checkbox_factory AdvCfgSkipToFirstNote;

#ifdef BASSMIDISUPPORT
extern advconfig_checkbox_factory AdvCfgBASSMIDIEffectsEnabled;
extern advconfig_integer_factory AdvCfgBASSMIDIVoices;
#endif

extern const char * _MuntGMSets[];
extern const size_t _MuntGMSetCount;

extern const char * _FileExtensions[];
extern const size_t _FileExtensionCount;

extern const char * _SysExFileExtensions[];
extern const size_t _SysExFileExtensionCount;

extern const char TagChannels[];
extern const char TagEncoding[];
extern const char TagSampleRate[];

extern const char TagMIDIHash[];
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

extern const char TagBASSMIDIVoiceCount[];
extern const char TagBASSMIDIVoicesMax[];

extern bool IsMIDIFileExtension(const char * fileExtension);
extern bool IsSysExFileExtension(const char * ext);
