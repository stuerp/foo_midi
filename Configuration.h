
/** $VER: Configuration.h (2022.12.31) **/

#pragma once

#include "MIDIPlayer.h"

#include "ConfigurationMap.h"

enum
{
    EmuDeMIDIPlugInId = 0,
    VSTiPlugInId = 1,
    FluidSynthPlugInId = 2,
    SuperMUNTPlugInId = 3,
    BASSMIDIPlugInId = 4,
    UnknownPlugInId = 5,
    ADLPlugInId = 6,
    OPNPlugInId = 7,
    OPLPlugInId = 8,
    NuclearOptionPlugInId = 9,
    SecretSaucePlugInId = 10,
};

enum
{
    DefaultPlugInId = ADLPlugInId,
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

    default_cfg_munt_gm = 0,

    DefaultADLBank = 72,
    DefaultADLChipCount = 10,
    DefaultADLPanning = 1,
//  DefaultADL4Op = 14,

    // MS Player
    DefaultMSSynth = 0,
    DefaultMSBank = 2,
    DefaultMSPanning = 0,

    default_cfg_midi_flavor = MIDIPlayer::filter_default,
    default_cfg_midi_reverb = 1,
#ifdef FLUIDSYNTHSUPPORT
    default_cfg_fluid_interp_method = FLUID_INTERP_DEFAULT
#endif
};

extern cfg_int
    CfgPlugInId,
    CfgSampleRate,

    cfg_thloopz,
    cfg_rpgmloopz,
    cfg_xmiloopz,
    cfg_ff7loopz,

    cfg_emidi_exclusion,

    cfg_filter_instruments,
    cfg_filter_banks,

    cfg_loop_type,
    cfg_loop_type_other,

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

extern advconfig_integer_factory cfg_midi_loop_count;
extern advconfig_integer_factory cfg_midi_fade_time;

extern advconfig_branch_factory cfg_adl_core_parent;

extern advconfig_checkbox_factory_t<true> cfg_adl_core_nuked;
extern advconfig_checkbox_factory_t<true> cfg_adl_core_nuked_174;
extern advconfig_checkbox_factory_t<true> cfg_adl_core_dosbox;

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
extern advconfig_checkbox_factory cfg_bassmidi_effects;
extern advconfig_integer_factory cfg_bassmidi_voices;

extern const bool _HASSSE2;
#endif

extern const char * _MUNTGMSets[];
extern const size_t _MUNTGMSetCount;

extern const char * _FileExtensions[];
extern const size_t _FileExtensionCount;

extern const char * _SyxExtension[];
extern const size_t _SyxExtensionCount;

extern const char field_hash[];
extern const char field_format[];
extern const char field_tracks[];
extern const char field_channels[];
extern const char field_ticks[];
extern const char field_type[];
extern const char field_loop_start[];
extern const char field_loop_end[];
extern const char field_loop_start_ms[];
extern const char field_loop_end_ms[];
extern const char field_preset[];
extern const char field_syx[];

extern bool IsFileExtensionSupported(const char * fileExtension);
extern bool g_test_extension_syx(const char * ext);

