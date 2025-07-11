
/** $VER: Configuration.cpp (2025.07.10) **/

#include "pch.h"

#include "Configuration.h"

#include "Resource.h"

const GUID PreferencesPageGUID = { 0x08390b0c, 0x5ba7, 0x4abc, { 0xb5, 0x29, 0x70, 0x79, 0x17, 0x27, 0x12, 0xa4 } };

cfg_var_modern::cfg_int     CfgPlayerType                   ({ 0x1253bac2, 0x9193, 0x420c, { 0xa9, 0x19, 0x9a, 0x1c, 0xf8, 0x70, 0x6e, 0x2c } }, (t_int32) PlayerTypes::Default);
cfg_var_modern::cfg_string  CfgPlugInFilePath               ({ 0x1a6ea7e5, 0x718a, 0x485a, { 0xb1, 0x67, 0xcf, 0xdf, 0x3b, 0x40, 0x61, 0x45 } }, "");   // Path name of the selected plug-in
cfg_var_modern::cfg_int     CfgCLAPIndex                    ({ 0x3519e151, 0x0c6b, 0x4459, { 0xa2, 0x5b, 0x64, 0x79, 0x7d, 0x66, 0x8d, 0x2a } }, 0);    // Index within the plug-in

cfg_var_modern::cfg_int     CfgSampleRate                   ({ 0xae5ba73b, 0xb0d4, 0x4261, { 0xbf, 0xf2, 0x11, 0xa1, 0xc4, 0x4e, 0x57, 0xea } }, DefaultSampleRate);

cfg_var_modern::cfg_int     CfgLoopTypePlayback             ({ 0x460a84b6, 0x910a, 0x496c, { 0xbe, 0xb6, 0x86, 0xfd, 0xeb, 0x41, 0xab, 0xdc } }, DefaultPlaybackLoopType);
cfg_var_modern::cfg_int     CfgLoopTypeOther                ({ 0xab5cc279, 0x1c68, 0x4824, { 0xb4, 0xb8, 0x06, 0x56, 0x85, 0x6a, 0x40, 0xa0 } }, DefaultOtherLoopType);

cfg_var_modern::cfg_int     CfgDecayTime                    ({ 0xee80a18d, 0x7327, 0x4ba4, { 0x9c, 0x20, 0xae, 0xe3, 0xea, 0x42, 0xd6, 0xb5 } }, DefaultDecayTime);
cfg_var_modern::cfg_int     CfgLoopCount                    ({ 0x6d23cdfc, 0x2e0a, 0x466d, { 0x9d, 0x09, 0x8a, 0xe5, 0x2b, 0x90, 0x4d, 0xe6 } }, DefaultLoopCount);
cfg_var_modern::cfg_int     CfgFadeOutTime                  ({ 0xdcd88167, 0x4d45, 0x4d72, { 0xa8, 0x54, 0xbe, 0x3f, 0x0a, 0xed, 0x00, 0x86 } }, DefaultFadeOutTime);

cfg_var_modern::cfg_int     CfgMIDIFlavor                   ({ 0x1a82a8db, 0x389e, 0x44aa, { 0x97, 0x19, 0x32, 0x6a, 0x5a, 0x2d, 0x7e, 0x8e } }, DefaultMIDIFlavor);
cfg_var_modern::cfg_int     CfgUseMIDIEffects               ({ 0x091c12a1, 0xd42b, 0x4f4e, { 0x80, 0x58, 0x8b, 0x7f, 0x4c, 0x4d, 0xf3, 0xa1 } }, DefaultUseMIDIEffects);
cfg_var_modern::cfg_int     CfgUseMT32EmuWithMT32           ({ 0xb91b9d1c, 0x17ea, 0x4e72, { 0xbb, 0x4c, 0x74, 0xdf, 0xae, 0x31, 0x2e, 0x92 } }, DefaultUseMT32EmuWithMT32);
cfg_var_modern::cfg_int     CfgUseVSTiWithXG                ({ 0xea9f43b2, 0xbe58, 0x4494, { 0x8f, 0xa0, 0x29, 0x50, 0x4b, 0x30, 0x6b, 0xaa } }, DefaultUseVSTiWithXG);
cfg_var_modern::cfg_bool    CfgDetectExtraDrum              ({ 0xcbc913c5, 0xa914, 0x4977, { 0x9e, 0xe1, 0x8e, 0x78, 0x92, 0xd9, 0xa5, 0xef } }, DefaultDetectExtraDrum);

cfg_var_modern::cfg_int     CfgDetectTouhouLoops            ({ 0x35a32d9e, 0xdf99, 0x4617, { 0x9f, 0xf3, 0x31, 0x2f, 0x9c, 0x20, 0x6e, 0x00 } }, DefaultDetectTouhouLoops);
cfg_var_modern::cfg_int     CfgDetectRPGMakerLoops          ({ 0x4d11dd87, 0x7a27, 0x4ecc, { 0xbc, 0xb8, 0xf0, 0x18, 0x02, 0x0b, 0xa2, 0xd5 } }, DefaultDetectRPGMakerLoops);
cfg_var_modern::cfg_int     CfgDetectLeapFrogLoops          ({ 0xa8875345, 0x2be4, 0x44f1, { 0x99, 0x70, 0x4c, 0x3f, 0x23, 0xb7, 0x8a, 0x45 } }, DefaultDetectLeapFrogLoops);
cfg_var_modern::cfg_int     CfgDetectXMILoops               ({ 0x0f580d09, 0xd57b, 0x450c, { 0x84, 0xa2, 0xd6, 0x0e, 0x34, 0xbd, 0x64, 0xf5 } }, DefaultDetectXMILoops);
cfg_var_modern::cfg_int     CfgDetectFF7Loops               ({ 0x2e0dbdc2, 0x7436, 0x4b70, { 0x91, 0xfc, 0xfd, 0x98, 0x37, 0x87, 0x32, 0xb2 } }, DefaultDetectFF7Loops);

cfg_var_modern::cfg_int     CfgExcludeEMIDITrackDesignation ({ 0xc090f9c7, 0x47f9, 0x4f6f, { 0x84, 0x7a, 0x27, 0xcd, 0x75, 0x96, 0xc9, 0xd4 } }, DefaultEmuDeMIDIExclusion);

cfg_var_modern::cfg_int     CfgFluidSynthInterpolationMode  ({ 0xa395c6fd, 0x492a, 0x401b, { 0x8b, 0xdb, 0x9d, 0xf5, 0x3e, 0x2e, 0xf7, 0xcf } }, DefaultFluidSynthInterpolationMethod);

cfg_var_modern::cfg_int     CfgFilterInstruments            ({ 0x6d30c919, 0xb053, 0x43aa, { 0x9f, 0x1b, 0x1d, 0x40, 0x18, 0x82, 0x80, 0x5e } }, DefaultFilterInstruments);
cfg_var_modern::cfg_int     CfgFilterBanks                  ({ 0x3145963c, 0x7322, 0x4b48, { 0x99, 0xff, 0x75, 0xea, 0xc5, 0xf4, 0xda, 0xcc } }, DefaultFilterBanks);
cfg_var_modern::cfg_bool    CfgSkipToFirstNote              ({ 0x6d2364cc, 0x9bfd, 0x489d, { 0xbc, 0xe8, 0xa0, 0x07, 0xdf, 0x6e, 0x90, 0xe1 } }, DefaultSkipToFirstNote);

// MT32Emu
cfg_var_modern::cfg_int     CfgMT32EmuConversionQuality     ({ 0xd94f3792, 0xb2c6, 0x45e3, { 0xab, 0xa2, 0x1a, 0xd3, 0xb1, 0x6c, 0xac, 0x4b } }, DefaultMT32EmuConversionQuality);
cfg_var_modern::cfg_int     CfgMT32EmuMaxPartials           ({ 0x96ba8540, 0x8c5d, 0x41a7, { 0xbc, 0x73, 0x93, 0xec, 0xc9, 0xcf, 0xc0, 0xb4 } }, DefaultMT32EmuMaxPartials);
cfg_var_modern::cfg_int     CfgMT32EmuAnalogOutputMode      ({ 0xe41fc2b1, 0x2ecb, 0x425b, { 0x8c, 0x48, 0x60, 0x01, 0xbc, 0x7e, 0x5d, 0x88 } }, DefaultMT32EmuAnalogOutputMode);
cfg_var_modern::cfg_int     CfgMT32EmuGMSet                 ({ 0x07257ac7, 0x9901, 0x4a5f, { 0x9d, 0x8b, 0xc5, 0xb5, 0xf1, 0xb8, 0xcf, 0x5b } }, DefaultMT32EmuGMSet);
cfg_var_modern::cfg_int     CfgMT32EmuDACInputMode          ({ 0x3d00615d, 0xe8a5, 0x4081, { 0x81, 0x8d, 0xa6, 0xa1, 0x00, 0x34, 0x66, 0xcf } }, DefaultMT32EmuDACInputMode);

cfg_var_modern::cfg_bool    CfgMT32EmuReverb                ({ 0xebbbc05c, 0xf4a7, 0x4bdb, { 0x9b, 0xc2, 0xa8, 0xf8, 0xac, 0x00, 0xca, 0x00 } }, DefaultMT32EmuReverb);

cfg_var_modern::cfg_bool    CfgMT32EmuNiceAmpRamp           ({ 0x945ff616, 0xedb5, 0x4f53, { 0xbb, 0xa4, 0x56, 0x5b, 0x76, 0x9c, 0xdc, 0x5d } }, DefaultMT32EmuNiceAmpRamp);
cfg_var_modern::cfg_bool    CfgMT32EmuNicePanning           ({ 0x4d2e4cdf, 0xcd36, 0x4cbd, { 0x85, 0x97, 0xa1, 0x1d, 0xf7, 0x5a, 0x07, 0x5c } }, DefaultMT32EmuNicePanning);
cfg_var_modern::cfg_bool    CfgMT32EmuNicePartialMixing     ({ 0x3565657b, 0xd5e1, 0x46a3, { 0xb1, 0x53, 0xf9, 0x65, 0xc9, 0xe6, 0x05, 0xda } }, DefaultMT32EmuNicePartialMixing);
cfg_var_modern::cfg_bool    CfgMT32EmuReverseStereo         ({ 0x88c5f0c7, 0x3776, 0x4fee, { 0x90, 0x1c, 0x43, 0x15, 0xe4, 0x7b, 0x99, 0x27 } }, DefaultMT32EmuReverseStereo);

// LibADLEmu
cfg_var_modern::cfg_int     CfgADLBank                      ({ 0xa62a00a7, 0x0dbf, 0x4475, { 0xbe, 0xca, 0xed, 0xbf, 0x5d, 0x06, 0x4a, 0x80 } }, DefaultADLBank);
cfg_var_modern::cfg_int     CfgADLEmulator                  ({ 0xe8780aaf, 0x7307, 0x4ff2, { 0xb3, 0x95, 0x5f, 0x25, 0xa5, 0x19, 0x6e, 0xab } }, DefaultADLEmulator);
cfg_var_modern::cfg_int     CfgADLChipCount                 ({ 0x974365ed, 0xd4f9, 0x4daa, { 0xb4, 0x89, 0xad, 0x7a, 0xd2, 0x91, 0xfa, 0x94 } }, DefaultADLChipCount);
cfg_var_modern::cfg_bool    CfgADLSoftPanning               ({ 0xad6821b4, 0x493f, 0x4bb3, { 0xb7, 0xbb, 0xe0, 0xa6, 0x7c, 0x5d, 0x59, 0x07 } }, DefaultADLSoftPanning);

// LibOPNEmu
cfg_var_modern::cfg_int     CfgOPNBank                      ({ 0xcde9a6a7, 0x8970, 0x4656, { 0x8f, 0x0e, 0x12, 0x79, 0x85, 0xf9, 0xdb, 0x51 } }, DefaultOPNBank);
cfg_var_modern::cfg_int     CfgOPNEmulator                  ({ 0xe303d4bd, 0xf1d9, 0x458f, { 0x98, 0xb9, 0xa0, 0x99, 0xe4, 0xea, 0xdd, 0x19 } }, DefaultOPNEmulator);
cfg_var_modern::cfg_int     CfgOPNChipCount                 ({ 0x9140a1ac, 0x71fb, 0x4e4e, { 0xb0, 0x3d, 0xa9, 0xc7, 0xd2, 0xd5, 0x5c, 0xed } }, DefaultOPNChipCount);
cfg_var_modern::cfg_bool    CfgOPNSoftPanning               ({ 0x60284059, 0x2946, 0x4ec9, { 0x93, 0x2d, 0xd3, 0x24, 0x88, 0x4f, 0x36, 0xb1 } }, DefaultOPNSoftPanning);

// Nuked OPL3
cfg_var_modern::cfg_int     CfgNukeSynthesizer              ({ 0x7423a720, 0xeb39, 0x4d7d, { 0x9b, 0x85, 0x52, 0x4b, 0xc7, 0x79, 0xb5, 0x8b } }, DefaultNukeSynth);
cfg_var_modern::cfg_int     CfgNukeBank                     ({ 0xa91d31f4, 0x22ae, 0x4c5c, { 0xa6, 0x21, 0xf6, 0xb6, 0x01, 0x1f, 0x5d, 0xdc } }, DefaultNukeBank);
cfg_var_modern::cfg_int     CfgNukePanning                  ({ 0x849c5c09, 0x520a, 0x4d62, { 0xa6, 0xd1, 0xe8, 0xb4, 0x32, 0x66, 0x49, 0x48 } }, DefaultNukePanning);

cfg_var_modern::cfg_float   CfgBASSMIDIVolume               ({ 0x143e8051, 0xa42b, 0x4225, { 0xb9, 0xd2, 0x79, 0xf1, 0x43, 0x1e, 0x70, 0x16 } }, DefaultBASSMIDIVolume);
cfg_var_modern::cfg_int     CfgBASSMIDIResamplingMode       ({ 0xf9ddd2c0, 0xd8fd, 0x442f, { 0x9e, 0x49, 0xd9, 0x01, 0xb5, 0x1d, 0x6d, 0x38 } }, DefaultBASSMIDIResamplingMode);

cfg_var_modern::cfg_string  CfgVSTiPlugInDirectoryPath      ({ 0xaaa1bade, 0xafe0, 0x4c16, { 0xbf, 0x90, 0xfa, 0x1c, 0x6f, 0x14, 0x03, 0x1c } }, "");
cfg_var_modern::cfg_string  CfgVSTiXGPlugInFilePath         ({ 0x433415d3, 0x7b49, 0x4dac, { 0xad, 0xa1, 0xd4, 0x29, 0x5f, 0xba, 0x12, 0x09 } }, "");
cfg_var_modern::cfg_string  CfgSecretSauceDirectoryPath     ({ 0xa75aa244, 0xcbd9, 0x4b57, { 0xba, 0x05, 0xc2, 0xc2, 0x76, 0xda, 0x4f, 0x77 } }, "");
cfg_var_modern::cfg_string  CfgCLAPPlugInDirectoryPath      ({ 0x9b150e32, 0x7e9e, 0x41e3, { 0x84, 0x89, 0x52, 0x3c, 0xaf, 0xc2, 0x32, 0xc9 } }, "");
cfg_var_modern::cfg_string  CfgSoundFontFilePath            ({ 0x696d12dd, 0xaf32, 0x43d9, { 0x8d, 0xf6, 0xbd, 0xd1, 0x1e, 0x81, 0x83, 0x29 } }, "");
cfg_var_modern::cfg_string  CfgMT32ROMDirectoryPath         ({ 0xd7e0ec5e, 0x872f, 0x41e3, { 0x9b, 0x5b, 0xd2, 0x02, 0xd8, 0xb9, 0x42, 0xa7 } }, "");
cfg_var_modern::cfg_string  CfgFluidSynthDirectoryPath      ({ 0x0774129c, 0x3baf, 0x471e, { 0xa3, 0xdd, 0x83, 0x78, 0xc4, 0x2c, 0x1a, 0x4e } }, "");
cfg_var_modern::cfg_string  CfgFluidSynthConfigFilePath     ({ 0xf5940e8a, 0x7f92, 0x4007, { 0xa0, 0x78, 0x02, 0xad, 0x9e, 0x5c, 0xf8, 0x95 } }, "");
cfg_var_modern::cfg_string  CfgProgramsFilePath             ({ 0xf329ea7d, 0x51bf, 0x447b, { 0xab, 0xa3, 0x07, 0xb7, 0x71, 0x2c, 0x9d, 0x7b } }, "");
cfg_var_modern::cfg_string  CfgADLBankFilePath              ({ 0xd5b8c8ed, 0x2034, 0x4783, { 0x84, 0xe1, 0x60, 0x38, 0x6a, 0x91, 0x32, 0x82 } }, "");
cfg_var_modern::cfg_string  CfgOPNBankFilePath              ({ 0x89338e5b, 0xf917, 0x4bec, { 0xbf, 0x47, 0x8c, 0x20, 0xaf, 0xbb, 0xa8, 0x76 } }, "");

cfg_map CfgVSTiConfig({ 0x44e7c715, 0xd256, 0x44c4, { 0x8f, 0xb6, 0xb7, 0x20, 0xfa, 0x9b, 0x31, 0xfc } });

cfg_var_modern::cfg_string  CfgPlugInName                   ({ 0xf6b1bdff, 0x5d92, 0x4bee, { 0xb6, 0x5f, 0xec, 0x0f, 0x45, 0xe8, 0xd9, 0x39 } }, "");

#pragma region Advanced Configuration GUIDs

static const GUID AdvCfgMIDIPlayerBranchGUID = { 0x66524470, 0x7ec7, 0x445e, { 0xa6, 0xfd, 0xc0, 0xfb, 0xae, 0x74, 0xe5, 0xfc } };

    static const GUID AdvCfgBASSMIDIEffectsEnabledGUID = { 0x62bf901b, 0x9c51, 0x45fe, { 0xbe, 0x8a, 0x14, 0xfb, 0x56, 0x20, 0x5e, 0x5e } };

    static const GUID AdvCfgFluidSynthBranchGUID = { 0xf1ad51c5, 0x4b04, 0x4c8b, { 0x84, 0x65, 0x6c, 0x86, 0x1e, 0x81, 0xc6, 0x69 } };
        static const GUID AdvCfgFluidSynthEnableEffectsEnabledGUID = { 0x996e95ca, 0xce4d, 0x4bd5, { 0xb7, 0xe6, 0x40, 0x61, 0x32, 0x83, 0xc3, 0x27 } };
        static const GUID AdvCfgFluidSynthVoiceCountGUID = { 0x9114d64d, 0x412c, 0x42d3, { 0xae, 0xd5, 0xa5, 0x52, 0x1e, 0x8f, 0xe2, 0xa6 } };
        static const GUID AdvCfgFluidSynthLoadSoundFontDynamicallyGUID = { 0x4c455226, 0xb107, 0x4e04, { 0xa9, 0xec, 0xf8, 0x9, 0x8f, 0x81, 0xe2, 0x96 } };

    static const GUID AdvCfgBASSMIDIBranchGUID = { 0xdd5adceb, 0x9b31, 0x47b6, { 0xaf, 0x57, 0x3b, 0x15, 0xd2, 0x2, 0x5d, 0x9f } };
        static const GUID AdvCfgBASSMIDIVoicesGUID = { 0x9e0a5dab, 0x6786, 0x4120, { 0xb7, 0x37, 0x85, 0xbb, 0x2d, 0xfa, 0xf3, 0x7 } };

#pragma endregion

advconfig_branch_factory AdvCfgMIDIPlayerBranch(STR_COMPONENT_NAME, AdvCfgMIDIPlayerBranchGUID, advconfig_branch::guid_branch_playback, 0);

    advconfig_branch_factory AdvCfgFluidSynthBranch("FluidSynth", AdvCfgFluidSynthBranchGUID, AdvCfgMIDIPlayerBranchGUID, 5.0);

        advconfig_integer_factory AdvCfgFluidSynthVoices         ("Maximum voice count",                 AdvCfgFluidSynthVoiceCountGUID,               AdvCfgFluidSynthBranchGUID, 1.0, 256, 1, 65535);
        advconfig_checkbox_factory AdvCfgFluidSynthEffectsEnabled("Enable reverb and chorus processing", AdvCfgFluidSynthEnableEffectsEnabledGUID,     AdvCfgFluidSynthBranchGUID, 2.0, true);
        advconfig_checkbox_factory AdvCfgLoadSoundFontDynamically("Load SoundFont samples dynamically",  AdvCfgFluidSynthLoadSoundFontDynamicallyGUID, AdvCfgFluidSynthBranchGUID, 3.0, true);

    advconfig_branch_factory AdvBASSMIDIBranch("BASSMIDI", AdvCfgBASSMIDIBranchGUID, AdvCfgMIDIPlayerBranchGUID, 6.0);

        advconfig_integer_factory AdvCfgBASSMIDIVoices         ("Maximum voice count",                 AdvCfgBASSMIDIVoicesGUID,         AdvCfgBASSMIDIBranchGUID,  1.0, 256, 1, 100000);
        advconfig_checkbox_factory AdvCfgBASSMIDIEffectsEnabled("Enable reverb and chorus processing", AdvCfgBASSMIDIEffectsEnabledGUID, AdvCfgBASSMIDIBranchGUID, 2.0, true);

// Names of the meta data fields
const char TagPreset[]                      = "midi_preset";
const char TagMIDISysExDumps[]              = "midi_sysex_dumps";

// Names of the info fields
const char TagMIDIFormat[]                  = "midi_format";
const char TagMIDITrackCount[]              = "midi_tracks";
const char TagMIDIChannelCount[]            = "midi_channels";
const char TagMIDITicks[]                   = "midi_ticks";
const char TagMIDIType[]                    = "midi_type";
const char TagMIDILoopStart[]               = "midi_loop_start";
const char TagMIDILoopEnd[]                 = "midi_loop_end";
const char TagMIDILoopStartInMs[]           = "midi_loop_start_ms";
const char TagMIDILoopEndInMs[]             = "midi_loop_end_ms";
const char TagMIDILyricsType[]              = "midi_lyrics_type";
const char TagMIDIHash[]                    = "midi_hash";
const char TagMIDIEmbeddedSoundFont[]       = "midi_embedded_soundfont";

// Names of the dynamic info fields
const char InfoSampleRate[]                 = "samplerate";
const char InfoMIDIActiveVoices[]           = "midi_active_voices";
const char InfoMIDIPeakVoices[]             = "midi_peak_voices";

const char InfoMIDIPlayer[]                 = "midi_player";
const char InfoMIDIPlayerExt[]              = "midi_player_ext";

const char InfoMIDIExtraPercusionChannel[]  = "midi_extra_percussion_channel";

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

    // Recomposer
    "RCP",
    "R36",
    "G18",
    "G36",

    "XMF", "MXMF",  // Extensible Music Format and Mobile Extensible Music Format

    "MMF",          // Mobile Music File / Synthetic-music Mobile Application Format (SMAF) (https://docs.fileformat.com/audio/mmf/) () **/

#ifdef _DEBUG
    "TST",
#endif
};

const size_t _FileExtensionCount = _countof(_FileExtensions);

const char * _SysExFileExtensions[] =
{
    "SYX",
    "DMP"
};

const size_t _SysExFileExtensionCount = _countof(_SysExFileExtensions);

/// <summary>
/// Returns true if the specified file extension is recognized as a MIDI file.
/// </summary>
bool IsMIDIFileExtension(const char * fileExtension)
{
    for (size_t i = 0; i < _FileExtensionCount; ++i)
    {
        if (::_stricmp(fileExtension, _FileExtensions[i]) == 0)
            return true;
    }

    return false;
}

/// <summary>
/// Returns true if the specified file extension is recognized as a SysEx file.
/// </summary>
bool IsSysExFileExtension(const char * fileExtension)
{
    for (size_t i = 0; i < _SysExFileExtensionCount; ++i)
    {
        if (::_stricmp(fileExtension, _SysExFileExtensions[i]) == 0)
            return true;
    }

    return false;
}
