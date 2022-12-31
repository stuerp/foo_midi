
/** $VER: Configuration.h (2022.12.31) **/

#pragma once

#include "MIDIPlayer.h"

#include "ConfigurationMap.h"

enum
{
    EmuDeMIDIPluginId = 0,

    FluidSynthPluginId = 2,
    SuperMUNTPluginId = 3,
    BASSMIDIPlugInId = 4,

    ADLPluginId = 6,
    OPNPluginId = 7,
    OPLPluginId = 8,
    NuclearOptionPluginId = 9,
    SecretSaucePluginId = 10,
};

enum
{
    DefaultPluginId = ADLPluginId,

    default_cfg_thloopz = 1,
    default_cfg_rpgmloopz = 1,
    default_cfg_xmiloopz = 1,
    default_cfg_ff7loopz = 1,
    default_cfg_emidi_exclusion = 1,
    default_cfg_filter_instruments = 0,
    default_cfg_filter_banks = 0,
//  default_cfg_recover_tracks = 0,
    default_cfg_loop_type = 0,
    default_cfg_loop_type_other = 0,
    default_cfg_srate = 44100,
    default_cfg_resampling = 1,
    default_cfg_adl_bank = 72,
    default_cfg_adl_chips = 10,
    default_cfg_adl_panning = 1,
//  default_cfg_adl_4op = 14,
    default_cfg_munt_gm = 0,

    // MS Player
    DefaultMSSynth = 0,
    DefaultMSBank = 2,

    default_cfg_midi_flavor = MIDIPlayer::filter_default,
    default_cfg_ms_panning = 0,
    default_cfg_midi_reverb = 1,
#ifdef FLUIDSYNTHSUPPORT
    default_cfg_fluid_interp_method = FLUID_INTERP_DEFAULT
#endif
};

extern cfg_int
    cfg_thloopz,
    cfg_rpgmloopz,
    cfg_xmiloopz,
    cfg_ff7loopz,

    cfg_emidi_exclusion,

    cfg_filter_instruments,
    cfg_filter_banks,

    cfg_loop_type,
    cfg_loop_type_other,

    cfg_srate,
    CfgPluginId,
    cfg_resampling,

    cfg_adl_bank,
    cfg_adl_chips,
    cfg_adl_panning,

    cfg_munt_gm,

    cfg_ms_synth,
    cfg_ms_bank,
    cfg_ms_panning,

    cfg_midi_flavor,
    cfg_midi_reverb
#ifdef FLUIDSYNTHSUPPORT
,
    cfg_fluid_interp_method
#endif
;

extern cfg_string
    cfg_vst_path,
    cfg_soundfont_path,
    cfg_munt_base_path;

extern cfg_map
    cfg_vst_config;

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
#endif

