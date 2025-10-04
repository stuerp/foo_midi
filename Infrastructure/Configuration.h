
/** $VER: Configuration.h (2025.10.04) **/

#pragma once

#include <sdk/foobar2000-lite.h>

#include <sdk/cfg_var.h>
#include <sdk/advconfig_impl.h>

#include "ConfigurationMap.h"

#include <fluidsynth.h>
#include <mt32emu.h>

using namespace cfg_var_modern;

extern const GUID PreferencesPageGUID;

enum class PlayerType : int8_t
{
    Unknown = -1,

    Min = 0,

    EmuDeMIDI = Min,
    VSTi = 1,
    FluidSynth = 2,
    MT32Emu = 3,
    BASSMIDI = 4,
    DirectX = 5,
    ADL = 6,
    OPN = 7,
    OPL = 8,
    NukedOPL3 = 9,
    SecretSauce = 10,
    MCI = 11,
    NukedSC55 = 12,
    FMMIDI = 13,
    CLAP = 14,

    Max = CLAP,

    Default = ADL,
};

enum class LoopType
{
    NeverLoop = 0,                      // Never loop
    NeverLoopAddDecayTime = 1,          // Never loop. Add configured decay time at the end

    LoopWhenDetectedAndFade = 2,        // Loop when detected and fade at the end
    RepeatAndFade = 3,                  // Repeat the complete song and fade at the end

    LoopWhenDetectedForever = 4,        // Loop when detected forever
    RepeatForever = 5,                  // Repeat the complete song forever
};

enum class MIDIFlavor
{
    Default = 0,                        // Defaults to SC88.

    GM,
    GM2,

    SC55,
    SC88,
    SC88Pro,
    SC8820,

    XG,

    None
};

enum
{
    DefaultSampleRate = 44100,
    DefaultPlaybackLoopType = 0,
    DefaultOtherLoopType = 0,

    DefaultDecayTime = 1000,
    DefaultLoopCount = 2,
    DefaultFadeOutTime = 5000,

    DefaultDetectTouhouLoops = 1,
    DefaultDetectRPGMakerLoops = 1,
    DefaultDetectLeapFrogLoops = 0,
    DefaultDetectXMILoops = 1,
    DefaultDetectFF7Loops = 1,

    DefaultMIDIFlavor = MIDIFlavor::Default,
    DefaultUseMIDIEffects = 1,
    DefaultUseMT32EmuWithMT32 = 1,
    DefaultUseSCWithGS = false,
    DefaultUseVSTiWithXG = 0,
    DefaultDetectExtraDrum = 1,

    DefaultEmuDeMIDIExclusion = 1,
    DefaultFilterInstruments = 0,
    DefaultFilterBanks = 0,
    DefaultSkipToFirstNote = false,

    DefaultBASSMIDIMaxVoices = 256,
    DefaultBASSMIDIProcessEffects = true,
    DefaultBASSMIDIUseDLS = true,

    DefaultFluidSynthInterpolationMethod = FLUID_INTERP_DEFAULT,
    DefaultFluidSynthMaxVoices = 256,
    DefaultFluidSynthProcessEffects = true,
    DefaultFluidSynthDynSampleLoading = false,
    DefaultFluidSynthUseDLS = false,

    DefaultBASSMIDIResamplingMode = 1,

    DefaultADLBank = 72,
    DefaultADLEmulator = 0,
    DefaultADLChipCount = 10,
    DefaultADLSoftPanning = true,

    DefaultOPNBank = 0,
    DefaultOPNEmulator = 0,
    DefaultOPNChipCount = 10,
    DefaultOPNSoftPanning = true,

    // MT32Emu
    DefaultMT32EmuConversionQuality = MT32Emu::SamplerateConversionQuality::SamplerateConversionQuality_BEST,
    DefaultMT32EmuMaxPartials = 256,
    DefaultMT32EmuAnalogOutputMode = MT32Emu::AnalogOutputMode::AnalogOutputMode_OVERSAMPLED,
    DefaultMT32EmuGMSet = 0,
    DefaultMT32EmuDACInputMode = MT32Emu::DACInputMode_NICE,

    DefaultMT32EmuReverb = true,
    DefaultMT32EmuNiceAmpRamp = false,
    DefaultMT32EmuNicePanning = true,
    DefaultMT32EmuNicePartialMixing = true,
    DefaultMT32EmuReverseStereo = true,

    DefaultNukeSynth = 0,
    DefaultNukeBank = 2,
    DefaultNukePanning = 0,
};

const float DefaultBASSMIDIGain = 0.15f;

const float DefaultSecretSauceGain = 0.0f;

const uint16_t DefaultEnabledChannels = 0xFFFF;

extern cfg_var_modern::cfg_int      CfgPlayerType;
extern cfg_var_modern::cfg_string   CfgPlugInFilePath;
extern cfg_var_modern::cfg_int      CfgCLAPIndex;

extern cfg_var_modern::cfg_int
    CfgSampleRate,

    CfgLoopTypePlayback,
    CfgLoopTypeOther,

    CfgDecayTime,
    CfgLoopCount,
    CfgFadeTime,

    CfgDetectTouhouLoops,
    CfgDetectRPGMakerLoops,
    CfgDetectLeapFrogLoops,
    CfgDetectXMILoops,
    CfgDetectFF7Loops,

    CfgExcludeEMIDITrackDesignation,

    CfgFilterInstruments,
    CfgFilterBanks,

    CfgBASSMIDIResamplingMode,
    CfgBASSMIDIMaxVoices,

    CfgFluidSynthInterpolationMode,
    CfgFluidSynthMaxVoices,

    CfgADLBank,
    CfgADLEmulator,
    CfgADLChipCount,

    CfgOPNBank,
    CfgOPNEmulator,
    CfgOPNChipCount,

    CfgMT32EmuConversionQuality,
    CfgMT32EmuMaxPartials,
    CfgMT32EmuAnalogOutputMode,
    CfgMT32EmuGMSet,
    CfgMT32EmuDACInputMode,

    CfgNukeSynthesizer,
    CfgNukeBank,
    CfgNukePanning,

    CfgMIDIFlavor,
    CfgUseMIDIEffects,
    CfgUseMT32EmuWithMT32,
    CfgUseVSTiWithXG;

extern cfg_var_modern::cfg_bool
    CfgUseSCWithGS,

    CfgWriteBarMarkers,
    CfgWriteSysExNames,
    CfgExtendLoops,
    CfgWolfteamLoopMode,
    CfgKeepMutedChannels,
    CfgIncludeControlData,
    CfgDetectExtraDrum,

    CfgADLSoftPanning,

    CfgOPNSoftPanning,

    CfgMT32EmuReverb,
    CfgMT32EmuNiceAmpRamp,
    CfgMT32EmuNicePanning,
    CfgMT32EmuNicePartialMixing,
    CfgMT32EmuReverseStereo,

    CfgSkipToFirstNote,

    CfgBASSMIDIProcessEffects,
    CfgBASSMIDIUseDLS,

    CfgFluidSynthProcessEffects,
    CfgFluidSynthDynSampleLoading,
    CfgFluidSynthUseDLS;

extern cfg_var_modern::cfg_float
    CfgBASSMIDIGain,
    CfgSecretSauceGain;

extern cfg_var_modern::cfg_string
    CfgVSTiPlugInDirectoryPath,
    CfgVSTiXGPlugInFilePath,
    CfgSecretSauceDirectoryPath,
    CfgCLAPPlugInDirectoryPath,
    CfgSoundFontFilePath,
    CfgMT32ROMDirectoryPath,
    CfgFluidSynthDirectoryPath,
    CfgFluidSynthConfigFilePath,
    CfgProgramsFilePath,
    CfgADLBankFilePath,
    CfgOPNBankFilePath;

extern cfg_var_modern::cfg_string
    CfgPlugInName;

extern cfg_vsti_map      CfgVSTiConfig;
extern cfg_clap_map CfgCLAPConfig;

// RCP
extern cfg_var_modern::cfg_int  CfgLoopExpansion;

// HMI
extern cfg_var_modern::cfg_int  CfgDefaultTempo;

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
extern const char TagPreset[];
extern const char TagMIDISysExDumps[];
extern const char TagMIDILyricsType[];
extern const char TagMIDIHash[];
extern const char TagMIDIEmbeddedSoundfont[];

extern const char InfoSampleRate[];
extern const char InfoMIDIActiveVoices[];
extern const char InfoMIDIPeakVoices[];

extern const char InfoMIDIPlayer[];
extern const char InfoMIDIPlayerExt[];

extern const char InfoMIDIExtraPercusionChannel[];

extern bool IsMIDIFileExtension(const char * fileExtension);
extern bool IsSysExFileExtension(const char * ext);
