
/** $VER: Configuration.h (2023.01.04) **/

#pragma once

#pragma warning(disable: 5045)

#include <sdk/foobar2000-lite.h>
#include <sdk/cfg_var.h>
#include <sdk/advconfig_impl.h>

#include "MIDIPlayer.h"

#include "cfg_map.h"

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

    default_cfg_emidi_exclusion = 1,

    default_cfg_filter_instruments = 0,
    default_cfg_filter_banks = 0,

//  default_cfg_recover_tracks = 0,

    DefaultResamplingMode = 1,

    DefaultGMSet = 0,

    DefaultADLBank = 72,
    DefaultADLChipCount = 10,
    DefaultADLPanning = 1,
//  DefaultADL4Op = 14,

    // MS Player
    DefaultMSSynth = 0,
    DefaultMSBank = 2,
    DefaultMSPanning = 0,

    DefaultMIDIFlavor = MIDIPlayer::filter_default,
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

    cfg_emidi_exclusion,

    cfg_filter_instruments,
    cfg_filter_banks,

    CfgResamplingMode,

    CfgADLBank,
    CfgADLChipCount,
    CfgADLPanning,

    CfgMUNTGMSet,

    CfgMSSynthesizer,
    CfgMSBank,
    CfgMSPanning,

    CfgMIDIFlavor,
    CfgAllowMIDIEffects
#ifdef FLUIDSYNTHSUPPORT
,
    Cfg_FluidSynthInterpolationMethod
#endif
;

extern cfg_string
    CfgVSTiPath,
    CfgSoundFontPath,
    CfgMUNTPath;

extern cfg_map
    CfgVSTiConfig;

extern advconfig_string_factory CfgVSTiSearchPath;

extern advconfig_string_factory_MT CfgSecretSaucePath;

extern advconfig_branch_factory cfg_midi_timing_parent;

extern advconfig_integer_factory CfgLoopCount;
extern advconfig_integer_factory CfgFadeTimeInMS;

extern advconfig_branch_factory cfg_adl_core_parent;

extern advconfig_checkbox_factory_t<true> cfg_adl_core_nuked;
extern advconfig_checkbox_factory_t<true> cfg_adl_core_nuked_174;
extern advconfig_checkbox_factory_t<true> CfgADLCoreDOSBox;

extern advconfig_branch_factory cfg_opn_core_parent;

extern advconfig_checkbox_factory_t<true> cfg_opn_core_mame;
extern advconfig_checkbox_factory_t<true> cfg_opn_core_nuked;
extern advconfig_checkbox_factory_t<true> cfg_opn_core_gens;

extern advconfig_branch_factory cfg_opn_bank_parent;

extern advconfig_checkbox_factory_t<true> cfg_opn_bank_xg;
extern advconfig_checkbox_factory_t<true> cfg_opn_bank_gs;
extern advconfig_checkbox_factory_t<true> cfg_opn_bank_gems;
extern advconfig_checkbox_factory_t<true> cfg_opn_bank_tomsoft;
extern advconfig_checkbox_factory_t<true> cfg_opn_bank_fmmidi;

extern advconfig_checkbox_factory cfg_SkipToFirstNote;

#ifdef BASSMIDISUPPORT
extern advconfig_checkbox_factory CfgBASSMIDIEffects;
extern advconfig_integer_factory CfgBASSMIDIVoices;
#endif

extern const char * _MUNTGMSets[];
extern const size_t _MUNTGMSetCount;

extern const char * _FileExtensions[];
extern const size_t _FileExtensionCount;

extern const char * _SysExFileExtensions[];
extern const size_t _SysExFileExtensionCount;

extern const char TagChannels[];
extern const char TagEncoding[];

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

extern bool IsMIDIFileExtension(const char * fileExtension);
extern bool IsSysExFileExtension(const char * ext);
