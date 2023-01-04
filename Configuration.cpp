
/** $VER: Configuration.cpp (2023.01.04) **/

#pragma warning(disable: 5045 26481 26485)

#include "Configuration.h"

#include "resource.h"

#pragma region("GUIDs")
static const GUID GUIDPlugInId = { 0x1253bac2, 0x9193, 0x420c, { 0xa9, 0x19, 0x9a, 0x1c, 0xf8, 0x70, 0x6e, 0x2c } };
static const GUID GUIDSampleRate = { 0xae5ba73b, 0xb0d4, 0x4261, { 0xbf, 0xf2, 0x11, 0xa1, 0xc4, 0x4e, 0x57, 0xea } };

static const GUID guid_cfg_loop_type = { 0x460a84b6, 0x910a, 0x496c, { 0xbe, 0xb6, 0x86, 0xfd, 0xeb, 0x41, 0xab, 0xdc } };
static const GUID guid_cfg_loop_type_other = { 0xab5cc279, 0x1c68, 0x4824, { 0xb4, 0xb8, 0x6, 0x56, 0x85, 0x6a, 0x40, 0xa0 } };

static const GUID guid_cfg_thloopz = { 0x35a32d9e, 0xdf99, 0x4617, { 0x9f, 0xf3, 0x31, 0x2f, 0x9c, 0x20, 0x6e, 0x0 } };
static const GUID guid_cfg_rpgmloopz = { 0x4d11dd87, 0x7a27, 0x4ecc, { 0xbc, 0xb8, 0xf0, 0x18, 0x2, 0xb, 0xa2, 0xd5 } };
static const GUID guid_cfg_xmiloopz = { 0xf580d09, 0xd57b, 0x450c, { 0x84, 0xa2, 0xd6, 0xe, 0x34, 0xbd, 0x64, 0xf5 } };
static const GUID guid_cfg_ff7loopz = { 0x2e0dbdc2, 0x7436, 0x4b70, { 0x91, 0xfc, 0xfd, 0x98, 0x37, 0x87, 0x32, 0xb2 } };

static const GUID guid_cfg_emidi_exclusion = { 0xc090f9c7, 0x47f9, 0x4f6f, { 0x84, 0x7a, 0x27, 0xcd, 0x75, 0x96, 0xc9, 0xd4 } };

static const GUID guid_cfg_filter_instruments = { 0x6d30c919, 0xb053, 0x43aa, { 0x9f, 0x1b, 0x1d, 0x40, 0x18, 0x82, 0x80, 0x5e } };
static const GUID guid_cfg_filter_banks = { 0x3145963c, 0x7322, 0x4b48, { 0x99, 0xff, 0x75, 0xea, 0xc5, 0xf4, 0xda, 0xcc } };

static const GUID GUIDResamplingMode = { 0xf9ddd2c0, 0xd8fd, 0x442f, { 0x9e, 0x49, 0xd9, 0x1, 0xb5, 0x1d, 0x6d, 0x38 } };

static const GUID guid_cfg_vst_path = { 0x1a6ea7e5, 0x718a, 0x485a, { 0xb1, 0x67, 0xcf, 0xdf, 0x3b, 0x40, 0x61, 0x45 } };
static const GUID guid_cfg_soundfont_path = { 0x696d12dd, 0xaf32, 0x43d9, { 0x8d, 0xf6, 0xbd, 0xd1, 0x1e, 0x81, 0x83, 0x29 } };
static const GUID guid_cfg_munt_base_path = { 0xd7e0ec5e, 0x872f, 0x41e3, { 0x9b, 0x5b, 0xd2, 0x2, 0xd8, 0xb9, 0x42, 0xa7 } };

#ifdef FLUIDSYNTHSUPPORT
static const GUID guid_cfg_fluid_interp_method = { 0xa395c6fd, 0x492a, 0x401b, { 0x8b, 0xdb, 0x9d, 0xf5, 0x3e, 0x2e, 0xf7, 0xcf } };
#endif

static const GUID guid_cfg_vst_config = { 0x44e7c715, 0xd256, 0x44c4, { 0x8f, 0xb6, 0xb7, 0x20, 0xfa, 0x9b, 0x31, 0xfc } };

#ifdef DXISUPPORT
static const GUID guid_cfg_dxi_plugin = { 0xd5c87282, 0xa9e6, 0x40f3, { 0x93, 0x82, 0x95, 0x68, 0xe6, 0x54, 0x1a, 0x46 } };
#endif

// Advanced Configuration
static const GUID GUIDMIDIPlayerBranch = { 0x66524470, 0x7ec7, 0x445e, { 0xa6, 0xfd, 0xc0, 0xfb, 0xae, 0x74, 0xe5, 0xfc } };

static const GUID GUIDVSTiSearchPath = { 0xbb4c61a1, 0x3c4, 0x4b62, { 0xb0, 0x4d, 0x2c, 0x86, 0xce, 0xde, 0x0, 0x5d } };
static const GUID GUIDSCCorePath = { 0x1bf1799d, 0x7691, 0x4075, { 0x98, 0xae, 0x43, 0xae, 0x82, 0xd8, 0xc9, 0xcf } };

static const GUID guid_cfg_midi_timing_parent = { 0x851583f7, 0x98b4, 0x44c7, { 0x9d, 0xf4, 0x4c, 0x7f, 0x85, 0x9d, 0x13, 0xba } };
static const GUID guid_cfg_midi_loop_count = { 0xd8492ad0, 0x3b70, 0x4768, { 0x8d, 0x7, 0x97, 0xf5, 0x50, 0x8c, 0x8, 0xe8 } };
static const GUID guid_cfg_midi_fade_time = { 0x1cc76581, 0x6fc8, 0x445e, { 0x9e, 0x3d, 0x2, 0x0, 0x43, 0xd9, 0x8b, 0x65 } };

static const GUID guid_cfg_adl_bank = { 0xa62a00a7, 0xdbf, 0x4475, { 0xbe, 0xca, 0xed, 0xbf, 0x5d, 0x6, 0x4a, 0x80 } };
static const GUID guid_cfg_adl_chips = { 0x974365ed, 0xd4f9, 0x4daa, { 0xb4, 0x89, 0xad, 0x7a, 0xd2, 0x91, 0xfa, 0x94 } };
static const GUID guid_cfg_adl_panning = { 0xad6821b4, 0x493f, 0x4bb3, { 0xb7, 0xbb, 0xe0, 0xa6, 0x7c, 0x5d, 0x59, 0x7 } };
//static const GUID guid_cfg_adl_4op = { 0xc5fb4053, 0x75bf, 0x4c0d, { 0xa1, 0xb1, 0x71, 0x73, 0x86, 0x32, 0x88, 0xa6 } };

static const GUID guid_cfg_munt_gm = { 0x7257ac7, 0x9901, 0x4a5f, { 0x9d, 0x8b, 0xc5, 0xb5, 0xf1, 0xb8, 0xcf, 0x5b } };

#ifdef BASSMIDISUPPORT
static const GUID guid_cfg_bassmidi_effects = { 0x62bf901b, 0x9c51, 0x45fe, { 0xbe, 0x8a, 0x14, 0xfb, 0x56, 0x20, 0x5e, 0x5e } };   // {62BF901B-9C51-45FE-BE8A-14FB56205E5E}
#endif
static const GUID guid_cfg_skip_to_first_note = { 0xf90c8abf, 0x68b5, 0x474a, { 0x8d, 0x9c, 0xff, 0xd9, 0xca, 0x80, 0x20, 0x2f } };

static const GUID guid_cfg_ms_synth = { 0x7423a720, 0xeb39, 0x4d7d, { 0x9b, 0x85, 0x52, 0x4b, 0xc7, 0x79, 0xb5, 0x8b } };
static const GUID guid_cfg_ms_bank = { 0xa91d31f4, 0x22ae, 0x4c5c, { 0xa6, 0x21, 0xf6, 0xb6, 0x1, 0x1f, 0x5d, 0xdc } };
static const GUID guid_cfg_ms_panning = { 0x849c5c09, 0x520a, 0x4d62, { 0xa6, 0xd1, 0xe8, 0xb4, 0x32, 0x66, 0x49, 0x48 } };

static const GUID guid_cfg_adl_core_parent = { 0x715c6e5d, 0x60bf, 0x43aa, { 0x8d, 0xa3, 0xf4, 0xf3, 0xb, 0x6, 0xff, 0x48 } };
static const GUID guid_cfg_adl_core_nuked = { 0x6b2c372, 0x2d86, 0x4368, { 0xb9, 0xd1, 0xfc, 0xb, 0xc8, 0x99, 0x38, 0xb1 } };
static const GUID guid_cfg_adl_core_nuked_174 = { 0x68252066, 0x2a7d, 0x4d74, { 0xb7, 0xc4, 0xd6, 0x9b, 0x1d, 0x67, 0x68, 0xd1 } };
static const GUID guid_cfg_adl_core_dosbox = { 0x2a0290f8, 0x805b, 0x4109, { 0xaa, 0xd3, 0xd5, 0xae, 0x7f, 0x62, 0x35, 0xc7 } };

#ifdef BASSMIDISUPPORT
static const GUID guid_cfg_bassmidi_parent = { 0xdd5adceb, 0x9b31, 0x47b6, { 0xaf, 0x57, 0x3b, 0x15, 0xd2, 0x2, 0x5d, 0x9f } };
static const GUID guid_cfg_bassmidi_voices = { 0x9e0a5dab, 0x6786, 0x4120, { 0xb7, 0x37, 0x85, 0xbb, 0x2d, 0xfa, 0xf3, 0x7 } };
#endif

static const GUID guid_cfg_opn_core_parent = { 0x5223b5bc, 0x41e8, 0x4d5d, { 0x83, 0x1f, 0x47, 0x7d, 0x9f, 0x8f, 0x31, 0x89 } };
static const GUID guid_cfg_opn_core_mame = { 0xc5617b26, 0xf011, 0x4674, { 0xb8, 0x5d, 0x12, 0xda, 0x2d, 0xa9, 0xd0, 0xdf } };
static const GUID guid_cfg_opn_core_nuked = { 0x8abbad90, 0x4e76, 0x4dd7, { 0x87, 0x77, 0x2b, 0x7e, 0xe7, 0xe9, 0x69, 0x53 } };
static const GUID guid_cfg_opn_core_gens = { 0xa74c885, 0xe917, 0x40cd, { 0x9a, 0x49, 0x52, 0xa7, 0x1b, 0x93, 0x7b, 0x8a } };
static const GUID guid_cfg_opn_bank_parent = { 0x7f53d374, 0x9731, 0x4321, { 0x92, 0x2b, 0x1, 0x46, 0x3f, 0x19, 0x72, 0x96 } };
static const GUID guid_cfg_opn_bank_xg = { 0x6a6f56a9, 0x513b, 0x4aad, { 0x90, 0x29, 0x32, 0x46, 0xec, 0xf0, 0x2d, 0x93 } };
static const GUID guid_cfg_opn_bank_gs = { 0xed7876ab, 0xfca3, 0x4dfd, { 0xaf, 0x56, 0x2, 0x8d, 0xd9, 0xbf, 0x54, 0x80 } };
static const GUID guid_cfg_opn_bank_gems = { 0x792b2366, 0x1768, 0x4f30, { 0x87, 0xc0, 0xc2, 0xea, 0xb5, 0xe8, 0x22, 0xd2 } };
static const GUID guid_cfg_opn_bank_tomsoft = { 0xad75fc74, 0xc8d6, 0x4399, { 0x89, 0xf2, 0xeb, 0x7f, 0xf0, 0x62, 0x33, 0xfe } };
static const GUID guid_cfg_opn_bank_fmmidi = { 0x47e69508, 0x2cb7, 0x4e32, { 0x83, 0x13, 0x15, 0x1a, 0x5f, 0x5a, 0xc7, 0x79 } };


#ifdef FLUIDSYNTHSUPPORT
static const GUID guid_cfg_soundfont_dynamic = { 0x4c455226, 0xb107, 0x4e04, { 0xa9, 0xec, 0xf8, 0x9, 0x8f, 0x81, 0xe2, 0x96 } };

static const GUID guid_cfg_fluidsynth_parent = { 0xf1ad51c5, 0x4b04, 0x4c8b, { 0x84, 0x65, 0x6c, 0x86, 0x1e, 0x81, 0xc6, 0x69 } };
static const GUID guid_cfg_fluidsynth_effects = { 0x996e95ca, 0xce4d, 0x4bd5, { 0xb7, 0xe6, 0x40, 0x61, 0x32, 0x83, 0xc3, 0x27 } };
static const GUID guid_cfg_fluidsynth_voices = { 0x9114d64d, 0x412c, 0x42d3, { 0xae, 0xd5, 0xa5, 0x52, 0x1e, 0x8f, 0xe2, 0xa6 } };
#endif

static const GUID guid_cfg_midi_flavor = { 0x1a82a8db, 0x389e, 0x44aa, { 0x97, 0x19, 0x32, 0x6a, 0x5a, 0x2d, 0x7e, 0x8e } };
static const GUID guid_cfg_midi_reverb = { 0x91c12a1, 0xd42b, 0x4f4e, { 0x80, 0x58, 0x8b, 0x7f, 0x4c, 0x4d, 0xf3, 0xa1 } };
#pragma endregion

cfg_int
    CfgPlayerType(GUIDPlugInId, DefaultPlayerType),
    CfgSampleRate(GUIDSampleRate, DefaultSampleRate),

    CfgLoopTypePlayback(guid_cfg_loop_type, DefaultPlaybackLoopType),
    CfgLoopTypeOther(guid_cfg_loop_type_other, DefaultOtherLoopType),

    cfg_thloopz(guid_cfg_thloopz, default_cfg_thloopz),
    cfg_rpgmloopz(guid_cfg_rpgmloopz, default_cfg_rpgmloopz),
    cfg_xmiloopz(guid_cfg_xmiloopz, default_cfg_xmiloopz),
    cfg_ff7loopz(guid_cfg_ff7loopz, default_cfg_ff7loopz),

    cfg_emidi_exclusion(guid_cfg_emidi_exclusion, default_cfg_emidi_exclusion),

    CfgResamplingMode(GUIDResamplingMode, DefaultResamplingMode),

    cfg_filter_instruments(guid_cfg_filter_instruments, default_cfg_filter_instruments),
    cfg_filter_banks(guid_cfg_filter_banks, default_cfg_filter_banks),

    CfgADLBank(guid_cfg_adl_bank, DefaultADLBank),
    CfgADLChipCount(guid_cfg_adl_chips, DefaultADLChipCount),
    CfgADLPanning(guid_cfg_adl_panning, DefaultADLPanning),
//  CfgADL4Op(guid_cfg_adl_4op, DefaultADL4Op),

    CfgMuntGMSet(guid_cfg_munt_gm, DefaultGMSet),

    CfgMSSynthesizer(guid_cfg_ms_synth, DefaultMSSynth),
    CfgMSBank(guid_cfg_ms_bank, DefaultMSBank),
    CfgMSPanning(guid_cfg_ms_panning, DefaultMSPanning),

    CfgMIDIFlavor(guid_cfg_midi_flavor, DefaultMIDIFlavor),
    CfgAllowMIDIEffects(guid_cfg_midi_reverb, DefaultMIDIEffects)

#ifdef FLUIDSYNTHSUPPORT
,
    Cfg_FluidSynthInterpolationMethod(guid_cfg_fluid_interp_method, DefaultFluidSynthInterpolationMethod)
#endif
;

#ifdef DXISUPPORT
static const GUID default_cfg_dxi_plugin = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };

cfg_guid cfg_dxi_plugin(guid_cfg_dxi_plugin, default_cfg_dxi_plugin);
#endif

cfg_string
    CfgVSTiPath(guid_cfg_vst_path, ""),
    CfgSoundFontPath(guid_cfg_soundfont_path, ""),
    CfgMuntPath(guid_cfg_munt_base_path, "");

cfg_map CfgVSTiConfig(guid_cfg_vst_config);

/** Advanced Configuration **/

advconfig_branch_factory CfgMIDIPlayer(STR_COMPONENT_NAME, GUIDMIDIPlayerBranch, advconfig_branch::guid_branch_playback, 0);

advconfig_string_factory CfgVSTiSearchPath("VSTi search path", GUIDVSTiSearchPath, GUIDMIDIPlayerBranch, 0, "");

advconfig_string_factory_MT CfgSecretSaucePath("Secret Sauce path", GUIDSCCorePath, GUIDMIDIPlayerBranch, 0, "");

advconfig_branch_factory cfg_midi_timing_parent("Playback timing when loops present", guid_cfg_midi_timing_parent, GUIDMIDIPlayerBranch, 1.0);

advconfig_integer_factory CfgLoopCount("Loop count", guid_cfg_midi_loop_count, guid_cfg_midi_timing_parent, 0, 2, 1, 10);
advconfig_integer_factory CfgFadeTimeInMS("Fade time (ms)", guid_cfg_midi_fade_time, guid_cfg_midi_timing_parent, 1, 5000, 0, 30000);

advconfig_branch_factory cfg_adl_core_parent("libADLMIDI emulator core", guid_cfg_adl_core_parent, GUIDMIDIPlayerBranch, 2.0);

advconfig_checkbox_factory_t<true> cfg_adl_core_nuked("Nuked OPL3 (slowest, most accurate)", guid_cfg_adl_core_nuked, guid_cfg_adl_core_parent, 0.0, false);
advconfig_checkbox_factory_t<true> cfg_adl_core_nuked_174("Nuked OPL3 v0.74 (slow, slightly less accurate)", guid_cfg_adl_core_nuked_174, guid_cfg_adl_core_parent, 1.0, false);
advconfig_checkbox_factory_t<true> CfgADLCoreDOSBox("Dosbox OPL3 (really fast, mostly accurate)", guid_cfg_adl_core_dosbox, guid_cfg_adl_core_parent, 2.0, true);

advconfig_branch_factory cfg_opn_core_parent("libOPNMIDI emulator core", guid_cfg_opn_core_parent, GUIDMIDIPlayerBranch, 3.0);

advconfig_checkbox_factory_t<true> cfg_opn_core_mame("MAME OPN", guid_cfg_opn_core_mame, guid_cfg_opn_core_parent, 0.0, true);
advconfig_checkbox_factory_t<true> cfg_opn_core_nuked("NukedOPN", guid_cfg_opn_core_nuked, guid_cfg_opn_core_parent, 1.0, false);
advconfig_checkbox_factory_t<true> cfg_opn_core_gens("Gens OPN", guid_cfg_opn_core_gens, guid_cfg_opn_core_parent, 2.0, false);

advconfig_branch_factory cfg_opn_bank_parent("libOPNMIDI bank", guid_cfg_opn_bank_parent, GUIDMIDIPlayerBranch, 4.0);

advconfig_checkbox_factory_t<true> cfg_opn_bank_xg("XG", guid_cfg_opn_bank_xg, guid_cfg_opn_bank_parent, 0.0, true);
advconfig_checkbox_factory_t<true> cfg_opn_bank_gs("GS (DMXOPN2)", guid_cfg_opn_bank_gs, guid_cfg_opn_bank_parent, 1.0, false);
advconfig_checkbox_factory_t<true> cfg_opn_bank_gems("GEMS fmlib GM", guid_cfg_opn_bank_gems, guid_cfg_opn_bank_parent, 2.0, false);
advconfig_checkbox_factory_t<true> cfg_opn_bank_tomsoft("Tomsoft's SegaMusic", guid_cfg_opn_bank_tomsoft, guid_cfg_opn_bank_parent, 3.0, false);
advconfig_checkbox_factory_t<true> cfg_opn_bank_fmmidi("FMMIDI original bank", guid_cfg_opn_bank_fmmidi, guid_cfg_opn_bank_parent, 4.0, false);

advconfig_checkbox_factory cfg_SkipToFirstNote("Skip to first note", guid_cfg_skip_to_first_note, GUIDMIDIPlayerBranch, 0, false);

#ifdef BASSMIDISUPPORT
advconfig_branch_factory cfg_bassmidi_parent("BASSMIDI", guid_cfg_bassmidi_parent, GUIDMIDIPlayerBranch, 3.0);

advconfig_checkbox_factory CfgBASSMIDIEffects("Enable reverb and chorus processing", guid_cfg_bassmidi_effects, guid_cfg_bassmidi_parent, 0, true);
advconfig_integer_factory CfgBASSMIDIVoices("Maximum voice count", guid_cfg_bassmidi_voices, guid_cfg_bassmidi_parent, 1, 256, 1, 100000);
#endif

#ifdef FLUIDSYNTHSUPPORT
advconfig_branch_factory cfg_fluidsynth_parent("FluidSynth", guid_cfg_fluidsynth_parent, GUIDMIDIPlayerBranch, 3.0);

advconfig_checkbox_factory cfg_soundfont_dynamic("Load SoundFont samples dynamically", guid_cfg_soundfont_dynamic, guid_cfg_fluidsynth_parent, 0, true);
advconfig_checkbox_factory cfg_fluidsynth_effects("Render reverb and chorus effects", guid_cfg_fluidsynth_effects, guid_cfg_fluidsynth_parent, 1, true);
advconfig_integer_factory cfg_fluidsynth_voices("Maximum voice count", guid_cfg_fluidsynth_voices, guid_cfg_fluidsynth_parent, 2, 256, 1, 65535);
#endif

// Names of the meta data fields
const char TagChannels[] = "channels";
const char TagEncoding[] = "encoding";
const char TagSampleRate[] = "samplerate";

const char TagMIDIHash[] = "midi_hash";
const char TagMIDIFormat[] = "midi_format";
const char TagMIDITrackCount[] = "midi_tracks";
const char TagMIDIChannelCount[] = "midi_channels";
const char TagMIDITicks[] = "midi_ticks";
const char TagMIDIType[] = "midi_type";
const char TagMIDILoopStart[] = "midi_loop_start";
const char TagMIDILoopEnd[] = "midi_loop_end";
const char TagMIDILoopStartInMs[] = "midi_loop_start_ms";
const char TagMIDILoopEndInMs[] = "midi_loop_end_ms";
const char TagMIDIPreset[] = "midi_preset";
const char TagMIDISysExDumps[] = "midi_sysex_dumps";

const char TagBASSMIDIVoiceCount[] = "bassmidi_voices";
const char TagBASSMIDIVoicesMax[] = "bassmidi_voices_max";

const char * _FileExtensions[] =
{
    "MID",
    "MIDI",
    "KAR",
    "RMI",
    "MIDS",
    "MDS",
//  "CMF",
//  "GMF",
    "HMI",
    "HMP",
    "HMQ",
    "MUS",
    "XMI",
    "LDS",
};

const size_t _FileExtensionCount = _countof(_FileExtensions);

const char * _SysExFileExtensions[] =
{
    "SYX",
    "DMP"
};

const size_t _SysExFileExtensionCount = _countof(_SysExFileExtensions);

bool IsMIDIFileExtension(const char * fileExtension)
{
    for (size_t i = 0; i < _FileExtensionCount; ++i)
    {
        if (::_stricmp(fileExtension, _FileExtensions[i]) == 0)
            return true;
    }

    return false;
}

bool IsSysExFileExtension(const char * fileExtension)
{
    for (size_t i = 0; i < _SysExFileExtensionCount; ++i)
    {
        if (::_stricmp(fileExtension, _SysExFileExtensions[i]) == 0)
            return true;
    }

    return false;
}
